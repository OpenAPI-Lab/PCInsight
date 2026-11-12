#pragma once
#include "actions.hpp"
#include "types.hpp"
namespace pcinsight {
enum class StartupCpuCounter { linux_procfs_ticks,linux_scheduler_runtime };
struct StartupProcessSample {
    std::uint32_t pid{};
    std::uint64_t linux_start_ticks{};
    std::string native_context; // boot + PID namespace, NOT a global/auth identity
    std::chrono::steady_clock::time_point sampled_at;
    Metric<std::chrono::nanoseconds> cumulative_cpu_time;
    Metric<std::uint64_t> cumulative_disk_bytes;
    StartupCpuCounter cpu_counter{StartupCpuCounter::linux_procfs_ticks};
};
struct StartupProcessSampleResult {
    std::optional<StartupProcessSample> value;
    ActionError error{ActionError::none};
    std::string message;
    explicit operator bool() const {return value.has_value() && error==ActionError::none;}
};
// Read-only Linux counters for a sampled PID+start_ticks. No process-tree inference.
StartupProcessSampleResult read_startup_process_sample(std::uint32_t pid,std::uint64_t expected_linux_start_ticks);
// Linux POSIX process CPU clock (scheduler-runtime ns), pidfd-alive/identity
// bracketed. Same group counter as the optional final-release provider, NOT
// procfs ticks. Missing clock/permission stays unknown; no external utility.
StartupProcessSampleResult read_startup_scheduler_process_sample(std::uint32_t pid,std::uint64_t expected_linux_start_ticks);
struct StartupChildExitSampleResult {
    bool exited{};
    std::optional<StartupProcessSample> sample;
    std::optional<int> exit_code,termination_signal;
    ActionError error{ActionError::none};
    std::string message;
    // Success can mean still running: inspect exited/sample, not bool alone.
    explicit operator bool() const {return error==ActionError::none;}
};
// Linux 5.4+: nonblocking, non-reaping final sample of a caller's child.
// Caller must coordinate other reapers/SIGCHLD and eventually waitpid itself.
// Does not launch, signal, classify, or recover an already reaped process.
StartupChildExitSampleResult read_startup_child_exit_sample(std::uint32_t pid,
    std::uint64_t expected_linux_start_ticks);
struct StartupImpactPolicy {
    std::chrono::nanoseconds medium_cpu{std::chrono::milliseconds(300)};
    std::chrono::nanoseconds high_cpu{std::chrono::seconds(1)};
    std::uint64_t medium_disk_bytes{300000},high_disk_bytes{3000000}; // explicit SI
};
struct StartupImpactMeasurement {
    std::string startup_entry_id; // association supplied explicitly by caller
    std::uint32_t pid{};
    std::uint64_t linux_start_ticks{};
    std::chrono::steady_clock::duration observation_interval{};
    Metric<std::chrono::nanoseconds> cpu_time;
    Metric<std::uint64_t> disk_bytes;
    Metric<StartupImpact> impact;
};
struct StartupImpactMeasurementResult {
    std::optional<StartupImpactMeasurement> value;
    ActionError error{ActionError::none};
    std::string message;
    explicit operator bool() const {return value.has_value() && error==ActionError::none;}
};
// Classifies ONLY deltas between these samples. Does not certify startup/PID
// association, initialization completion, complete boot coverage, or Windows parity.
// Does NOT update Monitor.startup_entries automatically; caller manages its record.
StartupImpactMeasurementResult measure_startup_impact(const std::string& startup_entry_id,
    const StartupProcessSample& before,const StartupProcessSample& after,
    const StartupImpactPolicy& policy={});
}
