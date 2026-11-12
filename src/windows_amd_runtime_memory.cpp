#include "windows_amd_runtime_memory.hpp"
#ifdef _WIN32
#include <array>
#include <cstring>
#include <limits>
namespace pcinsight::detail {
namespace {
constexpr ClUint vendor_key=0x1001,driver_key=0x102d,free_key=0x4039;
bool amd(const GpuInfo& gpu){return gpu.hardware_vendor_id.support==Support::available && gpu.hardware_vendor_id.value==0x1002U;}
bool vendor(WindowsOpenClApi api,ClDevice device){ClUint id{};std::size_t returned{};return api.info && api.info(device,vendor_key,sizeof(id),&id,&returned)==0 && returned==sizeof(id) && id==0x1002U;}
bool extension(WindowsOpenClApi api,ClDevice device){const auto list=read_opencl_text(api,device,cl_extensions,65536);return list && has_opencl_extension(*list,"cl_amd_device_attribute_query");}
}
WindowsAmdRuntimeMemorySnapshot read_windows_amd_runtime_memory(const std::vector<GpuInfo>& gpus,WindowsOpenClApi api,OpenClExtensionAddress resolve,GpuInventoryApi dxgi){
    WindowsAmdRuntimeMemorySnapshot out;out.note="Experimental AMD runtime estimates unavailable; unique D3D11/OpenCL association/extension required";
    for(const auto& gpu:gpus)if(amd(gpu)){
        WindowsAmdRuntimeMemory row;row.adapter_id=gpu.id;
        row.reported_free_bytes={{},Support::unsupported,out.note};row.reported_largest_block_bytes=row.reported_free_bytes;
        row.opencl_driver_version={{},Support::unsupported,"CL_DRIVER_VERSION unavailable"};out.gpus.push_back(std::move(row));
    }
    if(out.gpus.empty()){out.support=Support::available;out.note="No AMD adapter in supplied DXGI inventory";return out;}
    if constexpr(sizeof(std::size_t)!=8){out.note="Experimental AMD decoder currently supports Windows64 callers only";return out;}
    const auto matches=windows_opencl_interop_devices(gpus,api,resolve,dxgi);
    for(auto& row:out.gpus){
        const auto found=matches.find(row.adapter_id);if(found==matches.end())continue;const auto device=found->second;
        if(!vendor(api,device) || !extension(api,device))continue;
        const auto driver=read_opencl_text(api,device,driver_key,1024);if(!driver || driver->empty())continue;
        row.opencl_driver_version={*driver,Support::available,"CL_DRIVER_VERSION for observed interoperable AMD OpenCL device; not DXGI UMD version"};
        // AMD's published CLR implementation returns size_t[2], in KiB:
        // total runtime-reported free, largest block. Request both explicitly:
        // its zero-sized size query can instead advertise only one element.
        constexpr auto absent=std::numeric_limits<std::size_t>::max();
        std::array<std::size_t,2> values{absent,absent};std::size_t returned{};
        const auto status=api.info(device,free_key,sizeof(values),values.data(),&returned);
        auto reject=[&](const char* note){row.reported_free_bytes={{},Support::unsupported,note};row.reported_largest_block_bytes=row.reported_free_bytes;};
        if(status!=0 || returned!=sizeof(values) || values[0]==absent || values[1]==absent || values[1]>values[0] || values[0]>std::numeric_limits<std::uint64_t>::max()/1024){reject("AMD runtime free query failed/absent/malformed/overflow; no scalar or capacity-minus-usage fallback");continue;}
        const auto checked_driver=read_opencl_text(api,device,driver_key,1024);
        if(!vendor(api,device) || !extension(api,device) || checked_driver!=driver){reject("AMD OpenCL metadata changed during runtime free query");row.opencl_driver_version={{},Support::unsupported,"AMD OpenCL metadata changed"};continue;}
        row.reported_free_bytes={static_cast<std::uint64_t>(values[0])*1024,Support::estimated,"EXPERIMENTAL AMD runtime-reported free estimate; may depend on process/budget/cache/hidden reserve; NOT global free or guaranteed allocatable"};
        row.reported_largest_block_bytes={static_cast<std::uint64_t>(values[1])*1024,Support::estimated,"EXPERIMENTAL AMD runtime-reported largest block estimate; not a successful allocation test"};
    }
    const auto checked=windows_opencl_interop_devices(gpus,api,resolve,dxgi);
    bool any=false;
    for(auto& row:out.gpus){
        const auto a=matches.find(row.adapter_id),b=checked.find(row.adapter_id);
        if(a==matches.end() || b==checked.end() || a->second!=b->second){row.reported_free_bytes={{},Support::unsupported,"D3D11/OpenCL association absent/changed during experimental query"};row.reported_largest_block_bytes=row.reported_free_bytes;row.opencl_driver_version={{},Support::unsupported,"Association not stable"};}
        if(row.reported_free_bytes.value)any=true;
    }
    if(any){out.support=Support::estimated;out.note="Explicit experimental AMD runtime estimates, observed one-to-one D3D11/OpenCL association; NOT physical LUID identity or global free memory";}
    return out;
}
}
#endif
namespace pcinsight {
WindowsAmdRuntimeMemorySnapshot query_experimental_windows_amd_runtime_memory(){
#ifdef _WIN32
    using namespace detail;
    const auto gpus=read_windows_gpus();WindowsOpenClApi api;
    struct Module {HMODULE value;~Module(){if(value)FreeLibrary(value);}} module{LoadLibraryExW(L"OpenCL.dll",nullptr,LOAD_LIBRARY_SEARCH_SYSTEM32)};
    const auto error=module.value?ERROR_SUCCESS:GetLastError();OpenClExtensionAddress resolve{};
    if(module.value){auto symbol=[&](auto& fn,const char* name){const auto raw=GetProcAddress(module.value,name);static_assert(sizeof(fn)==sizeof(raw));std::memcpy(&fn,&raw,sizeof(raw));};symbol(api.platforms,"clGetPlatformIDs");symbol(api.devices,"clGetDeviceIDs");symbol(api.info,"clGetDeviceInfo");symbol(resolve,"clGetExtensionFunctionAddressForPlatform");}
    auto out=read_windows_amd_runtime_memory(gpus,api,resolve,native_gpu_inventory_api());
    if(!module.value && !out.gpus.empty()){
        out.support=error==ERROR_ACCESS_DENIED?Support::permission_required:Support::unsupported;out.note="Experimental AMD query: System32 OpenCL loader unavailable";
        for(auto& row:out.gpus){row.reported_free_bytes={{},out.support,out.note};row.reported_largest_block_bytes=row.reported_free_bytes;row.opencl_driver_version={{},out.support,out.note};}
    }
    return out;
#else
    return {Support::unsupported,"Experimental AMD runtime memory query is Windows-only",{}};
#endif
}
}
