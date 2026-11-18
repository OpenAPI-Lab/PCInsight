# Private Linux SELF counter session

Revision 68 adds a separate internal [selected-actor method](LINUX_NETWORK_SELECTED.en.md)
with a declared token and explicit type. It does not turn SELF into discovery
or attestation of external processes. The SELF contract below is unchanged.

Revision 64 extracts the classified, guarded capture from the test executable
into `src/linux_network_session.{hpp,cpp}`. This is an **internal** component,
not an installed API or a provider enabled from Monitor. F03 is still in progress
at this checkpoint.

Revision 65 adds a [mode scoped to the socket cookie](LINUX_NETWORK_NAMESPACE.en.md),
with optional BTF and sticky rejection of mixed namespaces. The revision 64
contract below describes the mode without that restriction; it is not a
multi-actor provider. Revision 66 connects the capture to [SELF windows and
rates](LINUX_NETWORK_WINDOW.en.md) without publishing global totals or converting
IDs to procfs.

## Scope and authorization

Without authorization, `start(false)` neither queries BTF nor calls BPF.
`start(true)` discovers only the calling process's identity: it accepts no
external PID, UID change, or global filter. It uses kernel BTF and BPF syscalls
without executing utilities or loading an external telemetry library. It
requires kernel support and load/attach permissions. EPERM/EACCES yield
`permission_required`, not a zero counter. It does not install packages, change
sysctl, create a daemon, or pin objects in bpffs. The temporary, bounded WSL
test remains covered by the human authorization received on 2026-10-18.

Discovery uses an unattached RAW_TRACEPOINT program, local TEST_RUN, and its
own three-entry map. The kernel TGID, initial-domain UID, and leader's
start_boottime must be exact and valid before creating a capture. It does
not turn getpid/start_ticks into kernel identity or accept estimated BTF or
counters as exact.

## Ownership and states

`stopped -> active -> detached -> stopped`; also `active -> stopped`.
The session cannot be copied or moved. The injectable API borrows context
that must outlive it. The caller serializes state and read operations;
concurrent access to the same object is not guaranteed safe.

Before attaching, it loads all classified inet_sendmsg, sock_recvmsg, and
inet6_sendmsg programs against a fresh nine-entry map and the fixed SELF
identity. A failure or exception during startup releases all resources already
acquired. The temporary identity map and program close before attachment.
FD 0 is valid and is also closed. Ownership is cleared before closing each
descriptor; close is neither retried after EINTR nor called twice on the
same descriptor.

`detach()` closes links in reverse order and then programs, retaining the
map for inspection. It does not promise portable draining of arbitrary writers
already executing; the reader retains the quality guard. The frozen-map test
uses traffic completed before detach, followed by new traffic. `stop()`
also closes the map, clears identity, and is idempotent. The destructor calls
stop. No objects persist after the process exits. A session cannot restart
while it owns a map, even when detached; stop first. After fork the child
must close its copy with stop. The inherited identity remains the creator's
and is **not** relabeled as the child's SELF traffic.

## Reads and limits

Four uint64 cumulative counters report instrumented payload: TCP send/receive
and UDP send/receive. They are not rates, interface bytes, ACKs, or complete
network coverage. Positive PEEK/TRUNC/ERRQUEUE results remain excluded and
are counted separately. The [earlier producer's](LINUX_NETWORK_QUALITY.en.md)
identity, sticky quality, overflow, and sequence checks are retained. Reads
use seq_cst fences and writer/sequence checks at both ends, with up to eight
attempts. Contention, counter movement, quality failures, overflow, or denied
lookup yield absence, not invented zero. A known zero is returned only from
a coherently read map. An absent read does not tear down capture; the caller
and destructor remain responsible.

An available read means those instrumented counters are coherent, **not**
that the entire network is captured. Revision 64 did not resolve the socket
netns cookie. Revision 65 adds a declared single-namespace restriction, not
dynamic buckets. Exact joins to procfs/apps/users, multiple actors, unverified
paths such as sendfile/splice/io_uring, reducer/Monitor integration, and
windows/history remained pending at this checkpoint. The session does not
publish total network usage in ProcessInfo/UserSession or invent a cookie
from a namespace inode. H04/J04 remain pending here. Native coherence is
validated on WSL x86_64, not every kernel or architecture.

## Tests

`tests/linux_network_session.cpp` uses an injected API with no privileges or
capture. It covers 14 failure and exception points, invalid identity, permission,
FD 0, estimated values, busy/moving/overflow reads, close order, destruction,
and restart. CTest keeps Release assertions enabled and uses a 20-second timeout.

`pcinsight-linux-network-session-native --authorized-self-session` is manual,
**not CTest**. Without the flag it returns 77 without loading probes. The
authorized mode uses only its own IPv4/IPv6 TCP/UDP loopback with
send/sendto/sendmsg, excluded peeks, an excluded private child, and its own
threads. It verifies detach, stop, restart, and descriptor counts before and
after. A 20-second watchdog and 2-second socket timeouts bound the run.
Unavailable or exit 77 never counts as a positive result.

Verification on 2026-10-21: Linux Release 116/116, injected test repeated
20/20 times, and an installed consumer rebuilt and exiting 0. The authorized
native test exited 0: TCP 6,144/6,144 B, UDP 384/384 B, 12 excluded peeks,
the child excluded, the map unchanged by traffic after detach, verified
stop/restart, and no FD leaks. Exit 77 was confirmed directly without the
flag. Windows was neither built nor run for this unit. Logs:
`work/network-session-{suite,repeat,native,no-optin,install,consumer}-linux.log`.
The initial build warned of 0.0062 s clock skew in an unrelated startup-reset
dependency. The verified later build had no warnings. The first wrapper
checking exit 77 failed because of shell quoting, not because capture was
enabled; a direct run and LASTEXITCODE confirmed 77. That failure was retained.
