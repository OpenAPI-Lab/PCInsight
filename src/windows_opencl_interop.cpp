#include "windows_opencl_interop.hpp"
#ifdef _WIN32
#include <map>
#include <set>
#include <cstdio>
#include <cstring>
#include <algorithm>
namespace pcinsight::detail {
namespace {
struct Reference {void* handle{};void (*release)(void*);~Reference(){if(handle)release(handle);}Reference(void (*fn)(void*)):release(fn){};Reference(const Reference&)=delete;};
std::string identity(LUID luid){char out[40];std::snprintf(out,sizeof(out),"dxgi-luid:%08x:%08x",static_cast<unsigned>(luid.HighPart),static_cast<unsigned>(luid.LowPart));return out;}
template<class T> bool scalar(WindowsOpenClApi api,ClDevice device,ClUint key,T& out){std::size_t size{};return api.info(device,key,sizeof(out),&out,&size)==0 && size==sizeof(out);}
bool associated(OpenClD3d11Devices fn,ClPlatform platform,void* adapter,std::vector<ClDevice>& out){
    for(unsigned retry=0;retry<3;++retry){
        ClUint count{};auto status=fn(platform,0x401a,adapter,0x401c,0,nullptr,&count);
        if(status==-1){out.clear();return true;}if(status!=0 || count>256)return false;
        if(!count){out.clear();return true;}
        out.assign(count,nullptr);ClUint returned{};status=fn(platform,0x401a,adapter,0x401c,count,out.data(),&returned);
        if(status==-1){out.clear();return true;}if(status!=0 || returned>256)return false;
        if(returned>count)continue;
        out.resize(returned);return true;
    }
    return false;
}
}
std::map<std::string,ClDevice> windows_opencl_interop_devices(const std::vector<GpuInfo>& gpus,WindowsOpenClApi api,OpenClExtensionAddress resolve,GpuInventoryApi dxgi){
    if(gpus.empty() || !resolve || !api.platforms || !api.devices || !api.info || !dxgi.create || !dxgi.enumerate || !dxgi.describe || !dxgi.release_factory || !dxgi.release_adapter)return {};
    struct Platform {ClPlatform id;OpenClD3d11Devices query;std::set<ClDevice> allowed;};
    std::vector<Platform> sources;std::vector<ClPlatform> platforms;
    if(!enumerate_opencl_platforms(api,platforms))return {};
    std::set<ClPlatform> platform_ids;std::set<ClDevice> root_ids;
    for(const auto platform:platforms){
        if(!platform || !platform_ids.insert(platform).second)return {};
        std::vector<ClDevice> devices;if(!enumerate_opencl_gpus(api,platform,devices))return {};
        Platform source{platform,nullptr,{}};
        for(const auto device:devices){
            if(!device || !root_ids.insert(device).second || root_ids.size()>4096)return {};
            ClPlatform owner{};ClType type{};
            if(!scalar(api,device,cl_platform,owner) || owner!=platform || !scalar(api,device,cl_type,type))return {};
            if(!(type&cl_gpu))continue;
            auto extensions=read_opencl_text(api,device,cl_extensions,65536);if(!extensions)return {};
            if(has_opencl_extension(*extensions,"cl_khr_d3d11_sharing"))source.allowed.insert(device);
        }
        if(source.allowed.empty())continue;
        auto* raw=resolve(platform,"clGetDeviceIDsFromD3D11KHR");static_assert(sizeof(raw)==sizeof(source.query));std::memcpy(&source.query,&raw,sizeof(raw));
        if(!source.query)return {}; // cannot exclude a hidden second association
        sources.push_back(std::move(source));
    }
    if(sources.empty())return {};
    std::set<std::string> targets;
    for(const auto& gpu:gpus)if(!targets.insert(gpu.id).second)return {};
    Reference factory(dxgi.release_factory);if(dxgi.create(&factory.handle)!=S_OK || !factory.handle)return {};
    std::set<std::string> seen,hardware;std::map<std::string,std::set<ClDevice>> by_adapter;
    std::map<ClDevice,std::set<std::string>> by_device;bool complete=false;
    for(UINT index=0;index<=256;++index){
        Reference adapter(dxgi.release_adapter);const auto status=dxgi.enumerate(factory.handle,index,&adapter.handle);
        if(status==DXGI_ERROR_NOT_FOUND){complete=true;break;}
        if(index==256 || status!=S_OK || !adapter.handle)return {};
        DXGI_ADAPTER_DESC1 desc{};if(dxgi.describe(adapter.handle,&desc)!=S_OK)return {};
        const auto id=identity(desc.AdapterLuid);if(!seen.insert(id).second)return {};
        if(desc.Flags&DXGI_ADAPTER_FLAG_SOFTWARE)continue;
        hardware.insert(id);
        for(const auto& source:sources){
            std::vector<ClDevice> matches;if(!associated(source.query,source.id,adapter.handle,matches))return {};
            std::set<ClDevice> unique;
            for(const auto device:matches){
                if(!device || !unique.insert(device).second || !source.allowed.contains(device))return {};
                by_adapter[id].insert(device);by_device[device].insert(id);
            }
        }
        DXGI_ADAPTER_DESC1 checked{};if(dxgi.describe(adapter.handle,&checked)!=S_OK || identity(checked.AdapterLuid)!=id || checked.Flags!=desc.Flags)return {};
    }
    if(!complete || hardware!=targets)return {}; // topology changed since inventory
    std::map<std::string,ClDevice> result;
    for(const auto& [id,devices]:by_adapter){
        if(devices.size()!=1)continue;
        const auto device=*devices.begin();if(by_device[device].size()==1)result.emplace(id,device);
    }
    return result;
}
void fill_windows_opencl_interop(std::vector<GpuInfo>& gpus,WindowsOpenClApi api,OpenClExtensionAddress resolve,GpuInventoryApi dxgi){
    // Independent fallback: never replace an existing value, even on failures.
    if(gpus.empty() || std::all_of(gpus.begin(),gpus.end(),[](const auto& gpu){return gpu.opencl_version.value.has_value();}))return;
    const auto matches=windows_opencl_interop_devices(gpus,api,resolve,dxgi);
    std::map<std::string,Metric<std::string>> staged;
    for(const auto& [id,device]:matches){
        auto metric=read_opencl_device_version(api,device);if(!metric.value)continue;
        metric.support=Support::estimated;
        metric.note="CL_DEVICE_VERSION of unique D3D11-interoperable OpenCL GPU for this DXGI adapter; one-to-one observed association, NOT physical LUID identity";
        staged.emplace(id,std::move(metric));
    }
    for(auto& gpu:gpus)if(!gpu.opencl_version.value){const auto found=staged.find(gpu.id);if(found!=staged.end())gpu.opencl_version=found->second;}
}
}
#endif
