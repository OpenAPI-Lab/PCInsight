#include "linux_gpu_driver_date.hpp"
namespace pcinsight::detail {
Metric<std::string> linux_gpu_driver_date_unavailable(LinuxGpuDriverDateProvider provider) {
    std::string source;
    switch(provider) {
    case LinuxGpuDriverDateProvider::drm_sysfs:
        source="DRM/sysfs has no verified release/installation date source; DRM_VERSION.date is deprecated (placeholder or historical static text)";
        break;
    case LinuxGpuDriverDateProvider::nvml:
        source="NVML system driver-version query provides a version, not a release/installation date";
        break;
    case LinuxGpuDriverDateProvider::drm_nvml:
        source="DRM/sysfs and NVML merged by PCI; neither queried interface provides a verified release/installation date";
        break;
    default:
        source="no registered Linux driver release/installation date source";
        break;
    }
    return {{},Support::unsupported,"Linux driver date unavailable: "+source+
        "; not inferred from BIOS, file timestamps, module build text or graphics API versions"};
}
}
