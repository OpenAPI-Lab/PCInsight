#pragma once
#include "pcinsight/types.hpp"
#include <filesystem>
#include <string_view>
namespace pcinsight::detail {
Metric<std::chrono::seconds> parse_linux_uptime(std::string_view);
Metric<std::chrono::seconds> read_linux_uptime(const std::filesystem::path& path="/proc/uptime");
void fill_linux_uptime(CpuInfo&,const std::filesystem::path& path="/proc/uptime");
}
