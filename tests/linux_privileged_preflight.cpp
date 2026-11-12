#include "../src/linux_privileged_preflight.hpp"
#include <cassert>
#include <iostream>
using namespace pcinsight;
using namespace pcinsight::detail;
int main(){
    std::vector<PrivilegedServiceRule> rules{{"private-example.service",7}};
    PrivilegedServiceRequest request{"private-example.service",ServiceAction::start,false};
    auto result=preflight_privileged_service(request,rules);assert(!result.value&&result.support==Support::permission_required);
    request.explicitly_requested=true;
    for(const auto op:{ServiceAction::start,ServiceAction::stop,ServiceAction::restart}){request.operation=op;result=preflight_privileged_service(request,rules);assert(result.value&&result.value->unit==request.unit&&result.value->operation==op&&result.value->policy_action==std::string(op==ServiceAction::start?"org.pcinsight.helper.systemd.start":op==ServiceAction::stop?"org.pcinsight.helper.systemd.stop":"org.pcinsight.helper.systemd.restart"));}
    // Owned snapshot, not pointers into mutable client/configuration storage.
    auto snapshot=*result.value;request.unit="different.service";rules[0].unit="changed.service";assert(snapshot.unit=="private-example.service"&&snapshot.operation==ServiceAction::restart);
    request={"private-example.service",ServiceAction::start,true};rules={{request.unit,1}};
    assert(preflight_privileged_service(request,rules).value);request.operation=ServiceAction::stop;assert(!preflight_privileged_service(request,rules).value);request.operation=ServiceAction::restart;assert(!preflight_privileged_service(request,rules).value);
    request.operation=ServiceAction::start;rules[0].operations=0;assert(!preflight_privileged_service(request,rules).value);rules[0].operations=8;assert(!preflight_privileged_service(request,rules).value);rules[0].operations=255;assert(!preflight_privileged_service(request,rules).value);
    rules[0].operations=7;request.operation=static_cast<ServiceAction>(55);assert(!preflight_privileged_service(request,rules).value);request.operation=ServiceAction::start;
    for(const auto name:{"", "relative", "/etc/example.service", "*.service", "example;id.service", "openrc-file:6161", "runit-report:6161", "s6-record:6161"}){request.unit=name;assert(!preflight_privileged_service(request,rules).value);}
    request.unit=std::string("private-example.service\0x",25);assert(!preflight_privileged_service(request,rules).value);request.unit=std::string(256,'a')+".service";assert(!preflight_privileged_service(request,rules).value);
    request.unit="private-example.service";assert(!preflight_privileged_service(request,{}).value);rules={{"other.service",7}};assert(!preflight_privileged_service(request,rules).value);
    rules={{request.unit,7},{request.unit,1}};assert(!preflight_privileged_service(request,rules).value);rules={{request.unit,7},{"bad/path.service",0}};assert(!preflight_privileged_service(request,rules).value); // Even invalid unrelated rule denies all.
    rules.clear();for(unsigned i=0;i<256;++i)rules.push_back({"private-"+std::to_string(i)+".service",1});request.unit="private-255.service";assert(preflight_privileged_service(request,rules).value);rules.push_back({"one-too-many.service",1});assert(!preflight_privileged_service(request,rules).value);
    rules={{"private@one.service",7}};request.unit="private@two.service";assert(!preflight_privileged_service(request,rules).value);request.unit="private@one.service";assert(preflight_privileged_service(request,rules).value);
    std::cout<<"Privileged service pure preflight closed operations/exact allowlist/intent/bounds/malformed/default-deny/owned snapshot passed; NO authentication/authorization/elevation/action\n";
}
