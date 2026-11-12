#pragma once
#include "linux_privileged_authorization.hpp"
#include "linux_privileged_policy.hpp"
#include "linux_privileged_budget.hpp"
namespace pcinsight::detail {
struct PrivilegedIdentityConfig {
    std::string socket_path{"/run/dbus/system_bus_socket"}; // Protected mechanism config, NEVER payload/env.
    std::optional<std::uint32_t> bus_uid,authority_uid;
    std::uint64_t connection_generation{},policy_generation{};
    std::shared_ptr<PrivilegedRequestBudget> budget; // Per-request, never client/config-file payload.
};
enum class PrivilegedRequestKind { authorization_only,control };
Metric<PrivilegedServiceRequest> decode_native_privileged_request(void*,PrivilegedRequestKind);
// Borrowed sd_bus_message* from a trusted receiver, alive/serialized through call.
// Queries bus daemon ONLY, no CheckAuthorization/activation/execution. No owned bus.
Metric<PrivilegedAuthorizationContext> observe_native_privileged_identity(void* incoming_message,const PrivilegedIdentityConfig&);
// Native CheckAuthorization only; rechecks identity before/after, never executes.
// Interaction is OFF unless mechanism explicitly enables it for this user request.
Metric<PrivilegedAuthorizationReply> check_native_privileged_authorization(void*,const PrivilegedIdentityConfig&,const PrivilegedAuthorizationContext&,const PrivilegedServiceRequest&,std::span<const PrivilegedServiceRule>,bool allow_interaction=false);
// Per-request frozen adapter, not a global backend or reusable authorization token.
PrivilegedAuthorizationApi native_privileged_authorization_api(void*,PrivilegedIdentityConfig,PrivilegedServiceRequest,std::vector<PrivilegedServiceRule>,bool permit_interaction=false);
// Closed authorize-only receiver: /org/pcinsight/Helper, org.pcinsight.Helper1,
// AuthorizeSystemService(u version=1,u operation=1..3,s unit,b explicit_intent).
// Current dispatch only; no UID/name/argv from payload; no executor or registration.
PrivilegedAuthorizationObservation receive_native_privileged_request(void*,PrivilegedIdentityConfig,std::vector<PrivilegedServiceRule>,bool permit_interaction=false);
// Managed variant binds policy generation and rejects invalidation/reload mid-query.
// Policy store must outlive this synchronous handler; still no executor/ticket.
PrivilegedAuthorizationObservation receive_native_privileged_policy_request(void*,PrivilegedIdentityConfig,const PrivilegedPolicyStore&,bool permit_interaction=false);
}
