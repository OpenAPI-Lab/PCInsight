# Declared namespace buckets per process

Revision 88: the [gated query](LINUX_NETWORK_QUERY_SESSION.en.md) passes the full
17-actor native test, including rates and cleanup. This does not certify a shared
producer or capture of 64 real actors.

Historical revision 87: [structural capacity for 64 and actual cost](LINUX_NETWORK_CAPACITY64.en.md).
The 16-process/256-row limit cited below is historical; the limit of 16 cookies
per actor does NOT change. The native 17-actor test failed because of cost and
bounded target lifetimes; scalable capture was NOT certified. Each revision's
original evidence is retained.

Revision 78 adds an opt-in mode to the public LinuxProcessNetworkSampler.
A selection can repeat PID/start when each row has a DIFFERENT socket namespace
cookie. At this revision the limit is 16 rows total, not 16 cookies per process.
It does not automatically discover cookies or processes, enter other namespaces,
require new packages, CLI tools, or SDKs, or automatically elevate privileges.

```cpp
#include <pcinsight/linux_process_network.hpp>

pcinsight::LinuxProcessNetworkSampler network(
    pcinsight::LinuxProcessNetworkSelectionMode::declared_namespace_buckets);
std::array<pcinsight::LinuxProcessNetworkSelection, 2> selection{{
    {pidfd, pid, start_ticks, cookie_a},
    {pidfd, pid, start_ticks, cookie_b}
}};
auto started = network.start(consent, selection);
// Use sample/update_selection/stop as with the earlier selected provider.
// Check support/coverage. The first sample in each epoch has no rates.
```

The default constructor retains strict_single_namespace: one row per process,
one declared cookie, and detection of another cookie invalidates capture.
The earlier mode does not silently change. An invalid constructor enum does not
start capture. Stop/start/update retain the mode; move transfers configuration.
Calling start on a moved-from object without its Pimpl reconstructs strict mode.
The mode cannot change during active capture. The API and enum are additive,
with no new public layouts. Link the current library for the new constructor.
Rebuild all consumers if coming from layouts before revision 75. Consumers
can use their IDE without CMake.

## Filtering and quality

Each bucket owns its map, links, and pidfd, and uses a program filtered by
initial TGID, leader start/UID, and the cookie of the SOCKET used by the
operation. It reuses live BTF and the verified read and writer/sequence/overflow
guards. The process's current namespace is not a substitute for the socket's.
Inherited or transferred sockets are not attributed by ownership alone; the
provider still counts the actor's operations.

In bucket mode, a different NONZERO cookie exits BEFORE writing to the map,
including excluded PEEK/TRUNC/ERRQUEUE receives. Two disjoint buckets therefore
do not count the same operation. Cookie zero, null pointers, read or identity
errors, overflow, and inconsistent quality still fail closed rather than
becoming zero. Do not sum these filtered buckets with strict or unfiltered
capture for the same actor. A row has one cookie, not a range or wildcard.

An unobserved cookie yields zero within ITS filter, not proof that the
namespace exists or that its total traffic is zero. The caller must obtain
the declared value from a correct source. This API does not verify its existence
or enumerate the target's sockets or network namespaces. A namespace without
an initialized cookie or compatible layout can remain unavailable. The provider
does not classify contents, addresses, or packets.

## Identity, windows, and reporting

The provider rejects duplicate PID/cookie pairs, different start_ticks for the
same PID, duplicate kernel buckets, distinct procfs identities for the same
kernel identity, and different UIDs or contexts for the same process. Local and
initial domains remain separate. Each actor is revalidated at both endpoints.
There is no atomic snapshot or continuity guarantee between them. Any failure
invalidates ALL common baselines.

coverage=declared_tcp_udp_socket_namespace_buckets distinguishes this report.
processes contains BUCKET ROWS, not deduplicated applications: the same process
can appear with different kernel.socket_network_namespace_cookie values.
Associations are resolved by key, not vector order. UID observed_buckets counts
buckets, not processes or sessions. It sums partial rates with
known/missing/overflow and does not present a global, app, or user total.
These partial values do not fill ProcessInfo/UserSession/app_history.

An explicit full update retains matching PID/start/cookie captures, closes
removed ones, starts new ones, creates a new epoch, and discards ALL baselines
and brackets. Retained counters keep their origin; new captures do not recover
traffic during gaps. Denial or a malformed list retains the selection; a runtime
failure closes EVERYTHING. Do not add deltas or join charts across epochs.
Use stop for an empty list.

Revision 79 adds [explicit bucket setup in Monitor/Sampler](LINUX_PROCESS_NETWORK_BUCKET_MONITOR.en.md).
Earlier calls remain strict and do not inherit bucket mode between sessions.
Automatic policy and discovery, dynamic buckets without manual cookies, and
broader coverage were still pending. F03/H04/J04 do not close here.
Revision 80 adds [private observed-cookie capture](LINUX_NETWORK_DISCOVERY.en.md)
with no input cookie but does not yet connect it to this public provider or
its windows, joins, or multi-actor capture. The APIs on this page still use
declared cookies.

## Evidence

The bytecode interpreter covers IPv4/IPv6, TCP/UDP, send/receive, two positive
64-bit cookies for the SAME actor separately, a third excluded without writes
or double counting, excluded receives, cookie zero/null/errors/UID-start/
sticky overflow, and unrelated TGIDs. Sessions test filter metadata and
strict restart without a retained flag. Collection/reducer/projection tests
cover two buckets for one process with different cookies, a common window,
four rates and UID sum, reordering without recreation, removal/rollback, and
rejection of duplicates, inconsistent identities or UIDs, and the 16-row limit.
The first new overflow test initialized a different protocol from the tested
lane. The fixture was fixed without weakening the producer, and initial logs
were retained.

Manual `--authorized-private-namespace-buckets`, NOT CTest: two authorized
private children, with capture of ONE using a real cookie and a deliberately
unobserved one. Four positive native oracles, zero in the other bucket, and a
UID sum without double counting; exclusion of unselected-cookie traffic;
removal and re-addition with a clean epoch, baseline, and gap; exclusion of
the other child and parent; fork rejection; and no FD leaks. Only ONE real
namespace was exercised. This does not test two simultaneous real namespaces
or discover or establish the existence of the second. No setns/unshare or new
interfaces. Private parent/child watchdogs of 60 s/50 s are not hard BPF
deadlines. The `--authorized-private-public-window` regression also passed
with two actors in strict mode and their four actor/UID oracles.
Logs: work/network-buckets-native-linux.log and strict-native-linux.log.

The same CTests are used, with no new task or dependency. Logs with the
network-buckets prefix retain build, suite, repeat, install, consumer, and shared
MSVC compatibility evidence. There is no new Windows capture. The general
MSVC consumer failure from revision 75 remains in N03. Full Windows compatibility
or coverage of all kernels, distributions, and hardware is not claimed.

Result: Linux Release 126/126 without skips, five focused tests repeated 20
times each, a warning-free build, and the installed consumer rebuilt, linked,
and exiting 0. The library and 5/5 focused MSVC tests pass without new C++
warnings; expected D9025 keeps assertions active. Official revision 78 count:
57/75 = 76.0% closed, 18/75 = 24.0% open, with no new IDs or partial credit.
