#pragma once
#include "pcinsight/types.hpp"
namespace pcinsight::detail {
struct ProcessNetworkMonitorApi {
    void* context=nullptr;
    Metric<std::uint64_t> (*start)(void*,bool,std::span<const LinuxProcessNetworkSelection>)=nullptr;
    Metric<LinuxProcessNetworkSnapshot> (*sample)(void*)=nullptr;
    void (*stop)(void*) noexcept=nullptr;
    Metric<std::uint64_t> (*update)(void*,bool,std::span<const LinuxProcessNetworkSelection>)=nullptr;
    Metric<std::uint64_t> (*start_mode)(void*,bool,std::span<const LinuxProcessNetworkSelection>,LinuxProcessNetworkSelectionMode)=nullptr;
    explicit operator bool() const {return start&&sample&&stop;}
};
// Internal lifecycle adapter; test contexts must outlive it. Serialize calls.
class LinuxProcessNetworkMonitor {
public:
    LinuxProcessNetworkMonitor();
    explicit LinuxProcessNetworkMonitor(ProcessNetworkMonitorApi api):api_(api){}
    ~LinuxProcessNetworkMonitor(){stop();}
    LinuxProcessNetworkMonitor(const LinuxProcessNetworkMonitor&)=delete;
    LinuxProcessNetworkMonitor& operator=(const LinuxProcessNetworkMonitor&)=delete;
    Metric<std::uint64_t> start(bool,std::span<const LinuxProcessNetworkSelection>);
    Metric<std::uint64_t> start(bool,std::span<const LinuxProcessNetworkSelection>,LinuxProcessNetworkSelectionMode);
    Metric<std::uint64_t> update_selection(bool,std::span<const LinuxProcessNetworkSelection>);
    void stop() noexcept;
    bool active() const noexcept {return active_;}
    void fill(SystemSnapshot&,const SampleOptions&);
private:
    LinuxProcessNetworkSampler native_;
    ProcessNetworkMonitorApi api_;
    bool active_=false;
    std::optional<Metric<std::uint64_t>> attempt_;
};
}
