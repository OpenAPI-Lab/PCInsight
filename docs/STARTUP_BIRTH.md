# Selected-creator birth-before-wake capture (internal)

Update113: STARTUP_FIRST_EXEC.md documents opt-in first successful exec evidence
from birth-enrolled groups, no automatic app/desktop/startup attribution.

Update112: STARTUP_BIRTH_OBSERVER.md exposes this sensor in the SDK through
LinuxStartupBirthObserver with per-child explicit association; no app inference.

Update111: STARTUP_BIRTH_PID.md adds optional, namespace-qualified local TGID at
birth. Kernel TGID is still distinct; this is not app attribution/live selection.

Update110: optional creator-reaped final counters now connect to these births
without per-child userspace selection. STARTUP_CHILD_FINALS.md covers the scope,
matching, final publication and remaining limits. Birth-only is still default.
The109 checkpoint below describes the original birth-only component.

Revision109, 2026-10-29. LinuxStartupBirthCapture records new process groups
created by an explicitly selected creator TGID. Each child need NOT be discovered
in procfs or pidfd-selected from user space before it exits. The internal birth
record survives child and creator reaping. This is not final child CPU/IO capture
or a claim that I07 is finished.

## Mechanism and scope

An optional FENTRY probe on wake_up_new_task(task_struct*) runs before first
enqueue/wake. The hook's ordering is visible in
[Linux v6.6 scheduler core](https://github.com/torvalds/linux/blob/v6.6/kernel/sched/core.c).
BTF validates the exact void(task*) definition and the existing nested accounting
layout. Bootstrap still uses the selected network pidfd join/schema; that is an
explicit compatibility limitation, not a universal Linux capability claim.

The current creator TGID is filtered BEFORE new-task or map access. Its leader
start_boottime and initial-namespace real UID must match the authenticated pidfd
join. The new task must equal its group_leader, with live=1/nr_threads=1, so new
threads do not consume process-group slots. Captured child identity uses kernel
TGID/UID/start_boottime. A creator UID change can move future calls outside scope;
no empty snapshot certifies that the creator made no processes.

The record contains kernel monotonic birth-observation time and sampled cumulative
scheduler runtime/read_bytes/write_bytes AT BIRTH. Genuine zeroes are measured
zeroes, not missing data. They are not final usage or an initialization-completion
claim. No mapping to host steady-clock or local PID/start_ticks is invented.

"Created by" means the task running this hook belongs to the selected TGID.
It is NOT proof of real_parent (e.g. CLONE_PARENT), executable/exec success, XDG
startup attribution or automatic startup. Children creating further children are
outside this first scope. There is no process-tree or IO summation: waited-child
accounting could otherwise be counted twice. No environments or argv are read.

## Capacity, publication and ownership

Two ARRAY maps hold control and1..64 fixed records (default16). A fetch-add reserves
a unique slot, with an early cap check bounding concurrent overshoot far below
uint64 wrap. Each slot publishes state LAST with atomic exchange. Partial writes
never become a birth. Sticky quality bits:1 read/schema/overflow failure,
2 capacity loss,4 duplicate slot. Existing valid births remain visible with
Support::estimated if quality is partial. It never reports all children captured.
Sampling brackets each whole-record copy; a crossing publication is counted as
publishing and skipped until a later sample, not synthesized.

Start requires explicit authorization, owns a CLOEXEC creator pidfd and authenticates
the creator before/after attachment. Calls are serialized by the caller in the same
FD table. Boot/observer/caller/euid guards surround reads; a forked copy closes its
inherited references without map writes. Stop/failure/destructor release owned
link/program/maps/pidfd; kernel object lifetime follows all references, so callers
must close/exec inherited fork references rather than retain them indefinitely.
No worker, external collector, install rule or startup activation is added.

## Verification and remaining work

The portable fixture VM covers creator filtering, identity mismatch, thread
exclusion, known-zero decoding, slots/capacity loss, duplicate/read/overflow flags
and bounded bytecode. It does not replace the kernel verifier. Additional BTF
fixtures cover the valid prototype, every truncation and ambiguity/bad return.

Manual target pcinsight-linux-startup-birth-native is NOT in CTest; it requires
--authorized-private-birth and the prior PRIVATE-process temporary eBPF permission.
Only a private selected creator and its two private forked groups are captured.
It creates/reaps both children BEFORE giving their local PIDs to the observer,
which verifies procfs absence. A private thread and unselected sentinel do not
consume slots. Capacity4 retains two births with quality0; capacity1 retains one
and reports loss2. A forked observer copy is denied, creator reaping preserves
records/replay, and descriptor census returns to baseline. No real apps/global
capture or login configuration is touched.

The sensor is INTERNAL, not connected automatically to Monitor/recorder/history.
Remaining I07 work includes child final accounting without per-child userspace
selection, namespace/local identity mapping, safe attribution/integration, further
descendant scope and reliable startup cause. It does not recover processes created
before this attachment, nor retroactively identify arbitrary already-gone apps.
