#pragma once
#include "pcinsight/types.hpp"
#include <filesystem>
#include <string_view>
namespace pcinsight::detail {
struct LinuxTopologyRow {
    std::uint32_t cpu{};
    std::optional<std::int32_t> package_id;
    std::optional<std::vector<std::uint32_t>> package_cpus,core_cpus;
    bool operator==(const LinuxTopologyRow&) const = default;
};
struct LinuxTopologySnapshot {
    std::vector<std::uint32_t> online;
    std::vector<LinuxTopologyRow> rows;
    bool operator==(const LinuxTopologySnapshot&) const = default;
};
struct LinuxTopologyCounts {
    Metric<unsigned> sockets,cores,logical;
};
std::optional<std::int32_t> parse_linux_package_id(std::string_view);
LinuxTopologyCounts linux_topology_counts(const LinuxTopologySnapshot&);
Metric<LinuxTopologySnapshot> read_linux_cpu_topology(const std::filesystem::path& root="/sys/devices/system/cpu");
void apply_linux_cpu_topology(CpuInfo&,const Metric<LinuxTopologySnapshot>& first,const Metric<LinuxTopologySnapshot>& second);
void fill_linux_cpu_topology(CpuInfo&,const std::filesystem::path& root="/sys/devices/system/cpu");
}
