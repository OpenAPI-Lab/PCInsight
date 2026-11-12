#include "pcinsight/linux_network_admission.hpp"
namespace pcinsight {
namespace {
template<class T> Metric<T> absent(std::string note,Support s=Support::unsupported){return {{},s==Support::permission_required?s:Support::unsupported,std::move(note)};}
}
Metric<LinuxNetworkAdmissionPlan> plan_linux_network_admission(const Metric<LinuxNetworkProcessInventory>& source,LinuxNetworkAdmissionPolicy policy){
    if(policy!=LinuxNetworkAdmissionPolicy::strict_all&&policy!=LinuxNetworkAdmissionPolicy::verified_only)
        return absent<LinuxNetworkAdmissionPlan>("Unknown admission policy");
    if(!source.value||source.support!=Support::available)return absent<LinuxNetworkAdmissionPlan>(source.note.empty()?"Verified inventory absent":source.note,source.support);
    const auto& s=*source.value;const auto& o=s.observer;
    if(!s.enumeration_completed||!(s.boot[0]||s.boot[1])||!o.pid||o.pid>2147483647U||!o.start_ticks||!o.procfs_device||!o.procfs_inode||!o.process_inode||!o.pid_namespace_device||!o.pid_namespace_inode||!o.user_namespace_device||!o.user_namespace_inode||
        !s.started_ns||s.finished_ns<s.started_ns||s.finished_ns-s.started_ns>linux_network_inventory_max_span_ns||s.processes.size()>linux_network_inventory_max_processes)
        return absent<LinuxNetworkAdmissionPlan>("Malformed/incomplete inventory; partial policy cannot hide bad metadata");
    LinuxNetworkAdmissionPlan out;out.policy=policy;out.visible_processes=s.processes.size();std::vector<LinuxNetworkProcessCandidate> rows;
    rows.reserve(s.processes.size());std::uint32_t previous=0;bool denied=false;
    for(const auto& r:s.processes){
        if(!r.pid||r.pid>2147483647U||r.pid<=previous)return absent<LinuxNetworkAdmissionPlan>("Inventory PID order/identity invalid");
        previous=r.pid;
        if(r.state==LinuxNetworkInventoryState::verified){
            if(!r.candidate.value||r.candidate.support!=Support::available||r.candidate.value->pid!=r.pid||!r.candidate.value->process_start_ticks||!r.observer_real_uid.value||r.observer_real_uid.support!=Support::available)
                return absent<LinuxNetworkAdmissionPlan>("Verified row lacks exact PID/start/observer UID");
            rows.push_back(*r.candidate.value);
        }else{
            if(r.state!=LinuxNetworkInventoryState::permission_denied&&r.state!=LinuxNetworkInventoryState::disappeared&&r.state!=LinuxNetworkInventoryState::unavailable)
                return absent<LinuxNetworkAdmissionPlan>("Unknown inventory row state");
            const auto support=r.state==LinuxNetworkInventoryState::permission_denied?Support::permission_required:Support::unsupported;
            if(r.candidate.value||r.observer_real_uid.value||r.candidate.support!=support||r.observer_real_uid.support!=support||r.candidate.note.empty()||r.observer_real_uid.note.empty())
                return absent<LinuxNetworkAdmissionPlan>("Malformed excluded row; cannot filter contradictory identity/quality");
            denied|=support==Support::permission_required;out.excluded.push_back({r.pid,r.state,r.candidate.note});
        }
    }
    out.verified_processes=rows.size();out.all_encountered_verified=out.excluded.empty();
    const auto eligible=policy==LinuxNetworkAdmissionPolicy::strict_all?s.processes.size():rows.size();
    if(eligible>LinuxProcessNetworkSampler::max_processes)
        out.candidates=absent<std::vector<LinuxNetworkProcessCandidate>>("Eligible set exceeds capture capacity; ALL admission rejected, no truncation/rotation");
    else if(policy==LinuxNetworkAdmissionPolicy::strict_all&&!out.excluded.empty())
        out.candidates=absent<std::vector<LinuxNetworkProcessCandidate>>("Strict policy requires ALL encountered identities verified; explicit exclusions in plan",denied?Support::permission_required:Support::unsupported);
    else out.candidates={std::move(rows),Support::available,"Complete eligible set within capacity; procfs verification is NOT BPF capture permission"};
    return {std::move(out),Support::available,"Policy decision only; inspect candidates readiness and exclusions, not global-host coverage"};
}
}
