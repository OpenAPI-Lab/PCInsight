#include <vulkan/vulkan_core.h>
#include <cstring>
#include <cstdint>
extern "C" {
VKAPI_ATTR VkResult VKAPI_CALL vkEnumerateInstanceVersion(std::uint32_t* out){*out=VK_MAKE_API_VERSION(0,1,4,500);return VK_SUCCESS;}
VKAPI_ATTR VkResult VKAPI_CALL vkCreateInstance(const VkInstanceCreateInfo*,const VkAllocationCallbacks*,VkInstance* out){*out=reinterpret_cast<VkInstance>(1);return VK_SUCCESS;}
VKAPI_ATTR void VKAPI_CALL vkDestroyInstance(VkInstance,const VkAllocationCallbacks*){}
VKAPI_ATTR VkResult VKAPI_CALL vkEnumeratePhysicalDevices(VkInstance,std::uint32_t* count,VkPhysicalDevice* out){
    if(!out){*count=2;return VK_SUCCESS;}if(*count<2)return VK_INCOMPLETE;
    out[0]=reinterpret_cast<VkPhysicalDevice>(10);out[1]=reinterpret_cast<VkPhysicalDevice>(11);*count=2;return VK_SUCCESS;
}
VKAPI_ATTR VkResult VKAPI_CALL vkEnumerateDeviceExtensionProperties(VkPhysicalDevice,const char*,std::uint32_t* count,VkExtensionProperties* out){
    if(!out){*count=1;return VK_SUCCESS;}if(*count<1)return VK_INCOMPLETE;*count=1;std::strcpy(out[0].extensionName,VK_EXT_PCI_BUS_INFO_EXTENSION_NAME);out[0].specVersion=2;return VK_SUCCESS;
}
VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceProperties2(VkPhysicalDevice device,VkPhysicalDeviceProperties2* out){
    const unsigned index=static_cast<unsigned>(reinterpret_cast<std::uintptr_t>(device)-10);
    auto* pci=static_cast<VkPhysicalDevicePCIBusInfoPropertiesEXT*>(out->pNext);pci->pciDomain=index;pci->pciBus=3;pci->pciDevice=pci->pciFunction=0;
    out->properties.deviceType=VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;out->properties.apiVersion=VK_MAKE_API_VERSION(0,1,index?4:3,123);
}
}
