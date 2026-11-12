#pragma once
#include "history_store.hpp"
namespace pcinsight {
struct WindowsHistoryLoadResult : HistoryIoResult {
    std::vector<WindowsAppHistoryEntry> entries;
    std::uint32_t format_version{};
};
// Explicit portable persistence, separate from legacy HistoryStore files.
// Preserves each metric's presence/support/note. No automatic I/O or collector.
class WindowsHistoryStore {
public:
    static WindowsHistoryLoadResult load(const std::filesystem::path&);
    static HistoryIoResult save(const std::filesystem::path&,std::span<const WindowsAppHistoryEntry>);
    // Cumulative (id, period_start in milliseconds); no repeated-snapshot double counting.
    // Newer counters cannot decrease or discard a previously known value in the same period.
    static std::vector<WindowsAppHistoryEntry> merge(std::span<const WindowsAppHistoryEntry>,std::span<const WindowsAppHistoryEntry>);
    static std::vector<WindowsAppHistoryEntry> retain_since(std::span<const WindowsAppHistoryEntry>,std::chrono::system_clock::time_point);
    static HistoryIoResult prune(const std::filesystem::path&,std::chrono::system_clock::time_point);
    static HistoryIoResult reset(const std::filesystem::path&);
};
}
