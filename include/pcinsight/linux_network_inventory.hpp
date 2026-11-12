#pragma once
#include "linux_process_network_selection.hpp"
namespace pcinsight {
enum class LinuxNetworkInventoryState { verified, permission_denied, disappeared, unavailable };
struct LinuxNetworkInventoryEntry {
    std::uint32_t pid{};
    LinuxNetworkInventoryState state=LinuxNetworkInventoryState::unavailable;
    Metric<LinuxNetworkProcessCandidate> candidate;
    Metric<std::uint32_t> observer_real_uid; // Procfs observer domain, NOT initial kernel UID.
};
struct LinuxNetworkProcessInventory {
    std::array<std::uint64_t,2> boot{};
    LinuxNetworkProcIdentity observer;
    std::uint64_t started_ns{},finished_ns{};
    bool enumeration_completed=false; // Directory reached EOF; NOT an atomic/global host census.
    std::vector<LinuxNetworkInventoryEntry> processes; // Every numeric entry encountered, including failed identities.
};
inline constexpr std::size_t linux_network_inventory_max_processes=100000;
inline constexpr std::uint64_t linux_network_inventory_max_span_ns=5'000'000'000ULL;
// Explicit read-only inventory of the caller's /proc view. No BPF, sockets,
// signals, external utilities, persistent FDs or automatic capture/selection.
// Hidden/invisible processes are NOT known absent; procfs view may be restricted.
// Errors/over-limit/observer change/budget exhaustion give no partial inventory.
Metric<LinuxNetworkProcessInventory> read_linux_network_process_inventory(bool explicitly_authorized);
// Strict bridge for the bounded64-actor collector. Requires ALL encountered
// identities verified, rejects >max_processes rather than filtering/truncating/rotating.
// Known empty is valid; caller must stop capture, not start/update with empty.
Metric<std::vector<LinuxNetworkProcessCandidate>> linux_network_inventory_candidates(const Metric<LinuxNetworkProcessInventory>&);
}
