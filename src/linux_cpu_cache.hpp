#pragma once
#include "pcinsight/types.hpp"
#include <array>
#include <filesystem>
#include <string_view>
namespace pcinsight::detail {
struct LinuxCacheEntry {
    std::uint32_t cpu{}, level{};
    std::string type;
    std::uint64_t bytes{};
    std::vector<std::uint32_t> shared;
    auto operator<=>(const LinuxCacheEntry&) const = default;
};
struct LinuxCacheSnapshot {
    std::vector<std::uint32_t> online;
    std::vector<LinuxCacheEntry> entries;
    bool operator==(const LinuxCacheSnapshot&) const = default;
};
std::optional<std::vector<std::uint32_t>> parse_linux_cache_cpulist(std::string_view);
std::optional<std::uint64_t> parse_linux_cache_size(std::string_view);
std::array<Metric<std::uint64_t>,3> linux_cache_totals(const LinuxCacheSnapshot&);
Metric<LinuxCacheSnapshot> read_linux_cache_snapshot(const std::filesystem::path& root="/sys/devices/system/cpu");
void fill_linux_cpu_cache(CpuInfo&,const std::filesystem::path& root="/sys/devices/system/cpu");
}
