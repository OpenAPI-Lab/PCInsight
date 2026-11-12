#pragma once
#include "pcinsight/linux_filesystem_capacity.hpp"
#include <span>
#include <functional>
namespace pcinsight::detail {
std::uint32_t linux_ext_superblock_crc32c(std::span<const std::uint8_t>);
Metric<LinuxFilesystemCapacity> parse_linux_ext_capacity(std::span<const std::uint8_t>, std::uint64_t region_bytes);
struct LinuxFilesystemRegion {
    std::string name, device;
    std::uint64_t offset{}, size{};
    bool operator==(const LinuxFilesystemRegion&) const = default;
};
struct LinuxFilesystemLayout {
    std::string disk, device, sequence;
    std::uint64_t size{};
    std::vector<LinuxFilesystemRegion> regions;
    bool operator==(const LinuxFilesystemLayout&) const = default;
};
struct LinuxFilesystemDiskApi {
    std::function<Metric<LinuxFilesystemLayout>(const std::string&)> layout;
    std::function<Metric<LinuxFilesystemCapacity>(const LinuxFilesystemTarget&)> probe;
};
Metric<LinuxDiskFilesystemCapacity> read_linux_disk_filesystem_capacity_with_api(
    const std::string&, const LinuxFilesystemDiskApi&);
}
