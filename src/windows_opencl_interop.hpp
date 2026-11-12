#pragma once
#ifdef _WIN32
#include "windows_gpu_opencl.hpp"
#include "windows_gpus.hpp"
#include <map>
namespace pcinsight::detail {
using OpenClD3d11Devices=ClInt (PCINSIGHT_CL_CALL *)(ClPlatform,ClUint,void*,ClUint,ClUint,ClDevice*,ClUint*);
using OpenClExtensionAddress=void* (PCINSIGHT_CL_CALL *)(ClPlatform,const char*);
std::map<std::string,ClDevice> windows_opencl_interop_devices(const std::vector<GpuInfo>&,WindowsOpenClApi,OpenClExtensionAddress,GpuInventoryApi);
void fill_windows_opencl_interop(std::vector<GpuInfo>&,WindowsOpenClApi,OpenClExtensionAddress,GpuInventoryApi=native_gpu_inventory_api());
}
#endif
