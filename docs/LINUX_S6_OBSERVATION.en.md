# s6: raw conditions tied to the control target

Revision 136/K03. API in `pcinsight/linux_s6.hpp`: `LinuxS6Control::read_record()`,
`observe_linux_s6_condition()`, and `LinuxS6Observation`.

```cpp
auto opened = pcinsight::LinuxS6Control::open("/service/ejemplo");
if (!opened.value) { /* show note/support */ return; }
auto op = std::make_unique<pcinsight::LinuxS6Observation>(*opened.value);
auto delivered = op->start(pcinsight::ServiceAction::restart, consent);
// If delivered, schedule poll() from the GUI's timer and read report().
// condition_observed does NOT mean the job completed or the service is alive or ready.
```

## Target and conditions

read_record() checks the handle's context and pins before and after reading
the raw 35-byte record relative to the pinned service. Replacing control or
supervise invalidates the read. Changing the original absolute alias does
NOT redirect it to the new service: like request(), it follows the pinned
target. It does not probe for a lock or reader, authenticate the peer, make
another request, run exec/CLI/scripts, send signals, or elevate privileges.
An available file with no reader remains a historical record, never proof
of a live supervisor.

The pure function validates the action, both TAI nanosecond fields, and
whether the PID fits the eligible Linux range (0..INT32_MAX). The raw decoder
retains the full u64; that does not imply lifecycle eligibility or local
identity. The rules for the subsequent record are:

| Action | Raw condition |
|---|---|
| start | Positive PID, want_up, neither finishing nor paused |
| stop | Zero PID, want_down, neither finishing nor paused |
| restart | Start condition and changed_at_tai64n strictly later than baseline |

The 12 TAI bytes are compared lexicographically, without conversion to Unix
time, uptime, or steady-clock time. A different PID with the same timestamp
is insufficient; the same PID number with a later timestamp can satisfy the
condition without proving identity or cause. Changing only readystamp or
ready_reported does not confirm restart. Ready is ignored because it can
be true while down. The finishing check avoids confusing the finish process's
PID with the daemon's. The down/want_up gap does not satisfy stop. If TAI
moves backwards, restart may not be observed; no confirmation is invented.

**An observed raw condition does NOT prove liveness, file freshness,
readiness, or that the request caused the state.** Start/stop may already
satisfy the baseline, and the same unchanged record may be observed later.
It is not reported as a causal transition or equivalent to s6-svwait,
events, or jobs. delivery.completed is ALWAYS UNKNOWN, even when
target_condition_observed=true.

## One request, caller-driven polling

Construction and report perform no I/O. start requires consent before reading,
a valid action, a timeout of 1–60,000 ms (7 s by default), and an available,
eligible baseline. An absent, estimated, or ineligible baseline sends nothing.
Revision 134 direct control remains separate. Denial or invalid arguments
leave the instance idle; other terminal outcomes consume it.

steady_clock measures from before the baseline. If that read exhausts the
window, nothing is delivered. After request, counts are retained without
additional allocation. The native source prepares results before writing,
and the private seam must obey that contract. No retry, worker, timer, sleep,
busy wait, or undo.

Time passing alone does not change report(): the caller must call poll()
to advance observation or timeout. On explicit poll, a valid nonmatching
record yields false/pending; unavailable, estimated, or ineligible results
yield UNKNOWN with no substitution of the last good value. A poll that starts
after the deadline does not read. A late result is kept in latest but does not
confirm the condition within the window; a positive read finishing exactly
at the deadline counts. Timeout does NOT prove the request failed and does not
cancel I/O. A backwards steady clock or an exception ends in failed with
delivery data preserved; partial delivery and errors are not retried.
condition_observed freezes the report and sampled_at; further polls do not
read. close abandons idle/pending and releases sources without a final read,
stop, or service cancellation. The destructor does not act on the service
either. The host coordinates threads/lifetime/fork/setns/credentials/FDs;
private callbacks must not reenter. There is no atomic snapshot, proof of
peer identity, or guarantee of detecting all ABA changes.

## Evidence and remaining work

Deterministic per-instance tests cover consent/arguments/baseline without
delivery, one request, suppression of false completion, timestamps/PID/
readystamp/flags/gaps, ineligible u64/TAI, unavailable/estimated reads, exact
and late deadlines, overflow/backwards clock, exceptions/partial delivery/
errors with counts, abandonment, and frozen reports.

Private Linux FIFO/status fixtures observe start/restart/stop as patterns.
Changing an alias to another tree does not change the read source. Finish,
readiness-only changes, and the gap are rejected. A missing baseline does
not write; a replaced control invalidates the read; no FDs leak.
**The test changes records, not a real s6-supervise.** It does not establish
real acceptance, liveness, or causation. MSVC covers portable conditions,
clock handling, and fallback only. The SDK consumer links the new symbols.

At this checkpoint K03 remains open for integration, real acceptance, and
other init systems. OpenRC revision 132 scripts remain deferred without
authorization, and the I07 observer is unchanged. Next: read-only SysV
inventory. Do not infer an exception to execute control scripts.

Primary source reviewed for flags and timestamps:
[s6-supervise.c](https://raw.githubusercontent.com/skarnet/s6/master/src/supervision/s6-supervise.c).
These conditions and the window are PCInsight's own contract, not a replica
of the supervisor's state machine.
[Control](LINUX_S6_CONTROL.en.md), [raw 35-byte record](LINUX_S6.en.md),
[inventory](LINUX_S6_INVENTORY.en.md).
