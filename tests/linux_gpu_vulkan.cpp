#include "../src/linux_gpu_vulkan.hpp"
#include "pcinsight/monitor.hpp"
#include <cassert>
#include <cstring>
#include <iostream>
using namespace pcinsight;
using namespace pcinsight::detail;
std::vector<GpuInfo> targets(){std::vector<GpuInfo> g(3);g[0].physical_location={std::string("0000:03:00.0"),Support::available,"sysfs fixture"};g[1].physical_location={std::string("00000001:03:00.0"),Support::available,"NVML fixture"};g[2].physical_location={std::string("0000:04:00.0"),Support::available,"unmatched"};return g;}
#if PCINSIGHT_HAS_LINUX_VULKAN_HEADERS
namespace {
int mode,creates,destroys,enumerations,properties,extension_calls;unsigned n;
VkInstance instance=reinterpret_cast<VkInstance>(1);
VKAPI_ATTR VkResult VKAPI_CALL version(std::uint32_t* out){*out=mode==1?VK_API_VERSION_1_0:VK_MAKE_API_VERSION(0,1,4,500);return mode==2?VK_ERROR_INITIALIZATION_FAILED:VK_SUCCESS;}
VKAPI_ATTR VkResult VKAPI_CALL create(const VkInstanceCreateInfo* info,const VkAllocationCallbacks* allocator,VkInstance* out){
    ++creates;assert(!allocator && !info->enabledLayerCount && !info->enabledExtensionCount);assert(info->pApplicationInfo->apiVersion==VK_API_VERSION_1_1);
    *out=mode==3?VK_NULL_HANDLE:instance;return mode==4?VK_ERROR_INITIALIZATION_FAILED:VK_SUCCESS;
}
VKAPI_ATTR void VKAPI_CALL destroy(VkInstance value,const VkAllocationCallbacks* allocator){assert(value==instance && !allocator);++destroys;}
VKAPI_ATTR VkResult VKAPI_CALL devices(VkInstance value,std::uint32_t* count,VkPhysicalDevice* out){
    assert(value==instance);++enumerations;if(mode==5)return VK_ERROR_INITIALIZATION_FAILED;
    if(!out){*count=mode==6?257:n;return VK_SUCCESS;}
    assert(*count==n);for(unsigned i=0;i<n;++i)out[i]=reinterpret_cast<VkPhysicalDevice>(static_cast<std::uintptr_t>(i+10));
    if(mode==7)return VK_INCOMPLETE;if(mode==8 && enumerations==2)return VK_INCOMPLETE;
    if(mode==9)++*count;if(mode==10)out[0]=VK_NULL_HANDLE;if(mode==11 && n>1)out[1]=out[0];return VK_SUCCESS;
}
VKAPI_ATTR VkResult VKAPI_CALL extensions(VkPhysicalDevice,const char* layer,std::uint32_t* count,VkExtensionProperties* out){
    assert(!layer);++extension_calls;if(mode==20)return VK_ERROR_INITIALIZATION_FAILED;
    if(!out){*count=mode==21?4097:1;return VK_SUCCESS;}
    assert(*count==1);std::strcpy(out[0].extensionName,mode==13?"VK_not_pci":VK_EXT_PCI_BUS_INFO_EXTENSION_NAME);out[0].specVersion=2;
    if(mode==22)std::memset(out[0].extensionName,'x',sizeof(out[0].extensionName));
    if(mode==23)return VK_INCOMPLETE;if(mode==24)++*count;return VK_SUCCESS;
}
VKAPI_ATTR void VKAPI_CALL props(VkPhysicalDevice device,VkPhysicalDeviceProperties2* out){
    ++properties;assert(out->sType==VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2);
    auto* pci=static_cast<VkPhysicalDevicePCIBusInfoPropertiesEXT*>(out->pNext);assert(pci && pci->sType==VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PCI_BUS_INFO_PROPERTIES_EXT);
    const unsigned index=static_cast<unsigned>(reinterpret_cast<std::uintptr_t>(device)-10);
    pci->pciDomain=mode==12?0:index;pci->pciBus=3;pci->pciDevice=0;pci->pciFunction=0;
    if(mode==14)pci->pciBus=256;if(mode==15)pci->pciDevice=32;if(mode==16)pci->pciFunction=8;
    out->properties.apiVersion=VK_MAKE_API_VERSION(0,1,index==0?3:4,123);
    if(mode==17)out->properties.apiVersion=0;if(mode==18)out->properties.apiVersion=VK_MAKE_API_VERSION(1,1,3,123);
    out->properties.deviceType=mode==19?VK_PHYSICAL_DEVICE_TYPE_CPU:VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;
}
LinuxVulkanApi api{version,create,destroy,devices,extensions,props};
std::vector<GpuInfo> run(int scenario,unsigned count=2){mode=scenario;n=count;creates=destroys=enumerations=properties=extension_calls=0;auto g=targets();
    for(auto& gpu:g)gpu.vulkan_version={std::string("stale loader"),Support::available,"old"};fill_linux_gpu_vulkan(g,api);
    assert(!g[2].vulkan_version.value);assert(destroys==(mode==1 || mode==2 || mode==3?0:1));return g;
}
}
#endif
int main(int argc,char**){
    if(argc>1){auto g=targets();fill_linux_gpu_vulkan(g);assert(g[0].vulkan_version.value=="1.3.123" && g[1].vulkan_version.value=="1.4.123" && !g[2].vulkan_version.value);std::cout<<"Vulkan dynamic loader fixture passed\n";return 0;}
#if PCINSIGHT_HAS_LINUX_VULKAN_HEADERS
    auto g=run(0);assert(g[0].vulkan_version.value=="1.3.123" && g[1].vulkan_version.value=="1.4.123" && properties==2);
    for(int m:{1,2,3,4,5,6,7,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24}){g=run(m);assert(!g[0].vulkan_version.value);if(m==7)assert(enumerations==6);if(m==13)assert(properties==0);}
    g=run(8);assert(g[0].vulkan_version.value=="1.3.123" && enumerations==4);
    g=run(0,0);assert(!g[0].vulkan_version.value && properties==0);
    mode=0;n=2;g=targets();g.push_back(g[0]);fill_linux_gpu_vulkan(g,api);assert(!g[0].vulkan_version.value && !g[3].vulkan_version.value && g[1].vulkan_version.value);
    g=targets();g[0].physical_location.value="bad pci";fill_linux_gpu_vulkan(g,api);assert(!g[0].vulkan_version.value && g[1].vulkan_version.value);
    g=targets();g[0].physical_location.support=Support::estimated;fill_linux_gpu_vulkan(g,api);assert(!g[0].vulkan_version.value);
    auto missing=api;missing.destroy=nullptr;creates=0;fill_linux_gpu_vulkan(g,missing);assert(creates==0);
    missing=api;missing.extensions=nullptr;fill_linux_gpu_vulkan(g,missing);assert(creates==0);
    g=targets();for(auto& gpu:g)gpu.physical_location.value.reset();fill_linux_gpu_vulkan(g,api);assert(creates==0);
#endif
    auto native=targets();fill_linux_gpu_vulkan(native);for(const auto& gpu:native)std::cout<<*gpu.physical_location.value<<" | Vulkan="<<gpu.vulkan_version.value.value_or("unavailable")<<" | "<<gpu.vulkan_version.note<<'\n';
    Monitor monitor;SampleOptions options;options.include_gpus=false;assert(monitor.sample(options).gpus.empty());options.include_gpus=true;
    for(const auto& gpu:monitor.sample(options).gpus){if(gpu.vulkan_version.value)assert(gpu.vulkan_version.note.find("physical-device")!=std::string::npos);}
#if !PCINSIGHT_HAS_LINUX_VULKAN_HEADERS
    for(const auto& gpu:native)assert(!gpu.vulkan_version.value && gpu.vulkan_version.note.find("not built")!=std::string::npos);
#endif
    std::cout<<"Linux Vulkan tests passed\n";
}
