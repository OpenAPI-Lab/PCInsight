#include "startup_birth_observer.hpp"
#include <mutex>
#include <stdexcept>
namespace pcinsight {
namespace {
bool text(const std::string& s,std::size_t limit,bool empty=false){return (empty||!s.empty())&&s.size()<=limit&&s.find('\0')==s.npos;}
template<class T> bool shape(const Metric<T>& m){return text(m.note,4096,true)&&static_cast<unsigned>(m.support)<=static_cast<unsigned>(Support::estimated)&&
    (m.value?(m.support==Support::available||m.support==Support::estimated):(m.support==Support::unsupported||m.support==Support::permission_required));}
template<class T> bool same(const Metric<T>& a,const Metric<T>& b){return a.value==b.value&&a.support==b.support&&a.note==b.note;}
bool same(const detail::PrivateNetworkIdentity& a,const detail::PrivateNetworkIdentity& b){return a.kernel_tgid==b.kernel_tgid&&a.kernel_real_uid==b.kernel_real_uid&&a.leader_start_boottime_ns==b.leader_start_boottime_ns;}
bool identity(const detail::PrivateNetworkIdentity& k){return k.kernel_tgid&&k.kernel_tgid<=2147483647U&&k.leader_start_boottime_ns;}
ActionError source_error(Support s){return s==Support::permission_required?ActionError::permission_denied:ActionError::not_supported;}
}
struct LinuxStartupBirthObserver::Impl {
    struct Row {detail::StartupChildBirth raw;StartupBirthObservation view;};
    mutable std::mutex mutex;
    detail::LinuxStartupBirthCapture native;
    detail::StartupBirthObserverApi api;
    StartupBirthObserverOptions options;
    StartupBirthObserverReport meta;
    std::optional<detail::PrivatePidfdNetworkJoin> creator;
    std::vector<Row> rows;
    std::optional<detail::NetworkSelectionContext> owner;bool owned{},mode_seen{};
    Impl(){
        api.context=[]{const auto source=detail::native_network_selection_source_api();
            if(!source.observe)return Metric<detail::NetworkSelectionContext>{{},Support::unsupported,"Birth capture requires Linux"};
            return source.observe(source.context);};
        api.start=[this](bool a,int f,std::uint32_t p,std::uint64_t t,std::uint32_t c,bool r,bool e){return native.start(a,f,p,t,c,r,e);};
        api.read=[this]{return native.sample();};api.stop=[this]{native.stop();};init();
    }
    explicit Impl(detail::StartupBirthObserverApi a):api(std::move(a)){
        if(!api.context||!api.start||!api.read||!api.stop)throw std::invalid_argument("Incomplete birth observer source");
        init();
    }
    void init(){meta.automatic_startup_origin={{},Support::unsupported,"Caller association/creator birth do not establish automatic-startup cause"};}
    bool owner_valid(){const auto c=api.context();return owner&&c.value&&c.support==Support::available&&*c.value==*owner;}
    ~Impl(){stop();}
    void stop() noexcept {if(owned){owned=false;try{api.stop();}catch(...){}}}
    ActionResult fail(ActionError e,std::string message){stop();meta.state=StartupBirthObserverState::failed;meta.error=e;meta.message=std::move(message);return {e,meta.message};}
    bool prefix(const detail::StartupChildBirth& a,const detail::StartupChildBirth& b){return same(a.kernel,b.kernel)&&a.monotonic_ns==b.monotonic_ns&&a.scheduler_runtime_at_birth_ns==b.scheduler_runtime_at_birth_ns&&a.read_bytes_at_birth==b.read_bytes_at_birth&&a.write_bytes_at_birth==b.write_bytes_at_birth&&same(a.local_pid,b.local_pid);}
    bool finals(const detail::StartupChildBirth& a,const detail::StartupChildBirth& b){return a.final_monotonic_ns==b.final_monotonic_ns&&same(a.cpu_time,b.cpu_time)&&same(a.read_bytes,b.read_bytes)&&same(a.write_bytes,b.write_bytes);}
    bool execs(const detail::StartupChildBirth& a,const detail::StartupChildBirth& b){return (!a.exec_state&&!a.final_monotonic_ns)||(a.exec_state==b.exec_state&&same(a.first_executable,b.first_executable));}
    bool valid(const detail::StartupChildBirth& r,const detail::StartupBirthRead& s){
        if(!identity(r.kernel)||!r.monotonic_ns||!shape(r.local_pid)||!shape(r.cpu_time)||!shape(r.read_bytes)||!shape(r.write_bytes))return false;
        if(!shape(r.first_executable)||(r.exec_state!=0&&r.exec_state!=1&&r.exec_state!=3)||(!options.observe_first_exec&&r.exec_state))return false;
        if(r.exec_state==1){if(!r.first_executable.value||r.first_executable.value->device_major>4095||r.first_executable.value->device_minor>1048575||r.first_executable.value->observed_monotonic_ns<r.monotonic_ns||(r.final_monotonic_ns&&r.first_executable.value->observed_monotonic_ns>*r.final_monotonic_ns))return false;}
        else if(r.first_executable.value||(r.exec_state==3&&!(s.quality_flags&64)))return false;
        if(r.local_pid.value){const auto& p=*r.local_pid.value;
            if(!s.pid_mapping_enabled||!p.tgid||p.tgid>2147483647U||p.pid_namespace_inode!=creator->local.observer.pid_namespace_inode)return false;
        }else if(s.pid_mapping_enabled&&!(s.quality_flags&32))return false;
        if(r.cpu_time.value&&r.cpu_time.value->count()<0)return false;
        if(!r.final_monotonic_ns)return !r.cpu_time.value&&!r.read_bytes.value&&!r.write_bytes.value;
        return options.recover_final_resources&&*r.final_monotonic_ns>=r.monotonic_ns&&r.read_bytes.value&&r.write_bytes.value;
    }
    void update(Row& r){
        r.view.birth_monotonic_ns=r.raw.monotonic_ns;r.view.final_monotonic_ns=r.raw.final_monotonic_ns;
        r.view.cpu_time=r.raw.cpu_time;r.view.read_bytes=r.raw.read_bytes;r.view.write_bytes=r.raw.write_bytes;
        r.view.first_executable=r.raw.first_executable;
        r.view.local_pid={{},r.raw.local_pid.support,r.raw.local_pid.note};
        if(r.raw.local_pid.value){const auto& p=*r.raw.local_pid.value;r.view.local_pid.value=StartupBirthPid{creator->local.observer.pid_namespace_device,p.pid_namespace_inode,p.tgid};}
    }
};
LinuxStartupBirthObserver::LinuxStartupBirthObserver():impl_(std::make_unique<Impl>()){}
LinuxStartupBirthObserver::LinuxStartupBirthObserver(std::unique_ptr<Impl> p):impl_(std::move(p)){}
LinuxStartupBirthObserver::~LinuxStartupBirthObserver()=default;
LinuxStartupBirthObserver detail::StartupBirthObserverAccess::make(StartupBirthObserverApi a){return LinuxStartupBirthObserver(std::make_unique<LinuxStartupBirthObserver::Impl>(std::move(a)));}
ActionResult LinuxStartupBirthObserver::start(bool authorized,int fd,std::uint32_t pid,std::uint64_t ticks,const StartupBirthObserverOptions& options){
    std::lock_guard lock(impl_->mutex);auto& p=*impl_;
    if(!authorized)return {ActionError::permission_denied,"Explicit selected-creator capture authorization required"};
    if(p.meta.state!=StartupBirthObserverState::idle||!text(options.capture_id,128)||!options.capacity||options.capacity>64||fd<0||!pid||pid>2147483647U||!ticks)
        return {ActionError::invalid_argument,"Invalid/active/used birth observer options or creator identity"};
    try{
        p.options=options;p.meta.capture_id=options.capture_id;const auto context=p.api.context();
        if(!context.value||context.support!=Support::available)return p.fail(source_error(context.support),context.note);
        p.owner=*context.value;
        p.owned=true;const auto selected=p.api.start(true,fd,pid,ticks,options.capacity,options.recover_final_resources,options.observe_first_exec);
        if(!selected.value||selected.support!=Support::available)return p.fail(source_error(selected.support),selected.note);
        const auto& k=*selected.value;const auto& c=k.local;
        if(!identity(k.kernel)||!(c.boot[0]||c.boot[1])||!detail::valid_network_self(c.observer)||!detail::valid_network_self(c.process)||c.process.local_pid!=pid||c.process.start_ticks!=ticks||c.boot!=p.owner->boot||c.observer!=p.owner->observer||!p.owner_valid())
            return p.fail(ActionError::stale_target,"Creator/observer identity changed or invalid at capture start");
        p.creator=k;p.meta.boot=c.boot;p.meta.pid_namespace_device=c.observer.pid_namespace_device;p.meta.pid_namespace_inode=c.observer.pid_namespace_inode;
        p.meta.final_resources_enabled=options.recover_final_resources;p.meta.state=StartupBirthObserverState::recording;
        p.meta.first_exec_enabled=options.observe_first_exec;
        return {ActionError::none,"Explicit creator capture started; no app association inferred"};
    }catch(...){return p.fail(ActionError::system_error,"Birth observer start exception; owned source stopped");}
}
ActionResult LinuxStartupBirthObserver::poll(){
    std::lock_guard lock(impl_->mutex);auto& p=*impl_;
    if(p.meta.state!=StartupBirthObserverState::recording)return {ActionError::invalid_argument,"Birth observer not recording"};
    try{
        if(!p.owner_valid())return p.fail(ActionError::stale_target,"Caller/fork/context changed; inherited source references closed");
        const auto snapshot=p.api.read();if(!snapshot.value||(snapshot.support!=Support::available&&snapshot.support!=Support::estimated))return p.fail(source_error(snapshot.support),snapshot.note);
        const auto& s=*snapshot.value;
        if(s.creator.local!=p.creator->local||!same(s.creator.kernel,p.creator->kernel)||s.births.size()>p.options.capacity||s.publishing>p.options.capacity||s.quality_flags&~127ULL||s.first_exec_enabled!=p.options.observe_first_exec||(!s.first_exec_enabled&&(s.quality_flags&64))||
            (s.quality_flags&p.meta.quality_flags)!=p.meta.quality_flags||(p.meta.creator_exited&&!s.creator_exited)||s.creator_reaped_totals_enabled!=p.options.recover_final_resources||
            !text(s.pid_mapping_note,4096,true)||(p.mode_seen&&s.pid_mapping_enabled!=p.meta.pid_mapping_enabled))
            return p.fail(ActionError::stale_target,"Birth source context/flags/mode changed; last accepted observations retained");
        auto next=p.rows;std::vector<detail::PrivateNetworkIdentity> seen;
        for(const auto& r:s.births){
            if(!p.valid(r,s))return p.fail(ActionError::stale_target,"Malformed birth/final/namespace metric; no partial snapshot commit");
            for(const auto& k:seen)if(k.kernel_tgid==r.kernel.kernel_tgid&&k.leader_start_boottime_ns==r.kernel.leader_start_boottime_ns)return p.fail(ActionError::stale_target,"Duplicate birth in snapshot");
            seen.push_back(r.kernel);
            auto found=next.end();for(auto it=next.begin();it!=next.end();++it)if(it->raw.kernel.kernel_tgid==r.kernel.kernel_tgid&&it->raw.kernel.leader_start_boottime_ns==r.kernel.leader_start_boottime_ns){found=it;break;}
            if(found!=next.end()){
                if(!p.prefix(found->raw,r)||!p.execs(found->raw,r)||(found->raw.final_monotonic_ns&&!p.finals(found->raw,r)))return p.fail(ActionError::stale_target,"Immutable birth/exec/final changed; no reassignment or counter rewrite");
                found->raw=r;p.update(*found);
            }else{
                if(next.size()>=p.options.capacity)return p.fail(ActionError::invalid_argument,"Retained birth capacity exceeded");
                Impl::Row row;row.raw=r;row.view.key={p.options.capture_id,p.meta.boot,r.kernel.kernel_tgid,r.kernel.kernel_real_uid,r.kernel.leader_start_boottime_ns};
                row.view.startup_entry_id={{},Support::unsupported,"No explicit startup-entry association"};p.update(row);next.push_back(std::move(row));
            }
        }
        auto note=s.pid_mapping_note;
        if(!p.owner_valid())return p.fail(ActionError::stale_target,"Caller/fork/context changed during snapshot; no commit");
        ActionResult ok{ActionError::none,"Birth snapshot accepted without tree sums/app inference"};
        p.rows.swap(next);p.meta.quality_flags=s.quality_flags;p.meta.publishing=s.publishing;p.meta.creator_exited=s.creator_exited;p.meta.pid_mapping_enabled=s.pid_mapping_enabled;p.meta.pid_mapping_note.swap(note);p.mode_seen=true;
        return ok;
    }catch(...){return p.fail(ActionError::system_error,"Birth observer poll exception; last accepted observations retained");}
}
ActionResult LinuxStartupBirthObserver::associate(const StartupBirthProcessKey& key,const std::string& id){
    std::lock_guard lock(impl_->mutex);auto& p=*impl_;
    if(!text(id,32768))return {ActionError::invalid_argument,"Invalid/excessive startup entry id"};
    try{
        if(!p.owner)return {ActionError::stale_target,"No capture owner for association"};
        if(!p.owner_valid())return p.fail(ActionError::stale_target,"Association caller/fork/context changed; inherited source references closed");
        for(auto& r:p.rows)if(r.view.key==key){
            if(r.view.startup_entry_id.value)return {*r.view.startup_entry_id.value==id?ActionError::none:ActionError::stale_target,"Existing binding is immutable; no reassignment"};
            Metric<std::string> binding{id,Support::estimated,"Caller-declared association only; no executable identity or automatic-startup cause proof"};
            r.view.startup_entry_id=std::move(binding);return {ActionError::none,"Retained child explicitly associated; no process-tree aggregation"};
        }
        return {ActionError::stale_target,"Exact retained capture/boot/kernel child key required, not a bare PID"};
    }catch(...){return {ActionError::system_error,"Association callback/allocation failed"};}
}
void LinuxStartupBirthObserver::close(){std::lock_guard lock(impl_->mutex);impl_->stop();if(impl_->meta.state==StartupBirthObserverState::recording)impl_->meta.state=StartupBirthObserverState::closed;}
StartupBirthObserverReport LinuxStartupBirthObserver::report()const{std::lock_guard lock(impl_->mutex);auto out=impl_->meta;out.observations.reserve(impl_->rows.size());for(const auto& r:impl_->rows)out.observations.push_back(r.view);return out;}
}
