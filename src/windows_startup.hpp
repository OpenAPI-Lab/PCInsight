#pragma once
#ifdef _WIN32
#include "pcinsight/types.hpp"
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
namespace pcinsight::detail {
struct WindowsStartupApi {
    decltype(&RegOpenKeyExW) open=&RegOpenKeyExW;
    decltype(&RegEnumValueW) enumerate=&RegEnumValueW;
    decltype(&RegCloseKey) close=&RegCloseKey;
    decltype(&GetNativeSystemInfo) native_info=&GetNativeSystemInfo;
};
std::vector<StartupEntry> read_windows_startup(WindowsStartupApi api = {});
// Native Run + known Startup folders; either provider failing rejects the snapshot.
std::vector<StartupEntry> read_windows_startup_inventory();
}
#endif
