#pragma once
#include "pcinsight/types.hpp"
#include <functional>
#include <set>
#include <string_view>
namespace pcinsight::detail {
struct LinuxSwapEntry {
    std::string path;
    bool file{};
    std::uint64_t size_kib{},used_kib{};
    std::int32_t priority{};
};
struct LinuxSwapDiskReport {
    std::set<std::string> disks;
    bool complete{true};
    Support unresolved_support{Support::unsupported};
};
struct LinuxSwapApi {
    std::function<Metric<std::string>()> inventory;
    std::function<Metric<std::string>(const LinuxSwapEntry&)> disk;
    // Optional provider policy: false preserves positives but forbids negative
    // flags on other rows when the mapped device may hide additional layers.
    std::function<bool(const std::string&)> may_exclude_others;
};
Metric<std::vector<LinuxSwapEntry>> parse_linux_swaps(std::string_view);
Metric<LinuxSwapDiskReport> sample_linux_swap_disks(const LinuxSwapApi&);
void apply_linux_swap_disk_flags(std::vector<DiskInfo>&,const Metric<LinuxSwapDiskReport>&);
Metric<std::string> read_linux_swap_path_disk(const LinuxSwapEntry&);
// Internal seam: native pinned-fd path validation around a supplied block resolver.
// Tests use it to force path mutation between the endpoint checks, never swapon.
Metric<std::string> read_linux_swap_path_disk_with_resolver(const LinuxSwapEntry&,
    const std::function<std::optional<std::string>(const std::string&)>&);
Metric<LinuxSwapDiskReport> read_native_linux_swap_disks();
}
