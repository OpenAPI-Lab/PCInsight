#pragma once
#include "pcinsight/types.hpp"
#include <filesystem>

namespace pcinsight::detail {
// The root is injectable for fixtures. Counts are scoped to visible procfs PIDs.
void read_linux_counts(CpuInfo& cpu, const std::filesystem::path& root = "/proc");
}
