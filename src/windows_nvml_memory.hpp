#pragma once
#ifdef _WIN32
#include "windows_gpus.hpp"
#include "linux_nvml.hpp"
#include "windows_gpu_temperature.hpp"
namespace pcinsight::detail {
// Narrow published C ABIs, no CUDA/NVML SDK headers or import libraries.
struct WindowsCudaUuid {unsigned char bytes[16];};
struct WindowsNvmlMemoryApi {
    int (WINAPI *cuda_init)(unsigned){};
    int (WINAPI *count)(int*){};
    int (WINAPI *device)(int*,int){};
    int (WINAPI *luid)(char*,unsigned*,int){};
    int (WINAPI *uuid)(WindowsCudaUuid*,int){}; // cuDeviceGetUuid_v2 only
    int (__cdecl *init)(){};
    int (__cdecl *shutdown)(){};
    int (__cdecl *by_uuid)(const char*,NvmlDevice*){};
    int (__cdecl *nvml_uuid)(NvmlDevice,char*,unsigned){};
    int (__cdecl *memory)(NvmlDevice,NvmlMemory*){}; // v1 only
    GpuTemperatureApi topology; // KMT exact-LUID open/count/close, no perfdata
};
void fill_windows_nvml_memory(std::vector<GpuInfo>&,WindowsNvmlMemoryApi);
void fill_windows_nvml_memory(std::vector<GpuInfo>&);
}
#endif
