#pragma once
#include "pcinsight/types.hpp"
#ifdef __linux__
#if PCINSIGHT_HAS_LINUX_VULKAN_HEADERS
#include <vulkan/vulkan_core.h>
#endif
namespace pcinsight::detail {
#if PCINSIGHT_HAS_LINUX_VULKAN_HEADERS
struct LinuxVulkanApi {
    PFN_vkEnumerateInstanceVersion version{};
    PFN_vkCreateInstance create{};
    PFN_vkDestroyInstance destroy{};
    PFN_vkEnumeratePhysicalDevices enumerate{};
    PFN_vkEnumerateDeviceExtensionProperties extensions{};
    PFN_vkGetPhysicalDeviceProperties2 properties{};
};
void fill_linux_gpu_vulkan(std::vector<GpuInfo>&,LinuxVulkanApi);
#endif
void fill_linux_gpu_vulkan(std::vector<GpuInfo>&);
}
#endif
