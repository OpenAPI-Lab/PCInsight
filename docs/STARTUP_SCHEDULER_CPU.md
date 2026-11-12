# Compatible running/final CPU in selected startup capture

Revision108, 2026-10-24. CPU final recovery is now integrated in the optional
recorder path, not inferred by subtracting procfs ticks from scheduler runtime.
This completes the compatible-CPU component for preselected, covered processes;
I07 still lacks never-preselected lifetimes and reliable automatic-startup cause.

## Counter identity and native sampling

StartupProcessSample now carries StartupCpuCounter: linux_procfs_ticks (legacy
default) or linux_scheduler_runtime. measure_startup_impact rejects different
or invalid kinds before subtraction. Observer/recorder preserve that identity;
an injected reader changing kind cannot overwrite a retained valid sample.
Recompile consumers for the new sample layout; no old binary-layout ABI promise.

`read_startup_scheduler_process_sample(pid,start_ticks)` uses the POSIX process
CPU clock from clock_getcpuclockid/clock_gettime. It independently validates the
Linux process CPUCLOCK_SCHED encoding. This samples group scheduler runtime,
including released threads and excluding waited-child CPU, matching the selected
final provider's signal.sum_sched_runtime + leader.se.sum_exec_runtime.
This relationship is grounded in the Linux clock's group sampling code and the
libc clock selection, not conversion of USER_HZ to supposedly equivalent counters.
See [Linux v6.6 POSIX CPU clocks](https://github.com/torvalds/linux/blob/v6.6/kernel/time/posix-cpu-timers.c)
and [glibc clock_getcpuclockid](https://github.com/bminor/glibc/blob/master/sysdeps/unix/sysv/linux/clock_getcpuclockid.c).
The selected final-release source/limits remain in STARTUP_FINAL_RESOURCES.md.

An owned CLOEXEC pidfd must remain non-exited before/after the read. Genuine
procfs, matching caller PID namespace and PID/start identity are bracketed as
in the existing sampler. PID reuse cannot publish another process's CPU clock
while the original pidfd is exited. Poll errors are not labeled process exits.
The sampler never sends a signal, changes scheduling, launches an app or installs
an external utility. The running clock itself needs no BPF; permissions/kernel/
libc support can still make it unavailable. Missing/overflowing CPU stays unknown.
Timespec bounds and nanosecond conversion are checked with integer arithmetic.

## Recorder integration and fallback

The existing opt-in `enable_privileged_exit_disk_recovery` keeps its legacy name.
Since108 it also attempts compatible CPU recovery using the SAME selected final
probe, with no extra global tracing. Defaults remain OFF. Before registering an
accepted hint, it tries a fresh compatible CPU baseline with matching identity,
context and chronology. A failed/unknown compatible baseline keeps legacy procfs
mode, including disk-only recovery and CPU LOWER BOUND. No mixed subtraction.

The chosen reader kind is retained for ALL registered identities, including
targets skipped by the BPF concurrent limit or with failed attachment. Subsequent
polls do not silently switch to ticks. On final recovery, a nondecreasing known
scheduler total replaces the last CPU only for a scheduler baseline. Unknown,
decreased or overflowing final CPU retains the last CPU lower bound explicitly.
Unknown counters never turn into measured zero. Valid zero remains a value.

Every recovered process still closes via finish_partial_process: gap=true and
window_complete=false, even with final CPU+disk. A high observed lower bound can
be reported, but low/medium and automatic-startup origin are NOT certified.
Selection/attachment happened after discovery, clocks are not atomic wall-time
boundaries, and early global startup coverage is not established by final totals.
No late final record is admitted after the phase deadline.

Archive v1/v2 formats stay unchanged: measurements are already DELTAS, not
resumable samples, and their CPU notes now identify the counter and recovered or
lower-bound provenance. No CPU kind is inferred from loaded rows for a live
baseline. V2 diagnostic names exit_disk_* still count DISK recovery, not successful
CPU recovery; inspect each CPU metric's presence/support/note. Old v1 and default
procfs sampling remain supported. No startup activation or recorder installation.

## Verification

Portable cases cover kind mismatch/invalid kind, compatible/zero/unknown/decreased
CPU, unavailable/foreign baseline fallback, changing reader kind, skipped-probe
reader continuity and disabled mode without CPU-clock calls. Native non-BPF
measurement includes a worker released BEFORE baseline, wrong start ticks,
independent high CPU work and already reaped PID rejection.

Manual native target remains outside CTest and requires the existing bounded
private-process authorization. Seven private scenarios use actual POSIX clocks,
pidfd/BPF final providers, recorder/observer and archive roundtrips. Six covered
exits recover CPU using exactly final_runtime - captured_scheduler_baseline;
one internal auto-reap remains uncovered/partial. The CPU-only high case exceeds
1s CPU with disk below300000B, so it cannot pass classification by disk alone.
Single/multithread/released-worker/waited-child/4MiB IO regressions, flags/notes/
diagnostics/0600 and descriptor cleanup remain checked. This is not proof of GIO
autostart attribution: the native selected census is explicitly private/injected.
