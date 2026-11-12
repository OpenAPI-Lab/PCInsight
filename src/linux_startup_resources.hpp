#pragma once
#include "linux_network_join.hpp"
namespace pcinsight::detail {
// INTERNAL. Scheduler-runtime counter, NOT procfs utime+stime/USER_HZ.
// IO is group accounting INCLUDING already waited children, not exclusive IO.
struct StartupFinalResources {
    PrivateNetworkIdentity identity;
    std::uint64_t monotonic_ns{},scheduler_runtime_ns{},group_accounted_read_bytes{},group_accounted_write_bytes{};
};
Metric<std::vector<NetworkProbeInstruction>> make_startup_resource_probe(int,const LinuxStartupResourceBtfLayout&,PrivateNetworkIdentity);
Metric<std::optional<StartupFinalResources>> decode_startup_resources(const std::array<std::uint64_t,9>&,PrivateNetworkIdentity);
struct StartupResourceRead {
    PrivatePidfdNetworkJoin selected;
    std::optional<StartupFinalResources> resources;
    bool pidfd_exited{}; // Exit can precede resource publication indefinitely.
};
class LinuxStartupResourceCapture {
    int pidfd_=-1,map_=-1,program_=-1,link_=-1;
    std::uint32_t creator_{},euid_{};
    std::optional<PrivatePidfdNetworkJoin> selected_;
public:
    ~LinuxStartupResourceCapture(){stop();}
    LinuxStartupResourceCapture()=default;
    LinuxStartupResourceCapture(const LinuxStartupResourceCapture&)=delete;
    LinuxStartupResourceCapture& operator=(const LinuxStartupResourceCapture&)=delete;
    Metric<PrivatePidfdNetworkJoin> start(bool explicitly_authorized,int borrowed_pidfd,std::uint32_t pid,std::uint64_t start_ticks);
    Metric<StartupResourceRead> sample();
    void stop() noexcept;
};
}
