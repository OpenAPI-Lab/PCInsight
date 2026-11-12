#pragma once
#ifdef _WIN32
#include "pcinsight/actions.hpp"
#include "pcinsight/types.hpp"
#include "windows_terminate.hpp"
#include <optional>
namespace pcinsight::detail {
using GetProcessInfoFn = BOOL (WINAPI*)(HANDLE, PROCESS_INFORMATION_CLASS, LPVOID, DWORD);
using SetProcessInfoFn = BOOL (WINAPI*)(HANDLE, PROCESS_INFORMATION_CLASS, LPVOID, DWORD);
GetProcessInfoFn windows_get_process_information();
SetProcessInfoFn windows_set_process_information();
struct EfficiencyApi {
    GetProcessInfoFn get = windows_get_process_information();
    SetProcessInfoFn set = windows_set_process_information();
    TerminateApi process{};
};
struct EfficiencyInfo { Metric<bool> available, requested; };
EfficiencyInfo read_windows_efficiency(HANDLE, EfficiencyApi = {});
ActionResult set_windows_efficiency(std::uint32_t, bool, EfficiencyApi = {},
                                   std::optional<std::uint64_t> expected_creation = std::nullopt);
}
#endif
