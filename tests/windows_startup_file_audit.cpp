#include "pcinsight/audit.hpp"
#include <cassert>
#include <iostream>
#include <stdexcept>
using namespace pcinsight;
struct Mock final:SystemActions {
    unsigned calls{},generic_calls{};bool registered{},throwing{};WindowsStartupFileRegistration received;
    ActionResult response{ActionError::none,"private backend diagnostic"};
    ActionResult launch_process(const LaunchRequest&)override{return {ActionError::not_supported,""};}
    ActionResult terminate_process(std::uint32_t,bool)override{return {ActionError::not_supported,""};}
    ActionResult set_efficiency_mode(std::uint32_t,bool)override{return {ActionError::not_supported,""};}
    ActionResult control_service(const std::string&,ServiceAction)override{return {ActionError::not_supported,""};}
    ActionResult set_startup_enabled(const std::string&,bool)override{++generic_calls;return {ActionError::not_supported,""};}
    ActionResult disconnect_user(const std::string&)override{return {ActionError::not_supported,""};}
    ActionResult set_windows_startup_file_registered(const WindowsStartupFileRegistration& value,bool enable)override{
        ++calls;received=value;registered=enable;if(throwing)throw std::runtime_error("private exception detail");return response;
    }
};
int main(){
    static_assert(static_cast<int>(AuditOperation::windows_run_registration)==11);
    static_assert(static_cast<int>(AuditOperation::windows_startup_file_registration)==12);
    WindowsStartupFileRegistration expected;expected.entry_id="winfolder:user:46697874757265";
    expected.state=WindowsStartupFileState::retained_by_pcinsight;expected.directory_identity={1,2,3,4,5};expected.file_identity=WindowsStartupFileIdentity{6,7,8,9,10};
    auto backend=std::make_unique<Mock>();auto* mock=backend.get();std::vector<AuditEvent> events;
    AuditedActions audited(std::move(backend),[&](const AuditEvent& event){events.push_back(event);},"test caller");
    for(bool registered:{false,true})for(auto error:{ActionError::none,ActionError::not_supported,ActionError::permission_denied,ActionError::not_found,ActionError::invalid_argument,ActionError::stale_target,ActionError::system_error}){
        events.clear();mock->response.error=error;
        const auto result=audited.set_windows_startup_file_registered(expected,registered);
        assert(result.error==error && result.message==mock->response.message && mock->registered==registered && mock->generic_calls==0);
        assert(mock->received.entry_id==expected.entry_id && mock->received.state==expected.state && mock->received.directory_identity==expected.directory_identity && mock->received.file_identity==expected.file_identity);
        assert(events.size()==2 && events.front().id==events.back().id);
        assert(events.front().stage==AuditStage::requested && !events.front().result && !events.front().backend_invoked);
        assert(events.back().stage==AuditStage::completed && events.back().result==error && events.back().backend_invoked && !events.back().backend_threw);
        for(const auto& event:events)assert(event.operation==AuditOperation::windows_startup_file_registration && event.enabled==registered && event.target==expected.entry_id && event.actor_label=="test caller" && !event.pid && !event.expected_windows_creation_time_100ns && !event.expected_linux_start_ticks);
    }
    expected.entry_id=std::string(6000,'A');events.clear();(void)audited.set_windows_startup_file_registered(expected,false);
    assert(mock->received.entry_id.size()==6000 && events.back().target.size()==4096 && events.back().metadata_truncated);
    mock->throwing=true;events.clear();bool threw=false;
    try{audited.set_windows_startup_file_registered(expected,false);}catch(const std::runtime_error& e){threw=std::string(e.what())=="private exception detail";}
    assert(threw && events.size()==2 && events.back().backend_threw && events.back().result==ActionError::system_error);
    {
        auto b=std::make_unique<Mock>();auto* m=b.get();AuditEvent completion;
        AuditedActions blocked(std::move(b),[&](const AuditEvent& e){if(e.stage==AuditStage::requested)throw 1;completion=e;});
        assert(!blocked.set_windows_startup_file_registered(expected,false) && m->calls==0 && !completion.backend_invoked && blocked.diagnostics().failed_request_records==1);
    }
    {
        auto b=std::make_unique<Mock>();auto* m=b.get();
        AuditedActions best(std::move(b),[](const AuditEvent&){throw 1;},"",AuditFailurePolicy::best_effort);
        assert(best.set_windows_startup_file_registered(expected,false) && m->calls==1 && best.diagnostics().failed_request_records==1 && best.diagnostics().failed_completed_records==1);
    }
    {
        auto b=std::make_unique<Mock>();auto* m=b.get();
        AuditedActions done(std::move(b),[](const AuditEvent& e){if(e.stage==AuditStage::completed)throw 1;});
        assert(done.set_windows_startup_file_registered(expected,true) && m->calls==1 && done.diagnostics().failed_completed_records==1);
    }
    {
        auto b=std::make_unique<Mock>();auto* m=b.get();const auto before=expected;
        AuditedActions stable(std::move(b),[&](const AuditEvent& e){if(e.stage==AuditStage::requested){expected.entry_id="changed by callback";expected.file_identity.reset();}});
        assert(stable.set_windows_startup_file_registered(expected,false) && m->received.entry_id==before.entry_id && m->received.file_identity==before.file_identity);
    }
    // Factory dispatch only for an invalid request: never mutate actual Startup.
    std::vector<AuditEvent> rejected;AuditedActions real(make_system_actions(),[&](const AuditEvent& e){rejected.push_back(e);});
    const auto result=real.set_windows_startup_file_registered({},false);
    assert(!result && rejected.size()==2 && rejected.back().backend_invoked && rejected.back().result==result.error);
#ifndef _WIN32
    assert(result.error==ActionError::not_supported);
#endif
    std::cout<<"Startup file audit: forwarding, frozen request, distinct operation, bounded metadata, errors, sink policies, exceptions and invalid native dispatch passed\n";
}
