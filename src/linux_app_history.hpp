#pragma once
#include "linux_cpu_usage.hpp"
#include "linux_process_details.hpp"
#include <map>
namespace pcinsight::detail {
struct LinuxHistoryObservation {
    std::uint32_t pid{};
    Metric<LinuxProcessStat> counters;
    Metric<LinuxExecutableInfo> executable;
    std::string name;
    std::chrono::steady_clock::time_point at;
};
class LinuxAppHistory {
    struct Accumulator {LinuxAppHistoryEntry row;std::uint64_t remainder{};};
    struct Previous {LinuxProcessStat counters;LinuxExecutableStamp executable;std::string path;std::chrono::steady_clock::time_point at;};
    std::map<std::uint32_t,Previous> previous_;
    std::map<std::string,Accumulator> rows_;
    std::chrono::system_clock::time_point started_{},last_{};
    std::optional<std::uint64_t> boot_;
    bool initialized_{};
    long hz_{};
public:
    Metric<std::vector<LinuxAppHistoryEntry>> sample(const std::optional<LinuxCpuCounters>&,long hz,std::span<const LinuxHistoryObservation>,std::chrono::system_clock::time_point now);
    void break_continuity(){previous_.clear();}
    void reset(){previous_.clear();rows_.clear();started_={};last_={};boot_.reset();initialized_=false;hz_=0;}
};
}
