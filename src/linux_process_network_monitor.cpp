#include "linux_process_network_monitor.hpp"
#include <exception>
namespace pcinsight::detail {
LinuxProcessNetworkMonitor::LinuxProcessNetworkMonitor():api_{&native_,
    [](void* p,bool authorized,std::span<const LinuxProcessNetworkSelection> selection){return static_cast<LinuxProcessNetworkSampler*>(p)->start(authorized,selection);},
    [](void* p){return static_cast<LinuxProcessNetworkSampler*>(p)->sample();},
    [](void* p) noexcept {static_cast<LinuxProcessNetworkSampler*>(p)->stop();},
    [](void* p,bool authorized,std::span<const LinuxProcessNetworkSelection> selection){return static_cast<LinuxProcessNetworkSampler*>(p)->update_selection(authorized,selection);},
    [](void* p,bool authorized,std::span<const LinuxProcessNetworkSelection> selection,LinuxProcessNetworkSelectionMode mode){
        auto& sampler=*static_cast<LinuxProcessNetworkSampler*>(p);sampler=LinuxProcessNetworkSampler(mode);return sampler.start(authorized,selection);}}{}
void LinuxProcessNetworkMonitor::stop() noexcept {
    if(active_&&api_.stop)api_.stop(api_.context);
    active_=false;attempt_.reset();
}
Metric<std::uint64_t> LinuxProcessNetworkMonitor::start(bool authorized,std::span<const LinuxProcessNetworkSelection> selection){
    return start(authorized,selection,LinuxProcessNetworkSelectionMode::strict_single_namespace);
}
Metric<std::uint64_t> LinuxProcessNetworkMonitor::start(bool authorized,std::span<const LinuxProcessNetworkSelection> selection,LinuxProcessNetworkSelectionMode mode){
    if(!authorized){Metric<std::uint64_t> denied{{},Support::permission_required,"Explicit selected BPF authorization required; no new capture started"};if(!active_)attempt_=denied;return denied;}
    if(active_)return {{},Support::unsupported,"Selected Monitor capture already started; stop before replacing selection"};
    if(mode!=LinuxProcessNetworkSelectionMode::strict_single_namespace&&mode!=LinuxProcessNetworkSelectionMode::declared_namespace_buckets&&mode!=LinuxProcessNetworkSelectionMode::observed_namespace_buckets){
        attempt_=Metric<std::uint64_t>{{},Support::unsupported,"Unknown selected Monitor capture mode; no provider called"};return *attempt_;}
    if(!api_){attempt_=Metric<std::uint64_t>{{},Support::unsupported,"Selected Monitor provider unavailable"};return *attempt_;}
    if(mode!=LinuxProcessNetworkSelectionMode::strict_single_namespace&&!api_.start_mode){
        attempt_=Metric<std::uint64_t>{{},Support::unsupported,"Selected Monitor provider has no explicit namespace-bucket setup"};return *attempt_;}
    try{auto result=api_.start_mode?api_.start_mode(api_.context,true,selection,mode):api_.start(api_.context,true,selection);
        if(result.value&&result.support==Support::available&&*result.value){active_=true;attempt_=result;return result;}
        api_.stop(api_.context);result.value.reset();if(result.support!=Support::permission_required)result.support=Support::unsupported;
        attempt_=result;return result;
    }catch(const std::exception& e){api_.stop(api_.context);attempt_=Metric<std::uint64_t>{{},Support::unsupported,std::string("Selected Monitor start failed: ")+e.what()};return *attempt_;}
    catch(...){api_.stop(api_.context);attempt_=Metric<std::uint64_t>{{},Support::unsupported,"Selected Monitor start failed; provider stopped"};return *attempt_;}
}
void LinuxProcessNetworkMonitor::fill(SystemSnapshot& snapshot,const SampleOptions& options){
    if(!options.include_linux_process_network){stop();snapshot.linux_process_network={{},Support::unsupported,"Selected process network report disabled; capture stopped"};return;}
    if(!active_){snapshot.linux_process_network={{},attempt_?attempt_->support:Support::unsupported,
        attempt_?attempt_->note:"No selected process network capture started; include flag alone never authorizes BPF"};return;}
    try{snapshot.linux_process_network=api_.sample(api_.context);
        if(!metric_is_known(snapshot.linux_process_network)){snapshot.linux_process_network.value.reset();
            if(snapshot.linux_process_network.support!=Support::permission_required)snapshot.linux_process_network.support=Support::unsupported;}
    }catch(const std::exception& e){stop();snapshot.linux_process_network={{},Support::unsupported,std::string("Selected Monitor sampling failed; capture stopped: ")+e.what()};}
    catch(...){stop();snapshot.linux_process_network={{},Support::unsupported,"Selected Monitor sampling failed; capture stopped"};}
}
Metric<std::uint64_t> LinuxProcessNetworkMonitor::update_selection(bool authorized,std::span<const LinuxProcessNetworkSelection> selection){
    if(!authorized)return {{},Support::permission_required,"Explicit authorization required for each Monitor selection update"};
    if(!active_)return {{},Support::unsupported,"Monitor capture inactive; update cannot authorize a new session"};
    auto fail=[&](Metric<std::uint64_t> result){stop();result.value.reset();
        if(result.support!=Support::permission_required)result.support=Support::unsupported;
        attempt_=result;return result;};
    if(!api_.update)return fail({{},Support::unsupported,"Monitor provider does not support selection updates; capture stopped"});
    try{auto result=api_.update(api_.context,true,selection);
        if(result.value&&result.support==Support::available&&*result.value){attempt_=result;return result;}
        return fail(std::move(result));
    }catch(const std::exception& e){return fail({{},Support::unsupported,std::string("Monitor selection update failed; capture stopped: ")+e.what()});}
    catch(...){return fail({{},Support::unsupported,"Monitor selection update failed; capture stopped"});}
}
}
