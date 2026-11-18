# Collection for multiple selected actors

Revision 88: the [gated query](LINUX_NETWORK_QUERY_SESSION.en.md) passes the
full 17-actor native test, including rates and cleanup. This does not certify
a shared producer or capture of 64 real actors.

Historical revision 87: [structural capacity for 64 actors and actual cost](LINUX_NETWORK_CAPACITY64.en.md).
The limit of 16 processes and 256 rows cited below is historical; the limit
of 16 cookies per actor does NOT change. The 17-actor native test failed
because of cost and bounded target lifetimes; this did NOT certify scalable
capture. Each revision's original evidence is retained.

Revision 72 introduces the internal, uninstalled `LinuxMultiNetworkSampler`.
It groups 1–16 [selected samplers](LINUX_NETWORK_SELECTED_WINDOW.en.md), each
with its own retained pidfd, capture, revalidated identity, clock, and baseline.
It does not enumerate processes, discover cookies, publish to Monitor or
history, or change Windows or the public API/ABI. At this checkpoint F03 is
in progress; H04/J04 are not closed.

## Startup and resources

start requires explicit opt-in and a fixed selection of pidfd, PID,
start_ticks, and cookie. An empty list, more than 16 entries, invalid
arguments, or duplicate local PIDs are rejected before descriptor duplication
or capture. Only ONE declared cookie is allowed per process; the same actor
cannot be captured for two different cookies and then summed. After the joins,
uniqueness is checked by initial TGID/start_boottime; UID and cookie are not
part of the deduplication key. Different processes can share a UID or cookie.

All samplers must see the same boot and collector context at startup.
A failure or exception rolls back every started sampler, including the current
one. Each owns a duplicate, so callers can close input descriptors after start.
The selection has its own epoch from the serial shared with SELF and selected
capture. Stop/restart creates a new epoch without retaining rate baselines.
Membership cannot change live: stop and start again with another selection.

Calls must be serialized, use the same FD table, and keep borrowed contexts
alive. A forked collector is rejected before reading any rows and closes its
inherited copies, not the parent's references. Destruction and stop close all
owned captures and pidfds without pinning or configuration changes.

## Reads, gaps, and availability

sample walks the selection in order. Each row performs full joins before and
after the read and the individual sampler's checks. It revalidates shape,
shared boot and observer context, and kernel/local uniqueness. The collection
is returned only if every row is valid; partial success is not presented as
complete.

If any row fails, even the second after the first row's reducer advanced,
ALL baselines are discarded. Exceptions trigger the same rollback. Context
changes between successful collections are also rejected and clear baselines.
The next stable collection starts without rates for any row; rates can return
afterward. No intervals span gaps. Sticky capture failures or target exit are
not hidden behind zeros and can require stop/restart or a new selection, as
with the individual provider.

`PrivateMultiNetworkWindow` returns an epoch and a vector of individual
windows, with separate local and kernel PIDs and UIDs. It is NOT a simultaneous
snapshot: each actor has its own timestamps, interval, and estimated rates.
There is no global observation timestamp or UID sum across actors with
different windows. UID sums within each row remain partial to the actor and
cookie. The selected list does not establish global coverage, total process
traffic, wire traffic, or commercial app identity. sample does not align
or aggregate windows. Revision 73 adds the separate
[`sample_common` method](LINUX_NETWORK_COMMON.en.md) with a bounded sweep and
estimated partial UID sums. sample retains independent intervals. Switching
modes resets all baselines.

The BTF, privilege, ptrace, and caller signal-mask limitations still apply.
There is no protection against all ABA changes, atomic endpoint observation,
future liveness, hard deadline, or performance guarantee. Cost grows with
each actor (four pidfd discoveries per individual sample). The limit of 16
is a resource and selection limit, not evidence of performance with 16 actors.

## Tests and next unit

Fixtures use two actors with the same UID and cookie but different TGIDs and
local PIDs, deliberately different 2 s and 3 s intervals, and four independent
oracles. Preflight rejects duplicates, empty lists, more than 16 entries,
invalid cookies, and missing opt-in. Five startup failures (permission,
exception, kernel alias, observer, and boot) check cleanup. Four failures in
the second actor (join, exception, capture, and context) after the first
verify recovery without rates for BOTH. Restart, epoch, and fork are also
tested. CTest does not load BPF.

Manual `--authorized-private-multi-window`, NOT CTest: the harness creates
two private children before loading probes. They report only cookies, not
kernel tokens. The parent reads start_ticks. Child A generates one unit of
TCP 4,096/4,096 B and UDP 256/256 B; B generates two units. The test checks
four lanes per actor with separate intervals, no cross-attribution between
A and B, parent exclusion, idle zero, closed input FDs, duplicate and fork
rejection, rejection of the entire group after B exits, and no FD leaks.
Harness watchdogs are 60 s and 50 s, not hard provider deadlines. No UID,
namespace, or configuration changes. Authorization covers private children
only, not system-wide capture.

Next for F03 at this checkpoint: bounded common windows and UID aggregation,
coverage, and buckets before public integration. Revision 72 in the official
SEGUIMIENTO.txt tracker and `work/network-multi-*` logs preserve the results,
limits, and percentage.

The native WSL run exited 0 with all those checks passing; see
`work/network-multi-native-linux.log`. Linux Release 124/124 without skips,
16 network tests repeated 20 times each, installed consumer rebuilt and
exiting 0, and no final build warnings. An initial for-loop indentation
warning was fixed and its log retained without relaxing tests. Final-build,
suite, repeat, install, and consumer logs share the same prefix. The positive
native result is limited to this WSL x86_64 environment. It does not validate
universal CI, hardware, or distro support, or performance with 16 actors.
