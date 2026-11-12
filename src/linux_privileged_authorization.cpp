#include "linux_privileged_authorization.hpp"
#include <exception>
#include <limits>
namespace pcinsight::detail {
namespace {
bool unique_name(const std::string& s){
    // Conservative local :decimal.decimal form; no well-known/client-selected names.
    if(s.size()<4||s.size()>255||s[0]!=':')return false;
    const auto dot=s.find('.',1);if(dot==s.npos||dot==1||dot+1==s.size())return false;
    for(std::size_t i=1;i<s.size();++i)if(i!=dot&&(s[i]<'0'||s[i]>'9'))return false;
    return true;
}
bool valid(const PrivilegedAuthorizationContext& c,std::uint32_t expected_uid){
    if(c.system_bus_id.size()!=32||!unique_name(c.caller_unique_name)||!unique_name(c.authority_unique_owner)||c.caller_unique_name==c.authority_unique_owner||c.authority_uid!=expected_uid||c.caller_uid==std::numeric_limits<std::uint32_t>::max()||!c.connection_generation||!c.mechanism_policy_generation)return false;
    for(char x:c.system_bus_id)if(!((x>='0'&&x<='9')||(x>='a'&&x<='f')))return false;
    return true;
}
}
PrivilegedAuthorizationObservation observe_privileged_authorization(const PrivilegedServiceRequest& request,std::span<const PrivilegedServiceRule> rules,const PrivilegedAuthorizationApi& api){
    PrivilegedAuthorizationObservation out;out.preflight=preflight_privileged_service(request,rules);
    auto reject=[&](PrivilegedAuthorizationState state,std::string note,Support support=Support::unsupported){out.state=state;out.authorization={{},support,note};out.note=std::move(note);};
    if(!metric_is_known(out.preflight)){reject(PrivilegedAuthorizationState::rejected,out.preflight.note,out.preflight.support);return out;}
    const auto expected_uid=api.expected_authority_uid;
    if(!api.observe||!api.check||!expected_uid||*expected_uid==std::numeric_limits<std::uint32_t>::max()){reject(PrivilegedAuthorizationState::unavailable,"No trusted identity/authorization adapter or Authority UID configuration; nothing queried");return out;}
    try{
        const auto before=api.observe();
        if(!before.value||before.support!=Support::available||!valid(*before.value,*expected_uid)){reject(PrivilegedAuthorizationState::unavailable,"Authenticated context unavailable/invalid; no authorization query",before.support==Support::permission_required?Support::permission_required:Support::unsupported);return out;}
        // Copy request + context before calling adapter. No references to mutable payload/rules.
        const auto context=*before.value;const auto plan=*out.preflight.value;
        out.check_attempted=true;
        const auto reply=api.check(context,plan,true); // Preflight requires explicit user intent.
        const auto after=api.observe();
        if(!after.value||after.support!=Support::available||!valid(*after.value,*expected_uid)||*after.value!=context){reject(PrivilegedAuthorizationState::stale,"Caller/Authority/bus/connection/policy changed or disconnected; result discarded");return out;}
        if(!reply.value||reply.support!=Support::available){reject(PrivilegedAuthorizationState::unavailable,"Authorization reply unavailable/non-exact",reply.support==Support::permission_required?Support::permission_required:Support::unsupported);return out;}
        const auto& r=*reply.value;
        if(r.sender_unique_owner!=context.authority_unique_owner||!r.call_serial||r.reply_to_serial!=r.call_serial){reject(PrivilegedAuthorizationState::stale,"Unbound reply sender/correlation; result discarded");return out;}
        if(!r.is_authorized||r.is_challenge||r.dismissed||r.retains_authorization||!r.temporary_authorization_id.empty()){
            out.state=PrivilegedAuthorizationState::denied;out.authorization={false,Support::available,"Denied/challenge/dismissed/retained or temporary authorization not accepted by this coordinator"};out.note=out.authorization.note;return out;
        }
        out.state=PrivilegedAuthorizationState::authorization_observed;
        out.authorization={true,Support::available,"Exact bound adapter authorization observed for this frozen request only; NOT execution/reusable grant"};out.note=out.authorization.note;
    }catch(const std::exception&){reject(PrivilegedAuthorizationState::unavailable,"Identity/authorization adapter threw; no authorization accepted, no retry");}
    catch(...){reject(PrivilegedAuthorizationState::unavailable,"Unknown adapter failure; no authorization accepted, no retry");}
    return out;
}
}
