#pragma once
#include "linux_gpu_opencl.hpp"
#ifdef _WIN32
namespace pcinsight::detail {
using WindowsOpenClApi=LinuxOpenClApi;
void fill_windows_gpu_opencl(std::vector<GpuInfo>&,WindowsOpenClApi);
void fill_windows_gpu_opencl(std::vector<GpuInfo>&);
}
#endif
