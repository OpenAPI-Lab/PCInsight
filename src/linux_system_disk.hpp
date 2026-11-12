#pragma once
#include "linux_formatted_capacity.hpp"
namespace pcinsight::detail {
struct LinuxSystemDiskApi {
    std::function<Metric<std::string>()> root_device;
    std::function<std::optional<std::string>(const std::string&)> disk;
};
Metric<std::string> identify_linux_system_disk(std::string_view mountinfo,const LinuxSystemDiskApi&);
Metric<std::string> read_linux_root_device();
Metric<std::string> read_native_linux_system_disk();
void apply_linux_system_disk_flags(std::vector<DiskInfo>&,const Metric<std::string>&);
}
