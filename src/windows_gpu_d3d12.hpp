#pragma once
#ifdef _WIN32
#include "windows_gpus.hpp"
#include <d3d12.h>
namespace pcinsight::detail {
// Validation-only CreateDevice: ppDevice stays null, success is S_FALSE.
Metric<std::string> probe_windows_gpu_d3d12(void* adapter,PFN_D3D12_CREATE_DEVICE);
Metric<std::string> probe_windows_gpu_d3d12(void* adapter);
}
#endif
