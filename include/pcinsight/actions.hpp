#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace pcinsight {
struct WindowsRunRegistration;
struct WindowsStartupFileRegistration;

enum class ActionError { none, not_supported, permission_denied, not_found, invalid_argument, system_error, stale_target };
struct ActionResult {
    ActionError error{ActionError::none};
    std::string message;
    // Launch child PID; also set if the Linux error channel failed (state unknown).
    // Linux: direct child; caller must reap with waitpid whenever this is nonzero.
    // A PID is not a durable identity or a promise that initialization succeeded.
    std::uint32_t process_id{};
    // Reset startup: recoverable personal file, also populated after a commit
    // whose directory fsync failed. Empty means no backup was published.
    std::string startup_backup_path{};
    explicit operator bool() const { return error == ActionError::none; }
};
enum class ServiceAction { start, stop, restart };
struct LaunchRequest {
    // Linux: byte strings, no embedded NUL; PATH search for names without '/'.
    // Args are literal, no shell parsing/fallback. Environment/fds are inherited.
    // Successful direct children belong to caller (waitpid); no hidden reaper.
    // Windows: UTF-8, fully qualified drive/UNC executable path (no PATH search).
    std::string executable;
    // Windows arguments use Microsoft C runtime quoting, not shell syntax.
    std::vector<std::string> arguments;
    // Windows: empty inherits caller's cwd; otherwise fully qualified drive/UNC.
    std::string working_directory;
    // Windows currently rejects true until an authorized UAC helper is available.
    bool elevated{false};
};

class SystemActions {
public:
    virtual ~SystemActions() = default;
    virtual ActionResult launch_process(const LaunchRequest& request) = 0;
    // Windows: force=false is unsupported; force=true verifies exit for up to 5s.
    // Rejects self/reserved/critical processes; no elevation or process-tree kill.
    // A PID alone does not protect against reuse before this call opens the process.
    // Linux: pidfd-only signaling, rejects self/0/1/out-of-range, waits up to 5s
    // for exit. No process-tree kill or unsafe fallback on old kernels.
    virtual ActionResult terminate_process(std::uint32_t pid, bool force = false) = 0;
    // Windows: compare ProcessInfo::windows_creation_time_100ns on the SAME handle
    // used for termination. Zero is invalid; never falls back to unchecked PID.
    // Linux: expected ProcessInfo::linux_start_ticks from this boot/PID namespace,
    // then signal the pinned pidfd. Tick-resolution identity is not globally unique.
    virtual ActionResult terminate_process_checked(std::uint32_t,
        std::uint64_t /*expected_native_creation_token*/, bool /*force*/ = false) {
        return {ActionError::not_supported, "verified process identity is not implemented on this backend"};
    }
    // Linux: disabled, returns not_supported without touching the target.
    // Advanced callers may use linux_efficiency.hpp's explicitly acknowledged
    // unchecked workflow, NEVER as an automatic fallback from this method.
    virtual ActionResult set_efficiency_mode(std::uint32_t pid, bool enabled) = 0;
    // Linux remains unsupported: priority syscalls don't accept a stable pidfd.
    // Comparing a start token then setpriority(PID) would still have a reuse race.
    virtual ActionResult set_efficiency_mode_checked(std::uint32_t,
        std::uint64_t /*expected_windows_creation_time_100ns*/, bool /*enabled*/) {
        return {ActionError::not_supported, "verified efficiency action is not implemented on this backend"};
    }
    // Windows: internal UTF-8 service name; synchronous, caller's existing rights.
    // Waits for the target state (30s polling budget; native RPC can take longer).
    // Does not elevate privileges or recursively stop dependent services.
    // Linux/systemd: canonical .service name; waits for this request's job result.
    // Success means job result=done, not application readiness or a permanent state.
    // Polling deadline 30s, D-Bus calls 5s; timeout doesn't cancel accepted work.
    virtual ActionResult control_service(const std::string& service, ServiceAction action) = 0;
    virtual ActionResult set_startup_enabled(const std::string& entry, bool enabled) = 0;
    // Explicit PCInsight HKCU/shared or HKLM/32/64 Run backup/restore by inventory ID,
    // NOT Windows approval or the generic action above. HKLM can affect all users.
    // Same checked contract as the startup.hpp free function; requires native transactions.
    virtual ActionResult set_windows_run_registration_enabled(const WindowsRunRegistration&, bool) {
        return {ActionError::not_supported, "managed Windows Run control is not implemented on this backend"};
    }
    // Linux: current caller's XDG context. Create/edit a personal override;
    // never edit the selected system-wide source. Existing personal edits win.
    // This changes configuration, not running processes or startup eligibility.
    virtual ActionResult set_startup_enabled_for_current_user(const std::string&, bool) {
        return {ActionError::not_supported, "personal startup override provider not implemented"};
    }
    // Linux: selected PERSONAL .desktop path only. Requires a valid lower-priority
    // system counterpart; archives the personal file without replacing anything.
    // Discards personal customizations from future logins, not running processes.
    virtual ActionResult reset_startup_for_current_user(const std::string&) {
        return {ActionError::not_supported, "startup reset provider not implemented"};
    }
    // Disconnect must NOT silently mean terminate/logout. Linux currently fails
    // closed with not_supported: utmp/TTY IDs cannot identify a safe detach API.
    // No SIGHUP by guessed process group, no implicit logind TerminateSession.
    // linux_session_control.hpp explains each built-in adapter's static capability.
    virtual ActionResult disconnect_user(const std::string& session_id) = 0;
    // Explicit destructive logout: Linux logind Terminate of a sampled session.
    // Pass UserSession::logout_token unchanged, not id/TTY/PID. Can terminate
    // applications and lose unsaved work; no elevation/interactive authorization.
    virtual ActionResult terminate_user_session(const std::string&) {
        return {ActionError::not_supported, "session termination provider not implemented"};
    }
    // Explicit checked Startup source-file retention/restore, NOT Windows approval.
    // Same contract as windows_startup_file_control.hpp; no generic startup fallback.
    virtual ActionResult set_windows_startup_file_registered(const WindowsStartupFileRegistration&, bool) {
        return {ActionError::not_supported, "Windows Startup file control is not implemented on this backend"};
    }
};

std::unique_ptr<SystemActions> make_system_actions();

} // namespace pcinsight
