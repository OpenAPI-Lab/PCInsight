#pragma once
#include "linux_cpu_usage.hpp"
#include "linux_process_stat.hpp"
#include <map>
namespace pcinsight::detail {
struct LinuxProcessCpuObservation {
    std::uint32_t pid{};
    Metric<LinuxProcessStat> counters;
    std::chrono::steady_clock::time_point at;
};
class LinuxProcessCpuTracker {
    std::optional<LinuxCpuCounters> previous_cpu_;
    long previous_hz_{};
    std::map<std::uint32_t,LinuxProcessCpuObservation> previous_;
public:
    std::map<std::uint32_t,Metric<double>> sample(const std::optional<LinuxCpuCounters>&,long ticks_per_second,std::span<const LinuxProcessCpuObservation>);
    void reset(){previous_cpu_.reset();previous_hz_=0;previous_.clear();}
};
}
