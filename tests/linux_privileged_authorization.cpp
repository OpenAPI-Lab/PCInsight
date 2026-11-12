#include "../src/linux_privileged_authorization.hpp"
#include <cassert>
#include <iostream>
#include <stdexcept>
using namespace pcinsight;
using namespace pcinsight::detail;
struct Fixture {
    PrivilegedAuthorizationContext context{"0123456789abcdef0123456789abcdef",":1.23",":1.7",1000,100,1,1};
    PrivilegedAuthorizationReply reply{":1.7",44,44,true,false,false,false,{}};
    unsigned observations{},checks{};
    PrivilegedAuthorizationApi api(){return {[this]{++observations;return Metric<PrivilegedAuthorizationContext>{context,Support::available,"fixture only"};},[this](const auto& c,const auto& p,bool interactive){++checks;assert(c==context&&p.unit=="private.service"&&p.operation==ServiceAction::start&&p.policy_action=="org.pcinsight.helper.systemd.start"&&interactive);return Metric<PrivilegedAuthorizationReply>{reply,Support::available,"fixture only"};},100};}
};
int main(){
    PrivilegedServiceRequest request{"private.service",ServiceAction::start,true};std::vector<PrivilegedServiceRule> rules{{request.unit,7}};
    Fixture f;auto out=observe_privileged_authorization(request,rules,f.api());assert(out.state==PrivilegedAuthorizationState::authorization_observed&&out.authorization.value==true&&out.check_attempted&&f.checks==1&&f.observations==2);
    // Every request checks again; no cache even for the same subject/action.
    out=observe_privileged_authorization(request,rules,f.api());assert(out.authorization.value==true&&f.checks==2&&f.observations==4);
    request.explicitly_requested=false;f=Fixture{};out=observe_privileged_authorization(request,rules,f.api());assert(!out.check_attempted&&!out.authorization.value&&f.checks==0&&f.observations==0);request.explicitly_requested=true;
    out=observe_privileged_authorization(request,rules,{});assert(out.state==PrivilegedAuthorizationState::unavailable&&!out.check_attempted);
    f=Fixture{};auto missing_uid=f.api();missing_uid.expected_authority_uid.reset();out=observe_privileged_authorization(request,rules,missing_uid);assert(!out.check_attempted&&!f.observations);
    f=Fixture{};f.context.authority_uid=0;auto root_authority=f.api();root_authority.expected_authority_uid=0;out=observe_privileged_authorization(request,rules,root_authority);assert(out.authorization.value==true&&f.checks==1); // Explicit root is supported too; no hardcoded root assumption.
    for(unsigned i=0;i<8;++i){f=Fixture{};if(i==0)f.context.system_bus_id="wrong";if(i==1)f.context.caller_unique_name="org.client.Name";if(i==2)f.context.authority_unique_owner="org.freedesktop.PolicyKit1";if(i==3)f.context.authority_uid=1000;if(i==4)f.context.caller_uid=0xffffffffU;if(i==5)f.context.connection_generation=0;if(i==6)f.context.mechanism_policy_generation=0;if(i==7)f.context.caller_unique_name=f.context.authority_unique_owner;out=observe_privileged_authorization(request,rules,f.api());assert(!out.authorization.value&&!out.check_attempted&&!f.checks);}
    for(unsigned i=0;i<7;++i){f=Fixture{};auto api=f.api();api.check=[&,i](const auto&,const auto&,bool){++f.checks;if(i==0)f.context.caller_unique_name=":1.24";if(i==1)++f.context.caller_uid;if(i==2)f.context.authority_unique_owner=":1.8";if(i==3)f.context.system_bus_id[0]='f';if(i==4)++f.context.connection_generation;if(i==5)++f.context.mechanism_policy_generation;if(i==6)f.context.authority_uid=1;return Metric<PrivilegedAuthorizationReply>{f.reply,Support::available,"fixture"};};out=observe_privileged_authorization(request,rules,api);assert(out.state==PrivilegedAuthorizationState::stale&&!out.authorization.value&&out.check_attempted&&f.checks==1);}
    for(unsigned i=0;i<8;++i){f=Fixture{};if(i==0)f.reply.is_authorized=false;if(i==1)f.reply.is_challenge=true;if(i==2)f.reply.dismissed=true;if(i==3)f.reply.retains_authorization=true;if(i==4)f.reply.temporary_authorization_id="temporary";if(i==5)f.reply.sender_unique_owner=":1.8";if(i==6)f.reply.call_serial=0;if(i==7)++f.reply.reply_to_serial;out=observe_privileged_authorization(request,rules,f.api());assert(out.authorization.value!=true&&f.checks==1);if(i<5)assert(out.state==PrivilegedAuthorizationState::denied&&out.authorization.value==false);else assert(out.state==PrivilegedAuthorizationState::stale&&!out.authorization.value);}
    f=Fixture{};auto api=f.api();api.observe=[]()->Metric<PrivilegedAuthorizationContext>{return {{},Support::permission_required,"denied"};};out=observe_privileged_authorization(request,rules,api);assert(!out.check_attempted&&out.authorization.support==Support::permission_required);
    f=Fixture{};api=f.api();api.observe=[&]()->Metric<PrivilegedAuthorizationContext>{++f.observations;if(f.observations==2)return {{},Support::unsupported,"disconnected"};return {f.context,Support::available,"fixture"};};out=observe_privileged_authorization(request,rules,api);assert(out.state==PrivilegedAuthorizationState::stale&&f.checks==1&&!out.authorization.value);
    f=Fixture{};api=f.api();api.check=[](const auto&,const auto&,bool)->Metric<PrivilegedAuthorizationReply>{throw std::runtime_error("fixture");};out=observe_privileged_authorization(request,rules,api);assert(out.check_attempted&&!out.authorization.value);
    f=Fixture{};api=f.api();api.check=[&](const auto&,const auto&,bool){return Metric<PrivilegedAuthorizationReply>{f.reply,Support::estimated,"not exact"};};out=observe_privileged_authorization(request,rules,api);assert(out.state==PrivilegedAuthorizationState::unavailable&&!out.authorization.value);
    // Callback mutating external request/rules cannot rewrite frozen request.
    f=Fixture{};api=f.api();api.check=[&](const auto&,const auto& plan,bool){request.unit="other.service";rules[0].unit="other.service";assert(plan.unit=="private.service");return Metric<PrivilegedAuthorizationReply>{f.reply,Support::available,"fixture"};};out=observe_privileged_authorization(request,rules,api);assert(out.authorization.value==true&&out.preflight.value->unit=="private.service");
    std::cout<<"Authorization coordinator binding/snapshot/no-cache/deny/challenge/stale/sender/cookie/exception tests passed; fixtures only, NOT native identity/polkit/helper/execution\n";
}
