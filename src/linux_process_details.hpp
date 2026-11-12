#pragma once
#include "linux_process_stat.hpp"
#include "pcinsight/types.hpp"
#include <filesystem>
namespace pcinsight::detail {
struct LinuxProcessStatus {
    std::uint32_t pid{};
    std::optional<std::uint32_t> real_uid;
    std::optional<std::string> name,state;
};
Metric<LinuxProcessStatus> parse_linux_process_status(std::string_view,std::uint32_t expected_pid);
Metric<LinuxProcessStatus> linux_process_status_for_identity(const LinuxProcessStat&,const std::optional<LinuxProcessStat>& before,const std::optional<LinuxProcessStat>& after,Metric<LinuxProcessStatus>);
Metric<LinuxProcessStat> read_linux_process_identity(const std::filesystem::path&);
Metric<LinuxProcessStatus> read_linux_process_status(const std::filesystem::path&,const LinuxProcessStat&);
Metric<std::string> linux_username_for_uid(std::uint32_t);
struct LinuxExecutableStamp {
    std::uint64_t device{},inode{},size{},mode{};
    std::int64_t mtime_seconds{},mtime_nanoseconds{},ctime_seconds{},ctime_nanoseconds{};
    bool operator==(const LinuxExecutableStamp&) const = default;
};
struct LinuxExecutableInfo {Metric<std::string> path,architecture;LinuxExecutableStamp stamp{};};
Metric<std::string> linux_executable_architecture(std::span<const unsigned char>);
bool linux_executable_matches(const LinuxProcessStat& expected,const std::optional<LinuxProcessStat>& before,const std::optional<LinuxProcessStat>& after,const LinuxExecutableStamp& first,const LinuxExecutableStamp& last,std::string_view first_path,std::string_view last_path);
Metric<LinuxExecutableInfo> read_linux_process_executable(const std::filesystem::path&,const LinuxProcessStat&);
struct LinuxProcessDescription {std::string text,source;};
void apply_linux_process_details(ProcessInfo&,const Metric<LinuxProcessStatus>&,const Metric<std::string>& account,const Metric<LinuxExecutableInfo>&,const Metric<LinuxProcessDescription>&);
}
