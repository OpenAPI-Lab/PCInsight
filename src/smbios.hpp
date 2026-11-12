#pragma once

#include "pcinsight/types.hpp"
#include <cstdint>
#include <optional>
#include <string>
#include <span>

namespace pcinsight::detail {

struct SmbiosInventory {
    Support support{Support::unsupported};
    std::string note;
    std::optional<std::uint64_t> installed_memory_bytes;
    std::optional<std::uint64_t> memory_speed_mts;
    std::optional<std::uint64_t> slots_used, slots_total;
    std::optional<std::string> memory_type, memory_form_factor;
    std::optional<double> firmware_cpu_ghz;
    std::optional<std::string> cpu_vendor,cpu_model;
};

template<class T> Metric<T> smbios_metric(const SmbiosInventory& inventory,
                                         const std::optional<T>& value) {
    auto support=inventory.support;
    if (!value && support!=Support::permission_required) support=Support::unsupported;
    return {value, support, inventory.note};
}

// Parses the raw SMBIOS structure table directly. No dmidecode or subprocesses.
SmbiosInventory read_smbios(const std::string& table_path = "/sys/firmware/dmi/tables/DMI");
SmbiosInventory parse_smbios(std::span<const unsigned char> table);
// Validate the Windows RawSMBIOSData wrapper before parsing the structure table.
SmbiosInventory parse_windows_smbios(std::span<const unsigned char> buffer);
#ifdef _WIN32
SmbiosInventory read_windows_smbios();
#endif

} // namespace pcinsight::detail
