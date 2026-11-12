#pragma once
#include "pcinsight/windows_services.hpp"
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <winsvc.h>
namespace pcinsight::detail {
struct WindowsServiceViewApi {
    decltype(&OpenSCManagerW) manager = &OpenSCManagerW;
    decltype(&OpenServiceW) open = &OpenServiceW;
    decltype(&QueryServiceStatusEx) query = &QueryServiceStatusEx;
    decltype(&QueryServiceConfigW) config = &QueryServiceConfigW;
    decltype(&QueryServiceConfig2W) description = &QueryServiceConfig2W;
    decltype(&CloseServiceHandle) close = &CloseServiceHandle;
};
WindowsServiceOpenResult open_windows_service_impl(const std::string&, WindowsServiceViewApi = {});
}
#endif
