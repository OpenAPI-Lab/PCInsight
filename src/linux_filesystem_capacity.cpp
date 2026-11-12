#include "linux_filesystem_capacity.hpp"
#include "linux_formatted_capacity.hpp"
#include <array>
#include <limits>
#ifdef __linux__
#include <cerrno>
#include <fcntl.h>
#include <linux/fs.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <unistd.h>
#endif
namespace pcinsight {
namespace {
using Result = Metric<LinuxFilesystemCapacity>;
Result absent(std::string note, Support support = Support::unsupported) { return {std::nullopt, support, std::move(note)}; }
std::uint32_t le32(std::span<const std::uint8_t> bytes, std::size_t offset) {
    std::uint32_t out = 0;
    for (unsigned i = 0; i != 4; ++i) out |= static_cast<std::uint32_t>(bytes[offset + i]) << (i * 8);
    return out;
}
std::uint16_t le16(std::span<const std::uint8_t> bytes, std::size_t offset) {
    return static_cast<std::uint16_t>(bytes[offset] | (static_cast<unsigned>(bytes[offset + 1]) << 8));
}
}
namespace detail {
std::uint32_t linux_ext_superblock_crc32c(std::span<const std::uint8_t> bytes) {
    std::uint32_t crc = UINT32_MAX;
    for (const auto byte : bytes) {
        crc ^= byte;
        for (unsigned i = 0; i < 8; ++i) crc = (crc >> 1) ^ ((crc & 1) ? 0x82f63b78U : 0U);
    }
    return crc; // ext superblock stores the raw CRC, no final complement.
}
Result parse_linux_ext_capacity(std::span<const std::uint8_t> bytes, std::uint64_t region_bytes) {
    if (bytes.size() != 1024 || le16(bytes, 0x38) != 0xef53) return absent("No supported primary ext-family superblock");
    const auto revision = le32(bytes, 0x4c), log_size = le32(bytes, 0x18);
    if (revision > 1 || log_size > 6) return absent("Unsupported ext revision or block size");
    LinuxFilesystemCapacity out;
    out.block_size_bytes = std::uint64_t{1024} << log_size;
    if (revision == 1) {
        out.compatible_features = le32(bytes, 0x5c); out.incompatible_features = le32(bytes, 0x60); out.readonly_features = le32(bytes, 0x64);
        constexpr std::uint32_t incompatible = 0x2 | 0x4 | 0x10 | 0x40 | 0x80 | 0x100 | 0x200 | 0x400 | 0x2000 | 0x4000 | 0x8000 | 0x10000 | 0x20000;
        constexpr std::uint32_t readonly = 0x1 | 0x2 | 0x8 | 0x10 | 0x20 | 0x40 | 0x100 | 0x400 | 0x1000 | 0x2000 | 0x8000 | 0x10000;
        if ((out.incompatible_features & ~incompatible) || (out.readonly_features & ~readonly))
            return absent("Unsupported ext feature/layout (including bigalloc/journal-device/unknown); incompat=" +
                std::to_string(out.incompatible_features) + "; ro_compat=" + std::to_string(out.readonly_features));
        if (((out.incompatible_features & 0x4) && !(out.compatible_features & 0x4)) ||
            ((out.incompatible_features & 0x2000) && !(out.readonly_features & 0x400)))
            return absent("Inconsistent ext journal/checksum feature dependencies");
        if ((out.readonly_features & 0x10000) && (!(out.compatible_features & 0x1000) ||
            !le32(bytes, 0x280) || le32(bytes, 0x280) > le32(bytes, 0)))
            return absent("Inconsistent ext orphan-file feature/inode declaration");
        const auto inode_size = le16(bytes, 0x58);
        if (inode_size < 128 || inode_size > out.block_size_bytes || (inode_size & (inode_size - 1)))
            return absent("Invalid ext inode size");
    }
    const bool wide = (out.incompatible_features & 0x80) != 0;
    if (!wide && (le32(bytes, 0x150) || le32(bytes, 0x154) || le32(bytes, 0x158)))
        return absent("Ext high block counters without 64bit feature");
    if (wide && (le16(bytes, 0xfe) < 64 || le16(bytes, 0xfe) > out.block_size_bytes || le16(bytes, 0xfe) % 8))
        return absent("Invalid ext 64bit group descriptor size");
    auto blocks = [&](std::size_t low, std::size_t high) {
        return std::uint64_t{le32(bytes, low)} | (wide ? std::uint64_t{le32(bytes, high)} << 32 : 0);
    };
    out.block_count = blocks(4, 0x150);
    const auto free_blocks = blocks(0xc, 0x158), reserved_blocks = blocks(8, 0x154);
    const auto first = le32(bytes, 0x14), per_group = le32(bytes, 0x20), inodes_per_group = le32(bytes, 0x28);
    const auto inodes = le32(bytes, 0), free_inodes = le32(bytes, 0x10);
    if (!out.block_count || out.block_count <= first || first != (log_size == 0 ? 1U : 0U) ||
        free_blocks > out.block_count || reserved_blocks > out.block_count || !inodes || free_inodes > inodes ||
        !per_group || per_group > out.block_size_bytes * 8 || !inodes_per_group || inodes_per_group > out.block_size_bytes * 8 ||
        le32(bytes, 0x1c) != log_size || le32(bytes, 0x24) != per_group)
        return absent("Inconsistent ext counters/group/cluster geometry");
    const auto groups = (out.block_count - first - 1) / per_group + 1;
    if (groups > UINT64_MAX / inodes_per_group || inodes > groups * inodes_per_group)
        return absent("Inconsistent ext inode/group bounds");
    if (out.block_count > UINT64_MAX / out.block_size_bytes) return absent("Ext capacity multiplication overflow");
    out.declared_capacity_bytes = out.block_count * out.block_size_bytes;
    if (region_bytes < 2048 || out.declared_capacity_bytes > region_bytes || out.declared_capacity_bytes < 2048)
        return absent("Ext declared extent exceeds selected region or is too small");
    if (out.readonly_features & 0x400) {
        if (bytes[0x175] != 1 || le32(bytes, 0x3fc) != linux_ext_superblock_crc32c(bytes.first(0x3fc)))
            return absent("Ext superblock metadata checksum invalid or unsupported");
        out.superblock_checksum_valid = {true, Support::available, "Primary ext superblock CRC32C matched, not a full filesystem consistency check"};
    } else out.superblock_checksum_valid = {std::nullopt, Support::unsupported, "No metadata_csum superblock checksum configured; not authenticated"};
    const char* hex = "0123456789abcdef";
    std::string uuid; bool uuid_nonzero = false;
    if (revision == 1) for (unsigned i = 0; i != 16; ++i) {
        if (i == 4 || i == 6 || i == 8 || i == 10) uuid += '-';
        uuid += hex[bytes[0x68 + i] >> 4]; uuid += hex[bytes[0x68 + i] & 15];
        uuid_nonzero = uuid_nonzero || bytes[0x68 + i] != 0;
    }
    out.filesystem_uuid = {uuid_nonzero ? std::optional<std::string>{uuid} : std::nullopt,
        uuid_nonzero ? Support::available : Support::unsupported,
        "Declared dynamic-revision UUID if nonzero; clones can share it, not authentication or dedup identity"};
    out.needs_journal_recovery = (out.incompatible_features & 0x4) != 0;
    out.orphan_entries_may_be_present = (out.readonly_features & 0x10000) != 0;
    out.region_length_bytes = region_bytes;
    return {std::move(out), Support::estimated,
        "Ext-family primary superblock DECLARED extent including filesystem metadata; not mounted statvfs capacity/free space, fsck/mountability proof or physical disk capacity"};
}
}
Metric<LinuxFilesystemCapacity> read_linux_filesystem_capacity(const LinuxFilesystemTarget& target) {
    if (target.path.empty() || target.path.size() > 32768 || target.path.front() != '/' || target.path.find('\0') != target.path.npos ||
        (target.region_length_bytes && *target.region_length_bytes < 2048))
        return absent("Invalid filesystem target path/region");
#ifdef __linux__
    struct File { int fd; ~File() { if (fd >= 0) ::close(fd); } } file{
        ::open(target.path.c_str(), O_RDONLY | O_CLOEXEC | O_NONBLOCK | O_NOFOLLOW)};
    auto error = [](const char* note) { return absent(note, errno == EACCES || errno == EPERM ? Support::permission_required : Support::unsupported); };
    if (file.fd < 0) return error("Filesystem metadata target cannot be opened");
    struct stat before{}, after{}, named{};
    if (::fstat(file.fd, &before) < 0) return error("Filesystem metadata target stat failed");
    const bool block = S_ISBLK(before.st_mode);
    if (!S_ISREG(before.st_mode) && !block) return absent("Filesystem metadata target must be regular image or block device");
    if (block && !target.allow_block_device) return absent("Block-device metadata reads require explicit allow_block_device opt-in");
    const auto block_identity = std::to_string(major(before.st_rdev)) + ":" + std::to_string(minor(before.st_rdev));
    if (target.expected_kernel_block_device && (!block || *target.expected_kernel_block_device != block_identity))
        return absent("Filesystem selected block device differs from expected kernel identity");
    auto size = [&]() -> std::optional<std::uint64_t> {
        if (block) { std::uint64_t bytes{}; if (::ioctl(file.fd, BLKGETSIZE64, &bytes) < 0) return {}; return bytes; }
        struct stat info{}; if (::fstat(file.fd, &info) < 0) return {};
        if (info.st_size < 0) { errno = EOVERFLOW; return {}; }
        return static_cast<std::uint64_t>(info.st_size);
    };
    auto sequence = [&]() -> Metric<std::uint64_t> {
        std::uint64_t value{};
#ifdef BLKGETDISKSEQ
        if (block && ::ioctl(file.fd, BLKGETDISKSEQ, &value) == 0)
            return {value, Support::available, "BLKGETDISKSEQ endpoint observation, not durable identity/security authorization"};
        if (block && (errno == EACCES || errno == EPERM))
            return {std::nullopt, Support::permission_required, "BLKGETDISKSEQ access denied; no durable source identity inferred"};
#else
        (void)value;
#endif
        return {std::nullopt, Support::unsupported, "BLKGETDISKSEQ not exposed for this source/kernel"};
    };
    const auto source_size = size();
    if (!source_size) return error("Filesystem source size unavailable");
    if (target.offset_bytes > *source_size) return absent("Filesystem region offset outside source");
    const auto region = target.region_length_bytes.value_or(*source_size - target.offset_bytes);
    if (region < 2048 || region > *source_size - target.offset_bytes ||
        target.offset_bytes > static_cast<std::uint64_t>(std::numeric_limits<off_t>::max()) - 2048)
        return absent("Filesystem region exceeds source or native offset range");
    const auto initial_sequence = sequence();
    auto read = [&](std::array<std::uint8_t, 1024>& header) {
        std::size_t total = 0;
        while (total < header.size()) {
            const auto count = ::pread(file.fd, header.data() + total, header.size() - total,
                static_cast<off_t>(target.offset_bytes + 1024 + total));
            if (count < 0 && errno == EINTR) continue;
            if (count <= 0) { if (!count) errno = EIO; return false; }
            total += static_cast<std::size_t>(count);
        }
        return true;
    };
    std::array<std::uint8_t, 1024> first{}, second{};
    if (!read(first)) return error("Filesystem primary superblock read failed or truncated");
    auto result = detail::parse_linux_ext_capacity(first, region);
    if (!result.value) return result;
    const auto disk = block ? detail::resolve_linux_block_device(block_identity) : std::optional<std::string>{};
    if (!read(second)) return error("Filesystem primary superblock re-read failed");
    if (first != second) return absent("Filesystem primary superblock changed during query");
    if (::fstat(file.fd, &after) < 0 || ::lstat(target.path.c_str(), &named) < 0)
        return error("Filesystem source endpoint cannot be revalidated");
    auto same = [](const struct stat& a, const struct stat& b) {
        return a.st_dev == b.st_dev && a.st_ino == b.st_ino && a.st_rdev == b.st_rdev && a.st_mode == b.st_mode &&
            a.st_nlink == b.st_nlink && a.st_size == b.st_size && a.st_mtim.tv_sec == b.st_mtim.tv_sec && a.st_mtim.tv_nsec == b.st_mtim.tv_nsec &&
            a.st_ctim.tv_sec == b.st_ctim.tv_sec && a.st_ctim.tv_nsec == b.st_ctim.tv_nsec;
    };
    const auto final_size = size();
    if (!final_size) return error("Filesystem source final size unavailable");
    const auto final_sequence = sequence();
    if (initial_sequence.value != final_sequence.value)
        return absent("Filesystem disk sequence changed or became unavailable", final_sequence.support == Support::permission_required ? Support::permission_required : Support::unsupported);
    if (!same(before, after) || !same(before, named) || final_size != source_size)
        return absent("Filesystem source identity/metadata/size/diskseq changed during query");
    auto& out = *result.value;
    out.source_path = target.path; out.source_size_bytes = *source_size; out.region_offset_bytes = target.offset_bytes;
    out.source_is_block_device = block;
    if (block) {
        out.kernel_block_device = {block_identity, Support::available,
            "Selected logical kernel block object; NOT physical backing disk, partition table discovery or per-disk aggregate"};
        out.disk_inventory_id = {disk, disk ? Support::estimated : Support::unsupported,
            "Direct sysfs disk/partition-parent inventory ID if resolvable; NOT backing-disk distribution for RAID/LVM/loop or permission to sum layers/aliases"};
        out.disk_sequence = initial_sequence.support == Support::permission_required ? initial_sequence : final_sequence;
    } else {
        out.kernel_block_device.note = "Regular filesystem image has no selected block-device association; host st_dev is NOT its target disk";
        out.disk_inventory_id.note = "No disk inventory association inferred from an image file's host filesystem";
        out.disk_sequence.note = "No block-device disk sequence for regular image";
    }
    return result;
#else
    return absent("Native filesystem metadata capacity reader requires Linux");
#endif
}
}
