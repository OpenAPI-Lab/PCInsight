# Optional selected exit disk recovery in the Linux recorder

Update108: the same optional path now recovers CPU when a compatible scheduler
baseline succeeds; see STARTUP_SCHEDULER_CPU.md. Legacy procfs fallback still
retains CPU as a lower bound. The106 disk integration described below remains
valid; it is no longer a universal statement that CPU final is unused.

Revision106, 2026-10-24. The recorder can now consume the selected final-accounting
provider105 and retain disk deltas after procfs disappears. This is an actual
recorder/observer/archive integration, not another standalone schema prototype.
It does NOT complete I07, certify automatic startup, or mix CPU counter kinds.

## Opt-in and scope

The library caller explicitly sets BOTH estimated-GIO consent and:

```cpp
options.enable_privileged_exit_disk_recovery = true; // default false
options.max_exit_disk_captures = 8;                 // concurrent bound1..32
```

There is no automatic elevation, installation, worker, startup activation or
external collector utility. Disabled means no pidfd/BPF attachment from this
feature. The standalone recorder CLI still uses the unprivileged default;
the new option is currently an embedding-library API only. Callers are
responsible for consent/scope before enabling privileged selected tracing.
Native tests attach ONLY to private test grandchildren, never all visible apps.

Each accepted candidate gets at most one attachment attempt, while alive, if a
slot is free. Full capacity increments skipped; no later retry for that identity.
Attach failure increments unavailable but does not disable normal procfs sampling.
A successful selected reader owns its pidfd/map/program/link. Closed observations,
deadline, explicit close, failure, destruction and moved-from replacement release
ownership. No hidden retry loop or extension of the observation phase.

## Recovery semantics

Native creation checks procfs PID/start/context before and after authenticated
kernel/procfs pidfd joins and attachment. The reader then inherits105's target
identity, observer/boot/euid/fork guards, one-shot publication and coverage limits.
It checks read_bytes+write_bytes overflow before exposing a total.

On poll, a retained final record supplies cumulative DISK only. The recorder
uses the latest observed procfs CPU as a LOWER BOUND, not scheduler-runtime ns.
It calls `StartupImpactObserver::finish_partial_process`, which always closes
with sampling_gap=true/window_complete=false, even after a full elapsed window.
Counter notes preserve partial-CPU and Linux group-IO semantics in measurements
and the existing archive. A high observed lower bound survives; low/medium cannot
be certified. Unknown CPU stays unknown; recovered zero disk is valid only when
it was actually published. A disk decrease never replaces a larger prior counter.
Group IO includes already waited children: do not add their IO a second time.

While an attached process has vanished but no final record is available, polling
records a gap and keeps the observation open until its deadline. Exit/pidfd alone
does not mean zero disk. A non-covered internal auto-reap release may remain
unknown forever; the host eventually closes it as partial. After the phase deadline
the recorder does not admit a newly read final record; an individual recovery read
that crosses that deadline is not admitted either. No conversion of kernel release
timestamps into host steady-clock timestamps is assumed. Sampled time is host
read time, not exact process exit time.

The phase starts BEFORE initial scanning/attachment, unlike103's warm-up rule.
Slow native setup may consume the phase and make start return already closed.
I/O is not cancellable; this bounds attribution, not syscall wall-clock duration.

## Evidence and remaining work

Portable tests cover disabled/no side effects, permission/exception/unusable
source, pending/uncovered, cap/skips/invalid bounds, known zero, high disk with
unknown CPU, resets, context failure, late publication, slow setup, cleanup,
and explicit partial closure after a complete elapsed window.

The manual native `pcinsight-linux-startup-resources-native` remains OUTSIDE CTest,
requiring `--authorized-private-resources`. A private injected census/association
selects ONLY the test grandchild (observer receives ECHILD); this is not native
GIO/automatic-startup proof. Actual native pidfd/BPF readers, recorder, observer
and StartupImpactStore are used. Modes0/1/2/3/5 recover after owner ACK/reaping and
procfs disappearance: single thread, leader-first, released worker, waited child,
and4MiB high-impact IO. Mode4 reproduces an uncovered internal auto-reap: no final
disk is invented. All cases save/load private0600 files, retain partial flags and
notes, and finish with the original descriptor census. Temporary IO files are
unlinked and private archives/directories removed by the test.

No permanent startup entries or real apps/services are activated. Diagnostic
counters exit_disk_* are serialized since107 in optional archive v2 metadata
(STARTUP_DIAGNOSTICS.md); v1 metadata remains unknown. I07 still
needs compatible CPU baseline/final integration, never-preselected short lifetimes,
reliable automatic-startup origin. Aggregate durable diagnostics are now included,
not proof of complete discovery. Existing
untrusted GIO hints do not become reliable merely because final disk was recovered.
