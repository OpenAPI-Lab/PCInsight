# Availability, units, and compatible history (A01)

New consumers use `Metric<T>`, which has an optional value, Support, and a
note. `metric_is_known(metric)` requires both a present value and support
of available or estimated. A present zero qualifies, as does a measured false.
Do not use value_or(0) to determine availability or treat permission_required
as zero consumption.

| State | Interpretation |
|---|---|
| Present value + available | Data published by the indicated source |
| Present value + estimated | Data or derived value with scope or approximation described in note |
| Missing value + permission_required | Access is missing; this does not establish authorization to elevate |
| Missing value + unsupported | Source is disabled, absent, unimplemented, or invalid; read note |

Available does not certify physical accuracy, authenticity, atomicity, or
freshness. Estimated is not zero and does not justify ignoring attribution
limits. Absence alone does not distinguish a disabled source from an error;
the reason remains in note. Metric is a public aggregate, so callers can
construct inconsistent states. The helper prevents those states from being
treated as known; it does not modify or sanitize the entire SystemSnapshot.
The history view below does normalize inconsistent fields within its scope.

## Action errors follow a separate contract

An action result (`pcinsight/actions.hpp`) is not a metric. Check
`ActionResult::error` (or `if (result)` for `ActionError::none`), not the
text, PID, or presence of a backup. `message` and `Metric::note` are
diagnostics to display or record, not stable codes.

| `ActionError` | What the consumer should do |
|---|---|
| `none` | Report the operation's specific success. For example, a completed systemd job does not certify that the application is ready. |
| `not_supported` | Disable the operation in this context. Do not automatically substitute a more permissive operation. |
| `permission_denied` | Show the denial. Do not elevate automatically. |
| `not_found`, `stale_target` | Refresh inventory. Do not retry against a different target or reuse a PID without verifying identity. |
| `invalid_argument` | Review the request before resubmitting it. |
| `system_error` | Check state and diagnostics before retrying. An error does not guarantee that nothing changed. |

On Linux, if launch returns a direct child PID, the caller must reap it with
`waitpid` even if the error channel failed and the outcome is uncertain.
A backup can exist even if synchronization after a startup change fails.
Neither that PID nor that path turns an error into success. The
`linux-sdk-result-contract` test checks interpretation against the installed
SDK and covers two Linux denials without dispatching an action.

## Replacing ambiguous history

Include `pcinsight/app_history.hpp`. `AppHistorySample` is the common
read view, with `cpu_time`, `network_bytes`, and `notifications` as Metrics.
It contains id/name, period_start/period_end, and origin (linux_observed,
windows_observed, legacy_unverified, unspecified). A default instance
does not measure three zeros.

```cpp
pcinsight::SampleOptions options;
options.include_app_history = true;
auto snapshot = monitor.sample(options);
auto view = pcinsight::app_history_view(snapshot);
if (pcinsight::metric_is_known(view)) {
    for (const auto& app : *view.value) {
        if (pcinsight::metric_is_known(app.cpu_time)) {
            auto nanoseconds = app.cpu_time.value->count();
            // Observed CPU time, not a percentage or guaranteed past usage.
        }
        // Missing network or notification data: show unknown, not zero.
    }
}
```

The snapshot overload selects only the build's native report: Linux uses
linux_app_history and Windows uses windows_app_history. It never combines
platforms or falls back to legacy snapshot.app_history. Other backends
return unsupported. Overloads for the two typed reports project data
already received, including on another system, without calling an API
from the source OS.

This is a projection, not capture, merge, or summation. It preserves labels,
IDs, periods, known values, support, and notes for each field, and adds
origin. It keeps periods separate and in order without deduplicating by
name or path. IDs retain their source scope and do not establish package
identity. Do not add rows from multiple sources or simultaneous collections
as distinct resources without a consumer policy.

A known empty report list is valid and distinct from an absent report.
A field with a value and unsupported or permission_required loses the
value and keeps denial where appropriate. A field without a value but
marked available or estimated becomes unsupported. Inconsistency is
noted. Negative CPU time is rejected at the field level without discarding
other valid independent measurements. No counter is clamped or summed.

Per-conversion limits: 100,000 rows, 1 MiB per id/name, and 16 MiB of
cumulative text. IDs cannot be empty, text cannot contain NUL, period_start
cannot predate the epoch, and end must be at or after start. Invalid
metadata rejects the whole report without partial rows. Sorting compares
time_points rather than subtracting them, avoiding duration overflow.
Copies allocate memory. There is no budget for the bytes of all Metrics
and notes, no deadline, and no conversion of allocation exceptions into
data. The caller synchronizes access to the source.

## Explicit legacy handling without guessing availability

`LegacyAppHistoryEntry` is an alias for the original AppHistoryEntry, with
the same actual type, layout, and symbols. HistoryStore and
snapshot.app_history use the alias to mark the boundary. The original
AppHistoryEntry remains for source compatibility but is not the recommended
telemetry view. Native Linux and Windows leave that vector empty. The
SDK neither removes compatibility nor invents availability from a file.

HistoryStore still reads and writes exactly v1/v2, with no new flags.
`import_legacy_history(loaded.entries)` produces a legacy_unverified view
with identities and periods and **all** counters unknown, even when the raw
value is zero or positive. The old format did not store support or provenance;
that evidence cannot be recovered. The raw payload remains accessible in
loaded.entries. Negative raw CPU time or invalid metadata rejects import.
The import does not modify the input, overwrite the file, or silently
migrate counters.

AppHistorySample and LegacyAppHistoryEntry are not implicitly convertible.
Do not write a view to HistoryStore by replacing absent metrics with zero.
A caller supplying new known counters to the legacy API takes responsibility
for their semantics explicitly. Reading numbers from an old file cannot
recover that knowledge. The legacy type persists numbers, not confidence.

WindowsHistoryStore has a separate format that preserves availability for
its Windows report. This change does not add LinuxHistoryStore or
persistence for the new common type. H02 retains its legacy scope.
At this checkpoint, H04/H05/M08 still tracked actual network or notification
capture. Adding fields or this view did not close those tasks.

## Units and compatibility boundaries

For row continuity rules and GUI conversions, see
[Linux SDK units and identity](LINUX_SDK_UNITS_IDENTITY.en.md).

| API | Unit / contract |
|---|---|
| AppHistorySample.cpu_time | chrono::nanoseconds duration, observed CPU time from its source |
| network_bytes | Cumulative bytes from the source, not bytes/s or bits/s |
| notifications | Cumulative count, not a rate or retroactive history |
| period_start/end | system_clock capture periods, not a monotonic stopwatch |
| *_bytes telemetry | Bytes. The GUI chooses GB or GiB; no implicit scaling |
| *_bytes_per_second | Bytes/s over the provider's window |
| *_percent | Percentage points, not a 0–1 fraction; scope depends on the provider |
| *_ghz / speed_mts | GHz / MT/s; these are not interchangeable |

Percentages do not all use the same denominator. Per-process CPU normalizes
against host capacity, and some summed DRM reports can exceed 100. Retain
notes rather than applying a global clamp based on the unit name. Use
Sampler steady-clock timestamps for 60-second charts, not wall-clock
subtraction that can be affected by clock jumps.

Legacy compatibility pending 1.0 stabilization: MemoryInfo.total_bytes may
contain placeholder zero; prefer reported_total_bytes. Prefer
reported_vendor/reported_model over the CPU vendor/model text fields. Linux
process and network details provide per-field reports; shorthand text alone
does not establish availability. Linux emits legacy network totals only for
valid counters. An omitted row does not mean zero traffic. Startup, services,
sessions, and GPU have limits documented in their open provider tasks. A01
closes the documented model and history replacement contract, not the final
audit of every field, complete backend coverage, 1.0 compatibility, or the
N03 requirements matrix.

This is an additive 0.x change: an alias, helper, new header, type, and
functions. Existing structure layouts and v1/v2 bytes do not change.
Rebuild consumers that use new symbols and link the current library.
Rebuild requirements from earlier layout changes still apply. A05 (1.0
API/ABI) remains open.

## Tests

tests/app_history_api.cpp covers defaults, known zero versus absence,
support and notes at the field and outer levels, negative and inconsistent
values, bounds, periods, rejection without partial results, Linux and Windows
projection, conservative legacy import, and native report selection.
Static assertions verify that the alias is identical and the views do not
convert to legacy. Native Monitor opt-in and opt-out tests and an installed
consumer exercise the new public API. tests/history_store.cpp preserves
format, checksum, migration, merge, and retention coverage with the explicit
alias. It neither captures network or notifications nor writes automatically.

Verification on 2026-10-16: Linux Release 83/83 and Windows Release 103/103.
Packages were reinstalled, and rebuilt external consumers exited 0. Native
opt-in, opt-out, and origin selection were verified on both platforms. The
final Linux build had no warnings. Initial dependency clock-skew warnings
did not recur after rebuilding; no clocks or files were changed to suppress
them. A01 closes the documented criterion for replacing ambiguous history.
It does not remove compatible legacy types or declare absent measurements
resolved. This passing suite does not diagnose the earlier intermittent CPU
risk recorded in N03. C03/E05, hardware validation, and CI remain open.
