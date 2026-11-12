# Birth-to-final counters without child userspace selection (internal)

Update115: [capture validation](STARTUP_CAPTURE_VALIDATION.md) separates exact
same-source passthrough from independent native CPU-clock/IO oracles. CPU is
release-observed scheduler runtime, not guaranteed post-final-switch lifetime
CPU. The source does not synchronize with that switch; independently attached
captures need not return bit-identical CPU. No probe/arithmetic change.

Update111: STARTUP_BIRTH_PID.md adds namespace-qualified birth PID and flag32;
private record size15 u64. The110 final-counter offsets and semantics are intact.

Revision110, 2026-10-29. LinuxStartupBirthCapture now has an OPTIONAL final stage:
`start(authorized,pidfd,pid,start_ticks,capacity,true)`. The last bool defaults to
false, preserving birth-only mode. No public API, archive format, automatic
recorder activation or additional external utility is introduced.

## Coverage and matching

The selected creator is authenticated as in109. Its birth hook records new groups
before first wake. The final hook is installed BEFORE birth is enabled, so a fast
child cannot fall into an attachment-order gap. Neither hook needs a subsequent
procfs census, child pidfd or child PID supplied by the controller.

The release_task FENTRY stage filters CURRENT TGID/start/UID as the selected
creator/reaper BEFORE target or map access. Only that creator's direct releases
are covered. It then matches TARGET TGID AND birth start_boottime across1..64
retained slots; it must continue past an older slot with a reused TGID. UID must
match, target must be group leader, live=0/nr_threads=1 before and after reads.
The scan is bounded and only occurs in the selected creator's release calls,
not a global per-release map census. Non-leader release, other reapers, credential
changes, orphan reparenting and auto-reap routes can leave the final unknown.

This release point precedes the leader's accumulation into signal. The retained
total is task+signal scheduler runtime/read_bytes/write_bytes, exactly once per
component with checked uint64 addition. The accounting/release order is in
[Linux v6.6 kernel/exit.c](https://github.com/torvalds/linux/blob/v6.6/kernel/exit.c).
CPU is scheduler-runtime group time, excludes waited-child CPU. IO is GROUP
ACCOUNTING, including already waited children; no exclusive physical-IO claim
and no automatic process-tree summation, which could double-count IO.

## Publication, delta and availability

The internal record grows from8 to13 u64 words: immutable birth prefix, final
publication state, release-observation monotonic timestamp, and three totals.
No kernel pointer is published. A final probe atomically acquires the birth state
(OR0|FETCH), checks final state is unused, writes the suffix and publishes final
state last via atomic exchange. Reader copies are separated by a full barrier;
crossing either publication causes a retry/omission, not a partially invented row.
Sticky quality flags1/2/4 retain109 semantics;8 denotes final read/UID/overflow
failure,16 duplicate final. Existing valid rows survive with partial quality.

decode_startup_birth checks final time/counter monotonicity before subtraction.
CPU deltas exceeding int64 nanoseconds stay unknown while valid IO can remain.
Pending, disabled or uncovered final fields have NO value, not zero. Published
zero deltas are measured zero. final_monotonic_ns is release ENTRY observation,
not last CPU use, last-thread exit or host steady-clock epoch. Delayed reaping
can make that timestamp much later than actual process execution.

Creator exit does not erase the records. Replay after child and creator reaping
preserves birth/final values. Existing descriptor/caller/boot/euid/fork guards
and inherited-reference coordination contract remain. Both hooks/maps are owned
and released on stop/failure/destruction; no background worker or activation.

## Native proof and remaining I07 work

The manual target still requires --authorized-private-birth and is NOT registered
in CTest. Only private creator/children are traced. The observer receives local
child PIDs and independent oracles ONLY AFTER their creator has reaped them;
procfs absence is checked before the first child read. Creator's regular fork/
wait naturally knows its own child PID; that is not library telemetry selection.

Eight scenarios cover birth-only modes(cap4/cap1), normal finals, leader-first
threaded finals, waited-grandchild IO, SIGCHLD-ignored auto-reap, known-zero writes,
and capacity1 with a final. Nine published final records are checked against private
process CPU-clock bounds and exact private /proc/self/io. Waited grandchildren
do not consume direct-creator birth slots. Auto-reap has two births with absent
final metrics, not fabricated zeros. Capacity loss remains explicit; unrelated
sentinel/thread exclusion, observer-fork rejection, creator reaping/replay and
FD census are checked in every run. Private IO files are unlinked immediately.
Both complete manual runs passed; owned temporary probe references were removed.
Evidence: work/startup-child-final-native-{initial,recheck}.log, Linux141/141
CTest plus30 focal repetitions, MSVC birth/final10 and exit/BTF10 pure runs,
installed SDK consumer exit0. Initial build clock-skew warnings are preserved;
the final incremental build is clean. These are local checks, not remote CI or
proof of support on every Linux kernel/hardware combination.

The fixture VM tests final arithmetic, known zero, overflow/decrease, wrong actor/
identity, live/non-final groups, UID failure, duplicate final and TGID reuse with
a different start. VM tests do NOT replace kernel verification.

Still INTERNAL, not integrated automatically into Monitor, recorder or history.
Kernel-only identities still need local namespace mapping, safe app/launch
attribution and public integration. Other reapers, further descendants and reliable
automatic-startup cause remain outside this component. No apps already gone before
attachment are recovered retroactively. I07 is not closed by these scoped totals.
