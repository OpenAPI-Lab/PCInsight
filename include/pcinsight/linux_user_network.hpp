#pragma once
#include "linux_process_network.hpp"
namespace pcinsight {
// Unique numeric identities in the OBSERVER user namespace (see report.observer).
// NOT login sessions, socket owners, usernames, or globally unique host users.
struct LinuxUserNetworkSample {
    std::uint32_t real_uid{};
    std::vector<std::uint32_t> initial_real_uids; // Distinct kernel UIDs mapped into this observer UID; may be many-to-one.
    std::uint64_t selected_processes{},idle_processes{},observed_buckets{};
    // TCP send/receive, UDP send/receive. known+missing==observed_buckets.
    std::array<std::uint64_t,4> known{},missing{},permission_denied{};
    std::array<bool,4> overflow{};
    std::array<Metric<std::uint64_t>,4> partial_payload_bytes_per_second;
    Metric<std::uint64_t> partial_send_bytes_per_second,partial_receive_bytes_per_second,partial_total_bytes_per_second;
    bool send_overflow{},receive_overflow{},total_overflow{};
};
struct LinuxUserNetworkReport {
    LinuxNetworkProcIdentity observer;
    std::array<std::uint64_t,2> observer_boot_token{};
    std::uint64_t selection_epoch{};
    std::uint32_t producer_initial_tgid{};
    std::uint64_t producer_start_boottime_ns{},counter_sweep_started_ns{},counter_sweep_finished_ns{};
    std::optional<std::array<std::uint64_t,2>> previous_counter_sweep;
    std::vector<LinuxUserNetworkSample> users; // Sorted unique observer-domain UID; no repetition per session.
};
// Pure projection of a trusted native F03 observed-mode frame, no capture/NSS/
// syscalls/authorization. Structural validation does not authenticate a caller's
// fabricated frame. Sums rounded-down bucket rates, not cumulative counters.
Metric<LinuxUserNetworkReport> summarize_linux_user_network(const Metric<LinuxProcessNetworkSnapshot>&);
}
