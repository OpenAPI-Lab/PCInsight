# Safe removal from the Linux shared producer

Revision 92 follow-up: [authenticated update](LINUX_NETWORK_SHARED_UPDATES.en.md)
combines removal with additions and readmission in new batches, keeping
retained counters intact. The public backend was still unchanged. The
revision 91 remaining work below is historical, not a current limitation
of the internal sampler.

Revision 91, 2026-10-22. Internal F03 unit:
`LinuxSharedNetworkSession::retain` removes actors without restarting capture
for retained actors. At this revision it is not yet the authenticated
sampler's update operation or the public provider's. F03/H04/J04 remain open.

## Contract

Explicit consent and a session in the creating process are required. The
proposal must be a subset of active kernel tokens with exact TGID/UID/start
and no duplicates. It does not admit new or returning actors. Output retains
original slot order, not proposal order. An empty set closes the entire
session. An identical proposal makes no syscalls and does not reset counters.

The proposal is validated and allocated before mutation. Missing consent,
invalid shape, or a missing deletion callback leaves the session intact.
An error or exception during deletion closes ALL FDs; partial removal is
not reported as success. A forked copy closes its own references without
deleting entries from the parent's map.

## Why slots are not recycled

Each removal performs `BPF_MAP_DELETE_ELEM` on the TGID-to-slot HASH. New
invocations for that actor miss the lookup before reaching context or counters.
An invocation that already obtained the entry, however, can finish AFTER
deletion. In-flight events are not guaranteed to be revoked.

Retired ARRAY records are therefore NOT cleared, reassigned, or reused.
A prior event can write only to its old retired record. Readers still visit
those records to reconcile the global sequence but do not publish their
buckets or individual quality. Sticky GLOBAL quality failures still
invalidate the set; they are not cleared to hide loss. The global
writer/sequence bracket still covers the whole sweep, including retired actors.

Retained actors keep their bytes, cookies, sequence, and per-identity
regression checkpoint, even if an earlier row disappears. Removal does not
close or recreate programs, links, or maps. The producer still owns eight FDs.
No new dependency, external command, daemon, public API, or persistence.

## Evidence

- Fixture pauses an event after it obtains a slot, removes the actor during
  the invocation, and verifies that the late write affects only the retired
  record. The actor is absent from output, and later events do not access
  context or counters.
- Retained counters advance from 1,024 to 2,048; regression is still rejected
  after removal. Duplicate, changed, or denied proposals leave the session
  intact. Readmission is rejected, an empty set closes capture, failed or
  throwing first or second deletion closes everything, and fork does not
  mutate the parent.
- An actor with an individual quality failure can be removed without
  falsifying the remaining rows. The sequence includes its earlier events
  and does not invent zero.
- Native 17→16→1: ALL 17 private children keep sending UDP. Only active
  actors gain new bytes or operations, earlier TCP/UDP counter origins are
  preserved, and the count remains eight FDs. Empty/stop/restart is clean,
  and all children exit normally. This still uses one real namespace and
  does not certify 64 real actors.

Logs: `work/network-shared-retain-*`. Final Linux Release 130/130 without
skips, two focused tests repeated 20 times each (40 runs), and installed
SDK `consumer` exiting 0. MSVC ran two focused tests 20 times each; this is
NOT full Windows support or resolution of N03. The final build had no new
C++ warnings; D9025 is expected. Initial indentation warnings were fixed
without changing logic, and the first build log was kept. The first consumer
attempt failed before execution because the executable name was wrong.
The log was retained and the actual name checked; this was not a library
failure. Revision 91 mirror confirmed: comment 90140272294875.
SEGUIMIENTO.txt remains authoritative.

## Next limitation at this checkpoint

Combine removal with additions in NEW records and authenticate the full
transition through pidfd. Preserve retained captures and counter origins,
exclude the interval when readmitted actors were absent, and reset all
rates and the epoch. Do not simulate an update by stopping and restarting
retained actors. The public backend remains unchanged at this revision.
