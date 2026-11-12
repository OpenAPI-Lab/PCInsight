#pragma once
#include "pcinsight/types.hpp"
#include <filesystem>
#include <string_view>
namespace pcinsight::detail {
struct LinuxFrequencyRow {
    std::uint32_t cpu{};
    std::optional<std::uint64_t> hardware_khz,scaling_khz,base_khz;
    bool operator==(const LinuxFrequencyRow&) const = default;
};
struct LinuxFrequencySnapshot {
    std::vector<std::uint32_t> online;
    std::vector<LinuxFrequencyRow> rows;
};
std::optional<std::uint64_t> parse_linux_frequency_khz(std::string_view);
Metric<LinuxFrequencySnapshot> read_linux_cpu_frequency(const std::filesystem::path& root="/sys/devices/system/cpu");
void apply_linux_cpu_frequency(CpuInfo&,const Metric<LinuxFrequencySnapshot>& first,const Metric<LinuxFrequencySnapshot>& second);
void fill_linux_cpu_frequency(CpuInfo&,const std::filesystem::path& root="/sys/devices/system/cpu");
}
