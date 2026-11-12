# Persistent application history

Since125, INTERNAL manual launch evidence uses a separate PCIMAN v1 envelope
with embedded PCISTART v3 births and raw receipt; see STARTUP_MANUAL_STORE.md.
It does not alter application-history merge/retention or PCISTART formats.
Loading historical PIDs/links never restores live child ownership or authority.
Since126 the existing recorder has an INTERNAL manual-only owner/finalize/save
bridge to that envelope; see STARTUP_RECORDER_MANUAL.md. Legacy recorder tables
and app-history stores are unchanged; no GIO-hint acceptance or scan is needed
for this explicit manual route.

Startup captures use the SEPARATE PCISTART format, not this history format.
Since118, PCISTART v3 optionally preserves native birth captures/window/policy;
v1/v2 remain readable with birth data ABSENT. See STARTUP_BIRTH_STORE.md. No
application-history merge/retention semantics or file formats change.
Since107, StartupImpactStore reads v1 and v2; optional recorder diagnostics cause
v2 writes. See STARTUP_DIAGNOSTICS.md. This does not migrate or merge application
history files, and absent legacy diagnostics must not become measured zeroes.

`HistoryStore` is an explicit storage API. `Monitor` collects in memory; the caller
chooses the path and when to load/save. No automatic disk writes are performed.

Observed CPU history now uses separate `linux_app_history` and
`windows_app_history` snapshot reports with per-field availability (see
LINUX_APP_HISTORY.md and WINDOWS_APP_HISTORY.md). `snapshot.app_history` stays
empty on these backends. HistoryStore still accepts legacy AppHistoryEntry only;
it does NOT serialize either observed report. WindowsHistoryStore persists and
merges that report in a separate format/file, preserving each metric's presence,
support and note; see WINDOWS_HISTORY_STORE.md. Do not coerce missing metrics
into measured zeroes or claim this legacy v1/v2 store serializes support flags.
LinuxHistoryStore now preserves LinuxAppHistoryEntry rows in a separate format;
see LINUX_HISTORY_STORE.md. It does not serialize the outer report's status or
restore F03 resources/baselines. H04 now publishes independent observed network
periods in linux_network_app_history; see LINUX_NETWORK_APP_HISTORY.md. Keep CPU
and network files separate; never merge independent periods by matching paths.
H02 covers the legacy
store, not this Linux format. The following example requires explicitly supplied legacy entries whose
integer counters are known; do not convert the observed reports to this type by
inventing zeroes for absent network/notification metrics.

Since2026-10-19, LinuxNotificationObserver provides a separate opt-in observed
report for successful Notify calls, with untrusted desktop-entry hints and
connection/period scope; see LINUX_NOTIFICATIONS.md. It does NOT populate the
CPU report or legacy integers, and HistoryStore does not serialize it. Do not
join its identities to executable CPU history or replace unknowns with zero.

A01 now exposes the explicit alias `LegacyAppHistoryEntry` (same original type,
layout and binary identity) in the storage API. Use `AppHistorySample` from
`pcinsight/app_history.hpp` for telemetry, not that raw legacy payload.
`import_legacy_history(previous.entries)` imports identities/periods with ALL
counters unknown, even positive raw counters: v1/v2 did not record availability.
Raw values remain accessible; neither conversion nor import rewrites a file.
There is no implicit conversion of the measured view to legacy storage.
See METRIC_AVAILABILITY.md for the contract and native projection helper.

```cpp
auto previous = pcinsight::HistoryStore::load(path);
// Handle load errors explicitly; do not silently overwrite a corrupt history.
auto combined = pcinsight::HistoryStore::merge(previous.entries, observed_legacy_entries);
auto result = pcinsight::HistoryStore::save(path, combined);
```

Merge treats `(id, period_start)` as the identity of a cumulative collection
period, with timestamps normalized to milliseconds as in the file format.
Repeated snapshots replace counters for that period instead of adding them.
Older snapshots are ignored. A newer snapshot with decreasing counters raises
`std::invalid_argument`; after resetting counters, use a new period start.
Different collection periods remain separate rows. Sum these rows in the UI
when displaying an application's lifetime total. Distinct simultaneous
collectors must not share the same application ID and period start.

The v1 little-endian format remains readable. New saves use v2, whose body has
the same records and an appended 64-bit little-endian FNV-1a checksum over the
header and records. This detects accidental corruption, not deliberate tampering.
`HistoryLoadResult::format_version` reports the decoded version. Load rejects invalid headers,
unsupported versions, negative/overflowing CPU durations, out-of-range times,
inverted intervals, truncated records and trailing data. Limits: 64 MiB per
file, 100,000 records and 1 MiB per string. Rejected loads expose no partial rows.
Older files with already doubled counters cannot be reconstructed automatically.

Save creates an exclusive temporary sibling, flushes and closes it, then replaces
the destination using the platform's file replacement operation. It never
renames the old destination away first or uses a shared `.backup` name.
Failures before replacement preserve the old file. Replacement guarantees depend
on the filesystem; this is not a guarantee against power loss on every storage
device. Linux directory metadata is not explicitly fsynced. Concurrent saves are
last-writer-wins; callers must serialize load/merge/save to avoid lost updates.

Reset removes the chosen history file and reports filesystem errors; an absent
file is already reset. Network/notification counters are only as complete as the
underlying collectors; persistence does not implement those missing collectors.

Validation on 2026-10-12: Windows Release 4/4 tests; Ubuntu WSL Release 5/5.
History regressions include every byte truncation of a sample file, repeated and
stale snapshots, separate sessions, invalid counters, failed replacements,
preservation of unrelated backup files and reset failures.

## Retention and migration

Call `retain_since(entries, cutoff)` to filter in memory, or `prune(path, cutoff)`
to load, filter and replace a file. Retention keeps whole periods whose end is
at or after the cutoff (millisecond precision). It does not prorate counters
for periods spanning that boundary. A long-running session remains until its
end precedes the cutoff; rotate collection periods if daily retention is needed.
The application schedules pruning; the library has no hidden maintenance thread.

`migrate(path)` validates and upgrades a v1 file in place to v2. Calling it on
v2 is a no-op. Unknown versions and corrupt files fail without being overwritten.
Old 0.2 executables that only understand v1 cannot read v2; upgrade readers
before migrating. Counter errors already present in legacy data are preserved,
not reconstructed. Concurrent load/merge/prune/save operations need caller locking.

Tests cover v1 loading, v1-to-v2 upgrade, repeat migration, corruption preserving
the original, cutoff boundary, empty retention and repeated pruning.
