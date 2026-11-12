#pragma once
#include "pcinsight/types.hpp"
#include <span>

namespace pcinsight {
enum class AppHistoryOrigin { unspecified, linux_observed, windows_observed, legacy_unverified };
// Availability-aware presentation view. No implicit conversion to legacy storage.
// CPU duration (ns), network total (bytes), notifications (count); not rates.
struct AppHistorySample {
    std::string id, name;
    Metric<std::chrono::nanoseconds> cpu_time;
    Metric<std::uint64_t> network_bytes, notifications;
    std::chrono::system_clock::time_point period_start, period_end;
    AppHistoryOrigin origin{AppHistoryOrigin::unspecified};
};
Metric<std::vector<AppHistorySample>> app_history_view(const Metric<std::vector<LinuxAppHistoryEntry>>&);
Metric<std::vector<AppHistorySample>> app_history_view(const Metric<std::vector<WindowsAppHistoryEntry>>&);
// Selects this build's native CPU report only, never merges independent network
// periods/platforms/legacy data. For Linux network pass linux_network_app_history.
Metric<std::vector<AppHistorySample>> app_history_view(const SystemSnapshot&);
// Conservative import: all legacy counters remain UNKNOWN, including nonzero.
// Original numeric payload remains accessible through HistoryStore's raw entries.
Metric<std::vector<AppHistorySample>> import_legacy_history(std::span<const LegacyAppHistoryEntry>);
}
