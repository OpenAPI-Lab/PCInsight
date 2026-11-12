#pragma once
#include "types.hpp"
#include <memory>

namespace pcinsight {
struct LinuxTcpSocketSample {
    std::uint8_t ip_version{}, state{}; // 4/6; native TCP_ESTABLISHED=1 or CLOSE_WAIT=8.
    std::uint32_t inode{}, socket_uid{}; // Kernel-reported socket UID, NOT process real UID.
    Metric<std::uint64_t> cookie;
    std::chrono::steady_clock::time_point captured_at;
    Metric<std::uint64_t> acknowledged_bytes, received_bytes; // Native TCP_INFO counters.
    Metric<std::uint64_t> acknowledged_bytes_per_second, received_bytes_per_second;
};
struct LinuxTcpRateCoverage {
    std::uint64_t known{}, missing{};
    bool overflow{};
};
struct LinuxTcpUidSample {
    std::uint32_t socket_uid{};
    std::uint64_t observed_sockets{};
    Metric<std::uint64_t> acknowledged_bytes_per_second, received_bytes_per_second;
    LinuxTcpRateCoverage acknowledged_coverage, received_coverage;
};
struct LinuxTcpSnapshot {
    std::uint64_t network_namespace_device{}, network_namespace_inode{};
    std::uint64_t user_namespace_device{}, user_namespace_inode{};
    std::chrono::steady_clock::time_point started_at, finished_at;
    Metric<bool> ipv4_dump_completed, ipv6_dump_completed;
    std::vector<LinuxTcpSocketSample> sockets;
    std::vector<LinuxTcpUidSample> users;
};
// Explicit opt-in, read-only SOCK_DIAG in the calling thread's namespaces.
// TCP ESTABLISHED/CLOSE_WAIT only; NOT total network, application/PID traffic,
// UDP, retroactive history or wire throughput. No external commands or helper.
// Not thread-safe. Missing samples/reset/namespace or socket identity changes
// discard baselines; first sample exposes counters but no rates.
class LinuxTcpSampler {
    struct Impl;
    std::unique_ptr<Impl> impl_;
public:
    LinuxTcpSampler();
    ~LinuxTcpSampler();
    LinuxTcpSampler(LinuxTcpSampler&&) noexcept;
    LinuxTcpSampler& operator=(LinuxTcpSampler&&) noexcept;
    LinuxTcpSampler(const LinuxTcpSampler&)=delete;
    LinuxTcpSampler& operator=(const LinuxTcpSampler&)=delete;
    Metric<LinuxTcpSnapshot> sample();
    void reset();
};
struct LinuxTcpSocketHolder {
    std::uint32_t pid{};
    std::uint64_t process_start_ticks{}; // From ProcessInfo on the same procfs/boot/PID namespace.
    int descriptor{-1};
    bool operator==(const LinuxTcpSocketHolder&) const = default;
};
struct LinuxTcpHeldSocketSample {
    LinuxTcpSocketHolder holder;
    std::uint8_t ip_version{}, state{};
    std::uint64_t socket_inode{};
    Metric<std::uint64_t> cookie, network_namespace_cookie;
    std::chrono::steady_clock::time_point captured_at;
    Metric<std::uint64_t> acknowledged_bytes, received_bytes;
    Metric<std::uint64_t> acknowledged_bytes_per_second, received_bytes_per_second;
};
// Selected descriptor only, no scan or causal/exclusive PID traffic attribution.
// Self: F_DUPFD_CLOEXEC. Other PID: pidfd_getfd, subject to ptrace access policy.
// Checks holder identity and descriptor cookie at both observation endpoints.
// Duplication briefly retains the socket; queries never consume data/change flags.
Metric<LinuxTcpHeldSocketSample> read_linux_tcp_socket_holder(const LinuxTcpSocketHolder&);
class LinuxTcpSocketHolderSampler {
    std::optional<LinuxTcpHeldSocketSample> previous_;
public:
    Metric<LinuxTcpHeldSocketSample> sample(const LinuxTcpSocketHolder&);
    void reset(){previous_.reset();}
};
}
