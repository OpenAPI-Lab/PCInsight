# TCP socket and UID diagnostics (partial F03)

Later F03 update: [TCP socket holders](LINUX_TCP_HOLDERS.en.md) adds a separate
query for a descriptor selected by PID/start_ticks through pinned references
and a cookie. It identifies a holder, not traffic exclusive to that process.
The diagnostic sampler described here retains its own scope.

`pcinsight/linux_tcp.hpp` provides an explicit read-only sampler without a
CLI, daemon, libbpf, systemd SDK, or firewall changes. It uses
NETLINK_SOCK_DIAG, INET_DIAG_INFO, and TCP_INFO directly. It is not enabled
in Monitor or in its default collection. It does not elevate privileges or
install dependencies.

```cpp
#include <pcinsight/linux_tcp.hpp>
pcinsight::LinuxTcpSampler tcp;
auto baseline = tcp.sample(); // Counters; rates are not available yet.
// On the next application tick, not immediately in a busy loop:
auto next = tcp.sample();
if (pcinsight::metric_is_known(next)) {
    for (const auto& socket : next.value->sockets) {
        // acknowledged_bytes_per_second, received_bytes_per_second
        // These are not total traffic per PID or Ethernet throughput.
    }
}
```

## Exact scope

The query covers IPv4 and IPv6 TCP sockets in ESTABLISHED (1) and
CLOSE_WAIT (8), within the **calling thread's** network namespace. It
excludes other states, sockets that disappear between samples, UDP,
QUIC, Unix sockets, and other families or protocols. The API does not
collect addresses, remote ports, or traffic contents, only diagnostic
identity, UID, state, and counters. This is not a packet capture tool.

acknowledged_bytes and received_bytes retain the native cumulative
tcpi_bytes_acked and tcpi_bytes_received fields. Queues and segments are
not converted to bytes. Acknowledged bytes are not bytes sent at that
instant: they depend on remote acknowledgments. The counters are not
physical interface bytes, headers, retransmissions, or total machine
traffic. Do not add the two to invent throughput. Missing fields or a
shorter TCP_INFO remain unknown per field, not zero.

The users grouping uses the **kernel-reported socket_uid**, not username,
the process's real or effective UID, a logind session, or causal
responsibility for an app. Sockets can be shared, inherited, or transferred.
Unrepresentable UIDs can map to the namespace's overflow UID; the provider
does not promise to distinguish their original identities. It neither
infers a PID from inode/fd nor arbitrarily assigns a shared socket to a
process. It retains dev/inode identity for both the network and **user**
namespaces. Do not combine these rows with J04 real-UID totals as equivalent.

## Rates, identity, and coverage

Each row records monotonic captured_at when its datagram is received.
The tracker requires a persistent native cookie and matching family,
inode, socket_uid, and namespaces. Inode alone is not an identity.
INET_DIAG_NOCOOKIE leaves counters visible without a rate. The first
observation, lost sample/socket/family, reset, changed identity or
namespace, and time without progress do not produce rates. If any known
counter decreases, the interval is discarded in both directions. A missing
field requires two consecutive valid observations of that field without
blocking the independent direction if it has counters. Comparing extreme
clocks does not overflow signed duration, and summing does not overflow uint64.

Rate is the delta divided by the socket's own elapsed seconds, in
integer bytes/s rounded down, with Support::estimated. Observed zero
is known; unknown is not replaced with zero. Duplicate cookies invalidate
the batch and clear baselines to prevent double counting. Sums by socket
UID retain known and missing counts per direction and an overflow flag.
Without known rates there is no sum, even when sockets exist. Some known
rates yield an estimated partial sum, not extrapolation. Overflow leaves
the total absent, not truncated or saturated. Socket windows can differ,
so this is not an atomic snapshot or a global user total.

ipv4_dump_completed and ipv6_dump_completed distinguish a complete
empty dump from an unavailable, denied, or incomplete family. If one
family fails, its whole partial response is discarded and the other is
retained with an explicit diagnostic. If both fail, the outer Metric
has no value and preserves observed denial. An empty dump does not
establish absence of traffic in other states or protocols. A failed sample
or family clears its baseline; rates do not bridge an unknown interruption.

## Protocol, permissions, and limits

Each query opens a dedicated Netlink socket and checks kernel origin
(PID 0), local port, sequence, lengths, alignment, attributes, dump
interruption flags, errors, and NLMSG_DONE completion. Truncation,
overrun, errors, exhausted time budget, inconsistent identity, or data
after DONE are not published as a valid enumeration. Namespaces are read
through /proc/thread-self/ns/net and /proc/thread-self/ns/user before and
after. A change during the query invalidates the entire sample.

These fields do not require CAP_NET_ADMIN by design. Actual availability
depends on the kernel, modules or configuration, namespaces, and
LSM/seccomp policies. Denial is reported, not bypassed with sudo. The
library does not load eBPF programs or call insmod/modprobe. The kernel
can resolve its own modules on demand when handling a query. Compilation
requires Linux UAPI headers with the TCP_INFO fields used; an older
runtime kernel can return shorter fields. Windows keeps the API, but
sample returns absence with a reason.

Per family, the limits are 50,000 rows, 16 MiB of datagrams, 100,000
messages, 256 KiB per datagram, eight interrupted operations retried,
a two-second budget between operations, and up to 100 ms per poll.
There is no hard syscall deadline guarantee. The combined maximum is
100,000 rows. That count bounds tracker memory, but storage is not
free: previous counters are retained. The object is not thread-safe.
The host must serialize sample/reset and avoid concurrent namespace
or credential changes. There are no background tasks or global caches
across instances.

## Project status and tests

This native implementation is useful but **does not by itself close F03**.
At this checkpoint, identity-aware per-process/app attribution, shared
sockets, UDP and other coverage, and an optional accounting provider
were still missing. It does not close J04 or H04 either. Legacy
ProcessInfo and UserSession network fields remain absent. Partial TCP
is not relabeled total traffic. No resolved ID or fractional credit
is added to the tracker. The additional header and symbols do not
change existing structs, history format, or virtual ABI. Rebuild and
relink to use the new symbols.

Portable tracker tests and Linux Netlink fixtures cover identity,
clocks, resets, missing data, zero, overflow, malformed or truncated
replies, sequence, port, family, state, NOCOOKIE, short/missing/duplicate
INFO, errors, and interrupted dumps. The native test creates only its
own sockets on 127.0.0.1, transfers 8,192 B, and checks counters and rates
for its cookie. It closes its sockets without modifying other
connections. If the native provider is absent, it explicitly returns
skip 77, not positive verification. Final results for this environment
are also recorded in SEGUIMIENTO.txt.

Verification on 2026-10-17: Linux Release passed 94/94 and Windows
Release passed 110/110, with no native TCP skip. Installed external
consumers were rebuilt and relinked and exited 0. Linux also queried
the installed API, and Windows checked absence with a reason. Library
code was unchanged after that installation: the last addition strengthened
receipt and IPv6 decoder tests only, and the complete suites were rerun.
Real WSL IPv4 loopback: client cookie 16385, socket_uid 1000, acknowledged
delta 8,192 B, server received 8,192 B, sampled rates 42,999,916 B/s,
two sockets, and one UID group. This is not a physical benchmark,
permanent value, or total traffic. The positive IPv6 parser case is
synthetic; native IPv6 transfer is not claimed. Logs:
work/tcp-diag-{build,verified-build,final-build}-{linux,windows}.log,
work/tcp-diag-{suite,final-suite}-{linux,windows}.log, and
work/tcp-diag-final-native-linux.log. Builds had no warnings. Passing
runs do not establish fixes for historical N03 CPU or history risks.

Primary sources: [inet_diag UAPI](https://github.com/torvalds/linux/blob/master/include/uapi/linux/inet_diag.h),
[UID and diagnostic structure](https://github.com/torvalds/linux/blob/master/net/ipv4/inet_diag.c),
and the [kernel TCP provider](https://github.com/torvalds/linux/blob/master/net/ipv4/tcp_diag.c).
