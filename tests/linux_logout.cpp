#include "../src/linux_logout.hpp"
#include "pcinsight/audit.hpp"
#include "pcinsight/monitor.hpp"
#include <cassert>
#include <iostream>
using namespace pcinsight;
using namespace pcinsight::detail;
struct Fake {
    LogoutApi api;
    LogoutToken token{":1.42", "/org/freedesktop/login1/session/_32", 12345};
    unsigned queries{}, closes{}, turns{};
    ActionError owner_error{ActionError::none}, query_error{ActionError::none}, close_error{ActionError::none};
    bool wrong_owner{}, wrong_time{}, removed{true}, post_error{};
    Fake() {
        api.owner = [&] { return std::pair{wrong_owner ? ":1.43" : token.owner, owner_error}; };
        api.query = [&](const LogoutToken& passed) {
            assert(passed.owner == token.owner && passed.path == token.path && passed.timestamp == token.timestamp);
            ++queries;
            if (queries > 1 && removed) return LogoutQuery{0, ActionError::not_found};
            if (queries > 1 && post_error) return LogoutQuery{0, ActionError::permission_denied};
            return LogoutQuery{wrong_time ? 999u : token.timestamp, query_error};
        };
        api.terminate = [&](const LogoutToken& passed) {
            assert(passed.owner == token.owner && passed.path == token.path); ++closes;
            return ActionResult{close_error, "fake close"};
        };
        api.next = [&] { return ++turns < 3; };
    }
    ActionResult run() { return logout_linux_session(logout_token(token), api); }
};
struct Backend : SystemActions {
    unsigned closed{};
    std::string token;
    ActionResult launch_process(const LaunchRequest&) override { return {}; }
    ActionResult terminate_process(std::uint32_t, bool) override { return {}; }
    ActionResult set_efficiency_mode(std::uint32_t, bool) override { return {}; }
    ActionResult control_service(const std::string&, ServiceAction) override { return {}; }
    ActionResult set_startup_enabled(const std::string&, bool) override { return {}; }
    ActionResult disconnect_user(const std::string&) override { return {ActionError::not_supported, "detach"}; }
    ActionResult terminate_user_session(const std::string& s) override { ++closed; token=s; return {}; }
};
int main() {
    Fake f;
    const auto encoded = logout_token(f.token);
    auto parsed = parse_logout_token(encoded); assert(parsed && parsed->timestamp == 12345 && parsed->owner == ":1.42");
    assert(f.run() && f.closes == 1 && f.queries == 2);
    for (const auto& id : std::vector<std::string>{"", "pts/0", "42", "login1@:1.42/org/freedesktop/login1/session/_32#0",
        encoded + "x", encoded + std::string(1,'\0'), "login1@:1.42/org/freedesktop/login1/session/../x#1", std::string(1341, 'x')}) {
        Fake bad; assert(logout_linux_session(id, bad.api).error == ActionError::invalid_argument && !bad.closes && !bad.queries);
    }
    { Fake g; g.wrong_owner=true; assert(g.run().error == ActionError::stale_target && !g.closes && !g.queries); }
    { Fake g; g.wrong_time=true; assert(g.run().error == ActionError::stale_target && !g.closes); }
    for (const auto error : {ActionError::permission_denied, ActionError::not_found, ActionError::not_supported, ActionError::system_error}) {
        Fake g; g.query_error=error; assert(g.run().error == error && !g.closes);
        Fake h; h.owner_error=error; assert(h.run().error == error && !h.closes);
        Fake i; i.close_error=error; assert(i.run().error == error && i.closes == 1 && i.queries == 1);
    }
    { Fake g; g.removed=false; assert(g.run().error == ActionError::system_error && g.closes == 1 && g.turns == 3); }
    { Fake g; g.removed=false; g.post_error=true; auto r=g.run(); assert(!r && r.message.find("aceptado") != std::string::npos && g.closes == 1); }
    { Fake g; g.api.query={}; assert(g.run().error == ActionError::not_supported && !g.closes); }
    auto backend=std::make_unique<Backend>(); auto* mock=backend.get(); std::vector<AuditEvent> events;
    AuditedActions audited(std::move(backend), [&](const AuditEvent& e) { events.push_back(e); });
    assert(audited.terminate_user_session(encoded)); assert(mock->closed==1 && mock->token==encoded);
    assert(events.size()==2 && events[0].operation==AuditOperation::terminate_user_session && events[1].result==ActionError::none);
#ifdef __linux__
    const auto native=read_logind_logins();
    std::cout << "login1 native read-only: " << (native ? std::to_string(native->size()) + " sessions" : "provider unavailable") << '\n';
    if (native) for (const auto& login : *native) {
        assert(login.id.starts_with("login1:"));
        if (!login.logout_token.empty()) assert(parse_logout_token(login.logout_token));
    }
    assert(make_system_actions()->terminate_user_session("pts/0").error==ActionError::invalid_argument);
    // No valid live session token is ever sent to the destructive native method.
    Monitor monitor; SampleOptions options; options.include_user_sessions=true;
    options.include_gpus=options.include_disks=options.include_networks=false;
    const auto snapshot=monitor.sample(options);
    for (const auto& row : snapshot.user_sessions) {
        assert(row.disconnect_available.value==false);
        if (!row.logout_token.empty()) assert(parse_logout_token(row.logout_token));
    }
#endif
}
