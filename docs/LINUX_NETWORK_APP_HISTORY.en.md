# Observed Linux network history by executable

Revision 100, 2026-10-24. H04 connects the optional F03 source to
`SystemSnapshot::linux_network_app_history` in `Monitor` and `Sampler`.
The report uses `Metric<vector<LinuxAppHistoryEntry>>` with accumulated
`network_bytes` and ABSENT CPU/notifications. It does not populate legacy
integer fields, run telemetry apps, install packages, or save files.

## Usage and independent periods

Both `include_app_history` and `include_linux_process_network` are required,
along with an F03 capture previously started with explicit consent. Use
`observed_namespace_buckets` in manual selection or the visible admission path.
A flag never authorizes or starts BPF. Strict and declared modes do not produce
this history; absence is explicit. For F03 privileges, kernel requirements,
coverage, and bounds, see [Linux network acceptance](LINUX_NETWORK_ACCEPTANCE.en.md).

```cpp
#include <pcinsight/monitor.hpp>
#include <pcinsight/app_history.hpp>
#include <pcinsight/linux_history_store.hpp>

pcinsight::Monitor monitor;
// First, the app requests consent and sets up selection/admissions.
// monitor.refresh_linux_visible_process_network(consent);
pcinsight::SampleOptions options;
options.include_app_history = true;
options.include_linux_process_network = true;
auto snapshot = monitor.sample(options); // Periodic caller-driven sample.
auto view = pcinsight::app_history_view(snapshot.linux_network_app_history);
if (pcinsight::metric_is_known(snapshot.linux_network_app_history)) {
    // Display each row's bytes/Support/notes; do not substitute 0 for absence.
    // Persist explicitly with load/merge/save under the caller's own lock.
    // See LINUX_HISTORY_STORE.en.md for a full example and error handling.
}
```

[`linux_app_history`](LINUX_APP_HISTORY.en.md) remains observed CPU history.
`app_history_view(snapshot)` selects that CPU report and does NOT mix the two.
Pass the network report explicitly to the overload. The GUI can display both
sources beside the same path without inventing a common period. Store CPU
and network in separate files. Do not `merge` them because path/start match:
independent periods can collide when normalized to milliseconds.

Rows group by exact absolute executable path across selected processes and
users. A path is not a package, brand, publisher, or permanent binary
identity. A replacement at the same path keeps the history row but breaks
the interval. No causal or exclusive attribution of shared sockets is claimed.
Bytes from unselected processes are not included.

## Identity and windows

Each call performs **one F03 sweep**. Previous metadata is reused to read
executables before the sweep, and new metadata is used after it. Each native
read verifies boot/observer/procfs/namespaces and PID/start/UID/inode with its
own pidfd before and after reading the pinned regular executable. It compares
the path and full stamp (device/inode/size/mode/mtime/ctime). The earlier CPU
process list is not treated as the boundary of the network sweep.

The first sample records actors without attributing a row. The second stable
sample establishes a per-executable baseline with bytes still unknown. The
third stable sample gives the first verified delta. New or recovered actors
must warm up again. Only the EXACT cumulative counter for each bucket/lane
present at both verified endpoints is subtracted; TCP send/receive and UDP
send/receive deltas are then summed. Rates are not integrated and snapshot
cumulative counters are not added together. Loopback send and receive both
count. Payload bytes do not equal wire bytes or all application traffic;
async/kernel/offload and other paths are excluded.

A new bucket does not contribute its initial bytes retroactively. An idle
actor can show zero for the instrumented subset after a verified interval,
without inventing cookie 0 or a synthetic bucket. A repeated snapshot does not
add bytes and breaks continuity. Backwards clocks, overflow, decreasing
counters, and inconsistent buckets reject the whole transaction.

A change in epoch/boot/producer/observer, PID reuse, executable changes,
denied metadata, or capture loss removes baselines, not confirmed totals.
Traffic during the gap cannot be recovered. Failure to read an executable
affects its attribution; it does not start alternate capture or raise
privileges. The original F03 frame remains available with its own coverage.
Selection changes require normal F03 consent; even retained actors lose
history continuity.

These are endpoint checks, not a trace of every exec. An exec followed by a
return to the same file (ABA) between reads can go undetected. Thus the
report and bytes are `estimated`, not an exact guarantee of application
identity throughout the interval. This does not certify all distros,
kernels, drivers, or traffic.

## Memory, shutdown, and persistence

F03 maxima: 1,024 actors, 16 buckets per actor and 16,384 total. History limits:
4,096 paths, 65,536 bytes per path, and 4 MiB of retained identities. Updates
are transactional: a global error keeps previous totals intact and discards
continuity, with no partial commit or uint64 wrap. Working copies are bounded;
there is no hard deadline.

Disabling `include_app_history` clears both this history and observed CPU
history. Stopping or disabling only network capture retains confirmed totals
but clears baselines. A new `Monitor` starts another period; pidfds, permissions,
and baselines are not restored from a file. Serialize Monitor calls. Sampler
uses its existing serialization and keeps reports in its window.

`LinuxHistoryStore` preserves rows, presence/Support/notes, and periods.
Replaying a cumulative row REPLACES it rather than adding to it. Decreases
or loss of known values are rejected. Only rows are saved, not outer status
or notes or F03 metadata. The caller chooses file, locking, frequency, and
retention. See [Linux history store](LINUX_HISTORY_STORE.en.md) for format,
bounds, privacy, and errors.

This changes the public 0.x layout. Rebuild the library and ALL consumers,
including the GUI even if it does not use CMake yet. This is not a new runtime
dependency.

## Evidence

`tests/linux_network_app_history.cpp` covers warmup, deltas, 50 persistence
replays, frame replay, exec/stamp/denial/changes during a sweep, PID/UID/start/
inode/boot/observer/producer, epoch changes, new cookies, idle zero, multiple
processes, interval and accumulation overflow, counter decreases, invalid
buckets/flags/joins, rollback, opt-out, and gaps. Release keeps assertions on.

`tests/linux_network_admission_native.cpp --authorized-private-admission`:
two private UDP children, real procfs/pidfds/BPF, the same Monitor/Sampler paths,
and selection limited to the children. Per-executable network totals went from
258 to 516 bytes; 50 replays kept 258, and real load/merge/save/load preserved
516 bytes. CPU/notifications were not promoted. Private temporary files and probes
were removed, the FD census returned to baseline, and children exited normally.
No global capture or new permission to access all of /proc. Revision 100 logs:
`work/linux-network-history-*`; final result in SEGUIMIENTO.txt.
