# Observed impact in the GUI inventory

Completed work: apply_startup_impact_observations in startup_overlay.hpp.
It integrates explicitly accepted observations into
SystemSnapshot.startup_entries by **exact ID**, without native I/O,
threads, automatic registration, or OS side effects. It operates on
portable data and does not imply a new native Windows reader.

```cpp
auto view = monitor.sample(options); // startup_entries enabled; fresh snapshot.
auto rows = observer.observations();
auto result = pcinsight::apply_startup_impact_observations(view, rows, accepted_run_context);
// GUI: use observed_startup_impact.value/support/note and impact_scope.
// CPU, I/O, and interval: observed_startup_cpu_time/disk_bytes/interval.
```

expected_native_context comes from the accepted phase
(baseline.native_context), not a user-selected label used as
authentication. StartupImpactObservation now contains native_context.
Batches mixing contexts are rejected. The API does not query the
current boot or authenticate the caller. Do not persist timepoints as
universal identities. The caller selects a valid phase and context and
synchronizes the snapshot.

For one record, copy CPU and I/O deltas and the actual duration without
adding earlier runs. Values with available or estimated support are
labeled estimated in the GUI. Unsupported or permission_required values
are not published as numbers even when an external provider supplied a
value. A baseline alone means unknown with no delta. Low and medium
require a closed, complete, gap-free window and sufficient evidence.
Known high can remain an observed lower bound for a partial window.
The overlay does not infer none from disabled or zero or change
status or configuration.

For multiple processes under one ID, it neither adds CPU, disk, or
windows nor takes the maximum of medium or low. If any has known high,
it applies estimated high as a lower bound; otherwise impact is unknown.
Aggregate totals and interval stay unknown. An aggregation policy must
explicitly resolve child accounting and overlap. Legacy entry.impact is
shorthand for the known result or not_measured. **Do not display it alone**
as if it were complete or certified startup impact. impact_scope states
the caller association, single or multiple scope, and that automatic
origin is not certified. A GIO marker does not establish that cause.

Entry IDs not present are counted and ignored, not relabeled by basename.
Entries without observations stay intact. Use a fresh snapshot per phase
or refresh. Reusing an old view without observations does not automatically
purge old overlays. Name, command, publisher, status, and conditions
are unchanged. Reapplying a batch replaces values; it neither increments
counters nor sums histories. Exact ID does not prove that .desktop
contents stayed unchanged after launch.

Validation covers IDs, counters, identity, context, sample_count,
positive interval, measurement-to-observation consistency, window flags,
and a classification supported by counters. A PID/start_ticks identity
cannot be accepted twice or for two apps. The original policy is not
recomputed because the row does not contain it. Caller-fabricated data
are not authenticated. A different context yields stale_target; other
invalid input yields invalid_argument. Full validation and staging
precede a noexcept commit so invalid input cannot partly modify the
snapshot. Private data and arguments, and provider exception messages,
are not logged.

Limits are 4,096 observations, 100,000 entries, 1,024 B context, 32 KiB
ID, 4,096 B note, and 8 MiB of validated input IDs, notes, and contexts,
plus bounded overhead and staging. Allocation errors can propagate
before commit without changing data. Writes from other parties to the
same snapshot are not synchronized by the API; the caller coordinates
access. Expanded public 0.x structs and APIs require rebuilding the
library and consumers together.

Verified on 2026-10-14: Linux Release passed 31/31 and Windows Release
passed 42/42, with installation and external consumers run. Portable tests
cover partial medium and low, high lower bounds, multiple processes without
sums, absent deltas, permissions and unsupported values, idempotence,
unchanged or ignored entries, invalid context/identity/metadata, and
rollback on validation failure. The real Linux pipeline uses its own
XDG /tmp, Monitor, a baseline from the test process, observer, and overlay.
GUI metrics are available as estimated and flags stay intact. A new
Monitor without apply retains not_measured, demonstrating there is no
hidden overlay. No real startup app or Exec was run and no real file
or service was changed.

This closes **explicit linking** of measurements to inventory. Reliable
automatic early capture, short-lived processes and descendants,
aggregation, and persistence remain open at this checkpoint. This link
does not complete startup impact.

Later update: [startup store](STARTUP_STORE.en.md) implements explicit
persistence of closed captures. Loaded files are not automatically
applied to inventory or presented as current boot or live-process data.
