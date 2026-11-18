# Temporal identity of processes and threads in a trace

Revision 62 adds [IRP correlation](WINDOWS_DISK_CORRELATION.en.md) with
injected tests and captures identity at I/O initiation. There is still no
ETW producer for lifecycle or initiation events and no Monitor adapter.
The pending correlation work below describes the historical revision 61 state.

Revision 61, M02, 2026-10-21. src/windows_trace_identity.hpp/.cpp implements
the private WindowsTraceIdentity temporal index. It is portable and was
tested without capturing ETW or requesting UAC. It is not yet an event
producer, a live system map, a Monitor metric, or a complete per-process
disk/network solution.

## Identity and coverage

Inputs are process lifetimes (PID/start/end) and thread lifetimes
(TID/PID/process-generation start/start/end), on one QPC clock and capture
epoch. End 0 means alive through the caller-guaranteed watermark, not
indefinitely. Returned identity includes epoch, PID, TID, and both start
times. These numbers are not Windows FILETIME values or tokens for
terminate_process_checked or other actions. Do not convert QPC ticks into
an action identity by rounding.

Set complete=true for replace only when the future producer has reconciled
lifecycle coverage, loss, and out-of-order events through the watermark.
The index does not establish this condition, and a zero ETW loss count
alone does not prove it. complete=false clears prior answers and leaves
the index unavailable. After a loss, the caller must rebuild coverage
with the appropriate epoch instead of treating the previous map as
complete. The caller manages the epoch; it is not a globally authenticated
or persistent identifier.

Each replacement accepts up to 100,000 combined process and thread records.
It builds temporary tables, sorts by start time, and publishes only after
validating everything. Invalid or oversized input and allocation exceptions
clear prior answers. The index does not mix partial captures or silently
truncate excess records. Empty complete sets are ready with no answers;
the absence of processes does not imply a complete OS capture. The object
is not thread-safe: use it on the collection thread or synchronize
replacement and queries externally.

Validation checks IDs, positive and ordered intervals, ends no later than
the watermark, an existing process generation, and containment of each
thread within its process. Exact duplicates and overlapping generations
of one PID or TID invalidate the entire replacement. Different TIDs can
coexist. A backward watermark in the same active epoch invalidates the
replacement; a new epoch is a separate namespace. Missing thread ends
are not inferred from a process that has already ended.

## Issuer lookup and late operations

owner_at_initiation(TID,issued_qpc) looks up the generation at the
**actual start of the operation**, not at callback delivery or completion.
It does not open current processes or threads to resolve older events.
owner_for_interval requires the entire [start,end] interval to belong to
the same lifetimes. It is conservative: crossing termination or reuse, or
landing exactly on a start, end, or watermark boundary, yields no answer
instead of arbitrarily choosing a generation.

For example, TID 7 of PID 10, generation 100, issues an operation at 150
and exits at 240. TID 7/PID 10 are then reused in generation 300, and the
earlier operation completes at 350. Looking up only 350 would incorrectly
select the new process. A lookup at initiation time 150 keeps the earlier
identity; requiring the interval 150..350 yields unknown. To count a
completion after its issuer exits, the future correlator must capture
identity at initiation, associate it with the operation, and handle reuse,
duplicates, and loss before completing the match.

This does not automatically connect to the [completion
decoder](WINDOWS_DISK_EVENTS.en.md). At that revision the decoder did not
provide operation initiation or IRP correlation. Do not substitute
completion time, arrival time, current time, or a subtraction based on
unvalidated latency for missing initiation time. Do not treat DCStart/DCEnd
as exact birth times without reconciling the observation boundaries.

## Tests and next integration step

tests/windows_trace_identity.cpp uses portable fixtures, without capture
APIs. They cover PID/TID reuse, the wrong process or thread, late lookups,
crossing intervals, exact boundaries, open ends, unknown/complete=false/reset,
unordered inputs, duplicates and overlaps, containment, invalid and negative
values, backward watermarks, epoch changes, int64max without overflow, empty
sets, the exact 100,000-record limit and excess, and recovery. An independent
linear oracle checks 20,000 queries against the binary-search index.

ETW was not started, and the MSVC elevation canceled in revision 60 was
not retried. Producing and reconciling lifecycle and I/O initiation events,
operation correlation, coverage and draining, and a public opt-in adapter
were still pending. M02 remained E. No public API, layout, ABI, vtable, or
persistence changes and no additional dependencies. Final Release suites:
Windows 135/135 with MinGW and MSVC, Linux 114/114. Twenty index repetitions
in each of the three environments and three installed external consumers
rebuilt and run with exit code 0. Logs: work/trace-identity-*. The first MSVC
build of the new test retained the expected D9025 warning due to /UNDEBUG.

Decision sources: [Thread_TypeGroup1](https://learn.microsoft.com/en-us/windows/win32/etw/thread-typegroup1)
documents payload PID/TID and distinguishes Start/End from rundown.
[Process_TypeGroup1](https://learn.microsoft.com/en-us/windows/win32/etw/process-typegroup1)
describes ID lifetime scope. [DiskIo_TypeGroup2](https://learn.microsoft.com/en-us/windows/win32/etw/diskio-typegroup2)
documents I/O initiation, IRPs, and issuing threads. This block does not yet
decode those classes.
