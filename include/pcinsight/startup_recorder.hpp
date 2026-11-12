#pragma once
#include "startup_store.hpp"
#include "startup_birth_impact.hpp"
#include <memory>
namespace pcinsight {
namespace detail {struct StartupRecorderAccess;}
enum class StartupRecorderState { idle, recording, closed, failed };
struct StartupRecorderOptions {
    std::string capture_id;
    std::chrono::milliseconds duration{60000},scan_interval{250};
    bool accept_estimated_gio_associations{false}; // Explicit acceptance of untrusted launch hints, NOT proof of autostart.
    // Privileged optional selected-process BPF. Never raises privileges itself.
    // Legacy name: also recovers CPU when a matching scheduler-clock baseline
    // succeeds. Otherwise CPU stays a last-observed lower bound, never mixed.
    bool enable_privileged_exit_disk_recovery{false};
    std::uint32_t max_exit_disk_captures{8}; // Concurrent bound, valid1..32 when enabled.
};
struct StartupRecorderReport {
    StartupRecorderState state{StartupRecorderState::idle};
    std::string capture_id,native_context;
    std::chrono::steady_clock::time_point started_at{},deadline{},next_scan{},finished_at{};
    std::chrono::system_clock::time_point recorded_at{};
    std::uint64_t scans{},partial_scans{},examined{},ignored{},unavailable{},registered{},rejected{};
    std::uint64_t poll_attempted{},poll_updated{},poll_failed{};
    std::uint64_t exit_disk_attempted{},exit_disk_attached{},exit_disk_skipped{},exit_disk_unavailable{},exit_disk_recovered{};
    std::uint64_t exit_disk_released_without_final{};
    ActionError error{ActionError::none};
    Metric<bool> automatic_startup_origin; // Always unknown in this GIO-hint provider.
    std::vector<StartupImpactObservation> observations;
    // Separate kernel identities/birth-relative windows. NEVER add these to the
    // legacy observations or interpret them as the recorder's global phase.
    std::optional<StartupBirthObserverReport> birth_capture;
    std::vector<StartupBirthImpactMeasurement> birth_impacts; // Explicitly bound rows only.
};
// Optional early-host controller. Construction never starts collection, installs
// startup entries or launches apps. Caller schedules poll; no hidden worker.
// Serializes calls; native I/O can block. One capture per instance, no resume.
class LinuxStartupRecorder {
public:
    LinuxStartupRecorder();
    ~LinuxStartupRecorder();
    LinuxStartupRecorder(LinuxStartupRecorder&&) noexcept;
    LinuxStartupRecorder& operator=(LinuxStartupRecorder&&) noexcept;
    LinuxStartupRecorder(const LinuxStartupRecorder&)=delete;
    LinuxStartupRecorder& operator=(const LinuxStartupRecorder&)=delete;
    ActionResult start(const StartupRecorderOptions&);
    ActionResult poll(); // Due scan + sample; one final sample at/past phase deadline, then close.
    // One selected creator per recorder, explicit opt-in while recording. No
    // privilege elevation, child launch, retroactive capture or inferred binding.
    ActionResult watch_births(bool explicitly_authorized,int borrowed_creator_pidfd,
        std::uint32_t creator_pid,std::uint64_t creator_start_ticks,const StartupBirthObserverOptions&);
    // Exact retained child key only; allowed after close, no hidden final poll.
    ActionResult associate_birth(const StartupBirthProcessKey&,const std::string& startup_entry_id);
    void close(); // No final I/O; closes partial observations, never silently saves.
    StartupRecorderReport report() const;
    // Closed success only. Explicit replace of valid historical archive, not
    // merge/checkpoint; corrupt/foreign existing file refused. Caller owns path.
    // Uses v3 when watch_births succeeded, preserving separate kernel identities,
    // per-birth window and thresholds. Otherwise v2. Not steady timestamps,
    // live resources or a resumable checkpoint. Old v1 remains readable.
    StartupArchiveIoResult save(const std::filesystem::path&) const;
private:
    friend struct detail::StartupRecorderAccess;
    struct Impl;
    explicit LinuxStartupRecorder(std::unique_ptr<Impl>);
    std::unique_ptr<Impl> impl_;
};
}
