#pragma once
#include "startup_observer.hpp"
#include "startup_birth_impact.hpp"
#include <filesystem>
namespace pcinsight {
// Historical recorder diagnostics, NOT global discovery completeness or a
// resumable checkpoint. Absent in legacy files: never interpret absence as0.
struct StartupCaptureDiagnostics {
    std::chrono::milliseconds requested_duration{},scan_interval{};
    std::chrono::nanoseconds elapsed{};
    bool exit_disk_enabled{};
    std::uint32_t max_exit_disk_captures{};
    std::uint64_t scans{},partial_scans{},examined{},ignored{},unavailable{},registered{},rejected{};
    std::uint64_t poll_attempted{},poll_updated{},poll_failed{};
    std::uint64_t exit_disk_attempted{},exit_disk_attached{},exit_disk_skipped{},exit_disk_unavailable{},exit_disk_recovered{};
    std::uint64_t exit_disk_released_without_final{}; // Includes alive-at-close; not proof an exit was missed.
    Metric<bool> automatic_startup_origin;
};
// Historical data, never a resumable capture or process-control authorization.
struct StartupBirthArchive {
    StartupBirthObserverReport capture;
    std::chrono::nanoseconds window{std::chrono::seconds(60)}; // Per birth, not global phase.
    StartupImpactPolicy policy{}; // Preserve thresholds for explicit reprojection.
};
struct StartupImpactArchive {
    std::string capture_id; // caller-defined capture identity, not authentication
    std::string native_context;
    std::chrono::system_clock::time_point recorded_at;
    std::vector<StartupImpactObservation> observations; // closed records only
    std::optional<StartupCaptureDiagnostics> diagnostics; // v2; absent in v1.
    std::optional<StartupBirthArchive> births; // v3; absent/UNKNOWN in v1/v2.
};
struct StartupArchiveIoResult {
    ActionError error{ActionError::none};
    bool replaced{}; // save: destination replaced, even if directory sync failed
    std::string message;
    explicit operator bool() const {return error==ActionError::none;}
};
struct StartupArchiveLoadResult {
    std::optional<StartupImpactArchive> archive;
    ActionError error{ActionError::none};
    std::string message;
    explicit operator bool() const {return error==ActionError::none && archive.has_value();}
};
// Explicit historical storage, not an observer checkpoint or automatic recorder.
// Save replaces one capture; callers serialize writes and choose private paths.
class StartupImpactStore {
public:
    static StartupArchiveLoadResult load(const std::filesystem::path& path);
    static StartupArchiveIoResult save(const std::filesystem::path& path,const StartupImpactArchive& archive);
};
}
