#include "../src/windows_gpu_vulkan.hpp"
#include "../src/windows_gpus.hpp"
#include "pcinsight/monitor.hpp"
#include <cassert>
#include <cstring>
#include <iostream>
using namespace pcinsight;
using namespace pcinsight::detail;
#if PCINSIGHT_HAS_VULKAN_HEADERS
namespace {
int mode,creates,destroys,enumerations,properties;unsigned n;
VkInstance instance=reinterpret_cast<VkInstance>(1);
VKAPI_ATTR VkResult VKAPI_CALL version(std::uint32_t* out){*out=mode==1?VK_API_VERSION_1_0:VK_MAKE_API_VERSION(0,1,4,500);return mode==2?VK_ERROR_INITIALIZATION_FAILED:VK_SUCCESS;}
VKAPI_ATTR VkResult VKAPI_CALL create(const VkInstanceCreateInfo* info,const VkAllocationCallbacks* allocator,VkInstance* out){
    ++creates;assert(!allocator && !info->enabledLayerCount && !info->enabledExtensionCount);
    assert(info->pApplicationInfo->apiVersion==VK_API_VERSION_1_1);
    *out=mode==3?VK_NULL_HANDLE:instance;return mode==4?VK_ERROR_INITIALIZATION_FAILED:VK_SUCCESS;
}
VKAPI_ATTR void VKAPI_CALL destroy(VkInstance value,const VkAllocationCallbacks* allocator){assert(value==instance && !allocator);++destroys;}
VKAPI_ATTR VkResult VKAPI_CALL enumerate(VkInstance value,std::uint32_t* count,VkPhysicalDevice* out){
    assert(value==instance);++enumerations;
    if(mode==5)return VK_ERROR_INITIALIZATION_FAILED;
    if(!out){*count=mode==6?257:n;return VK_SUCCESS;}
    const auto capacity=*count;assert(capacity==n);
    for(unsigned i=0;i<n;++i)out[i]=reinterpret_cast<VkPhysicalDevice>(static_cast<std::uintptr_t>(i+10));
    if(mode==7)return VK_INCOMPLETE;
    if(mode==8 && enumerations==2)return VK_INCOMPLETE;
    if(mode==9)++*count;
    if(mode==10)out[0]=VK_NULL_HANDLE;
    if(mode==11 && n>1)out[1]=out[0];
    return VK_SUCCESS;
}
VKAPI_ATTR void VKAPI_CALL props(VkPhysicalDevice device,VkPhysicalDeviceProperties2* out){
    ++properties;assert(out->sType==VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2);
    auto* ids=static_cast<VkPhysicalDeviceIDProperties*>(out->pNext);assert(ids && ids->sType==VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ID_PROPERTIES);
    const unsigned index=static_cast<unsigned>(reinterpret_cast<std::uintptr_t>(device)-10);
    LUID luid{index+1,static_cast<LONG>(index==1?0xffffffff:0)};
    if(mode==12)luid={1,0};
    std::memcpy(ids->deviceLUID,&luid,sizeof(luid));ids->deviceNodeMask=1;ids->deviceLUIDValid=VK_TRUE;
    if(mode==13)ids->deviceLUIDValid=VK_FALSE;
    if(mode==14)ids->deviceLUIDValid=2;
    if(mode==15)ids->deviceNodeMask=3;
    if(mode==16)ids->deviceNodeMask=0;
    out->properties.apiVersion=VK_MAKE_API_VERSION(0,1,index==0?3:4,123);
    if(mode==17)out->properties.apiVersion=0;
    if(mode==18)out->properties.apiVersion=VK_MAKE_API_VERSION(1,1,3,123);
    out->properties.deviceType=mode==19?VK_PHYSICAL_DEVICE_TYPE_CPU:VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;
}
GpuVulkanApi api{version,create,destroy,enumerate,props};
std::vector<GpuInfo> run(int scenario,unsigned count=2){mode=scenario;n=count;creates=destroys=enumerations=properties=0;
    std::vector<GpuInfo> gpus(3);gpus[0].id="dxgi-luid:00000000:00000001";gpus[1].id="dxgi-luid:ffffffff:00000002";gpus[2].id="dxgi-luid:00000000:00000003";
    for(auto& gpu:gpus)gpu.vulkan_version={std::string("stale"),Support::available,"old"};
    fill_windows_gpu_vulkan(gpus,api);assert(!gpus[2].vulkan_version.value);
    assert(destroys==(mode==1 || mode==2 || mode==3?0:1));return gpus;
}
}
#endif
int main(){
#if PCINSIGHT_HAS_VULKAN_HEADERS
    auto g=run(0);assert(g[0].vulkan_version.value=="1.3.123" && g[1].vulkan_version.value=="1.4.123");assert(g[0].vulkan_version.support==Support::available);
    // Even an unambiguous source must not be attributed to duplicate targets.
    auto duplicate=g;duplicate.push_back(g[0]);fill_windows_gpu_vulkan(duplicate,api);
    assert(!duplicate[0].vulkan_version.value && !duplicate[3].vulkan_version.value);
    assert(duplicate[1].vulkan_version.value=="1.4.123");
    duplicate=g;duplicate[0].id.clear();fill_windows_gpu_vulkan(duplicate,api);
    assert(!duplicate[0].vulkan_version.value && duplicate[1].vulkan_version.value=="1.4.123");
    for(int m:{1,2,3,4,5,6,7,9,10,11,12,13,14,15,16,17,18,19}){g=run(m);assert(!g[0].vulkan_version.value);if(m==7)assert(enumerations==6);}
    g=run(8);assert(g[0].vulkan_version.value=="1.3.123" && enumerations==4);
    g=run(0,0);assert(!g[0].vulkan_version.value && properties==0);
    auto missing=api;missing.destroy=nullptr;creates=0;fill_windows_gpu_vulkan(g,missing);assert(creates==0);
    missing=api;missing.properties=nullptr;fill_windows_gpu_vulkan(g,missing);assert(creates==0);
    std::vector<GpuInfo> empty;fill_windows_gpu_vulkan(empty,api);assert(creates==0);
#endif
    auto native=read_windows_gpus();fill_windows_gpu_vulkan(native);
    for(const auto& gpu:native)std::cout<<gpu.model<<" | Vulkan="<<gpu.vulkan_version.value.value_or("unavailable")<<" | "<<gpu.vulkan_version.note<<'\n';
    Monitor monitor;SampleOptions options;options.include_gpus=false;assert(monitor.sample(options).gpus.empty());options.include_gpus=true;
    for(const auto& gpu:monitor.sample(options).gpus){if(gpu.vulkan_version.value)assert(gpu.vulkan_version.value->starts_with("1."));}
    options.include_gpus=false;assert(monitor.sample(options).gpus.empty());
#if !PCINSIGHT_HAS_VULKAN_HEADERS
    for(const auto& gpu:native)assert(!gpu.vulkan_version.value && gpu.vulkan_version.note.find("not built")!=std::string::npos);
#endif
    std::cout<<"Windows Vulkan tests passed\n";
}
