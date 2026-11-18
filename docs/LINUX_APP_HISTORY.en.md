# Observed Linux CPU history (H01)

Functionally complete as of 2026-10-16. `SystemSnapshot::linux_app_history` is
a `Metric<vector<LinuxAppHistoryEntry>>` separate from legacy history. Each row
exposes `id`, `name`, `observed_cpu_time`, `network_bytes`, `notifications`,
and `period_start/period_end`. CPU is an accumulated duration in nanoseconds,
NOT a percentage or a 60-second graph. The Sampler window was completed later
under A03; see [Sampler window](SAMPLER_WINDOW.en.md). That window is not the
retention period for accumulated per-app history.

## Usage and lifecycle

```cpp
pcinsight::Monitor monitor;
pcinsight::SampleOptions options;
options.include_app_history = true;
options.include_processes = false; // Hides rows, not internal collection.
auto baseline = monitor.sample(options);
// Wait for the caller's chosen interval, then sample again.
auto snapshot = monitor.sample(options);
if (snapshot.linux_app_history.value) {
    for (const auto& app : *snapshot.linux_app_history.value) {
        if (app.observed_cpu_time.value) {
            auto ns = app.observed_cpu_time.value->count();
            // Use ns and retain support/note; do not substitute 0 for absence.
        }
    }
}
```

The caller serializes calls to the same Monitor; concurrent access is not
supported without external synchronization. Collection happens only while
`include_app_history=true`. Sampling with that option disabled clears totals,
fractional remainders, and baselines. Re-enabling it starts another period.
Constructing another Monitor also starts from scratch. There are no hidden
threads or automatic writes. Hiding the process list does not disable history;
enabling only processes/users does NOT accumulate history.

The first observation of a path produces a row with CPU absent, not a fabricated
zero. Two consecutive valid observations with a zero delta do publish an
estimated zero. A valid empty list means there were no eligible paths, not that
the report failed. Existing rows remain after an app exits. Replaying an
observation does not count time twice. Confirmed CPU time survives a capture gap.

## Identity, scope, and availability

Rows group by the exact absolute path observed through `/proc/PID/exe` across
visible users. They are not grouped by comm, package, or brand. No supposed
global identity is canonicalized across namespaces. `name` is the kernel name
from the first observation (or the path if unavailable), not a description or
publisher. Two different programs installed successively at the same path
share a row; the file change breaks that PID's interval, not creates a package
identity.

Each interval requires matching PID/start_ticks, nondecreasing utime and stime,
checked sums, a strictly increasing local steady clock, and matching executable
path and full file stamp. Stat and executable readings reuse D03's verified
readers and D04's delta helper. File metadata is compared, not a universal exec
generation. An exec of the same file or an ABA change between endpoints can go
undetected. There is no atomic snapshot of all processes and fields.

PID reuse, detected exec, individual counter reset, missing or denied stat/exe,
process disappearance, or a clock that does not advance prevents attribution of
that interval. The next eligible observation becomes the baseline. Processes
born and gone between samples, time before the first sample, and invisible
processes are not reconstructed. `Support::estimated` describes the observed
subset, not system-wide historical totals or one particular user's resources.

The collector adds utime and stime; guest time is already included in utime.
It does not add guest time again or add child cutime/cstime. Duration is not
normalized by core count, cgroup quota, affinity, or frequency. A valid CPU
context provides evidence of the boot and counter schema; no positive global
delta is required or used as a denominator.

`network_bytes` and `notifications` remain absent in THIS CPU report with an
explicit note. Lack of support is not a measured zero. H04 was completed in
revision 100 as the separate [linux_network_app_history report](LINUX_NETWORK_APP_HISTORY.en.md),
and H05 as a separate [notification observer](LINUX_NOTIFICATIONS.en.md). Matching
paths or hints do not join their periods. The Linux report is unavailable on
Windows, which retains its own `windows_app_history`.

## Exact conversion, errors, and bounds

A fractional remainder is kept per path: each interval contributes
`floor((delta_ticks * 1e9 + previous_remainder) / CLK_TCK)` nanoseconds and
retains the remainder modulo CLK_TCK. Thus three individual ticks at 3 Hz sum
to exactly one second. There is no division before multiplication or loss of
fractions between processes at the same path. The portable conversion needs
no uint128: it uses a checked integer part and nine decimal digits computed with
bounded modular arithmetic. Conversion and addition are checked against the
maximum `chrono::nanoseconds::rep`, with no wraparound or saturation.

CLK_TCK must be positive and constant. A change in boot (including whether its
token is present) or units requires explicitly restarting collection. Missing
or invalid context, a nonpositive or backwards wall clock, PID 0 or duplicates,
and oversized input reject the report with a note. Limits: 100,000 observations,
16,384 B per name, 65,536 B per path, 16 MiB of input text, 4,096 retained paths,
and 4 MiB of retained text. These bound memory and input, not overall procfs time.

Internal updates are transactional. A validation error or overflow returns an
absent, unsupported report, preserves confirmed rows and remainders, and breaks
baselines. No partial results are returned. A later valid call can expose the
confirmed totals again; it cannot recover the failed interval. Unexpected
errors propagate after continuity is broken. There is no atomic transaction
across all other Monitor groups: an exception in another provider does not
automatically make all its captures part of one transaction.

`period_start` is the capture timestamp of the Monitor's first accepted sample.
A row's `period_end` updates when it is created or an interval is accepted.
An absent app keeps its last endpoint. These are wall-clock capture times,
not exact bounds for CPU execution or a guarantee of continuous observation.

## Persistence and compatibility

Legacy `snapshot.app_history` remains empty on Linux. HistoryStore accepts
only legacy AppHistoryEntry and does NOT serialize this report or its support
flags. Do not turn unknown network/notifications into zero to save them there.
H02 remains complete for its legacy scope, not for a new Linux format. A caller
persisting this report chooses its file and preserves availability and notes.
Since revision 99, [LinuxHistoryStore](LINUX_HISTORY_STORE.en.md) preserves
these rows with their availability and notes without converting absence to zero.
Since revision 100, H04 integrates F03 into an independent report and period:
save CPU and network in separate files and do not merge them by path. A01 was
completed later with the common AppHistorySample view, an explicit legacy alias,
and conservative import; see [metric availability](METRIC_AVAILABILITY.en.md).
A05/API 1.0 and the final N03 audit remain open; the view does not create a new
Linux storage format.

New public 0.x layout: LinuxAppHistoryEntry and a field at the end of
SystemSnapshot. Rebuild the library AND ALL consumers with the new installed
headers. Do not mix old objects with the new library. No stable ABI is promised.

## Evidence

`tests/linux_app_history.cpp` covers 3 Hz/7 Hz/LONG_MAX fractions, multiple
instances, replay, PID/start/file/path, permission and absence, backwards
individual counters and clocks, gaps, reset, context/units/boot, duplicates and
bounds, 4,096 paths, and conversion/accumulation overflow without partial commit.
Assertions remain active in Release. Native WSL integration loaded only its own
process: first observed row PID 2123, CPU 130000000 ns. The result hid process
rows and showed positive CPU, with network and notifications absent.
Disabling and re-enabling reset the period. Monitor reads visible processes,
but the test does not modify or generate load in unrelated processes.

Final regression: Linux 78/78 and Windows 98/98 Release; packages reinstalled
and external consumers rebuilt and run to exit 0. This does not validate other
kernels, distros, or hardware, or resolve the previous isolated absence of
global CPU recorded in N03. No external utilities, new runtime dependencies,
or permission changes.
