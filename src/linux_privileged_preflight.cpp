#include "linux_privileged_preflight.hpp"
#include "linux_service_actions.hpp"
#include <set>
namespace pcinsight::detail {
namespace {
Metric<PrivilegedServicePreflight> denied(std::string note,Support support=Support::unsupported){return {{},support,std::move(note)};}
}
Metric<PrivilegedServicePreflight> preflight_privileged_service(const PrivilegedServiceRequest& request,std::span<const PrivilegedServiceRule> rules){
    if(!request.explicitly_requested)return denied("Explicit request required; no authorization attempted",Support::permission_required);
    std::uint8_t bit{};std::string policy;
    switch(request.operation){
    case ServiceAction::start:bit=1;policy="org.pcinsight.helper.systemd.start";break;
    case ServiceAction::stop:bit=2;policy="org.pcinsight.helper.systemd.stop";break;
    case ServiceAction::restart:bit=4;policy="org.pcinsight.helper.systemd.restart";break;
    default:return denied("Operation outside closed start/stop/restart set");
    }
    if(!valid_systemd_service_name(request.unit))return denied("Invalid canonical systemd service target");
    if(rules.size()>256)return denied("Mechanism rule bound256 exceeded");
    std::set<std::string> seen;const PrivilegedServiceRule* selected=nullptr;
    for(const auto& rule:rules){
        if(!valid_systemd_service_name(rule.unit)||(rule.operations&~7U)||!seen.insert(rule.unit).second)return denied("Invalid/ambiguous mechanism configuration; no partial allowlist");
        if(rule.unit==request.unit)selected=&rule;
    }
    if(!selected||!(selected->operations&bit))return denied("Target/operation not explicitly allowed by mechanism rules",Support::permission_required);
    return {PrivilegedServicePreflight{request.unit,std::move(policy),request.operation},Support::available,"Syntax/scope preflight ONLY; no authenticated peer, polkit result, privilege or operation granted"};
}
}
