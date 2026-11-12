#include "linux_network_admission.hpp"
#include <exception>
namespace pcinsight::detail {
namespace {
template<class T> Metric<T> absent(std::string note,Support s=Support::unsupported){return {{},s==Support::permission_required?s:Support::unsupported,std::move(note)};}
template<class T> bool exact(const Metric<T>& m){return m.value&&m.support==Support::available;}
bool same(const LinuxNetworkSelfIdentity& a,const LinuxNetworkProcIdentity& b){return
    a.local_pid==b.pid&&a.local_real_uid==b.real_uid&&a.start_ticks==b.start_ticks&&
    a.procfs_device==b.procfs_device&&a.procfs_inode==b.procfs_inode&&a.process_inode==b.process_inode&&
    a.pid_namespace_device==b.pid_namespace_device&&a.pid_namespace_inode==b.pid_namespace_inode&&
    a.user_namespace_device==b.user_namespace_device&&a.user_namespace_inode==b.user_namespace_inode;}
}
NetworkAdmissionCaptureApi network_admission_capture_api(LinuxProcessNetworkSampler& s){return {&s,
    [](void* p,std::span<const LinuxProcessNetworkSelection> rows){return static_cast<LinuxProcessNetworkSampler*>(p)->start(true,rows);},
    [](void* p,std::span<const LinuxProcessNetworkSelection> rows){return static_cast<LinuxProcessNetworkSampler*>(p)->update_selection(true,rows);},
    [](void* p){return static_cast<LinuxProcessNetworkSampler*>(p)->sample();},
    [](void* p) noexcept {static_cast<LinuxProcessNetworkSampler*>(p)->stop();}};}
Metric<LinuxNetworkAdmissionChange> LinuxNetworkAdmission::refresh(bool authorized,LinuxNetworkAdmissionPolicy policy){
    if(!authorized)return absent<LinuxNetworkAdmissionChange>("Explicit authorization required for EVERY visible-process admission refresh",Support::permission_required);
    auto reject=[&](std::string note,Support s=Support::unsupported){stop();return absent<LinuxNetworkAdmissionChange>(std::move(note),s);};
    if(!inventory_||!source_||!capture_)return reject("Visible admission sources unavailable");
    if(creator_&&source_.caller(source_.context)!=creator_)return reject("Forked admission refresh rejected; inherited references closed");
    try{
        if(policy!=LinuxNetworkAdmissionPolicy::strict_all&&policy!=LinuxNetworkAdmissionPolicy::verified_only)return reject("Unknown visible admission policy");
        const auto inventory=collect_linux_network_inventory(true,inventory_);
        auto planned=plan_linux_network_admission(inventory,policy);
        if(!exact(planned))return reject(planned.note,planned.support);
        const auto& candidates=planned.value->candidates;
        if(!exact(candidates))return reject(candidates.note,candidates.support);
        const auto& view=*inventory.value;
        auto decorate=[&](LinuxNetworkAdmissionChange& r){r.policy=policy;r.verified_processes=planned.value->verified_processes;
            r.complete_encountered_set=planned.value->all_encountered_verified;r.excluded=planned.value->excluded;r.visible_processes=view.processes.size();};
        const auto caller=source_.caller(source_.context);
        auto current=[&]()->Metric<bool>{
            const auto context=source_.observe(source_.context);if(!exact(context))return {{},context.support,context.note};
            if(!caller||source_.caller(source_.context)!=caller||context.value->boot!=view.boot||
                context.value->observer.local_pid!=caller||!same(context.value->observer,view.observer))
                return absent<bool>("Inventory observer boot/procfs/namespace changed before admission commit");
            const auto now=inventory_.now(inventory_.context);if(!exact(now))return {{},now.support,now.note};
            if(*now.value<view.finished_ns||*now.value<view.started_ns||*now.value-view.started_ns>linux_network_inventory_max_span_ns)
                return absent<bool>("Inventory is stale/clock regressed;5s admission budget exceeded");
            return {true,Support::available,{}};
        };
        auto checked=current();if(!exact(checked))return reject(checked.note,checked.support);
        const auto prepared=owner_.refresh(true,*candidates.value);if(!exact(prepared))return reject(prepared.note,prepared.support);
        auto rows=owner_.selections();if(!exact(rows))return reject(rows.note,rows.support);
        const bool changed=!epoch_||prepared.value->changed;
        if(rows.value->empty()){
            checked=current();if(!exact(checked))return reject(checked.note,checked.support);
            const bool was_active=epoch_!=0;
            Metric<LinuxNetworkAdmissionChange> result{LinuxNetworkAdmissionChange{false,was_active,0,0,view.started_ns,view.finished_ns},Support::available,"No eligible actors; all capture stopped. Inspect exclusions: NOT proof of no visible/host processes"};
            decorate(*result.value);
            stop();return result;
        }
        std::uint64_t epoch=epoch_;
        if(changed){const auto admitted=epoch_?capture_.update(capture_.context,*rows.value):capture_.start(capture_.context,*rows.value);
            if(!exact(admitted)||!*admitted.value)return reject(admitted.note.empty()?"Admission capture did not return a valid epoch":admitted.note,admitted.support);
            epoch=*admitted.value;
        }
        checked=current();if(!exact(checked))return reject(checked.note,checked.support);
        Metric<LinuxNetworkAdmissionChange> result{LinuxNetworkAdmissionChange{true,changed,rows.value->size(),epoch,view.started_ns,view.finished_ns},Support::available,
            "Complete eligible set admitted; inspect exclusions/coverage, not all-host guarantee; unchanged sets preserve rate baselines"};
        decorate(*result.value);auto checkpoint=*result.value;
        checked=current();if(!exact(checked))return reject(checked.note,checked.support);
        last_=std::move(checkpoint);creator_=caller;epoch_=epoch;return result;
    }catch(const std::exception& e){stop();return absent<LinuxNetworkAdmissionChange>(std::string("Visible admission failed; ALL capture stopped: ")+e.what());}
    catch(...){stop();return absent<LinuxNetworkAdmissionChange>("Visible admission failed; ALL capture stopped");}
}
Metric<LinuxProcessNetworkSnapshot> LinuxNetworkAdmission::sample(){
    if(!epoch_)return absent<LinuxProcessNetworkSnapshot>("Visible admission stopped; explicit successful refresh required");
    if(source_.caller(source_.context)!=creator_){stop();return absent<LinuxProcessNetworkSnapshot>("Forked admitted sampler rejected; inherited references closed");}
    try{auto r=capture_.sample(capture_.context);
        if(!metric_is_known(r)||r.support!=Support::estimated||r.value->selection_epoch!=epoch_||r.value->coverage!=LinuxProcessNetworkCoverage::observed_tcp_udp_socket_namespace_buckets){
            auto note=r.note.empty()?std::string("Admitted capture quality/epoch/coverage invalid; ALL stopped"):r.note;const auto support=r.support;stop();return absent<LinuxProcessNetworkSnapshot>(std::move(note),support);}
        if(last_&&!last_->excluded.empty())r.note+="; PARTIAL inventory admission: excluded "+std::to_string(last_->excluded.size())+" encountered rows; sample_with_admission carries reasons";
        return r;
    }catch(const std::exception& e){stop();return absent<LinuxProcessNetworkSnapshot>(std::string("Admitted sample failed; ALL stopped: ")+e.what());}
    catch(...){stop();return absent<LinuxProcessNetworkSnapshot>("Admitted sample failed; ALL stopped");}
}
Metric<LinuxAdmittedProcessNetworkSnapshot> LinuxNetworkAdmission::sample_with_admission(){
    auto r=sample();if(!r.value)return {{},r.support,r.note};
    try{
        if(!last_){stop();return absent<LinuxAdmittedProcessNetworkSnapshot>("Admission provenance absent; capture stopped");}
        return {LinuxAdmittedProcessNetworkSnapshot{*last_,std::move(*r.value)},Support::estimated,"Selected network with last successful admission coverage; NOT a fresh process census"};
    }catch(...){stop();throw;}
}
}
namespace pcinsight {
struct LinuxVisibleProcessNetworkSampler::Impl {
    LinuxProcessNetworkSampler capture{LinuxProcessNetworkSelectionMode::observed_namespace_buckets};
    detail::LinuxNetworkAdmission admission{detail::native_network_inventory_api(),detail::native_network_selection_source_api(),detail::network_admission_capture_api(capture)};
};
LinuxVisibleProcessNetworkSampler::LinuxVisibleProcessNetworkSampler():impl_(std::make_unique<Impl>()){}
LinuxVisibleProcessNetworkSampler::~LinuxVisibleProcessNetworkSampler()=default;
LinuxVisibleProcessNetworkSampler::LinuxVisibleProcessNetworkSampler(LinuxVisibleProcessNetworkSampler&&) noexcept=default;
LinuxVisibleProcessNetworkSampler& LinuxVisibleProcessNetworkSampler::operator=(LinuxVisibleProcessNetworkSampler&&) noexcept=default;
void LinuxVisibleProcessNetworkSampler::stop() noexcept {if(impl_)impl_->admission.stop();}
Metric<LinuxNetworkAdmissionChange> LinuxVisibleProcessNetworkSampler::refresh_visible(bool authorized){
    return refresh_visible(authorized,LinuxNetworkAdmissionPolicy::strict_all);
}
Metric<LinuxNetworkAdmissionChange> LinuxVisibleProcessNetworkSampler::refresh_visible(bool authorized,LinuxNetworkAdmissionPolicy policy){
    if(!authorized)return {{},Support::permission_required,"Explicit visible-process admission authorization required"};
    try{if(!impl_)impl_=std::make_unique<Impl>();return impl_->admission.refresh(true,policy);}
    catch(const std::exception& e){stop();return {{},Support::unsupported,std::string("Visible admission setup failed: ")+e.what()};}
}
Metric<LinuxProcessNetworkSnapshot> LinuxVisibleProcessNetworkSampler::sample(){
    if(!impl_)return {{},Support::unsupported,"Moved-from visible admission sampler"};
    return impl_->admission.sample();
}
Metric<LinuxAdmittedProcessNetworkSnapshot> LinuxVisibleProcessNetworkSampler::sample_with_admission(){
    if(!impl_)return {{},Support::unsupported,"Moved-from visible admission sampler"};
    try{return impl_->admission.sample_with_admission();}
    catch(const std::exception& e){stop();return {{},Support::unsupported,std::string("Admitted provenance sample failed: ")+e.what()};}
}
}
