#pragma once
#ifdef _WIN32
#include "windows_gpus.hpp"
#include <setupapi.h>
namespace pcinsight::detail {
// Documented d3dkmthk ABI subset; D3DKMT_HANDLE is a 32-bit UINT, not HANDLE.
struct GpuOpenDevice {PCWSTR name;UINT handle;LUID luid;};
struct GpuCloseDevice {UINT handle;};
struct GpuDeviceApi {
    decltype(&SetupDiGetClassDevsW) list=&SetupDiGetClassDevsW;
    decltype(&SetupDiEnumDeviceInterfaces) enumerate=&SetupDiEnumDeviceInterfaces;
    decltype(&SetupDiGetDeviceInterfaceDetailW) detail=&SetupDiGetDeviceInterfaceDetailW;
    decltype(&SetupDiGetDevicePropertyW) property=&SetupDiGetDevicePropertyW;
    decltype(&SetupDiDestroyDeviceInfoList) destroy=&SetupDiDestroyDeviceInfoList;
    LONG (APIENTRY *open)(GpuOpenDevice*){};
    LONG (APIENTRY *close)(const GpuCloseDevice*){};
};
void fill_windows_gpu_devices(std::vector<GpuInfo>&,GpuDeviceApi);
void fill_windows_gpu_devices(std::vector<GpuInfo>&);
}
#endif
