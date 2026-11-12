#include "pcinsight/audit.hpp"
#include <atomic>
#include <limits>
#include <mutex>
#include <stdexcept>

namespace pcinsight {
namespace {
constexpr std::size_t metadata_limit = 4096;
void bound(std::string& value, bool& truncated) {
    if (value.size() > metadata_limit) { value.resize(metadata_limit); truncated = true; }
}
void target(AuditEvent& event, const std::string& value) {
    event.target = value.substr(0, metadata_limit);
    event.metadata_truncated = value.size() > metadata_limit;
}
}
struct AuditedActions::Impl {
    std::unique_ptr<SystemActions> backend;
    AuditSink sink;
    std::string actor;
    bool actor_truncated{};
    AuditFailurePolicy policy;
    std::atomic<std::uint64_t> next_id{1};
    mutable std::mutex diagnostics_mutex;
    std::mutex backend_mutex;
    AuditDiagnostics diagnostic;

    Impl(std::unique_ptr<SystemActions> b, AuditSink s, std::string label, AuditFailurePolicy p)
        : backend(std::move(b)), sink(std::move(s)), actor(std::move(label)), policy(p) {
        if (!backend || !sink) throw std::invalid_argument("audit requires backend and sink");
        if (p != AuditFailurePolicy::require_request_record && p != AuditFailurePolicy::best_effort)
            throw std::invalid_argument("invalid audit policy");
        bound(actor, actor_truncated);
        diagnostic.last_failure.reserve(64);
    }
    bool emit(const AuditEvent& event) noexcept {
        try { sink(event); return true; }
        catch (...) {
            try {
            std::lock_guard lock(diagnostics_mutex);
            auto& counter = event.stage == AuditStage::requested ? diagnostic.failed_request_records : diagnostic.failed_completed_records;
            if (counter != std::numeric_limits<std::uint64_t>::max()) ++counter;
            // Do not copy exception text: it can contain credentials or arbitrary data.
            diagnostic.last_failure = event.stage == AuditStage::requested
                ? "audit request record failed" : "audit completion record failed";
            } catch (...) { /* even a diagnostic failure must not mask an executed action */ }
            return false;
        }
    }
    ActionResult invoke(AuditEvent event, const std::function<ActionResult()>& action) {
        auto id = next_id.load();
        do {
            if (id == std::numeric_limits<std::uint64_t>::max()) throw std::overflow_error("audit ID exhausted");
        } while (!next_id.compare_exchange_weak(id, id + 1));
        event.id = id;
        event.actor_label = actor;
        event.metadata_truncated |= actor_truncated;
        bound(event.target, event.metadata_truncated);
        event.timestamp = std::chrono::system_clock::now();
        const bool recorded = emit(event);
        event.stage = AuditStage::completed;
        if (!recorded && policy == AuditFailurePolicy::require_request_record) {
            event.timestamp = std::chrono::system_clock::now();
            event.result = ActionError::system_error;
            emit(event);
            return {ActionError::system_error, "audit request could not be recorded; action not executed"};
        }
        auto start = std::chrono::steady_clock::now();
        ActionResult result;
        try {
            std::lock_guard lock(backend_mutex);
            start = std::chrono::steady_clock::now();
            event.backend_invoked = true;
            result = action();
        } catch (...) {
            event.backend_threw = event.backend_invoked;
            event.result = ActionError::system_error;
            event.execution_time = std::chrono::steady_clock::now() - start;
            event.timestamp = std::chrono::system_clock::now();
            emit(event);
            throw; // preserve backend semantics; exception does not prove no side effects
        }
        event.result = result.error;
        event.execution_time = std::chrono::steady_clock::now() - start;
        event.timestamp = std::chrono::system_clock::now();
        emit(event); // operation may already have succeeded; never falsely report rollback
        return result;
    }
};

AuditedActions::AuditedActions(std::unique_ptr<SystemActions> backend, AuditSink sink,
    std::string label, AuditFailurePolicy policy)
    : impl_(std::make_unique<Impl>(std::move(backend), std::move(sink), std::move(label), policy)) {}
AuditedActions::~AuditedActions() = default;
AuditDiagnostics AuditedActions::diagnostics() const {
    std::lock_guard lock(impl_->diagnostics_mutex);
    return impl_->diagnostic;
}
ActionResult AuditedActions::launch_process(const LaunchRequest& request) {
    AuditEvent event; event.operation = AuditOperation::launch_process;
    target(event, request.executable); event.elevated = request.elevated;
    return impl_->invoke(std::move(event), [&] { return impl_->backend->launch_process(request); });
}
ActionResult AuditedActions::terminate_process(std::uint32_t pid, bool force) {
    AuditEvent event; event.operation = AuditOperation::terminate_process; event.pid = pid; event.force = force;
    return impl_->invoke(std::move(event), [&] { return impl_->backend->terminate_process(pid, force); });
}
ActionResult AuditedActions::terminate_process_checked(std::uint32_t pid, std::uint64_t identity, bool force) {
    AuditEvent event; event.operation = AuditOperation::terminate_process_checked;
    event.pid = pid; event.force = force;
#ifdef __linux__
    event.expected_linux_start_ticks = identity;
#else
    event.expected_windows_creation_time_100ns = identity;
#endif
    return impl_->invoke(std::move(event), [&] { return impl_->backend->terminate_process_checked(pid, identity, force); });
}
ActionResult AuditedActions::set_efficiency_mode(std::uint32_t pid, bool enabled) {
    AuditEvent event; event.operation = AuditOperation::efficiency_mode; event.pid = pid; event.enabled = enabled;
    return impl_->invoke(std::move(event), [&] { return impl_->backend->set_efficiency_mode(pid, enabled); });
}
ActionResult AuditedActions::set_efficiency_mode_checked(std::uint32_t pid, std::uint64_t identity, bool enabled) {
    AuditEvent event; event.operation = AuditOperation::efficiency_mode_checked;
    event.pid = pid; event.expected_windows_creation_time_100ns = identity; event.enabled = enabled;
    return impl_->invoke(std::move(event), [&] { return impl_->backend->set_efficiency_mode_checked(pid, identity, enabled); });
}
ActionResult AuditedActions::control_service(const std::string& service, ServiceAction action) {
    AuditEvent event; event.operation = AuditOperation::service; target(event, service); event.service_action = action;
    return impl_->invoke(std::move(event), [&] { return impl_->backend->control_service(service, action); });
}
ActionResult AuditedActions::set_startup_enabled(const std::string& entry, bool enabled) {
    AuditEvent event; event.operation = AuditOperation::startup; target(event, entry); event.enabled = enabled;
    return impl_->invoke(std::move(event), [&] { return impl_->backend->set_startup_enabled(entry, enabled); });
}
ActionResult AuditedActions::set_windows_run_registration_enabled(const WindowsRunRegistration& expected, bool enabled) {
    const auto frozen=expected; // Fix hive/view/content before calling the sink.
    AuditEvent event;event.operation=AuditOperation::windows_run_registration;
    target(event,frozen.entry_id);event.enabled=enabled;
    // Never put command, native bytes/type or backend diagnostics into the event.
    return impl_->invoke(std::move(event),[&]{return impl_->backend->set_windows_run_registration_enabled(frozen,enabled);});
}
ActionResult AuditedActions::set_startup_enabled_for_current_user(const std::string& entry, bool enabled) {
    AuditEvent event; event.operation = AuditOperation::startup_current_user; target(event, entry); event.enabled = enabled;
    return impl_->invoke(std::move(event), [&] { return impl_->backend->set_startup_enabled_for_current_user(entry, enabled); });
}
ActionResult AuditedActions::disconnect_user(const std::string& session) {
    AuditEvent event; event.operation = AuditOperation::disconnect_user; target(event, session);
    return impl_->invoke(std::move(event), [&] { return impl_->backend->disconnect_user(session); });
}
ActionResult AuditedActions::reset_startup_for_current_user(const std::string& entry) {
    AuditEvent event; event.operation = AuditOperation::startup_reset_current_user; target(event, entry);
    return impl_->invoke(std::move(event), [&] { return impl_->backend->reset_startup_for_current_user(entry); });
}
ActionResult AuditedActions::terminate_user_session(const std::string& token) {
    AuditEvent event; event.operation = AuditOperation::terminate_user_session; target(event, token);
    return impl_->invoke(std::move(event), [&] { return impl_->backend->terminate_user_session(token); });
}
ActionResult AuditedActions::set_windows_startup_file_registered(const WindowsStartupFileRegistration& observed,bool registered){
    // Freeze this request before the sink callback. The caller must still avoid
    // concurrent mutation while this copy is made; the filesystem is rechecked by the backend.
    const auto expected=observed;
    AuditEvent event;event.operation=AuditOperation::windows_startup_file_registration;
    target(event,expected.entry_id);event.enabled=registered;
    // No file/directory identity, content or backend diagnostic is added to the log.
    return impl_->invoke(std::move(event),[&]{return impl_->backend->set_windows_startup_file_registered(expected,registered);});
}
}
