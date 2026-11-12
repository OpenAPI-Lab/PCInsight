#include "pcinsight/startup_observer.hpp"
#include "pcinsight/monitor.hpp"
#include <cassert>
#include <limits>
#include <stdexcept>
#include <thread>
#ifdef __linux__
#include <unistd.h>
#endif
using namespace pcinsight;
using namespace std::chrono_literals;
StartupProcessSample baseline(std::uint32_t pid=10) {
    StartupProcessSample s;s.pid=pid;s.linux_start_ticks=100;s.native_context="boot:namespace";
    s.sampled_at=std::chrono::steady_clock::time_point(1s);
    s.cumulative_cpu_time={0ns,Support::available,"zero"};s.cumulative_disk_bytes={0,Support::available,"zero"};return s;
}
int main() {
    for(const auto duration:{0ms,-1ms,std::chrono::milliseconds::max()}) {
        try {StartupImpactObserver invalid(duration);assert(false);}catch(const std::invalid_argument&){}
    }
    auto first=baseline();auto current=first;current.sampled_at+=50ms;current.cumulative_cpu_time.value=20ms;
    StartupImpactObserver observer(100ms);
    assert(observer.register_process("app",first));
    assert(!observer.register_process("app",first));
    assert(observer.register_process("other-app",first).error==ActionError::stale_target);
    assert(observer.observe_sample(current));
    auto rows=observer.observations();assert(rows.size()==1 && rows[0].sample_count==2 && !rows[0].closed);
    assert(rows[0].measurement && rows[0].measurement->cpu_time.value==20ms && !rows[0].measurement->impact.value);
    current.sampled_at+=100ms;current.cumulative_cpu_time.value=30ms;assert(observer.observe_sample(current));
    rows=observer.observations();assert(rows[0].closed && rows[0].window_complete && !rows[0].sampling_gap);
    assert(rows[0].measurement->impact.value==StartupImpact::low && rows[0].measurement->observation_interval==150ms);
    assert(observer.poll().attempted==0 && !observer.observe_sample(current));
    assert(observer.register_process("app",baseline(11))); // distinct process, separate record, no aggregation
    assert(observer.observations().size()==2);
    observer.close();observer.close();assert(observer.poll().attempted==0 && !observer.register_process("late",baseline(12)));
    StartupImpactObserver short_window(1s);
    assert(short_window.register_process("app",first));current=first;current.sampled_at+=50ms;
    assert(short_window.observe_sample(current));short_window.close();rows=short_window.observations();
    assert(rows[0].closed && !rows[0].window_complete && !rows[0].measurement->impact.value);
    StartupImpactObserver high(1s);
    assert(high.register_process("app",first));current.cumulative_cpu_time.value=1100ms;
    assert(high.observe_sample(current));high.close();assert(high.observations()[0].measurement->impact.value==StartupImpact::high);
    unsigned reads=0;StartupProcessSampleResult next{std::nullopt,ActionError::permission_denied,"denied"};
    StartupImpactObserver gapped(100ms,{},[&](auto pid,auto ticks){++reads;assert(pid==10 && ticks==100);return next;});
    assert(gapped.register_process("app",first));auto report=gapped.poll(first.sampled_at+50ms);
    assert(report.attempted==1 && report.failed==1 && !report.updated && reads==1);
    assert(gapped.observations()[0].sampling_gap && !gapped.observations()[0].closed);
    current=first;current.sampled_at+=150ms;
    next={current,ActionError::none,"ok"};assert(gapped.poll(current.sampled_at).updated==1);
    rows=gapped.observations();assert(rows[0].closed && !rows[0].window_complete && rows[0].last_error==ActionError::none && !rows[0].measurement->impact.value);
    next={std::nullopt,ActionError::not_found,"gone"};
    StartupImpactObserver exited(1s,{},[&](auto,auto){return next;});assert(exited.register_process("app",first));
    assert(exited.poll().failed==1 && exited.observations()[0].closed && exited.poll().attempted==0);
    StartupImpactObserver reused(1s,{},[&](auto,auto){auto wrong=first;wrong.linux_start_ticks=101;wrong.sampled_at+=10ms;return StartupProcessSampleResult{wrong,ActionError::none,"wrong process"};});
    assert(reused.register_process("app",first));assert(reused.poll().failed==1);
    assert(reused.observations()[0].closed && reused.observations()[0].last_error==ActionError::stale_target);
    StartupImpactObserver throwing(1s,{},[](auto,auto)->StartupProcessSampleResult{throw std::runtime_error("secret exception text");});
    assert(throwing.register_process("app",first));assert(throwing.poll(first.sampled_at+50ms).failed==1 && throwing.observations()[0].last_error==ActionError::system_error);
    assert(!throwing.observations()[0].closed);
    assert(throwing.poll(first.sampled_at+1s).failed==1 && throwing.observations()[0].closed);
    assert(throwing.poll().attempted==0);
    // Persistent denial stops at the deadline without inventing a zero delta.
    reads=0;
    StartupImpactObserver denied(100ms,{},[&](auto,auto){++reads;return StartupProcessSampleResult{std::nullopt,ActionError::permission_denied,"denied"};});
    assert(denied.register_process("app",first));
    assert(denied.poll(first.sampled_at+99ms).failed==1 && !denied.observations()[0].closed);
    assert(denied.poll(first.sampled_at+100ms).failed==1 && reads==2);
    rows=denied.observations();assert(rows[0].closed && !rows[0].window_complete && rows[0].sampling_gap);
    assert(!rows[0].measurement && rows[0].sample_count==1 && rows[0].last_error==ActionError::permission_denied);
    assert(denied.poll(std::chrono::steady_clock::time_point::max()).attempted==0 && reads==2);
    // Retain a measured high lower bound if the final read fails.
    StartupImpactObserver expired_high(100ms,{},[](auto,auto){return StartupProcessSampleResult{std::nullopt,ActionError::system_error,"read failure"};});
    assert(expired_high.register_process("app",first));current=first;current.sampled_at+=50ms;current.cumulative_cpu_time.value=1100ms;
    assert(expired_high.observe_sample(current));
    assert(expired_high.poll(first.sampled_at+100ms).failed==1);
    rows=expired_high.observations();assert(rows[0].closed && !rows[0].window_complete && rows[0].measurement->impact.value==StartupImpact::high);
    assert(rows[0].measurement->observation_interval==50ms && rows[0].measurement->cpu_time.value==1100ms);
    // Final successful read is admitted before deadline closure; distinct
    // baselines expire independently. Extreme injected times do not overflow.
    auto final_sample=first;final_sample.sampled_at+=100ms;
    StartupImpactObserver independent(100ms,{},[&](auto pid,auto){auto s=final_sample;s.pid=pid;return StartupProcessSampleResult{s,ActionError::none,"ok"};});
    assert(independent.register_process("app",first));auto later=baseline(11);later.sampled_at+=50ms;
    assert(independent.register_process("later",later));
    assert(independent.poll(first.sampled_at+100ms).updated==2);
    rows=independent.observations();assert(rows[0].closed && rows[0].window_complete);
    assert(!rows[1].closed && !rows[1].window_complete);
    assert(independent.poll(std::chrono::steady_clock::time_point::max()).failed==1);
    assert(independent.observations()[1].closed && independent.poll().attempted==0);
    // Default clock also bounds a failed native/synthetic reader.
    auto old=first;old.sampled_at=std::chrono::steady_clock::now()-1s;
    StartupImpactObserver real_clock(1ms,{},[](auto,auto){return StartupProcessSampleResult{std::nullopt,ActionError::system_error,"unavailable"};});
    assert(real_clock.register_process("app",old));assert(real_clock.poll().failed==1);
    assert(real_clock.observations()[0].closed && !real_clock.observations()[0].window_complete);
    assert(real_clock.poll().attempted==0);
    StartupImpactObserver reset(100ms);assert(reset.register_process("app",first));
    current=first;current.sampled_at+=20ms;current.cumulative_cpu_time.value=20ms;assert(reset.observe_sample(current));
    current.sampled_at+=20ms;current.cumulative_cpu_time.value=10ms;assert(reset.observe_sample(current));
    current.sampled_at+=100ms;current.cumulative_cpu_time.value=30ms;assert(reset.observe_sample(current));
    assert(reset.observations()[0].sampling_gap && !reset.observations()[0].measurement->impact.value);
    StartupImpactObserver partial(100ms);assert(partial.register_process("app",first));
    current=first;current.sampled_at+=150ms;current.cumulative_disk_bytes={std::nullopt,Support::permission_required,"denied"};
    assert(partial.observe_sample(current));assert(partial.observations()[0].sampling_gap && !partial.observations()[0].window_complete);
    assert(partial.observations()[0].measurement->impact.support==Support::permission_required);
    StartupImpactObserver invalid_sample(1s);assert(invalid_sample.register_process("app",first));
    for(bool high:{false,true}){
        StartupImpactObserver partial_final(100ms);assert(partial_final.register_process("partial",first));
        auto final=first;final.sampled_at+=150ms;final.cumulative_cpu_time.note="CPU LOWER BOUND";final.cumulative_disk_bytes.value=high?4000001:0;
        assert(partial_final.finish_partial_process(final));auto view=partial_final.observations().front();
        assert(view.closed&&view.sampling_gap&&!view.window_complete&&view.measurement);
        assert(view.measurement->cpu_time.note.find("LOWER BOUND")!=std::string::npos);
        assert(high?view.measurement->impact.value==StartupImpact::high:!view.measurement->impact.value);
        assert(!partial_final.finish_partial_process(final));
    }
    current=first;current.sampled_at+=1ms;current.cumulative_cpu_time.note=std::string(4097,'x');assert(!invalid_sample.observe_sample(current));
    assert(invalid_sample.observations()[0].sample_count==1);
    current=first;current.sampled_at=std::chrono::steady_clock::time_point::max();assert(!invalid_sample.register_process("overflow",current));
    // Bounded metadata: long IDs consume the shared budget, not unbounded RAM.
    StartupImpactObserver bounded;
    unsigned added=0;
    for(unsigned i=1;i<=4096;++i){auto s=baseline(i);if(!bounded.register_process(std::string(32768,'a'),s))break;++added;}
    assert(added>0 && added<4096);
    auto threaded_sample=first;
    StartupImpactObserver threaded(1s,{},[&](auto,auto){threaded_sample.sampled_at+=1ms;return StartupProcessSampleResult{threaded_sample,ActionError::none,"ok"};});
    assert(threaded.register_process("app",first));
    std::thread polling([&]{for(unsigned i=0;i<40;++i)assert(threaded.poll(first.sampled_at).updated==1);});
    for(unsigned i=0;i<40;++i){const auto visible=threaded.observations();assert(visible.size()==1 && visible[0].sample_count>=1 && visible[0].sample_count<=41);}
    polling.join();assert(threaded.observations()[0].sample_count==41);
#ifdef __linux__
    Monitor monitor;SampleOptions options;options.include_disks=options.include_networks=options.include_gpus=false;
    const auto snapshot=monitor.sample(options);std::uint64_t ticks=0;
    for(const auto& p:snapshot.processes)if(p.pid==static_cast<std::uint32_t>(getpid()))ticks=p.linux_start_ticks.value.value_or(0);
    assert(ticks);const auto own=read_startup_process_sample(getpid(),ticks);assert(own);
    StartupImpactObserver native(1ms);assert(native.register_process("private-observer-test-not-a-real-startup",*own.value));
    std::this_thread::sleep_for(5ms);report=native.poll();assert(report.attempted==1 && report.updated==1 && !report.failed);
    rows=native.observations();assert(rows[0].closed && rows[0].window_complete && rows[0].measurement);
    assert(rows[0].measurement->cpu_time.value && rows[0].measurement->disk_bytes.value && rows[0].measurement->impact.value);
    assert(native.poll().attempted==0);
#endif
}
