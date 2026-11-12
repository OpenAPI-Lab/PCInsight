#pragma once
#include "linux_network_shared_set.hpp"
#include "linux_network_multi.hpp"
namespace pcinsight::detail {
// INTERNAL authenticated observed selection over disjoint admission batches.
// Owns pidfds; joins every actor before/after every sweep. Not a replacement
// for the public provider yet: no automatic discovery or publication.
// Serialize calls in the same descriptor table; borrowed source contexts live
// longer than this object. Stop/start creates new counters/epoch/baselines.
class LinuxSharedSelectedNetworkSampler {
public:
    explicit LinuxSharedSelectedNetworkSampler(NetworkSessionApi capture=native_network_session_api(),NetworkSamplerApi clock=native_network_sampler_api(),SelectedNetworkSourceApi target=native_selected_network_source_api()):
        api_(capture),clock_(clock),target_(target),capture_(capture,clock),query_(capture,clock.context,clock.local_process){}
    ~LinuxSharedSelectedNetworkSampler(){stop();}
    LinuxSharedSelectedNetworkSampler(const LinuxSharedSelectedNetworkSampler&)=delete;
    LinuxSharedSelectedNetworkSampler& operator=(const LinuxSharedSelectedNetworkSampler&)=delete;
    Metric<std::uint64_t> start(bool,std::span<const PrivateNetworkSelection>);
    Metric<std::uint64_t> update_selection(bool,std::span<const PrivateNetworkSelection>);
    Metric<PrivateCommonNetworkWindow> sample_common();
    void discard_window() noexcept;
    void stop() noexcept;
private:
    struct Held {int fd=-1;std::uint32_t pid{};std::uint64_t start{};PrivateNetworkIdentity identity;};
    Metric<PrivatePidfdNetworkJoin> join(const Held&);
    NetworkSessionApi api_;NetworkSamplerApi clock_;SelectedNetworkSourceApi target_;
    LinuxSharedNetworkSet capture_;LinuxPidfdIdentityQuery query_;
    std::vector<Held> held_;
    std::optional<LinuxNetworkBtfLayout> layout_;
    std::optional<NetworkAccountingScope> scope_;
    std::vector<PrivateNetworkProcContext> previous_;
    std::optional<std::array<std::uint64_t,2>> bracket_;
    LinuxNetworkAccounting reducer_;
    std::uint64_t creator_{};
};
}
