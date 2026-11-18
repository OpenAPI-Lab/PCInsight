# Correlating disk I/O initiation and completion

Revision 62, M02, 2026-10-21. Connects the [temporal identity index](WINDOWS_TRACE_IDENTITY.en.md)
to normalized initiation and completion events through an opaque IRP. This is
a private component: no new Monitor/Sampler metric, public API or ABI, or
persistence. No live ETW, UAC, workload files, or retry of canceled elevation.

## TDH decoder

decode_windows_disk_operation preserves the earlier completion decoder and
adds DiskIo ReadInit (12) and WriteInit (13). Read (10) and Write (11) remain
completion events. Only classic version 3 is supported, with an unambiguous
32- or 64-bit width from event flags, a positive timestamp, and the required
payload and functions. Other GUIDs or opcodes, including FlushInit (15),
are ignored. Unsupported versions or ambiguous data do not yield a valid
event. Version 3 init support is tested by fixtures, NOT by native capture.

IssuingThreadId, TransferSize, and DiskNumber remain uint32 properties.
TDH reads the IRP by the name Irp: 4 bytes in a 32-bit event or 8 in a
64-bit event. The decoder zero-extends 32 bits, preserves all 64 bits,
and rejects incorrect lengths, zero, or an all-ones pointer of the
corresponding width. The two-read, two-seed scalar reader was generalized.
Values equal to the seeds remain valid if both reads write them fully.
There are no manual offsets, no truncation of a 64-bit event pointer to
32 bits, and no changes to the revision 60 completion query.

The IRP is strictly an opaque internal key: it is never dereferenced or
used as a permanent operation or process identifier. Do not export it in
public metadata or persist kernel addresses. The optional thread filter
can discard other issuers before retrieving sizes or IRPs.

## Bounded correlator

WindowsDiskCorrelation takes a maximum age in QPC ticks (not implicit
seconds) and a pending-entry limit of 1..100,000. sample takes an epoch,
watermark, events, and a WindowsTraceIdentity with reconciled coverage.
It requires identity to be ready, the same epoch, coverage through the
watermark, and complete=true from the caller. This is a **precondition**,
not proof that ETW was complete. Missing coverage, reported loss, an epoch
change, or a reset discards earlier pending entries.

A future producer must order and reconcile the stream before calling
sample. Each batch contains events in [previous watermark, new watermark),
in nondecreasing QPC order. The watermark must advance. Nonpositive or
out-of-window timestamps, backward ordering, invalid IDs or pointers, or
an invalid disk on completion invalidate the entire batch and clear
pending entries. The limit is 100,000 events per batch. No successful prefix
is published if a later operation in the batch fails. Allocation exceptions
also reset state before propagating.

On initiation it saves the IRP, TID, read/write type, timestamp, and the
identity resolved **at initiation**. If identity cannot be resolved, it
does not invent it from the header or a current query: the completion will
be unmatched. A second initiation for a pending IRP is ambiguous and
invalidates the batch without overwriting the original issuer. Exceeding
the pending-entry limit also invalidates the batch.

Completion requires the same IRP, TID, and type, with a timestamp strictly
after initiation. It keeps the captured owner even if the process or thread
has exited or its IDs have been reused. It does not resolve the owner again
at completion. It consumes the entry once. An orphan or duplicate completion
without a pending entry counts as unmatched and never generates a second
row. An IRP can be reused after completion with a later initiation. An old
duplicate arriving after that reuse is rejected for a backward timestamp or
window under the real-timestamp contract. The correlator does not authenticate
timestamps or detect forgery by an arbitrary caller.

If the maximum age is exceeded, the completion is unmatched. At the end
of each batch the correlator removes overly old pending entries and reports
them as discarded, not as zero activity. It does not expire entries at the
final watermark BEFORE processing earlier valid completions in the batch.
The object is not thread-safe; serialize sample/reset. Tables and results
are bounded, with no unbounded history.

## Output and limits

processed means matching pairs were processed, **not** that all disk
activity for every process was measured. Each row contains TraceOwner,
epoch and generation, the internal IRP, start and end QPC timestamps, and
the completion's bytes, disk, and type. Valid zero bytes are preserved.
No rates or percentages are calculated, and no zero rows are added for
processes without events. unmatched and discarded indicate gaps; neither
can establish that a window is complete, even if both are zero.

Still pending: a native producer for lifecycle and I/O initiation events,
temporal reconciliation, reordering, draining and loss handling, live
validation of versions, IRPs and workloads, and a public opt-in adapter.
The revision 59 controller still enables only DISK_IO, NOT DISK_IO_INIT.
It is not claimed to be connected to the new decoder or correlator, and its
flags are not silently changed. The revision 60 private test does not
validate this new path. General M02 disk and network work remains open.

## Verification

Portable fixtures cover ordinary pairs and pairs spanning batches,
completion after process/thread exit or PID/TID reuse, mismatched type or
TID, duplicate or reused IRPs, orphan and duplicate completions, time,
window, epoch, coverage, loss and reset, exact and exceeded TTL, zero and
uint32max bytes, int64max ticks, budgets, 100,000 pending entries, and the
input limit. A deterministic stream of 10,000 pairs checks amounts and
identities. Windows fixtures cover TDH 32/64, init and completion, full-width
IRPs, sizes, sentinels and errors, and an injected TDH → temporal index →
correlator pipeline with incorrect header PID/TID and the old issuer
retained after its IDs are reused. No fixture enables ETW. The first build
failed because the local name far collided with a legacy SDK macro; it was
renamed to extreme_identity without changing the logic. Initial logs were
retained. Final Release suites: Windows 136/136 with MinGW and MSVC,
Linux 115/115. Twenty correlator and twenty decoder repetitions in each
of the three environments. Three installed external consumers were rebuilt
and run with exit code 0. work/disk-correlation-* retains builds, suites,
repetitions, installation, and consumer logs. MSVC still reports the expected
D9025 because of /UNDEBUG, with no other final warnings.

Primary sources: [DiskIo_TypeGroup2](https://learn.microsoft.com/en-us/windows/win32/etw/diskio-typegroup2)
documents initiation, IRPs, and issuing threads.
[DiskIo_TypeGroup1](https://learn.microsoft.com/en-us/windows/win32/etw/diskio-typegroup1)
documents completion, amounts, and correlation by IRP. TDH and the QPC
source follow the contracts in the [disk event guide](WINDOWS_DISK_EVENTS.en.md).
