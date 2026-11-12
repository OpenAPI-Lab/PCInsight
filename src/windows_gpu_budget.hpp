#pragma once
#ifdef _WIN32
#include "windows_gpus.hpp"
#include <dxgi1_4.h>
namespace pcinsight::detail {
struct GpuBudgetApi {
    HRESULT (*open)(void*,void**);
    HRESULT (*query)(void*,UINT,DXGI_MEMORY_SEGMENT_GROUP,DXGI_QUERY_VIDEO_MEMORY_INFO*);
    void (*release)(void*);
};
GpuBudgetApi native_gpu_budget_api();
void fill_windows_gpu_budget(void* adapter,GpuInfo&,GpuBudgetApi);
void fill_windows_gpu_budget(void* adapter,GpuInfo&);
}
#endif
