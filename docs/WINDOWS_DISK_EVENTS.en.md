# ETW disk events and private attribution test

Revision 62 adds the [init/IRP decoder and correlator](WINDOWS_DISK_CORRELATION.en.md).
It was tested with injected data, without a new capture. The positive private
test from revision 60 does not validate the new path natively.

Revision 61 adds the [temporal identity index](WINDOWS_TRACE_IDENTITY.en.md),
with portable PID/TID reuse tests. At that point there was no producer for
lifecycle or I/O initiation events and no IRP correlation: do not attribute
operations using their completion time.

Revision 60, M02, 2026-10-21. Continues the temporary experiment authorized
in revisions 58/59. [Session control and lifecycle](WINDOWS_DISK_TRACE_LIFECYCLE.en.md).
This block does NOT integrate per-process disk/network metrics into Monitor or
enable capture by default. It changes no public API, layouts, vtables, or persistence.

## Native decoding

src/windows_disk_events.hpp/.cpp filters for the documented DiskIo GUID
and Read (10)/Write (11) opcodes. It supports only classic version 3, with
exactly one 32-bit or 64-bit pointer flag, a present payload, and a positive
timestamp. It does not interpret WPP events or guess earlier formats or
versions; it returns invalid/unsupported. Other GUIDs or opcodes are ignored.
The caller guarantees a QPC clock through session ClientContext 1 and consumer
RAW_TIMESTAMP. The decoder itself does not authenticate the origin or clock
of an EVENT_RECORD supplied by another caller.

It reads IssuingThreadId, TransferSize, and DiskNumber by name through
TdhGetPropertySize/TdhGetProperty, requiring four bytes for each. It uses
no overlays or manual offsets tied to FileObject/IRP width. Two reads of the
immutable payload with different seeds must agree. No write, partial writes,
and mismatches are errors; no uint32 value is reserved as a seed. A zero
TransferSize is valid, not missing information. TID 0/UINT32max and disk
UINT32max are not attributed. Header PID/ThreadId are NOT treated as the
issuer of the disk operation.

The optional required_thread filter discards other issuers before reading
TransferSize/DiskNumber. It can also discard an issuer that cannot be
attributed. This reduces transient data but does NOT establish complete
coverage of all the process's I/O. Missing schemas, TDH, or functions do
not yield known zero counters. TDH is a native Windows library, not a tool
or package installed by PCInsight. CMake links tdh only on Windows and
preserves the dependency for consumers of the static package. No CLI or
third-party SDK is required. TDH schema resolution can depend on WMI and
has no hard deadline of its own.

## Bounded counting, not a general PID resolver

accumulate_private_disk is portable and takes a thread identity that the
caller guarantees is valid throughout the inclusive QPC window [begin,end].
It sums only completions for that thread and window, not all events with
the header PID. It keeps uint64 read/write byte and operation counts without
converting to double and checks overflow without partially changing totals.
A valid zero-byte operation increments the operation count, not the byte
count. Other threads or timestamps are ignored; invalid identity, window,
or disk, or overflow, returns invalid. It does not deduplicate or reorder
events or calculate rates, known intervals, or known zero activity.

The probe uses its own main thread, which stays alive until after joining
ProcessTrace. It opens a real handle and checks GetThreadId and
GetProcessIdOfThread against its own PID. **A handle alone does not prevent
TID reuse after thread exit.** Here the identity is also valid because the
main thread remains alive throughout, not because the probe resolves other
threads' TIDs on receipt of delayed events. This is not a general thread,
process, or session map.

## Private workload and cleanup

The manual --authorized-private-io mode is not part of CTest. It creates an
exclusive file with CREATE_NEW next to the executable under work/, using
DELETE_ON_CLOSE and NO_BUFFERING|WRITE_THROUGH. It queries FILE_STORAGE_INFO,
checks sectors and alignment of the 64 KiB VirtualAlloc buffer, and verifies
offsets and sizes are sector multiples. It writes 16 × 64 KiB, seeks back to
0, and reads 16 × 64 KiB, checking every byte of its own data. The QPC window
encloses those operations, not queries, creation, or logging. No files
belonging to others are read, and no paths, contents, or arguments from
others are retained.

Closing the handle deletes only the newly created file; the probe then
checks for FILE_NOT_FOUND. A CREATE_NEW collision does not open or delete
existing files. The mode retains counters for its own thread, not the entire
payload or other issuers' data. There is no ETL file. Capture retains the
revision 59 limit and watchdog; after I/O it waits up to 3 seconds for buffered
delivery, with an independent 10-second watchdog. If the OS blocks I/O or
TDH, there is no hard deadline for the executable as a whole.

STOP now retains loss statistics for the session it owns, known only when
the API succeeds: EventsLost/RealTimeBuffersLost/LogBuffersLost. These are
not confused with traffic from other processes. Zeros do not guarantee that
the consumer drained all events: CloseTrace can cancel pending delivery.
The test requires positive totals of at least 1 MiB in each direction,
confirmed absence of the session and file, no schema errors, zero reported
losses, and clean consumer exit. It does not publish the totals as a
complete capture.

Observed write bytes can exceed 1 MiB due to metadata or other I/O from
the same thread inside the window. This does not demonstrate single-file
attribution, bytes persisted to media, original causality for deferred or
cached writes, or absence of double counting across storage topologies.
NO_BUFFERING does not necessarily bypass internal hardware caches.

## Verification and remaining work

Portable fixtures cover window, identity, zero, uint64, and overflow, with
an oracle of 10,000 events. Windows fixtures simulate TDH, sizes, errors,
missing and partial writes, mismatches, invalid GUID/opcode/version/flags/
payload/timestamp, both pointer widths, and issuer filtering. They do not
activate live ETW. The lifecycle test covers retained loss counters,
nonzero values, and validity when STOP fails.

First MinGW test: positive read 1048576/write 1159168 B, but schemaError 13;
the file and session were removed. The precise source of error 13 was not
isolated to a field, and no root cause is claimed. Early filtering was
improved and valid zero-byte values retained. MinGW repeat PASS:
read 1048576/write 1142784 B, 16 reads/38 writes, schemaError 0, all three
losses 0, STOP 0/QUERY 4201, ProcessTrace 0/CloseTrace 0, file deleted.
Initial logs were retained, not rewritten as successes.

Final Release suites: Windows 134/134 with MinGW and MSVC, Linux 113/113;
20 decoder and 20 lifecycle repetitions per Windows compiler. Three
installed external consumers were rebuilt and run with exit code 0,
including the Windows TDH dependency. Logs: work/etw-disk-events-verified-*,
repeat/install/consumer. MSVC D9025 is expected because of /UNDEBUG; no other
final build warnings. The live MSVC --authorized-private-io case was NOT
run: Start-Process reported that elevation was canceled. It was not retried,
and old lifecycle evidence does not count as a new positive result. The
PowerShell wrapper did not stop on that error and returned 0. That was the
wrapper's exit code, not the probe's, and was discarded as evidence of
success. Future launches must use -ErrorAction Stop and require a new
process and fresh evidence before interpreting its ExitCode.

Remaining work: resolve general temporal thread/process identity despite
reuse; produce complete windows with draining, loss handling, and
reordering; validate workloads from other controlled processes and storage
topologies; expose an appropriate opt-in; and integrate metrics without
substituting generic I/O for network metrics. M02 remains E, with no new
closure or ID.

Sources: [DiskIo_TypeGroup1](https://learn.microsoft.com/en-us/windows/win32/etw/diskio-typegroup1),
[TdhGetPropertySize](https://learn.microsoft.com/en-us/windows/win32/api/tdh/nf-tdh-tdhgetpropertysize),
[TdhGetProperty](https://learn.microsoft.com/en-us/windows/win32/api/tdh/nf-tdh-tdhgetproperty),
[file buffering and alignment](https://learn.microsoft.com/en-us/windows/win32/fileio/file-buffering).
