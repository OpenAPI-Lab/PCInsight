#pragma once
#include "linux_network_accounting.hpp"
#include "linux_network_session.hpp"
#include "linux_network_self.hpp"
#include <string_view>
namespace pcinsight::detail {
struct NetworkSamplerApi {
    void* context=nullptr;
    Metric<std::array<std::uint64_t,2>> (*boot)(void*)=nullptr;
    Metric<std::uint64_t> (*now)(void*)=nullptr;
    std::uint64_t (*local_process)(void*) noexcept=nullptr; // Fork guard only, NOT kernel TGID translation.
    Metric<LinuxNetworkSelfIdentity> (*self)(void*)=nullptr; // Optional, only for joined SELF reads.
};
NetworkSamplerApi native_network_sampler_api();
std::uint64_t next_private_network_epoch(); // Shared serial across private sampler types.
Metric<std::array<std::uint64_t,2>> parse_network_boot_token(std::string_view);
struct PrivateNetworkWindow {
    NetworkAccountingOutput accounting;
    PrivateNetworkQualityCounters quality;
    std::uint64_t read_started_ns{},read_finished_ns{};
};
struct PrivateJoinedNetworkWindow {PrivateNetworkWindow window;LinuxNetworkSelfIdentity local;};
// Internal SELF/single socket-netns source. No global coverage, Monitor or history.
// Serialize calls; injected contexts must outlive this object.
class LinuxPrivateNetworkSampler {
public:
    explicit LinuxPrivateNetworkSampler(NetworkSessionApi capture=native_network_session_api(),NetworkSamplerApi source=native_network_sampler_api()):capture_(capture),source_(source){}
    Metric<PrivateNetworkIdentity> start(bool authorized,std::uint64_t socket_netns_cookie);
    Metric<PrivateNetworkWindow> sample();
    Metric<PrivateJoinedNetworkWindow> sample_joined_self();
    void stop() noexcept;
    static constexpr std::uint64_t max_read_span_ns=1'000'000'000;
private:
    LinuxPrivateNetworkSession capture_;
    NetworkSamplerApi source_;
    LinuxNetworkAccounting reducer_;
    std::optional<NetworkAccountingScope> scope_;
    std::optional<LinuxNetworkSelfIdentity> last_join_;
    PrivateNetworkIdentity identity_{};
    std::uint64_t cookie_{},local_creator_{},last_end_{};
};
}
