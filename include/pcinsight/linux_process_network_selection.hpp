#pragma once
#include "linux_process_network.hpp"
namespace pcinsight {
struct LinuxNetworkProcessCandidate {
    std::uint32_t pid{};
    std::uint64_t process_start_ticks{};
    auto operator<=>(const LinuxNetworkProcessCandidate&) const = default;
};
struct LinuxNetworkSelectionChange {
    std::uint64_t generation{};
    bool changed{};
    std::size_t added{},retired{},retained{};
};
// Owns pidfds for a COMPLETE replacement list supplied by the caller's policy.
// No process enumeration, BPF, signal delivery or implicit capture/update.
// Capacity is64, never truncate/rotate silently. Empty list is valid and means
// stop capture, NOT start/update with an empty list. Reordering is unchanged.
// Serialize calls in the same FD table; borrowed rows expire on refresh/clear/
// destruction. Downstream capture duplicates them and authenticates kernel IDs.
class LinuxProcessNetworkSelectionOwner {
    struct Impl;std::unique_ptr<Impl> impl_;
public:
    static constexpr std::size_t max_processes=LinuxProcessNetworkSampler::max_processes;
    LinuxProcessNetworkSelectionOwner();
    ~LinuxProcessNetworkSelectionOwner();
    LinuxProcessNetworkSelectionOwner(LinuxProcessNetworkSelectionOwner&&) noexcept;
    LinuxProcessNetworkSelectionOwner& operator=(LinuxProcessNetworkSelectionOwner&&) noexcept;
    LinuxProcessNetworkSelectionOwner(const LinuxProcessNetworkSelectionOwner&)=delete;
    LinuxProcessNetworkSelectionOwner& operator=(const LinuxProcessNetworkSelectionOwner&)=delete;
    // Consent/shape errors preserve current selection; runtime/context failures
    // clear ALL owned FDs. No PID-only fallback when start/permissions are absent.
    Metric<LinuxNetworkSelectionChange> refresh(bool explicitly_authorized,std::span<const LinuxNetworkProcessCandidate>);
    Metric<std::vector<LinuxProcessNetworkSelection>> selections(); // Cookie0, canonical PID order; borrowed handles.
    void clear() noexcept;
};
}
