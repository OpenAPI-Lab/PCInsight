# Observed cookies: internal common window for multiple actors

Revision 88: the [gated query](LINUX_NETWORK_QUERY_SESSION.en.md) passed the
full 17-actor native test, including rates and cleanup. This does not
certify a shared producer or capture of 64 real actors.

Historical revision 87: [structural capacity for 64 and actual cost](LINUX_NETWORK_CAPACITY64.en.md).
The limit of 16 processes and 256 rows cited below is historical. The
16-cookie limit per actor does NOT change. The 17-actor native run failed
because of cost and bounded target lifetimes; scalable capture was NOT
certified. Each revision's original evidence is retained.

Revision 83 follow-up: the [public observed provider](LINUX_PROCESS_NETWORK_OBSERVED.en.md)
now projects this source, including metadata for actors without buckets,
rates, and quality. Observed Monitor/Sampler integration was still pending
at that checkpoint. The revision 82 contract and evidence follow.

Revision 82 extends LinuxMultiNetworkSampler with a fixed internal
observed_cookies policy (the fifth constructor argument). It does not
change the public API/layout, LinuxProcessNetworkSampler, Monitor/Sampler,
or history formats. F03 remains open: EXPLICIT process selection is not
general app discovery.

## Selection and consent

Observed mode requires 1–16 rows with valid pidfd/PID/start and
socket_cookie=0. It rejects nonzero cookies, duplicate processes, and
combination with declared namespace_buckets. It neither silently ignores
cookies nor changes policy after stop/restart. Start and EVERY
update_selection require explicit consent. Earlier modes retain declared
cookies and the strict default. Independent sample is unavailable in
observed mode; use sample_common.

Each actor retains its own pidfd and BPF registry for up to 16 observed
cookies, with the existing kernel identity filters and sticky quality
checks. The caller can close its pidfds. No enumeration of other processes
or sockets, utility execution, automatic elevation, new packages, global
capture, or permanent configuration. Earlier internal samplers' serialization,
context, and FD-table rules apply.

Updates retain captures with matching PID/start, close removed ones BEFORE
creating new ones, and revalidate all retained and added associations.
Retained captures preserve their counter origin and registry. Re-added
captures start empty, without traffic from the gap. Every successful update
changes the epoch and discards ALL baselines and brackets, even when
reordering or repeating a selection. Invalid input shape or missing consent
does not mutate capture. Runtime failure stops everything rather than
leaving a partial selection or reauthorizing on update.

## Sweep and result

Run pidfd/procfs joins for ALL actors before the sweep. Require consistent
boot and observer context and distinct kernel actors without ambiguous
aliases. Sweep complete valid registries, then repeat the joins for ALL
actors and verify endpoint contexts and the creator-process guard. One
counter bracket must be available, ordered, nonoverlapping, and no longer
than one second. This is NOT an atomic snapshot: rows are read at different
times with common nominal endpoints.

Internal selected_observed_actor_set coverage allows 0–256 rows, up to 16
actors and 16 cookies per actor; it does not relax limits for older scopes.
Initial UID must be consistent per identity, and cookies and rows must be
unique as applicable. The reducer rejects invalid limits or identities.
An idle actor still has an entry in observed with identity and context,
an empty list, and sequence zero. It does not invent cookie zero, a traffic
or user row, or a zero rate.

PrivateCommonNetworkWindow.observed contains ONE record per selected process
in selection order, with kernel identity, local context, buckets, and the
whole ACTOR's completed_updates sequence. Do not sum that sequence per cookie.
The old associations vector is empty in this mode; declared modes do the
opposite. Accounting contains rows sorted by actor/cookie key and partial UID
sums. Look up by key, not by position across vectors. Exclusions remain
per bucket and are not payload. The same cookie on two actors does NOT merge
their counters.

A new row needs two clean samples of its own before it can have a rate,
even if other actors already have baselines. An empty registry is not a zero
baseline. TCP/UDP send/receive rates are estimated: payload deltas for positive
instrumented operations divided by the interval between nominal group sweep
endpoints. They are not wire bytes, interface data, ACKs, total process
traffic, or exclusive socket ownership. Initial-UID sums report known/missing
buckets and overflow, not total user traffic.

Any actor/lookup/quality/join/clock/context/fork failure or exception
discards ALL baselines and previous_read_bracket without returning a partial
group. The last validated registries are kept as monotonicity guards even
after errors. New registries are committed only after ALL reads, joins, and
reduction complete; the first actor's capture data is not committed when
the last actor fails. Recovery takes a new baseline. Stop closes all
resources; fork closes only the inherited copy.

## Evidence

linux-network-multi covers consent, input shape, cookies, conflicting modes,
empty results with two identities, staggered arrival of buckets A and B,
the same cookie in distinct actors and an additional cookie, four rates
and known/missing UID sums, independent-mode rejection, update/reorder
without recreating captures, removal/re-addition without the old registry,
epoch/reset, stop/restart, and simulated fork. Twenty read failures include
the second actor failing after the first, joins/drift/UID/boot context,
lookup/exception/estimated values, writers/flags/overflow, clock, and
registry/counter invariants. Recovery does not span a hidden interval.
Five startup failures and a runtime addition failure clean up everything.
Fixtures cover 32 buckets across two actors/UIDs and zero payload from
exclusion-only activity. The reducer accepts 256 rows (16×16) and rejects
17 actors, 17 cookies per actor, and inconsistent UID. No real 256-bucket
measurement was made.

Manual `--authorized-private-observed-multi`, outside CTest: two authorized
private children, original pidfds closed, and NO input cookies. The first
result is empty, then the first actor/cookie has no rate. The later second
actor has no rate while the first measures zero. Two concurrent threads per
actor then produce cumulative TCP 12,288/12,288, UDP 768/768, and 24 peeks for
the first actor, and twice that for the second. Four rates per actor and
the UID sum are checked against an integer oracle with a common interval.
Parent traffic is excluded and fork is rejected. Removal/re-addition
preserves retained counters, clears the new capture's registry, excludes
traffic during the gap, and starts new epochs with ALL baselines cleared.
One child's exit causes group rejection twice. Removing it recovers without
crossing the previous baseline. Both children exit normally with no FD leaks.

Only ONE real namespace was shared. SO_NETNS_COOKIE initializes the children's
cookies for an independent oracle. This does not test multiple real namespaces,
uninitialized cookies, 16 real processes, or coverage of all paths, kernels,
or distributions. Parent/child watchdogs are 60 s/50 s, not hard BPF loading
deadlines. No unselected processes are captured.

The native strict common-window and individual observed-window regressions
passed. Linux Release 126/126 without skips, three tests repeated 20 times
each, installed consumer rebuilt, linked, and exiting 0, and no build warnings.
MSVC passed 3/3 focused shared-code tests without new C++ warnings; expected
D9025 keeps assertions enabled. No new Windows functionality or validation
of its full suite or general consumer. The revision 75 general consumer
failure remains in N03. Logs: work/network-observed-multi-*.

Next at this checkpoint: expose opt-in observed policy in the public selected
provider, preserving identities, empty registries, limits, quality, and
brackets, then integrate Monitor/Sampler. General selection policy is still
pending. F03/H04/J04 remain open, with no new persistence, GUI, or partial
progress credit.
