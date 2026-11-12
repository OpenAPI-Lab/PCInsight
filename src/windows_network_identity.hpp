#pragma once
#include "pcinsight/windows_network_identity.hpp"
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <setupapi.h>
namespace pcinsight::detail {
struct NetworkIdentityApi {
    decltype(&GetIfEntry2) row = &GetIfEntry2;
    decltype(&SetupDiGetClassDevsW) list = &SetupDiGetClassDevsW;
    decltype(&SetupDiEnumDeviceInfo) enumerate = &SetupDiEnumDeviceInfo;
    decltype(&SetupDiOpenDevRegKey) key = &SetupDiOpenDevRegKey;
    decltype(&RegQueryValueExW) value = &RegQueryValueExW;
    decltype(&SetupDiGetDeviceRegistryPropertyW) property = &SetupDiGetDeviceRegistryPropertyW;
    decltype(&RegCloseKey) close = &RegCloseKey;
    decltype(&SetupDiDestroyDeviceInfoList) destroy = &SetupDiDestroyDeviceInfoList;
};
Metric<WindowsNetworkIdentity> query_windows_network_identity_impl(const std::string&, NetworkIdentityApi = {});
}
#endif
