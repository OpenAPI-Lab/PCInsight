#pragma once
#include "linux_network_sampler.hpp"
namespace pcinsight::detail {
struct PrivateSharedNetworkRead {
    std::vector<PrivateDiscoveredNetworkSessionRead> actors;
    std::uint64_t completed_updates{},read_started_ns{},read_finished_ns{};
};
// INTERNAL caller-declared kernel tokens (monotonic removal), not authenticated PID/procfs
// associations. Exactly3 traffic links for the entire explicit selected set.
// No enumeration/automatic admission, rates, public provider or durable history.
// Serialize calls in the same FD table; no descriptor manipulation/unshare.
class LinuxSharedNetworkSession {
public:
    explicit LinuxSharedNetworkSession(NetworkSessionApi api=native_network_session_api(),NetworkSamplerApi clock=native_network_sampler_api()):api_(api),clock_(clock){}
    ~LinuxSharedNetworkSession(){stop();}
    LinuxSharedNetworkSession(const LinuxSharedNetworkSession&)=delete;
    LinuxSharedNetworkSession& operator=(const LinuxSharedNetworkSession&)=delete;
    Metric<bool> start(bool,std::span<const PrivateNetworkIdentity>);
    // Monotonic removal only: no additions/re-admission, slot reuse or reorder.
    // Retained counters keep their origin; empty selection stops the session.
    Metric<bool> retain(bool,std::span<const PrivateNetworkIdentity>);
    Metric<PrivateSharedNetworkRead> read();
    void stop() noexcept;
private:
    void close(int&) noexcept;
    NetworkSessionApi api_;NetworkSamplerApi clock_;
    std::vector<PrivateNetworkIdentity> actors_;
    std::vector<bool> active_;
    std::uint64_t creator_{};
    std::optional<PrivateSharedNetworkRead> previous_;
    int table_=-1,counters_=-1;
    std::array<int,3> programs_{-1,-1,-1},links_{-1,-1,-1};
};
}
