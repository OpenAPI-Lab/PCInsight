#include "linux_service_actions.hpp"
#include <algorithm>
#include <cerrno>
#include <chrono>
#include <cstring>
#include <filesystem>
#include <string_view>
#ifdef __linux__
#include <dlfcn.h>
#endif

namespace pcinsight::detail {
namespace {
bool valid_job_path(std::string_view path) {
    constexpr std::string_view prefix = "/org/freedesktop/systemd1/job/";
    if (!path.starts_with(prefix) || path.size() <= prefix.size() || path.size() > 1024) return false;
    path.remove_prefix(prefix.size());
    return std::all_of(path.begin(), path.end(), [](char c) { return c >= '0' && c <= '9'; });
}
}
bool valid_systemd_service_name(const std::string& name) {
    if (name.size() <= 8 || name.size() > 255 || !name.ends_with(".service")) return false;
    // Canonical systemd unit names are ASCII; escaped byte names use \\xHH.
    auto hex = [](unsigned char c) { return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'); };
    for (std::size_t i = 0; i < name.size(); ++i) {
        const auto c = static_cast<unsigned char>(name[i]);
        if (c == '\\') {
            if (i + 3 >= name.size() || name[i + 1] != 'x' || !hex(name[i + 2]) || !hex(name[i + 3])) return false;
            i += 3;
        } else if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') ||
            c == '_' || c == '-' || c == '.' || c == ':' || c == '@')) return false;
    }
    return true;
}
void ServiceJobTracker::accept(ServiceJobCompletion completion) {
    if (!valid_job_path(completion.path) || completion.unit.size() > 1024 || completion.result.size() > 1024) {
        invalid = true; return;
    }
    if (!expected_path.empty()) {
        if (completion.path == expected_path) {
            if (result && *result != completion.result) invalid = true;
            else result = std::move(completion.result);
        }
    } else if (early.size() < 1024) early.push_back(std::move(completion));
    else invalid = true;
}
void ServiceJobTracker::select(std::string path) {
    expected_path = std::move(path);
    if (!valid_job_path(expected_path)) invalid = true;
    for (auto& completion : early) if (completion.path == expected_path) {
        if (result && *result != completion.result) invalid = true;
        else result = std::move(completion.result);
    }
    early.clear();
}
ActionResult systemd_job_result(const std::string& result) {
    if (result == "done") return {};
    return {ActionError::system_error, "systemd job completed without success: " + result};
}
SystemdActionApi::operator bool() const {
    return open && unref && message_unref && slot_unref && error_free && timeout && call && read && match && process && wait;
}
SystemdActionApi load_systemd_action_api() {
    SystemdActionApi api;
#ifdef __linux__
    auto* library = dlopen("libsystemd.so.0", RTLD_NOW | RTLD_LOCAL);
    if (!library) return api;
    api.library = std::shared_ptr<void>(library, [](void* p) { dlclose(p); });
#define PCINSIGHT_ACTION_SYMBOL(member, name) api.member = reinterpret_cast<decltype(api.member)>(dlsym(library, name))
    PCINSIGHT_ACTION_SYMBOL(open, "sd_bus_open_system");
    PCINSIGHT_ACTION_SYMBOL(unref, "sd_bus_unref");
    PCINSIGHT_ACTION_SYMBOL(message_unref, "sd_bus_message_unref");
    PCINSIGHT_ACTION_SYMBOL(slot_unref, "sd_bus_slot_unref");
    PCINSIGHT_ACTION_SYMBOL(error_free, "sd_bus_error_free");
    PCINSIGHT_ACTION_SYMBOL(timeout, "sd_bus_set_method_call_timeout");
    PCINSIGHT_ACTION_SYMBOL(call, "sd_bus_call_method");
    PCINSIGHT_ACTION_SYMBOL(read, "sd_bus_message_read");
    PCINSIGHT_ACTION_SYMBOL(match, "sd_bus_match_signal");
    PCINSIGHT_ACTION_SYMBOL(process, "sd_bus_process");
    PCINSIGHT_ACTION_SYMBOL(wait, "sd_bus_wait");
#undef PCINSIGHT_ACTION_SYMBOL
#endif
    return api;
}
namespace {
using Clock = std::chrono::steady_clock;
ActionResult failure(int rc, const char* operation, const ActionBusError* error = nullptr) {
    ActionError code = ActionError::system_error;
    if (rc == -EACCES || rc == -EPERM) code = ActionError::permission_denied;
    if (error && error->name) {
        const std::string name(error->name);
        if (name == "org.freedesktop.systemd1.NoSuchUnit") code = ActionError::not_found;
        if (name == "org.freedesktop.DBus.Error.AccessDenied" || name == "org.freedesktop.DBus.Error.InteractiveAuthorizationRequired") code = ActionError::permission_denied;
    }
    return {code, std::string(operation) + ": D-Bus error " + std::to_string(rc)};
}
struct Session {
    const SystemdActionApi& api;
    ActionBus* bus{}; ActionSlot* slot{}; ActionMessage* reply{}; ActionBusError error{};
    bool owns_bus{true};
    ~Session() {
        if (reply) api.message_unref(reply);
        if (slot) api.slot_unref(slot);
        api.error_free(&error);
        if (bus && owns_bus) api.unref(bus);
    }
};
struct SignalContext { const SystemdActionApi& api; ServiceJobTracker tracker; };
int removed(ActionMessage* message, void* userdata, ActionBusError*) noexcept {
    auto& context = *static_cast<SignalContext*>(userdata);
    try {
        unsigned id{}; const char *path{}, *unit{}, *result{};
        const int rc = context.api.read(message, "uoss", &id, &path, &unit, &result);
        if (rc <= 0 || !path || !unit || !result || strnlen(path, 1025) > 1024 ||
            strnlen(unit, 1025) > 1024 || strnlen(result, 1025) > 1024) context.tracker.invalid = true;
        else context.tracker.accept({path, unit, result});
    } catch (...) { context.tracker.invalid = true; }
    return 0;
}
}
ActionResult control_systemd_service(const std::string& name, ServiceAction action) {
    if (!valid_systemd_service_name(name) ||
        (action != ServiceAction::start && action != ServiceAction::stop && action != ServiceAction::restart))
        return {ActionError::invalid_argument, "invalid systemd service name or action"};
#ifdef __linux__
    std::error_code ec;
    if (!std::filesystem::exists("/run/systemd/system", ec) || ec)
        return {ActionError::not_supported, "systemd is not the active init manager"};
#else
    return {ActionError::not_supported, "systemd service control requires Linux"};
#endif
    auto api = load_systemd_action_api();
    if (!api) return {ActionError::not_supported, "libsystemd action API unavailable"};
    Session session{api};
    int rc = api.open(&session.bus);
    if (rc < 0) return failure(rc, "open system bus");
    return run_systemd_service_job(api,session.bus,"org.freedesktop.systemd1",name,action);
}
ActionResult run_systemd_service_job(const SystemdActionApi& api,ActionBus* bus,
    const std::string& destination,const std::string& name,ServiceAction action){
    if(!api||!bus||destination.empty()||!valid_systemd_service_name(name)||
        (action!=ServiceAction::start&&action!=ServiceAction::stop&&action!=ServiceAction::restart))
        return {ActionError::invalid_argument,"Invalid systemd job target/action/API"};
    SignalContext context{api,{}};
    Session session{api};session.bus=bus;session.owns_bus=false;
    auto deadline=Clock::now()+std::chrono::seconds(30);
    int rc = api.timeout(session.bus, 5000000);
    if (rc < 0) return failure(rc, "set call timeout");
    rc = api.match(session.bus, &session.slot, destination.c_str(), "/org/freedesktop/systemd1",
        "org.freedesktop.systemd1.Manager", "JobRemoved", removed, &context);
    if (rc < 0) return failure(rc, "subscribe JobRemoved");
    rc = api.call(session.bus, destination.c_str(), "/org/freedesktop/systemd1",
        "org.freedesktop.systemd1.Manager", "Subscribe", &session.error, &session.reply, "");
    if (rc < 0) return failure(rc, "Subscribe", &session.error);
    if (session.reply) { api.message_unref(session.reply); session.reply = nullptr; }
    const char* method = action == ServiceAction::start ? "StartUnit" : action == ServiceAction::stop ? "StopUnit" : "RestartUnit";
    rc = api.call(session.bus, destination.c_str(), "/org/freedesktop/systemd1",
        "org.freedesktop.systemd1.Manager", method, &session.error, &session.reply, "ss", name.c_str(), "replace");
    if (rc < 0) return failure(rc, method, &session.error);
    const char* job{};
    if (!session.reply || api.read(session.reply, "o", &job) <= 0 || !job || strnlen(job, 1025) > 1024 ||
        !valid_job_path(job))
        return {ActionError::system_error, "systemd accepted request but job identity is unavailable; outcome unknown"};
    context.tracker.select(job);
    for (unsigned messages = 0; messages < 100000 && Clock::now() < deadline; ++messages) {
        if (context.tracker.invalid) return {ActionError::system_error, "invalid/bounded job signal; outcome unknown"};
        if (context.tracker.result) return systemd_job_result(*context.tracker.result);
        rc = api.process(session.bus, nullptr);
        if (rc < 0) return failure(rc, "job wait; accepted request outcome unknown");
        if (rc == 0) {
            const auto remaining = std::chrono::duration_cast<std::chrono::microseconds>(deadline - Clock::now()).count();
            if (remaining <= 0) break;
            rc = api.wait(session.bus, static_cast<std::uint64_t>(std::min<std::int64_t>(remaining, 1000000)));
            if (rc < 0) return failure(rc, "job bus wait; accepted request outcome unknown");
        }
    }
    return {ActionError::system_error, "systemd job wait timeout/limit; request remains accepted, outcome unknown (not canceled)"};
}
}
