#pragma once
#include "actions.hpp"

namespace pcinsight {
// Best-effort numeric TID workflow, NOT pidfd-protected. Tokens are local to the
// current boot/PID namespace and only mitigate, never eliminate, reuse races.
struct LinuxThreadPriorityChange {
    std::uint32_t tid{};
    std::uint64_t start_ticks{};
    int original_nice{}, original_io{}, requested_nice{}, requested_io{};
    // Successful writes still requiring explicit restoration. Preserve these
    // even on failure; never substitute set_efficiency_mode(pid, false).
    bool nice_pending{}, io_pending{};
};
struct LinuxEfficiencyReceipt {
    std::uint32_t process_id{};
    std::uint64_t process_start_ticks{};
    std::vector<LinuxThreadPriorityChange> threads;
};
struct LinuxEfficiencyUpdate {
    ActionResult result;
    LinuxEfficiencyReceipt receipt;
};
// Explicit opt-in required. Captures a bounded set of current threads, requests
// nice=max(original,10) and I/O idle, and verifies observations. New threads and
// descendants are NOT monitored. Errors may leave partial effects in receipt.
// No suspension, elevation, automatic rollback, energy guarantee or audit sink.
LinuxEfficiencyUpdate apply_linux_efficiency_unchecked(std::uint32_t pid,
    std::uint64_t expected_start_ticks, bool acknowledge_pid_reuse_risk = false);
// Use the returned receipt on retries (completed fields have pending=false).
// Refuses observed identity/priority conflicts; compare+write is NOT atomic.
// Exact originals, including I/O class NONE, are requested, never fixed defaults.
// A receipt is not authority or durable storage; do not accept untrusted input.
LinuxEfficiencyUpdate restore_linux_efficiency_unchecked(const LinuxEfficiencyReceipt&,
    bool acknowledge_pid_reuse_risk = false);
}
