#pragma once
#include "linux_process_details.hpp"
#include <map>
namespace pcinsight::detail {
// Private seam: implementations must verify the full frame observer/boot and
// selected procfs identity around the executable read. No raw PID-only lookup.
struct NetworkHistoryExecutableApi {
    void* context{};
    Metric<LinuxExecutableInfo> (*read)(void*,const LinuxProcessNetworkSnapshot&,const LinuxNetworkObservedProcess&){};
};
NetworkHistoryExecutableApi native_network_history_executable_api();
class LinuxNetworkAppHistory {
    struct Point {
        LinuxNetworkObservedProcess actor;
        LinuxExecutableInfo executable;
        std::map<std::uint64_t,std::array<std::uint64_t,4>> buckets;
    };
    NetworkHistoryExecutableApi api_;
    std::optional<LinuxProcessNetworkSnapshot> frame_;
    std::map<std::uint32_t,Metric<LinuxExecutableInfo>> before_;
    std::map<std::uint32_t,Point> previous_;
    std::map<std::string,LinuxAppHistoryEntry> rows_;
    std::chrono::system_clock::time_point started_{},last_{};
    bool initialized_{};
public:
    explicit LinuxNetworkAppHistory(NetworkHistoryExecutableApi api=native_network_history_executable_api()):api_(api){}
    // Exactly one network sweep between prepare and finish; never read BPF to
    // discover actors here. New actors need an extra warmup frame.
    void prepare(bool enabled);
    void finish(SystemSnapshot&,const SampleOptions&);
    void break_continuity() noexcept {frame_.reset();before_.clear();previous_.clear();}
    void reset() noexcept {break_continuity();rows_.clear();started_={};last_={};initialized_=false;}
};
}
