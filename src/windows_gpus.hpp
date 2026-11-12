#pragma once
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <dxgi.h>
#include "pcinsight/types.hpp"
namespace pcinsight::detail {
// Opaque COM handles with injectable operations; each non-null handle returned
// by create/enumerate owns one reference, including failure paths.
struct GpuInventoryApi {
    HRESULT (*create)(void**);
    HRESULT (*enumerate)(void*,UINT,void**);
    HRESULT (*describe)(void*,DXGI_ADAPTER_DESC1*);
    HRESULT (*driver_version)(void*,LARGE_INTEGER*);
    void (*release_factory)(void*);
    void (*release_adapter)(void*);
    Metric<std::string> (*d3d12_level)(void*){};
    void (*process_memory)(void*,GpuInfo&){};
};
GpuInventoryApi native_gpu_inventory_api();
// DXGI non-software adapters only. Errors throw; no empty-on-failure fallback.
// Creates/releases factory per sample so hotplug is not hidden behind a cache.
std::vector<GpuInfo> read_windows_gpus(GpuInventoryApi api=native_gpu_inventory_api());
}
#endif
