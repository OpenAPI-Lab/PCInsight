#include "../src/linux_filesystem_capacity.hpp"
#include <array>
#include <cassert>
#include <iostream>
using namespace pcinsight;
using namespace pcinsight::detail;
using Header = std::array<std::uint8_t, 1024>;
void put(Header& b, std::size_t offset, std::uint32_t value) { for (unsigned i = 0; i != 4; ++i) b[offset + i] = static_cast<std::uint8_t>(value >> (8 * i)); }
Header valid(unsigned log = 2) {
    Header b{};
    put(b, 0, 32); put(b, 4, 128); put(b, 0xc, 100); put(b, 0x10, 20);
    put(b, 0x14, log ? 0 : 1); put(b, 0x18, log); put(b, 0x1c, log);
    put(b, 0x20, 128); put(b, 0x24, 128); put(b, 0x28, 32);
    b[0x38] = 0x53; b[0x39] = 0xef; put(b, 0x4c, 1); b[0x58] = 128;
    for (unsigned i = 0; i != 16; ++i) b[0x68 + i] = static_cast<std::uint8_t>(i);
    return b;
}
int main() {
    const std::array<std::uint8_t, 9> vector{'1','2','3','4','5','6','7','8','9'};
    assert(linux_ext_superblock_crc32c(vector) == 0x1cf96d7cU); // Raw CRC32C, complemented standard check value.
    auto bytes = valid();
    auto parsed = parse_linux_ext_capacity(bytes, 1024 * 1024);
    assert(parsed.value && parsed.value->declared_capacity_bytes == 128 * 4096 && parsed.support == Support::estimated);
    assert(parsed.value->filesystem_uuid.value == "00010203-0405-0607-0809-0a0b0c0d0e0f");
    assert(!parsed.value->superblock_checksum_valid.value && !parsed.value->kernel_block_device.value);
    for (unsigned log = 0; log <= 6; ++log) {
        bytes = valid(log); parsed = parse_linux_ext_capacity(bytes, UINT64_MAX);
        assert(parsed.value && parsed.value->block_size_bytes == (1024ULL << log));
    }
    bytes = valid(0); put(bytes, 0x4c, 0); parsed = parse_linux_ext_capacity(bytes, 1024 * 1024);
    assert(parsed.value && !parsed.value->filesystem_uuid.value);
    for (const auto& change : std::array<std::pair<std::size_t, std::uint32_t>, 15>{{
        {0x4c,2}, {0x18,7}, {0x60,0x80000000U}, {0x60,8}, {0x64,0x200}, {0x64,0x80000000U},
        {0x58,127}, {0x150,1}, {4,0}, {0xc,129}, {0x10,33}, {0x14,1}, {0x20,0}, {0x24,1}, {0x28,0}}}) {
        bytes = valid(); put(bytes, change.first, change.second);
        assert(!parse_linux_ext_capacity(bytes, UINT64_MAX).value);
    }
    bytes = valid(); put(bytes, 0x64, 0x400); bytes[0x175] = 1;
    put(bytes, 0x3fc, linux_ext_superblock_crc32c(std::span(bytes).first(0x3fc)));
    parsed = parse_linux_ext_capacity(bytes, 1024 * 1024);
    assert(parsed.value && parsed.value->superblock_checksum_valid.value == true);
    bytes[0x80] ^= 1; assert(!parse_linux_ext_capacity(bytes, 1024 * 1024).value);
    bytes = valid(); put(bytes, 0x60, 0x80); bytes[0xfe] = 64; put(bytes, 0x150, 1);
    parsed = parse_linux_ext_capacity(bytes, UINT64_MAX); assert(parsed.value && parsed.value->block_count == (1ULL << 32) + 128);
    put(bytes, 0x150, UINT32_MAX); assert(!parse_linux_ext_capacity(bytes, UINT64_MAX).value); // Product overflow.
    bytes = valid(); put(bytes, 0x60, 4); assert(!parse_linux_ext_capacity(bytes, UINT64_MAX).value);
    put(bytes, 0x5c, 4); assert(parse_linux_ext_capacity(bytes, UINT64_MAX).value->needs_journal_recovery);
    bytes = valid(); put(bytes, 0x64, 0x10000); assert(!parse_linux_ext_capacity(bytes, UINT64_MAX).value);
    put(bytes, 0x5c, 0x1000); put(bytes, 0x280, 12);
    assert(parse_linux_ext_capacity(bytes, UINT64_MAX).value->orphan_entries_may_be_present);
    bytes = valid();
    assert(!parse_linux_ext_capacity(std::span(bytes).first(1023), UINT64_MAX).value);
    assert(!parse_linux_ext_capacity(bytes, 1024).value);
    assert(!parse_linux_ext_capacity(bytes, 128 * 4096 - 1).value);
    LinuxFilesystemTarget target;
    assert(!read_linux_filesystem_capacity(target).value);
    target.path = "relative"; assert(!read_linux_filesystem_capacity(target).value);
    target.path = std::string("/bad\0path", 9); assert(!read_linux_filesystem_capacity(target).value);
    // Disk-level policy: all selected regions, no partial/overlap/alias sum.
    LinuxFilesystemLayout layout{"testdisk", "8:0", "1", 4 * 1024 * 1024,
        {{"testdisk1", "8:1", 0, 1024 * 1024}, {"testdisk2", "8:2", 1024 * 1024, 1024 * 1024}}};
    unsigned enumerations = 0, probes = 0; bool denied = false, wrong_device = false, changed = false;
    LinuxFilesystemDiskApi api;
    api.layout = [&](const auto&) -> Metric<LinuxFilesystemLayout> {
        auto copy = layout; if (++enumerations % 2 == 0 && changed) copy.sequence = "2";
        return {copy, Support::estimated, "fixture"};
    };
    api.probe = [&](const LinuxFilesystemTarget& input) -> Metric<LinuxFilesystemCapacity> {
        ++probes; assert(input.allow_block_device && input.expected_kernel_block_device && input.region_length_bytes);
        if (denied && input.path.ends_with("2")) return {std::nullopt, Support::permission_required, "denied"};
        LinuxFilesystemCapacity value; value.source_path = input.path; value.source_is_block_device = true;
        value.source_size_bytes = value.region_length_bytes = *input.region_length_bytes;
        value.declared_capacity_bytes = 512 * 1024; value.disk_inventory_id = {"testdisk", Support::estimated, "fixture"};
        value.kernel_block_device = {wrong_device ? "8:99" : *input.expected_kernel_block_device, Support::available, "fixture"};
        return {value, Support::estimated, "fixture"};
    };
    auto disk = read_linux_disk_filesystem_capacity_with_api("testdisk", api);
    assert(disk.value && disk.value->regions.size() == 2 && disk.value->declared_total_bytes.value == 1024 * 1024 && probes == 2);
    denied = true; disk = read_linux_disk_filesystem_capacity_with_api("testdisk", api);
    assert(disk.value && !disk.value->declared_total_bytes.value && disk.value->declared_total_bytes.support == Support::permission_required);
    denied = false; wrong_device = true; disk = read_linux_disk_filesystem_capacity_with_api("testdisk", api);
    assert(disk.value && !disk.value->declared_total_bytes.value); wrong_device = false;
    changed = true; assert(!read_linux_disk_filesystem_capacity_with_api("testdisk", api).value); changed = false;
    layout.regions[1].offset = 512 * 1024;
    disk = read_linux_disk_filesystem_capacity_with_api("testdisk", api); assert(disk.value && !disk.value->declared_total_bytes.value);
    layout.regions[1].device = layout.regions[0].device; probes = 0;
    assert(!read_linux_disk_filesystem_capacity_with_api("testdisk", api).value && probes == 0);
    assert(!read_linux_disk_filesystem_capacity("testdisk").value); // No opt-in, no raw probing.
    assert(!read_linux_disk_filesystem_capacity("../bad", true).value);
#ifndef __linux__
    target.path = "/tmp/nonexistent.img"; assert(!read_linux_filesystem_capacity(target).value);
    assert(!read_linux_disk_filesystem_capacity("testdisk", true).value);
#endif
    std::cout << "PASS ext-family capacity geometry, units, 64bit, CRC32C, unknown features, overflow and invalid targets\n";
}
