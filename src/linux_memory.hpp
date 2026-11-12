#pragma once
#include "pcinsight/types.hpp"
#include <map>
#include <string_view>
namespace pcinsight::detail {
using LinuxMeminfo=std::map<std::string,std::uint64_t>;
Metric<LinuxMeminfo> parse_linux_meminfo(std::string_view);
Metric<LinuxMeminfo> read_linux_meminfo();
void apply_linux_meminfo(MemoryInfo&,const Metric<LinuxMeminfo>&);
void apply_linux_memory_reservation(MemoryInfo&,const Metric<std::uint64_t>& installed);
}
