#pragma once
#include "pcinsight/linux_services.hpp"
#include "pcinsight/types.hpp"
#include <functional>

namespace pcinsight::detail {
struct SystemdUnit {
    std::string name, description, load, active, sub, object_path;
};
struct SystemdUnitFile { std::string path, state; };
struct SystemdServiceReader {
    std::function<std::vector<SystemdUnit>()> loaded;
    std::function<std::vector<SystemdUnitFile>()> installed;
    std::function<Metric<std::uint32_t>(const std::string&)> main_pid;
    std::function<Metric<std::string>(const std::string&)> slice, unit_id;
};
std::vector<ServiceInfo> read_systemd_services(const SystemdServiceReader& reader);
void apply_linux_service_file_metadata(ServiceInfo&,bool sysv);
// Absent if libsystemd/system bus cannot be opened; failures after opening throw.
std::optional<std::vector<ServiceInfo>> read_native_systemd_services();
Metric<LinuxServiceInventory> read_native_systemd_inventory(LinuxServiceScope scope);
Metric<LinuxServiceView> select_linux_service(const LinuxServiceTarget& target,
    const Metric<LinuxServiceInventory>& inventory);
}
