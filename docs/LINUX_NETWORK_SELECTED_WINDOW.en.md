# Traffic windows for a selected actor

Revision 71: `LinuxSelectedNetworkSampler` connects selected capture, the
[pidfd/procfs join](LINUX_NETWORK_PIDFD_JOIN.en.md), and the reducer. It is
**internal**, not installed or enabled through Monitor. No new public API,
ABI, history, GUI, or Windows work. F03 remains E.

## Ownership and transactions

`start(authorized,pidfd,pid,start_ticks,socket_cookie)` requires opt-in, valid
arguments, and complete APIs. It retains its own CLOEXEC duplicate of the
pidfd; after start the caller may close the input descriptor. It obtains a
join, discovers the producer's kernel identity with local TEST_RUN, and starts
selected capture. The epoch uses the same nonwrapping atomic serial as SELF
within the same linked copy, not a global UUID across DSOs. Errors and
exceptions roll back capture and pidfd. Stop and destruction release both
and discard time baselines. Injected contexts are borrowed and must outlive
the sampler.

Calls are serialized and must use the same FD table; threads sharing a table
are supported. Do not unshare the table or replace or close internal FDs
asynchronously. The getpid guard detects only fork, NOT initial TGID. The
child copy closes its inherited references before joining or reading capture.

Each sample validates a fresh join before and after the counter read: expected
PID/start, scope boot, correct observer, and fixed kernel identity. Each join
performs two pidfd discovery operations, four per sample, with no authenticity
cache. Full context must match within the sample. A local change between calls
discards the time baseline before sampling. A different boot or kernel identity
is rejected and requires restarting the sampler.

Counter reads require active state, selected_actor type, matching token and
cookie, coherent quality, and a positive, ordered, nonoverlapping
CLOCK_MONOTONIC bracket of at most one second. The bracket covers the counter
read, NOT all join costs or a hard syscall deadline. Only after the
post-checks does the sampler hand a row to the reducer and retain its context
and timestamp.

Failure, absence, denied permission, changes, or exceptions clear the entire
baseline. After recovery, the first valid sample has no rate and the second
can produce one. A window longer than 60 s cannot produce rates either.
This sampler does not clear sticky capture failures or overflow: resolve the
problem and restart capture.

## Semantics and coverage

`PrivateSelectedNetworkWindow` retains local context and a
`PrivateNetworkWindow` with four lanes (TCP send/receive and UDP send/receive),
quality, bracket, and accounting. Rates use floor(delta_bytes*1e9/delta_ns)
with checked integer arithmetic. Timestamps are read endpoints and rates are
Support::estimated, not exact instantaneous throughput. Sample frequency,
latency, and production performance are not guaranteed.

Typed coverage `selected_single_socket_namespace` differs from SELF and general
coverage: exactly one valid kernel row, with a producer separate from the
target actor. enumeration_complete covers only that declared bucket, NOT the
whole system or process. The operation's initial UID is separate from the
observer's procfs UID; UID totals are partial, and IDs and ticks/ns are not
converted. No new ProcessInfo or UserSession totals or app/user history
integration.

The join's limits apply: compatible BTF/FEXIT/ptrace permissions and privileges,
temporary blocking and restoration of the caller's signal mask (which can delay
handlers), and no asynchronous cancellation or hard deadline. It does not
attach with ptrace, signal or stop the target, run external capture commands,
install anything, or change permanent configuration. Endpoint checks are not
atomic against credential/namespace ABA changes and do not establish future
liveness or retain namespaces.

The cookie is still declared, not discovered automatically. There are no
multiple actors or dynamic buckets, complete path/asynchronous/offload/wire
coverage, or commercial app identity. Next for F03: coverage, buckets, and
multiple actors before publication. H04/J04 stay open. Authorized tests use
only owned, private processes, never system-wide capture.
Revision 72 adds a [fixed multi-actor collection](LINUX_NETWORK_MULTI.en.md) with
independent intervals and rollback of all baselines on failure; it does not
yet address a common window, UID aggregation, or dynamic cookies.

## Evidence

Fixtures cover retained FD 0, distinct producer/target/local domains, four
rates and UID against the oracle, idle zero, context change without crossing
the interval, restart, epoch, and fork. Eight startup failures and exceptions
exercise rollback; 15 sample failures cover join, support, context, kernel,
boot, capture, quality, clock, and exceptions. They verify baseline discard
and recovery without a rate across the gap, and intervals over 60 s.
The reducer rejects zero or two rows, a zero start, and a TGID outside the
signed range. No CTest test loads real BPF.

Manual `--authorized-private-selected-window`, NOT CTest: a private child
does not disclose its kernel token; the parent reads start, and the sampler
keeps its duplicate after the caller closes its FD. The test checks a first
sample without rates, four lanes and UID against an oracle for the real
interval, parent exclusion and idle zero, fork rejection without affecting
the parent, restart with a new epoch and baseline, repeated rejection after
exit, and cleanup without FD leaks. Earlier capture/join/detach/sticky
identity/mask checks remain. Harness watchdogs are 45 s and 35 s, not hard
BPF deadlines. Positive WSL result: local PID 786/kernel TGID 2629, UID 0,
TCP 4,096/4,096 B, UDP 256/256 B, eight peeks, and a 2.003597868 s interval.
`work/network-selected-window-native-linux.log`.

Linux Release: 123/123 without skips; 15 network tests repeated 20 times
each; installed consumer rebuilt and exiting 0; verified build with no
warnings. The first full relink logged make warnings about DrvFS timestamps
fractions of a second in the future, not compiler warnings. The log was
retained and build/suite were repeated without changing the clock or relaxing
tests; see verified-build/suite logs.
Logs: `work/network-selected-window-{final-build,suite,repeat,install,consumer}-linux.log`.
A native repeat with the final binary also exited 0: local PID 402/kernel
TGID 913, UID 0, and a 1.104113519 s interval;
`work/network-selected-window-final-native-linux.log`.
These real results are limited to this WSL x86_64 environment, not universal
hardware, CI, or distributions.
