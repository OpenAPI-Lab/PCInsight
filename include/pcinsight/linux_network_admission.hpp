#pragma once
#include "linux_network_inventory.hpp"
namespace pcinsight {
enum class LinuxNetworkAdmissionPolicy { strict_all, verified_only };
struct LinuxNetworkAdmissionExclusion {
    std::uint32_t pid{};
    LinuxNetworkInventoryState state=LinuxNetworkInventoryState::unavailable;
    std::string note;
};
struct LinuxNetworkAdmissionPlan {
    LinuxNetworkAdmissionPolicy policy=LinuxNetworkAdmissionPolicy::strict_all;
    std::size_t visible_processes{},verified_processes{};
    bool all_encountered_verified=false; // Identity quality only, NOT capture readiness/coverage.
    Metric<std::vector<LinuxNetworkProcessCandidate>> candidates;
    std::vector<LinuxNetworkAdmissionExclusion> excluded{};
};
// Pure policy decision; no syscalls/capture. A valid plan may have ABSENT
// candidates (strict unknown rows or over-capacity). Reasons remain inspectable.
// verified_only excludes ONLY well-formed unverified inventory rows, explicitly.
// More than max_processes eligible actors rejects ALL; never truncate/rotate.
Metric<LinuxNetworkAdmissionPlan> plan_linux_network_admission(const Metric<LinuxNetworkProcessInventory>&,LinuxNetworkAdmissionPolicy);
struct LinuxNetworkAdmissionChange {
    bool capture_active{},selection_changed{};
    std::size_t visible_processes{};
    std::uint64_t selection_epoch{},inventory_started_ns{},inventory_finished_ns{};
    LinuxNetworkAdmissionPolicy policy=LinuxNetworkAdmissionPolicy::strict_all;
    std::size_t verified_processes{};
    bool complete_encountered_set=false;
    std::vector<LinuxNetworkAdmissionExclusion> excluded{};
};
struct LinuxAdmittedProcessNetworkSnapshot {
    LinuxNetworkAdmissionChange admission; // Last successful refresh; NOT a new census at sample time.
    LinuxProcessNetworkSnapshot network; // Selected actors only; no invented rows for exclusions.
};
// Explicit refresh of the caller's visible /proc inventory. Default strict_all
// requires ALL encountered identities verified. Opt-in verified_only reports
// every excluded row. Never truncate/rotate or interpret a restricted view as
// all host processes. Owns selection pidfds and an observed shared sampler.
// Construction does not enumerate/capture. No background polling or automatic
// reauthorization: call refresh_visible(true) for EACH admission refresh.
// Denied consent preserves state. Any authorized refresh/sample failure stops
// ALL capture, not a stale/partial selection. Known empty stops successfully.
// Unchanged PID/start set preserves epoch/rate baselines; changes use the normal
// retained-origin/fresh-addition update. Sampling rechecks kernel permissions.
// Serialize calls; same FD table; forked copies close inherited references.
class LinuxVisibleProcessNetworkSampler {
    struct Impl;std::unique_ptr<Impl> impl_;
public:
    LinuxVisibleProcessNetworkSampler();
    ~LinuxVisibleProcessNetworkSampler();
    LinuxVisibleProcessNetworkSampler(LinuxVisibleProcessNetworkSampler&&) noexcept;
    LinuxVisibleProcessNetworkSampler& operator=(LinuxVisibleProcessNetworkSampler&&) noexcept;
    LinuxVisibleProcessNetworkSampler(const LinuxVisibleProcessNetworkSampler&)=delete;
    LinuxVisibleProcessNetworkSampler& operator=(const LinuxVisibleProcessNetworkSampler&)=delete;
    Metric<LinuxNetworkAdmissionChange> refresh_visible(bool explicitly_authorized);
    // Explicit partial policy; default overload stays strict_all. Runtime BPF/
    // pidfd permission failures still stop ALL, not per-actor retry/filtering.
    Metric<LinuxNetworkAdmissionChange> refresh_visible(bool explicitly_authorized,LinuxNetworkAdmissionPolicy);
    Metric<LinuxProcessNetworkSnapshot> sample(); // Never enumerates/rearms.
    Metric<LinuxAdmittedProcessNetworkSnapshot> sample_with_admission();
    void stop() noexcept;
};
}
