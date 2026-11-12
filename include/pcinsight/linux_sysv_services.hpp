#pragma once
#include "linux_sysv.hpp"
#include "types.hpp"
namespace pcinsight {
struct LinuxSysVServiceInventory {
    LinuxSysVDescribedCatalog source; // Scope/evidence/budget, not a manager identity.
    std::vector<ServiceInfo> services; // Aligned with source.catalog.scripts, same call only.
};
// Explicit read-only SysV candidate projection; no manager detection or control.
// IDs are opaque, provider-prefixed filename bytes, scoped to THIS inventory's roots.
// No PID/running/enabled/group inferred from files or runlevel link names.
Metric<LinuxSysVServiceInventory> query_linux_sysv_service_inventory(
    const std::string& absolute_init_directory,const std::string& absolute_runlevel_root,
    std::uint32_t max_entries=1024,std::uint64_t max_prefix_bytes=8*1024*1024);
}
