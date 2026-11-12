#include "../src/linux_service_actions.hpp"
#include "../src/linux_systemd.hpp"
#include <cassert>
#include <chrono>
#include <iostream>
#include <stdexcept>
#include <thread>
#ifdef __linux__
#include <filesystem>
#include <unistd.h>
#endif
using namespace pcinsight;
using namespace pcinsight::detail;
#ifdef __linux__
struct Fixture {
    SystemdActionApi api{load_systemd_action_api()};
    ActionBus* bus{};
    std::string unit;
    bool owned{}, referenced{};
    Fixture() {
        if (!api || api.open(&bus) < 0) throw std::runtime_error("fixture bus unavailable");
        api.timeout(bus, 5000000);
        unit = "pcinsight-test-" + std::to_string(getpid()) + "-" +
            std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) + ".service";
    }
    template<class... Args> void invoke(const char* method, const char* signature, Args... args) {
        ActionMessage* reply{}; ActionBusError error{};
        int rc = api.call(bus, "org.freedesktop.systemd1", "/org/freedesktop/systemd1",
            "org.freedesktop.systemd1.Manager", method, &error, &reply, signature, args...);
        std::string message = error.message ? error.message : "";
        if (reply) api.message_unref(reply);
        api.error_free(&error);
        if (rc < 0) throw std::runtime_error(std::string(method) + ": " + message + " / " + std::to_string(rc));
    }
    void create(const std::string& executable, bool fail = false) {
        // No shell, external utility, files or existing service configuration.
        invoke("StartTransientUnit", "ssa(sv)a(sa(sv))", unit.c_str(), "fail", 4,
            "Description", "s", "PCInsight isolated action fixture",
            "ExecStart", "a(sasb)", 1, executable.c_str(), 2, executable.c_str(), fail ? "--fixture-fail" : "--fixture", 0,
            "Type", "s", fail ? "oneshot" : "simple",
            "RemainAfterExit", "b", 1, 0);
        owned = true;
        invoke("RefUnit", "s", unit.c_str()); referenced = true;
        std::cout << "Owned transient fixture: " << unit << std::endl;
    }
    ~Fixture() {
        if (bus) {
            if (owned) { try { invoke("StopUnit", "ss", unit.c_str(), "replace"); } catch (...) {} }
            if (referenced) { try { invoke("UnrefUnit", "s", unit.c_str()); } catch (...) {} }
            api.unref(bus);
        }
    }
    void release() {
        invoke("UnrefUnit", "s", unit.c_str()); referenced = false;
        api.unref(bus); bus = nullptr; owned = false;
    }
};
ServiceInfo inspect(const std::string& unit) {
    auto rows = read_native_systemd_services();
    if (!rows) throw std::runtime_error("native service inventory unavailable");
    for (const auto& row : *rows) if (row.id == unit) return row;
    throw std::runtime_error("fixture disappeared before inspection");
}
void native_fixture() {
    if (geteuid() != 0) throw std::runtime_error("isolated native fixture requires explicitly selected root test context");
    Fixture fixture;
    fixture.create(std::filesystem::read_symlink("/proc/self/exe").string());
    auto actions = make_system_actions();
    auto perform = [&](ServiceAction action) {
        const auto result = actions->control_service(fixture.unit, action);
        if (!result) throw std::runtime_error(result.message);
    };
    perform(ServiceAction::start);
    assert(inspect(fixture.unit).status == "running");
    perform(ServiceAction::stop);
    assert(inspect(fixture.unit).active_state == "inactive");
    perform(ServiceAction::stop); // idempotent job
    perform(ServiceAction::start);
    const auto first = inspect(fixture.unit);
    assert(first.status == "running" && first.pid);
    perform(ServiceAction::restart);
    const auto restarted = inspect(fixture.unit);
    assert(restarted.status == "running" && restarted.pid && first.pid != restarted.pid);
    perform(ServiceAction::stop);
    assert(inspect(fixture.unit).active_state == "inactive");
    const auto name = fixture.unit;
    fixture.release();
    bool removed = false;
    for (unsigned attempt = 0; attempt < 20; ++attempt) {
        auto rows = read_native_systemd_services();
        assert(rows);
        removed = true;
        for (const auto& row : *rows) if (row.id == name) removed = false;
        if (removed) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    assert(removed);
    std::cout << "start/stop/restart confirmed; own fixture stopped and removed\n";
    Fixture failure;
    failure.create(std::filesystem::read_symlink("/proc/self/exe").string(), true);
    const auto failed = actions->control_service(failure.unit, ServiceAction::start);
    assert(!failed && failed.error == ActionError::system_error);
    assert(failed.message.find("job completed without success: failed") != std::string::npos);
    assert(inspect(failure.unit).active_state == "failed");
    const auto stopped = actions->control_service(failure.unit, ServiceAction::stop);
    if (!stopped) throw std::runtime_error(stopped.message);
    // systemd deliberately retains failed units for diagnosis; only the test
    // fixture's failure record is reset so that its transient definition can GC.
    failure.invoke("ResetFailedUnit", "s", failure.unit.c_str());
    const auto failure_name = failure.unit;
    failure.release();
    removed = false;
    for (unsigned attempt = 0; attempt < 20; ++attempt) {
        auto rows = read_native_systemd_services(); assert(rows);
        removed = true;
        for (const auto& row : *rows) if (row.id == failure_name) removed = false;
        if (removed) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    if (!removed) throw std::runtime_error("owned failed fixture did not unload: " + failure_name);
    std::cout << "real failed oneshot reported as failure; own failed fixture stopped and removed\n";
}
void cleanup_fixture(const std::string& name) {
    if (geteuid() != 0 || !name.starts_with("pcinsight-test-") || !valid_systemd_service_name(name))
        throw std::runtime_error("refuse cleanup outside owned test namespace");
    const auto rows = read_native_systemd_services();
    if (!rows) throw std::runtime_error("cleanup inventory unavailable");
    std::optional<ServiceInfo> row;
    for (const auto& service : *rows) if (service.id == name) row = service;
    if (!row) { std::cout << "Owned fixture already absent: " << name << '\n'; return; }
    if (row->description != "PCInsight isolated action fixture") throw std::runtime_error("fixture description mismatch");
    const auto stopped = make_system_actions()->control_service(name, ServiceAction::stop);
    if (!stopped) throw std::runtime_error(stopped.message);
    Fixture api;
    api.invoke("ResetFailedUnit", "s", name.c_str());
    std::cout << "Stopped and cleared owned fixture: " << name << '\n';
}
#endif
int main(int argc, char** argv) {
#ifdef __linux__
    if (argc == 2 && std::string(argv[1]) == "--fixture") { for (;;) pause(); }
    if (argc == 2 && std::string(argv[1]) == "--fixture-fail") return 7;
    if (argc == 2 && std::string(argv[1]) == "--native-fixture") { native_fixture(); return 0; }
    if (argc == 3 && std::string(argv[1]) == "--cleanup-fixture") { cleanup_fixture(argv[2]); return 0; }
#else
    (void)argc; (void)argv;
#endif
    assert(valid_systemd_service_name("dbus.service"));
    assert(valid_systemd_service_name("worker@one.service"));
    assert(valid_systemd_service_name("worker\\x20one.service"));
    assert(!valid_systemd_service_name("worker\\xZZ.service"));
    assert(!valid_systemd_service_name("worker\\.service"));
    for (const auto& name : {"", "a", ".service", "../bad.service", "bad\n.service", "a.socket"})
        assert(!valid_systemd_service_name(name));
    assert(!valid_systemd_service_name(std::string("a\0b.service", 11)));
    assert(!valid_systemd_service_name(std::string(260, 'a') + ".service"));
    assert(control_systemd_service("../bad.service", ServiceAction::start).error == ActionError::invalid_argument);
    assert(control_systemd_service("safe.service", static_cast<ServiceAction>(999)).error == ActionError::invalid_argument);
    assert(systemd_job_result("done"));
    for (const auto& state : {"failed", "canceled", "timeout", "dependency", "skipped", "unknown", ""})
        assert(!systemd_job_result(state));
    const std::string job = "/org/freedesktop/systemd1/job/123";
    ServiceJobTracker early;
    early.accept({job, "canonical.service", "done"});
    assert(!early.result);
    early.select(job); assert(early.result == "done"); // result can arrive inside synchronous StartUnit
    ServiceJobTracker late;
    late.select(job);
    late.accept({"/org/freedesktop/systemd1/job/124", "other.service", "done"});
    assert(!late.result);
    late.accept({job, "alias-primary.service", "failed"}); assert(late.result == "failed");
    late.accept({job, "alias-primary.service", "done"}); assert(late.invalid);
    ServiceJobTracker malformed;
    malformed.accept({"bad", "unit.service", "done"}); assert(malformed.invalid);
    ServiceJobTracker malformed_suffix;
    malformed_suffix.select("/org/freedesktop/systemd1/job/not_numeric"); assert(malformed_suffix.invalid);
    ServiceJobTracker bounded;
    for (unsigned i = 0; i < 1025; ++i) bounded.accept({job, "unit.service", "done"});
    assert(bounded.invalid && bounded.early.size() == 1024);
}
