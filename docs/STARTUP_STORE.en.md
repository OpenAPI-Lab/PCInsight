# Explicit persistence of startup captures

Revision 118: [PCISTART v3 for child captures](STARTUP_BIRTH_STORE.en.md)
preserves keys, metrics, availability, windows, and thresholds without
coercing them into the legacy schema. The reader supports v1/v2/v3.
Missing birth data in older files is not empty or zero. The recorder
with watch_births now saves v3; rebuild the SDK and consumers.

Revision 107: StartupImpactArchive adds optional diagnostics. The recorder
saves historical diagnostics in v2. v1 files remain readable and their
diagnostics remain absent, not zero counters. Rebuild consumers for the
new fields. See [startup diagnostics](STARTUP_DIAGNOSTICS.en.md) for scope
and wire format.

StartupImpactStore (startup_store.hpp) saves and loads **one** historical
capture per file. It does not use external tools or new dependencies,
start a maintenance thread, or write automatically from Monitor. It
does not reuse AppHistoryEntry: startup CPU and I/O have different
identity, interval, coverage, and confidence from cumulative app history.
HistoryStore and its files were not changed.

```cpp
observer.close();
pcinsight::StartupImpactArchive capture;
capture.capture_id = "login-capture-2026-10-14"; // Caller-chosen identity.
capture.native_context = accepted_native_context;
capture.recorded_at = std::chrono::system_clock::now();
capture.observations = observer.observations();
auto saved = pcinsight::StartupImpactStore::save(private_path, capture);
// Handle saved.error and saved.replaced; do not assume rollback after publication.
auto loaded = pcinsight::StartupImpactStore::load(private_path);
// If loaded fails, there is no archive or partial row set. Do not overwrite
// it automatically in response to a load error.
```

Only closed observations are saved. The file preserves ID, PID/start_ticks,
native context, sample_count, window_complete, sampling_gap, last_error,
optional measurement, CPU/I/O/interval, and their Support, notes, and
classification. Missing values do not become zeros and confidence is not
upgraded. capture_id is a consumer-supplied identifier up to 256 bytes
without NUL, not a certificate of causation or authenticity. Every row
belongs to the same native_context. Duplicate identity, mixed contexts,
inconsistent windows, and invalid enums are rejected.

## Historical data is not current state

Absolute steady_clock timestamps, baselines, and state needed to resume
an observer are not persisted. Only the actual interval in nanoseconds
and recorded_at as UTC system_clock time with millisecond precision
are stored. PID, token, and context are historical. Do not use them
for actions on current processes or register loaded rows as live processes.
Another boot or PID namespace has a different context, and overlay with
that current context fails. Even during the same boot an older capture
is not current usage. The GUI should label it historical and display
recorded_at, capture_id, and scope. Nothing is automatically overlaid on
Monitor inventory.

The saved classification preserves the caller's policy at the time.
It is not recalculated on load, and the format does not retain an
executable policy. Validation checks structure and evidence consistency
as the overlay does; it does not authenticate counters or recalculate
consumer-chosen thresholds. Captures, processes, and resources are not
summed. Save replaces the chosen file with the complete capture.
Repeating the same save produces the same bytes rather than accumulating
deltas. To retain multiple captures, choose distinct paths and manage
retention. There is no merge, prune, or automatic deletion.

## Format and limits

The little-endian v1/v2 binary formats contain the PCISTART magic,
version, capture_id and context, recorded_at in milliseconds, row count
and observations, and a 64-bit FNV-1a checksum. V2 inserts diagnostics
before the checksum. At the revision 107 checkpoint, saving without
diagnostics emitted v1; saving with diagnostics emitted v2, including
automatic use by LinuxStartupRecorder.save. Readers before revision
107 reject v2; update before loading newer captures. Load does not migrate
or rewrite anything. Replacement stays an explicit caller operation;
there is no merge, checkpoint, or separate sidecar. The checksum detects
accidental corruption, not deliberate tampering.

Unknown versions, trailing data, truncated records, negative CPU or
overflow, unrepresentable time, and invalid enums or flags are rejected.
Load returns the entire validated file or nothing. Limits are a 16 MiB
file, 4,096 observations, and 8 MiB of overlay-compatible metadata:
32 KiB IDs, 1,024 B context, and 4,096 B notes. The store does not interpret
.desktop Exec or save other processes' env or argv. A platform whose
clock ticks cannot exactly represent a stored interval rejects it rather
than converting silently. Recorded_at cannot be negative. The host clock
can be wrong and does not certify the actual date.

## Writes, permissions, and concurrency

The caller chooses a file and a trusted existing directory. The store
does not create system or user directories or enable startup apps.
Load and save reject directories and final symlinks or reparse points.
They are not a sandbox for hostile path ancestors or compare-and-swap
against adversarial writers. Consumers must serialize writes; concurrent
saves are last-writer-wins. Load takes a historical snapshot, not a
persistent lock on the capture.

Linux creates an exclusive mode-0600 temporary file relative to a
directory descriptor, writes, fsyncs, closes, calls renameat, and
fsyncs the directory. An error before rename preserves the previous
destination and removes only its own temporary file. If directory
fsync fails after rename, the error is reported with replaced=true.
The new file has already been published; rollback is not claimed.
Load uses O_NOFOLLOW/O_NONBLOCK and requires a bounded regular file.
A FIFO is rejected without blocking. Replacement creates a new private
file; it does not preserve the old destination's xattrs, ACL, or owner.

Windows uses CREATE_NEW for a regular temporary file, FlushFileBuffers
and close, and MoveFileEx with REPLACE_EXISTING|WRITE_THROUGH. It does
not move the old file to a shared backup. A locked destination fails
without overwrite and the temporary file is cleaned up. Permissions
depend on inherited directory ACLs; use a private directory. The store
does not install an ACL or encrypt data. Atomicity and durability depend
on the filesystem and device; survival through every power loss is
not guaranteed. Load errors do not delete the existing capture.
There is no anti-tamper protection.

## Verification

Tests cover round-trip from a closed observer, known/unknown/permission
data, rows without deltas, checksum, every truncation and byte
corruption, version/count/flags/enums, CPU overflow even with a
recomputed checksum, trailing data covered by a valid checksum,
oversized files, overlay rejection of another boot's context, invalid
saves preserving the file, a new capture replacing without summation,
and temporary file cleanup. Linux tests mode 0600, symlinks, FIFOs,
and a nonwritable directory as a non-root UID. Windows tests a blocked
replacement with a real HANDLE and preservation of the file. Installed
consumers include the new API without affecting real files.

On 2026-10-14, Linux Release passed 33/33 and Windows Release passed
44/44. Packages were installed and rebuilt external consumers ran.
Verified builds introduced no new warnings.

Early recording, reliable automatic origin, and capture of short-lived
processes belonging to other programs remain open. Persistence does
not implement those collectors or close the startup impact parent task.
