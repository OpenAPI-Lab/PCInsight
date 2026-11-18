# Observation after runit requests

Revision 129, part of K03. Public API in `pcinsight/linux_runit.hpp`. It does
not integrate automatically with Monitor or replace the systemd provider.

`LinuxRunitTransition` retains an explicit handle and allows **one request**.
`start(action, authorized, timeout)` requires consent and a valid available
initial reading before writing. It accepts 1–60,000 ms (7 s by default),
measured with steady_clock from before the initial read. If the read consumes
the window, nothing is sent. Denial or invalid arguments do not consume the
instance. Requests and partial deliveries are not retried. Construction
does no I/O.

Afterward, the caller schedules `poll()` at its own cadence, for example
from the GUI. There is no thread, timer, busy wait, or script/CLI execution.
Native reads follow the [runit contract](LINUX_RUNIT.en.md). The deadline
limits which samples are accepted; it does NOT cancel I/O calls or guarantee
maximum latency. The host must still serialize context changes and forks as
required by the handle.

| Action | Predicate on the supervisor's reported state |
|---|---|
| start | run, nonzero PID, want_up, neither paused nor term_sent |
| stop | down, zero PID, and want_down; the down/want_up gap is not enough |
| restart | Start predicate and a run TAI64N timestamp strictly later than the initial one |

TAI is compared lexicographically over the 12 validated bytes, with no Unix
conversion or mixing with steady_clock. A different PID with the same
timestamp does NOT confirm restart. A later timestamp with the same PID does
satisfy the predicate; the reported PID is not a local kernel identity. If the
TAI clock moves backwards, the predicate may not be observed; no confirmation
is invented. `finish` with zero PID is valid but is not down.
`observe_linux_runit_target()` exposes the same pure rule with no I/O; calling
it does not send requests.

`baseline_satisfied` describes only the initial state. Even if it already
satisfies start/stop, the request is delivered and a later sample is required.
`target_state_observed=true` means **the reported state was observed within
the window**, NOT that the request caused it, readiness, execution of the
check hook, successful work, or a guarantee that the process remains alive.
`delivery.completed` stays UNKNOWN even with a positive observation. Nor
does it authenticate the FIFO peer or convert the reported PID into the
observer's namespace or identity.

A valid nonmatching read yields known false/pending. An unavailable read
yields UNKNOWN without reusing the last good value. A read that finishes
after the deadline can remain in latest but does not confirm the target in
the window. A positive read finishing exactly at the deadline counts.
A poll started after the deadline does not read. Timeout does not prove
service failure or undo delivered bytes. A backwards steady_clock or an
observation exception ends in failed while preserving available delivery data.

`report()` only copies the report; it performs no I/O. Once in `observed`,
the report is frozen and another poll does not reread or resend. `close()`
abandons idle/pending and releases references without a final read, signal,
cancellation, or undo. The destructor also does not act on the service.
The caller coordinates object lifetime across its threads.

Conceptual example: keep the object alive while the GUI schedules polling.

```cpp
auto opened = pcinsight::LinuxRunitService::open("/service/mi-servicio");
if (!opened.value) { /* show availability/note */ return; }
auto operation = std::make_unique<pcinsight::LinuxRunitTransition>(*opened.value);
auto result = operation->start(pcinsight::ServiceAction::restart, consent);
// Only on success: call operation->poll() from the host's timer.
// Inspect report().state; do not interpret delivery as readiness.
```

## Evidence and limits

`tests/linux_runit_transition.cpp`: private per-instance clock injection,
consent/arguments/baseline without writes, single delivery, retained counts,
no false completion, timestamps/PID/flags, finish and auto-restart gap,
exact and late deadlines, unavailable reads and clock reversal, abandonment,
and frozen reports. The Linux fixture uses a private directory, FIFOs, and
status file. It observes start/restart/stop through the native handle and
checks for descriptor leaks. It does not run runsv.

Validation against a real runit supervisor remains pending; it was not
installed in the tested environment. These fixtures are NOT real acceptance.
At this revision, K03 remains open for that validation, other init systems
(OpenRC/s6/SysV), and integration. No utilities are installed, real services
changed, startup apps launched, or I07 scope expanded.

Primary protocol references. The predicate above is this library's observation
contract, not a reproduction of sv checks or readiness:
[runsv.c](https://raw.githubusercontent.com/void-linux/runit/master/src/runsv.c),
[sv.c](https://raw.githubusercontent.com/void-linux/runit/master/src/sv.c).
