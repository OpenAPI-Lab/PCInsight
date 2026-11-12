#pragma once
#include "linux_runit.hpp"
#include "types.hpp"
namespace pcinsight {
struct LinuxRunitServiceInventory {
    LinuxRunitInventory source;
    std::vector<ServiceInfo> services; // Same-call source.services index alignment.
};
// Explicit trusted tree, read only. reported-* labels are NOT kernel liveness.
// Raw PID stays in source; common pid/main_pid remain unknown (namespace unverified).
// Opaque IDs are scoped to this inventory root, NOT control/process tokens.
Metric<LinuxRunitServiceInventory> query_linux_runit_service_inventory(const std::string& absolute_root,std::uint32_t max_entries=1024);
}
