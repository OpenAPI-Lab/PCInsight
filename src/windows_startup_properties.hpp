#pragma once
#ifdef _WIN32
#include "pcinsight/startup.hpp"
#include "windows_startup.hpp"
namespace pcinsight::detail {
struct WindowsStartupPropertiesApi {
    decltype(&RegOpenKeyExW) open=&RegOpenKeyExW;
    decltype(&RegQueryValueExW) query=&RegQueryValueExW;
    decltype(&RegCloseKey) close=&RegCloseKey;
    decltype(&GetNativeSystemInfo) native_info=&GetNativeSystemInfo;
};
StartupPropertiesResult get_windows_startup_properties(const std::string&,WindowsStartupPropertiesApi = {});
}
#endif
