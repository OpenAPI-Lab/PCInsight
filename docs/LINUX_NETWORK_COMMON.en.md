# Common nominal window and partial UID sums

Revision 73 adds `LinuxMultiNetworkSampler::sample_common()` to the
[selected multi-actor component](LINUX_NETWORK_MULTI.en.md). It remains
internal, not installed or exposed through Monitor, ProcessInfo, UserSession,
or history. No new dependencies, GUI, Windows support, or public API/ABI
changes. At this checkpoint F03 remains E and H04/J04 stay open.

## Sampling order

First, perform fresh pidfd/procfs joins for ALL actors and verify fixed
identities, context, boot, and observer. Then read CLOCK_MONOTONIC, read
each counter map with the existing quality checks, and read the clock again.
The whole sweep must take at most one second, start at a positive timestamp,
have ordered endpoints, and not overlap the preceding sweep. Costly joins
stay OUTSIDE that bracket. Timestamps from a previous independent sample are
not reused.

Finally, repeat fresh joins for every actor and require the same contexts
as before the sweep and the same caller. ONLY then are all rows passed to
the reducer. Each capture's producer, boot, and domains must match. The
fixed selection of 1–16 and the initial deduplication rules remain in place.
The new selected_actor_set coverage also rejects zero or more than 16 rows,
invalid TGID/start, or the same TGID/start with a different UID/cookie. A
process is not counted twice.

Any absence, failure, exception, or context change discards ALL baselines,
the previous bracket, and individual baselines. The first stable sample after
recovery has no rates. Switching between independent sample and sample_common
also discards baselines, so rates never span modes. Stop, restart, and fork
retain the existing pidfd and capture ownership and cleanup rules.

## What common means

Every row uses the sweep end as a common NOMINAL timestamp. The interval is
the difference between consecutive sweep end times. Each rate is
floor(delta_bytes*1e9/interval_ns), using checked integer arithmetic and
Support::estimated. It is an aligned estimate, NOT simultaneous or atomic
acquisition of all maps. Each row was read sometime within the bracket,
whose width is exposed.

`PrivateCommonNetworkWindow` retains the full accounting result, current
bracket, and optional previous_read_bracket. Both brackets show the timing
uncertainty: elapsed time between reads of one row is bounded by
current_start minus previous_end and current_end minus previous_start,
not necessarily by the exact nominal interval. The one-second limit applies
to the sweep, not to the provider, joins, or syscalls as a hard deadline.
Sampling frequency and performance are not guaranteed, especially with
16 actors.

associations contains the NetworkActor key, procfs context, and quality in
selection order. accounting.actors uses reducer order; join BY KEY, never
by vector position. The operation's initial kernel UID remains separate
from the observer's procfs UID and namespace and from a username or session.

The reducer groups by initial UID and adds the already aligned rates for
its selected buckets. This sums the per-actor floors rather than flooring
a rate calculated from aggregate bytes. Per-lane known/missing coverage
and overflow remain visible. Without any bucket with a measured rate, the
sum is absent, not zero. Overflow retains absence and the flag rather than
wrapping or saturating. A row can lack a rate after a counter reset or an
interval longer than 60 s. Other rates do not then represent the entire
UID. Different users stay separate. The UID sum is always partial and estimated.

## Coverage and evidence

enumeration_complete describes only the EXPLICIT actor/cookie list, not
all processes, users, or sockets on the machine. Cookies are still declared,
one per process, with no discovery or dynamic/global membership. Existing
BTF/FEXIT, privilege, ptrace, temporary caller signal-mask, shared FD-table,
and serialization requirements apply. Endpoint checks remain non-atomic;
there is no complete ABA detection, future liveness, or hard deadline.
There is no new ptrace attachment, signal, target stop, permanent
configuration, package, or capture CLI.

Extended linux_network_multi and linux_network_accounting fixtures cover
one UID with two actors, two distinct UIDs, the same nominal interval, four
actor/UID oracles, brackets, mode changes and fork, 12 sample failures with
reset and recovery without rates spanning gaps (including a post-sweep
change), the one-second limit, intervals over 60 s, and UID overflow.
The reducer rejects TGID/start aliases with different UIDs/cookies and
invalid cardinality. No CTest loads real BPF. Extending cases does not
inflate the number of CTests.

Manual `--authorized-private-common-window`, NOT CTest: two private
children generate A 4,096/4,096/256/256 B and B 8,192/8,192/512/512 B.
The test revalidates keys and contexts and checks four actor oracles and
four UID sums using the common nominal interval, retention of both
brackets, idle and parent exclusion, closed input FDs, duplicate/fork/exit
rejection, and cleanup without FD leaks. The 60 s parent and 50 s child
watchdogs are not hard BPF deadlines. No UID, namespace, or configuration
changes. Prior permission covers private processes only, not global capture.
`work/network-common-native-linux.log`.

Linux Release 124/124 without skips, 16 network tests repeated 20 times
each, installed consumer rebuilt and exiting 0, and a warning-free final
build. Native selected and independent regressions also exited 0 with the
final binary. Selected: local PID 388/kernel TGID 4939, UID 0, interval
1.143744392 s, with oracle, restart, fork, and cleanup checks. Existing
CTests were extended without changing the denominator or adding credit per
test. Final results and earlier native regressions are in revision 73 of
the official tracker and `work/network-common-*` logs. The native positive
result is limited to WSL x86_64, not universal hardware, distro, or CI
validation. Next for F03: a coverage contract and a public opt-in selected
provider, without presenting these sums as total traffic.
