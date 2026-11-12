#include "linux_drm_monitor.hpp"
#include "linux_nvml.hpp"
#include <algorithm>
namespace pcinsight::detail {
void LinuxDrmMonitor::fill(SystemSnapshot& snapshot,const SampleOptions& options){
    auto& report=snapshot.linux_drm;report={};
    auto inactive=[&](const char* why){reset();report.discovery.note=why;report.aggregates.note=why;};
    if(!options.include_linux_drm_clients){inactive("DRM client discovery disabled; opt in with include_linux_drm_clients");return;}
    if(!options.include_gpus){inactive("DRM client discovery suppressed because include_gpus=false");return;}
    if(!limits_ || *limits_!=options.linux_drm_limits){sampler_.reset();limits_=options.linux_drm_limits;}
    if(!discover_){inactive("DRM discovery provider unavailable");return;}
    try {
        decltype(inventory_) inventory;
        for(const auto& gpu:snapshot.gpus)inventory.emplace_back(gpu.id,gpu.physical_location.support,gpu.physical_location.value?nvml_pci_identity(*gpu.physical_location.value):std::string{});
        std::sort(inventory.begin(),inventory.end());
        if(inventory!=inventory_){sampler_.reset();inventory_=std::move(inventory);}
        auto found=discover_(options.linux_drm_limits);
        if(!found.value || (found.support!=Support::available && found.support!=Support::estimated)){
            sampler_.reset();report.discovery={{},found.support,found.note};
            report.aggregates={{},found.support,"No discovery result; aggregate baseline reset"};return;
        }
        // Keep coverage metadata, not all per-PID/fd raw engine vectors in 60s history.
        report.discovery={static_cast<const LinuxDrmDiscoverySummary&>(*found.value),Support::estimated,found.note};
        report.aggregates=sampler_.sample(found.value->observations);
        if(!report.aggregates.value)return;
        for(auto& group:*report.aggregates.value){
            group.inventory_gpu_id={{},Support::unsupported,"No unique available PCI match in current GPU inventory"};
            const auto key=nvml_pci_identity(group.pci_address);if(key.empty())continue;
            const GpuInfo* match=nullptr;unsigned count=0;
            for(const auto& gpu:snapshot.gpus)if(gpu.physical_location.support==Support::available && gpu.physical_location.value && nvml_pci_identity(*gpu.physical_location.value)==key){match=&gpu;++count;}
            if(count==1 && !match->id.empty()){
                unsigned id_count=0;for(const auto& gpu:snapshot.gpus)if(gpu.id==match->id)++id_count;
                if(id_count==1)group.inventory_gpu_id={match->id,Support::available,"Unique full PCI match to this snapshot's GPU inventory; not a global utilization claim"};
            }
        }
    }catch(...){reset();throw;}
}
}
