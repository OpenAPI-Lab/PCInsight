#include "windows_gpu_opencl.hpp"
#include "windows_opencl_interop.hpp"
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <cstring>
namespace pcinsight::detail {
void fill_windows_gpu_opencl(std::vector<GpuInfo>& gpus,WindowsOpenClApi api){fill_opencl_gpus(gpus,api,OpenClIdentity::windows_luid);}
void fill_windows_gpu_opencl(std::vector<GpuInfo>& gpus){
    if(gpus.empty())return;
    struct Module {HMODULE handle;~Module(){if(handle)FreeLibrary(handle);}} module{LoadLibraryExW(L"OpenCL.dll",nullptr,LOAD_LIBRARY_SEARCH_SYSTEM32)};
    if(!module.handle){
        const auto error=GetLastError();fill_windows_gpu_opencl(gpus,{});
        for(auto& gpu:gpus){gpu.opencl_version.note="System OpenCL loader unavailable";if(error==ERROR_ACCESS_DENIED)gpu.opencl_version.support=Support::permission_required;}
        return;
    }
    WindowsOpenClApi api;
    auto symbol=[&](auto& function,const char* name){const auto proc=GetProcAddress(module.handle,name);static_assert(sizeof(function)==sizeof(proc));std::memcpy(&function,&proc,sizeof(proc));};
    symbol(api.platforms,"clGetPlatformIDs");symbol(api.devices,"clGetDeviceIDs");symbol(api.info,"clGetDeviceInfo");
    fill_windows_gpu_opencl(gpus,api);
    OpenClExtensionAddress resolve{};symbol(resolve,"clGetExtensionFunctionAddressForPlatform");
    fill_windows_opencl_interop(gpus,api,resolve);
}
}
#endif
