#pragma once
#include "pcinsight/types.hpp"
#include <functional>
#include <map>
#include <string_view>
namespace pcinsight::detail {
struct LinuxCapacityMount { std::string device,path,filesystem; };
std::optional<LinuxCapacityMount> parse_linux_capacity_mount(std::string_view);
bool linux_capacity_filesystem_supported(std::string_view);
Metric<std::string> read_linux_mountinfo();
std::optional<std::string> resolve_linux_block_device(const std::string& device);
Metric<std::uint64_t> linux_filesystem_capacity(std::uint64_t blocks,std::uint64_t fragment_size);
using LinuxFormattedMap=std::map<std::string,Metric<std::uint64_t>>;
struct LinuxFormattedApi {
    std::function<std::optional<std::string>(const std::string& device)> disk;
    std::function<Metric<std::uint64_t>(const LinuxCapacityMount&)> capacity;
};
Metric<LinuxFormattedMap> aggregate_linux_formatted_capacity(std::string_view mountinfo,const LinuxFormattedApi&);
Metric<LinuxFormattedMap> read_native_linux_formatted_capacity();
// Internal native reader exposed for read-only integration tests of fd identity checks.
Metric<std::uint64_t> read_linux_mount_capacity(const LinuxCapacityMount&);
}
