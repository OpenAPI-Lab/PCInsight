# Incremental startup impact observation

Completed on 2026-10-14: StartupImpactObserver (startup_observer.hpp)
is poll-driven and uses explicit associations. It registers a baseline
per process, accepts successive samples, closes records, and returns
results and coverage status. It does not discover associations, launch
apps, or create a thread, timer, daemon, or CLI. Monitor.startup_entries.impact
remains not_measured: this API alone neither completes an automatic startup
observer nor invents impact for the entire boot.

```cpp
pcinsight::StartupImpactObserver observer(std::chrono::seconds(60));
auto baseline = pcinsight::read_startup_process_sample(pid, linux_start_ticks);
if (baseline) observer.register_process(entry_id, *baseline.value);
// The caller's scheduler or GUI periodically calls observer.poll().
auto rows = observer.observations();
// At the end of the phase, call observer.close(); it does not take a hidden final sample.
```

Each PID/start_ticks/native_context identity can be registered only once,
even if another entry_id attempts to claim it. Multiple processes for one
ID remain separate rows. **Do not sum them automatically**: kernel I/O
can include waited-for children and duplicate those children's rows.
There is no per-app aggregation or validation of entry_id, Exec, or
cgroups. A reused PID with a new token or context does not inherit the
old record's counters. Polling with stale_target or not_found closes
that record.

The per-process window starts at its baseline timestamp, defaults to
60 seconds, and must be positive and at most one hour. The record closes
automatically when it accepts a sample whose interval reaches the window.
The real interval is retained, including poll overshoot; nothing is
interpolated and exactly 60 seconds of consumption is not promised.
window_complete means the duration was reached without gaps in accepted
observations, not complete observation of the boot or app.

A failed read does not advance sample timestamps. Since the 2026-10-14
fix, poll also checks the clock at the end of every attempt. Once the
baseline-plus-window deadline is reached, it admits that final attempt
and closes the record even if the read fails, throws, or returns no valid
sample. It does not read the record again. Clock-based closure does not
fill samples or certify window_complete. Without a final valid delta
covering the window, low and medium remain unknown. Observed partial high
is retained as a lower bound, and last_error is not cleared.

poll() uses steady_clock::now after each read. poll(now) accepts an explicit
time from the same clock for deterministic tests or replay. The caller
must use increasing times consistent with samples, not system_clock or
Unix epoch. A slow read can finish after the deadline: there is no I/O
cancellation, internal timer, or guaranteed maximum overall poll time.
Processes with different baselines expire independently, and new
associations are accepted until close(). observe_sample ingests data; it
is not a scheduler and does not read the real clock. With no new samples,
call poll or close to finish. No work runs automatically if none of
poll/observe_sample/close is called. close is idempotent, performs no more
I/O, and prevents new associations. Use a new instance for another phase.
Automatically closed rows are never sampled again.

A short window or a gap cannot establish low or medium. Observed high
stays as an estimated lower bound for that process and interval. Early
closure, process disappearance, reader errors or exceptions, decreasing
or reset counters, and missing or protected metrics leave incomplete
coverage. sampling_gap remains set even after a read recovers and is
not hidden by a later low result. last_error describes the last attempt
and can return to none without clearing the gap. permission_required is
preserved in an unknown impact result when applicable. The observer
neither invents zero nor reconstructs unobserved activity after exit.

Snapshots return sample_count (including the baseline), closed,
window_complete, sampling_gap, last_error, and an optional measurement.
A baseline alone yields no delta. Observed raw CPU and I/O remain
available even if classification is unknown; those partial data do not
describe the entire app or tree. Classifier, policy, and native
provenance retain the limits of [startup impact](STARTUP_IMPACT.en.md).

A mutex serializes the interfaces. poll and readers run under the same
lock, so snapshots can wait on slow reads. An injected reader supports
tests and custom adapters. It must not reenter the observer or wait for
a thread that needs its lock. Reader exceptions become a per-record
system_error, and poll continues with other records without copying
sensitive text. The caller coordinates destruction and lifecycle.
There is no overall poll or filesystem deadline. observe_sample allows
explicit ingestion and does not apply invalid identity or time. Sample
copies are prepared before commit so copy failure does not corrupt state.

Limits are 4,096 records, 8 MiB of retained strings plus bounded fixed
overhead, a 32 KiB ID, 1,024 B context, and 4,096 B per note. Exceeded
limits, invalid policies, and invalid duration are rejected without silent
eviction. Increasing chronology, timepoint or interval overflow, and
counter monotonicity are checked. Invalid construction throws
invalid_argument. Operations return ActionResult or poll counters for
attempted, updated, and failed. A failed final attempt at the deadline
counts as attempted and failed. Later polls of that record neither
increment counters nor call the reader. There is no added persistence,
historical retention, or security association.

Linux Release passed 29/29 and Windows Release passed 40/40. The header
was installed and external consumers ran. Portable tests cover baselines,
partial deltas, automatic closure with overshoot, early and idempotent
closure, double attribution, changed PID or context, denial, gaps,
recovery, disappearance, reader exceptions, counter reset, unknown
values, limits, policies, timepoints, and concurrent snapshots. Linux
polled the test's own process using Monitor identity and native counters,
closed the window, and did not read it again. No real startup or apps
were modified. This work provides no native Windows reader for these
counters; the Linux reader returns unsupported there.

The fix was verified with Linux Release 31/31 and Windows Release
42/42, installed packages, and external consumers. Deadline regression
tests cover persistent denial before or at the limit, a throwing reader,
a successful final read, a final error after partial high, staggered
baselines, a maximum injected time without subtraction or overflow,
the real clock, and no retries after closure. Both variants were
exercised through the installed API.

Remaining work includes reliable automatic app/process correlation,
early login observation, short-lived processes and descendants,
aggregation without double counting, persistence, and consistent
inventory linking. This does not close the parent startup-impact task.

The later [startup overlay](STARTUP_OVERLAY.en.md) implements explicit
linking, not automatic discovery or registration or certified early capture.

[Child exit sampling](STARTUP_CHILD_EXIT.en.md) adds a final non-reaping
read for the consumer's own children and finish_process to apply it and
close one identity without closing the others. Universal short-lived
capture and already-reaped processes remain open.

[Startup store](STARTUP_STORE.en.md) implements explicit saving and loading
of closed observations as historical captures, not resumable checkpoints
or a boot recorder.
