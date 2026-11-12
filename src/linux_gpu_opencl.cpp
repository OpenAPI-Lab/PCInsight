#include "linux_gpu_opencl.hpp"
#include "linux_nvml.hpp"
#include <algorithm>
#include <charconv>
#include <cstring>
#include <cstdio>
#include <map>
#include <set>
#include <string_view>
#ifdef __linux__
#include <dlfcn.h>
#endif
namespace pcinsight::detail {
static_assert(sizeof(ClPci)==16 && sizeof(ClType)==8 && sizeof(ClInt)==4);
namespace {
Metric<std::string> missing(const char* note){return {{},Support::unsupported,note};}
template<class Query> bool enumerate(std::vector<void*>& values,unsigned limit,ClInt empty,Query query){
    for(unsigned attempt=0;attempt<3;++attempt){
        ClUint count{};auto status=query(0,static_cast<void**>(nullptr),&count);
        if(status==empty){values.clear();return true;}
        if(status!=0 || count>limit)return false;
        if(!count){values.clear();return true;}
        values.assign(count,nullptr);ClUint returned{};status=query(count,values.data(),&returned);
        if(status==empty){values.clear();return true;}
        if(status!=0 || returned>limit)return false;
        if(returned>count)continue;
        values.resize(returned);return true;
    }
    return false;
}
template<class T> bool scalar(LinuxOpenClApi api,ClDevice device,ClUint key,T& out){
    std::size_t size{};return api.info(device,key,sizeof(out),&out,&size)==0 && size==sizeof(out);
}
std::optional<std::string> text(LinuxOpenClApi api,ClDevice device,ClUint key,std::size_t limit){
    std::size_t size{};if(api.info(device,key,0,nullptr,&size)!=0 || !size || size>limit)return {};
    std::string value(size,'\0');std::size_t returned{};
    if(api.info(device,key,size,value.data(),&returned)!=0 || !returned || returned>size || value[returned-1]!='\0')return {};
    value.resize(returned-1);
    if(!std::all_of(value.begin(),value.end(),[](unsigned char c){return c>=32 && c<=126;}))return {};
    return value;
}
bool extension(std::string_view list,std::string_view wanted){
    while(!list.empty()){
        const auto space=list.find(' ');if(list.substr(0,space)==wanted)return true;
        if(space==std::string_view::npos)break;
        list.remove_prefix(space+1);
    }
    return false;
}
bool version(std::string_view value){
    if(!value.starts_with("OpenCL "))return false;
    value.remove_prefix(7);const auto dot=value.find('.');if(dot==std::string_view::npos || !dot)return false;
    ClUint major{},minor{};auto parsed=std::from_chars(value.data(),value.data()+dot,major);
    if(parsed.ec!=std::errc{} || parsed.ptr!=value.data()+dot || !major)return false;
    value.remove_prefix(dot+1);const auto space=value.find(' ');if(space==std::string_view::npos || !space)return false;
    parsed=std::from_chars(value.data(),value.data()+space,minor);return parsed.ec==std::errc{} && parsed.ptr==value.data()+space;
}
}
bool enumerate_opencl_platforms(LinuxOpenClApi api,std::vector<ClPlatform>& out){return api.platforms && enumerate(out,128,-1001,[&](auto n,auto* values,auto* count){return api.platforms(n,values,count);});}
bool enumerate_opencl_gpus(LinuxOpenClApi api,ClPlatform platform,std::vector<ClDevice>& out){return api.devices && enumerate(out,256,-1,[&](auto n,auto* values,auto* count){return api.devices(platform,cl_gpu,n,values,count);});}
std::optional<std::string> read_opencl_text(LinuxOpenClApi api,ClDevice device,ClUint key,std::size_t limit){if(!api.info)return {};return text(api,device,key,limit);}
bool has_opencl_extension(std::string_view list,std::string_view name){return extension(list,name);}
Metric<std::string> read_opencl_device_version(LinuxOpenClApi api,ClDevice device){
    auto result=missing("CL_DEVICE_VERSION query failed or malformed; no platform-version fallback");
    const auto reported=read_opencl_text(api,device,cl_version,1024);
    if(reported && version(*reported))result={*reported,Support::available,"CL_DEVICE_VERSION; device association supplied by caller"};
    return result;
}
void fill_opencl_gpus(std::vector<GpuInfo>& gpus,LinuxOpenClApi api,OpenClIdentity identity){
    const bool luid=identity==OpenClIdentity::windows_luid;
    for(auto& gpu:gpus)gpu.opencl_version=missing(luid?"OpenCL device version unavailable; valid unique DXGI LUID correspondence not proven":"OpenCL device version unavailable; unique PCI correspondence not proven");
    if(gpus.empty() || !api.platforms || !api.devices || !api.info)return;
    std::map<std::string,unsigned> targets;
    auto target_key=[&](const GpuInfo& gpu){
        if(luid)return gpu.id;
        return gpu.physical_location.value && gpu.physical_location.support==Support::available?nvml_pci_identity(*gpu.physical_location.value):std::string{};
    };
    for(const auto& gpu:gpus){auto key=target_key(gpu);if(!key.empty())++targets[key];}
    if(targets.empty())return;
    std::vector<void*> platforms;if(!enumerate(platforms,128,-1001,[&](auto n,auto* out,auto* count){return api.platforms(n,out,count);}))return;
    std::set<void*> seen_platforms,seen_devices;
    struct Match {Metric<std::string> version;bool ambiguous;};std::map<std::string,Match> matches;
    for(auto platform:platforms){
        if(!platform || !seen_platforms.insert(platform).second)return;
        std::vector<void*> devices;if(!enumerate(devices,256,-1,[&](auto n,auto* out,auto* count){return api.devices(platform,cl_gpu,n,out,count);}))return;
        for(auto device:devices){
            if(!device || !seen_devices.insert(device).second || seen_devices.size()>4096)return;
            ClPlatform owner{};ClType type{};
            if(!scalar(api,device,cl_platform,owner) || owner!=platform || !scalar(api,device,cl_type,type))return;
            if(!(type&cl_gpu))continue;
            const auto extensions=text(api,device,cl_extensions,65536);if(!extensions)return;
            std::string key;
            if(luid){
                if(!extension(*extensions,"cl_khr_device_uuid"))continue;
                ClUint valid{};if(!scalar(api,device,cl_luid_valid,valid))return;
                if(valid==0)continue; // LUID/node mask undefined; do not read them
                if(valid!=1)return;
                struct Luid {ClUint low,high;} bits{};static_assert(sizeof(bits)==8);
                ClUint mask{};
                if(!scalar(api,device,cl_luid,bits) || !scalar(api,device,cl_node_mask,mask) || !mask || (mask&(mask-1)))return;
                char text[40];std::snprintf(text,sizeof(text),"dxgi-luid:%08x:%08x",static_cast<unsigned>(bits.high),static_cast<unsigned>(bits.low));key=text;
            }else{
                if(!extension(*extensions,"cl_khr_pci_bus_info"))continue;
                ClPci pci{};if(!scalar(api,device,cl_pci,pci) || pci.bus>255 || pci.device>31 || pci.function>7)return;
                key=std::to_string(pci.domain)+":"+std::to_string(pci.bus)+":"+std::to_string(pci.device)+"."+std::to_string(pci.function);
            }
            auto metric=read_opencl_device_version(api,device);
            if(metric.value)metric.note=luid?"CL_DEVICE_VERSION of GPU matched by valid unique Windows DXGI LUID; not platform, OpenCL C, or optional feature support":"CL_DEVICE_VERSION of GPU matched by unique PCI domain/bus/device/function; not platform, OpenCL C, or optional feature support";
            auto [it,inserted]=matches.emplace(key,Match{metric,false});if(!inserted)it->second.ambiguous=true;
        }
    }
    for(auto& gpu:gpus){
        const auto key=target_key(gpu);const auto it=matches.find(key);
        if(!key.empty() && targets[key]==1 && it!=matches.end() && !it->second.ambiguous)gpu.opencl_version=it->second.version;
    }
}
void fill_linux_gpu_opencl(std::vector<GpuInfo>& gpus,LinuxOpenClApi api){fill_opencl_gpus(gpus,api,OpenClIdentity::linux_pci);}
void fill_linux_gpu_opencl(std::vector<GpuInfo>& gpus){
#ifdef __linux__
    if(gpus.empty())return;
    struct Module {void* handle;~Module(){if(handle)dlclose(handle);}} module{dlopen("libOpenCL.so.1",RTLD_NOW|RTLD_LOCAL)};
    LinuxOpenClApi api;
    if(module.handle){
        auto symbol=[&](auto& function,const char* name){void* raw=dlsym(module.handle,name);static_assert(sizeof(function)==sizeof(raw));std::memcpy(&function,&raw,sizeof(raw));};
        symbol(api.platforms,"clGetPlatformIDs");symbol(api.devices,"clGetDeviceIDs");symbol(api.info,"clGetDeviceInfo");
    }
    fill_linux_gpu_opencl(gpus,api);
#else
    fill_linux_gpu_opencl(gpus,{});
#endif
}
}
