# Core startup impact measurement

Subtask verified on 2026-10-14: read counters from an identified process
and classify their deltas between two samples. The API in startup_impact.hpp
provides read_startup_process_sample(pid,linux_start_ticks) and
measure_startup_impact(id,before,after,policy). There is no automatic startup
observer or fabricated association by name or Exec. The caller supplies
the startup_entry_id-to-process relation and defines the window.
Monitor.startup_entries.impact remains not_measured until full correlation
and observation are integrated; this separate API does not change it.

The later StartupImpactObserver adds incremental observation with explicit
associations and no hidden scheduler; see [startup observer](STARTUP_OBSERVER.en.md).

```cpp
#include <pcinsight/startup_impact.hpp>
auto before = pcinsight::read_startup_process_sample(pid, start_ticks);
// Later, at the boundary chosen by the caller; no hidden library sleep.
auto after = pcinsight::read_startup_process_sample(pid, start_ticks);
if (before && after) {
    auto measured = pcinsight::measure_startup_impact(entry_id, *before.value, *after.value);
    // measured.value->impact is a Metric<StartupImpact>; do not turn unknown into low.
}
```

## Native Linux reads

The reader requires genuine procfs (fstatfs) and a mount PID namespace
compatible with the caller's getpid. It opens the `/proc/<PID>` descriptor
before stat/io/stat and does not look for another process if it exits.
PID and start_ticks from both reads must match the expected ProcessInfo
token. Disappearing or malformed counters or changed identity yield explicit
errors. Invalid PID and zero token are rejected. No signals, pidfd actions,
CLI, or elevation are involved.

Sample context consists of the kernel boot_id and the PID namespace's dev
and inode. Before and after must match in context, PID, and start_ticks.
steady_clock timepoints come from the same context and execution; they are
not portable timestamps to persist. Tokens have tick resolution and are not
global or authenticated identities, or proof against a caller fabricating
samples. Do not use them to authorize destructive actions.

CPU uses the process's utime+stime, without child cutime/cstime or double
counting guest_time. Ticks are converted through _SC_CLK_TCK with overflow
checks, and quantization remains reflected as estimated. The module does
not aggregate separate child CPU. Disk uses read_bytes+write_bytes from
/proc/PID/io, without rchar/wchar or subtraction of cancelled_write_bytes.
Kernel accounting can include I/O from waited-for children, not all live
children or exact physical transfers or latency. Page cache, writeback,
and the filesystem affect its meaning. This does not measure network,
memory, readiness, or login delay.

Protected I/O yields an absent/permission_required Metric while readable
CPU is retained. Missing permission does not imply zero. Individual reads
are limited to 64 KiB. CPU and I/O are not an atomic snapshot: the final
CPU read follows I/O, and the timestamp is taken at the end. Intervals and
deltas are bounded observations with skew, not perfect integration. No
worker, daemon, or sleep is created. Procfs, filesystems, and syscalls can
block without a universal read deadline. Counters are not recovered after
exit, and automatic correlation with Exec or a startup cgroup is not
validated.

## Portable classification

The policy is configurable and inspired by
[Microsoft's criteria](https://learn.microsoft.com/en-us/windows/compatibility/startup-apps),
without claiming equality with Windows Task Manager. Explicit SI defaults:
medium CPU 300 ms or disk 300,000 bytes; high CPU 1 s or disk 3,000,000 bytes.
High applies if a known delta exceeds its high threshold. With both
counters known, medium applies if either reaches medium; otherwise low.
Exactly the high threshold is still medium. A known high resource is
enough even when the other is unknown; the other cannot reduce it.
Without both counters and without a known high result, classification
is unknown, not low or medium. Known zeros for both are low, not none;
disabled or no app is a different concept.

All impact results are estimated and describe only that window and process
scope. A counter decrease or reset yields an unknown delta, never a negative,
clamped, or wrapped unsigned difference. Unsupported or permission_required
values are not used even if they contain a number. Changed identity or
context yields stale_target. Empty, NUL-containing, or oversized IDs,
negative CPU, nonincreasing chronology or interval overflow, and invalid
policies yield invalid_argument. The startup ID is opaque and caller-supplied;
the file's current existence is not checked. The delta is not added to
histories or saved or merged automatically. Avoid double-counting
overlapping intervals.

## Verification and remaining scope

Linux Release passed 28/28 and Windows Release passed 39/39. The new
header was installed, and external consumers were built and run.
Windows checks the portable algorithm and the unsupported Linux reader,
not a native Windows impact provider. A private Linux /tmp fixture forks
a C++ child, obtains identity from Monitor, samples a baseline, runs
1.1 s of the child's CPU work and 4 MiB of writes/fsync, then takes a
second sample and classifies it high. An incorrect token and a reaped
process are rejected. Only the test child is terminated and reaped;
no real autostart entry or app was run or changed. Tests also cover
limits, exact thresholds, permissions and unknown, reset, mismatched
PID/token/boot, invalid policy/CPU/chronology, and interval overflow.

Complete startup impact still requires reliable correlation of apps,
processes, and descendants; an early login or boot observer and window
definition; capture of short-lived processes before exit; aggregation
without double counting; persistence and retention; and explicit inventory
linking. This subtask closes the measurement core, not the startup parent
task or total project-wide startup impact.

Later update: [startup overlay](STARTUP_OVERLAY.en.md) handles explicit linking,
[child exit](STARTUP_CHILD_EXIT.en.md) captures final counters for unreaped
children, and [startup store](STARTUP_STORE.en.md) saves and loads closed
historical captures. Universal early recording, reliable origin,
descendants, aggregation, and automatic maintenance or retention remain open.
