#pragma once
#include "pcinsight/types.hpp"
#include <string_view>
namespace pcinsight::detail {
Metric<LinuxZramMemory> parse_linux_zram_memory(std::string_view);
// Root override is internal, for isolated read-only fixtures. Never creates devices.
Metric<std::vector<LinuxZramDevice>> read_linux_zram(const std::string& root="/sys/block");
}
