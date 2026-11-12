#include "pcinsight/startup_birth_overlay.hpp"
#include <cassert>
#include <iostream>
using namespace pcinsight;using namespace std::chrono_literals;
StartupBirthArchive capture(){
    StartupBirthArchive a;a.window=1000ns;a.policy={1ns,10ns,1,50};auto& c=a.capture;
    c.capture_id="capture";c.boot={1,2};c.pid_namespace_device=4;c.pid_namespace_inode=99;c.state=StartupBirthObserverState::closed;c.final_resources_enabled=true;
    StartupBirthObservation r;r.key={"capture",{1,2},100,0,10};r.birth_monotonic_ns=1000;r.final_monotonic_ns=2000;r.startup_entry_id={"app",Support::estimated,"declared"};
    r.cpu_time={5ns,Support::available,"CPU"};r.read_bytes={20,Support::available,"read"};r.write_bytes={30,Support::available,"write"};c.observations.push_back(r);return a;
}
StartupBirthOverlayContext context(){return {"capture",{1,2},4,99,true};}
SystemSnapshot view(){SystemSnapshot s;StartupEntry e;e.id="app";e.name="retained name";e.command="replacement --not-verified";e.status=StartupStatus::disabled;
    e.impact=StartupImpact::medium;e.observed_startup_cpu_time={99ns,Support::available,"old"};e.observed_startup_interval={2s,Support::estimated,"old interval"};e.impact_scope="previous provider";
    s.startup_entries.push_back(e);e.id="unrelated";s.startup_entries.push_back(e);return s;}
void unchanged(const SystemSnapshot& s){for(const auto& e:s.startup_entries)assert(e.impact==StartupImpact::medium&&e.observed_startup_cpu_time.value==99ns&&e.observed_startup_interval.value==2s&&e.impact_scope=="previous provider");}
int main(){
    auto a=capture();auto c=context();auto s=view();c.accept_declared_associations=false;assert(apply_startup_birth_impact(s,a,c).error==ActionError::permission_denied);unchanged(s);c=context();
    auto result=apply_startup_birth_impact(s,a,c);assert(result&&result.applied_entries==1&&!result.unbound_observations&&!result.not_present_observations);
    auto& e=s.startup_entries[0];assert(e.impact==StartupImpact::not_measured&&!e.observed_startup_impact.value&&e.observed_startup_cpu_time.value==5ns&&e.observed_startup_disk_bytes.value==50&&e.observed_startup_cpu_time.support==Support::estimated&&!e.observed_startup_interval.value);
    assert(e.status==StartupStatus::disabled&&e.name=="retained name"&&e.command=="replacement --not-verified"&&e.impact_scope.find("HISTORICAL")!=e.impact_scope.npos&&e.impact_scope.find("capture=capture")!=e.impact_scope.npos);
    assert(s.startup_entries[1].observed_startup_cpu_time.value==99ns&&s.startup_entries[1].impact_scope=="previous provider");
    assert(a.capture.observations[0].cpu_time.value==5ns&&apply_startup_birth_impact(s,a,c).applied_entries==1&&e.observed_startup_disk_bytes.value==50); // Idempotent, no source mutation/sum.
    a.capture.observations[0].cpu_time.value=11ns;assert(apply_startup_birth_impact(s,a,c));assert(e.impact==StartupImpact::high&&e.observed_startup_impact.support==Support::estimated);
    auto second=a.capture.observations[0];++second.key.kernel_tgid;++second.key.kernel_start_boottime_ns;second.cpu_time={{},Support::permission_required,"protected"};a.capture.observations.push_back(second);
    assert(apply_startup_birth_impact(s,a,c)&&e.impact==StartupImpact::high&&!e.observed_startup_cpu_time.value&&!e.observed_startup_disk_bytes.value&&!e.observed_startup_interval.value&&e.impact_scope.find("NOT summed")!=e.impact_scope.npos);
    a.capture.observations[0].cpu_time.value=5ns;assert(apply_startup_birth_impact(s,a,c)&&!e.observed_startup_impact.value&&e.observed_startup_impact.support==Support::permission_required);
    a=capture();a.capture.observations[0].cpu_time.value=0ns;a.capture.observations[0].read_bytes.value=0;a.capture.observations[0].write_bytes.value=0;
    assert(apply_startup_birth_impact(s,a,c)&&e.observed_startup_cpu_time.value==0ns&&e.observed_startup_disk_bytes.value==0&&!e.observed_startup_impact.value);
    a.capture.observations[0].final_monotonic_ns=2001;assert(apply_startup_birth_impact(s,a,c)&&!e.observed_startup_cpu_time.value&&!e.observed_startup_disk_bytes.value&&!e.observed_startup_impact.value); // Late not prorated.
    a=capture();a.capture.observations[0].startup_entry_id.value="missing";second=a.capture.observations[0];++second.key.kernel_tgid;++second.key.kernel_start_boottime_ns;second.startup_entry_id={};a.capture.observations.push_back(second);
    s=view();result=apply_startup_birth_impact(s,a,c);assert(result&&!result.applied_entries&&result.not_present_observations==1&&result.unbound_observations==1);unchanged(s);
    for(unsigned n=0;n<4;++n){s=view();auto wrong=context();if(n==0)wrong.capture_id="different";if(n==1)++wrong.boot[0];if(n==2)++wrong.pid_namespace_device;if(n==3)++wrong.pid_namespace_inode;assert(apply_startup_birth_impact(s,capture(),wrong).error==ActionError::stale_target);unchanged(s);}
    for(unsigned n=0;n<11;++n){s=view();a=capture();
        if(n==0)s.startup_entries[1].id="app";if(n==1)s.startup_entries[1].id="";if(n==2)a.capture.observations[0].cpu_time.value=-1ns;
        if(n==3)a.capture.state=StartupBirthObserverState::recording;if(n==4)a.capture.quality_flags=128;if(n==5)a.policy.high_cpu=0ns;
        if(n==6){second=a.capture.observations[0];++second.key.kernel_tgid;second.startup_entry_id={};second.cpu_time.support=Support::unsupported;a.capture.observations.push_back(second);} // Malformed unbound row cannot hide.
        if(n==7)a.capture.observations.push_back(a.capture.observations[0]);if(n==8)a.capture.automatic_startup_origin={true,Support::available,"forged"};
        if(n==9)a.capture.observations[0].startup_entry_id.note.resize(4097,'x');if(n==10)a.capture.observations[0].first_executable={StartupBirthObservation::Executable{1500,1,0,0,0},Support::available,"mode disabled"};
        assert(apply_startup_birth_impact(s,a,c).error==ActionError::invalid_argument);unchanged(s);
    }
    a=capture();a.capture.state=StartupBirthObserverState::failed;a.capture.error=ActionError::system_error;a.capture.quality_flags=2;a.capture.observations[0].cpu_time.value=11ns;s=view();
    assert(apply_startup_birth_impact(s,a,c)&&s.startup_entries[0].impact==StartupImpact::high&&s.startup_entries[0].impact_scope.find("source=failed")!=s.startup_entries[0].impact_scope.npos);
    a=capture();a.capture.observations.clear();s=view();assert(apply_startup_birth_impact(s,a,c).applied_entries==0);unchanged(s);
    std::cout<<"birth overlay: consent/context/exact declared IDs/historical scope/high-only/unknown/zero/no sum/no clock coercion/transaction/idempotence passed\n";
}
