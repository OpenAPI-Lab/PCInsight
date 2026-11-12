#pragma once
#include "types.hpp"
#include "actions.hpp"

namespace pcinsight {
enum class LinuxServiceScope { system, current_user };
struct LinuxServiceTarget {
    LinuxServiceScope scope{LinuxServiceScope::system};
    std::string name; // Canonical .service name, never a filename or command.
};
struct LinuxServiceInventory {
    LinuxServiceScope scope{LinuxServiceScope::system};
    // Current effective UID for current_user; absent for the system manager.
    std::optional<std::uint32_t> user_uid;
    std::string manager_owner; // Unique D-Bus owner for this observation only.
    std::vector<ServiceInfo> services;
};
struct LinuxServiceView {
    LinuxServiceTarget target;
    std::optional<std::uint32_t> user_uid;
    std::string manager_owner;
    ServiceInfo service;
};
// Read-only, explicit manager selection; no file-scan fallback, activation or shell.
Metric<LinuxServiceInventory> query_linux_service_inventory(LinuxServiceScope scope);
// "Open service" for an application's own details view, NOT an external editor.
// Only names observed in the inventory are resolved; unloaded aliases stay unresolved.
Metric<LinuxServiceView> open_linux_service(const LinuxServiceTarget& target);
// Explicit manager; never falls back from current_user to system or elevates.
// Success means this request's systemd job completed with result=done.
ActionResult control_linux_service(const LinuxServiceTarget& target, ServiceAction action);
}
