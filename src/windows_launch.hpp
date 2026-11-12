#pragma once
#ifdef _WIN32
#include "pcinsight/actions.hpp"
#include <windows.h>
namespace pcinsight::detail {
struct LaunchApi {
    decltype(&CreateProcessW) create = &CreateProcessW;
    decltype(&CloseHandle) close = &CloseHandle;
};
ActionResult launch_windows_process(const LaunchRequest&, LaunchApi = {});
}
#endif
