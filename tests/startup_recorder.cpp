#include "../src/startup_recorder.hpp"
#include <cassert>
#include <iostream>
#include <stdexcept>
using namespace pcinsight;using namespace pcinsight::detail;using namespace std::chrono_literals;
struct Source {
    std::chrono::steady_clock::time_point now=std::chrono::steady_clock::time_point{}+1s;
    std::string context="private-boot-pidns";unsigned snapshots=0,reads=0,context_reads=0;
    bool partial=false,failed_batch=false,throw_scan=false,denied=false,late=false,context_bad=false,claim_origin=false,vanished=false;
    StartupProcessSample sample(std::uint32_t pid,bool baseline=false){StartupProcessSample s;s.pid=pid;s.linux_start_ticks=10;s.native_context=context;s.sampled_at=now;s.cumulative_cpu_time={baseline?0ns:std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::milliseconds(reads)),Support::estimated,"fixture"};s.cumulative_disk_bytes={0,Support::estimated,"fixture"};return s;}
    StartupRecorderApi api(){return {
        [this]{++context_reads;if(context_bad)return StartupProcessSampleResult{{},ActionError::permission_denied,"fixture"};return StartupProcessSampleResult{sample(1),ActionError::none,{}};},
        [this]{++snapshots;if(throw_scan)throw std::runtime_error("private exception text must not leak");SystemSnapshot s;ProcessInfo p;p.pid=42;p.linux_start_ticks={10,Support::available,{}};s.processes.push_back(p);if(late){p.pid=43;s.processes.push_back(p);}return s;},
        [this](const SystemSnapshot& s){StartupCandidateReport r;r.complete=!partial;r.unavailable=partial?1:0;r.examined=s.processes.size();r.error=failed_batch?ActionError::system_error:ActionError::none;
            for(const auto& p:s.processes){StartupProcessCandidate c;c.startup_entry_id="/private/app.desktop";c.baseline=sample(p.pid,true);if(claim_origin)c.automatic_startup_origin={true,Support::available,"untrusted claim"};r.candidates.push_back(c);}return r;},
        [this](std::uint32_t pid,std::uint64_t){++reads;now+=1ns;if(vanished)return StartupProcessSampleResult{{},ActionError::not_found,"fixture"};if(denied)return StartupProcessSampleResult{{},ActionError::permission_denied,"fixture"};return StartupProcessSampleResult{sample(pid),ActionError::none,{}};},
        [this]{return now;},[]{return std::chrono::system_clock::time_point{}+1h;}};}
};
StartupRecorderOptions options(){StartupRecorderOptions o;o.capture_id="private";o.accept_estimated_gio_associations=true;o.duration=100ms;o.scan_interval=20ms;return o;}
int main(){
    Source s;auto recorder=StartupRecorderAccess::make(s.api());auto o=options();o.accept_estimated_gio_associations=false;
    assert(!recorder.start(o)&&s.context_reads==0&&s.snapshots==0&&recorder.report().state==StartupRecorderState::idle);
    o=options();o.scan_interval=0ms;assert(!recorder.start(o)&&s.context_reads==0);o=options();assert(recorder.start(o));
    auto r=recorder.report();assert(r.state==StartupRecorderState::recording&&r.registered==1&&r.scans==1&&r.observations.size()==1&&!r.automatic_startup_origin.value);
    assert(!recorder.start(o)&&!recorder.save({}));const auto reads=s.reads;assert(recorder.poll()&&s.reads==reads&&s.snapshots==1); // Not due.
    s.now=r.next_scan;assert(recorder.poll());assert(recorder.report().registered==1&&s.snapshots==2); // Filtered, not registered twice.
    s.late=true;s.now=recorder.report().next_scan;assert(recorder.poll());assert(recorder.report().registered==2);
    s.now=recorder.report().deadline;assert(recorder.poll());r=recorder.report();assert(r.state==StartupRecorderState::closed&&r.observations.size()==2&&r.observations[0].window_complete&&!r.observations[1].window_complete);
    assert(r.observations[0].closed&&r.observations[1].closed&&r.observations[1].measurement&&!r.observations[1].measurement->impact.value);
    const auto stopped=s.reads;assert(recorder.poll()&&s.reads==stopped);recorder.close();assert(s.reads==stopped);
    auto moved=std::move(recorder);assert(moved.report().observations.size()==2&&!recorder.start(o));
    for(unsigned mode=0;mode<7;++mode){Source f;auto c=StartupRecorderAccess::make(f.api());assert(c.start(options()));
        f.now=c.report().next_scan;
        if(mode==0)f.now=c.report().started_at-1ns;
        if(mode==1)f.context="changed-context";
        if(mode==2)f.context_bad=true;
        if(mode==3)f.throw_scan=true;
        if(mode==4)f.partial=true;
        if(mode==5)f.failed_batch=true;
        if(mode==6)f.denied=true;
        const auto result=c.poll();const auto report=c.report();
        if(mode<4){assert(!result&&report.state==StartupRecorderState::failed&&report.observations.front().closed&&!c.save({}));}
        else {assert(result&&report.state==StartupRecorderState::recording);if(mode<6)assert(report.partial_scans==1);else assert(report.poll_failed>0&&report.observations.front().sampling_gap);}
    }
    {Source f;f.failed_batch=true;auto c=StartupRecorderAccess::make(f.api());assert(c.start(options()));assert(c.report().registered==0&&c.report().partial_scans==1);c.close();assert(c.report().state==StartupRecorderState::closed&&c.report().observations.empty());}
    {Source f;f.claim_origin=true;auto c=StartupRecorderAccess::make(f.api());assert(c.start(options()));assert(c.report().rejected==1&&c.report().registered==0);}
    {Source f;auto c=StartupRecorderAccess::make(f.api());assert(c.start(options()));auto time=f.now;f.now=c.report().deadline+50ms;assert(c.poll());assert(c.report().state==StartupRecorderState::closed&&f.snapshots==1);(void)time;}
    {Source f;auto c=StartupRecorderAccess::make(f.api());assert(c.start(options()));const auto count=f.reads;c.close();assert(c.report().state==StartupRecorderState::closed&&f.reads==count&&!c.report().observations.front().window_complete);}
    {Source f;f.now=std::chrono::steady_clock::time_point::max()-1ms;auto c=StartupRecorderAccess::make(f.api());assert(!c.start(options())&&c.report().state==StartupRecorderState::failed);}
    {Source f;auto c=StartupRecorderAccess::make(f.api());assert(c.start(options()));c.close();f.context="different-at-save";assert(c.save({}).error==ActionError::stale_target);}
    {Source f;auto api=f.api();api.candidates=[](const SystemSnapshot&){StartupCandidateReport r;r.examined=4097;return r;};auto c=StartupRecorderAccess::make(std::move(api));assert(!c.start(options())&&c.report().state==StartupRecorderState::failed);}
    {Source f;auto c=StartupRecorderAccess::make(f.api());auto invalid=options();invalid.capture_id=std::string(257,'x');assert(!c.start(invalid)&&!f.context_reads);invalid=options();invalid.duration=std::chrono::hours(2);assert(!c.start(invalid)&&!f.context_reads);}
    {Source f;auto c=StartupRecorderAccess::make(f.api());assert(c.start(options()));auto active=std::move(c);f.now=active.report().next_scan;assert(active.poll()&&active.report().state==StartupRecorderState::recording);active.close();}
    LinuxStartupRecorder native;StartupRecorderOptions rejected;assert(native.start(rejected).error==ActionError::permission_denied&&native.report().state==StartupRecorderState::idle);
    for(unsigned mode=0;mode<12;++mode){
        Source f;auto api=f.api();bool publish=false;unsigned calls=0,active=0;
        if(mode==9||mode==11){auto read=api.sample;api.sample=[&,read](auto pid,auto ticks){auto value=read(pid,ticks);if(value){if(mode==9)value.value->cumulative_disk_bytes.value=100;else value.value->cumulative_cpu_time={{},Support::permission_required,"unknown CPU"};}return value;};}
        struct Owned {unsigned& active;Owned(unsigned& n):active(n){++active;}~Owned(){--active;}};
        api.exit_disk=[&](const auto&)->Metric<StartupExitDiskReader>{
            ++calls;if(mode==4)throw std::runtime_error("attach failed");
            if(mode==5)return {{},Support::permission_required,"denied"};
            auto owned=std::make_shared<Owned>(active);
            return {StartupExitDiskReader{[&,owned]()->Metric<std::optional<detail::StartupExitResourceTotals>>{
                if(mode==6)throw std::runtime_error("reader failed");
                if(mode==10)return {std::optional<detail::StartupExitResourceTotals>{0},Support::estimated,"not exact capture"};
                if(!publish)return {std::optional<detail::StartupExitResourceTotals>{},Support::available,"pending"};
                return {std::optional<detail::StartupExitResourceTotals>{mode==1||mode==9?0:4000001},Support::available,"private IO"};
            }},Support::available,"attached"};
        };
        auto c=StartupRecorderAccess::make(std::move(api));auto opt=options();opt.enable_privileged_exit_disk_recovery=mode!=3;opt.max_exit_disk_captures=1;
        if(mode==8)f.late=true;
        assert(c.start(opt));auto initial=c.report();
        if(mode==3){assert(!calls&&!active);c.close();continue;} // Disabled means NO attach side effects.
        assert(calls==1&&initial.exit_disk_attempted==1);
        if(mode==4||mode==5||mode==6||mode==10){assert(!active&&c.report().exit_disk_unavailable==1);c.close();continue;}
        if(mode==8){assert(initial.exit_disk_skipped==1);c.close();assert(!active);continue;}
        assert(active==1);f.vanished=true;f.now=c.report().next_scan;assert(c.poll());
        auto waiting=c.report();assert(!waiting.observations.front().closed&&waiting.observations.front().sampling_gap&&active==1);
        const auto cpu=waiting.observations.front().measurement->cpu_time.value;
        f.now=c.report().next_scan;if(mode==7)f.context="different";else if(mode==2)f.now=c.report().deadline+1ms;
        publish=true;const auto result=c.poll();auto final=c.report();
        if(mode==7){assert(!result&&final.state==StartupRecorderState::failed&&!active);continue;}
        assert(result&&!active);const auto& row=final.observations.front();assert(row.closed&&row.sampling_gap&&!row.window_complete);
        if(mode==9){assert(!final.exit_disk_recovered&&final.exit_disk_unavailable==1&&row.measurement->disk_bytes.value==100);continue;}
        if(mode==2){assert(!final.exit_disk_recovered);continue;} // No late attribution after phase deadline.
        assert(final.exit_disk_recovered==1&&row.measurement&&row.measurement->disk_bytes.value==(mode==1?0:4000001));
        assert(row.measurement->cpu_time.value==cpu&&row.measurement->cpu_time.note.find("LOWER BOUND")!=std::string::npos);
        assert(mode==1?!row.measurement->impact.value:row.measurement->impact.value==StartupImpact::high);
        c.close();assert(!active);
    }
    {Source f;auto api=f.api();unsigned called=0;api.exit_disk=[&](const auto&)->Metric<StartupExitDiskReader>{++called;f.now+=150ms;return {{},Support::unsupported,"slow attach"};};auto c=StartupRecorderAccess::make(std::move(api));auto opt=options();opt.enable_privileged_exit_disk_recovery=true;assert(c.start(opt));auto view=c.report();assert(called==1&&view.state==StartupRecorderState::closed&&view.deadline-view.started_at==100ms);}
    {Source f;auto c=StartupRecorderAccess::make(f.api());auto opt=options();opt.enable_privileged_exit_disk_recovery=true;assert(!c.start(opt)&&!f.context_reads);}
    for(unsigned bound:{0U,33U}){Source f;auto api=f.api();api.exit_disk=[](const auto&)->Metric<StartupExitDiskReader>{assert(false);return {};};auto c=StartupRecorderAccess::make(std::move(api));auto opt=options();opt.enable_privileged_exit_disk_recovery=true;opt.max_exit_disk_captures=bound;assert(!c.start(opt)&&!f.context_reads);}
    {Source f;auto c=StartupRecorderAccess::make(f.api());assert(c.start(options()));f.now=c.report().started_at-1ns;c.close();assert(c.report().state==StartupRecorderState::failed&&!c.save({}));}
    {Source f;f.now=std::chrono::steady_clock::time_point{}-1s;auto c=StartupRecorderAccess::make(f.api());assert(c.start(options()));f.now=std::chrono::steady_clock::time_point::max()-1ms;c.close();assert(c.report().state==StartupRecorderState::closed&&c.save({}).error==ActionError::invalid_argument);}
    for(unsigned mode=0;mode<8;++mode){Source f;auto api=f.api();unsigned clocks=0;bool publish=false;if(mode==7)f.late=true;
        api.scheduler_sample=[&](auto pid,auto)->StartupProcessSampleResult{
            if(mode==3)return {{},ActionError::not_supported,"clock absent"};
            f.now+=1ns;auto sample=f.sample(pid);sample.cpu_counter=StartupCpuCounter::linux_scheduler_runtime;
            sample.cumulative_cpu_time={mode==1?0ns:10ms+std::chrono::milliseconds(clocks),Support::estimated,"scheduler fixture"};
            if(mode==5&&clocks==1)sample.cpu_counter=StartupCpuCounter::linux_procfs_ticks;
            if(mode==6)sample.native_context="wrong context";
            ++clocks;return {sample,ActionError::none,{}};
        };
        api.exit_disk=[&](const auto&)->Metric<StartupExitDiskReader>{return {StartupExitDiskReader{[&]()->Metric<std::optional<detail::StartupExitResourceTotals>>{
            if(!publish)return {std::optional<detail::StartupExitResourceTotals>{},Support::available,"pending"};
            detail::StartupExitResourceTotals total{0};total.scheduler_cpu_time={mode==1||mode==4?0ns:1200ms,Support::available,"same-kind final"};
            if(mode==2)total.scheduler_cpu_time={{},Support::permission_required,"unknown final"};
            return {std::optional<detail::StartupExitResourceTotals>{total},Support::available,"published"};
        }},Support::available,"attached"};};
        auto c=StartupRecorderAccess::make(std::move(api));auto opt=options();opt.enable_privileged_exit_disk_recovery=true;opt.max_exit_disk_captures=1;
        assert(c.start(opt));publish=true;f.now=c.report().next_scan;assert(c.poll());const auto view=c.report();const auto& row=view.observations.front();
        assert(view.exit_disk_recovered==1&&row.closed&&row.sampling_gap&&!row.window_complete&&row.measurement);
        if(mode==0||mode==5||mode==7){assert(row.measurement->cpu_time.value&&*row.measurement->cpu_time.value>1s&&row.measurement->impact.value==StartupImpact::high);assert(row.measurement->cpu_time.note.find("Recovered final scheduler-runtime")!=std::string::npos);}
        else {assert(!row.measurement->impact.value);if(mode==1)assert(row.measurement->cpu_time.value==0ns);else assert(row.measurement->cpu_time.note.find("LOWER BOUND")!=std::string::npos);}
        if(mode==5)assert(view.poll_failed==1&&row.sample_count==2);
        if(mode==7)assert(view.exit_disk_skipped==1&&!view.observations[1].sampling_gap&&view.poll_failed==0);
        c.close();
    }
    {Source f;auto api=f.api();api.scheduler_sample=[](auto,auto)->StartupProcessSampleResult{assert(false);return {};};auto c=StartupRecorderAccess::make(std::move(api));assert(c.start(options()));c.close();}
    std::cout<<"recorder: scheduling/consent/coverage/cleanup and scheduler CPU recovery/kind guard/zero/unknown/reset/fallback/skipped-reader passed\n";
}
