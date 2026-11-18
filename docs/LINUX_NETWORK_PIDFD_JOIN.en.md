# Private pidfd/procfs association

Revision 70: `read_private_pidfd_network_join(authorized,pidfd,pid,start_ticks)`
associates a selected process's kernel token with its local procfs identity.
This is an **internal** component, not installed or enabled through Monitor.
It does not start general traffic capture, enumerate tasks, or close F03/H04/J04.

## Transaction

Without authorization or with invalid arguments, it does not query BPF.
It retains a CLOEXEC duplicate of the pidfd throughout the operation and
checks poll before and after. It does not replace the handle by calling
pidfd_open on a possibly reused number. The caller must serialize close
and reuse of the input descriptor during duplication. Reentrant manipulation
of internal descriptors is not supported.

The sequence is: procfs/boot context before, two kernel identity queries
through the pidfd, procfs/boot context after, final poll, and strict comparison.
Each context contains the boot UUID and the observer and target identities.
The observer retains PID/UID/start_ticks, procfs device/inode and process
inode, and PID/user namespace device/inode pairs, with the existing SELF
checks.

The selected reader pins `/proc`, verifies procfs and its self link, and
opens the observer's numeric directory, its `task/<gettid>/fdinfo`, and
the target's numeric directory with NOFOLLOW/CLOEXEC. It reads fdinfo's
`Pid:` **in the same procfs** before and after; it must match the expected
PID. It does not interpret NSpid as an initial TGID. The calling thread's
FD table matters because it can differ from the leader's. Ordinary files
are read with a 64 KiB limit, at most four EINTR retries, and procfs
verification. Namespace links are deliberately followed and their
descriptors must belong to nsfs.

The reader checks the expected stat PID/start_ticks before and after,
requires matching PID/Tgid/real UID in both status reads, and verifies
stable target namespaces. Both complete contexts must match and share
the procfs device/inode. Both kernel tokens must agree on initial TGID,
initial UID, and leader_start_boottime_ns; UID 0 is valid. Failure, denial,
exit, change, or ambiguity yields absence with a reason, not zero or
an approximate join. Temporary FDs and probes are removed.

## Domains and limits

The join does not require numeric PID/UID equality between procfs and
the kernel or convert ticks to ns. The UID in status is expressed in the
**observer's** user namespace, not the target process's. The token retains
both namespaces separately. Kernel overflow/munged values are not turned
into usernames or initial IDs. The pidfd projection is local to the procfs
mount's namespace, not a universal map.
Linux 6.6 sources reviewed:
[pidfd fdinfo/poll](https://raw.githubusercontent.com/torvalds/linux/v6.6/kernel/fork.c),
[UID in proc status](https://github.com/torvalds/linux/blob/v6.6/fs/proc/array.c),
and [per-task fdinfo table](https://raw.githubusercontent.com/torvalds/linux/v6.6/fs/proc/fd.c).

This is an endpoint observation, not an atomic transaction against changes
and ABA reversions in credentials, namespaces, or mounts. Inodes are not
global certificates and are not retained on return. It does not guarantee
future liveness, retain namespace handles, detect every intervening change,
or prevent target mutations. It associates only the observed selected
process. It does not identify each thread, a commercial app, history, or
total traffic. Revision 71 integrates revalidation into
[internal selected windows](LINUX_NETWORK_SELECTED_WINDOW.en.md).
It does not discover external socket cookies or multiple actors.

The [pidfd discovery](LINUX_NETWORK_PIDFD.en.md) requirements and limits
apply: compatible kernel BTF/ABI, BPF/FEXIT, and ptrace/LSM permissions.
The calling thread's signal mask is temporarily blocked and restored during
each discovery. This can delay handlers; there is no hard deadline or
supported asynchronous cancellation. The component does not attach with
ptrace, signal or stop the target, install anything, or change permanent
configuration. Test authorization remains limited to private children, not
the whole system. Linux is the priority; no new Windows implementation.

## Tests

Pure fixture: deliberately different local/kernel domains, a target
namespace different from the observer's, UID 0, 17 context changes,
three kernel changes, nine invalid context/token cases, and invalid
arguments or missing opt-in rejected without capture. It simulates the
endpoints rather than showing that the real kernel changes credentials
or namespaces.

The authorized manual `--authorized-private-pidfd-session` mode creates
a private child that does not report its kernel identity. The parent reads
start_ticks in procfs, resolves and repeats the join, and checks against
the harness's local oracle. It rejects a stale start, the parent's PID,
a pipe, and an exited pidfd without leaks. It retains capture checks for
TCP 4,096/4,096 B, UDP 256/256 B, eight peeks, parent exclusion, frozen
counters after detach, and sticky failure for incorrect identity. These
bytes do not establish total coverage.

Unprivileged native CTest `linux-network-selected-proc-native`: a worker
calls unshare(CLONE_FILES) and replaces a pipe FD with a pidfd at the same
number, which remains a pipe in the leader. The reader must resolve the
worker's table. A stale start and a pipe are rejected, with no leaks.
If the environment denies unshare or pidfd_open, it returns 77 rather
than inventing coverage. It changes neither namespaces nor the leader's
table; the private table goes away when the thread exits.

Final evidence and suite counts: revision 70 in the official SEGUIMIENTO.txt
tracker and `work/network-join-*` logs. Positive native results are limited
to this WSL x86_64 environment, without universal hardware/distribution
verification or remote CI.
