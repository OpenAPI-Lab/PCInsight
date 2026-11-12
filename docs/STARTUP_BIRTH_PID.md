# Namespace-qualified child PID at birth (internal)

Update112: the public owning API is described in STARTUP_BIRTH_OBSERVER.md.
Kernel/namespace identity and caller-declaration limits remain in force.

Revision111,2026-10-29. LinuxStartupBirthCapture can now retain the child's TGID
as seen in the OBSERVER's active PID namespace, before the child first runs.
It does not require discovering that child's PID/pidfd/procfs record while alive.
The existing explicit private-creator authorization/attachment is still required;
Monitor, recorder, login and global capture are NOT automatically enabled.

## Identity, not a PID heuristic

StartupChildBirth.local_pid is a Metric<StartupBirthLocalPid> with namespace inode
and TGID. It is independent from kernel.kernel_tgid. The surrounding report retains
the capture boot/context and observer namespace device/inode. Never detach a bare
number and treat it as a globally unique identity or permission to control a
process: PID reuse remains possible, and the captured task may already be gone.
Kernel UID and kernel start_boottime remain kernel-domain fields; no conversion
to observer UID or procfs start_ticks is implied. There is still no executable,
successful exec, app identity, automatic-startup cause or transitive-tree claim.

Native selection establishes the observer PID namespace inode, checked again via
the existing caller/context/boot/fork/euid guards at sample endpoints. The active
observer process holds its namespace alive during capture. Do NOT substitute a
creator's namespace-for-children or the child's innermost namespace. Namespace
inodes can be reused after namespace lifetime; this is not a permanent archive ID.

The independent BTF query validates task.thread_pid -> pid, unsigned level,
trailing flexible/one-element upid numbers array, bounded stride, signed32 nr,
pid_namespace pointer and embedded ns_common unsigned32 inum. Scalars, widths,
offsets, nested struct bounds, pointer targets and ambiguity are checked, not
hardcoded to one build. Native BTF remains bounded to64MiB and64-bit pointers.
Unavailable schema or unrepresentable namespace inode disables mapping with an
explicit note; existing birth/final collection can still work without it.

The birth hook, AFTER existing creator and new-group filters, examines only
indices0..pid.level, capped at32. It matches namespace inode, then accepts a
positive signed32 TGID. No equality assumption with kernel PID or innermost PID.
No unrelated task list, external executable, package, procfs mount or daemon.
The layout and namespace semantics are based on Linux's
[pid/upid definitions](https://github.com/torvalds/linux/blob/v6.6/include/linux/pid.h),
[PID lookup](https://github.com/torvalds/linux/blob/v6.6/kernel/pid.c) and
[namespace level bound](https://github.com/torvalds/linux/blob/v6.6/include/linux/pid_namespace.h).

## Failure and publication

The private map record grows13->15 u64, appending namespace inode/local TGID.
Final fields8..12 are unchanged. Both new fields are written only after a complete
match and BEFORE atomic birth publication. They remain immutable through final
publication/replay. Existing two-copy/fence/bracket logic covers both stages.
The decoder rejects half-present, zero or out-of-range namespace/PID pairs.
Sample rejects a mapped namespace differing from its authenticated observer.

Read failure, null pointer, excessive level, missing namespace or invalid nr sets
sticky flag32 and resumes birth publication WITHOUT namespace values. It does
not throw away measured birth/resource counters or invent PID0. Outer quality is
partial/estimated; local_pid has no value. Other loss flags retain their meaning.
If the schema itself is unavailable, pid_mapping_enabled=false plus a note states
why no attempt was made. Published zero CPU/IO is still valid measured zero.

No additional descriptors, workers or mount operations are needed by collection.
No public API/ABI or persisted archive format changes in this checkpoint. This
private type is not an automatically integrated StartupImpactObserver input.

## Evidence and limits

Fixture VM: global TGID88 maps to namespace-local12, not88; level0 and32, a high
unsigned inode, absent namespace, out-of-bound level, bad nr/null/read fault,
birth retention and final zero CPU after mapping failure. No read beyond level.
BTF: truncations at every length, missing members, ambiguity, signedness, pointer
targets, invalid arrays and accepted flexible/one-element layouts. VM validation
is not a substitute for loading the actual generated program in the kernel.

Manual native scenarios preserve110's eight cases and add ONE private init in a
fresh namespace-for-children. Only that private creator calls unshare(CLONE_NEWPID);
it is not moved, no procfs is mounted, and no existing process is reconfigured.
See [unshare semantics](https://man7.org/linux/man-pages/man2/unshare.2.html).
The child reports self PID1; observer-qualified PID is a different number. Private
fork/wait oracles reach the observer only AFTER reaping; all captured local PIDs
match them. Totals/replay/thread/sentinel/fork rejection/capacity/FD checks remain.
The final complete repeat records selfPID1/observerPID450/kernelTGID1568 in
namespace inode4026532221: the REAL observer and kernel PID domains also differ.
All15 retained local PIDs match their private post-reap oracles, with10 final
CPU/IO records and5 expected absent finals. Both complete corrected runs exit0;
owned temporary probe references are removed. Numeric examples are ephemeral,
not stable process IDs or a promise that different domains always differ numerically.
Private-test cleanup uses its owned pidfd to signal the creator after setup,
avoiding an unrelated reused PID if a status assertion failed after reaping.

An initial TWO-child namespace fixture ended in IPC EOF; its exact failing syscall/
wait cause was not logged. It is preserved in startup-birth-pid-native-namespaces.log,
not reinterpreted as a proven kernel/library bug. The corrected single-init
namespace case passes separately, then in the complete nine-scenario run. No
collector implementation change was needed for this fixture correction.

Logs: work/startup-birth-pid-*. Linux141/141+30 focal repetitions, MSVC20 pure
fixture runs, installed SDK consumer exit0. These are local checks, not remote CI,
all-kernel coverage or full Windows support. I07 remains open: safe app/exec/launch
attribution and integration, other reapers, further descendants and reliable
automatic-startup origin remain outstanding. No startup-impact classification
is certified simply because a namespace-qualified PID was captured.
