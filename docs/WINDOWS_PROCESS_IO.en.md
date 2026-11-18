# General Windows process I/O

Revision 57, M02. `Monitor` populates `ProcessInfo.windows_io` through
GetProcessIoCounters and calculates rates between samples for the same process
identity. It runs no CLI, installs no components, and changes no privileges.
This unit does not implement physical disk or process-attributed network
usage. It **does not populate** disk_bytes_per_second or
network_bytes_per_second with these values.

## Data and units

The new Metric<WindowsProcessIoDetails> contains:

- counters: cumulative read_operations/write_operations/other_operations and
  read_transfer_bytes/write_transfer_bytes/other_transfer_bytes. All six retain
  the process's full uint64 values. Child processes and job objects are not added.
- read_bytes_per_second/write_bytes_per_second/other_bytes_per_second:
  estimated Metric<double> values, calculated by dividing each byte delta by
  monotonic elapsed seconds. These are floating-point approximations, not
  exact integers or cumulative bytes. The raw counters preserve precision above 2^53.
- interval_seconds: an estimated Metric<double> with the process's own window
  for those three rates. It does not reuse a global CPU or disk interval or
  normalize by processor count.

The outer metric can be available with valid counters even if the first
sample's rates are unknown. Inspect each Metric. A valid zero delta yields
a known zero rate, not absence. Missing or denied reads do not yield zero
counters. Linux and other backends leave windows_io unsupported and absent.

These counters cover **all process I/O** and can include files, networking,
and devices. They do not represent bytes persisted to disk, storage space,
latency, physical disk IOPS, or traffic exclusive to one interface. Disk and
network cannot be separated by subtraction. A private pipe alone can
increase these counters without generating disk I/O. Label them as general
I/O in the GUI; do not add them to other providers as if each were a disjoint
part of physical I/O.

Primary sources: [GetProcessIoCounters and permissions](https://learn.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-getprocessiocounters),
[IO_COUNTERS fields and units](https://learn.microsoft.com/en-us/windows/win32/api/winnt/ns-winnt-io_counters).

## Identity, errors, and lifecycle

The reader uses the real handle already opened by the collector. It does not
reopen by name or PID to query another instance. It verifies PID, exact
FILETIME creation time, and absence of an exit time before and after reading.
Without validated creation identity it publishes no I/O. The existing guard
against processes born after snapshot start remains active.

Two native reads with differently initialized buffers detect unwritten or
partially written output and regression in any of the six counters. Output
indistinguishable from its initialization pattern is conservatively rejected:
an actual value matching 0xa5a5... in the first read or 0x5a5a... in the second
could also become unknown. Windows does not reserve those numbers as
sentinels. Tests retain zero, the maximum uint64, and other large values.
API success does not turn ambiguous output into zero.

ACCESS_DENIED yields permission_required; other errors, changed identity,
or ambiguous output yield unsupported with a reason. Win32 failure with
error zero is normalized to GEN_FAILURE, not accepted as valid identity.
The reader does not enable SeDebugPrivilege, a helper, or UAC elevation. It
borrows process handles without acquiring or closing them; the collector
retains its existing RAII cleanup. There is no guaranteed maximum duration
or atomic transaction across all metadata and processes.

The tracker requires two consecutive readable samples with matching PID and
creation time, increasing timestamps, and no decrease in any of the six
counters. PID reuse, first observation, loss or denial, a decreasing counter,
or nonpositive elapsed time leaves rates unknown. The current raw counters
can still be known. A per-process read failure does not invalidate other
processes. The table is replaced on each capture so departed or inaccessible
processes are not retained. It is capped at 100,000 entries; duplicates or
excess entries invalidate the tracker capture and reset its baseline. PID 0
can appear with an unknown result, never an invented identity for measuring Idle.

Disabling **internal** process collection clears the tracker. Re-enabling
requires a fresh baseline. include_processes=false only guarantees no
process rows are returned. User or CPU history collection can still require
the shared internal capture. This unit adds no I/O accounting to those
aggregates or histories and does not treat it as session or app disk/network data.

## Compatibility and verification

WindowsProcessIoCounters, WindowsProcessIoDetails, and ProcessInfo.windows_io
are added. **The public ProcessInfo layout and stride change in 0.x.** Rebuild
the library and ALL consumers; do not mix older binaries. Vtables and
persistence formats are unchanged, and API/ABI 1.0 is not declared. Sampler
snapshots can carry the field through the common type without converting it
to a cumulative AppHistory counter.

Release results: Windows MinGW 132/132, MSVC 132/132, and Linux 112/112.
Tracker tests are portable; the native reader and probe run only on Windows.
Each Windows toolchain ran 20 repetitions. Three installed external consumers
were rebuilt, relinked, and run, checking the new field and ABI. Linux checks
that windows_io is not fabricated. MSVC retains the expected /UNDEBUG D9025.
A full rebuild also exposed C4312 in the volume fixture. Its int-to-HANDLE
conversion was corrected through uintptr_t without changing simulated
values 2/3, the native source, or the assertions. The initial log was kept.
Logs under work/process-io-* cover builds, suites, repeats, installation, and
consumers. The first patch did not match the CMake block and was rejected
in full without partial changes; this was not a runtime failure.

Tests cover baselines, changed PID or creation time, equal or backwards time
and extreme timestamps without duration overflow, zero, raw values above
2^53 and uint64 maximum, errors/denial/missing API/unwritten or partial output,
process exit or change around a read, counter regression, disappearance/reset,
duplicates, and recovery. A probe uses the caller's own process handle with
QUERY_LIMITED_INFORMATION and a private 1,024 B pipe: both Windows toolchains
observed a read delta of 1,024 and a write delta of 1,024. Monitor's first
sample, subsequent rates, and off/on cycle were checked; disk and network
metrics remain absent. Only private handles were used, with no new files,
installation, modification of unrelated processes, or services.

General I/O is complete; **M02 remains open** for disk- and network-specific
metrics, attribution, and remaining coverage. M04 still has global-source
and hardware work pending. The revision 56 experimental AMD query is unchanged
in scope.
