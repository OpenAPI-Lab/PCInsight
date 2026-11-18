# Selected TCP reference by process and descriptor (partial F03)

The additional API in `pcinsight/linux_tcp.hpp` queries an explicitly
selected process descriptor without inferring exclusive ownership from
inode, UID, or username. It does not enumerate all FDs or causally
attribute traffic to an application.

```cpp
pcinsight::LinuxTcpSocketHolder target{pid, process_start_ticks, descriptor};
auto observation = pcinsight::read_linux_tcp_socket_holder(target);
pcinsight::LinuxTcpSocketHolderSampler sampler;
auto baseline = sampler.sample(target);
// On the next tick:
auto next = sampler.sample(target);
```

process_start_ticks comes from ProcessInfo::linux_start_ticks in the same
host, boot, and PID view. A positive token, a positive PID that fits
pid_t, and descriptor >= 0 are required. The descriptor is an observed
number, not a command, pathname, or persistent socket token. The first
result discovers the current cookie. If that FD points to a different
object on the next tick, the rate resets rather than reusing the baseline.

## Pinning and checks

For the calling process, the provider uses F_DUPFD_CLOEXEC. For another
process, it opens a pidfd and duplicates the descriptor with pidfd_getfd.
There is **no** fallback to /proc/PID/fd, readlink or inode matching,
ptrace attachment, sudo, or a helper. Both duplicated references belong
to the library and are closed when the query ends.

The provider checks native procfs and that /proc/self corresponds to the
caller PID. For an external target, it additionally requires the Pid
field in `/proc/self/fdinfo/<pidfd>` to match the requested PID. That field
projects the pinned process into the namespace of the procfs mount.
An absent, zero, negative-one, malformed, duplicate, or differing value
is rejected before querying the descriptor. A coincidental match for
the caller's own PID does not establish namespace equivalence for other
PIDs. The fdinfo read pins its source, checks procfs, and is bounded to
64 KiB, 10,000 lines, and four EINTR retries.

PID and start_ticks are checked before and after. An external pidfd must
show no exit at either point: PID reuse does not substitute a different
process. The pinned FD must be an IPv4 or IPv6 TCP stream socket in
ESTABLISHED or CLOSE_WAIT. The provider reads SO_COOKIE and TCP_INFO from
that reference. It then duplicates the target FD number **again** and
requires the same cookie, dev, and inode so that a socket change in the
slot during the observation does not produce a published reference.

This establishes association observed at the endpoints, not continuous
possession or causation. An FD can change from A to B and back to A
between checks, be shared, inherited, or transferred. start_ticks has
tick resolution and does not identify exec or boot globally. A read
is neither a transaction nor an action token. The query does not alter
socket state, flags, options, offset, or data. However, duplicating a
reference **can briefly extend the socket's lifetime**, even if the
target closes its copy. The sampler keeps no FDs between ticks, only
metadata and counters. The short-lived reference can affect connection
close timing; zero impact is not guaranteed.

## Results and rates

LinuxTcpHeldSocketSample retains the target, cookie, full inode,
ip_version, TCP state, monotonic captured_at, and native acknowledged
and received counters. It does not report an owner username or UID or
invent a causal PID. Optional SO_NETNS_COOKIE identifies the namespace
**of the socket**, which may differ from the current thread's. This
is not the same kind of ID as the diagnostic sampler's namespace inode.
Its absence is reported with a reason. SO_COOKIE is mandatory here.

The sampler requires compatible target (PID/token/FD), cookie, inode,
family, and namespace metadata, with increasing time and nondecreasing
counters. Each direction's rate is its delta divided by elapsed seconds,
marked Support::estimated. It is not total process traffic or physical
throughput. First sample, error or denial, reset, or an observed change
of FD, process, socket, or namespace invalidates the baseline. An absent
field does not clear the other valid direction; recovery requires two
observations. If any known counter decreases, discard the interval for
both directions. Measured zero is known; overflow and absence are not zero.

Two holders with the same cookie observe **the same socket**. Do not add
their rates as independent PID traffic. A single process can also have
multiple duplicated FDs for one cookie. The API exposes identities so
the GUI can display sharing, but does not offer a per-process aggregate
that arbitrarily assigns shared traffic. A cookie seen in only one target
does not establish that no hidden holders exist.

## Permissions and portability

pidfd_getfd is subject to the PTRACE_MODE_ATTACH_REALCREDS ptrace check,
LSM/Yama/seccomp, and the caller's current rights. EPERM/EACCES remains
permission_required. Dumpability, policies, and privileges are not
changed to avoid denial. A kernel or headers without the required pidfd
or SO_COOKIE support yield absence with a reason. Windows retains the
API but does not publish Linux observations. Denied options or identities
remain denied rather than becoming a successful empty result. A token
does not grant authorization.

Each call queries one reference, with no scans or background work.
There is no overall kernel syscall deadline. The host must serialize
sample/reset and avoid concurrently manipulating internal FDs it does
not own, namespaces, credentials, or the procfs mount. Sampler
exceptions clear its baseline.

## Verification and remaining scope

Portable fixtures cover identity, cookie, FD, inode, namespaces, clock,
counter reset, absence with a residual value or permissions, recovery,
and zero. The fdinfo parser rejects missing, duplicate, -1, zero,
out-of-range, oversized, and junk Pid input. The native test creates
two private loopback TCP sockets and one private child with bounded
exit handling. It covers duplicate and shared references, FD replacement,
stale token, invalid FD, unsupported UDP or listening sockets, and
cookie correlation with Netlink diagnostics when available. Queries
do not consume pending data or change F_GETFL. External queries target
only that private child, never other processes' descriptors. A kernel
denial of duplication is recorded as a limitation, not a positive
result for that provider.

At this checkpoint F03 remained in progress. Holder association is not
exclusive per-app accounting. UDP and other coverage, shared and
short-lived sockets, and attribution and integration remained open.
ProcessInfo/UserSession.network is not populated as if these TCP numbers
were total network traffic. This does not close J04, H04, N02, or CI.
The new API adds symbols and types without changing existing layouts,
virtual ABI, or history format. Rebuild and relink to use it.

On 2026-10-17, Linux Release passed 96/96 and Windows Release passed
111/111 with no native holder test skip. Packages were reinstalled,
and rebuilt, relinked external consumers on both systems exited 0
against the final library. WSL: self PID 1799, start_ticks 5284, FD 4,
cookie 12289, acknowledged delta 8,192 B and 8,192 B received. The
sampled rate was 86,347,011 B/s, not a physical benchmark. The private
child query through pidfd_getfd was **positive**, with the same cookie
12289 as the parent; fdinfo and PID projection were verified. Aliased
FDs identified the same socket; replacement reset the baseline;
invalid tokens, UDP, and listeners were rejected; pending data and
F_GETFL were preserved. This does not validate all processes, rights,
distributions, or architectures.

Logs: work/tcp-holder-{build,verified-build,final-build,clean-build}-{linux,windows}.log,
work/tcp-holder-{suite,final-suite,clean-suite}-{linux,windows}.log, and
work/tcp-holder-clean-native-linux.log. Initial builds recorded GCC
optional-return/maybe-uninitialized and misleading-indentation warnings.
The absence return was restructured to explicitly construct an empty
Metric, and parser statements were split. Warnings were not disabled
and assertions were not relaxed. Final builds had no warnings. Passing
these runs does not resolve historical N03 CPU or history risks.
F03 remained E; the count stayed 46/75 (61.3%) without fractional
credit or new IDs.

Primary sources: [pidfd_getfd](https://kernel.googlesource.com/pub/scm/docs/man-pages/man-pages/+/master/man/man2/pidfd_getfd.2),
[pidfd fdinfo Pid projection](https://github.com/torvalds/linux/blob/master/fs/pidfs.c),
and the [kernel SO_NETNS_COOKIE test](https://kernel.googlesource.com/pub/scm/linux/kernel/git/stable/linux-stable-rc.git/+/refs/tags/v5.16.14/tools/testing/selftests/net/so_netns_cookie.c).
