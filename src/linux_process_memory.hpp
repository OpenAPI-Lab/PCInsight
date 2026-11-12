#pragma once
#include "pcinsight/types.hpp"
#include "linux_process_stat.hpp"
#include <filesystem>
namespace pcinsight::detail {
Metric<std::uint64_t> parse_linux_process_rss(std::string_view,std::optional<std::uint32_t> expected_pid=std::nullopt);
Metric<std::uint64_t> linux_process_rss_for_identity(const LinuxProcessStat& expected,const std::optional<LinuxProcessStat>& before,const std::optional<LinuxProcessStat>& after,Metric<std::uint64_t> rss);
Metric<std::uint64_t> read_linux_process_memory(const std::filesystem::path& process_dir,const LinuxProcessStat& expected);
}
