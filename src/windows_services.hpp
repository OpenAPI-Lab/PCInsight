#pragma once
#ifdef _WIN32
#include "pcinsight/types.hpp"
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <winsvc.h>
namespace pcinsight::detail {
struct ServicesApi {
    decltype(&OpenSCManagerW) manager=&OpenSCManagerW;
    decltype(&EnumServicesStatusExW) enumerate=&EnumServicesStatusExW;
    decltype(&OpenServiceW) service=&OpenServiceW;
    decltype(&QueryServiceConfigW) config=&QueryServiceConfigW;
    decltype(&QueryServiceConfig2W) config2=&QueryServiceConfig2W;
    decltype(&CloseServiceHandle) close=&CloseServiceHandle;
};
std::string windows_service_state(DWORD state);
std::vector<ServiceInfo> read_windows_services(ServicesApi api = {});
}
#endif
