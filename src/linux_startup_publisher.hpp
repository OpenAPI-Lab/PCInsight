#pragma once
#include "pcinsight/linux_startup_publisher.hpp"
#include <filesystem>
#include <string_view>
namespace pcinsight::detail {
Metric<std::vector<LinuxStartupPublisher>> parse_linux_startup_metainfo(std::string_view);
Metric<std::vector<LinuxStartupPublisher>> read_linux_startup_publishers_from(
    const std::vector<std::filesystem::path>&);
std::vector<std::filesystem::path> resolve_linux_metainfo_directories(
    std::string_view home, std::string_view data_home, std::string_view data_dirs);
}
