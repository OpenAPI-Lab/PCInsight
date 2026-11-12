#pragma once
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <setupapi.h>
#include <winioctl.h>
#include "pcinsight/types.hpp"
#include <map>
namespace pcinsight::detail {
struct DiskApi {
    decltype(&SetupDiGetClassDevsW) devices = &SetupDiGetClassDevsW;
    decltype(&SetupDiEnumDeviceInterfaces) enumerate = &SetupDiEnumDeviceInterfaces;
    decltype(&SetupDiGetDeviceInterfaceDetailW) detail = &SetupDiGetDeviceInterfaceDetailW;
    decltype(&SetupDiDestroyDeviceInfoList) destroy = &SetupDiDestroyDeviceInfoList;
    decltype(&CreateFileW) open = &CreateFileW;
    decltype(&CloseHandle) close = &CloseHandle;
    decltype(&DeviceIoControl) query = &DeviceIoControl;
};
struct DiskHardwareIdentity {
    std::string serial;
    std::uint64_t capacity{};
    unsigned bus{};
    bool operator==(const DiskHardwareIdentity&) const = default;
};
std::vector<DiskInfo> read_windows_disks(DiskApi = {}, std::map<std::uint32_t,std::string>* device_numbers = nullptr,
    std::map<std::string,DiskHardwareIdentity>* hardware_identities = nullptr);
}
#endif
