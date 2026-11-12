#pragma once
#include "pcinsight/windows_amd_runtime_memory.hpp"
#ifdef _WIN32
#include "windows_opencl_interop.hpp"
namespace pcinsight::detail {
WindowsAmdRuntimeMemorySnapshot read_windows_amd_runtime_memory(const std::vector<GpuInfo>&,WindowsOpenClApi,OpenClExtensionAddress,GpuInventoryApi);
}
#endif
