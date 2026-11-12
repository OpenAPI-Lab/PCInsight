#include "linux_network_admission_monitor.hpp"
#include <set>
#include <type_traits>
namespace pcinsight::detail {
namespace {
template<class T> Metric<T> absent(std::string note,Support s=Support::unsupported){return {{},s==Support::permission_required?s:Support::unsupported,std::move(note)};}
bool valid(const LinuxNetworkAdmissionChange& r){
    if(r.policy!=LinuxNetworkAdmissionPolicy::strict_all&&r.policy!=LinuxNetworkAdmissionPolicy::verified_only)return false;
    if(r.visible_processes>linux_network_inventory_max_processes||r.verified_processes>LinuxProcessNetworkSampler::max_processes||r.verified_processes>r.visible_processes||r.excluded.size()!=r.visible_processes-r.verified_processes||
        r.complete_encountered_set!=r.excluded.empty()||(r.policy==LinuxNetworkAdmissionPolicy::strict_all&&!r.excluded.empty())||
        r.capture_active!=(r.verified_processes!=0)||r.capture_active!=(r.selection_epoch!=0)||!r.inventory_started_ns||r.inventory_finished_ns<r.inventory_started_ns||r.inventory_finished_ns-r.inventory_started_ns>linux_network_inventory_max_span_ns)return false;
    std::uint32_t prior=0;for(const auto& e:r.excluded){if(!e.pid||e.pid>2147483647U||e.pid<=prior||e.note.empty()||
        (e.state!=LinuxNetworkInventoryState::permission_denied&&e.state!=LinuxNetworkInventoryState::disappeared&&e.state!=LinuxNetworkInventoryState::unavailable))return false;
        prior=e.pid;}
    return true;
}
bool same(const LinuxNetworkAdmissionChange& a,const LinuxNetworkAdmissionChange& b){
    if(a.capture_active!=b.capture_active||a.selection_changed!=b.selection_changed||a.visible_processes!=b.visible_processes||a.selection_epoch!=b.selection_epoch||
       a.inventory_started_ns!=b.inventory_started_ns||a.inventory_finished_ns!=b.inventory_finished_ns||a.policy!=b.policy||a.verified_processes!=b.verified_processes||a.complete_encountered_set!=b.complete_encountered_set||a.excluded.size()!=b.excluded.size())return false;
    for(std::size_t i=0;i<a.excluded.size();++i)if(a.excluded[i].pid!=b.excluded[i].pid||a.excluded[i].state!=b.excluded[i].state||a.excluded[i].note!=b.excluded[i].note)return false;
    return true;
}
}
LinuxVisibleNetworkMonitor::LinuxVisibleNetworkMonitor():api_{&native_,
    [](void* p,bool a,LinuxNetworkAdmissionPolicy policy){return static_cast<LinuxVisibleProcessNetworkSampler*>(p)->refresh_visible(a,policy);},
    [](void* p){return static_cast<LinuxVisibleProcessNetworkSampler*>(p)->sample_with_admission();},
    [](void* p) noexcept {static_cast<LinuxVisibleProcessNetworkSampler*>(p)->stop();}}{}
void LinuxVisibleNetworkMonitor::stop() noexcept {if(configured_&&api_.stop)api_.stop(api_.context);configured_=ready_=false;last_.reset();attempt_.reset();}
Metric<LinuxNetworkAdmissionChange> LinuxVisibleNetworkMonitor::refresh(bool authorized,LinuxNetworkAdmissionPolicy policy){
    if(!authorized)return absent<LinuxNetworkAdmissionChange>("Explicit authorization required for EACH visible Monitor refresh",Support::permission_required);
    configured_=true;
    bool stopped=false;
    auto close=[&]{if(!stopped&&api_.stop)api_.stop(api_.context);stopped=true;ready_=false;last_.reset();};
    auto fail=[&](std::string note,Support s=Support::unsupported){close();if(note.empty())note="Visible admission provider failed; capture stopped";attempt_=absent<LinuxNetworkAdmissionChange>(std::move(note),s);return *attempt_;};
    if(!api_)return fail("Visible Monitor provider unavailable");
    try{
        auto r=api_.refresh(api_.context,true,policy);
        if(!r.value||r.support!=Support::available)return fail(r.note,r.support);
        if(!valid(*r.value)||r.value->policy!=policy)return fail("Invalid visible admission metadata/policy; capture stopped");
        auto checkpoint=*r.value;last_=std::move(checkpoint);attempt_.reset();ready_=true;return r;
    }catch(const std::exception& e){close();return fail(std::string("Visible Monitor refresh failed: ")+e.what());}
    catch(...){return fail("Visible Monitor refresh failed; capture stopped");}
}
void LinuxVisibleNetworkMonitor::fill(SystemSnapshot& snapshot,const SampleOptions& options){
    auto missing=[&](const std::string& note,Support s=Support::unsupported){snapshot.linux_process_network=absent<LinuxProcessNetworkSnapshot>(note,s);snapshot.linux_network_admission=absent<LinuxNetworkAdmissionChange>(note,s);};
    if(!options.include_linux_process_network){stop();missing("Visible network report disabled; capture stopped");return;}
    if(!configured_||!ready_||!last_){missing(attempt_?attempt_->note:"No successful visible admission setup; flag never authorizes capture",attempt_?attempt_->support:Support::unsupported);return;}
    bool stopped=false;
    auto close=[&]{if(!stopped)api_.stop(api_.context);stopped=true;ready_=false;last_.reset();};
    auto fail=[&](std::string note,Support s=Support::unsupported){close();if(note.empty())note="Visible sample unavailable; both reports rejected";attempt_=absent<LinuxNetworkAdmissionChange>(note,s);missing(note,s);};
    try{
        if(!last_->capture_active){auto metadata=Metric<LinuxNetworkAdmissionChange>{*last_,Support::available,"Last explicit refresh: no eligible actors; inspect exclusions, not global absence"};
            auto network=absent<LinuxProcessNetworkSnapshot>("No eligible actors; no invented network zero");snapshot.linux_process_network=std::move(network);snapshot.linux_network_admission=std::move(metadata);return;}
        auto frame=api_.sample(api_.context);
        if(!frame.value||frame.support!=Support::estimated){fail(frame.note,frame.support);return;}
        const auto& r=frame.value->admission;const auto& n=frame.value->network;
        if(!valid(r)||!same(r,*last_)||n.selection_epoch!=r.selection_epoch||n.coverage!=LinuxProcessNetworkCoverage::observed_tcp_udp_socket_namespace_buckets||n.observed_processes.size()!=r.verified_processes){fail("Visible frame coverage/epoch/provenance mismatch; both reports rejected");return;}
        std::set<std::uint32_t> selected;for(const auto& actor:n.observed_processes)if(!actor.process.pid||actor.process.pid>2147483647U||!selected.insert(actor.process.pid).second){fail("Visible frame duplicate/invalid selected PID");return;}
        for(const auto& e:r.excluded)if(selected.contains(e.pid)){fail("Excluded PID appeared in selected network metadata");return;}
        if(n.processes.size()>LinuxProcessNetworkSampler::max_observed_buckets){fail("Visible frame exceeds bucket capacity");return;}
        for(const auto& row:n.processes)if(!selected.contains(row.process.pid)){fail("Network row not in admitted metadata; both reports rejected");return;}
        Metric<LinuxNetworkAdmissionChange> metadata{r,Support::available,"Last successful explicit inventory refresh, NOT a new census at sample time"};
        Metric<LinuxProcessNetworkSnapshot> network{std::move(frame.value->network),Support::estimated,"Selected network only; see linux_network_admission for explicit exclusions/coverage"};
        static_assert(std::is_nothrow_move_assignable_v<decltype(metadata)>);static_assert(std::is_nothrow_move_assignable_v<decltype(network)>);
        snapshot.linux_process_network=std::move(network);snapshot.linux_network_admission=std::move(metadata);
    }catch(const std::exception& e){close();fail(std::string("Visible Monitor sampling failed: ")+e.what());}
    catch(...){fail("Visible Monitor sampling failed; both reports rejected");}
}
}
