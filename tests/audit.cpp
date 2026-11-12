#include "pcinsight/audit.hpp"
#include <cassert>
#include <mutex>
#include <set>
#include <stdexcept>
#include <thread>

using namespace pcinsight;
struct Mock final : SystemActions {
    unsigned calls{};
    bool throwing{};
    std::uint32_t pid{};
    std::uint64_t identity{};
    bool flag{};
    std::string text;
    ServiceAction action{};
    LaunchRequest launch;
    ActionResult response{ActionError::permission_denied, "backend diagnostic with private data"};
    ActionResult result() { ++calls; if (throwing) throw std::runtime_error("original backend exception"); return response; }
    ActionResult launch_process(const LaunchRequest& r) override { launch = r; return result(); }
    ActionResult terminate_process(std::uint32_t p, bool f) override { pid = p; flag = f; return result(); }
    ActionResult terminate_process_checked(std::uint32_t p, std::uint64_t i, bool f) override {
        pid = p; identity = i; flag = f; return result();
    }
    ActionResult set_efficiency_mode(std::uint32_t p, bool f) override { pid = p; flag = f; return result(); }
    ActionResult set_efficiency_mode_checked(std::uint32_t p, std::uint64_t i, bool f) override {
        pid = p; identity = i; flag = f; return result();
    }
    ActionResult control_service(const std::string& s, ServiceAction a) override { text = s; action = a; return result(); }
    ActionResult set_startup_enabled(const std::string& s, bool f) override { text = s; flag = f; return result(); }
    ActionResult disconnect_user(const std::string& s) override { text = s; return result(); }
};
int main() {
    auto backend = std::make_unique<Mock>(); auto* mock = backend.get();
    std::vector<AuditEvent> events;
    AuditedActions audited(std::move(backend), [&](const AuditEvent& e) { events.push_back(e); }, "test actor");
    LaunchRequest request{"/test/app", {"secret-password", "arg 2"}, "/private/cwd", true};
    auto r = audited.launch_process(request);
    assert(r.error == mock->response.error && r.message == mock->response.message);
    assert(mock->launch.arguments == request.arguments && mock->launch.working_directory == request.working_directory);
    assert(events[0].target == request.executable && events[0].elevated);
    (void)audited.terminate_process(10, true); assert(mock->pid == 10 && mock->flag);
    (void)audited.terminate_process_checked(11, 12345, false); assert(mock->pid == 11 && mock->identity == 12345 && !mock->flag);
    (void)audited.set_efficiency_mode(12, true); assert(mock->pid == 12 && mock->flag);
    (void)audited.set_efficiency_mode_checked(13, 54321, false); assert(mock->pid == 13 && mock->identity == 54321 && !mock->flag);
    (void)audited.control_service("fixture.service", ServiceAction::restart); assert(mock->text == "fixture.service" && mock->action == ServiceAction::restart);
    (void)audited.set_startup_enabled("fixture.desktop", false); assert(mock->text == "fixture.desktop" && !mock->flag);
    (void)audited.disconnect_user("fixture-session"); assert(mock->text == "fixture-session");
    assert(mock->calls == 8 && events.size() == 16);
    const AuditOperation expected[]{AuditOperation::launch_process, AuditOperation::terminate_process,
        AuditOperation::terminate_process_checked, AuditOperation::efficiency_mode, AuditOperation::efficiency_mode_checked,
        AuditOperation::service, AuditOperation::startup, AuditOperation::disconnect_user};
    for (unsigned i = 0; i < 8; ++i) {
        const auto& first = events[i * 2]; const auto& last = events[i * 2 + 1];
        assert(first.id == i + 1 && first.id == last.id && first.operation == expected[i] && first.operation == last.operation);
        assert(first.stage == AuditStage::requested && !first.result && !first.backend_invoked);
        assert(last.stage == AuditStage::completed && last.result == ActionError::permission_denied && last.backend_invoked);
        assert(!last.backend_threw && last.execution_time.count() >= 0);
        assert(first.actor_label == "test actor" && first.target.find("secret-password") == std::string::npos);
        assert(first.timestamp.time_since_epoch().count() > 0);
    }
#ifdef __linux__
    assert(events[4].expected_linux_start_ticks == 12345 && !events[4].expected_windows_creation_time_100ns);
#else
    assert(events[4].expected_windows_creation_time_100ns == 12345 && !events[4].expected_linux_start_ticks);
#endif
    assert(events[8].expected_windows_creation_time_100ns == 54321);
    assert(events[2].pid == 10 && events[2].force && events[4].pid == 11 && !events[4].force);
    assert(events[6].pid == 12 && events[6].enabled && events[8].pid == 13 && !events[8].enabled);
    assert(events[10].service_action == ServiceAction::restart && events[10].target == "fixture.service");
    assert(!events[12].enabled && events[12].target == "fixture.desktop" && events[14].target == "fixture-session");
    mock->throwing = true;
    try { (void)audited.disconnect_user("throw"); assert(false); }
    catch (const std::runtime_error& e) { assert(std::string(e.what()) == "original backend exception"); }
    assert(events.back().backend_threw && events.back().backend_invoked && events.back().result == ActionError::system_error);
    mock->throwing = false;
    (void)audited.disconnect_user(std::string(10000, 'a'));
    assert(mock->text.size() == 10000 && events.back().target.size() == 4096 && events.back().metadata_truncated);
    {
        auto b = std::make_unique<Mock>(); auto* m = b.get();
        AuditEvent completed;
        AuditedActions blocked(std::move(b), [&](const AuditEvent& e) {
            if (e.stage == AuditStage::requested) throw std::runtime_error("sink secret");
            completed = e;
        });
        assert(!blocked.terminate_process(9, true) && m->calls == 0);
        assert(!completed.backend_invoked && completed.result == ActionError::system_error);
        assert(blocked.diagnostics().failed_request_records == 1 && blocked.diagnostics().failed_completed_records == 0);
        assert(blocked.diagnostics().last_failure.find("secret") == std::string::npos);
    }
    {
        auto b = std::make_unique<Mock>(); auto* m = b.get();
        AuditedActions best(std::move(b), [](const AuditEvent&) { throw 1; }, "", AuditFailurePolicy::best_effort);
        assert(best.disconnect_user("x").error == ActionError::permission_denied && m->calls == 1);
        assert(best.diagnostics().failed_request_records == 1 && best.diagnostics().failed_completed_records == 1);
    }
    {
        auto b = std::make_unique<Mock>(); b->response = {}; auto* m = b.get();
        AuditedActions done(std::move(b), [](const AuditEvent& e) { if (e.stage == AuditStage::completed) throw 1; });
        assert(done.disconnect_user("x") && m->calls == 1);
        assert(done.diagnostics().failed_completed_records == 1);
    }
    {
        auto b = std::make_unique<Mock>(); auto* m = b.get();
        std::mutex mutex;
        std::vector<AuditEvent> concurrent;
        AuditedActions* observer{};
        AuditedActions safe(std::move(b), [&](const AuditEvent& e) {
            (void)observer->diagnostics(); // callback runs outside internal locks
            std::lock_guard lock(mutex); concurrent.push_back(e);
        }, std::string(5000, 'u'));
        observer = &safe;
        std::vector<std::thread> threads;
        for (unsigned i = 0; i < 4; ++i) threads.emplace_back([&] {
            for (unsigned j = 0; j < 50; ++j) (void)safe.terminate_process_checked(10, 20, true);
        });
        for (auto& thread : threads) thread.join();
        assert(m->calls == 200 && concurrent.size() == 400);
        std::set<std::uint64_t> requested, completed;
        for (const auto& e : concurrent) {
            assert(e.metadata_truncated && e.actor_label.size() == 4096);
            auto& ids = e.stage == AuditStage::requested ? requested : completed;
            assert(ids.insert(e.id).second);
        }
        assert(requested.size() == 200 && requested == completed);
    }
    try { AuditedActions bad(nullptr, [](const AuditEvent&) {}); assert(false); } catch (const std::invalid_argument&) {}
    try { AuditedActions bad(std::make_unique<Mock>(), {}); assert(false); } catch (const std::invalid_argument&) {}
    try { AuditedActions bad(std::make_unique<Mock>(), [](const AuditEvent&) {}, "", static_cast<AuditFailurePolicy>(99)); assert(false); }
    catch (const std::invalid_argument&) {}
}
