# Public API for network traffic from selected Linux processes

Revision 97: [capacity for 1,024 processes](LINUX_NETWORK_SCALE.en.md), observed
batches of up to 64, structural capacity of 16,384 buckets, and a native test
with 65 private actors. The earlier limits below are historical. The number
of cookies per process remains 16, not 1,024. Rebuild ALL consumers.

Revision 88: the [gated query](LINUX_NETWORK_QUERY_SESSION.en.md) passed the full
17-actor native test, including rates and cleanup. This does not certify a
shared producer or capture of 64 real actors.

Historical revision 87: [structural capacity for 64 and actual cost](LINUX_NETWORK_CAPACITY64.en.md).
The limit of 16 processes and 256 rows cited below is historical; the limit
of 16 cookies per actor does NOT change. The native 17-actor run failed due
to cost and bounded target lifetimes; it did NOT certify scalable capture.
Each revision's original evidence is retained.

Revision 83 adds [opt-in observed mode](LINUX_PROCESS_NETWORK_OBSERVED.en.md)
to the standalone provider: no manual cookies, actor metadata even when
buckets are empty, and up to 16 cookies per process. The 0.x network snapshot
and SystemSnapshot layouts changed: rebuild ALL consumers. Revision 84
integrated [observed mode in Monitor/Sampler](LINUX_PROCESS_NETWORK_OBSERVED_MONITOR.en.md).
The strict contract and historical revision 74 evidence are retained below.
The default constructor remains strict.

Revision 74: `pcinsight/linux_process_network.hpp` exposes
`pcinsight::LinuxProcessNetworkSampler`. This additive API is separate from
Monitor and legacy fields. This revision does not change existing
SystemSnapshot/SampleOptions/ProcessInfo layouts. Construction and stop do
not attach probes. Outside Linux, authorized start returns unsupported,
with no new Windows implementation.

## C++20 usage

```cpp
#include <pcinsight/linux_process_network.hpp>
#include <array>

pcinsight::Metric<std::uint64_t> start_capture(
    pcinsight::LinuxProcessNetworkSampler& network,
    int pidfd, std::uint32_t pid, std::uint64_t start_ticks,
    std::uint64_t socket_namespace_cookie, bool consent)
{
    const std::array<pcinsight::LinuxProcessNetworkSelection, 1> selection{{
        {pidfd, pid, start_ticks, socket_namespace_cookie}
    }};
    return network.start(consent, selection);
}
```

Keep the object on the caller's worker and call `network.sample()` each cycle.
Check `metric_is_known(result)` and each rate Metric. The first sample or the
first after recovering a baseline has counters but no rates. Display absence
as unavailable, not zero. Arrays use TCP send/receive, then UDP send/receive;
LinuxNetworkPayloadLane names the indices. Stop and destruction release
resources. The API is synchronous and promises neither 1 Hz nor a hard deadline.
Keep this work off the GUI thread.

The caller can use the include directory and link the library from an IDE;
it need not set up CMake before building the GUI. The internal build continues
to compile and test PCInsight. Installation includes the new header; link the
updated library for its symbols. This unit creates or modifies no GUI or app build.

## Selection, permissions, and ownership

Under the original strict contract, start requires explicit opt-in and 1–16
distinct processes with live pidfds, local PID and start ticks in the same procfs
context, and ONE DECLARED socket namespace cookie per process. A false start
does not begin new capture or stop an existing session. No automatic elevation,
installation, services, or external commands. Denial is permission_required;
an incompatible source is unsupported, with a note. No invented data or
fallback to adapter counters.

ProcessInfo.linux_start_ticks and local PID are not the kernel's initial start
or TGID. For a selected TCP socket, read_linux_tcp_socket_holder already
exposes network_namespace_cookie if the system and permissions allow. It does
not automatically discover every socket. Do not invent a missing cookie.
Start does not attest the declared cookie: an instrumented operation in another
namespace invalidates capture. Idle or zero counters do not prove the cookie
exists or coverage is complete.

The sampler owns duplicates. The caller may close its pidfds after start,
not concurrently during that call. Serialize calls and use the same FD table;
do not unshare or asynchronously manipulate internal FDs. The object is movable
but not copyable. Move preserves capture; move assignment releases the
destination's old capture. Sampling a moved-from object fails closed, stop is
safe, and start can reuse it. Fork rejects sampling and closes only the child
copy's inherited references.

Compatible BTF/FEXIT/pidfd/BPF and ptrace/LSM policy are required. BTF architecture
and byte-order limits apply; real positive evidence is limited to WSL x86_64.
Discovery temporarily blocks signals in the caller and restores its mask,
which may delay handlers. No asynchronous cancellation, hard deadline, ptrace
attach, signal, or target stop. Accounting does not read traffic contents.

## Data and coverage

Typed coverage selected_tcp_udp_socket_operations counts payload from positive
returns on instrumented inet_sendmsg/inet6_sendmsg/sock_recvmsg operations for
TCP/UDP IPv4/IPv6. PEEK/TRUNC/ERRQUEUE receives are excluded and counted separately.
Other paths, asynchronous operations, offload, families, and protocols are
not included in these totals. This does not count wire bytes, packets, or ACKs,
or assume exclusive ownership of shared sockets. Attribution is to the process
performing the instrumented operation, from start onward, not retroactive history.

processes joins kernel identity (initial_tgid/initial_real_uid/start in ns/cookie)
with local procfs identity and quality BY KEY. observer retains its own context.
The local UID is expressed in the OBSERVER's namespace, not the target's.
Do not equate numeric IDs or convert ticks to ns. Each row includes counters
since start, four rate Metrics, the nominal interval, and quality counters.

users contains partial_payload_bytes_per_second per INITIAL UID, not a
username, session, or full total. known/missing/overflow preserve availability;
UID 0 and measured zero are valid. Rates rounded down per actor are summed
without wrapping on overflow. These partial values do not fill legacy
ProcessInfo/UserSession/AppHistoryEntry fields. The overall snapshot is
Support::estimated; verified counters are Support::available, while rates
and sums are estimated. Do not present partial coverage as a total.

The common window is NOMINAL. It retains current/previous brackets, epoch,
boot, and producer, with a sweep of at most 1 s, NOT atomic acquisition.
Endpoint joins do not eliminate ABA changes or establish future liveness.
See [window semantics](LINUX_NETWORK_COMMON.en.md). An ordinary failure clears
baselines and may retain capture for recovery; sticky failure or actor exit can
require stop/restart or a selection change. An exception or inconsistent frame
stops capture. The first recovered sample has no rates. Performance and
cadence are not guaranteed.

## Verification and remaining work

Pure projection tests check scope, domains, brackets, cardinality, deduplication,
key-based association, quality, bytes, availability, UID sums, and
known/missing/overflow. Fixtures use different orderings and local/kernel IDs,
a zero baseline without a rate, UID 0, overflow, and 34 invalid frames.
Construction, move, opt-out, and invalid arguments are tested without BPF
in CTest. An incorrect fixture field name in the first build was corrected
without weakening tests; the initial log was retained.

Manual `--authorized-private-public-window`, NOT CTest: two private children
do not disclose their kernel tokens. The test checks four actor and partial
UID oracles, coverage, identities, brackets, closed caller FD, active move
without losing the baseline, idle zero, parent exclusion, fork and exit
rejection, and no FD leaks. Log: work/network-public-native-linux.log;
nominal interval 2.307999437 s on this WSL run. The installed consumer includes
and links the new header without enabling capture. No unrelated processes
or global capture.

This completes the public SELECTED provider, not all of F03. Revision 75
adds [opt-in Monitor/Sampler integration](LINUX_PROCESS_NETWORK_MONITOR.en.md)
with a changed 0.x layout and recompilation of all consumers. Revision 76
allows [explicit selection updates](LINUX_PROCESS_NETWORK_UPDATE.en.md) without
recreating retained captures; it does not discover processes automatically.
Revision 77 adds [updates with the worker running](LINUX_PROCESS_NETWORK_WORKER.en.md).
Automatic selection, coverage, and dynamic buckets without overstating totals
are still pending at this checkpoint. H04/J04 history and sessions remain pending.
Revision 78 adds [declared namespace buckets](LINUX_PROCESS_NETWORK_BUCKETS.en.md)
through an opt-in constructor, allowing multiple rows/cookies per process
in the individual provider. Revision 79 lets callers
[choose a mode at Monitor/Sampler startup](LINUX_PROCESS_NETWORK_BUCKET_MONITOR.en.md),
without changing earlier strict calls. No automatic discovery or closure of
F03/H04/J04 at that checkpoint.

Historical results for the initial public unit: SEGUIMIENTO.txt revision 74,
logs work/network-public-*.

Verified: Linux Release 125/125 without skips, 17 network tests repeated 20
times each, and a clean final build. The header was installed under
include/pcinsight/ and an external consumer was built, relinked, and run to
exit 0 against that installation. Final-build, suite, repeat, install, and
consumer logs share that prefix. This does not establish universal CI,
hardware, or distro coverage, or a new Windows run.
