#pragma once
#include "pcinsight/types.hpp"
#include "linux_network_btf.hpp"
#include <array>
#include <cstddef>

namespace pcinsight::detail {
// Private capability-test bytecode only; not the accounting producer. It counts
// positive __sys_sendto/sock_recvmsg returns for exactly one TGID (all socket
// families). It does not cover sendmsg/sendmmsg, async or every socket I/O path.
// No actor metadata, payload content, persistent maps, kernel writes or pinning.
struct NetworkProbeInstruction {
    std::uint8_t code{}, registers{};
    std::int16_t offset{};
    std::int32_t immediate{};
    bool operator==(const NetworkProbeInstruction&) const = default;
};
static_assert(sizeof(NetworkProbeInstruction)==8);
struct PrivateNetworkIdentity {
    std::uint32_t kernel_tgid{}, kernel_real_uid{};
    std::uint64_t leader_start_boottime_ns{};
};
// RAW_TRACEPOINT, TEST_RUN in caller only, unattached. ARRAY3 output TGID/UID/start.
Metric<std::vector<NetworkProbeInstruction>> make_private_network_identity_probe(
    int map_fd,const LinuxNetworkBtfLayout&);
Metric<std::vector<NetworkProbeInstruction>> make_private_network_probe(
    std::uint32_t tgid,int map_fd,bool receive);
// Classified private probe: inet{,6}_sendmsg / sock_recvmsg, return slot24.
// Map ARRAY u32->u64 entries6: TCP send/recv, UDP send/recv, read failures,
// excluded positive receives (PEEK/TRUNC/ERRQUEUE). No producer identity key.
Metric<std::vector<NetworkProbeInstruction>> make_private_classified_network_probe(
    std::uint32_t tgid,int map_fd,bool receive,const LinuxNetworkBtfLayout&,
    const PrivateNetworkIdentity* expected=nullptr,std::uint64_t expected_socket_netns_cookie=0,
    bool filter_other_namespaces=false);
// Explicit bucket filter: other NONZERO socket cookies exit without map writes.
// Cookie0/null/read errors remain sticky failures. Requires identity+cookie.
// Nonzero cookie requires exact namespace BTF and expected identity. Every
// positive socket operation must match, otherwise sticky quality failure.
// With expected: ARRAY9. Extra slots6 sticky overflow flag,7 active writers,
// 8 completed update sequence. Requires exact leader start and initial-domain UID.
struct PrivateNetworkQualityRead {
    std::uint64_t active_before{},sequence_before{};
    std::array<std::uint64_t,7> values{};
    std::uint64_t sequence_after{},active_after{};
};
struct PrivateNetworkQualityCounters {
    std::array<std::uint64_t,4> bytes{};
    std::uint64_t excluded_positive_receives{},completed_updates{};
};
Metric<PrivateNetworkQualityCounters> validate_private_network_quality_read(const PrivateNetworkQualityRead&);
// Private observed-cookie registry. No eviction/deletion or cookie supplied by caller.
// Prefix9 retains global failure/overflow/writer/sequence slots4/6/7/8;
// each slot is cookie, four payload lanes, excluded receive count.
inline constexpr unsigned private_network_discovery_capacity=16;
inline constexpr unsigned private_network_discovery_stride=6;
inline constexpr unsigned private_network_discovery_entries=9+private_network_discovery_capacity*private_network_discovery_stride;
// Shared selected producer: ARRAY prefix9 (slot0 enabled,4 failure,6 overflow,
// 7 writers,8 sequence), then per actor UID/start + existing ARRAY105 registry.
// HASH u32 TGID -> u64(slot+1), immutable while attached. No eviction/global scan.
inline constexpr unsigned private_network_shared_stride=2+private_network_discovery_entries;
inline constexpr unsigned private_network_shared_max_actors=64;
Metric<std::vector<NetworkProbeInstruction>> make_private_shared_network_probe(
    int actors_map_fd,int counters_map_fd,unsigned actors,bool receive,const LinuxNetworkBtfLayout&);
Metric<std::vector<NetworkProbeInstruction>> make_private_discovering_network_probe(
    std::uint32_t tgid,int map_fd,bool receive,const LinuxNetworkBtfLayout&,const PrivateNetworkIdentity&);
struct PrivateDiscoveredNetworkBucket {
    std::uint64_t socket_namespace_cookie{};
    std::array<std::uint64_t,4> bytes{};
    std::uint64_t excluded_positive_receives{};
};
struct PrivateNetworkDiscoveryRead {
    std::uint64_t active_before{},sequence_before{};
    std::array<std::uint64_t,private_network_discovery_entries> values{};
    std::uint64_t sequence_after{},active_after{};
};
Metric<std::vector<PrivateDiscoveredNetworkBucket>> validate_private_network_discovery_read(const PrivateNetworkDiscoveryRead&);
}
