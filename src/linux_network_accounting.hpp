#pragma once
#include "pcinsight/types.hpp"
#include <array>
#include <compare>
#include <map>

namespace pcinsight::detail {
// Internal producer contract, NOT a live collector or a public capture API.
// Counter order: TCP send/receive, UDP send/receive. Successful socket-operation
// payload bytes, NOT TCP ACKs, packets/wire bytes, interface or exclusive-owner I/O.
using NetworkCounters = std::array<std::uint64_t,4>;
enum class NetworkIdentityDomain { namespace_inode, kernel_initial };
inline constexpr std::size_t max_selected_network_actors=LinuxProcessNetworkSampler::max_processes;
inline constexpr std::size_t max_network_namespaces_per_actor=LinuxProcessNetworkSampler::max_observed_namespaces_per_process;
enum class NetworkAccountingCoverage { declared_buckets, self_single_socket_namespace, selected_single_socket_namespace, selected_actor_set, selected_namespace_bucket_set, selected_observed_namespace_set, selected_observed_actor_set };
struct NetworkAccountingScope {
    std::array<std::uint64_t,2> boot{};
    std::uint64_t producer_epoch{}, pid_namespace{}, user_namespace{};
    NetworkIdentityDomain pid_domain=NetworkIdentityDomain::namespace_inode,user_domain=NetworkIdentityDomain::namespace_inode;
    NetworkAccountingCoverage coverage=NetworkAccountingCoverage::declared_buckets;
    std::uint32_t producer_tgid{};
    std::uint64_t producer_start_boottime_ns{};
    auto operator<=>(const NetworkAccountingScope&) const = default;
};
struct NetworkActor {
    std::uint32_t tgid{}, real_uid{}; // Explicit producer PID/UID domains above.
    std::uint64_t leader_start_boottime_ns{}, socket_network_namespace_cookie{};
    auto operator<=>(const NetworkActor&) const = default;
};
struct NetworkAccountingRow {
    NetworkActor actor;
    NetworkCounters bytes{};
    bool coherent{}, saturated{};
};
struct NetworkAccountingInput {
    NetworkAccountingScope scope;
    std::uint64_t observed_at_ns{}, lost_updates{};
    bool enumeration_complete{};
    std::vector<NetworkAccountingRow> rows;
};
struct NetworkActorRates {
    NetworkActor actor;
    Metric<NetworkCounters> bytes;
    std::array<Metric<std::uint64_t>,4> bytes_per_second;
    Metric<std::uint64_t> interval_ns;
};
struct NetworkUidRates {
    std::uint32_t real_uid{};
    std::uint64_t observed_buckets{};
    std::array<std::uint64_t,4> known{}, missing{};
    std::array<bool,4> overflow{};
    std::array<Metric<std::uint64_t>,4> bytes_per_second;
};
struct NetworkAccountingOutput {
    NetworkAccountingScope scope;
    std::uint64_t observed_at_ns{}, lost_updates{};
    std::vector<NetworkActorRates> actors;
    std::vector<NetworkUidRates> users; // UID at operation time, NOT current session owner.
};
// Bounded, deterministic reducer. No syscalls, no kernel attachment, no history.
// One coherent input per sampling point; producer must enforce these guarantees.
// On absence/malformed/partial input all previous baselines are discarded.
class LinuxNetworkAccounting {
    std::optional<NetworkAccountingInput> previous_;
public:
    static constexpr std::size_t max_rows=LinuxProcessNetworkSampler::max_observed_buckets;
    static constexpr std::uint64_t max_interval_ns=60'000'000'000ULL;
    Metric<NetworkAccountingOutput> sample(const Metric<NetworkAccountingInput>&);
    void reset(){previous_.reset();}
};
}
