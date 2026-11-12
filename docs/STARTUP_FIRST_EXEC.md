# First successful exec: historical filesystem evidence

Update114: STARTUP_EXECUTABLE_MATCHES.md adds current-caller XDG filesystem
candidate comparison, estimated even when unique, not automatic app attribution.

Checkpoint113,2026-10-29. Public option `observe_first_exec` defaults OFF in
StartupBirthObserverOptions. When enabled, each authorized child's
`first_executable` can retain kernel monotonic observation time, inode, device
major/minor and PROCESS kernel real UID at its first successful exec. These are
filesystem identifiers read from its active image, not a path, content hash,
publisher, file owner, current executable, `.desktop` identity or autostart cause.
Public options/observation/report layouts changed: recompile SDK consumers.
No persisted archive format or app-history merge changed.

## What is observed and what is not

The sensor attaches to sched_process_exec, not execve syscall entry. Kernel
[exec_binprm](https://raw.githubusercontent.com/torvalds/linux/refs/tags/v6.6/fs/exec.c)
emits this after binary-handler success. Non-leader exec preserves the original
group's birth time while assuming the original group PID. No assumption about
application initialization or having survived to user main is made.
The [tracepoint definition](https://github.com/torvalds/linux/blob/v6.6/include/trace/events/sched.h)
identifies exec events; this program does not inspect its filename or arguments.

The first successful event is immutable. A later exec, even of a different file,
NEVER replaces it. If the first event cannot be read, that failure is retained:
the sensor must not quietly call a later executable the first. Pending/disabled/
failed metadata remains absent, not false/zero. A measured zero inode/device
component is represented as a value, not used to infer absence.

Scripts, interpreters, wrappers and apps sharing one binary prevent a one-to-one
mapping to desktop entries. Inodes/devices can be reused and filesystem-specific
stat behavior can differ: this is historical VFS metadata, not a permanent file
identity or universal path-matching guarantee. The WSL fixture compares it to its
own opened-image fstat at the same execution. A caller-supplied startup association
remains estimated; automatic_startup_origin stays unknown even with known exec.
No automatic binding, process-tree sum or impact classification is introduced.

## Scope and publication

An additional bounded HASH map holds only TGID->birth slot entries enrolled by
the already authenticated selected-creator birth hook, before the new group can
first run. Only retained groups fit the map; capacity loss remains explicit.
Exec attaches BEFORE birth enrollment. For each exec, an unselected TGID returns
after ONE allowlist lookup, before any task/file/credential metadata read. There
is no global task census or all-exec user-space event stream.

A hit must match a published birth's TGID AND kernel group start time. Reused
unselected PIDs cannot yield executable metadata. The first event atomically
claims state0->2 using CMPXCHG. It reads current group mm->exe_file->f_inode,
i_ino and i_sb->s_dev, UID and time; it publishes state1 LAST. State3 records a
failed enrollment/read. Later failures do not erase an already published first.
No pointer, pathname, argv or environment is published. Device numbers use the
[kernel major/minor encoding](https://raw.githubusercontent.com/torvalds/linux/v6.6/include/linux/kdev_t.h),
not a raw cast to userspace st_dev.

The private ARRAY record grows15->20 u64; birth/final/PID fields retain their
offsets. Reader double-copy/fence/bracket now includes exec state; in-flight
state2 is omitted as publishing rather than decoded as a complete record. Bounded
BTF validation resolves pointer targets, scalar widths and offsets. Actual
mm_struct uses an anonymous struct here: traversal is checked with depth8 and
256-node budgets, including parent bounds and ambiguity/cycle rejection.

Sticky quality flag64 reports exec enrollment/read failure (not an exact count).
Existing CPU/IO and namespace records survive a failed exec read. SDK validation
allows pending->published/failed only, rejects rewritten first metadata and
exec changes after a final release. Existing associations remain immutable.
Start/attach failure releases all owned references; opt-in unsupported kernels
fail explicitly rather than claiming execution data. Defaults add no exec map/
program/hook. Closing releases owned references, with the existing fork-reference
coordination contract; no daemon, package, privilege elevation or external utility.

## Evidence

Linux142/142 CTest without skips+30 focal repetitions, MSVC30 pure runs across
birth VM/BTF/public observer, SDK installed and consumer rebuilt exit0.
Native manual target checks public and internal captures independently in13private
scenarios:23retained rows,18final resource records,5expected absent finals. The
four new cases are direct exec, ENOENT exec failure, repeated exec and non-leader
exec after the leader exits. Six first-executable records match independent own
image fstat; failed exec produces no successful-exec record. Associations/replay/
close/FD census remain checked; temporary hooks/maps are released.

Initial native start failed because the BTF reader did not traverse mm_struct's
anonymous member; support was added and fuzz/shape tests extended. A later test
fixture failed because [/proc/self/exe](https://man7.org/linux/man-pages/man5/proc_pid_exe.5.html)
is unavailable after the main thread exits;
the worker now uses execveat with its own image fd pinned BEFORE leader exit, and
queries the image oracle only after exec. These initial failures are preserved
in work/startup-exec-native-{initial,recheck}.log, not counted as successes.
Acceptance is startup-exec-native-fd.log and startup-exec-native-full.log.
VM covers allowlist early exit, UID/device/time, publication, PID reuse, read/map
failures, immutable first image, late auth failure and known zero. BTF tests cover
direct/anonymous layouts, truncation, missing fields, signedness, ambiguity/cycles.

Still I07 E: reliable desktop/launch association and automatic-startup cause,
time-bounded recorder/classifier/history integration, other reapers and further
descendants. File evidence is a completed component, not completion of I07,
all-Linux hardware coverage or remote CI. Windows remains postponed/N03 open.
