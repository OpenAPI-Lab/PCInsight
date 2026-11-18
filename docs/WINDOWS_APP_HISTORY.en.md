# Observed CPU history by Windows executable

Uses the existing opt-in `SampleOptions.include_app_history=true`. Windows
returns `SystemSnapshot.windows_app_history`, a Metric of WindowsAppHistoryEntry,
separately from the legacy app_history vector so that zeros are not introduced
as if network or notification metrics had been measured. The legacy vector
retains its Linux provider and stays empty on Windows. Linux does not run
this Win32 provider and reports windows_app_history as unsupported with a
platform note.

## What it measures

ProcessInfo.windows_cpu_time_100ns exposes cumulative kernel and user CPU
time from GetProcessTimes, measured since process creation, with explicit
support and permission status. This is CPU duration summed across threads,
not wall time or a percentage; it can exceed elapsed time in multithreaded
programs. Creation time and the counter come from the same handle, and the
sum is checked for overflow. Process text is now converted to UTF-8 strictly:
conversion errors do not silently introduce replacement characters into
paths used for grouping.

The history sums only **observed deltas** between consecutive samples with
the same PID, nonzero creation time, and exact executable path. The first
observation sets a baseline and records zero observed CPU; it does not
include CPU used before capture was enabled. PID reuse, path changes, a
decreasing counter, or a missing previous sample do not add a delta.
Unavailable permissions, counters, identities, or paths break continuity
for that PID. PID 0 is not counted as an application.

Visible processes from all accounts are grouped by exact observed path,
without normalizing case or deduplicating aliases, hard links, versions, or
commercial packages. The process name alone is not a fallback identity.
A file replaced at the same path remains in the same group. This is not a
security identity or AppUserModelID attribution. The provider does not count
every process that starts and exits between polls or reconstruct CPU time
during gaps or after losing access.

Each row contains id (path), name (first observed name), observed_cpu_time
as an estimated Metric<nanoseconds>, network_bytes and notifications
without values and with notes, and period_start/end. period_end is the
timestamp of the latest published sample, even if the app is no longer
visible. It does not prove continuous usage or when the app last ran.
Counters are retained while capture is active; rows for apps that disappear
remain with their accumulated values.

## Capture and resets

Windows collects processes internally when history is requested, even if
include_processes=false. In that case snapshot.processes stays empty.
Global CPU/RAM and other flags continue to work as before. Disabling
include_app_history clears this provider's baselines and totals; re-enabling
starts a new period. Process enumeration failure breaks continuity without
promising recovery of missing deltas. The provider does not write files,
launch processes, elevate permissions, or install dependencies. It does
not retrieve Windows history from before this Monitor instance or fully
replicate Task Manager's app history.

A sample rejected for duplicates, limits, a backward wall clock, or
overflow does not publish a partial report or increase totals; it clears
continuity. Earlier totals are retained, and the caller can opt out to
reset capture and start a new period. Do not turn a missing report into
zero CPU, network, or notifications.

## Limits and persistence

Up to 10,000 processes per sample, 4,096 retained paths, 16 MiB of input
text, and 4 MiB of retained identity text (including keys, IDs, and names).
Paths are limited to 32,768 bytes and names to 16,384 bytes. These are data
budgets, not total RAM limits or API deadlines. Accumulated duration must
fit in int64 nanoseconds; overflow is rejected. Transactional copies of
the map and copies of Sampler snapshots/history add memory overhead.
More frequent samples reduce some gaps but do not guarantee full coverage.

HistoryStore v1/v2 is unchanged: it accepts legacy AppHistoryEntry, **not**
this report with per-field availability. WindowsHistoryStore now persists
and merges the report in a separate format and file, preserving value,
support, and note and rejecting corrupt or legacy destinations. See the
[Windows history store guide](WINDOWS_HISTORY_STORE.en.md). Do not convert
the report to legacy and lose the distinction between absence and zero.
The application coordinates I/O. The common view added in A01,
app_history_view and AppHistorySample, preserves availability and
provenance and does not replace WindowsHistoryStore. LegacyAppHistoryEntry
is an explicit alias for the old payload. Importing v1/v2 does not
reconstruct known measurements; see [metric availability](METRIC_AVAILABILITY.en.md).
No network or notification collectors are added. The 0.x API was extended:
rebuild the library and its consumers, including those that use public
structures by value.

Official source: [GetProcessTimes: user/kernel times and 100 ns units](https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-getprocesstimes).

## Verification

2026-10-15: Release Windows 77/77 and Linux 57/57; packages and external
consumers were rebuilt and run with exit code 0 on both. Native Windows:
the test captured its own executable and observed an increase of 93,750,000 ns
(93.75 ms) in one pass. include_processes=false retains history and hides
the process vector; opting out clears the report, and re-enabling starts
with zero baselines. The workload is a short loop in the test itself; it
does not launch or modify other applications.

Portable tests cover 100 ns to nanosecond deltas, multiple instances per
path, PID reuse, disappearance, gaps, access denial, a decreasing counter,
path changes, missing identity, PID 0, path and size limits, duplicate
PIDs, a backward clock, and overflow. The Windows consumer requests
history without exposing processes and verifies CPU and the absence of
network/notification data. Linux tests the aggregator with fixtures and
its own provider for regressions, not native GetProcessTimes. HistoryStore
tests were retained without changing the v1/v2 format. Other hosts and
MSVC require more coverage.
