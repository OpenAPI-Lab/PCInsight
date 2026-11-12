#pragma once

#include "pcinsight/types.hpp"
#include <filesystem>
#include <span>
#include <string>
#include <vector>

namespace pcinsight {

struct HistoryIoResult {
    bool success{};
    std::string error;
    explicit operator bool() const { return success; }
};

struct HistoryLoadResult : HistoryIoResult {
    std::vector<LegacyAppHistoryEntry> entries;
    std::uint32_t format_version{};
};

// Legacy v1/v2 persistence ONLY. Counters have no availability flags; never
// promote them into measured telemetry automatically (see app_history.hpp).
class HistoryStore {
public:
    static HistoryLoadResult load(const std::filesystem::path& path);
    static HistoryIoResult save(const std::filesystem::path& path,
                                std::span<const LegacyAppHistoryEntry> entries);
    // Cumulative snapshots keyed by (id, period_start in milliseconds).
    // Repeated snapshots are idempotent. Different sessions remain separate rows.
    // Throws invalid_argument for invalid entries or decreasing counters.
    static std::vector<LegacyAppHistoryEntry> merge(std::span<const LegacyAppHistoryEntry> persisted,
                                               std::span<const LegacyAppHistoryEntry> current);
    static HistoryIoResult reset(const std::filesystem::path& path);
    // Keep whole periods ending at/after cutoff; no invented prorated counters.
    static std::vector<LegacyAppHistoryEntry> retain_since(
        std::span<const LegacyAppHistoryEntry> entries,
        std::chrono::system_clock::time_point cutoff);
    // Explicit file maintenance; failed loads leave the original untouched.
    static HistoryIoResult prune(const std::filesystem::path& path,
        std::chrono::system_clock::time_point cutoff);
    static HistoryIoResult migrate(const std::filesystem::path& path);
};

} // namespace pcinsight
