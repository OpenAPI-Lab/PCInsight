#include "windows_gpu_vulkan.hpp"
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <cstring>
#include <cstdio>
#include <map>
#include <set>
namespace pcinsight::detail {
namespace {
void clear(std::vector<GpuInfo>& gpus,const char* reason){for(auto& gpu:gpus)gpu.vulkan_version={{},Support::unsupported,reason};}
}
#if PCINSIGHT_HAS_VULKAN_HEADERS
namespace {
std::string identity(const std::uint8_t* bytes){
    LUID luid{};static_assert(sizeof(luid)==VK_LUID_SIZE);std::memcpy(&luid,bytes,sizeof(luid));
    char text[40];std::snprintf(text,sizeof(text),"dxgi-luid:%08x:%08x",static_cast<unsigned>(luid.HighPart),static_cast<unsigned>(luid.LowPart));return text;
}
struct Instance {VkInstance handle{};PFN_vkDestroyInstance destroy;~Instance(){if(handle)destroy(handle,nullptr);}};
template<class T> T symbol(HMODULE module,const char* name){T out{};const auto proc=GetProcAddress(module,name);static_assert(sizeof(out)==sizeof(proc));std::memcpy(&out,&proc,sizeof(out));return out;}
}
void fill_windows_gpu_vulkan(std::vector<GpuInfo>& gpus,GpuVulkanApi api){
    clear(gpus,"Vulkan physical-device API version unavailable; no proven unique DXGI LUID match");
    if(gpus.empty() || !api.version || !api.create || !api.destroy || !api.enumerate || !api.properties)return;
    std::uint32_t version{};
    if(api.version(&version)!=VK_SUCCESS || VK_API_VERSION_VARIANT(version)!=0 || version<VK_API_VERSION_1_1)return;
    VkApplicationInfo app{};app.sType=VK_STRUCTURE_TYPE_APPLICATION_INFO;app.pApplicationName="PCInsight telemetry";app.apiVersion=VK_API_VERSION_1_1;
    VkInstanceCreateInfo info{};info.sType=VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;info.pApplicationInfo=&app;
    Instance instance{{},api.destroy};
    if(api.create(&info,nullptr,&instance.handle)!=VK_SUCCESS || !instance.handle)return;
    std::vector<VkPhysicalDevice> devices;bool complete=false;
    for(unsigned retry=0;retry<3;++retry){
        std::uint32_t count{};const auto counted=api.enumerate(instance.handle,&count,nullptr);
        if((counted!=VK_SUCCESS && counted!=VK_INCOMPLETE) || count>256)return;
        if(!count){if(counted==VK_SUCCESS){devices.clear();complete=true;break;}continue;}
        devices.assign(count,VK_NULL_HANDLE);auto returned=count;
        const auto result=api.enumerate(instance.handle,&returned,devices.data());
        if(returned>count)return;
        if(result==VK_INCOMPLETE)continue;
        if(result!=VK_SUCCESS)return;
        devices.resize(returned);complete=true;break;
    }
    if(!complete)return;
    struct Match {std::uint32_t version;bool ambiguous;};
    std::map<std::string,Match> matches;std::set<VkPhysicalDevice> seen;
    std::map<std::string,unsigned> targets;
    for(const auto& gpu:gpus)++targets[gpu.id];
    for(const auto device:devices){
        if(!device || !seen.insert(device).second)return;
        VkPhysicalDeviceIDProperties ids{};ids.sType=VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ID_PROPERTIES;
        VkPhysicalDeviceProperties2 props{};props.sType=VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;props.pNext=&ids;
        api.properties(device,&props);
        if(ids.deviceLUIDValid==VK_FALSE)continue;
        if(ids.deviceLUIDValid!=VK_TRUE || !ids.deviceNodeMask || (ids.deviceNodeMask&(ids.deviceNodeMask-1)))return;
        if(props.properties.deviceType==VK_PHYSICAL_DEVICE_TYPE_CPU)continue;
        auto [it,inserted]=matches.emplace(identity(ids.deviceLUID),Match{props.properties.apiVersion,false});
        if(!inserted)it->second.ambiguous=true; // linked nodes/ICDs: never choose first or max
    }
    for(auto& gpu:gpus){
        if(gpu.id.empty() || targets[gpu.id]!=1)continue;
        const auto it=matches.find(gpu.id);if(it==matches.end() || it->second.ambiguous)continue;
        const auto v=it->second.version;
        if(VK_API_VERSION_VARIANT(v)!=0 || VK_API_VERSION_MAJOR(v)!=1)continue;
        gpu.vulkan_version={std::to_string(VK_API_VERSION_MAJOR(v))+'.'+std::to_string(VK_API_VERSION_MINOR(v))+'.'+std::to_string(VK_API_VERSION_PATCH(v)),Support::available,"Vulkan physical-device apiVersion matched by valid Windows LUID; not loader version or enabled application API version"};
    }
}
#endif
void fill_windows_gpu_vulkan(std::vector<GpuInfo>& gpus){
#if PCINSIGHT_HAS_VULKAN_HEADERS
    clear(gpus,"System Vulkan loader/required 1.1 entrypoints unavailable");
    if(gpus.empty())return;
    struct Module {HMODULE handle;~Module(){if(handle)FreeLibrary(handle);}} module{LoadLibraryExW(L"vulkan-1.dll",nullptr,LOAD_LIBRARY_SEARCH_SYSTEM32)};
    if(!module.handle){
        if(GetLastError()==ERROR_ACCESS_DENIED)for(auto& gpu:gpus)gpu.vulkan_version.support=Support::permission_required;
        return;
    }
    GpuVulkanApi api{symbol<PFN_vkEnumerateInstanceVersion>(module.handle,"vkEnumerateInstanceVersion"),
        symbol<PFN_vkCreateInstance>(module.handle,"vkCreateInstance"),symbol<PFN_vkDestroyInstance>(module.handle,"vkDestroyInstance"),
        symbol<PFN_vkEnumeratePhysicalDevices>(module.handle,"vkEnumeratePhysicalDevices"),symbol<PFN_vkGetPhysicalDeviceProperties2>(module.handle,"vkGetPhysicalDeviceProperties2")};
    fill_windows_gpu_vulkan(gpus,api);
#else
    clear(gpus,"Windows Vulkan provider not built: optional Vulkan 1.1 headers unavailable at build time");
#endif
}
}
#endif
