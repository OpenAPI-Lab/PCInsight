#pragma once
#include "types.hpp"
namespace pcinsight {
struct LinuxFilesystemTarget {
    std::string path; // Absolute regular image file, or explicitly allowed block device.
    std::uint64_t offset_bytes{}; // Filesystem start, not superblock offset.
    std::optional<std::uint64_t> region_length_bytes; // Absent -> remainder of source.
    bool allow_block_device{false}; // Explicit opt-in; never auto-elevates.
    std::optional<std::string> expected_kernel_block_device; // major:minor guard, no authorization implied.
};
struct LinuxFilesystemCapacity {
    std::string source_path, format{"ext-family"};
    Metric<std::string> filesystem_uuid; // Declared UUID only; clones can share it, not authorization/dedup identity.
    std::uint64_t declared_capacity_bytes{}, block_count{}, block_size_bytes{};
    std::uint64_t source_size_bytes{}, region_offset_bytes{}, region_length_bytes{};
    std::uint32_t compatible_features{}, incompatible_features{}, readonly_features{};
    bool source_is_block_device{}, needs_journal_recovery{}, orphan_entries_may_be_present{};
    Metric<bool> superblock_checksum_valid;
    // Kernel device identity of the selected logical block object, NOT physical
    // backing disks. Absent for regular images (their host disk is irrelevant).
    Metric<std::string> kernel_block_device;
    Metric<std::string> disk_inventory_id; // Direct sysfs disk/partition parent ID, not physical backing or aggregate.
    Metric<std::uint64_t> disk_sequence;
};
// Read-only metadata query independent of mountinfo/mount state; ext2/3/4 policy.
// Reads the primary 1024-byte superblock twice. No format/mount/fsck/helper/CLI.
// Declared filesystem extent includes metadata, NOT statvfs usable/free capacity.
// Does not discover partitions, infer physical backing, or sum aliases/layers.
Metric<LinuxFilesystemCapacity> read_linux_filesystem_capacity(const LinuxFilesystemTarget&);
struct LinuxFilesystemRegionCapacity {
    LinuxFilesystemTarget target;
    std::uint64_t offset_in_disk_bytes{};
    Metric<LinuxFilesystemCapacity> capacity;
};
struct LinuxDiskFilesystemCapacity {
    std::string disk_inventory_id;
    std::vector<LinuxFilesystemRegionCapacity> regions;
    // Sum of non-overlapping selected kernel partitions (or whole object if no
    // partitions). Absent if ANY region cannot be measured; not physical backing.
    Metric<std::uint64_t> declared_total_bytes;
};
Metric<LinuxDiskFilesystemCapacity> read_linux_disk_filesystem_capacity(
    const std::string& disk_inventory_id, bool allow_block_devices = false);
}
