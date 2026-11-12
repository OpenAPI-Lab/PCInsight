#pragma once
#include "pcinsight/monitor.hpp"
#include "pcinsight/sampler.hpp"
namespace pcinsight::detail {
struct VisibleNetworkMonitorApi {
    void* context=nullptr;
    Metric<LinuxNetworkAdmissionChange> (*refresh)(void*,bool,LinuxNetworkAdmissionPolicy)=nullptr;
    Metric<LinuxAdmittedProcessNetworkSnapshot> (*sample)(void*)=nullptr;
    void (*stop)(void*) noexcept=nullptr;
    explicit operator bool()const{return refresh&&sample&&stop;}
};
// Internal lifecycle adapter. Borrowed injected context must outlive it.
class LinuxVisibleNetworkMonitor {
    LinuxVisibleProcessNetworkSampler native_;
    VisibleNetworkMonitorApi api_;
    bool configured_=false,ready_=false;
    std::optional<LinuxNetworkAdmissionChange> last_;
    std::optional<Metric<LinuxNetworkAdmissionChange>> attempt_;
public:
    LinuxVisibleNetworkMonitor();
    explicit LinuxVisibleNetworkMonitor(VisibleNetworkMonitorApi api):api_(api){}
    ~LinuxVisibleNetworkMonitor(){stop();}
    LinuxVisibleNetworkMonitor(const LinuxVisibleNetworkMonitor&)=delete;
    LinuxVisibleNetworkMonitor& operator=(const LinuxVisibleNetworkMonitor&)=delete;
    bool configured()const noexcept{return configured_;}
    Metric<LinuxNetworkAdmissionChange> refresh(bool,LinuxNetworkAdmissionPolicy);
    void fill(SystemSnapshot&,const SampleOptions&);
    void stop() noexcept;
};
// Per-object dependency injection for fixtures/private native tests. No global
// hooks or public production override. Both factories return stopped workers;
// constructing them neither enumerates nor authorizes capture.
struct MonitorAdmissionAccess {static Monitor make(VisibleNetworkMonitorApi);};
struct SamplerAdmissionAccess {static Sampler make(Monitor&&,std::chrono::milliseconds,std::size_t,SampleOptions);};
}
