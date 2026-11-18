# Direct SELF association between kernel and procfs

Revision 67 adds `sample_joined_self()` to the private sampler. It returns
a network window with a local PID/start_ticks/UID token and procfs/namespace
context. This associates **the caller itself**; it is not a general PID/UID
translation or a join for unrelated processes. It does not change the
installed API, Monitor, or history.

## Native procfs source

`read_linux_network_self()` does not run utilities, load BPF, or query
another process. It opens /proc as a directory without following the final
symlink and verifies procfs with fstatfs. `readlinkat(root,"self")` must match
the caller's getpid. If the mount presents a different PID domain, the
reader does not guess. It opens the numeric process directory and the
calling thread's directory relative to that pinned root, verifying procfs
for directories and data. It takes no external paths or PIDs.

The reader checks stat before and after, and reads status for the leader
and thread. PID/Tgid/UID must agree with getpid/gettid/getuid, and start_ticks
must be nonzero and stable. Parsing is bounded to 64 KiB and rejects
duplicate fields, NUL, overflow, signs, incomplete Uid, and extra data.
NStgid/NSpid are not interpreted as initial kernel identifiers. The reader
does not convert start_boottime into ticks or ticks into nanoseconds.

The ns/pid and ns/user magic links are opened intentionally from procfs,
verified as nsfs, and stored in the token as device/inode pairs. Caller and
leader must share the same context, which is verified again at the end.
Regular files use NOFOLLOW/CLOEXEC/NONBLOCK. Reads are bounded to 64 KiB
with up to four EINTR retries. All FDs are owned and closed through RAII,
including on errors and exceptions. No namespace, UID, permissions, mount,
or configuration is changed.

The token also retains the procfs root device/inode and process directory
inode. These describe observation context, not authenticated host identity
against an administrator who can replace mounts or permanent handles that
prevent future inode reuse. The reader does not export a universal namespace
map or interpret an inode number as a socket cookie.

## Association with capture

Only after authorized startup and with a valid creator-process guard:

1. Read local procfs identity and validate its token and the caller's PID.
2. Run the unattached identity program through local TEST_RUN using BTF
   stored at startup and owned temporary map/program resources. Require the
   initial TGID/UID and leader's start_boottime to equal the fixed capture token.
3. Verify the boot token, sample the window, and repeat the kernel identity,
   procfs identity, and boot-token checks. Require the complete local identity
   to be stable at both endpoints.

If a source or permission is missing, identity/UID/namespace/boot changes,
or an exception occurs, the joined window is not returned and the baseline
is discarded. A failure after the reducer calculates a rate still makes
that rate unusable. permission_required is propagated and zero is never
fabricated.

Joined samples require the same complete local token. A changed context
starts a new baseline without a rate. A raw `sample()` call invalidates
local continuity, so the next joined sample also starts without a rate.
Stop/restart/fork retain their earlier guards. The same object does not
support concurrent calls or fork during a call; injected contexts are borrowed.

These are endpoint checks, **not** an atomic credential snapshot or proof
that transient ABA changes did not occur. The probe still requires its fixed
UID/start on each positive instrumented operation and sets a sticky failure
on mismatch. Accumulated counters belong to the kernel token; they are not
retroactively converted into local-account totals. The local token accompanies
the window but does not authorize attributing all past bytes to a current
username, app/executable, or session. Rates remain estimated from read
endpoints and the UID sum remains partial; see [SELF windows](LINUX_NETWORK_WINDOW.en.md).

## Tests and limits

`tests/linux_network_self.cpp` covers parser, limits, and duplicates, plus
the native read-only --native reader with 20 matching observations and
before/after FD counts. The native test returns 77 when verification is
unavailable; that is not a positive result. Extended sampler fixtures use
local PID 7 versus kernel TGID 123 and local UID 42 versus kernel UID 1000
on purpose. Numeric equality does not replace direct association. They test
changes in ten local fields, estimated or denied data, differences in three
kernel identity fields, boot changes, recovery without a rate across the
failure, context changes between windows, and alternating raw/joined calls.
They do not change the user's real credentials or namespaces.

The session executable's manual `--authorized-self-joined-window` mode
uses prior temporary WSL eBPF authorization and private loopback only.
It checks real local identity, four rates and the UID sum against an oracle,
idle zero, fork, restart, and cleanup. CTest and Monitor do not activate
it. No external tools or packages are required.

Verified on 2026-10-21: Linux Release 119/119 without skips; 11 network
tests repeated 20 times each; installed consumer rebuilt and exiting 0.
The unprivileged native procfs reader observed PID 950/UID 1000/start 18067,
with 20 stable reads and no FD leaks. The root WSL joined capture exited 0:
local PID 390 versus kernel TGID 4965, UID 0/0, TCP 4,096/4,096 B, UDP
256/256 B, eight peeks, four rates and UID sum checked against the oracle,
793,949 ns interval, idle zero, fork rejection, clean restart, and no FD
leaks. These are run-specific observations, not constant IDs or intervals.
Logs: `work/network-self-join-*-linux.log`. The initial indentation warning
was fixed without semantic changes. Builds with transient WSL timestamp
warnings up to 0.27 s were retained; the final build had no warnings.
No tests were skipped to obtain these results.

F03 work pending at this checkpoint: external actor association with identity
and credentials at the event, dynamic multi-actor/network namespace buckets,
additional coverage, and public integration. Descriptions, executables,
apps, and complete aggregates remain pending. Demonstrating SELF does not
close H04/J04. Windows remains deferred.

Primary sources: [procfs](https://docs.kernel.org/filesystems/proc.html),
[status and NStgid domains](https://man7.org/linux/man-pages/man5/proc_pid_status.5.html),
and [Linux identity helpers](https://raw.githubusercontent.com/torvalds/linux/v6.6/kernel/bpf/helpers.c).
