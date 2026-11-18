# Authenticated changes to the Linux shared selection

Revision 93 follow-up: the [public observed backend](LINUX_NETWORK_SHARED_PUBLIC.en.md)
enables this path in LinuxProcessNetworkSampler, Monitor, Sampler, and the
selection owner, with dedicated native tests. The pending activation below
is historical. General admission and history/app/session integration were
still pending.

Revision 92, 2026-10-22. This internal F03 unit completes
`LinuxSharedSelectedNetworkSampler::update_selection`. It does not yet replace
the PUBLIC backend, Monitor, or Sampler. There is no automatic admission.
F03/H04/J04 remain open until their criteria are met, with no partial credit.

## Transition contract

Each update requires fresh consent, 1–64 entries with unique PID/start/pidfd
selections, and zero cookies. Consent, shape, and capacity errors leave the
selection and rates unchanged. An empty list is rejected; use stop. The
sampler keeps its pidfd copies for matching PID/start rows and obtains its
own copies for new actors. As in the earlier public update, a proposed FD
for a retained actor does not replace the one already owned by the sampler.
Permission results are not cached as current authorization.

Fresh joins authenticate every proposed actor BEFORE and AFTER the change.
Each native join contains two independent kernel queries. Kernel tokens,
boot, and observer context must be consistent, with unique actor identities.
Retained actors must still match their original identities. A context change
between endpoints, denied permission, exited actor, or runtime exception
closes the ENTIRE session, including the query and pidfds. No partial selection
is kept. New pidfds have independent RAII guards until commit.

Every success, including a reorder, creates a new epoch and discards ALL
rate baselines, contexts, and the previous bracket. Retained actors keep
the origin of their bytes, cookies, and sequences. Readmitting a removed
actor does NOT preserve its previous origin or count the interval it was absent.

## Disjoint batches without recycled slots

`LinuxSharedNetworkSet` combines shared producers by admission batch.
It starts with one batch. Before adding replacements, it removes departing
rows from each batch's HASH; retired records are never cleared or reused.
It then starts a NEW shared producer for all additions in that update.
A batch with no active actors is closed entirely. Retained actors are never
detached, restarted, or moved to another map.

This avoids reassigning a slot that an older invocation may still write to.
An already admitted invocation finishes only in its retired record in the
old map, which is not published as an active actor. New invocations for a
removed actor exit before reading context or counters. In-flight events are
not guaranteed to be revoked. See [safe removal](LINUX_NETWORK_SHARED_RETENTION.en.md).

For N active actors and G nonempty batches, the cost is N pidfds, six reusable
query FDs, and 8G traffic FDs (two maps, three programs, and three links per
batch). At this revision, 1 <= G <= N <= 64; startup costs N+14. Selection
changes can fragment capture into up to 64 batches, with at most 582 persistent
FDs after an operation in this native mode. There is no promise of three
constant probes after updates or universal improvement over earlier fan-out.
Each batch has at most 64 original records. Memory and links are bounded by
live batches, NOT by the lifetime number of updates. There is no lifetime
budget of 64 additions. Update preparation can hold up to 64 NEW pidfd copies
alongside the old ones (up to 646 owned persistent/provisional FDs before
commit), plus transient query/BTF FDs. Published native FD counts are taken
after each operation, not at a measured peak inside a syscall.

## Reads and quality

Each batch retains its writer/sequence bracket and regression checks, including
retired records to reconcile its global sequence. Output follows proposed
identities, not recycled indices. The sweep of ALL batches checks time order
and takes at most one second. It is not an atomic cut across batches: one
batch's counters can change while another is being read. Nominal rates and
UID sums remain estimated and partial, not global throughput, link-layer
packet counts, app/user totals, or socket ownership.

A batch failure, sequence-sum overflow, or context/clock/join/shape failure
rejects the ENTIRE frame and clears all sampler baselines. Already-read raw
checkpoints still guard against regression; they are not cleared to recover
from sticky quality failure. The next valid frame establishes rate baselines.

## Evidence for this unit

- Retention/reordering/addition/readmission fixtures verify preserved bytes,
  absence of a 1,000-byte gap, no initial rate for new buckets, a new epoch,
  reset of all intervals, owned copies, and closure of retired pidfds.
- 80 alternating updates retaining one actor: two live batches and 16 traffic
  FDs. The retained counter grows and each readmission starts fresh.
- Worst-case fragmentation: 64 batches, 64 actors, 1,024 buckets, four rate
  lanes, and rejection of 65 without mutation. Reducing to one batch preserves
  bytes and releases 63.
- Injected failure and exception at EVERY update operation, including deletion,
  new load/attach, and the last join: all resources close. Tests reject bad
  UID, duplicate kernel identity, boot, observer, fork, and quality.
- Quality loss in one batch rejects the whole frame and resets rates. Local
  sweeps under one second are also rejected if their combined sweep exceeds it.
- Private native startup with 17 actors and restart with 16: 16→15→16
  while the removed actor keeps sending. Retained actors keep TCP/UDP bytes;
  the readmitted actor has no old bucket or gap bytes. A new batch adds exactly
  eight FDs. TCP/UDP IPv4/IPv6 windows, four rates per nominal interval, and
  UID sums are checked against an integer oracle. Reordering does not rebuild
  producers but resets rates and epoch. The original owner is released,
  the forked copy is rejected, an exited actor rejects the whole result,
  and children and FDs are cleaned up.

Logs: `work/network-shared-update-*`. Linux Release 130/130 without skips
and 40 focused runs; installed consumer exited 0. MSVC ran 40 focused tests,
not full Windows certification or an N03 fix. Final build had no new C++
warnings; D9025 is expected. Revision 92 mirror confirmed: comment
90140272299452. The TXT remains authoritative. Native testing used only
one real namespace and 17 private children. The 64/1,024 cases are fixtures,
not universal native certification. No new capture CLI, dependency, installation,
permanent configuration, history, GUI, or public layout.

Next at this checkpoint: enable and verify this backend in the public observed
provider and its Monitor/Sampler/owner paths. A private projection accepting
internal frames is not evidence that public integration is complete.
