#pragma once
#include "types.hpp"

namespace pcinsight {
enum class LinuxDisconnectProvider { utmp, login1 };

// Static capability of PCInsight's built-in Linux inventory/control adapters,
// NOT a live session query, remote-transport probe, permission check or action.
// Both recognized adapters currently return known false with a provider reason
// on Linux. Unknown enum values/non-Linux builds return no value, not false.
// Can be queried even when the session inventory is empty or disabled.
// Does not contact a bus, inspect processes, signal, lock, logout or elevate.
Metric<bool> query_linux_disconnect_capability(LinuxDisconnectProvider provider);
}
