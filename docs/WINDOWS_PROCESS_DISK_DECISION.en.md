# Per-process disk activity on Windows: M02 next step

Revision 59 update: the user authorized continuing. [Session control and
native tests](WINDOWS_DISK_TRACE_LIFECYCLE.en.md) were implemented and verified.
Attribution and Monitor integration were not yet implemented. The rest of
this document preserves the historical revision 58 decision and request;
it does not describe an active blocker.

Revision 58, 2026-10-21. Documentation research only: no provider was
implemented and no ETW test was run. M02 remains E; the percentage does not
increase. Revision 57 already implements general I/O, which does **not**
distinguish disk from network or device I/O.

## Approach evaluated

ETW is a native system interface that does not require running external
tools. [DiskIo](https://learn.microsoft.com/en-us/windows/win32/etw/diskio)
can enable disk events on a system logger. The
[DiskIo_TypeGroup1](https://learn.microsoft.com/en-us/windows/win32/etw/diskio-typegroup1)
class documents the size in bytes, disk number, and issuing thread. The
issuing thread is not available on every older version. The PID in a
completion header is not enough to identify the process that originally
requested the operation.

Attribution requires a thread-to-process mapping valid at the time of the
event, handling of identifier reuse, and detection of lost events. The
provider does not promise to attribute every deferred system or cache write
to the original app, or to interpret a lack of events as known zero activity.
Unsupported events or schemas and incomplete windows must remain unknown.
Do not sum multiple layers of one I/O operation as separate transfers.

[StartTraceW](https://learn.microsoft.com/en-us/windows/win32/api/evntrace/nf-evntrace-starttracew)
documents access restrictions and limited session resources. Do not assume
that the current token can start a system logger. A uniquely named session
is not the same as a capture limited to one process. Monitor.sample() will
not start a session or enable elevation as a side effect.

## Authorization requested for the next experiment

- One real-time ETW test session with its own descriptive name, no ETL
  file, and no more than 15 seconds of capture.
- Only the required disk and process/thread identity events. Events from
  other processes can pass transiently through memory; discard their data
  and retain only the private test process's identity and counters.
- Do not record other processes' paths, file contents, or arguments.
  Limit the requested buffer budget to 64 MiB; abort if that limit cannot
  be met. Do not raise global quotas or change the registry or configuration.
- Keep the test executable and file in work/. Use bounded I/O on the test
  files only. Use administrator privileges only if needed for this test,
  without installing services, drivers, or utilities or changing groups.
- Abort if the requested session already exists. Do not adopt, modify,
  or stop sessions owned by others. On all normal and error exits, close
  the consumer, stop only the session created by the test, and verify that
  it was removed.
- An independent watchdog must request shutdown at the deadline. If
  cleanup cannot be verified, report the session name and error and do not
  keep capturing. Do not guarantee cleanup after an OS crash or forced
  termination.

The authorization would not cover permanent capture, network ETW, account
changes, autologgers, or integration by default. First verify access and
lifecycle; then test attribution with private I/O, event loss, identifier
reuse, and errors. At the time of this request, the experiment had not yet
been written or run. The decision does not replace the implementation and
validation required to close M02.
