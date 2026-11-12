#pragma once
#ifdef _WIN32
#include "pcinsight/actions.hpp"
#include <windows.h>
namespace pcinsight::detail {
using IsCriticalFn = BOOL (WINAPI*)(HANDLE, PBOOL);
IsCriticalFn windows_is_critical();
struct TerminateApi {
    decltype(&OpenProcess) open = &OpenProcess;
    decltype(&CloseHandle) close = &CloseHandle;
    decltype(&GetCurrentProcessId) current_pid = &GetCurrentProcessId;
    IsCriticalFn critical = windows_is_critical();
    decltype(&TerminateProcess) terminate = &TerminateProcess;
    decltype(&WaitForSingleObject) wait = &WaitForSingleObject;
    decltype(&GetProcessTimes) times = &GetProcessTimes;
};
ActionResult terminate_windows_process(std::uint32_t, bool, TerminateApi = {});
ActionResult terminate_windows_process_checked(std::uint32_t, std::uint64_t, bool, TerminateApi = {});
}
#endif
