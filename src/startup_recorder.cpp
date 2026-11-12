#include "startup_recorder.hpp"
#include "pcinsight/monitor.hpp"
#include "linux_process_details.hpp"
#include "linux_network_self.hpp"
#include "linux_startup_resources.hpp"
#include <algorithm>
#include <limits>
#include <mutex>
#include <map>
#include <stdexcept>
#ifdef __linux__
#include <cerrno>
#include <sys/syscall.h>
#include <unistd.h>
#endif
namespace pcinsight {
namespace {
void add(std::uint64_t& target,std::uint64_t amount){if(amount>UINT64_MAX-target)throw std::overflow_error("recorder diagnostic overflow");target+=amount;}
template<class T> bool same_birth_metric(const Metric<T>& a,const Metric<T>& b){return a.value==b.value&&a.support==b.support&&a.note==b.note;}
}
struct LinuxStartupRecorder::Impl {
    mutable std::mutex mutex;
    detail::StartupRecorderApi api;
    std::unique_ptr<Monitor> monitor; // Lazy: construction/denied start never reads a native backend.
    std::optional<detail::LinuxNetworkSelfIdentity> native_owner;
    std::optional<std::uint32_t> native_euid;
    std::unique_ptr<StartupImpactObserver> observer;
    StartupRecorderOptions options;
    StartupRecorderReport status;
    std::map<std::pair<std::uint32_t,std::uint64_t>,StartupCpuCounter> registered;
    struct ExitDisk {detail::StartupExitDiskReader reader;StartupProcessSample last;};
    std::map<std::pair<std::uint32_t,std::uint64_t>,ExitDisk> exits;
    std::optional<detail::StartupRecorderBirthSource> birth;
    std::string birth_capture_id;StartupBirthObserverOptions birth_options;bool birth_stopped{};
    std::optional<detail::StartupRecorderManualSource> manual;
    detail::StartupRecorderManualOptions manual_options;
    std::optional<detail::LinuxManualLaunchArchive> manual_saved;
    std::string manual_entry;
    bool manual_mode{},manual_stopped{};
    std::uint32_t manual_pid{};ActionError manual_error{ActionError::none};
    std::chrono::steady_clock::time_point last{};
    explicit Impl(detail::StartupRecorderApi source):api(std::move(source)){}
    Impl(){
        api.context=[this]{
#ifdef __linux__
            const auto uid=static_cast<std::uint32_t>(::geteuid());
            const auto self=detail::read_linux_network_self();
            if(!self.value||self.support!=Support::available||uid!=static_cast<std::uint32_t>(::geteuid()))return StartupProcessSampleResult{{},ActionError::stale_target,"Recorder caller context unavailable/changed"};
            if((native_owner&&*native_owner!=*self.value)||(native_euid&&*native_euid!=uid))return StartupProcessSampleResult{{},ActionError::stale_target,"Recorder caller/fork/UID/namespace changed"};
            native_owner=*self.value;native_euid=uid;
            const auto id=detail::read_linux_process_identity("/proc/self");
            if(id.value&&id.support==Support::available)return read_startup_process_sample(static_cast<std::uint32_t>(::getpid()),id.value->start_ticks);
            return StartupProcessSampleResult{{},ActionError::not_supported,"Native recorder context unavailable"};
#else
            return StartupProcessSampleResult{{},ActionError::not_supported,"Linux recorder requires Linux"};
#endif
        };
        api.snapshot=[this]{if(!monitor)monitor=std::make_unique<Monitor>();SampleOptions o;
            o.include_disks=o.include_networks=o.include_gpus=o.include_disk_media=false;
            o.include_linux_process_descriptions=false;o.include_startup_entries=true;
            return monitor->sample(o);};
        api.candidates=discover_startup_process_candidates;api.sample=read_startup_process_sample;
        api.now=[]{return std::chrono::steady_clock::now();};api.wall=[]{return std::chrono::system_clock::now();};
        api.exit_disk=detail::make_native_startup_exit_disk_reader;
        api.scheduler_sample=read_startup_scheduler_process_sample;
        api.birth=detail::make_native_startup_recorder_birth_source;
        api.manual=detail::make_native_startup_recorder_manual_source;
    }
    ~Impl(){stop_manual();stop_births();}
    void stop_manual() noexcept {if(manual&&!manual_stopped){manual_stopped=true;try{manual->close();}catch(...){}}}
    void stop_births() noexcept {
        if(birth&&!birth_stopped){birth_stopped=true;try{birth->close();}catch(...){}
            if(status.birth_capture&&status.birth_capture->state==StartupBirthObserverState::recording)status.birth_capture->state=StartupBirthObserverState::closed;}
    }
    ActionResult fail(ActionError error){if(observer)observer->close();exits.clear();stop_births();stop_manual();status.state=StartupRecorderState::failed;status.error=error;return {error,"Startup recorder failed; confirmed observations retained, no successful archive save"};}
    ActionResult finish_manual(){
        verify_context();
        if(api.now()<last)return fail(ActionError::stale_target);
        const auto captured=manual->finish(manual_options.window,manual_options.policy);stop_manual();
        if(!captured.value||captured.support!=Support::available)return fail(ActionError::system_error);
        const auto& source=*captured.value;
        if(source.births.capture_id!=manual_options.capture_id)return fail(ActionError::stale_target);
        if(source.receipt.value){const auto& r=*source.receipt.value;
            if(r.route.entry_id!=manual_entry||r.caller_owned_pid!=manual_pid||r.launch_error!=manual_error||(native_owner&&r.selected.local.observer!=*native_owner))return fail(ActionError::stale_target);
        }
        detail::LinuxManualLaunchArchive archive{{source.births,manual_options.window,manual_options.policy},source.receipt};
        const auto encoded=detail::encode_linux_manual_launch(archive);if(!encoded.value)return fail(ActionError::invalid_argument);
        auto canonical=detail::decode_linux_manual_launch(*encoded.value);if(!canonical)return fail(ActionError::invalid_argument);
        std::optional<StartupBirthObserverReport> raw{canonical.archive->births.capture};
        const auto now=api.now();const auto wall=api.wall();
        if(now<last||now<status.started_at||wall.time_since_epoch().count()<=0)return fail(ActionError::stale_target);
        verify_context();manual_saved.swap(canonical.archive);status.birth_capture.swap(raw);
        status.finished_at=last=now;status.recorded_at=wall;status.state=StartupRecorderState::closed;return {};
    }
    void snapshot_births(){
        auto r=birth->report();std::vector<StartupBirthImpactMeasurement> projected;
        if(r.capture_id!=birth_capture_id||!(r.boot[0]||r.boot[1])||!r.pid_namespace_device||!r.pid_namespace_inode||
           r.observations.size()>birth_options.capacity||r.publishing>birth_options.capacity||r.quality_flags&~127ULL||
           r.state==StartupBirthObserverState::idle||static_cast<unsigned>(r.state)>static_cast<unsigned>(StartupBirthObserverState::failed)||r.automatic_startup_origin.value||
           r.final_resources_enabled!=birth_options.recover_final_resources||r.first_exec_enabled!=birth_options.observe_first_exec)
            throw std::invalid_argument("Invalid owned birth capture report");
        if(status.birth_capture){const auto& before=*status.birth_capture;
            if(r.boot!=before.boot||r.pid_namespace_device!=before.pid_namespace_device||r.pid_namespace_inode!=before.pid_namespace_inode||
               (r.quality_flags&before.quality_flags)!=before.quality_flags||(before.creator_exited&&!r.creator_exited)||(before.pid_mapping_enabled&&!r.pid_mapping_enabled))throw std::invalid_argument("Owned birth context/quality changed");
            for(const auto& old:before.observations){const auto found=std::find_if(r.observations.begin(),r.observations.end(),[&](const auto& row){return row.key==old.key;});
                if(found==r.observations.end()||found->birth_monotonic_ns!=old.birth_monotonic_ns||!same_birth_metric(found->local_pid,old.local_pid)||
                   (old.startup_entry_id.value&&!same_birth_metric(found->startup_entry_id,old.startup_entry_id))||
                   ((old.first_executable.value||old.final_monotonic_ns)&&!same_birth_metric(found->first_executable,old.first_executable))||
                   (old.final_monotonic_ns&&(found->final_monotonic_ns!=old.final_monotonic_ns||!same_birth_metric(found->cpu_time,old.cpu_time)||!same_birth_metric(found->read_bytes,old.read_bytes)||!same_birth_metric(found->write_bytes,old.write_bytes))))
                    throw std::invalid_argument("Owned birth report rewrote/omitted an immutable retained row");
            }
        }
        for(const auto& row:r.observations){
            auto measurement=measure_startup_birth_impact(r,row.key,options.duration);
            if(measurement)projected.push_back(std::move(*measurement.value));
            else if(measurement.error!=ActionError::not_found)throw std::invalid_argument("Invalid owned birth observation");
        }
        // Prepare allocations before swapping either accepted public view.
        std::optional<StartupBirthObserverReport> accepted{std::move(r)};
        status.birth_capture.swap(accepted);status.birth_impacts.swap(projected);
    }
    void poll_births(){
        if(!birth||birth_stopped)return;
        const auto result=birth->poll();snapshot_births();
        if(!result||status.birth_capture->state!=StartupBirthObserverState::recording)throw std::runtime_error("Owned birth capture failed");
    }
    void verify_context(){const auto own=api.context();if(!own||own.value->native_context!=status.native_context)throw std::invalid_argument("recorder context changed");}
    StartupProcessSampleResult read_sample(std::uint32_t pid,std::uint64_t ticks){
        const auto kind=registered.find({pid,ticks});
        auto result=kind!=registered.end()&&kind->second==StartupCpuCounter::linux_scheduler_runtime?api.scheduler_sample(pid,ticks):api.sample(pid,ticks);
        if(result&&kind!=registered.end()&&result.value->cpu_counter!=kind->second)return {{},ActionError::invalid_argument,"Recorder CPU counter kind changed; sample rejected"};
        auto found=exits.find({pid,ticks});
        if(found!=exits.end()){
            if(result){auto replacement=*result.value;found->second.last=std::move(replacement);}
            else if(result.error==ActionError::not_found)return {{},ActionError::not_supported,"Selected exit disk record pending/uncovered; keep observation open until deadline, not zero"};
        }
        return result;
    }
    void poll_samples(){
        for(auto it=exits.begin();it!=exits.end();){
            if(api.now()>status.deadline){++it;continue;} // Never attribute a late release to this phase.
            Metric<std::optional<detail::StartupExitResourceTotals>> value;
            try{value=it->second.reader();}catch(...){value={{},Support::unsupported,"Selected exit reader failed"};}
            if(!value.value||value.support!=Support::available){add(status.exit_disk_unavailable,1);it=exits.erase(it);continue;}
            if(!value.value->has_value()){++it;continue;}
            auto final=it->second.last;final.sampled_at=api.now();
            if(final.sampled_at>status.deadline){++it;continue;}
            const auto& totals=**value.value;
            final.cumulative_disk_bytes={totals.disk_bytes,Support::estimated,"Recovered Linux group-accounted read_bytes+write_bytes; includes waited-child IO, not exclusive IO"};
            final.cumulative_cpu_time.note="Last observed procfs CPU LOWER BOUND; final scheduler-runtime counter intentionally not mixed";
            if(final.cpu_counter==StartupCpuCounter::linux_scheduler_runtime){
                if(metric_is_known(totals.scheduler_cpu_time)&&totals.scheduler_cpu_time.value->count()>=0&&
                   (!metric_is_known(final.cumulative_cpu_time)||*totals.scheduler_cpu_time.value>=*final.cumulative_cpu_time.value))
                    final.cumulative_cpu_time={*totals.scheduler_cpu_time.value,Support::estimated,"Recovered final scheduler-runtime group CPU; excludes waited-child CPU"};
                else final.cumulative_cpu_time.note="Last observed scheduler CPU LOWER BOUND; final CPU unavailable/decreased";
            }
            // Reject resets rather than replacing a larger prior measurement.
            const bool reset=metric_is_known(it->second.last.cumulative_disk_bytes)&&totals.disk_bytes<*it->second.last.cumulative_disk_bytes.value;
            const auto finished=reset?ActionResult{ActionError::stale_target,"Recovered IO decreased"}:observer->finish_partial_process(final);
            if(finished)add(status.exit_disk_recovered,1);else add(status.exit_disk_unavailable,1);
            it=exits.erase(it);
        }
        const auto result=observer->poll(api.now());add(status.poll_attempted,result.attempted);add(status.poll_updated,result.updated);add(status.poll_failed,result.failed);
        for(const auto& row:observer->observations())if(row.closed)add(status.exit_disk_released_without_final,exits.erase({row.pid,row.linux_start_ticks}));
    }
    void scan(){
        auto snapshot=api.snapshot();
        if(snapshot.processes.size()>100000||snapshot.startup_entries.size()>100000)throw std::length_error("recorder snapshot bounds");
        std::erase_if(snapshot.processes,[&](const auto& p){return metric_is_known(p.linux_start_ticks)&&registered.contains({p.pid,*p.linux_start_ticks.value});});
        const auto report=api.candidates(snapshot);add(status.scans,1);
        if(report.candidates.size()>4096||report.examined>4096||report.ignored>4096||report.unavailable>4096)throw std::length_error("recorder candidate bounds");
        add(status.examined,report.examined);add(status.ignored,report.ignored);add(status.unavailable,report.unavailable);
        if(!report.complete||report.error!=ActionError::none)add(status.partial_scans,1);
        if(report.error!=ActionError::none)return; // Never accept a partial failed batch's claimed associations.
        for(const auto& candidate:report.candidates){
            if(candidate.baseline.native_context!=status.native_context)throw std::invalid_argument("recorder context changed");
            if(candidate.association_support!=Support::estimated||candidate.automatic_startup_origin.value){add(status.rejected,1);continue;}
            const auto key=std::pair{candidate.baseline.pid,candidate.baseline.linux_start_ticks};
            if(registered.contains(key))continue;
            auto baseline=candidate.baseline;
            if(options.enable_privileged_exit_disk_recovery&&api.scheduler_sample){
                try{const auto sampled=api.scheduler_sample(baseline.pid,baseline.linux_start_ticks);
                    if(sampled&&sampled.value->pid==baseline.pid&&sampled.value->linux_start_ticks==baseline.linux_start_ticks&&sampled.value->native_context==baseline.native_context&&
                       sampled.value->sampled_at>=baseline.sampled_at&&sampled.value->cpu_counter==StartupCpuCounter::linux_scheduler_runtime&&metric_is_known(sampled.value->cumulative_cpu_time))baseline=*sampled.value;
                }catch(...){/* Optional compatible CPU baseline unavailable: retain procfs mode. */}
            }
            const auto result=observer->register_process(candidate.startup_entry_id,baseline);
            if(result){
                registered.emplace(key,baseline.cpu_counter);add(status.registered,1);
                if(options.enable_privileged_exit_disk_recovery){
                    if(exits.size()>=options.max_exit_disk_captures){add(status.exit_disk_skipped,1);continue;}
                    add(status.exit_disk_attempted,1);
                    try{auto attached=api.exit_disk(baseline);
                        if(attached.value&&attached.support==Support::available&&*attached.value){exits.emplace(key,ExitDisk{std::move(*attached.value),baseline});add(status.exit_disk_attached,1);}
                        else add(status.exit_disk_unavailable,1);
                    }catch(...){add(status.exit_disk_unavailable,1);}
                }
            }else add(status.rejected,1);
        }
    }
    void finish(){
        const auto finished=api.now();
        if(finished<status.started_at||finished<last)throw std::invalid_argument("recorder finish clock inverted");
        const auto time=api.wall();if(time.time_since_epoch().count()<=0)throw std::invalid_argument("recorder wall time invalid");
        if(observer)observer->close();
        add(status.exit_disk_released_without_final,exits.size());exits.clear();stop_births();stop_manual();status.finished_at=finished;status.recorded_at=time;status.state=StartupRecorderState::closed;
    }
    void schedule(std::chrono::steady_clock::time_point now){
        const auto interval=std::chrono::duration_cast<std::chrono::steady_clock::duration>(options.scan_interval);
        status.next_scan=interval>=status.deadline-now?status.deadline:now+interval;
    }
};
LinuxStartupRecorder::LinuxStartupRecorder():impl_(std::make_unique<Impl>()){}
LinuxStartupRecorder::LinuxStartupRecorder(std::unique_ptr<Impl> impl):impl_(std::move(impl)){}
LinuxStartupRecorder::~LinuxStartupRecorder()=default;
LinuxStartupRecorder::LinuxStartupRecorder(LinuxStartupRecorder&&) noexcept=default;
LinuxStartupRecorder& LinuxStartupRecorder::operator=(LinuxStartupRecorder&&) noexcept=default;
ActionResult LinuxStartupRecorder::start(const StartupRecorderOptions& options){
    if(!options.accept_estimated_gio_associations)return {ActionError::permission_denied,"Explicit acceptance of estimated GIO associations required; no capture started"};
    if(!impl_)return {ActionError::invalid_argument,"Moved-from recorder"};
    std::lock_guard lock(impl_->mutex);auto& i=*impl_;
    if(i.status.state!=StartupRecorderState::idle)return {ActionError::invalid_argument,"One capture per recorder; create a new instance"};
    if(options.capture_id.empty()||options.capture_id.size()>256||options.capture_id.find('\0')!=std::string::npos||
       options.duration<std::chrono::milliseconds(10)||options.duration>std::chrono::hours(1)||
       options.scan_interval<std::chrono::milliseconds(10)||options.scan_interval>std::chrono::seconds(60))return {ActionError::invalid_argument,"Invalid recorder ID/duration/scan interval"};
    if(options.enable_privileged_exit_disk_recovery&&(!options.max_exit_disk_captures||options.max_exit_disk_captures>32||!i.api.exit_disk))return {ActionError::invalid_argument,"Exit disk recovery requires a source and concurrent bound1..32"};
    if(!i.api.context||!i.api.snapshot||!i.api.candidates||!i.api.sample||!i.api.now||!i.api.wall)return {ActionError::not_supported,"Recorder source unavailable"};
    try{
        const auto own=i.api.context();if(!own)return i.fail(own.error==ActionError::none?ActionError::not_supported:own.error);
        const auto& context=*own.value;
        if(!context.pid||!context.linux_start_ticks||context.native_context.empty()||context.native_context.size()>1024||context.native_context.find('\0')!=std::string::npos)return i.fail(ActionError::stale_target);
        i.options=options;i.status.capture_id=options.capture_id;i.status.native_context=context.native_context;
        i.status.automatic_startup_origin={{},Support::unsupported,"GIO launch hints may be forged/manual; automatic startup origin NOT certified"};
        i.observer=std::make_unique<StartupImpactObserver>(options.duration,StartupImpactPolicy{},[source=&i](auto pid,auto ticks){return source->read_sample(pid,ticks);});
        const auto now=i.api.now();const auto window=std::chrono::duration_cast<std::chrono::steady_clock::duration>(options.duration);
        if(now.time_since_epoch().count()>std::numeric_limits<std::chrono::steady_clock::duration::rep>::max()-window.count())return i.fail(ActionError::invalid_argument);
        i.status.started_at=i.last=now;i.status.deadline=now+window;i.status.state=StartupRecorderState::recording;
        i.scan();i.poll_samples();
        const auto completed=i.api.now();if(completed<i.last)return i.fail(ActionError::stale_target);
        i.last=completed;
        if(completed>=i.status.deadline)i.finish();else i.schedule(completed);
        return {};
    }catch(...){return i.fail(ActionError::system_error);}
}
ActionResult LinuxStartupRecorder::poll(){
    if(!impl_)return {ActionError::invalid_argument,"Moved-from recorder"};
    std::lock_guard lock(impl_->mutex);auto& i=*impl_;
    if(i.status.state==StartupRecorderState::closed)return {};
    if(i.status.state!=StartupRecorderState::recording)return {i.status.error==ActionError::none?ActionError::invalid_argument:i.status.error,"Recorder is not recording"};
    try{
        auto now=i.api.now();if(now<i.last)return i.fail(ActionError::stale_target);
        i.last=now;
        if(i.manual_mode){i.verify_context();if(now>=i.status.deadline)return i.finish_manual();return {};}
        if(now>=i.status.deadline){i.verify_context();i.poll_samples();i.poll_births();i.finish();return {};}
        if(now<i.status.next_scan){if(i.birth){i.verify_context();i.poll_births();const auto completed=i.api.now();if(completed<i.last)return i.fail(ActionError::stale_target);i.last=completed;if(completed>=i.status.deadline)i.finish();}return {};}
        i.verify_context();i.scan();i.poll_samples();i.poll_births();
        now=i.api.now();if(now<i.last)return i.fail(ActionError::stale_target);
        i.last=now;
        if(now>=i.status.deadline)i.finish();else i.schedule(now); // Slow I/O may overshoot; no catch-up scan storm.
        return {};
    }catch(...){return i.fail(ActionError::system_error);}
}
ActionResult LinuxStartupRecorder::watch_births(bool authorized,int fd,std::uint32_t pid,std::uint64_t ticks,const StartupBirthObserverOptions& options){
    if(!authorized)return {ActionError::permission_denied,"Explicit selected-creator birth capture authorization required"};
    if(!impl_)return {ActionError::invalid_argument,"Moved-from recorder"};
    std::lock_guard lock(impl_->mutex);auto& i=*impl_;
    if(i.manual_mode)return {ActionError::not_supported,"Manual recorder owns its separate birth source"};
    if(i.status.state!=StartupRecorderState::recording||i.birth||fd<0||!pid||pid>2147483647U||!ticks||options.capture_id.empty()||options.capture_id.size()>128||
       options.capture_id.find('\0')!=std::string::npos||options.capture_id==i.status.capture_id||!options.capacity||options.capacity>64)
        return {ActionError::invalid_argument,"Recording state, one distinct creator capture ID and valid selected identity/options required"};
    if(!i.api.birth)return {ActionError::not_supported,"Selected birth source unavailable"};
    try{
        i.verify_context();const auto now=i.api.now();
        if(now<i.last)return i.fail(ActionError::stale_target);
        if(now>=i.status.deadline){i.finish();return {ActionError::invalid_argument,"Recorder phase ended before birth attachment"};}
        auto name=options.capture_id;auto selected_options=options;auto attached=i.api.birth(true,fd,pid,ticks,options);
        if(!attached.source)return attached.result?ActionResult{ActionError::system_error,"Birth source missing"}:attached.result;
        // Take ownership BEFORE any further validation/allocation, so every
        // exception/invalid factory result closes the successfully created source.
        i.birth=std::move(attached.source);i.birth_capture_id.swap(name);i.birth_options=std::move(selected_options);
        if(!attached.result||!i.birth->poll||!i.birth->report||!i.birth->associate||!i.birth->close)return i.fail(ActionError::system_error);
        i.verify_context();i.snapshot_births();if(i.status.birth_capture->state!=StartupBirthObserverState::recording)return i.fail(ActionError::system_error);
        const auto completed=i.api.now();if(completed<now)return i.fail(ActionError::stale_target);i.last=completed;
        if(completed>=i.status.deadline)i.finish();
        return {};
    }catch(...){return i.fail(ActionError::system_error);}
}
ActionResult LinuxStartupRecorder::associate_birth(const StartupBirthProcessKey& key,const std::string& id){
    if(!impl_)return {ActionError::invalid_argument,"Moved-from recorder"};
    std::lock_guard lock(impl_->mutex);auto& i=*impl_;
    if(!i.birth||(i.status.state!=StartupRecorderState::recording&&i.status.state!=StartupRecorderState::closed))return {ActionError::invalid_argument,"No usable retained birth capture"};
    try{i.verify_context();const auto result=i.birth->associate(key,id);if(!result)return result;i.snapshot_births();return {};}
    catch(...){return i.fail(ActionError::system_error);}
}
void LinuxStartupRecorder::close(){
    if(!impl_)return;
    std::lock_guard lock(impl_->mutex);auto& i=*impl_;
    if(i.status.state!=StartupRecorderState::recording)return;
#ifdef __linux__
    if((i.native_owner&&i.native_owner->local_pid!=static_cast<std::uint32_t>(::getpid()))||(i.native_euid&&*i.native_euid!=static_cast<std::uint32_t>(::geteuid()))){i.fail(ActionError::stale_target);return;}
#endif
    try{i.finish();}catch(...){i.fail(ActionError::system_error);}
}
StartupRecorderReport LinuxStartupRecorder::report()const{
    if(!impl_)return {};
    std::lock_guard lock(impl_->mutex);auto result=impl_->status;
    if(impl_->observer)result.observations=impl_->observer->observations();
    return result;
}
StartupArchiveIoResult LinuxStartupRecorder::save(const std::filesystem::path& path)const{
    if(!impl_)return {ActionError::invalid_argument,false,"Moved-from recorder"};
    std::lock_guard lock(impl_->mutex);const auto& i=*impl_;
    if(i.manual_mode)return {ActionError::not_supported,false,"Manual-only recorder requires explicit PCIMAN save, never legacy PCISTART"};
    if(i.status.state!=StartupRecorderState::closed||i.status.error!=ActionError::none)return {ActionError::invalid_argument,false,"Recorder must be closed successfully before explicit save"};
    try{const auto context=i.api.context();if(!context||context.value->native_context!=i.status.native_context)return {ActionError::stale_target,false,"Recorder context changed; archive not saved"};}
    catch(...){return {ActionError::stale_target,false,"Recorder context unavailable; archive not saved"};}
    std::error_code error;const auto existing=std::filesystem::symlink_status(path,error);
    if(error&&error!=std::errc::no_such_file_or_directory)return {ActionError::system_error,false,"Cannot inspect archive path"};
    if(std::filesystem::exists(existing)&&!StartupImpactStore::load(path))return {ActionError::invalid_argument,false,"Existing archive invalid/foreign; preserved"};
    StartupImpactArchive archive{i.status.capture_id,i.status.native_context,i.status.recorded_at,i.observer->observations(),{}, {}};
    StartupCaptureDiagnostics d;d.requested_duration=i.options.duration;d.scan_interval=i.options.scan_interval;
    // Native steady clocks are nanoseconds on the supported builds. Use a
    // checked conversion rather than persisting absolute steady-clock epochs.
    using Duration=std::chrono::steady_clock::duration;using Ratio=std::ratio_divide<Duration::period,std::nano>;
    const auto earlier=i.status.started_at.time_since_epoch().count(),later=i.status.finished_at.time_since_epoch().count();
    if(later<earlier||(earlier<0&&later>std::numeric_limits<Duration::rep>::max()+earlier))return {ActionError::invalid_argument,false,"Recorder elapsed interval overflow"};
    const auto elapsed=i.status.finished_at-i.status.started_at;
    if(static_cast<std::uint64_t>(elapsed.count())>static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max())/Ratio::num)return {ActionError::invalid_argument,false,"Recorder elapsed duration cannot be represented"};
    d.elapsed=std::chrono::duration_cast<std::chrono::nanoseconds>(elapsed);
    if(std::chrono::duration_cast<Duration>(d.elapsed)!=elapsed)return {ActionError::invalid_argument,false,"Recorder elapsed duration loses precision"};
    d.exit_disk_enabled=i.options.enable_privileged_exit_disk_recovery;d.max_exit_disk_captures=i.options.max_exit_disk_captures;
    d.scans=i.status.scans;d.partial_scans=i.status.partial_scans;d.examined=i.status.examined;d.ignored=i.status.ignored;d.unavailable=i.status.unavailable;d.registered=i.status.registered;d.rejected=i.status.rejected;
    d.poll_attempted=i.status.poll_attempted;d.poll_updated=i.status.poll_updated;d.poll_failed=i.status.poll_failed;
    d.exit_disk_attempted=i.status.exit_disk_attempted;d.exit_disk_attached=i.status.exit_disk_attached;d.exit_disk_skipped=i.status.exit_disk_skipped;d.exit_disk_unavailable=i.status.exit_disk_unavailable;d.exit_disk_recovered=i.status.exit_disk_recovered;
    d.exit_disk_released_without_final=i.status.exit_disk_released_without_final;
    d.automatic_startup_origin=i.status.automatic_startup_origin;archive.diagnostics=std::move(d);
    if(i.birth){if(!i.status.birth_capture)return {ActionError::invalid_argument,false,"Owned birth capture missing from retained report"};archive.births=StartupBirthArchive{*i.status.birth_capture,i.options.duration,{}};}
    return StartupImpactStore::save(path,archive);
}
}
namespace pcinsight::detail {
LinuxStartupRecorder StartupRecorderAccess::make(StartupRecorderApi api){return LinuxStartupRecorder(std::make_unique<LinuxStartupRecorder::Impl>(std::move(api)));}
ActionResult StartupRecorderAccess::start_manual(LinuxStartupRecorder& recorder,const std::string& entry,bool launch_consent,bool capture_consent,const StartupRecorderManualOptions& options){
    if(!launch_consent||!capture_consent)return {ActionError::permission_denied,"Explicit manual launch and creator/exec capture consents required"};
    if(!recorder.impl_)return {ActionError::invalid_argument,"Moved-from recorder"};
    std::lock_guard lock(recorder.impl_->mutex);auto& i=*recorder.impl_;
    if(i.status.state!=StartupRecorderState::idle)return {ActionError::invalid_argument,"One mode/capture per recorder"};
    if(options.capture_id.empty()||options.capture_id.size()>128||options.capture_id.find('\0')!=std::string::npos||options.window.count()<=0||options.window>std::chrono::hours(1)||options.policy.medium_cpu.count()<=0||options.policy.high_cpu<options.policy.medium_cpu||!options.policy.medium_disk_bytes||options.policy.high_disk_bytes<options.policy.medium_disk_bytes||entry.empty()||entry.front()!='/'||entry.size()>32768||entry.find('\0')!=std::string::npos)return {ActionError::invalid_argument,"Invalid manual entry/capture/window/policy"};
    if(!i.api.context||!i.api.now||!i.api.wall||!i.api.manual)return {ActionError::not_supported,"Manual recorder source unavailable"};
    try{
        const auto own=i.api.context();if(!own||!own.value->pid||!own.value->linux_start_ticks||own.value->native_context.empty()||own.value->native_context.size()>1024||own.value->native_context.find('\0')!=std::string::npos)return i.fail(ActionError::stale_target);
        i.manual_entry=std::filesystem::path(entry).lexically_normal().generic_string();i.manual_options=options;
        i.status.capture_id=options.capture_id;i.status.native_context=own.value->native_context;
        i.status.automatic_startup_origin={{},Support::unsupported,"Explicit MANUAL controlled launch, never automatic-startup proof"};
        auto source=i.api.manual();if(!source.start||!source.finish||!source.close)return i.fail(ActionError::not_supported);
        i.manual=std::move(source);i.manual_mode=true;
        const auto now=i.api.now();const auto window=std::chrono::duration_cast<std::chrono::steady_clock::duration>(options.window);
        if(std::chrono::duration_cast<std::chrono::nanoseconds>(window)!=options.window||now.time_since_epoch().count()>std::numeric_limits<std::chrono::steady_clock::duration::rep>::max()-window.count())return i.fail(ActionError::invalid_argument);
        i.status.started_at=i.last=now;i.status.deadline=i.status.next_scan=now+window;i.status.state=StartupRecorderState::recording;
        auto result=i.manual->start(i.manual_entry,true,true,options.capture_id);
        // No allocations/reads or failure paths after a released PID is returned.
        i.manual_pid=result.process_id;i.manual_error=result.error;return result;
    }catch(...){return i.fail(ActionError::system_error);}
}
ActionResult StartupRecorderAccess::finish_manual(LinuxStartupRecorder& recorder){
    if(!recorder.impl_)return {ActionError::invalid_argument,"Moved-from recorder"};
    std::lock_guard lock(recorder.impl_->mutex);auto& i=*recorder.impl_;
    if(!i.manual_mode||!i.manual||i.manual_stopped||i.status.state!=StartupRecorderState::recording)return {ActionError::invalid_argument,"No active manual recorder"};
    try{return i.finish_manual();}catch(...){return i.fail(ActionError::system_error);}
}
Metric<LinuxManualLaunchArchive> StartupRecorderAccess::manual_archive(const LinuxStartupRecorder& recorder){
    if(!recorder.impl_)return {{},Support::unsupported,"Moved-from recorder"};
    std::lock_guard lock(recorder.impl_->mutex);const auto& i=*recorder.impl_;
    if(!i.manual_saved||i.status.state!=StartupRecorderState::closed||i.status.error!=ActionError::none)return {{},Support::unsupported,"No finalized manual evidence; close alone does not sample"};
    return {*i.manual_saved,Support::available,"Historical manual recorder archive, no live authority"};
}
StartupArchiveIoResult StartupRecorderAccess::save_manual(const LinuxStartupRecorder& recorder,const std::filesystem::path& path){
    if(!recorder.impl_)return {ActionError::invalid_argument,false,"Moved-from recorder"};
    std::lock_guard lock(recorder.impl_->mutex);const auto& i=*recorder.impl_;
    if(!i.manual_saved||i.status.state!=StartupRecorderState::closed||i.status.error!=ActionError::none)return {ActionError::invalid_argument,false,"Explicit manual finalization required before save"};
    try{const auto own=i.api.context();if(!own||own.value->native_context!=i.status.native_context)return {ActionError::stale_target,false,"Recorder context changed, existing file preserved"};}
    catch(...){return {ActionError::stale_target,false,"Recorder context unavailable, existing file preserved"};}
    std::error_code error;const auto existing=std::filesystem::symlink_status(path,error);
    if(error&&error!=std::errc::no_such_file_or_directory)return {ActionError::system_error,false,"Cannot inspect manual archive path"};
    if(std::filesystem::exists(existing)&&!load_linux_manual_launch(path))return {ActionError::invalid_argument,false,"Corrupt/foreign existing evidence preserved"};
    return save_linux_manual_launch(path,*i.manual_saved);
}
Metric<StartupExitDiskReader> make_native_startup_exit_disk_reader(const StartupProcessSample& baseline){
#ifdef __linux__
    auto fail=[](std::string n,Support s=Support::unsupported){return Metric<StartupExitDiskReader>{{},s,std::move(n)};};
    // The caller explicitly opted in; this helper never elevates privileges.
    const auto before=read_startup_process_sample(baseline.pid,baseline.linux_start_ticks);
    if(!before||before.value->native_context!=baseline.native_context)return fail("Exit disk baseline context not current");
    struct Fd {int n;~Fd(){if(n>=0)::close(n);}};
    Fd pidfd{static_cast<int>(::syscall(SYS_pidfd_open,baseline.pid,0))};
    if(pidfd.n<0)return fail("Cannot open selected exit disk pidfd",errno==EPERM||errno==EACCES?Support::permission_required:Support::unsupported);
    auto capture=std::make_shared<LinuxStartupResourceCapture>();
    const auto started=capture->start(true,pidfd.n,baseline.pid,baseline.linux_start_ticks);
    if(!started.value||started.support!=Support::available)return fail(started.note,started.support);
    if(started.value->local.process.local_pid!=baseline.pid||started.value->local.process.start_ticks!=baseline.linux_start_ticks)return fail("Selected exit disk identity mismatch");
    const auto after=read_startup_process_sample(baseline.pid,baseline.linux_start_ticks);
    if(!after||after.value->native_context!=baseline.native_context)return fail("Exit disk context changed while attaching");
    StartupExitDiskReader reader=[capture]{
        const auto sampled=capture->sample();
        if(!sampled.value||sampled.support!=Support::available)return Metric<std::optional<detail::StartupExitResourceTotals>>{{},sampled.support,sampled.note};
        if(!sampled.value->resources)return Metric<std::optional<detail::StartupExitResourceTotals>>{std::optional<detail::StartupExitResourceTotals>{},Support::available,"Selected final disk record pending or not covered"};
        const auto& r=*sampled.value->resources;
        if(r.group_accounted_read_bytes>UINT64_MAX-r.group_accounted_write_bytes)return Metric<std::optional<detail::StartupExitResourceTotals>>{{},Support::unsupported,"Selected final read+write bytes overflow"};
        StartupExitResourceTotals totals{r.group_accounted_read_bytes+r.group_accounted_write_bytes};
        if(r.scheduler_runtime_ns<=static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max()))totals.scheduler_cpu_time={std::chrono::nanoseconds(static_cast<std::int64_t>(r.scheduler_runtime_ns)),Support::available,"Final scheduler-runtime group CPU total"};
        else totals.scheduler_cpu_time={{},Support::unsupported,"Final scheduler CPU duration overflow"};
        return Metric<std::optional<detail::StartupExitResourceTotals>>{std::optional<detail::StartupExitResourceTotals>{std::move(totals)},Support::available,"Recovered final group-accounted disk and separately typed scheduler CPU"};
    };
    return {std::move(reader),Support::available,"Optional selected exit disk recovery attached; no automatic-startup proof"};
#else
    (void)baseline;return {{},Support::unsupported,"Selected exit disk recovery requires Linux"};
#endif
}
}
