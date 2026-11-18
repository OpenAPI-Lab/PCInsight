# Explicit persistence for observed Windows app history

`pcinsight/windows_history_store.hpp` exposes `WindowsHistoryStore`:
load, save, merge, retain_since, prune, and reset for WindowsAppHistoryEntry.
It is portable: Linux can read and write this report, but that does not
enable a Win32 collector. Monitor/Sampler do not save automatically or
choose paths. The consumer decides when to persist and must check each
result before continuing.

The store preserves the optional value, Support, and note for observed CPU,
network, and notifications. Missing does not become measured zero, and
estimates remain estimates. Persistence does not add network or notification
collectors or reconstruct CPU use before capture. See [Windows app
history](WINDOWS_APP_HISTORY.en.md).

## Usage and merging

For an existing valid file, use load, check success, merge its entries with
the value from the snapshot.windows_app_history Metric, and save the result.
For a new path, explicitly start with an empty vector; do not treat every
load error as a new file. If the snapshot Metric has no value, do not
substitute zero counters. The program must serialize the entire
load/merge/save sequence, including prune and reset, across threads and
processes that share the destination.

The cumulative key is (id, period_start in milliseconds). Repeated
snapshots replace the row rather than adding its CPU time again. Snapshots
with an earlier period_end are ignored. A row with a later or equal
period_end cannot reduce a known counter or drop its value; merge throws
std::invalid_argument. A transition from missing to known is allowed.
Resetting counters requires a new period. Different collectors must not
share an ID and period start in the same millisecond. Distinct periods
remain separate. Name, notes, and support come from the accepted row.

retain_since keeps entire periods whose end is at or after the cutoff,
with both times normalized to milliseconds. It does not prorate usage for
periods crossing the cutoff. prune loads, filters, and saves; there is no
maintenance thread. reset deletes only the chosen path and reports errors;
an absent file is already reset. This is an explicit destructive action,
not corruption recovery.

## Format and limits

Separate little-endian v1 format, distinct from legacy HistoryStore v1/v2:

- Eight-byte magic: PCIWHST followed by NUL; uint32 version 1 and uint32
  row count.
- Per row: ID and name (each string has a uint32 length), start/end as
  uint64 milliseconds since the epoch, then CPU, network, and notifications.
- Per Metric: a Support byte (0 available, 1 permission_required,
  2 unsupported, 3 estimated), a 0/1 presence byte, a uint64 value if
  present, and a note string. CPU is nonnegative nanoseconds that fit in
  int64; the other two are uint64 counters. available/estimated require
  a value; the other statuses do not allow one.
- A uint64 FNV-1a trailer over all preceding bytes. It detects accidental
  corruption, not authenticity or deliberate tampering.

Limits: 64 MiB per encoded file, 100,000 rows, 1 MiB per string, nonempty
NUL-free identity, NUL-free name, nonnegative times, and end no earlier
than start. Notes are preserved as bytes without UTF-8 validation. Times
are truncated to milliseconds. load rejects unknown formats or versions,
truncation, invalid flags, intervals or ranges, and trailing bytes without
publishing partial rows. format_version reports 1 only after a successful
read. These budgets do not cap total RAM: there are copies of buffers,
strings, maps, and rows.

There is no automatic conversion to or from HistoryStore. Choose a
separate file: the legacy format does not preserve per-field availability.
Each reader rejects the other format. The existing legacy format and its
v1/v2 migration are unchanged.

## Writes and security

save validates entries and, if the destination exists, requires it to
load as valid Windows history before replacing it. It rejects corrupt,
legacy, or unrelated destinations and does not deliberately repair or
overwrite them. This is a prior check, not compare-and-swap: another
writer may change the path afterward. The caller must coordinate writers
and choose a trusted directory. This is neither protection from an
attacker controlling the directory nor a locking mechanism.

It reuses HistoryStore's replacement procedure: an exclusive sibling
temporary file, flush, close, and replacement without first removing
the destination or using a shared backup. Failures before replacement
preserve the destination and clean up the store's own temporary file.
Guarantees depend on the filesystem. There is no universal power-loss
guarantee; on Linux there is no directory fsync, and on Windows the
temporary file inherits the directory ACL and does not necessarily
preserve the replaced file's metadata. There is no encryption; executable
paths, names, and notes can be sensitive. The consumer chooses appropriate
permissions and ACLs and handles resource errors and exceptions.

## Verification

2026-10-15: Release Windows 78/78 and Ubuntu WSL Linux 58/58. Installation
and the external consumer were rebuilt and run with exit code 0 on both.
The new windows-history-store test keeps assertions active in Release.
It covers round trips for notes, support and Unicode; missing versus
measured zero; repeated, stale, or decreasing merges, dropped values,
and distinct periods; retention; every truncation of a fixture; changed
checksum, flags and sizes; CPU overflow; reversed or negative times;
trailing bytes; separate formats; rejected saves preserving corrupt,
legacy, or valid files on invalid inputs; directory destinations;
preservation of an unrelated backup; cleanup; and repeated reset.
The installed consumer includes the header and links load/save/merge
with empty entries without writing user data. Power loss was not
simulated and concurrency was not tested as a transaction; there is
no claim of multiwriter locking.

On Windows, run these filesystem tests with TMP/TEMP pointing to work
and access allowed to their private temporary files, as in the full suite.
An additional run without that environment failed on the first save;
repeating it in the suite environment passed 1/1. The implementation
was not changed to bypass system permissions.
