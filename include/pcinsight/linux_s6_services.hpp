#pragma once
#include "linux_s6.hpp"
#include "types.hpp"
namespace pcinsight {
struct LinuxS6ServiceInventory {
    LinuxS6Inventory source;
    std::vector<ServiceInfo> services; // Same-call source.entries index alignment.
};
// Explicit trusted scan tree; historical record-* display, NOT current life.
// PID/wait/ready/control-type/logger relation remain in source, not action tokens.
// Common pid/main_pid unknown; IDs opaque and scoped to this inventory root.
Metric<LinuxS6ServiceInventory> query_linux_s6_service_inventory(const std::string& absolute_scan_root,std::uint32_t max_entries=1024);
}
