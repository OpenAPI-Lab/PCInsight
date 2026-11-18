# Public API: observed cookies for selected processes

Revision 93 enables the [public shared backend](LINUX_NETWORK_SHARED_PUBLIC.en.md)
in standalone, Monitor, Sampler, and owner APIs. A native run with 17 actors
verified the initial N+14 FD count, updates, retention, readmission, and
cleanup. Earlier notes below about an unshared producer and cost-related
failures are historical. The 64/1,024 cases are still fixtures at this
checkpoint; this change does not establish general admission or app/user totals.

Revision 88: the [gated query](LINUX_NETWORK_QUERY_SESSION.en.md) passed the full
17-actor native test, including rates and cleanup. This did not certify a
shared producer or 64 real actors.

Historical revision 87: [structural capacity for 64 and actual cost](LINUX_NETWORK_CAPACITY64.en.md).
The limit of 16 processes and 256 rows cited below is historical; the limit
of 16 cookies per actor is unchanged. The 17-actor native run failed due to
cost and bounded target lifetimes; scalable capture was NOT certified. Each
revision's original evidence is retained.

Revision 83 exposes observed_namespace_buckets in LinuxProcessNetworkSampler.
It is explicitly opt-in; the default constructor stays strict_single_namespace.
There is no global capture or general process enumeration. Revision 84 adds
three-argument opt-in setup in [Monitor/Sampler](LINUX_PROCESS_NETWORK_OBSERVED_MONITOR.en.md).
Historical revision 83 evidence follows.

## C++20 usage

```cpp
#include <pcinsight/linux_process_network.hpp>
#include <array>

pcinsight::LinuxProcessNetworkSampler network(
    pcinsight::LinuxProcessNetworkSelectionMode::observed_namespace_buckets);

pcinsight::Metric<std::uint64_t> start_network(
    int pidfd, std::uint32_t pid, std::uint64_t start_ticks,
    bool consent)
{
    const std::array<pcinsight::LinuxProcessNetworkSelection, 1> selection{{
        {pidfd, pid, start_ticks, 0} // Zero is REQUIRED: no manual cookie.
    }};
    return network.start(consent, selection);
}

// Use the caller's worker; do not block the GUI thread:
// auto snapshot = network.sample();
// if (pcinsight::metric_is_known(snapshot)) { ... }
// network.stop();
```

PID and start must come from the caller's same procfs/boot view. Use the
library's PID and known ProcessInfo.linux_start_ticks and a live pidfd for
that process. Do not convert ticks to kernel start or assume local PID/UID
are initial-domain IDs. Setup owns a duplicate, so the original can be closed
afterward. No CLI runs, and callers need not create CMake before their GUI:
including the headers and linking the updated library from an IDE also works.

At this revision, select 1–16 distinct processes with input cookie zero in
ALL rows. Manual cookies, duplicates, and mode changes during an active
session are rejected. It discovers up to 16 SOCKET namespace cookies per
actor (256 rows total) during positive instrumented operations. It does not
inventory idle sockets or processes in general. Construction and stop do
not load BPF; start with false does not authorize capture. Updates require
fresh consent and a complete selection. Move, stop, and restart preserve the
mode. Authorized start outside Linux returns unsupported, with no new Windows
implementation.

## Results and availability

Snapshot coverage observed_tcp_udp_socket_namespace_buckets distinguishes
this mode from strict and declared modes. observed_processes contains 1–16
associations in selection order: local PID/start/context, separate kernel
TGID/initial real UID/start, observed_buckets, and actor-wide completed_updates.
There is no fabricated cookie zero.

processes remains a vector of BUCKETS, not one row per process. Each row has
a real cookie, identities, bytes, exclusions, and rates. observed_processes
retains an idle actor even when it has no bucket. An empty selection is
invalid, but an empty traffic registry is valid: processes/users are empty,
observed_processes is nonempty, and counts are zero. This does not establish
that the process uses no network through other or uninstrumented paths.

Associate by identity, not index. Accounting and buckets are sorted by key;
associations follow selection order. completed_updates REPEATS across an
actor's cookie rows because it is an actor-wide sequence. Do NOT sum it by
cookie; the single value is also in observed_processes. Exclusions are per
bucket, not bytes. Do not present any sum as a process or user total.

The first observation of EACH bucket has bytes but NO rate or interval,
even after earlier empty registries. Late arrivals do not invalidate other
baselines. A second clean sample can yield measured zero or a positive rate.
The snapshot is estimated because of partial coverage and sweep skew;
verified bytes are available, and rates and UID sums are estimated. Respect
each Metric: absence is not zero. Array order is TCP send/receive, then UDP
send/receive. Users represent the initial UID at operation time, not a local
account, session, or username.

There is one sweep bracket of at most one second and a nominal interval
between endpoints, NOT atomic acquisition. Actor exit, sticky quality
failure/overflow, inconsistent identity/context, or a read/clock failure
rejects the ENTIRE group and clears ALL baselines. Recovery has no rate, and
intervals over 60 seconds do not produce rates either. An inconsistent
frame or exception stops capture. The producer retains the monotonicity
guard for validated registries and counters when discarding windows, so an
error cannot conceal a regression.

Update preserves the counter origin for captures with matching PID/start.
Removal closes resources; re-addition creates an empty registry with no
traffic from the gap. Every successful update changes the epoch and resets
ALL rates and brackets. Invalid input shape or consent does not mutate
capture; a runtime failure closes everything. Stop/restart requires fresh
opt-in and creates new maps. Fork rejects read/update and closes the
inherited copy of references without affecting the parent.

## Permissions, quality, and compatibility

Compatible kernel/BTF/FEXIT/pidfd/64-bit atomics and BPF/ptrace/LSM permissions
are required. Unavailable or denied results carry a note, never fallback to
adapter traffic. An uninitialized zero cookie, 17th namespace per actor,
overflow, or identity/read failure causes sticky absence rather than eviction.
The provider does not force cookie initialization in other namespaces.
It counts payload from instrumented TCP/UDP operations, not wire bytes, ACKs,
interface totals, exclusive ownership, or every I/O path. Serialize calls and
use the same FD table. Do not manipulate owned FDs, unshare, or cancel
asynchronously. There is no hard BPF deadline or automatic privilege elevation.

**0.x layout change:** LinuxProcessNetworkSnapshot adds observed_processes
and LinuxNetworkObservedProcess is a new type. Types containing the snapshot,
including SystemSnapshot, also change layout. Rebuild the library and ALL
consumers; do not mix older objects or binaries with new headers. Enum values
and limit constants were added; update exhaustive switches. Earlier APIs
retain their policies and results, with observed_processes empty. No new
persistence format or GUI; durable history is not integrated here.

## Verification

linux-process-network retains prior fixtures and adds observed-mode cases:
empty traffic with two associations, idle and active actors together, first
bucket without a rate, different orders, separate local/kernel IDs, several
cookies and repeated update without duplicate UID accounting, the projection
limit of 16×16=256 rows, and 23 invalid frames involving identities,
duplicates, quality, impossible inactivity, cardinality, zero cookies,
bytes/UID/sequence, or ambiguous structure. Projection publishes no partial
results. At revision 83 the Monitor test confirms observed mode is rejected
without calling the provider, not falsely supported. The installed consumer
links the new types, mode, and limits and checks consent without capture.
The 256 positive rows are fixture data.

Manual `--authorized-private-public-observed` tests the PUBLIC API with
two authorized private children. It requires cookie zero, keeps the strict
default intact, duplicates pidfds, closes originals, and moves an active
sampler. Metadata retains idle actors; the first and late buckets have no
rate. Two concurrent threads per actor produce A TCP 12,288/12,288, UDP
768/768, and 24 peeks, with twice that for B. Four rates per actor and the
UID sum are checked against an integer oracle. Checks cover parent exclusion,
fork rejection, update consent, removal and re-addition preserving retained
origins and excluding the gap for new captures, and clean epochs and resets.
B's exit rejects the group twice; removal recovers without crossing a window.
Stop/restart retains observed mode, rejects a declared cookie, and starts
empty. Both children exit without FD leaks.

Only ONE real namespace was shared. The child initializes SO_NETNS_COOKIE
for an independent oracle and does NOT pass it to the producer. This does
not test multiple real namespaces, uninitialized cookies, 16 real actors
or 256 real buckets, or every distro/kernel. Parent/child watchdogs are
60 s/50 s. No persistent change, CLI, dependency, or global capture.
Linux Release 126/126 without skips, three tests repeated 20 times each,
a warning-free library build, and the installed consumer rebuilt, linked,
and exiting 0 with the new header and layout. The native strict public
regression passed. MSVC passed 3/3 focused shared-code tests without new
C++ warnings; expected D9025 keeps assertions active. No new full Windows
suite or consumer run. The revision 75 consumer failure remains open in
N03, so full compatibility is not claimed. Logs: work/network-public-observed-*.

At revision 83, F03 remains open. The next step is observed mode in
Monitor/Sampler, then general selection policy. H04/J04 stay open; do not
fill ProcessInfo/UserSession/AppHistoryEntry with these partial data as totals.
