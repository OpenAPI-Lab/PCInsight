#pragma once
#ifdef _WIN32
#include "windows_gpu_devices.hpp"
namespace pcinsight::detail {
// Documented D3DKMT ABI subset; native loader restricted to 64-bit callers.
struct GpuOpenLuid {LUID luid;UINT handle;};
struct GpuAdapterQuery {UINT handle,type;void* data;UINT size;};
struct GpuAdapterPerfData {
    UINT physical_index;
    alignas(8) std::uint64_t memory_frequency,max_memory_frequency,max_memory_frequency_oc,memory_bandwidth,pcie_bandwidth;
    ULONG fan_rpm,power,temperature;
    UCHAR power_state;
};
struct GpuTemperatureApi {
    LONG (APIENTRY *open)(GpuOpenLuid*){};
    LONG (APIENTRY *query)(GpuAdapterQuery*){};
    LONG (APIENTRY *close)(const GpuCloseDevice*){};
};
void fill_windows_gpu_temperature(std::vector<GpuInfo>&,GpuTemperatureApi);
void fill_windows_gpu_temperature(std::vector<GpuInfo>&);
}
#endif
