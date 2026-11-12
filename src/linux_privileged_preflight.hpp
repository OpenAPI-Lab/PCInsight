#pragma once
#include "pcinsight/actions.hpp"
#include "pcinsight/metric.hpp"
#include <span>
namespace pcinsight::detail {
// INTERNAL pure preflight, NOT a root helper, authentication or authorization.
// Rules must eventually come from mechanism-owned configuration, NEVER the client.
struct PrivilegedServiceRule {
    std::string unit;
    std::uint8_t operations{}; // start1/stop2/restart4; 0 denies, unknown bits invalid.
};
struct PrivilegedServiceRequest {
    std::string unit;
    ServiceAction operation{ServiceAction::start};
    bool explicitly_requested{}; // User intent only; NOT polkit/authenticated identity.
};
struct PrivilegedServicePreflight {
    std::string unit,policy_action;
    ServiceAction operation;
};
// Snapshot owned strings. No callbacks/I/O/elevation/action, no public grant token.
Metric<PrivilegedServicePreflight> preflight_privileged_service(
    const PrivilegedServiceRequest&,std::span<const PrivilegedServiceRule> mechanism_rules);
}
