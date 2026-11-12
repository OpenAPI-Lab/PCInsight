#include "windows_gpu_budget.hpp"
#ifdef _WIN32
namespace pcinsight::detail {
namespace {
HRESULT open(void* raw,void** out){return static_cast<IDXGIAdapter1*>(raw)->QueryInterface(__uuidof(IDXGIAdapter3),out);}
HRESULT query(void* raw,UINT node,DXGI_MEMORY_SEGMENT_GROUP group,DXGI_QUERY_VIDEO_MEMORY_INFO* out){return static_cast<IDXGIAdapter3*>(raw)->QueryVideoMemoryInfo(node,group,out);}
void release(void* raw){static_cast<IDXGIAdapter3*>(raw)->Release();}
struct Reference {void* handle{};void (*release)(void*);~Reference(){if(handle)release(handle);}};
void unavailable(GpuProcessMemoryBudget& out,HRESULT status){
    const auto support=status==E_ACCESSDENIED || status==HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED)?Support::permission_required:Support::unsupported;
    const Metric<std::uint64_t> metric{{},support,"DXGI caller-process/node-0 memory budget query unavailable/failed; not GPU-global free memory"};
    out.budget_bytes=out.current_usage_bytes=out.available_for_reservation_bytes=out.current_reservation_bytes=out.budget_headroom_bytes=metric;
}
void read(GpuProcessMemoryBudget& out,void* adapter,DXGI_MEMORY_SEGMENT_GROUP group,GpuBudgetApi api){
    DXGI_QUERY_VIDEO_MEMORY_INFO data{};const auto status=api.query(adapter,0,group,&data);
    if(status!=S_OK){unavailable(out,status);return;}
    out.budget_bytes={data.Budget,Support::available,"OS budget target for caller process/node0/segment group; not total capacity"};
    out.current_usage_bytes={data.CurrentUsage,Support::available,"Caller process memory usage for node0/segment group; not GPU-global usage"};
    out.available_for_reservation_bytes={data.AvailableForReservation,Support::available,"Caller process amount available for reservation; not global free or allocation guarantee"};
    out.current_reservation_bytes={data.CurrentReservation,Support::available,"Caller process reservation hint; this provider does not set reservations"};
    out.budget_headroom_bytes={data.Budget>data.CurrentUsage?data.Budget-data.CurrentUsage:0,Support::available,"max(process budget - process usage,0); arithmetic budget headroom, not free or guaranteed allocatable memory"};
}
}
GpuBudgetApi native_gpu_budget_api(){return {open,query,release};}
void fill_windows_gpu_budget(void* adapter,GpuInfo& gpu,GpuBudgetApi api){
    gpu.local_process_memory={};gpu.non_local_process_memory={};
    for(auto* out:{&gpu.local_process_memory,&gpu.non_local_process_memory}){
        out->process_id={GetCurrentProcessId(),Support::available,"Calling process PID at query; not all processes and not a selected remote PID"};
        out->node_index={0,Support::available,"Requested physical node index 0 only; no aggregation or claim about other linked nodes"};
        unavailable(*out,E_NOINTERFACE);
    }
    if(!adapter || !api.open || !api.query || !api.release)return;
    Reference ref{nullptr,api.release};const auto status=api.open(adapter,&ref.handle);
    if(status!=S_OK || !ref.handle){for(auto* out:{&gpu.local_process_memory,&gpu.non_local_process_memory})unavailable(*out,status==S_OK?E_UNEXPECTED:status);return;}
    read(gpu.local_process_memory,ref.handle,DXGI_MEMORY_SEGMENT_GROUP_LOCAL,api);
    read(gpu.non_local_process_memory,ref.handle,DXGI_MEMORY_SEGMENT_GROUP_NON_LOCAL,api);
}
void fill_windows_gpu_budget(void* adapter,GpuInfo& gpu){fill_windows_gpu_budget(adapter,gpu,native_gpu_budget_api());}
}
#endif
