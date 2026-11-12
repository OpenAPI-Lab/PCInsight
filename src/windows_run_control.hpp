#pragma once
#include "pcinsight/startup.hpp"
#ifdef _WIN32
#include "windows_startup.hpp"
#include <ktmw32.h>
namespace pcinsight::detail {
inline constexpr wchar_t managed_run_path[]=L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";
inline constexpr wchar_t managed_run_backup_path[]=L"Software\\PCInsight\\StartupDisabled\\v1\\Run";
struct WindowsRunControlApi {
    decltype(&RegOpenCurrentUser) user=&RegOpenCurrentUser;
    decltype(&RegOpenKeyExW) open_plain=&RegOpenKeyExW; // Read-only observations, independent of KTM.
    decltype(&RegEnumValueW) enumerate=&RegEnumValueW;
    decltype(&RegOpenKeyTransactedW) open=&RegOpenKeyTransactedW;
    decltype(&RegCreateKeyTransactedW) create=&RegCreateKeyTransactedW;
    decltype(&RegQueryValueExW) query=&RegQueryValueExW;
    decltype(&RegSetValueExW) set=&RegSetValueExW;
    decltype(&RegDeleteValueW) erase=&RegDeleteValueW;
    decltype(&RegCloseKey) close_key=&RegCloseKey;
    decltype(&CreateTransaction) begin=&CreateTransaction;
    decltype(&CommitTransaction) commit=&CommitTransaction;
    decltype(&RollbackTransaction) rollback=&RollbackTransaction;
    decltype(&CloseHandle) close=&CloseHandle;
    decltype(&GetNativeSystemInfo) native_info=&GetNativeSystemInfo;
};
Metric<WindowsRunRegistration> read_windows_run_registration(const std::string&,WindowsRunControlApi);
Metric<std::vector<WindowsRunRegistration>> list_windows_run_registrations(WindowsRunControlApi);
Metric<std::vector<WindowsRunRegistration>> list_windows_machine_run_registrations(WindowsMachineRunView,WindowsRunControlApi);
ActionResult set_windows_run_registration_enabled(const WindowsRunRegistration&,bool,WindowsRunControlApi);
}
#endif
