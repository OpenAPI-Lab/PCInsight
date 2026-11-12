# Selected Linux final accounting (internal)

Update108: recorder integration now uses final scheduler CPU with a matching
POSIX process CPU clock baseline; STARTUP_SCHEDULER_CPU.md. No conversion/mixing
with procfs ticks. All selected-release coverage limits below remain in force.

Revision105, 2026-10-24. `LinuxStartupResourceCapture` is an INTERNAL opt-in
provider, not enabled by Monitor, LinuxStartupRecorder or SDK construction.
Revision106 adds explicitly enabled DISK-only integration in LinuxStartupRecorder;
defaults still do not attach anything. See [STARTUP_DISK_TAIL.md](STARTUP_DISK_TAIL.md).
It does not install anything, start apps, reap children or prove autostart origin.

## Measurement contract

The FENTRY probe on `release_task(task_struct*)` reads the TARGET argument, NOT
the current reaper's PID or UID. It requires the selected initial-namespace
TGID, real UID and leader start_boottime, the argument to equal group_leader,
signal.live.counter=0 and signal.nr_threads=1, checked again after reads.
BTF validates the prototype and complete nested field types/widths/offsets.

Before the leader is accumulated in signal, the probe adds exactly once:

- task.se.sum_exec_runtime + signal.sum_sched_runtime, in scheduler runtime ns;
- task.ioac.read_bytes + signal.ioac.read_bytes;
- task.ioac.write_bytes + signal.ioac.write_bytes.

All additions reject uint64 overflow. IO is Linux group ACCOUNTING, including
IO of already waited children propagated into signal.ioac; it is NOT exclusive
physical IO caused by the app, and must not be added again to descendant IO.
Cancelled writes are not subtracted. CPU does not add waited-child CPU. The CPU
counter is NOT procfs utime+stime in USER_HZ: never subtract the existing
StartupProcessSample CPU baseline from it. This provider does not currently
have a matching running baseline/public observer integration.

The timestamp is kernel monotonic observation time at release, NOT time of
last CPU use, exit entry or a std::steady_clock/time-namespace conversion.
Zero counters are valid measurements; an empty optional is unknown/waiting.
Maps use one-shot final publication, bracketed reads and sticky error flags.
Duplicate publication, read errors, overflow, caller/fork/euid/boot/observer
changes fail closed. Owned CLOEXEC pidfd/map/program/link close on stop/failure/
destruction; no map reset on inherited fork references.

## Explicit coverage limits

The kernel tested does not expose `__exit_signal` in BTF. `release_task` FENTRY
only sees DIRECT calls, not the function's internal repeat for an auto-reaped
leader. Such exits can be missed. Tracing restrictions, missing BTF fields,
CONFIG_TASK_IO_ACCOUNTING, incompatible kernels/prototypes or permissions can
make the provider unavailable. Validated BTF is not proof load/attach succeeds.
Bootstrap reuses the selected network pidfd identity join and thus also requires
that join's validated kernel hooks and permissions.

Pidfd exit alone does not publish totals. A parent may keep a zombie indefinitely;
until a matching direct release is observed, resources remain UNKNOWN and the
capture remains active. No automatic timeout-to-zero and no wait/reap.
The caller stops at its own deadline and records incomplete coverage.
Only processes explicitly selected WHILE ALIVE before attachment are observed,
not processes already gone or all startup apps/descendants.

The reference accounting/release order and internal repeat are in
[Linux v6.6 kernel/exit.c](https://github.com/torvalds/linux/blob/v6.6/kernel/exit.c).
This is a version-scoped kernel interface, not a portable stable ABI promise.

## Verification

Portable fixtures extend `linux-startup-exit`: bounded BTF/truncations/missing
fields/width/sign/prototype, decoder/known zero/unknown/identity, and a test-only bytecode interpreter
exercising filters, additions, overflow, read failure and duplicate publication.
The interpreter is NOT a replacement for kernel verification.

Manual Linux-only target `pcinsight-linux-startup-resources-native` is NEVER
registered in CTest. It requires `--authorized-private-resources`, private
grandchildren and temporary privileged probes, with alarms and descriptor census.
It compares CPU to independent process CPU-clock bounds (not claimed exact
counter equivalence) and IO to private /proc/self/io before exit. Modes cover
single thread, leader-first exit, previously released worker and waited-child IO.
An additional SIGCHLD-ignored, leader-first case exercises the internal repeat:
procfs disappears/pidfd exits, but resources must remain unknown, never zero.
Before owner reaps, pidfd exit must coexist with unknown totals; after owner ACK,
/proc is absent and retained totals/replay must still match. Unselected sentinel
and fork-copy rejection must not mutate the selected parent's capture.

I07 stays open: matching counter baselines, public recorder integration, early
unselected lifetimes, reliable automatic-startup attribution and durable coverage
diagnostics remain. No new task IDs or partial closure credit.
