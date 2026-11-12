#pragma once
#include "pcinsight/types.hpp"
#ifdef _WIN32
#if PCINSIGHT_HAS_VULKAN_HEADERS
#include <vulkan/vulkan_core.h>
#endif
namespace pcinsight::detail {
#if PCINSIGHT_HAS_VULKAN_HEADERS
struct GpuVulkanApi {
    PFN_vkEnumerateInstanceVersion version{};
    PFN_vkCreateInstance create{};
    PFN_vkDestroyInstance destroy{};
    PFN_vkEnumeratePhysicalDevices enumerate{};
    PFN_vkGetPhysicalDeviceProperties2 properties{};
};
void fill_windows_gpu_vulkan(std::vector<GpuInfo>&,GpuVulkanApi);
#endif
void fill_windows_gpu_vulkan(std::vector<GpuInfo>&);
}
#endif
