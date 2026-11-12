#include "linux_gpu_vulkan.hpp"
#ifdef __linux__
#include "linux_nvml.hpp"
#include <dlfcn.h>
#include <cstring>
#include <map>
#include <set>
#include <algorithm>
namespace pcinsight::detail {
namespace {
void clear(std::vector<GpuInfo>& gpus,const char* reason){for(auto& gpu:gpus)gpu.vulkan_version={{},Support::unsupported,reason};}
}
#if PCINSIGHT_HAS_LINUX_VULKAN_HEADERS
namespace {
struct Instance {VkInstance handle{};PFN_vkDestroyInstance destroy;~Instance(){if(handle)destroy(handle,nullptr);}};
template<class T> T symbol(void* module,const char* name){T out{};auto* proc=dlsym(module,name);static_assert(sizeof(out)==sizeof(proc));std::memcpy(&out,&proc,sizeof(out));return out;}
// Both count and fill are bounded; incomplete arrays are never published.
template<class T,class Query> bool enumerate(std::vector<T>& out,unsigned limit,Query query){
    for(unsigned retry=0;retry<3;++retry){
        std::uint32_t count{};auto status=query(&count,static_cast<T*>(nullptr));
        if((status!=VK_SUCCESS && status!=VK_INCOMPLETE) || count>limit)return false;
        if(!count){if(status==VK_SUCCESS){out.clear();return true;}continue;}
        out.assign(count,T{});auto returned=count;status=query(&returned,out.data());
        if(returned>count)return false;
        if(status==VK_INCOMPLETE)continue;
        if(status!=VK_SUCCESS)return false;
        out.resize(returned);return true;
    }
    return false;
}
}
void fill_linux_gpu_vulkan(std::vector<GpuInfo>& gpus,LinuxVulkanApi api){
    clear(gpus,"Vulkan physical-device API version unavailable; unique PCI correspondence not proven");
    if(gpus.empty() || !api.version || !api.create || !api.destroy || !api.enumerate || !api.extensions || !api.properties)return;
    std::map<std::string,unsigned> target_counts;
    for(const auto& gpu:gpus)if(gpu.physical_location.value && gpu.physical_location.support==Support::available){
        const auto key=nvml_pci_identity(*gpu.physical_location.value);if(!key.empty())++target_counts[key];
    }
    if(target_counts.empty())return;
    std::uint32_t version{};
    if(api.version(&version)!=VK_SUCCESS || VK_API_VERSION_VARIANT(version)!=0 || version<VK_API_VERSION_1_1)return;
    VkApplicationInfo app{};app.sType=VK_STRUCTURE_TYPE_APPLICATION_INFO;app.pApplicationName="PCInsight telemetry";app.apiVersion=VK_API_VERSION_1_1;
    VkInstanceCreateInfo info{};info.sType=VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;info.pApplicationInfo=&app;
    Instance instance{{},api.destroy};
    if(api.create(&info,nullptr,&instance.handle)!=VK_SUCCESS || !instance.handle)return;
    std::vector<VkPhysicalDevice> devices;
    if(!enumerate(devices,256,[&](auto* count,auto* out){return api.enumerate(instance.handle,count,out);}))return;
    struct Match {std::uint32_t version;bool ambiguous;};
    std::map<std::string,Match> matches;std::set<VkPhysicalDevice> seen;
    for(const auto device:devices){
        if(!device || !seen.insert(device).second)return;
        std::vector<VkExtensionProperties> extensions;
        if(!enumerate(extensions,4096,[&](auto* count,auto* out){return api.extensions(device,nullptr,count,out);}))return;
        bool pci_supported=false;
        for(const auto& extension:extensions){
            const auto end=std::find(std::begin(extension.extensionName),std::end(extension.extensionName),'\0');
            if(end==std::end(extension.extensionName))return;
            if(std::strcmp(extension.extensionName,VK_EXT_PCI_BUS_INFO_EXTENSION_NAME)==0)pci_supported=true;
        }
        if(!pci_supported)continue;
        VkPhysicalDevicePCIBusInfoPropertiesEXT pci{};pci.sType=VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PCI_BUS_INFO_PROPERTIES_EXT;
        VkPhysicalDeviceProperties2 props{};props.sType=VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;props.pNext=&pci;
        api.properties(device,&props);
        if(pci.pciBus>255 || pci.pciDevice>31 || pci.pciFunction>7)return;
        if(props.properties.deviceType==VK_PHYSICAL_DEVICE_TYPE_CPU)continue;
        const auto key=std::to_string(pci.pciDomain)+":"+std::to_string(pci.pciBus)+":"+std::to_string(pci.pciDevice)+"."+std::to_string(pci.pciFunction);
        auto [it,inserted]=matches.emplace(key,Match{props.properties.apiVersion,false});if(!inserted)it->second.ambiguous=true;
    }
    for(auto& gpu:gpus){
        if(!gpu.physical_location.value || gpu.physical_location.support!=Support::available)continue;
        const auto key=nvml_pci_identity(*gpu.physical_location.value);const auto it=matches.find(key);
        if(key.empty() || target_counts[key]!=1 || it==matches.end() || it->second.ambiguous)continue;
        const auto v=it->second.version;if(VK_API_VERSION_VARIANT(v)!=0 || VK_API_VERSION_MAJOR(v)!=1)continue;
        gpu.vulkan_version={std::to_string(VK_API_VERSION_MAJOR(v))+'.'+std::to_string(VK_API_VERSION_MINOR(v))+'.'+std::to_string(VK_API_VERSION_PATCH(v)),Support::available,"Vulkan physical-device apiVersion matched by unique PCI domain/bus/device/function; not loader version"};
    }
}
#endif
void fill_linux_gpu_vulkan(std::vector<GpuInfo>& gpus){
#if PCINSIGHT_HAS_LINUX_VULKAN_HEADERS
    clear(gpus,"Vulkan loader/required 1.1 entrypoints unavailable");if(gpus.empty())return;
    struct Module {void* handle;~Module(){if(handle)dlclose(handle);}} module{dlopen("libvulkan.so.1",RTLD_NOW|RTLD_LOCAL)};
    if(!module.handle)return;
    LinuxVulkanApi api{symbol<PFN_vkEnumerateInstanceVersion>(module.handle,"vkEnumerateInstanceVersion"),symbol<PFN_vkCreateInstance>(module.handle,"vkCreateInstance"),
        symbol<PFN_vkDestroyInstance>(module.handle,"vkDestroyInstance"),symbol<PFN_vkEnumeratePhysicalDevices>(module.handle,"vkEnumeratePhysicalDevices"),
        symbol<PFN_vkEnumerateDeviceExtensionProperties>(module.handle,"vkEnumerateDeviceExtensionProperties"),symbol<PFN_vkGetPhysicalDeviceProperties2>(module.handle,"vkGetPhysicalDeviceProperties2")};
    fill_linux_gpu_vulkan(gpus,api);
#else
    clear(gpus,"Linux Vulkan provider not built: optional Vulkan PCI query headers unavailable at build time");
#endif
}
}
#endif
