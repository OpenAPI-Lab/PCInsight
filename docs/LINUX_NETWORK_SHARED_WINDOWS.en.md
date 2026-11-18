# Authenticated windows on the Linux shared producer

Revision 93: [public observed-mode activation](LINUX_NETWORK_SHARED_PUBLIC.en.md)
was verified in the standalone provider, Monitor, Sampler, and the selection
owner. The activation limits below for revisions 90 and 92 are historical;
general admission was still pending.

Revision 92 follow-up: [authenticated updates](LINUX_NETWORK_SHARED_UPDATES.en.md)
now preserve retained actors and admit or readmit actors with fresh capture
in bounded, disjoint batches. Public activation was still pending. The fixed
selection and N+14 cost below describe revision 90. After updates, the cost
is N+6+8G, where G is the number of batches.

Revision 90, 2026-10-22. This internal F03 unit is complete:
LinuxSharedSelectedNetworkSampler connects the shared producer to owned
pidfds, verified associations, nominal windows, four estimated rates, and
partial UID sums. This is a FIXED observed selection of 1–64 actors with
input cookie zero. At this revision it does NOT replace the public provider,
Monitor/Sampler, or their update_selection with retained counters.

## Authentication and ownership

Start requires consent, a complete list without duplicates, and valid
PID/start/pidfd values. Shape, capacity, and cookie errors are rejected before
accessing sources. The sampler retains its own copy of each pidfd, so the
caller can release the originals. It reads producer identity, verifies each
kernel/procfs association and shared boot and observer context, checks kernel
identity uniqueness, and revalidates ALL actors after starting capture. Failure
at any step releases capture, query resources, and all owned references.

It uses ONE reusable LinuxPidfdIdentityQuery for the whole selection when
the source offers join_query. Older join/join_layout sources still work.
Permissions and identities are not cached as current results. Each sample
joins ALL actors before and after the sweep. Each native join retains TWO
independent kernel queries and its procfs context checks at both endpoints.
The sampler checks PID/start/UID/kernel start, pidfd liveness, current
permission, and boot/observer/context. It does not convert local UID/PID
integers to initial-domain IDs; both domains remain separate.

There are three shared traffic links and one gated identity-query link,
disarmed outside a query, not three traffic links PER process. After the
external owner is released, the sampler owns N+14 FDs: N pidfds, eight
producer FDs, and six query FDs. The fork guard rejects sampling and closes
only the copy's references without altering the parent's maps or capture.
Stop and destruction close everything.

## Windows and quality

The full sweep has a global writer/sequence guard, per-actor validation,
immutable records, and a monotonic bracket of at most one second. Joins
fall outside the COUNTER bracket. There is no hard deadline, future liveness
guarantee, or atomic system snapshot. The nominal window uses the sweep end
for all selected buckets. The first observation of a bucket has no rate or
interval, not zero. Actors without traffic retain metadata without fabricated
buckets or users.

If the final join or any identity, permission, quality, context, or clock
check fails, no partial result is published and ALL baselines are discarded.
The next accepted sample has no rates. Discarding windows does not clear the
producer's regression checks. discard_window explicitly resets the baseline.
The existing reducer uses selected_observed_actor_set, its own epoch,
kernel_initial domains, and partial UID sums with known/missing/overflow.

The result is PrivateCommonNetworkWindow. The existing
publish_linux_process_network projection validates its fields and produces
the same public schema. Testing the projection does NOT enable the backend
in LinuxProcessNetworkSampler. Start on an active session is rejected.
Stop/start creates fresh counters and a new epoch; it is NOT equivalent to
updating the selection while preserving counter origins.

## Evidence

- New fixture tests/linux_network_shared_selected.cpp: 64 actors and 1,024
  buckets, four rates of 20 B/s per bucket over a 2 s interval, two initial
  UIDs distinct from the local UID, 512 known buckets per UID/lane and a
  sum of 10,240 B/s per UID/lane; empty metadata, no rate for the first bucket,
  validated projection, reset/recovery, and rejection of the whole result
  when the last AFTER join fails. Only three traffic loads/links for 64 actors.
- Startup failures and exceptions at each operation exercise rollback.
  Older join/layout behavior is retained. Tests cover duplicate kernel
  identities, start/context/boot/observer, quality, permission, clock, fork,
  and endpoint failures. discard_window does not reopen counters. At this
  revision, stop and restart to change the selection.
- Private manual WSL run `--authorized-private-shared-windows17` in
  pcinsight-linux-network-capacity-native, outside CTest: 17 children,
  owner released, 31 owned FDs, empty registry/first bucket/rates and UID
  sums checked against an integer oracle. UDP 64–80 B over IPv4/IPv6 and
  TCP four times 64–80 B per direction exercise all four lanes. Switching
  main→worker→main preserves identity and counters; parent TCP/UDP is excluded,
  fork is rejected without affecting the parent, and one exited actor causes
  the whole result to be rejected twice. Stop and start with 16 actors gives
  a new selection and empty epoch, fresh counters, discard/recovery, and
  30 owned FDs. All children exit normally and final FD counts are clean.
  From harness startup: prepared at 16 ms, started at 420 ms, measured at
  517 ms, and stopped at 2,262 ms. This includes joins, IPC, traffic, and restart;
  it is NOT a benchmark or latency promise. One real namespace was shared,
  and SO_NETNS_COOKIE was initialized for the oracle. The 64/1,024 cases are fixtures.
- Linux Release 130/130 without skips; three tests repeated 20 times each
  (60 runs); installed external consumer rebuilt and exiting 0. MSVC passed
  3/3 focused tests and 20 repeats of the new test, with no new C++ warnings;
  D9025 is expected. The initial MSVC fixture segfault is preserved in
  network-shared-window-final-targeted-msvc.log: a range-for iterated data
  from a temporary result that had already been destroyed. The sample is
  now stored before iteration; final2-targeted-msvc.log and repeat-msvc.log
  pass. This is NOT claimed as the cause of the revision 75 consumer failure,
  which remains open in N03. See work/network-shared-window-* and the
  final-suite and final-repeat Linux logs.

## Next step at this checkpoint

Update the shared selection dynamically: keep retained captures and counter
origins, remove actors, and admit new ones without including gap traffic or
unsafely reusing slots while events are in flight. Only then enable the
backend in the public provider, Monitor, and Sampler, followed by automatic
admission with explicit coverage. This revision does not change the public
update_selection contract or add a fallback that silently resets retained
process counters.

No new public API/layout, GUI, persistence, dependencies, CLI, or permanent
configuration. BPF tests remain limited to authorized private processes,
without general capture. F03/H04/J04 remain open; 57/75 = 76.0% closed,
18/75 = 24.0% open.
