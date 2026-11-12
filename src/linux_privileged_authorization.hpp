#pragma once
#include "linux_privileged_preflight.hpp"
#include <functional>
namespace pcinsight::detail {
// INTERNAL trusted-adapter contract, never populated from client payload.
// Native adapters are internal and opt-in; empty callbacks deny. Not a grant/execution API.
struct PrivilegedAuthorizationContext {
    std::string system_bus_id,caller_unique_name,authority_unique_owner;
    std::uint32_t caller_uid{},authority_uid{};
    std::uint64_t connection_generation{},mechanism_policy_generation{};
    bool operator==(const PrivilegedAuthorizationContext&)const=default;
};
struct PrivilegedAuthorizationReply {
    std::string sender_unique_owner;
    std::uint64_t call_serial{},reply_to_serial{}; // Actual transport correlation, not echoed payload.
    bool is_authorized{},is_challenge{},dismissed{},retains_authorization{};
    std::string temporary_authorization_id;
};
struct PrivilegedAuthorizationApi {
    // Must read authenticated connection/peer/Authority identity, not assertions.
    std::function<Metric<PrivilegedAuthorizationContext>()> observe;
    // Exactly one call; adapter binds subject and action/unit to SAME request.
    std::function<Metric<PrivilegedAuthorizationReply>(const PrivilegedAuthorizationContext&,const PrivilegedServicePreflight&,bool allow_user_interaction)> check;
    // Mechanism-owned trust configuration, never caller-supplied. polkitd need not run as root.
    std::optional<std::uint32_t> expected_authority_uid;
};
enum class PrivilegedAuthorizationState { rejected,unavailable,stale,denied,authorization_observed };
struct PrivilegedAuthorizationObservation {
    PrivilegedAuthorizationState state{PrivilegedAuthorizationState::rejected};
    Metric<PrivilegedServicePreflight> preflight;
    bool check_attempted{};
    Metric<bool> authorization; // Observation only, NOT a reusable grant/ticket.
    std::string note;
};
// Synchronous internal coordinator; no executor, retries, cache or native IPC.
// Request/rules must be serialized while preflight snapshots them.
PrivilegedAuthorizationObservation observe_privileged_authorization(
    const PrivilegedServiceRequest&,std::span<const PrivilegedServiceRule>,const PrivilegedAuthorizationApi&);
}
