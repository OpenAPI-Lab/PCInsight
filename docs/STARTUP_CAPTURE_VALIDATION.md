# Same-source equality and independent native resource oracles

Checkpoint115,2026-10-29. I07 validation correction, not a new collector or a
claim that all N03 issues are resolved. No formula, kernel probe, counter kind,
threshold, publication format, privilege scope or capture activation changed.
The birth CPU metric note now explicitly says release-observed, with no
post-final-scheduler-switch guarantee. It is not a complete lifetime CPU claim.

## Reproduced failure and what it establishes

The old manual harness attached TWO independent native captures to its private
creator: a raw capture and the default public observer's own capture. It required
their CPU results to be exactly equal, although they came from separate probe
invocations/observation instants. This was not a valid exact source-to-facade
passthrough oracle. Each capture's own accepted final remains immutable.

The bounded pre-change repeat stopped after21 successful scenario checks. In
mode8 (failed exec, not just nonleader exec), kernel TGID1083 gave CPU-public
10441812ns versus CPU-private10581094ns, a139282ns difference. Both reported read0
and write65536. Log: work/startup-resource-differential-repeat.log. This run
FAILED; it must not be counted as a successful10-repeat validation. The earlier
mode10 failure in revision114 is retained too.

This proves an observed cross-capture CPU difference, not the precise kernel
interleaving that caused it. No on_cpu/scheduler-state evidence was captured.
The source does not wait for the target's final scheduler switch. In the
[Linux v6.6 exit path](https://github.com/torvalds/linux/blob/v6.6/kernel/exit.c),
exit notification precedes final scheduling; the
[scheduler switch code](https://github.com/torvalds/linux/blob/v6.6/kernel/sched/core.c)
separately documents on_cpu completion. These are reasons NOT to assume a frozen
post-switch total, not proof that this specific interleaving caused the139282ns.
No kernel/environment fault is asserted. N03 retains that unresolved distinction.

## Corrected acceptance topology

The manual private harness still uses two real kernel captures, with distinct
capture IDs. It now separates their roles:

1. The existing raw capture feeds a public observer through the existing PRIVATE
   source seam using REAL native context/start result/sample/stop callbacks.
   Exact CPU/read/write equality with the SAME immutable raw capture is still
   required, along with key, binding, file metadata, replay and close retention.
   Start/stop are checked exactly once. No synthetic resource values are injected.
2. A separate DEFAULT-CONSTRUCTED public observer owns its own native capture,
   exercising production start/poll/close wiring. Each retained identity is matched
   to the private child's post-reap oracle, not to another sensor's CPU value.
   CPU must satisfy the SAME original independent process-clock bounds (within
   the original1ms lower/20ms upper margins); IO must equal the child's exact
   private procfs read/write oracle. First-exec identifiers must equal private
   fstat evidence. Disabled/auto-reap finals stay absent, and the observer's OWN
   replay/close values must be EXACTLY unchanged. No tolerance was increased.

The pure startup-birth-observer fixture additionally mutates an already published
CPU final by ONLY1ns. That source rewrite must fail transactionally, stop the
source and preserve the last accepted row/binding. Cross-capture variation is
not permission to mutate a final in one capture. Known zero remains known zero.

The fixtures and native harness do not autoassociate candidates, launch desktop
entries, promote startup cause or install an early recorder. Private eBPF scope,
timeouts, child reaping, temporary-file cleanup and FD census remain in force.
I07 still requires reliable app/launch receipt/cause and time-window integration.

## Evidence

Linux143/143 CTest without skips,20 final pure fixture repetitions; MSVC10 pure
fixture repetitions, installed Linux SDK/consumer exit0. Full native13 scenarios
and five4-scenario exec repetitions (20 more) passed with both exact-source and
independent-native checks, including cleanup and FD census. No new C++ warnings;
MSVC's assert-enabling D9025 remains intentional. Logs: work/startup-single-source-*.
Results are also recorded in SEGUIMIENTO.txt, not inferred from compilation.
No universal Linux/kernel support, remote CI, total Windows support or N03 closure
is implied. The historical failed cross-capture runs remain available separately.
