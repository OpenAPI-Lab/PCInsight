#pragma once
#include "pcinsight/types.hpp"
#include <array>
#include <map>
#include <span>
#include <string_view>
namespace pcinsight::detail {
using LinuxDiskStat=std::array<std::uint64_t,11>;
std::optional<LinuxDiskStat> parse_linux_disk_stat(std::string_view);
bool valid_linux_disk_device(std::string_view);
struct LinuxDiskCounters {
    std::string name,device;
    std::optional<std::uint64_t> disk_sequence;
    LinuxDiskStat stat{};
};
struct LinuxDiskRates {
    Metric<std::uint64_t> read,write,transfer;
    Metric<double> active,response;
};
class LinuxDiskRateTracker {
    struct Previous { LinuxDiskCounters counters;std::chrono::steady_clock::time_point at; };
    std::map<std::string,Previous> previous_;
public:
    std::map<std::string,LinuxDiskRates> sample(std::span<const LinuxDiskCounters>,std::chrono::steady_clock::time_point);
    void reset(){previous_.clear();}
};
}
