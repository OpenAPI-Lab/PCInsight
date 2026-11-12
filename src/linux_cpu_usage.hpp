#pragma once
#include "pcinsight/types.hpp"
#include <array>
#include <string_view>
namespace pcinsight::detail {
struct LinuxCpuCounters {
    std::array<std::uint64_t,8> ticks{};
    unsigned fields{};
    std::vector<std::uint32_t> cpu_ids;
    std::optional<std::uint64_t> boot_time;
};
struct LinuxCpuInterval {std::uint64_t total{},idle{};};
bool valid_linux_cpu_counters(const LinuxCpuCounters&);
std::optional<LinuxCpuCounters> parse_linux_cpu_counters(std::string_view);
std::optional<LinuxCpuCounters> read_linux_cpu_counters();
std::optional<LinuxCpuInterval> linux_cpu_interval(const std::optional<LinuxCpuCounters>& previous,const std::optional<LinuxCpuCounters>& current);
Metric<double> linux_cpu_usage(const std::optional<LinuxCpuCounters>& previous,const std::optional<LinuxCpuCounters>& current);
}
