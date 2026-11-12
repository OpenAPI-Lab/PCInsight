#pragma once
#include "pcinsight/linux_network_admission.hpp"
#include "linux_network_inventory.hpp"
namespace pcinsight::detail {
struct NetworkAdmissionCaptureApi {
    void* context=nullptr;
    Metric<std::uint64_t> (*start)(void*,std::span<const LinuxProcessNetworkSelection>)=nullptr;
    Metric<std::uint64_t> (*update)(void*,std::span<const LinuxProcessNetworkSelection>)=nullptr;
    Metric<LinuxProcessNetworkSnapshot> (*sample)(void*)=nullptr;
    void (*stop)(void*) noexcept=nullptr;
    explicit operator bool()const{return start&&update&&sample&&stop;}
};
NetworkAdmissionCaptureApi network_admission_capture_api(LinuxProcessNetworkSampler&);
// Internal injection only; borrowed sources/capture must outlive this object.
class LinuxNetworkAdmission {
    NetworkInventoryApi inventory_;NetworkSelectionSourceApi source_;
    NetworkAdmissionCaptureApi capture_;LinuxNetworkSelectionOwner owner_;
    std::uint64_t creator_{},epoch_{};
    std::optional<LinuxNetworkAdmissionChange> last_;
public:
    LinuxNetworkAdmission(NetworkInventoryApi inventory,NetworkSelectionSourceApi source,NetworkAdmissionCaptureApi capture):
        inventory_(inventory),source_(source),capture_(capture),owner_(source){}
    ~LinuxNetworkAdmission(){stop();}
    LinuxNetworkAdmission(const LinuxNetworkAdmission&)=delete;
    LinuxNetworkAdmission& operator=(const LinuxNetworkAdmission&)=delete;
    Metric<LinuxNetworkAdmissionChange> refresh(bool,LinuxNetworkAdmissionPolicy=LinuxNetworkAdmissionPolicy::strict_all);
    Metric<LinuxProcessNetworkSnapshot> sample();
    Metric<LinuxAdmittedProcessNetworkSnapshot> sample_with_admission();
    void stop() noexcept {if(capture_.stop)capture_.stop(capture_.context);owner_.clear();last_.reset();creator_=epoch_=0;}
};
}
