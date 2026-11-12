#pragma once
#include "pcinsight/actions.hpp"
#include "pcinsight/types.hpp"
#include <chrono>
#include <functional>
#include <optional>

namespace pcinsight {
enum class AuditOperation { launch_process, terminate_process, terminate_process_checked,
    efficiency_mode, efficiency_mode_checked, service, startup, disconnect_user, terminate_user_session, startup_current_user, startup_reset_current_user, windows_run_registration, windows_startup_file_registration };
enum class AuditStage { requested, completed };
enum class AuditFailurePolicy { require_request_record, best_effort };
struct AuditEvent {
    std::uint64_t id{}; // unique within this wrapper; both stages share an ID
    AuditOperation operation{};
    AuditStage stage{};
    std::chrono::system_clock::time_point timestamp;
    std::chrono::nanoseconds execution_time{};
    std::string actor_label, target; // caller-supplied label, NOT authenticated identity
    bool metadata_truncated{};
    std::uint32_t pid{};
    std::uint64_t expected_windows_creation_time_100ns{};
    bool force{}, enabled{}, elevated{};
    ServiceAction service_action{ServiceAction::start};
    std::optional<ActionError> result; // absent for requested stage; backend message omitted
    bool backend_invoked{}, backend_threw{};
    std::uint64_t expected_linux_start_ticks{};
};
using AuditSink = std::function<void(const AuditEvent&)>;
struct AuditDiagnostics {
    std::uint64_t failed_request_records{}, failed_completed_records{};
    std::string last_failure;
};

// Explicit opt-in decorator. No hidden files, background threads or elevation.
// Sink must be thread-safe and copy an event if retaining it beyond the callback.
// Default policy refuses an action if its request could not be recorded. A
// failed completion record never changes the already executed action's result.
class AuditedActions final : public SystemActions {
public:
    AuditedActions(std::unique_ptr<SystemActions> backend, AuditSink sink,
        std::string actor_label = {},
        AuditFailurePolicy policy = AuditFailurePolicy::require_request_record);
    ~AuditedActions() override;
    AuditedActions(const AuditedActions&) = delete;
    AuditedActions& operator=(const AuditedActions&) = delete;
    AuditDiagnostics diagnostics() const;
    ActionResult launch_process(const LaunchRequest&) override;
    ActionResult terminate_process(std::uint32_t, bool force = false) override;
    ActionResult terminate_process_checked(std::uint32_t, std::uint64_t, bool force = false) override;
    ActionResult set_efficiency_mode(std::uint32_t, bool) override;
    ActionResult set_efficiency_mode_checked(std::uint32_t, std::uint64_t, bool) override;
    ActionResult control_service(const std::string&, ServiceAction) override;
    ActionResult set_startup_enabled(const std::string&, bool) override;
    ActionResult set_windows_run_registration_enabled(const WindowsRunRegistration&, bool) override;
    ActionResult set_startup_enabled_for_current_user(const std::string&, bool) override;
    ActionResult reset_startup_for_current_user(const std::string&) override;
    ActionResult disconnect_user(const std::string&) override;
    ActionResult terminate_user_session(const std::string&) override;
    ActionResult set_windows_startup_file_registered(const WindowsStartupFileRegistration&, bool) override;
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
}
