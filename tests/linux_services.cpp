#include "pcinsight/linux_services.hpp"
#include "../src/linux_systemd.hpp"
#include <cassert>
#include <cstdlib>
#include <iostream>
#include <string_view>
#ifdef __linux__
#include <unistd.h>
#endif
using namespace pcinsight;
int main(int argc,char** argv){
#ifdef __linux__
    if(argc>1&&std::string_view(argv[1])=="--fixture-loader"){
        // These must be ignored, not contacted and not used to select a UID.
        setenv("DBUS_SESSION_BUS_ADDRESS","tcp:host=invalid.example,port=1",1);
        setenv("XDG_RUNTIME_DIR","/invalid/foreign-user",1);
        for(const auto* mode:{"ok","empty","denied","wronguid","ownerchange","broken","missing","busdenied"}){
            setenv("PCINSIGHT_SYSTEMD_FIXTURE",mode,1);
            auto r=query_linux_service_inventory(LinuxServiceScope::current_user);
            const std::string_view m=mode;
            if(m=="ok"||m=="empty"){
                assert(metric_is_known(r)&&r.support==Support::available);
                assert(r.value->scope==LinuxServiceScope::current_user&&r.value->user_uid==geteuid());
                assert(r.value->manager_owner==":1.77");
                assert(r.value->services.size()==(m=="ok"?1u:0u));
                if(m=="ok"){
                    assert(r.value->services[0].linux_details.value->provider=="systemd-user-manager");
                    auto view=open_linux_service({LinuxServiceScope::current_user,"fixture.service"});
                    assert(metric_is_known(view)&&view.value->user_uid==geteuid());
                    assert(!view.value->service.pid); // Installed file != running service.
                }
            }else{
                assert(!r.value&&!r.note.empty());
                if(m=="denied"||m=="wronguid"||m=="busdenied")assert(r.support==Support::permission_required);
            }
        }
        for(const auto* mode:{"ok","early","failed","denied","wronguid","ownerchange","missing","busdenied","actiondenied","nosuchunit","badjob","waiterror"}){
            setenv("PCINSIGHT_SYSTEMD_FIXTURE",mode,1);
            for(auto action:{ServiceAction::start,ServiceAction::stop,ServiceAction::restart}){
                const auto result=control_linux_service({LinuxServiceScope::current_user,"fixture.service"},action);
                const std::string_view m=mode;
                if(m=="ok"||m=="early")assert(result);
                else{
                    assert(!result&&!result.message.empty());
                    if(m=="denied"||m=="wronguid"||m=="busdenied"||m=="actiondenied")assert(result.error==ActionError::permission_denied);
                    if(m=="nosuchunit")assert(result.error==ActionError::not_found);
                    if(m=="missing")assert(result.error==ActionError::not_supported);
                }
            }
        }
        return 0;
    }
    if(argc>1&&std::string_view(argv[1])=="--native"){
        for(auto scope:{LinuxServiceScope::system,LinuxServiceScope::current_user}){
            auto r=query_linux_service_inventory(scope);
            std::cout<<(scope==LinuxServiceScope::system?"system":"current_user")<<": "<<r.note<<'\n';
            if(metric_is_known(r)){
                std::cout<<"owner="<<r.value->manager_owner<<" count="<<r.value->services.size()<<'\n';
                if(scope==LinuxServiceScope::current_user)assert(r.value->user_uid==geteuid());
                if(!r.value->services.empty()){
                    auto v=open_linux_service({scope,r.value->services.front().id});
                    assert(metric_is_known(v));
                    std::cout<<"opened="<<v.value->target.name<<'\n';
                }
            }else assert(!r.value);
        }
        return 0;
    }
#else
    (void)argc;(void)argv;
    assert(!query_linux_service_inventory(LinuxServiceScope::current_user).value);
    assert(!query_linux_service_inventory(LinuxServiceScope::system).value);
    assert(control_linux_service({LinuxServiceScope::current_user,"fixture.service"},ServiceAction::start).error==ActionError::not_supported);
#endif
    assert(!open_linux_service({LinuxServiceScope::current_user,"../bad.service"}).value);
    assert(!open_linux_service({static_cast<LinuxServiceScope>(99),"valid.service"}).value);
    assert(!query_linux_service_inventory(static_cast<LinuxServiceScope>(99)).value);
    assert(control_linux_service({LinuxServiceScope::current_user,"../bad.service"},ServiceAction::start).error==ActionError::invalid_argument);
    assert(control_linux_service({LinuxServiceScope::current_user,"valid.service"},static_cast<ServiceAction>(99)).error==ActionError::invalid_argument);
    assert(control_linux_service({static_cast<LinuxServiceScope>(99),"valid.service"},ServiceAction::start).error==ActionError::invalid_argument);
    Metric<LinuxServiceInventory> inv{LinuxServiceInventory{},Support::available,"fixture"};
    auto& data=*inv.value;data.manager_owner=":1.4";
    ServiceInfo row;row.id=row.name="same.service";data.services.push_back(row);
    auto v=detail::select_linux_service({LinuxServiceScope::system,"same.service"},inv);
    assert(metric_is_known(v)&&!v.value->user_uid&&v.value->target.scope==LinuxServiceScope::system);
    assert(!detail::select_linux_service({LinuxServiceScope::current_user,"same.service"},inv).value);
    data.scope=LinuxServiceScope::current_user;
    assert(!detail::select_linux_service({LinuxServiceScope::current_user,"same.service"},inv).value);
    data.user_uid=42;
    v=detail::select_linux_service({LinuxServiceScope::current_user,"same.service"},inv);
    assert(metric_is_known(v)&&v.value->user_uid==42&&v.value->manager_owner==":1.4");
    assert(!detail::select_linux_service({LinuxServiceScope::current_user,"missing.service"},inv).value);
    data.services.push_back(row);
    assert(!detail::select_linux_service({LinuxServiceScope::current_user,"same.service"},inv).value);
    data.services.clear();
    assert(!detail::select_linux_service({LinuxServiceScope::current_user,"same.service"},inv).value);
    inv.value.reset();inv.support=Support::permission_required;inv.note="denied";
    v=detail::select_linux_service({LinuxServiceScope::current_user,"same.service"},inv);
    assert(!v.value&&v.support==Support::permission_required&&v.note=="denied");
}
