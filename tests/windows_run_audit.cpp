#include "pcinsight/audit.hpp"
#include <cassert>
#include <iostream>
#include <stdexcept>
using namespace pcinsight;
struct Mock final:SystemActions {
    unsigned calls{},generic_calls{};bool enabled{},throwing{};WindowsRunRegistration received;
    ActionResult response{ActionError::none,"private backend diagnostic"};
    ActionResult launch_process(const LaunchRequest&)override{return {ActionError::not_supported,""};}
    ActionResult terminate_process(std::uint32_t,bool)override{return {ActionError::not_supported,""};}
    ActionResult set_efficiency_mode(std::uint32_t,bool)override{return {ActionError::not_supported,""};}
    ActionResult control_service(const std::string&,ServiceAction)override{return {ActionError::not_supported,""};}
    ActionResult set_startup_enabled(const std::string&,bool)override{++generic_calls;return {ActionError::not_supported,""};}
    ActionResult disconnect_user(const std::string&)override{return {ActionError::not_supported,""};}
    ActionResult set_windows_run_registration_enabled(const WindowsRunRegistration& value,bool enable)override{
        ++calls;received=value;enabled=enable;if(throwing)throw std::runtime_error("private exception detail");return response;
    }
};
int main(){
    WindowsRunRegistration expected;expected.entry_id="winrun:hkcu:shared:46697874757265";
    expected.command="secret command --private-token";expected.native_type=2;expected.native_data={1,2,0,255};expected.state=WindowsRunRegistrationState::disabled_by_pcinsight;
    auto backend=std::make_unique<Mock>();auto* mock=backend.get();std::vector<AuditEvent> events;
    AuditedActions audited(std::move(backend),[&](const AuditEvent& event){events.push_back(event);},"test caller");
    for(const auto error:{ActionError::none,ActionError::not_supported,ActionError::permission_denied,ActionError::stale_target,ActionError::system_error}){
        events.clear();mock->response.error=error;
        const auto result=audited.set_windows_run_registration_enabled(expected,true);
        assert(result.error==error && result.message==mock->response.message && mock->enabled && mock->generic_calls==0);
        assert(mock->received.entry_id==expected.entry_id && mock->received.command==expected.command && mock->received.native_data==expected.native_data && mock->received.native_type==expected.native_type && mock->received.state==expected.state);
        assert(events.size()==2 && events[0].id==events[1].id);
        assert(events[0].operation==AuditOperation::windows_run_registration && events[1].operation==AuditOperation::windows_run_registration);
        assert(events[0].stage==AuditStage::requested && !events[0].result && !events[0].backend_invoked);
        assert(events[1].stage==AuditStage::completed && events[1].result==error && events[1].backend_invoked && !events[1].backend_threw);
        for(const auto& event:events)assert(event.enabled && event.target==expected.entry_id && event.actor_label=="test caller" && event.target.find("secret")==std::string::npos && event.target.find("private")==std::string::npos);
    }
    expected.entry_id=std::string(6000,'A');events.clear();(void)audited.set_windows_run_registration_enabled(expected,false);
    assert(mock->received.entry_id.size()==6000 && !mock->enabled && events.back().target.size()==4096 && events.back().metadata_truncated && !events.back().enabled);
    mock->throwing=true;events.clear();bool threw=false;
    try{audited.set_windows_run_registration_enabled(expected,false);}catch(const std::runtime_error& e){threw=std::string(e.what())=="private exception detail";}
    assert(threw && events.size()==2 && events.back().backend_threw && events.back().backend_invoked && events.back().result==ActionError::system_error);
    {
        auto b=std::make_unique<Mock>();auto* m=b.get();AuditEvent completion;
        AuditedActions blocked(std::move(b),[&](const AuditEvent& e){if(e.stage==AuditStage::requested)throw 1;completion=e;});
        const auto result=blocked.set_windows_run_registration_enabled(expected,false);
        assert(!result && m->calls==0 && !completion.backend_invoked && blocked.diagnostics().failed_request_records==1);
    }
    {
        auto b=std::make_unique<Mock>();auto* m=b.get();
        AuditedActions best(std::move(b),[](const AuditEvent&){throw 1;},"",AuditFailurePolicy::best_effort);
        assert(best.set_windows_run_registration_enabled(expected,false) && m->calls==1 && best.diagnostics().failed_request_records==1 && best.diagnostics().failed_completed_records==1);
    }
    {
        auto b=std::make_unique<Mock>();auto* m=b.get();
        AuditedActions done(std::move(b),[](const AuditEvent& e){if(e.stage==AuditStage::completed)throw 1;});
        assert(done.set_windows_run_registration_enabled(expected,true) && m->calls==1 && done.diagnostics().failed_completed_records==1);
    }
    // Native dispatch only with invalid ID: no registry mutation can be requested by this test.
    {
        auto b=std::make_unique<Mock>();auto* m=b.get();const auto original=expected;
        AuditedActions frozen(std::move(b),[&](const AuditEvent& e){if(e.stage==AuditStage::requested){expected.entry_id="winrun:hklm:64:41";expected.native_data.clear();}});
        assert(frozen.set_windows_run_registration_enabled(expected,false) && m->received.entry_id==original.entry_id && m->received.native_data==original.native_data && m->received.command==original.command);
    }
    std::vector<AuditEvent> rejected;
    AuditedActions real(make_system_actions(),[&](const AuditEvent& e){rejected.push_back(e);});
    const auto result=real.set_windows_run_registration_enabled({},false);
    assert(!result && rejected.size()==2 && rejected.back().backend_invoked && rejected.back().result==result.error);
#ifdef _WIN32
    assert(result.error==ActionError::invalid_argument);
#else
    assert(result.error==ActionError::not_supported);
#endif
    std::cout<<"Managed Run audit: exact forwarding, distinct operation, privacy, sink failures, exception semantics and invalid native dispatch passed\n";
}
