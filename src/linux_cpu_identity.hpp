#pragma once
#include "smbios.hpp"
#include <filesystem>
#include <string_view>
namespace pcinsight::detail {
struct LinuxCpuNames {
    Metric<std::string> vendor,model;
    bool vendor_fallback{true},model_fallback{true};
    std::vector<std::uint32_t> cpu_ids;
};
Metric<LinuxCpuNames> parse_linux_cpu_names(std::string_view);
Metric<LinuxCpuNames> read_linux_cpu_names(const std::filesystem::path& path="/proc/cpuinfo");
void apply_linux_cpu_names(CpuInfo&,const Metric<LinuxCpuNames>&,const SmbiosInventory&);
void fill_linux_cpu_names(CpuInfo&,const SmbiosInventory&,const std::filesystem::path& path="/proc/cpuinfo");
}
