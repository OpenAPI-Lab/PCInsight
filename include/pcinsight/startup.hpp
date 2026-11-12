#pragma once
#include "actions.hpp"
#include "types.hpp"
namespace pcinsight {
struct StartupProperty {
    std::string group, key, raw_value; // XDG raw encoding or Windows registry UTF-8; NOT shell tokens
};
struct StartupProperties {
    StartupEntry entry;
    std::uint64_t file_size_bytes{}; // Legacy Linux .desktop size only; do not interpret on Windows.
    std::vector<StartupProperty> fields; // order and duplicates preserved
    Metric<std::uint64_t> source_file_size_bytes; // No value for registry sources (not a zero-byte file).
};
struct StartupPropertiesResult {
    std::optional<StartupProperties> value;
    ActionError error{ActionError::none};
    std::string message;
    explicit operator bool() const {return value.has_value() && error==ActionError::none;}
};
// Read-only Linux XDG or Windows Run/Startup-source-file query. Pass a StartupEntry.id.
// Does not execute Exec, resolve its command to an executable, or open OS dialogs.
StartupPropertiesResult get_startup_properties(const std::string& entry_id);
// Explicit PCInsight-owned registration management, NOT Windows StartupApproved.
// HKCU/shared or HKLM/32/64 selected by inventory ID. Not Windows approval.
// WindowsRunRegistration/State are defined in types.hpp for snapshots as well.
// Read-only, two matching observations, estimated/non-atomic; does not require KTM.
// Reading a state does NOT imply the transaction-based mutation provider is available.
Metric<WindowsRunRegistration> read_windows_run_registration(const std::string& entry_id);
// Bounded, two matching enumerations; estimated/non-atomic. No KTM or writes.
Metric<std::vector<WindowsRunRegistration>> list_windows_run_registrations();
enum class WindowsMachineRunView { registry32, registry64 };
// Separate explicit machine-view listing, including PCInsight backups in that view.
// Existing list_windows_run_registrations remains HKCU-only; no elevation/KTM/writes.
Metric<std::vector<WindowsRunRegistration>> list_windows_machine_run_registrations(WindowsMachineRunView view);
// false: atomically move Run value to PCInsight backup; true: restore without overwrite.
// Pass the latest read unchanged in the same Windows user context. No elevation/execution.
// HKLM can affect ALL users, requires existing rights and native registry transactions.
ActionResult set_windows_run_registration_enabled(const WindowsRunRegistration& expected,bool enabled);
}
