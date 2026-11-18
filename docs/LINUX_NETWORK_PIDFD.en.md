# External actor identity from a pidfd

Revision 69 implements `read_private_pidfd_network_identity(authorized,pidfd)`
in an internal Linux component. It obtains the leader's initial-domain TGID
and UID and its start_boottime without asking the target to report them.
There is no installed API, Monitor activation, global task traversal, or
process enumeration.

## Bounded query

Without opt-in or with a negative descriptor, it makes no capture queries.
It duplicates the selected descriptor with CLOEXEC to retain the same object
throughout the query and uses poll to check that it is not ready or exited.
It does not open another numeric PID as a substitute. A handle that is not
a pidfd yields no identity.

BTF validates the exact prototype
`security_ptrace_access_check(task_struct*, unsigned int) -> int` and the
offsets of task.tgid, group_leader, start_boottime, cred, and cred.uid.val.
It checks signedness, widths, wrappers, container bounds, and the 1 MiB
limit. The capability is optional: a missing offset or hook is not invented.
An earlier candidate, __pidfd_fget, does not appear in WSL's BTF. It was
discarded, with no hidden fallback to that symbol.

The component discovers the **calling thread's initial-domain TGID/TID**
through local unattached programs. It attaches a temporary FEXIT probe
that filters on the full pid_tgid before reading context, accepting only
ATTACH_REALCREDS=0x12 with return value 0. It does not modify the security
decision, credentials, or kernel memory.

With the probe active, it calls only `pidfd_getfd(dup_pidfd,-1,0)`. This
triggers the access check without copying any FD from the target's table.
The final result must be EBADF, with exactly one valid observation and no
errors. EPERM/EACCES are reported as permission required. The component
does not attach through ptrace, signal the target, or stop it. If the
check does not reach the hook or cannot be verified, it returns absence,
not a guessed identity. The operation can acquire kernel locks and pass
through LSM or system audit paths; it is not guaranteed free of overhead
or audit events.

To prevent handler reentrancy during this window, the component temporarily
blocks signals allowed by pthread_sigmask in the calling thread and
restores the mask through RAII. This can delay handlers, including a
signal-based watchdog. It is not a hard deadline. Asynchronous thread
cancellation, reentrant caller operations, and caller manipulation of
internal descriptors are unsupported. The target's mask and those of
other threads are unchanged. Signals reserved by libc or not blockable
retain their system rules.

The probe reads the task being checked, requires task==group_leader, and
copies TGID, UID from cred.uid.val, and start_boottime into a private
five-entry ARRAY with success/error counters. The non-sleepable trampoline
protects reads with RCU on the reviewed path. No pointers, user contents,
or network data are exposed. Unrelated or denied returns and contexts do
not reach the task read. The link is removed before reading the map; the
triggering call is synchronous and filtered to the same thread. Maps,
programs, link, and duplicate are closed even on error. No pinning, daemon,
packages, external commands, or permanent configuration.

## Guarantees and limits

The result associates one identity observation with the queried pidfd
object on this verified path. It checks liveness before and after, not
future liveness. It does not certify a permanently stable UID, atomic
identity across credential changes, the state of all threads, or a
username association. A nonleader thread does not yield an invented process
token. SELF can skip this hook due to an optimization in the check; use
the earlier SELF provider rather than interpreting absence as PID 0.

This depends on compatible BTF, tracing/FEXIT, BPF permission, pidfd_getfd,
ptrace/LSM policy, and kernel behavior matching the reviewed path. Native
verification is limited to this WSL x86_64 environment. A matching BTF
prototype does not establish all semantics on another kernel. Do not
promise universal support. Revision 70 adds a
[selected procfs/boot/namespace/start_ticks join](LINUX_NETWORK_PIDFD_JOIN.en.md)
with endpoint checks. At this checkpoint, F03 still needs revalidation in
windows, buckets and coverage, and public integration. This change does
not convert start_ticks to nanoseconds or automatically obtain an external
socket's namespace cookie.

## Evidence

BTF fixtures cover a positive capability and eight negative prototype,
type, field, and linkage cases. A private interpreter without capture
checks exact TGID/TID filtering before context access, mode/return,
leader/null/failed reads, UID 0/UINT32_MAX, absent maps, offsets/support,
and validation of a single error-free result. That CTest does not run
real BPF. Earlier suites remain in place.

The manual `--authorized-private-pidfd-session` mode is NOT a CTest. The
parent creates a private workload child and keeps its pidfd. The child does
not run BPF discovery or report its kernel token; the harness only
coordinates traffic and supplies local data and the socket cookie. The
parent obtains and revalidates the identity through the pidfd twice and
then uses start_selected. This demonstrates identity discovery without the
child's cooperation on identity, **not** a universal application join or
cookie discovery without cooperation.

Authorized WSL run exited 0: child local PID 554, kernel TGID 1640,
initial UID 0, TCP 4,096/4,096 B, UDP 256/256 B, and eight peeks. The parent
was excluded, detach froze counters, wrong start/UID produced sticky failure,
exit was observed through pidfd, and invalid or exited handles were rejected.
The caller's mask and FD count matched before and after each query; the
session and child were cleaned up. No signals went to the target except
for private child cleanup if the harness failed. No UID, namespace, or
configuration was changed. Log: `work/network-pidfd-native-linux.log`.

The first build of a new target was requested before regenerating CMake
and returned No rule. CMake was regenerated and the log retained without
relaxing tests. An initial pair-copy warning was fixed. The absence of
the first __pidfd_fget candidate is documented in
`work/network-pidfd-layout-native-linux.log`; the BTF alternative was
validated in `work/network-pidfd-layout-security-native-linux.log`.

Linux Release regression: 120/120 without skips, 12 network tests repeated
20 times each, and the installed consumer rebuilt and exiting 0. Final
build had no warnings. Logs:
`work/network-pidfd-{final-build,suite,repeat,install,consumer}-linux.log`.
No Windows builds or captures and no public API, ABI, or history changes.
H04/J04 are not closed, and the official count keeps F03 in progress.

Primary Linux 6.6 sources reviewed:
[pidfd_getfd](https://raw.githubusercontent.com/torvalds/linux/v6.6/kernel/pid.c),
[ptrace_may_access](https://raw.githubusercontent.com/torvalds/linux/v6.6/kernel/ptrace.c),
[LSM](https://raw.githubusercontent.com/torvalds/linux/v6.6/security/security.c),
[flags](https://raw.githubusercontent.com/torvalds/linux/v6.6/include/linux/ptrace.h),
and [trampoline RCU](https://raw.githubusercontent.com/torvalds/linux/v6.6/kernel/bpf/trampoline.c).
