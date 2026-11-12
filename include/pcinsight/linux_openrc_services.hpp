#pragma once
#include "linux_openrc.hpp"
#include "types.hpp"
namespace pcinsight {
struct LinuxOpenrcServiceInventory {
    Metric<LinuxOpenrcCatalog> catalog;
    Metric<LinuxOpenrcMarkerInventory> markers;
    // Separate index-aligned views. NO identity join between filenames and marks.
    std::vector<ServiceInfo> configured_services; // catalog.value->scripts[i]
    std::vector<ServiceInfo> marked_services; // markers.value->entries[i]
};
// Independent read-only observations, not an atomic combined/manager census.
// Distinct provider/ID namespaces; PID/group/effective enablement unknown.
Metric<LinuxOpenrcServiceInventory> query_linux_openrc_service_inventory(
    const std::string& absolute_init_directory,const std::string& absolute_runlevel_directory,
    const std::string& absolute_runtime_root,std::uint32_t max_entries_per_source=1024);
}
