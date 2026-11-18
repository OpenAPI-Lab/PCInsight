# Structural capacity and actual cost of Linux capture

Revision 97: [selections larger than 64](LINUX_NETWORK_SCALE.en.md) raise
structural capacity to 1,024 processes and 16,384 buckets using RAW batches
of up to 64. The 64-process/1,024-bucket limits and fan-out costs below are
historical.

Revision 93: the [public shared observed backend](LINUX_NETWORK_SHARED_PUBLIC.en.md)
passes the native 17-actor test with N+14 FDs, rates, UID sums, and cleanup.
Monitor, Sampler, the selection owner, and selection changes also passed.
The 64-actor/1,024-bucket cases were still fixtures. Earlier cost and fan-out
failures and remaining work below are historical evidence, not the current
observed implementation. Strict and declared modes retain fan-out.

Revision 88 update: the [gated session query](LINUX_NETWORK_QUERY_SESSION.en.md)
passes the full native 17-actor test, including rates, UID sums, and cleanup;
phases took 27–29 ms in that run. The failures and next step below describe
revision 87. The 64-actor/1,024-bucket cases remain fixtures at this checkpoint,
not real coverage of 64 actors.

Revision 87, 2026-10-22. F03 remains IN PROGRESS at this checkpoint. Raising
a limit does not complete general capture or establish suitability for GUI
refresh.

## Revision 87 contract

`LinuxProcessNetworkSampler::max_processes` increases from 16 to 64.
Strict mode allows 64 processes; declared mode allows 64 PID/start/cookie
rows, not 64 cookies per process. Observed mode allows 64 distinct processes
with up to 16 cookies each, for 1,024 total buckets. The selection owner and
inventory bridge use the same limit. A selection of 65 is rejected, never
silently truncated, rotated, or filtered. The per-process registry still
rejects a 17th cookie: the process limit and namespace limit are distinct.

This is a STRUCTURAL expansion of existing fan-out: one capture per actor,
not a shared BPF producer. Previous quality and identity checks, group-wide
resets, absence of a rate for new buckets, and partial UID sums are unchanged.
Rebuild the library and consumers because a public constexpr changes. No
existing public layout, GUI, persistence, new dependency, external metric
command, or permanent configuration changes.

## Cache BTF per session, not identity

The selected sampler reuses its BTF schema for the session, including for both
independent identity queries in each join. Previously every query reread and
parsed the complete kernel BTF. Stop clears the saved schema; start obtains
a new one. Identity, permission, and liveness results are not cached. pidfd,
procfs, context, and fresh BPF queries are still checked before and after
reading counters. Older test sources without a layout-aware callback retain
their previous path.

This does NOT eliminate repeated creation, loading, and attachment of BPF
identity queries for each check. There is not yet a persistent shared query.

## Evidence and an unsuccessful run

Fixtures cover 64 valid actors, projection of 64×16=1,024 buckets, rejection
of 65 actors and a 17th cookie, UID sums and four rates over a common interval,
reordering/retention/removal/re-addition without counting traffic from the gap,
and failure of the last actor with group-wide rollback. The per-session BTF
callback is tested with endpoint identity checks, permission, exceptions,
stop/restart, and failure of the new schema.

Manual `pcinsight-linux-network-capacity-native --authorized-private-capacity17`
(outside CTest): 17 private children, 64–80 B UDP loopback traffic,
SO_NETNS_COOKIE as an independent oracle, and child/parent watchdogs of
55 s/60 s. Only authorized private actors are captured. Two attempts were
made and both logs were preserved:

- `work/network-capacity64-native-linux.log`: FAILED when a pidfd indicated
  that a process exited during the query.
- `work/network-capacity64-native2-linux.log`, after BTF reuse: prepared at
  19 ms, started at 11,212 ms, empty registry at 35,072 ms, and first sample
  at 53,607 ms. Assertions for all 17 initial counters and absent initial
  rates passed. The second sample failed when a pidfd indicated exit during
  the query as the children's bounded lifetime expired. Native rates for
  17 actors, UID sums, and the harness's normal final cleanup were NOT
  verified. Destructors close capture and FDs and clean up and reap the
  children on exception. The watchdog was not extended, and no third run
  was attempted.

Reads between phases took about 24 s and 19 s in THIS environment. This is
not a portable benchmark or proof of individual syscall costs. It shows
a real cost problem for fan-out and repeated queries. The 64 actors and
1,024 buckets passed in fixtures, NOT as certified native capture or useful
GUI performance. Only one real namespace was shared in this test.

Linux Release regression: 128/128 without skips, six focused tests repeated
20 times each, and the installed external consumer rebuilt and exiting 0.
MSVC passed 6/6 focused tests; no full Windows validation, and the revision
75 consumer failure remains in N03. Logs: `work/network-capacity64-final-*`.
Final builds had no new C++ warnings.

## Next step at this checkpoint

Reduce repeated BPF loads and attachments with a session-owned identity
query while retaining caller-thread and target filtering, isolation, current
permission checks, liveness, and both endpoints of each join. Then repeat
a bounded private test before evaluating a shared multi-actor producer and
automatic admission/removal with explicit coverage. Do not raise limits or
extend timeouts instead of addressing cost. F03/H04/J04 remain open; the
official count stays 57/75 = 76.0% closed and 18/75 = 24.0% open.
