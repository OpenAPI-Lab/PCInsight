#include "../src/sampler_history.hpp"
#include <cassert>
#include <iostream>
#include <limits>
#include <thread>
using namespace pcinsight;
using namespace pcinsight::detail;
using namespace std::chrono_literals;
using Clock = std::chrono::steady_clock;
const auto origin = Clock::time_point{} + 1h;
SamplerSample point(std::chrono::milliseconds start, std::chrono::milliseconds cost = 0ms,
                    int wall = 0) {
    SystemSnapshot snapshot;
    snapshot.captured_at = std::chrono::system_clock::time_point{} + std::chrono::seconds(wall);
    return {origin + start, origin + start + cost, std::move(snapshot)};
}
template<class F> void rejects(F f) {
    bool rejected=false; try { f(); } catch(const std::invalid_argument&) { rejected=true; }
    assert(rejected);
}
int main() {
    assert(sampler_capacity(1s,0)==61 && sampler_capacity(100ms,0)==601);
    assert(sampler_capacity(1ms,0)==60001 && sampler_capacity(1h,0)==1);
    assert(sampler_capacity(100ms,2)==2);
    rejects([]{sampler_interval(0ms);}); rejects([]{sampler_interval(-1ms);});
    rejects([]{sampler_interval(std::chrono::milliseconds::max());});
    rejects([]{SamplerHistory invalid(0);});
    assert(sampler_cutoff(Clock::time_point::min())==Clock::time_point::min());
    assert(sampler_cutoff(Clock::time_point::min()+60s)==Clock::time_point::min());
    assert(sampler_deadline(Clock::time_point::max()-1s,2s)==Clock::time_point::max());
    assert(sampler_deadline(origin,2s)==origin+2s);

    SamplerHistory h(sampler_capacity(100ms,0));
    // 0..60s inclusive: 601 points, not the previous fixed60. Wall jumps ignored.
    for (int n=0;n<=600;++n) h.append(point(n*100ms,0ms,n%2 ? -100000 : 100000));
    auto rows=h.timed_history(origin+60s); assert(rows.size()==601);
    assert(rows.front().started_at==origin && rows.back().started_at==origin+60s);
    assert(h.history(origin+60s).size()==rows.size());
    rows=h.timed_history(origin+60s+1ns); assert(rows.size()==600);
    assert(rows.front().started_at==origin+100ms);
    h.append(point(60100ms,10ms));
    rows=h.timed_history(origin+60110ms); assert(rows.size()==600);
    assert(rows.front().started_at==origin+200ms);
    // Read-time expiry while stopped, but latest remains explicitly stale.
    assert(h.timed_history(origin+121s).empty()); assert(h.history(origin+121s).empty());
    assert(h.latest_timed()->started_at==origin+60100ms && h.latest());
    h.clear(); assert(!h.latest() && !h.latest_timed());

    // Variable collection costs, sparse sampling and a >60s collection.
    h.append(point(0ms,2s)); h.append(point(7s,4s)); h.append(point(30s,15s));
    h.append(point(59s,5s)); rows=h.timed_history(origin+64s);
    assert(rows.size()==3 && rows.front().started_at==origin+7s);
    h.append(point(70s,61s)); assert(h.timed_history(origin+131s).empty());
    assert(h.latest_timed()->started_at==origin+70s);
    h.append(point(132s,1s)); rows=h.timed_history(origin+133s);
    assert(rows.size()==1 && rows.front().started_at==origin+132s);
    const auto committed=h.latest_timed()->started_at;
    rejects([&]{h.append(point(134s,-1s));});
    rejects([&]{h.append(point(132s,3s));}); // overlaps previous collection
    assert(h.latest_timed()->started_at==committed);

    SamplerHistory capped(2);
    capped.append(point(0ms)); capped.append(point(1s)); capped.append(point(2s));
    rows=capped.timed_history(origin+2s); assert(rows.size()==2 && rows.front().started_at==origin+1s);
    assert(capped.timed_history(origin+62s).size()==1); // exact cutoff is included
    assert(capped.timed_history(origin+62s+1ns).empty());

    // Actual worker: envelope ordering, selected groups, concurrent reads, restart.
    SampleOptions options;
    options.include_processes=false; options.include_app_history=false;
    options.include_linux_process_descriptions=false; options.include_disks=false;
    options.include_networks=false; options.include_gpus=false;
    options.include_services=false; options.include_startup_entries=false;
    options.include_user_sessions=false;
    Sampler sampler(10ms,0,options);
    assert(!sampler.latest_timed() && sampler.timed_history().empty());
    sampler.start();
    const auto deadline=Clock::now()+10s;
    std::size_t count=0;
    while(sampler.running() && Clock::now()<deadline) {
        count=sampler.timed_history().size();
        if(count>=3) break;
        std::this_thread::sleep_for(5ms);
    }
    sampler.stop(); assert(count>=3 && sampler.last_error().empty());
    rows=sampler.timed_history(); assert(rows.size()>=3);
    for(std::size_t n=0;n<rows.size();++n) {
        assert(rows[n].started_at<=rows[n].finished_at && rows[n].finished_at<=Clock::now());
        assert(rows[n].snapshot.processes.empty());
        if(n) {assert(rows[n-1].finished_at<=rows[n].started_at);
            assert(rows[n].started_at-rows[n-1].started_at>=10ms);}
    }
    assert(sampler.latest()->captured_at==sampler.latest_timed()->snapshot.captured_at);
    assert(sampler.history().size()==rows.size());
    const auto restart=Clock::now(); sampler.start();
    while(sampler.running() && !sampler.latest_timed() && Clock::now()<deadline)
        std::this_thread::sleep_for(5ms);
    sampler.stop(); assert(sampler.latest_timed() && sampler.latest_timed()->started_at>=restart);
    for(const auto& s:sampler.timed_history()) assert(s.started_at>=restart);
    assert(sampler.last_error().empty());
    std::cout << "Sampler: inclusive monotonic60s, 601 samples/100ms, variable cost, sparse/slow/stopped expiry, wall jumps, capacity, overflow bounds and native restart/envelopes passed\n";
}
