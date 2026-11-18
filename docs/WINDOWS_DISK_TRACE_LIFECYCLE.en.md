# Disk ETW: access and lifecycle of a session owned by the probe

Revision 60 adds the [decoder and private I/O test](WINDOWS_DISK_EVENTS.en.md),
a separate --authorized-private-io mode, and STOP statistics. Statements in
this document about discarding ALL events describe the two original lifecycle
modes, not the new private counting mode. No integration by default.

Revision 59, M02, 2026-10-21. The user replied “prosigue” to the explicit
revision 58 request. This was treated as authorization for the bounded,
temporary experiment, including elevation if needed.
[Scope](WINDOWS_PROCESS_DISK_DECISION.en.md).

This block implements a **private** controller and a manual probe. It does not
publish per-process disk or network data, add a continuous capture option, or
change Monitor/Sampler. No new public types, layouts, vtables, or persistence.

## Controller

src/windows_disk_trace.hpp/.cpp uses StartTraceW/ControlTraceW, native APIs
already linked through advapi32, with no CLI, external SDK at runtime, or
installation. The fixed descriptive name `PCInsight Private Disk Test` and
its own GUID are not NT Kernel Logger/SystemTraceControlGuid. “Private” refers
to ownership of the experiment, NOT an in-process logger or PID capture filter.

StartTrace is attempted only once per object. ALREADY_EXISTS, access denied,
and other errors do not acquire ownership or trigger QUERY/UPDATE/STOP on
another session. Only a handle returned by successful StartTrace authorizes
UPDATE/STOP. The controller never stops by name or automatically adopts or
cleans up presumed orphan sessions.

It requests SYSTEM_LOGGER|REAL_TIME|NO_PER_PROCESSOR_BUFFERING and explicitly
requests STOP_ON_HYBRID_SHUTDOWN. No ETL file or autologger; QPC clock. Buffer
size is 64 KiB, min 4/max 64 (4 MiB requested ceiling). QUERY checks the exact
GUID, modes, and flags, absence of a file, and the buffer size/count against an
accepted ceiling of 64 MiB. If the OS adjusts the configuration outside the
allowed bounds, the probe aborts and removes the session. This does not guarantee
the OS will never reserve more temporarily before the check.

The session is first created with EnableFlags 0. The probe opens OpenTraceW
and starts ProcessTrace on another thread before enable_disk(). The controller
requires the caller to follow that order; it cannot check consumer ownership.
UPDATE enables only EVENT_TRACE_FLAG_DISK_IO and explicitly retains REAL_TIME.
QUERY revalidates the configuration. This block does not enable process, thread,
network, file-name, or stackwalk events.

stop() serializes concurrent calls and issues a single STOP for the acquired
handle, then queries the name (read-only). ERROR_WMI_INSTANCE_NOT_FOUND (4201)
confirms absence. If a different session appears under that name or the query
fails, cleanup is reported as unconfirmed. The controller does not stop by
name or retry a potentially recycled handle. stop_error and query_error are
kept separate. The destructor attempts the same cleanup only if it has not
already been attempted. There is no automatic recovery from uncertain cleanup:
report it and stop the investigation before another capture.

## Manual probe, not CTest

tests/windows_disk_trace_native.cpp requires --authorized-lifecycle or
--authorized-watchdog; without either option it exits with code 77 without
creating evidence or a session. It is not registered with CTest. Automated
tests inject simulated APIs only.

The probe attempts to enable SeSystemProfilePrivilege only in its own token,
restores the state on exit, and does not change groups, policies, or
configuration. UAC elevation was requested only after the ordinary token
failed. Neither Monitor.sample() nor the controller performs elevation.

The callback discards ALL events without reading or storing payloads, paths,
contents, arguments, identities, or counters. Only metadata about the owned
session and control errors are saved in CREATE_NEW files next to the executable
under work/. These are not ETL traces. The probe creates no disk workload file
and accesses no other party's data. This is not yet an attribution test.

A watchdog thread separate from the consumer requests STOP and CloseTrace at
10 seconds; watchdog test mode uses 1.5 seconds. Normal shutdown is requested
after 1 second, or the probe waits 2 seconds to exercise the watchdog. It wakes
and joins the watchdog and joins the consumer after session shutdown/CloseTrace.
There is no hard deadline if OS APIs block, nor a cleanup guarantee after a
process kill, process crash, or system failure. The authorized time budget is
15 seconds. The observed paths completed normally. If cleanup is reported as
uncertain, do not continue capturing.

## Evidence and limits

Ordinary token: AdjustTokenPrivileges 1300, StartTrace 5; no session created.
Elevated MinGW and MSVC: start/verify 0, enable/verify 0; mode 0x12400100,
flags 0->256, 64 KiB buffer, 4 reserved/max 64, matching GUID. Normal and
watchdog shutdown on both: STOP 0, QUERY 4201, ProcessTrace 0, CloseTrace 0,
cleanup confirmed 1, exit 0. All events were discarded. This demonstrates
control, configuration, and cleanup, NOT event delivery, PID/thread attribution,
counts or rates, absence of loss, cache or deferred-write semantics, or
universal machine coverage.

Initial failures were retained: an ANSI macro in LookupPrivilegeValueW was
replaced by a wide literal; initial validation rejected the default hybrid
shutdown flag (13), fixed by requesting it explicitly; initial UPDATE with
mode 0 failed with 87, fixed by retaining REAL_TIME without relaxing validation.
Removal was confirmed for every session created by those attempts before retrying.

Fixtures cover collisions, permissions, resources, missing functions,
QUERY/UPDATE failures, invalid GUIDs, modes, flags, files and buffers, overflow,
the exact 64 MiB ceiling, STOP failure or uncertain query, destructor/idempotence,
and eight concurrent shutdown calls. Logs are under work/etw-lifecycle-* and
in .result_*.txt alongside each probe. Event decoding, temporal identity,
loss handling, and private I/O validation were still required before integrating
per-process disk activity in M02. Final Release suites: Windows 133/133 with
MinGW and MSVC, Linux 112/112. Twenty fixture repetitions per Windows compiler
and three installed external consumers rebuilt and run with exit code 0.
MSVC D9025 is expected because of /UNDEBUG.

Primary sources: [StartTraceW](https://learn.microsoft.com/en-us/windows/win32/api/evntrace/nf-evntrace-starttracew),
[ControlTraceW and UPDATE REAL_TIME](https://learn.microsoft.com/en-us/windows/win32/api/evntrace/nf-evntrace-controltracew),
[modes and hybrid shutdown](https://learn.microsoft.com/en-us/windows/win32/etw/logging-mode-constants),
[EVENT_TRACE_PROPERTIES buffers](https://learn.microsoft.com/en-us/windows/win32/api/evntrace/ns-evntrace-event_trace_properties).
