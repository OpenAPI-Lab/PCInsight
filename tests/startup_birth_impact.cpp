#include "pcinsight/startup_birth_impact.hpp"
#include <cassert>
#include <iostream>
#include <limits>
using namespace pcinsight;
using namespace std::chrono_literals;
namespace {
StartupBirthObserverReport frame(){
    StartupBirthObserverReport r;r.capture_id="capture";r.boot={1,2};r.state=StartupBirthObserverState::closed;r.final_resources_enabled=true;
    StartupBirthObservation o;o.key={r.capture_id,r.boot,100,0,10};o.birth_monotonic_ns=1000;o.final_monotonic_ns=2000;
    o.startup_entry_id={"private.desktop",Support::estimated,"explicit declaration"};
    o.cpu_time={10ns,Support::available,"native release CPU"};o.read_bytes={20,Support::available,"native read"};o.write_bytes={30,Support::available,"native write"};
    r.observations.push_back(o);return r;
}
}
int main(){
    auto r=frame();const auto key=r.observations[0].key;
    auto measured=measure_startup_birth_impact(r,key);assert(measured&&measured.value->released_within_window);
    const auto& m=*measured.value;
    assert(m.key==key&&m.begin_monotonic_ns==1000&&m.deadline_monotonic_ns==60000001000ULL&&m.release_monotonic_ns==2000);
    assert(m.startup_entry_id.value=="private.desktop"&&m.startup_entry_id.support==Support::estimated&&m.cpu_time.value==10ns&&m.disk_bytes.value==50);
    assert(m.cpu_time.support==Support::estimated&&m.disk_bytes.support==Support::estimated&&!m.impact.value&&!m.automatic_startup_origin.value);
    assert(r.observations[0].cpu_time.value==10ns&&r.observations[0].startup_entry_id.note=="explicit declaration"); // No mutation/overlay.
    StartupImpactPolicy policy;policy.medium_cpu=1ns;policy.high_cpu=10ns;policy.medium_disk_bytes=1;policy.high_disk_bytes=50;
    assert(!measure_startup_birth_impact(r,key,1000ns,policy).value->impact.value); // EXACT thresholds are not high.
    r.observations[0].cpu_time.value=11ns;measured=measure_startup_birth_impact(r,key,1000ns,policy);
    assert(measured.value->impact.value==StartupImpact::high&&measured.value->impact.support==Support::estimated);
    r.observations[0].cpu_time={{},Support::permission_required,"denied"};
    measured=measure_startup_birth_impact(r,key,1000ns,policy);assert(!measured.value->impact.value&&measured.value->impact.support==Support::permission_required&&!measured.value->cpu_time.value);
    r.observations[0].write_bytes.value=31;measured=measure_startup_birth_impact(r,key,1000ns,policy);
    assert(measured.value->impact.value==StartupImpact::high&&measured.value->disk_bytes.value==51); // High disk dominates unknown CPU.
    r=frame();r.observations[0].cpu_time.value=11ns;r.observations[0].read_bytes={{},Support::permission_required,"denied"};
    measured=measure_startup_birth_impact(r,key,1000ns,policy);assert(measured.value->impact.value==StartupImpact::high&&!measured.value->disk_bytes.value);
    r=frame();r.observations[0].cpu_time.value=0ns;r.observations[0].read_bytes.value=0;r.observations[0].write_bytes.value=0;
    measured=measure_startup_birth_impact(r,key,1000ns,policy);assert(measured.value->cpu_time.value==0ns&&measured.value->disk_bytes.value==0&&!measured.value->impact.value); // Zero != complete low.
    r=frame();r.observations[0].read_bytes.value=std::numeric_limits<std::uint64_t>::max();r.observations[0].write_bytes.value=1;
    measured=measure_startup_birth_impact(r,key,1000ns,policy);assert(!measured.value->disk_bytes.value&&!measured.value->impact.value);
    r.observations[0].cpu_time.value=11ns;measured=measure_startup_birth_impact(r,key,1000ns,policy);
    assert(!measured.value->disk_bytes.value&&measured.value->impact.value==StartupImpact::high); // Overflow does not erase valid CPU evidence.
    r.observations[0].write_bytes.value=0;measured=measure_startup_birth_impact(r,key,1000ns,policy);
    assert(measured.value->disk_bytes.value==std::numeric_limits<std::uint64_t>::max()&&measured.value->impact.value==StartupImpact::high);
    r=frame();r.observations[0].cpu_time.value=11ns;r.observations[0].final_monotonic_ns=2001;
    measured=measure_startup_birth_impact(r,key,1000ns,policy);assert(measured&&!measured.value->released_within_window&&!measured.value->cpu_time.value&&!measured.value->disk_bytes.value&&!measured.value->impact.value&&measured.value->release_monotonic_ns==2001); // Late must not be prorated.
    r.observations[0].final_monotonic_ns.reset();r.observations[0].cpu_time={};r.observations[0].read_bytes={};r.observations[0].write_bytes={};
    measured=measure_startup_birth_impact(r,key);assert(measured&&!measured.value->released_within_window&&!measured.value->cpu_time.value&&!measured.value->disk_bytes.value);
    r=frame();r.observations[0].startup_entry_id={};assert(measure_startup_birth_impact(r,key).error==ActionError::not_found);
    for(unsigned n=0;n<6;++n){r=frame();auto other=key;
        if(n==0)other.capture_id="other";if(n==1)++other.boot[0];if(n==2)++other.kernel_tgid;if(n==3)++other.kernel_real_uid;if(n==4)++other.kernel_start_boottime_ns;if(n==5)r.observations.clear();
        assert(measure_startup_birth_impact(r,other).error==ActionError::stale_target);
    }
    for(unsigned n=0;n<19;++n){r=frame();auto& o=r.observations[0];
        if(n==0)o.birth_monotonic_ns=0;if(n==1)o.final_monotonic_ns=999;if(n==2)r.final_resources_enabled=false;
        if(n==3)o.cpu_time.value=-1ns;if(n==4)o.cpu_time.support=Support::unsupported;if(n==5)o.write_bytes.note.resize(4097,'x');
        if(n==6)o.startup_entry_id.value=std::string("a\0b",3);if(n==7)o.final_monotonic_ns.reset();if(n==8)o.read_bytes.support=static_cast<Support>(99);
        if(n==9)r.observations.push_back(o);if(n==10)r.observations.resize(65,o);if(n==11)r.quality_flags=128;
        if(n==12)r.state=StartupBirthObserverState::idle;if(n==13)r.state=static_cast<StartupBirthObserverState>(99);
        if(n==14)r.publishing=65;if(n==15)o.birth_monotonic_ns=std::numeric_limits<std::uint64_t>::max()-1,o.final_monotonic_ns=o.birth_monotonic_ns;
        if(n==16)o.startup_entry_id.value="";if(n==17)o.cpu_time={std::nullopt,Support::available,"contradictory"};if(n==18)o.startup_entry_id.note=std::string("a\0b",3);
        assert(measure_startup_birth_impact(r,key).error==ActionError::invalid_argument);
    }
    r=frame();assert(!measure_startup_birth_impact(r,key,0ns)&&!measure_startup_birth_impact(r,key,-1ns)&&!measure_startup_birth_impact(r,key,1h+1ns));
    assert(measure_startup_birth_impact(r,key,1h));
    r.observations[0].birth_monotonic_ns=std::numeric_limits<std::uint64_t>::max()-1000;r.observations[0].final_monotonic_ns=std::numeric_limits<std::uint64_t>::max();
    measured=measure_startup_birth_impact(r,key,1000ns);assert(measured&&measured.value->released_within_window&&measured.value->deadline_monotonic_ns==std::numeric_limits<std::uint64_t>::max());
    r=frame();
    for(unsigned n=0;n<4;++n){auto p=policy;if(n==0)p.medium_cpu=0ns;if(n==1)p.high_cpu=0ns;if(n==2)p.medium_disk_bytes=0;if(n==3)p.high_disk_bytes=0;assert(!measure_startup_birth_impact(r,key,1000ns,p));}
    // Global quality/failure does not erase an already retained high lower bound.
    r.state=StartupBirthObserverState::failed;r.quality_flags=127;r.publishing=64;r.automatic_startup_origin={true,Support::available,"untrusted input"};
    r.observations[0].cpu_time.value=11ns;measured=measure_startup_birth_impact(r,key,1000ns,policy);
    assert(measured.value->impact.value==StartupImpact::high&&!measured.value->automatic_startup_origin.value);
    // Same app, different child: NEVER add overlapping group IO across rows.
    r=frame();r.observations.push_back(r.observations[0]);++r.observations[1].key.kernel_start_boottime_ns;r.observations[1].write_bytes.value=999;
    assert(measure_startup_birth_impact(r,key).value->disk_bytes.value==50);
    std::cout<<"birth impact: exact key/binding/window/deadline/unknown/high-only/overflow/no tree sum/pure projection passed\n";
}
