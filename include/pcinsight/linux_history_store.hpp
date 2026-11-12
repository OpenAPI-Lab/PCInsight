#pragma once
#include "history_store.hpp"
namespace pcinsight {
struct LinuxHistoryLoadResult : HistoryIoResult {
    std::vector<LinuxAppHistoryEntry> entries;
    std::uint32_t format_version{};
};
// Explicit portable persistence, separate from legacy AND Windows history files.
// Preserves each metric's presence/support/note. No automatic I/O or collector.
class LinuxHistoryStore {
public:
    static LinuxHistoryLoadResult load(const std::filesystem::path&);
    static HistoryIoResult save(const std::filesystem::path&,std::span<const LinuxAppHistoryEntry>);
    // Cumulative (id, period_start in milliseconds); no repeated-snapshot double counting.
    // Newer counters cannot decrease or discard a previously known value in the same period.
    // Output times normalized to milliseconds. Each input must have unique period keys.
    // Never merges independent collectors by guessing ownership or availability.
    static std::vector<LinuxAppHistoryEntry> merge(std::span<const LinuxAppHistoryEntry>,std::span<const LinuxAppHistoryEntry>);
    static std::vector<LinuxAppHistoryEntry> retain_since(std::span<const LinuxAppHistoryEntry>,std::chrono::system_clock::time_point);
    static HistoryIoResult prune(const std::filesystem::path&,std::chrono::system_clock::time_point);
    // Explicit unlink only; absent file succeeds, directories are never removed.
    static HistoryIoResult reset(const std::filesystem::path&);
};
}
