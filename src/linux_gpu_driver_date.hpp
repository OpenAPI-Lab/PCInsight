#pragma once
#include "pcinsight/types.hpp"
namespace pcinsight::detail {
enum class LinuxGpuDriverDateProvider { drm_sysfs, nvml, drm_nvml };
// Capability policy for the providers actually used by this library. It is not
// a runtime query failure, date parser, or assertion about every future driver.
// Never derive a release/installation date from firmware, filesystem times,
// module build text, API version strings or deprecated DRM_VERSION.date.
Metric<std::string> linux_gpu_driver_date_unavailable(LinuxGpuDriverDateProvider);
}
