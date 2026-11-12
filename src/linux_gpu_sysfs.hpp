#pragma once
#include "pcinsight/types.hpp"
#include <filesystem>
#include <functional>
namespace pcinsight::detail {
struct LinuxGpuSensorInput {std::string id;Metric<std::string> label,millidegrees;};
struct LinuxGpuSysfsInputs {
    Metric<std::string> vendor,device,location,driver,module,module_version;
    Metric<std::string> busy,vram_total,vram_used,gtt_total,gtt_used;
    std::vector<LinuxGpuSensorInput> sensors;
};
GpuInfo parse_linux_gpu_sysfs(const std::string& id,const LinuxGpuSysfsInputs&);
Metric<GpuInfo> read_linux_gpu_sysfs_card(const std::filesystem::path&,
    const std::function<void()>& after_read={});
Metric<std::string> fill_linux_gpu_sysfs(std::vector<GpuInfo>&,const std::filesystem::path& root="/sys/class/drm");
Metric<std::string> linux_gpu_inventory_source(const Metric<std::string>& drm,Support nvml_support,const std::string& nvml_note);
}
