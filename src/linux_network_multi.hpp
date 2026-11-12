#pragma once
#include "linux_network_selected_sampler.hpp"
#include <memory>
#include <span>
namespace pcinsight::detail {
struct PrivateNetworkSelection {
    int pidfd=-1;
    std::uint32_t pid{};
    std::uint64_t start_ticks{},socket_cookie{};
};
// Independent per-actor intervals, NOT a simultaneous snapshot or UID sum.
struct PrivateMultiNetworkWindow {
    std::uint64_t selection_epoch{};
    std::vector<PrivateSelectedNetworkWindow> actors; // Selection order, distinct process/cookie buckets.
};
struct PrivateCommonNetworkActor {
    NetworkActor actor;
    PrivateNetworkProcContext local;
    PrivateNetworkQualityCounters quality;
};
struct PrivateCommonNetworkWindow {
    NetworkAccountingOutput accounting;
    std::vector<PrivateCommonNetworkActor> associations; // Selection order; match by actor key, not vector position.
    std::uint64_t read_started_ns{},read_finished_ns{};
    std::optional<std::array<std::uint64_t,2>> previous_read_bracket;
    struct ObservedActor {
        PrivateNetworkIdentity identity;
        PrivateNetworkProcContext local;
        std::vector<PrivateDiscoveredNetworkBucket> buckets;
        std::uint64_t completed_updates{}; // Actor-wide, NOT a per-bucket counter.
    };
    std::vector<ObservedActor> observed; // Observed mode only; includes selected actors with no activity.
};
// Internal explicit process selection. Optional observed cookies, no process enumeration/publication.
// Same serialization/FD-table/context-lifetime contract as selected sampler.
class LinuxMultiNetworkSampler {
public:
    static constexpr std::size_t max_actors=max_selected_network_actors;
    explicit LinuxMultiNetworkSampler(NetworkSessionApi capture=native_network_session_api(),
        NetworkSamplerApi clock=native_network_sampler_api(),SelectedNetworkSourceApi target=native_selected_network_source_api(),bool namespace_buckets=false,bool observed_cookies=false):
        capture_(capture),clock_(clock),target_(target),namespace_buckets_(namespace_buckets),observed_cookies_(observed_cookies){}
    ~LinuxMultiNetworkSampler(){stop();}
    LinuxMultiNetworkSampler(const LinuxMultiNetworkSampler&)=delete;
    LinuxMultiNetworkSampler& operator=(const LinuxMultiNetworkSampler&)=delete;
    Metric<std::uint64_t> start(bool authorized,std::span<const PrivateNetworkSelection>);
    // Complete replacement list,1..max_actors. Observed mode uses one row/process and
    // socket_cookie=0; declared modes require nonzero cookies. Retain matching PID/start/cookie captures,
    // close removals, start additions. New consent required on EVERY call.
    // Shape/consent errors leave selection intact; runtime failure stops ALL.
    // Success creates a new epoch and discards ALL rate baselines, even if same list.
    Metric<std::uint64_t> update_selection(bool authorized,std::span<const PrivateNetworkSelection>);
    Metric<PrivateMultiNetworkWindow> sample(); // Not available in observed-cookie mode.
    Metric<PrivateCommonNetworkWindow> sample_common(); // Bounded counter sweep; common nominal end-time, NOT atomic.
    void stop() noexcept;
private:
    void discard_windows() noexcept;
    NetworkSessionApi capture_;
    NetworkSamplerApi clock_;
    SelectedNetworkSourceApi target_;
    std::vector<std::unique_ptr<LinuxSelectedNetworkSampler>> actors_;
    std::vector<PrivateNetworkProcContext> previous_;
    LinuxNetworkAccounting common_reducer_;
    std::optional<std::array<std::uint64_t,2>> common_bracket_;
    enum class Mode { none,independent,common } mode_=Mode::none;
    std::uint64_t epoch_{},creator_{};
    std::array<std::uint64_t,2> boot_{};
    bool namespace_buckets_=false; // Fixed for lifetime; stop/update never silently change policy.
    bool observed_cookies_=false; // Fixed, mutually exclusive with declared buckets; selection cookies must be zero.
};
}
