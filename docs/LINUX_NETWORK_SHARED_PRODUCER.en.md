# Shared network producer: fixed selection and internal counters

Revision 93: the [public observed backend](LINUX_NETWORK_SHARED_PUBLIC.en.md)
now enables shared capture and updates in standalone, Monitor, Sampler,
and owner paths. Pending activation notes for revisions 89–92 below are
retained only as history.

Revision 92: [authenticated batch updates](LINUX_NETWORK_SHARED_UPDATES.en.md)
preserve retained actors and admit or readmit actors with fresh capture.
Public activation was still pending. The remaining work below for revisions
89–91 is historical.

Revision 91: [safe removal](LINUX_NETWORK_SHARED_RETENTION.en.md) supports
subsets without restarting retained actors or recycling slots. Additions,
readmission, and the complete authenticated transition were still pending.
The revision 89 description below is historical: the HASH now supports
deletion, never reassignment.

Revision 90 follow-up: [authenticated shared windows](LINUX_NETWORK_SHARED_WINDOWS.en.md)
integrate owned pidfds, joins at both endpoints, rates and UID sums, and
validated projection for a FIXED internal selection. Updates that preserve
counter origins and public backend activation were still pending. The
revision 89 state follows.

Revision 89, 2026-10-22. This internal F03 unit is complete. At this checkpoint
LinuxSharedNetworkSession is NOT yet the backend for LinuxProcessNetworkSampler,
Monitor, or Sampler. It still needs continuous pidfd joins, windows, rates,
and selection changes that preserve counter origins. F03/H04/J04 remain open.

## One explicit set, three traffic probes

Start requires consent and 1–64 distinct kernel identity tokens (TGID, real
UID, and leader start_boottime). These are declared tokens, NOT public
PID/procfs authentication or a guarantee of future liveness. The session
does not enumerate processes or admit others automatically. It uses exactly
three traffic programs and links: inet_sendmsg, inet6_sendmsg, and
sock_recvmsg. There is no program or registry per CPU and no probe group
per process.

Unselected TGIDs exit on a HASH lookup miss BEFORE reading network context
or counters. Selected actors' current start/UID are checked before bytes
are counted. The namespace comes from the socket, not the task's current
namespace. Each actor retains up to 16 observed cookies using CAS, four
TCP/UDP send/receive lanes, and PEEK/TRUNC/ERRQUEUE exclusions. Payload is
not inspected. Cookies are not evicted: a full registry, incorrect identity,
failed read, or overflow sets a sticky quality failure. The entire read is
rejected rather than returning partial data or an invented zero.

| Owned map | Layout |
|---|---|
| HASH u32 → u64 | Kernel TGID → slot+1; immutable for the session in revision 89, with capacity equal to the selection size |
| ARRAY u32 → u64 | Nine-cell global prefix and N records: UID/start plus the earlier 105-cell ARRAY per actor |

The HASH is not LRU and does not evict entries automatically. Its type and
max_entries limit are documented by the [Linux kernel](https://docs.kernel.org/bpf/map_hash.html).
Both maps are created and updated through direct BPF syscalls, with no
libbpf, SDK, external tools, or collection processes. The maximum ARRAY
holds 6,857 u64 values (54,856 bytes of values, excluding kernel overhead
and the HASH).

## Startup, consistency, and resources

Capture remains disabled until both tables are initialized and all three
probes are attached. Failure at any step releases all owned resources.
The selection is fixed in this revision: replacing it requires stop/start
and does NOT retain counters. It does not yet replace the public provider's
update_selection.

A GLOBAL writer/sequence guard brackets all actor and cookie updates in
addition to the individual guards. Index errors also use the global guard
and a sticky flag. Read sweeps the full selection and revalidates the table,
metadata, registries, and the sum of actor sequences against the global
sequence. It retries up to eight times when concurrent writes or sequence
changes are detected, never returning part of the set. The clock must be
positive and monotonic, and the bracket must not exceed one second. This
is not a hard syscall deadline. The last valid read is kept to reject
decreasing sequences, bytes, or exclusions, or disappearing or replaced
slots/cookies. Failure does not erase that reference or hide a later regression.

Start/read require the same FD table, serialized calls, and a live source
context. A forked copy rejects reads and closes only inherited references
without writing to the parent's maps. Stop and destruction close links,
programs, and maps. There is no pinning, permanent configuration, automatic
privilege acquisition, or daemon. Non-Linux reports absence with a reason;
fixtures remain runnable.

## Evidence

- tests/linux_network_shared.cpp: bytecode subset interpreter, 64 actors
  with 16 cookies each (1,024 buckets), isolation by actor/UID/start,
  IPv4/IPv6/TCP/UDP and all four lanes, no context or counter access for
  unselected actors, startup gate, invalid table, and read/identity/overflow
  failures. A 17th cookie for the last actor rejects EVERYTHING. Exactly
  three loads and links for 64 actors.
- Startup: denials and exceptions at each operation with full rollback.
  Reads: quality, metadata/table/clock/writer changes, errors, fork,
  reserved fields, decreases or regressions in bytes/cookies/sequence/registry,
  and recovery without clearing the validation reference.
- Private manual WSL run: `--authorized-private-shared-capacity17` in
  pcinsight-linux-network-capacity-native, outside CTest. The owner prepares
  17 children and their tokens are associated by pidfd/procfs join BEFORE
  start. Capture owns exactly eight FDs (two maps, three programs, three
  links) after owner and query release their resources. UDP uses 64–80 B
  payloads, twice on IPv4 and once on IPv6; TCP uses IPv4/IPv6, four times
  64–80 B per direction. The run checks four lanes, cookies, per-actor and
  global sequences, empty registries, parent TCP/UDP exclusion, fork
  rejection without affecting the parent, and fresh empty stop/restart.
  All children exit normally and FD counts return to baseline. From
  harness startup: prepared at 19 ms, start at 532 ms, measured at 548 ms,
  and stop after restart at 1,765 ms. Phases include IPC, traffic, and joins;
  this is NOT a benchmark or universal guarantee. Only ONE real namespace
  was shared. SO_NETNS_COOKIE was initialized for the oracle. The 64-actor
  and 1,024-bucket cases are fixtures, not certification of 64 real actors.
- The earlier PUBLIC provider's 17-actor native regression passes four
  rates, partial UID sums, and cleanup. It is NOT connected to the shared
  producer in this revision.
- Linux Release 129/129 without skips; three tests repeated 20 times each
  (60 focused runs); installed external consumer rebuilt and exiting 0.
  MSVC passed 3/3 focused tests with no new C++ warnings; D9025 is expected.
  The Windows revision 75 consumer failure remains open in N03.
  Logs: work/network-shared-*. Native tests and regressions are private and
  capture no other PIDs.

## Next step at this checkpoint

Integrate the producer with authenticated selections: owned pidfds, joins
before and after, quality checks, baselines, four rates, and partial UID
sums, without losing the update_selection and retained-counter-origin
contract. Next come the public provider, Monitor, Sampler, and automatic
admission/removal with explicit coverage. Do not publish these counters
as total network usage for ProcessInfo, UserSession, or apps. This does
not cover all asynchronous I/O, kernel actors, link-layer bytes, or
retransmissions, or validate all kernels, drivers, or multiple real namespaces.

No new public API/layout, GUI, persistence, dependencies, or CLI.
Official count without partial credit: 57/75 = 76.0% closed,
18/75 = 24.0% open.
