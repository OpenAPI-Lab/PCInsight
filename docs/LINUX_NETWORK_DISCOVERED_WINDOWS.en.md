# Internal selected windows for observed cookies

Revision 82 follow-up: the [internal common multi-actor window](LINUX_NETWORK_OBSERVED_MULTI.en.md)
now aggregates records for several actors with group-wide resets and
updates, without a public API. The individual revision 81 contract and
evidence are retained below.

Revision 81 connects the [revision 80 engine](LINUX_NETWORK_DISCOVERY.en.md)
to LinuxSelectedNetworkSampler through
start_discovering(authorized,pidfd,pid,start_ticks) and sample_discovered().
The caller does not supply a cookie. This remains an INTERNAL provider for
ONE selected actor, not a public API, general selection, Monitor/Sampler
integration, multi-actor capture, or history. Public declared-cookie APIs
are unchanged. F03 stays open.

## Identity and lifecycle

Explicit consent is required before acquiring resources, along with valid
PID/start and complete APIs. The sampler duplicates and owns the pidfd, so
the caller can close the original. The initial pidfd/procfs join supplies
the kernel TGID, initial real UID, and leader start token, separate from
the local namespace PID and UID. Producer identity also comes from the
existing authenticated probe. Missing results, denials, and exceptions
follow the same rollback rules as strict mode. There is no automatic
elevation, CLI, download, new dependency, or namespace/configuration change.
Serialize calls, keep injected contexts alive longer than the object, and
do not replace its descriptor table.

Each sample validates the pidfd and context before and after sweeping the
whole map. Original kernel identity and boot must match, local context must
agree at both endpoints, and the clock bracket must be available, ordered,
nonoverlapping, and at most one second. A forked collector rejects reads
and closes its copy's inherited descriptors. An exited process yields
absence; a reused PID is not reassigned. Rows are not attributed to a local
user by numeric equality with the initial UID.

## Rates and quality

sample_discovered returns raw per-cookie counters, exclusions, sequence,
bracket, local context, and NetworkAccountingOutput. The new internal
coverage, selected_observed_namespace_set, means zero to 16 observed
cookies from ONE actor, not a socket inventory or network total. The initial
empty read is known and invents no cookie zero, process/user row, or zero rate.

The first observation of EACH cookie establishes its baseline. No rate is
inferred from capture start or an empty sample. A second clean observation
can produce four rates (TCP send/receive and UDP send/receive), including
measured zero. Rates are estimated: payload delta divided by nominal
sweep-end intervals, not wire throughput or exact per-event timing. A new
bucket does not invalidate earlier ones. The initial-UID sum remains
partial and reports known/missing buckets. An exclusion-only row can
have known zero payload; this is not absence. Exclusion counts are not
turned into payload.

An error, estimated source support, invalid bracket, context change during
the read, or exception discards ALL baselines. A context change between
calls starts a new baseline. An interval over 60 s produces no rates;
discard_window also discards rate baselines without destroying capture.
Stop closes probes, map, and pidfd and clears registry and state. Restart
requires consent and creates a new epoch and empty map.

In addition to the engine's sticky quality guarantees, the sampler
checks that sequence and cumulative counters never decrease and that
observed slots never disappear or change cookies. It keeps the last
validated registry even when discarding baselines, so a regression cannot
be disguised as the first sample after an error. Classic and observed
methods reject the wrong mode without converting results or silently
switching modes.

## Verification and limits

linux-network-selected-sampler fixtures cover consent and arguments without
calls, eight startup failures/exceptions and cleanup, empty → cookie A →
A+B, four rates and a partial UID sum, a missing rate for a new cookie,
16 cookies and exclusion-only activity, a valid empty scope, and rejection
of multiple actors or 17 rows. Twenty-one read failures cover denial,
estimated quality, drift between joins, UID/boot, global flags/writers,
clock, exceptions, decreasing sequence/slot/bytes/exclusions/registry,
and a corrupt prefix. Recovery keeps counters without crossing invalid
windows. Between-call context changes, discard, long intervals,
stop/restart/mode changes, and fork are simulated.

Manual `--authorized-private-observed-window`, outside CTest, uses two
private children and captures only one. Original pidfds close after setup.
The first read is empty, then the first cookie appears without rates.
Additional traffic from two concurrent threads of the same actor produces
TCP 12,288/12,288, UDP 768/768, and 24 cumulative exclusions. Four rates
and the UID sum are checked against an integer oracle. The other child and
parent are excluded, fork is rejected without affecting the parent,
discard leaves no rate, restart is empty with a new epoch, and an exited
process is rejected twice. No FD leaks. Parent/child watchdogs of 60 s/50 s
are not hard BPF loading deadlines.

Only ONE real namespace was tested. The child initializes SO_NETNS_COOKIE
for an independent oracle but does NOT pass a cookie to start. This does
not demonstrate uninitialized cookies, multiple real namespaces, other
kernels or drivers, complete socket-path coverage, or arbitrary services.
The 16-cookie cases are fixtures, not 16 physical namespaces.

The native `--authorized-private-selected-window` regression passed; strict
mode is unchanged. Linux Release 126/126 without skips, two tests repeated
20 times each, and the installed consumer rebuilt, linked, and exiting 0.
MSVC passed 2/2 focused shared-code tests; this does not establish a new
Windows implementation or validate the full Windows suite or general
consumer. The revision 75 consumer failure remains N03. Logs:
work/network-discovered-window-*. No public layout/API, GUI, or persistence
changes. F03/H04/J04 remain open.

Next: integrate dynamic records into the selected multi-actor collection
with a common bracket, joins, and group-wide reset before adding observed
API/Monitor/Sampler support. Do not expand capture to unauthorized processes.
