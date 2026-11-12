#include "pcinsight/linux_network_admission.hpp"
#include <cassert>
using namespace pcinsight;
namespace {
Metric<LinuxNetworkProcessInventory> inventory(unsigned n){LinuxNetworkProcessInventory s;s.boot={1,2};s.observer={7,42,100,1,2,3,4,5,6,7};s.started_ns=100;s.finished_ns=200;s.enumeration_completed=true;
    for(unsigned i=1;i<=n;++i)s.processes.push_back({i,LinuxNetworkInventoryState::verified,{LinuxNetworkProcessCandidate{i,100+i},Support::available,{}},{0,Support::available,{}}});return {std::move(s),Support::available,{}};}
void exclude(LinuxNetworkInventoryEntry& r,LinuxNetworkInventoryState state){r.state=state;const auto support=state==LinuxNetworkInventoryState::permission_denied?Support::permission_required:Support::unsupported;r.candidate={{},support,"specific reason"};r.observer_real_uid={{},support,"specific reason"};}
void tests(){
    for(auto policy:{LinuxNetworkAdmissionPolicy::strict_all,LinuxNetworkAdmissionPolicy::verified_only}){
        auto p=plan_linux_network_admission(inventory(0),policy);assert(p.value&&p.value->all_encountered_verified&&p.value->candidates.value->empty());
        p=plan_linux_network_admission(inventory(64),policy);assert(p.value&&p.value->candidates.value->size()==64&&p.value->verified_processes==64&&p.value->excluded.empty());
        p=plan_linux_network_admission(inventory(65),policy);assert(p.value&&p.value->candidates.value->size()==65);
        constexpr auto cap=LinuxProcessNetworkSampler::max_processes;
        p=plan_linux_network_admission(inventory(cap),policy);assert(p.value&&p.value->candidates.value->size()==cap);
        p=plan_linux_network_admission(inventory(cap+1),policy);assert(p.value&&!p.value->candidates.value&&p.value->verified_processes==cap+1&&p.value->visible_processes==cap+1&&p.value->all_encountered_verified);
    }
    auto s=inventory(67);exclude(s.value->processes[0],LinuxNetworkInventoryState::permission_denied);exclude(s.value->processes[1],LinuxNetworkInventoryState::disappeared);exclude(s.value->processes[2],LinuxNetworkInventoryState::unavailable);
    auto p=plan_linux_network_admission(s,LinuxNetworkAdmissionPolicy::verified_only);assert(p.value&&p.value->candidates.value->size()==64&&p.value->visible_processes==67&&p.value->verified_processes==64&&!p.value->all_encountered_verified&&p.value->excluded.size()==3&&p.value->candidates.value->front().pid==4);
    auto strict=plan_linux_network_admission(s,LinuxNetworkAdmissionPolicy::strict_all);assert(strict.value&&!strict.value->candidates.value&&strict.value->excluded.size()==3);
    auto large=inventory(LinuxProcessNetworkSampler::max_processes+3);for(unsigned i=0;i<3;++i)exclude(large.value->processes[i],LinuxNetworkInventoryState::unavailable);
    auto admitted=plan_linux_network_admission(large,LinuxNetworkAdmissionPolicy::verified_only);assert(admitted.value&&admitted.value->candidates.value->size()==LinuxProcessNetworkSampler::max_processes&&admitted.value->excluded.size()==3);
    assert(!plan_linux_network_admission(large,LinuxNetworkAdmissionPolicy::strict_all).value->candidates.value);
    auto denied=inventory(2);exclude(denied.value->processes[1],LinuxNetworkInventoryState::permission_denied);strict=plan_linux_network_admission(denied,LinuxNetworkAdmissionPolicy::strict_all);assert(strict.value&&strict.value->candidates.support==Support::permission_required);
    for(auto& r:s.value->processes)exclude(r,LinuxNetworkInventoryState::unavailable);p=plan_linux_network_admission(s,LinuxNetworkAdmissionPolicy::verified_only);assert(p.value&&p.value->candidates.value->empty()&&p.value->verified_processes==0&&!p.value->all_encountered_verified&&p.value->excluded.size()==67);
    // Partial policy must reject corrupted rows, not hide them as exclusions.
    for(unsigned fault=0;fault<20;++fault){auto bad=denied;auto& v=*bad.value;auto& r=v.processes[1];
        if(fault==0)bad.support=Support::estimated;if(fault==1)v.enumeration_completed=false;if(fault==2)v.boot={};if(fault==3)v.observer.start_ticks=0;
        if(fault==4)v.started_ns=0;if(fault==5)v.finished_ns=1;if(fault==6)v.finished_ns+=linux_network_inventory_max_span_ns;
        if(fault==7)r.pid=1;if(fault==8)r.pid=0;if(fault==9)r.state=static_cast<LinuxNetworkInventoryState>(99);
        if(fault==10)r.candidate.value=LinuxNetworkProcessCandidate{2,102};if(fault==11)r.observer_real_uid.value=0;
        if(fault==12)r.candidate.support=Support::unsupported;if(fault==13)r.observer_real_uid.support=Support::unsupported;
        if(fault==14)r.candidate.note.clear();if(fault==15)r.observer_real_uid.note.clear();if(fault==16)v.processes[0].candidate.value->pid=9;
        if(fault==17)v.processes[0].candidate.value->process_start_ticks=0;if(fault==18)v.processes[0].observer_real_uid.support=Support::estimated;if(fault==19)bad.value.reset();
        assert(!plan_linux_network_admission(bad,LinuxNetworkAdmissionPolicy::verified_only).value);
    }
    assert(!plan_linux_network_admission(inventory(1),static_cast<LinuxNetworkAdmissionPolicy>(99)).value);
}
}
int main(){tests();}
