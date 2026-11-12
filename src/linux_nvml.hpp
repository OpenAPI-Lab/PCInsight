#pragma once
#include "pcinsight/types.hpp"
#include <functional>
#include <memory>
#include <string_view>

namespace pcinsight::detail {
// Stable NVML v1 memory/utilization and v3 PCI ABI, without SDK dependency.
// Definitions follow NVIDIA's published nvml.h; never mix versioned layouts.
struct NvmlMemory { unsigned long long total, free, used; };
struct NvmlUtilization { unsigned gpu, memory; };
struct NvmlPci {
    char legacy[16];
    unsigned domain, bus, device, device_id, subsystem_id;
    char bus_id[32];
};
using NvmlDevice = void*;
struct NvmlApi {
    std::function<int()> init, shutdown;
    std::function<int(unsigned*)> count;
    std::function<int(unsigned, NvmlDevice*)> handle;
    std::function<int(NvmlDevice, char*, unsigned)> name, uuid;
    std::function<int(char*, unsigned)> driver;
    std::function<int(NvmlDevice, NvmlPci*)> pci;
    std::function<int(NvmlDevice, NvmlMemory*)> memory;
    std::function<int(NvmlDevice, NvmlUtilization*)> utilization;
    std::function<int(NvmlDevice, unsigned, unsigned*)> temperature;
    // Keeps the library alive until after shutdown; injected tests need no owner.
    std::shared_ptr<void> library;
};
NvmlApi load_linux_nvml();
struct NvmlSnapshot {
    std::vector<GpuInfo> gpus;
    Support support{Support::unsupported};
    std::string note;
};
NvmlSnapshot read_linux_nvml(const NvmlApi& api);
std::string nvml_pci_identity(std::string_view value);
// Merge only unambiguous exact PCI identities. Never match by model or index.
void merge_linux_nvml(std::vector<GpuInfo>& drm, const NvmlSnapshot& nvml);
}
