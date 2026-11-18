# Windows processes: initial implementation

Revision 60: [ETW decoder and private disk test](WINDOWS_DISK_EVENTS.en.md).
A controlled positive result for one thread owned by the test, NOT a general
per-PID collector or new Monitor metric. ProcessInfo disk and network data
still require their own providers.

Revision 57: [general per-process I/O](WINDOWS_PROCESS_IO.en.md) completes
ProcessInfo.windows_io with six cumulative counters and three rates, with
its own identity and interval. It does not replace the pending disk- and
network-specific metrics. The 0.x ProcessInfo layout changed: rebuild every
consumer. Shared internal collection can continue for user or history reports
without exposing process rows; read the historical statements about disabling
collection with that qualification. This revision adds no per-user or per-app
I/O aggregation.

Updated 2026-10-13. Native Toolhelp enumeration without subprocesses,
additional installation, process termination, or privilege changes.

## Available data

- PID, parent PID, name, and thread count from the Toolhelp snapshot.
- windows_session_id: optional Terminal Services ID from the primary token,
  with no PID-based fallback. It is used with account and creation time for
  visible per-session sums. See [user resource usage](WINDOWS_USER_RESOURCES.en.md).
  It is not a persistent identity for actions.
- Whether EcoQoS policy is available and requested, when Windows permits
  querying it. See [EcoQoS efficiency control](WINDOWS_EFFICIENCY.en.md) for
  semantics and the action.
- `windows_creation_time_100ns`: native creation timestamp when GetProcessTimes
  is accessible and the process did not start after the snapshot began.
  Keep it with the PID for `terminate_process_checked`. Do not round it or
  convert it to Unix time before passing it to that API. Without it, verified
  termination is unavailable.
- Executable path and architecture through Windows APIs. If `IsWow64Process2`
  is absent or does not recognize the type, architecture is empty.
- Description from the VERSIONINFO `FileDescription` resource, converted to
  UTF-8. The reader checks declared languages and encodings until it finds a
  valid string; preference for the GUI language is not guaranteed. An absent
  or invalid resource or inaccessible file leaves the field empty. This is
  unverified metadata, not proof of signature, publisher, or program identity.
- User in domain/name form from the process token. Domain-account resolution
  can depend on connectivity and has no separate maximum duration here.
- Resident memory (working set), including shared pages. It is not the private
  working set and cannot be summed across processes without double counting.
- CPU: kernel plus user time delta divided by monotonic elapsed time and the
  number of logical processors. A first sample, counter regression, CPU-count
  change, or reused PID yields no percentage. A valid zero delta yields 0%.
  This is not frequency-weighted utility.

`active` means the process had not exited when its times were queried. It
does not establish that its threads were executing at that moment. Other
results are `exited` and `unknown`. Suspension and an unresponsive GUI are
not yet detected.

## Consistency and errors

Every delta requires the same PID and creation timestamp. Measurements are
discarded if the opened process started after the snapshot began, avoiding
mixing old metadata with a newly reused PID. Snapshots are not atomic:
processes can exit during reads.

Inaccessible processes remain in the basic inventory, with absent metrics
and `permission_required` when Windows reports access denied. Unavailable
metadata strings are empty; the public API does not expose per-string status.
SYSTEM is not inferred from a PID.

A global Toolhelp failure throws `std::system_error`, rather than reporting
an empty or partial list as successful. The `Monitor` caller must handle
the exception; `Sampler` already catches errors. Enumeration finishes before
individual processes are opened. It validates record size, NUL termination
of names, and PID uniqueness, with a limit of 100,000 records. An invalid
record, excess count, or intermediate failure discards the whole list and
resets caches and deltas. The snapshot closes before metric reads.

Injected Toolhelp tests cover creation, first-read and next-read failures,
empty snapshots, null handles, malformed records, names without NUL, duplicate
PIDs, exact and exceeded limits, handle closure, and subsequent recovery.
These tests do not open, modify, or terminate real processes. Later metadata
and metric queries still need broader failure injection.

In the original implementation, `include_processes=false` skips enumeration
and clears process deltas, subject to the shared collection qualification
above. Re-enabling requires another sample before percentages are available.
Disappeared or inaccessible PIDs are removed from the cache.

Descriptions are cached by path while that path still appears among accessible
processes. This avoids rereading resources every sample but can retain old
metadata after replacement on disk. Recreating Monitor or disabling and
re-enabling processes invalidates the cache. Only absolute paths, resources
up to 4 MiB, and text up to 32,768 characters are accepted. The described
program is not loaded or executed.

## Pending

Detailed thread and suspension state, combined efficiency-mode priority,
and process-attributed disk and network I/O. EcoQoS implements the energy
component. Generic I/O counters are not a substitute for disk because they
can include other devices. Run new task uses CreateProcessW with injected
tests; see [Windows launch](WINDOWS_LAUNCH.en.md) for paths, arguments, and limits.
Forced End task is implemented and tested with injection; see
[Windows termination](WINDOWS_TERMINATE.en.md). The checked variant compares
PID and creation time; the older PID-only variant retains reuse risk.
Cooperative shutdown is still pending.

## Verification

A VERSIONINFO resource compiled into the test executable has no description
in the first language and an accented description in the second. Tests check
selection of the second language, UTF-8 conversion, stability across samples,
and rejection of empty, relative, and nonexistent paths. At this checkpoint,
Windows passed 19/19 and Linux WSL 6/6. CMake installation and an external
consumer were verified with the native `version` dependency.

Integration finds the test process itself and checks identity, user, executable,
memory, threads, subsequent CPU data, and disabling and re-enabling collection.
Delta fixtures cover PID reuse, topology changes, nonpositive time, counter
regression, valid zero, values above 100%, and unknown CPU. No real process
is modified or terminated. Validation of protected processes, domains, MSVC,
and other architectures remained pending in this original test record.

References: [process enumeration](https://learn.microsoft.com/en-us/windows/win32/procthread/process-enumeration),
[working set](https://learn.microsoft.com/en-us/windows/win32/procthread/process-working-set),
and [native process functions](https://learn.microsoft.com/en-us/windows/win32/procthread/process-and-thread-functions).
