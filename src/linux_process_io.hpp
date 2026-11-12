#pragma once
#include "linux_process_stat.hpp"
#include "linux_process_details.hpp"
#include "pcinsight/types.hpp"
#include <filesystem>
#include <map>
#include <span>
namespace pcinsight::detail {
struct LinuxProcessIoCounters {std::uint64_t read_bytes{},write_bytes{};};
Metric<LinuxProcessIoCounters> parse_linux_process_io_counters(std::string_view);
Metric<LinuxProcessIoCounters> linux_process_io_for_identity(const LinuxProcessStat& expected,const std::optional<LinuxProcessStat>& before,const std::optional<LinuxProcessStat>& after,Metric<LinuxProcessIoCounters> counters);
Metric<LinuxProcessIoCounters> read_linux_process_io_counters(const std::filesystem::path& process_dir,const LinuxProcessStat& expected);
Metric<std::uint32_t> linux_process_io_uid(const Metric<LinuxProcessStatus>& before,
    const Metric<LinuxProcessStatus>& after);
struct LinuxProcessIoObservation {
    std::uint32_t pid{};std::uint64_t start_ticks{};
    Metric<LinuxProcessIoCounters> counters;
    std::chrono::steady_clock::time_point at;
    Metric<std::uint32_t> real_uid{};
};
class LinuxProcessIoRateTracker {
    std::map<std::uint32_t,LinuxProcessIoObservation> previous_;
    std::map<std::uint32_t,Metric<std::uint32_t>> attributed_uids_;
public:
    std::map<std::uint32_t,Metric<std::uint64_t>> sample(std::span<const LinuxProcessIoObservation>);
    Metric<std::uint32_t> attributed_uid(std::uint32_t pid) const;
    void reset(){previous_.clear();attributed_uids_.clear();}
};
}
