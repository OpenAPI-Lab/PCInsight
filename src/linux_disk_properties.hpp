#pragma once
#include "pcinsight/types.hpp"
#include <string_view>
namespace pcinsight::detail {
Metric<std::uint64_t> linux_disk_capacity(std::string_view sectors);
void apply_linux_disk_properties(DiskInfo&,std::string_view sectors,
    std::string_view rotational,std::string_view subsystem);
}
