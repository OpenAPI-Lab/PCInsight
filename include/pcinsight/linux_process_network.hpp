#pragma once
#include "metric.hpp"
#include <cstdint>
#include <vector>
#include <array>
#include <compare>
#include <memory>
#include <span>
namespace pcinsight {
// All four arrays use this order. Successful instrumented socket-operation
// payload bytes, NOT interface/wire throughput or all process network paths.
enum class LinuxNetworkPayloadLane { tcp_send, tcp_receive, udp_send, udp_receive };
enum class LinuxProcessNetworkCoverage { selected_tcp_udp_socket_operations, declared_tcp_udp_socket_namespace_buckets, observed_tcp_udp_socket_namespace_buckets };
enum class LinuxProcessNetworkSelectionMode { strict_single_namespace, declared_namespace_buckets, observed_namespace_buckets };
struct LinuxProcessNetworkSelection {
    int pidfd=-1; // Borrowed during start/addition; must not be concurrently closed/replaced.
    std::uint32_t pid{}; // Caller procfs view, NOT initial kernel TGID.
    std::uint64_t process_start_ticks{},socket_network_namespace_cookie{}; // Observed mode MUST use0; declared modes require a nonzero cookie.
};
struct LinuxNetworkProcIdentity {
    std::uint32_t pid{},real_uid{}; // UID mapped in OBSERVER user namespace, not target namespace below.
    std::uint64_t start_ticks{},procfs_device{},procfs_inode{},process_inode{};
    std::uint64_t pid_namespace_device{},pid_namespace_inode{},user_namespace_device{},user_namespace_inode{};
    auto operator<=>(const LinuxNetworkProcIdentity&) const = default;
};
struct LinuxNetworkKernelIdentity {
    std::uint32_t initial_tgid{},initial_real_uid{};
    std::uint64_t leader_start_boottime_ns{},socket_network_namespace_cookie{};
    auto operator<=>(const LinuxNetworkKernelIdentity&) const = default;
};
struct LinuxProcessNetworkSample {
    LinuxNetworkKernelIdentity kernel;
    LinuxNetworkProcIdentity process;
    Metric<std::array<std::uint64_t,4>> payload_bytes_since_start;
    std::array<Metric<std::uint64_t>,4> payload_bytes_per_second;
    Metric<std::uint64_t> nominal_interval_ns;
    std::uint64_t excluded_positive_receives{},completed_updates{}; // In observed mode updates are ACTOR-wide, repeated per bucket; do not sum them.
};
// Observed mode only: selected process association even BEFORE any instrumented
// activity. No fictitious socket cookie, payload or zero-rate row for empty actors.
struct LinuxNetworkObservedProcess {
    std::uint32_t initial_tgid{},initial_real_uid{};
    std::uint64_t leader_start_boottime_ns{};
    LinuxNetworkProcIdentity process;
    std::uint64_t observed_buckets{},completed_updates{}; // Actor-wide completed update sequence.
};
struct LinuxProcessNetworkUidSample {
    std::uint32_t initial_real_uid{}; // NOT a local account/session username.
    std::uint64_t observed_buckets{};
    std::array<std::uint64_t,4> known{},missing{};
    std::array<bool,4> overflow{};
    std::array<Metric<std::uint64_t>,4> partial_payload_bytes_per_second;
};
struct LinuxProcessNetworkSnapshot {
    LinuxProcessNetworkCoverage coverage=LinuxProcessNetworkCoverage::selected_tcp_udp_socket_operations;
    std::array<std::uint64_t,2> observer_boot_token{};
    std::uint64_t selection_epoch{};
    std::uint32_t producer_initial_tgid{};
    std::uint64_t producer_start_boottime_ns{};
    LinuxNetworkProcIdentity observer;
    std::uint64_t counter_sweep_started_ns{},counter_sweep_finished_ns{}; // CLOCK_MONOTONIC; nominal common end time.
    std::optional<std::array<std::uint64_t,2>> previous_counter_sweep;
    std::vector<LinuxProcessNetworkSample> processes; // Bucket rows: process MAY repeat in either bucket mode; empty actors have no row.
    std::vector<LinuxProcessNetworkUidSample> users; // Partial selected-bucket sums, never total user traffic.
    std::vector<LinuxNetworkObservedProcess> observed_processes; // Selection order, bounded by max_processes; empty in declared/strict modes.
};
// Explicit Linux BPF opt-in; construction/stop never attach probes. No automatic
// Monitor activation, elevation, process discovery or retroactive history.
// Observed-cookie mode discovers only cookies of positive instrumented operations
// by explicitly selected actors; no inventory of idle sockets or global totals.
// Requires compatible BTF/FEXIT/pidfd + BPF/ptrace/LSM access. Unknown/denied
// measurements stay absent. First/recovered bucket has counters but no rates;
// observed mode may have no buckets yet (see observed_processes).
// Samples/rates are estimated (bounded sweep skew), not atomic/global totals.
// Serialize calls in the same descriptor table. Do not unshare/manipulate owned
// FDs. Temporary caller signal masking can delay handlers; no hard deadline or
// async cancellation support. Forked copies reject sampling and close their refs.
class LinuxProcessNetworkSampler {
    struct Impl;
    std::unique_ptr<Impl> impl_;
public:
    // Structural bound, NOT a throughput/latency guarantee. Observed mode uses
    // shared admission batches; strict/declared modes retain per-row capture.
    static constexpr std::size_t max_processes=1024;
    static constexpr std::size_t max_observed_namespaces_per_process=16;
    static constexpr std::size_t max_observed_buckets=max_processes*max_observed_namespaces_per_process;
    static constexpr std::uint64_t max_counter_sweep_ns=1'000'000'000;
    LinuxProcessNetworkSampler();
    // Mode is retained across stop/start/update and transferred by move. In
    // DECLARED bucket mode the limit is max_processes ROWS, not cookies per process. PID/start rows
    // require distinct declared cookies. Other nonzero cookies are excluded,
    // NOT discovered. Each row remains partial; never sum as process totals.
    // Observed mode instead requires1..max_processes DISTINCT processes with cookie0 and
    // discovers up to16 cookies EACH (16384 total); overflow is sticky absence,
    // never eviction. Empty registry known; first observation per cookie has no rate.
    // Observed capture starts with3 traffic links per batch of at most64, plus a
    // gated identity query. Updates add disjoint fresh admission batches while
    // retained batches keep running; empty batches close.1<=batches<=actors<=max_processes.
    // Native persistent FDs after each operation: actors+6+8*batches (initial
    // actors+6+8*ceil(actors/64)). Update preparation holds new pidfd copies temporarily.
    // Common sweep<=1s, per-batch coherent, NOT an atomic cross-batch snapshot.
    explicit LinuxProcessNetworkSampler(LinuxProcessNetworkSelectionMode);
    ~LinuxProcessNetworkSampler();
    LinuxProcessNetworkSampler(LinuxProcessNetworkSampler&&) noexcept;
    LinuxProcessNetworkSampler& operator=(LinuxProcessNetworkSampler&&) noexcept;
    LinuxProcessNetworkSampler(const LinuxProcessNetworkSampler&)=delete;
    LinuxProcessNetworkSampler& operator=(const LinuxProcessNetworkSampler&)=delete;
    Metric<std::uint64_t> start(bool explicitly_authorized,std::span<const LinuxProcessNetworkSelection>);
    // Complete explicit replacement,1..max_processes; no automatic discovery. Requires an
    // active capture and fresh consent. Retained PID/start/cookie uses its owned
    // pidfd (input fd is only borrowed for additions/replacements). Every success
    // changes epoch and clears ALL rate baselines; retained counters keep origin.
    // Denied/malformed requests preserve capture; runtime failure stops ALL.
    Metric<std::uint64_t> update_selection(bool explicitly_authorized,std::span<const LinuxProcessNetworkSelection>);
    Metric<LinuxProcessNetworkSnapshot> sample();
    void stop() noexcept;
};
}
