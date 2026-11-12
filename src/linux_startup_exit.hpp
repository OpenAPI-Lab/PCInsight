#pragma once
#include "linux_network_join.hpp"
namespace pcinsight::detail {
// INTERNAL selected lifecycle proof. No CPU/disk/app attribution or automatic
// discovery. Event is taskstats_exit(group_dead=1) ENTRY, not final scheduling.
struct StartupSelectedExitEvent {
    PrivateNetworkIdentity identity;
    std::uint64_t monotonic_ns{};
};
Metric<std::vector<NetworkProbeInstruction>> make_startup_selected_exit_probe(int map_fd,const LinuxNetworkBtfLayout&,PrivateNetworkIdentity);
Metric<std::optional<StartupSelectedExitEvent>> decode_startup_selected_exit(const std::array<std::uint64_t,7>&,PrivateNetworkIdentity);
struct StartupSelectedExitRead {
    PrivatePidfdNetworkJoin selected;
    std::optional<StartupSelectedExitEvent> event;
    bool pidfd_exited{}; // Independent lifecycle confirmation, never wait/reap.
};
class LinuxStartupSelectedExitCapture {
    int pidfd_=-1,map_=-1,program_=-1,link_=-1;
    std::uint32_t creator_{},euid_{};
    std::optional<PrivatePidfdNetworkJoin> selected_;
public:
    ~LinuxStartupSelectedExitCapture(){stop();}
    LinuxStartupSelectedExitCapture()=default;
    LinuxStartupSelectedExitCapture(const LinuxStartupSelectedExitCapture&)=delete;
    LinuxStartupSelectedExitCapture& operator=(const LinuxStartupSelectedExitCapture&)=delete;
    Metric<PrivatePidfdNetworkJoin> start(bool explicitly_authorized,int borrowed_pidfd,std::uint32_t pid,std::uint64_t start_ticks);
    Metric<StartupSelectedExitRead> sample();
    void stop() noexcept;
};
}
