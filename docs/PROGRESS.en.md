# PCInsight progress

This document preserves the development history. Its percentages and
checkpoints do not describe the current state of this release. See the
[current scope](SUPPORT_STATUS.en.md).

Internal reference tracker: SEGUIMIENTO.txt (not distributed here),
revision 99, audited on 2026-10-15 against the original requirements,
code and API, documentation, and test records. ClickUp is only a
secondary mirror.

Checkpoint 99: [LinuxHistoryStore](LINUX_HISTORY_STORE.en.md) completed;
H04 P->E. Separate format with value, Support, and notes. Measured zero
is distinct from absence. Cumulative merge replaces rather than adding
replays; unique periods are normalized to milliseconds. Rejects decreases,
loss of known values, corruption, exceeded bounds, and other formats.
Retention keeps whole periods; reset unlinks files, not directories.
No automatic I/O or new eBPF capture.
Linux 134/134 plus 30 focused checks; installed consumer exit 0.
A REAL CPU round trip kept network and notifications absent. Twenty
focused MSVC checks passed outside the sandbox. Inside, both stores
failed replacement with AccessDenied 5; logs were retained and Windows
code was unchanged. Initial fixtures corrected (reused corrupt file,
portable sub-millisecond chrono), assertions active, no new C++ warnings,
expected D9025, N03 open.
H04 is not closed: integrate F03 with executable attribution and
windows, validate a native positive, and persist that real network
data. C58/E11/P4/B2, 77.3% closed/22.7% open, no partial credit.
Mirror 99 confirmed, comment 90140272938633; TXT authoritative.

Checkpoint 98: [F03 acceptance and real view](LINUX_NETWORK_ACCEPTANCE.en.md),
F03 E->C. SELF fixed with COMPLETE local identity and two fresh kernel
reads, without inferred IDs or an external permission bypass. REAL
WSL /proc: 30/30 strict, zero excluded, three Monitor snapshots pairing
network data, metadata, and epoch; UDP delta 97, rates, and cleanup PASS.
Temporary permission confirmed; probes used under 10 s total and were
removed, with no permanent change. Linux 133/133 plus 30 focused checks,
MSVC 10 join checks, installed consumer exit 0. Private 17-actor TCP/UDP,
IPv4/IPv6, update, gap, and fork regression passed. EINVAL and non-live
actor failures retained. Churn still fails closed; no universal total.
N02/N03/H04/J04 not closed. C58/E10/P5/B2, 58/75=77.3% closed,
17/75=22.7% open. Next H04; read [History](HISTORY.en.md). Linux build
recheck exited 0 without warnings after the clock-skew warning; initial
logs retained. Mirror 98 confirmed, comment 90140272937903; TXT authoritative.

Checkpoint 97: [Capacity 1,024 and batching](LINUX_NETWORK_SCALE.en.md)
completed. RAW64 per batch, 16,384 structural buckets. Additions use new
batches; retained actors do not restart and gaps are not counted. Common
limit across owner, inventory, policy, controller, projection, Monitor,
and Sampler. More than 1,024 is rejected without truncation. Rebuild ALL
for the constexpr change. Fixtures cover 65/129/1,024 actors, 16,384
buckets, and failures in batches 2 and 16. Native PUBLIC test with 65
PRIVATE actors and restricted admission passed, including 87 and 152 FD
counts, rates/UID, unchanged refresh, cleanup, and 17-actor regression.
Linux 133/133 plus 70 focused, MSVC 70 focused, installed consumer exit 0.
Initial consumer assertion expected the old 1,024-bucket limit; updated to
16,384 without weakening checks, log retained. No new C++ warnings and
expected D9025. F03 remains E for real-view validation and coverage.
WSL authorization was requested but NOT received, and no global capture
occurred in this revision. H04/J04 follow. 57/75=76.0% closed,
18/75=24.0% open, no partial credit. Mirror 97 confirmed, comment 90140272937319.

Checkpoint 96: [Monitor/Sampler admission integration](LINUX_NETWORK_ADMISSION_MONITOR.en.md)
completed. SystemSnapshot keeps the last refresh, network report, epoch,
and exclusions together. Consent EVERY time, strict_all default, and
mutually exclusive modes. Failures close both reports, the worker
continues, and flag/restart does not rearm. Concurrent refresh/history,
retention/readmission without counting gaps, and partial metadata were
tested with ONLY two private children plus a synthetic marker, not global
capture. SystemSnapshot 0.x layout changed: rebuild ALL.
Linux 133/133 without skips plus 100 focused, installed consumer exit 0;
MSVC 40 focused. Initial MSVC assertion failure retained, cause not isolated;
rebuild and repetitions passed without weakening assertions. N03 consumer
issue from checkpoint 75 remains open. Temporary WSL clock skew recorded;
verification rebuild exit 0 with no warnings. F03 remains E for scaling
beyond 64, coverage, and authorized live validation. H04/J04 open.
57/75=76.0% closed, 18/75=24.0% open, no partial credit.
Mirror 96 confirmed, comment 90140272936245; remote history unchanged,
TXT authoritative.

Checkpoint 95: [Opt-in partial policy and provenance](LINUX_NETWORK_ADMISSION_POLICY.en.md)
completed. strict_all defaults EVERY time; explicit verified_only excludes
unverified rows with PID, state, and reason. It does not hide corruption
or incomplete metadata. Pure plan separates readiness from quality.
More than 64 eligible actors rejects ALL, not first 64.
The sample_with_admission envelope pairs the last refresh with network
data, not a current or global census. An unchanged set preserves epoch
and rates even if policy or reasons change. AdmissionChange 0.x layout
extended: REBUILD library and consumers. Linux 132/132 without skips plus
100 focused, installed consumer exit 0; MSVC 40 focused. Native test used
ONLY two private actors plus a synthetic marker: no invented row,
epoch/baseline and 26 FDs, strict closes ALL, cleanup passed. Monitor/Sampler,
scaling beyond 64, and authorized real validation remain pending.
F03/H04/J04 open, 57/75=76.0% closed, 18/75=24.0% open, no partial credit.
Mirror 95 confirmed, comment 90140272317289; remote history unchanged.

Checkpoint 94: [Standalone strict admission](LINUX_NETWORK_ADMISSION.en.md)
completed. Connects inventory, owner, and public provider with consent
on EVERY refresh. ALL candidates must be verified and at most 64, or
the entire set is rejected without hidden filtering or truncation.
Unchanged set retains rates and epoch without hidden update or sample;
changes, PID reuse, gaps, and provenance handled. Native test used ONLY
two private children with restricted enumeration and native
procfs/pidfd/BPF: AB->unchanged AB->B->AB, 18->16->26 FDs, fork, unknown
closes ALL, restart, empty, and cleanup. NO full /proc capture tested or
global authorization. Linux 131/131 without skips plus 80 focused,
installed consumer with new header/API exit 0. MSVC fixture passed
20 times; final build had no new C++ warnings, expected D9025.
F03/H04/J04 remain open for practical policy for large or inaccessible
views, authorized real validation, and Monitor/Sampler admission.
57/75=76.0% closed, 18/75=24.0% open, no partial credit.
Mirror 94 confirmed, comment 90140272312863; TXT authoritative.

Checkpoint 93: [PUBLIC shared observed backend](LINUX_NETWORK_SHARED_PUBLIC.en.md)
activated in standalone, Monitor, Sampler, and owner. Strict and declared
modes unchanged. Native 17 with N+14 FDs and AB->B->AB with 16->15->24
owned FDs passed rates/UID, gap, provenance, epoch, fork, concurrent worker
update and history, opt-out without rearming, and cleanup. Native PUBLIC
strict and declared paths passed. Final Linux 130/130 without skips plus
60 focused, installed consumer exit 0; MSVC 3/3 plus 60 focused passed,
not full Windows validation. No new C++ warnings, expected D9025.
Initial builds collided because the assistant used the same directory
(ranlib); logs retained and rebuild serialized. General admission and
H04/J04 integrations pending, no global capture. F03/H04/J04 open,
57/75=76.0% closed, 18/75=24.0% open, no partial credit.
Mirror 93 confirmed, comment 90140272305719; remote history unchanged.

Checkpoint 92: [Authenticated shared update](LINUX_NETWORK_SHARED_UPDATES.en.md)
completed. Retained actors do not restart; additions and readmissions
start fresh and exclude gaps. Epoch and ALL rates reset. Bounded
disjoint batches, not a constant three probes after churn. Per-batch
consistency and total scan <=1 s, no atomic cut across batches.
Native 16->15->16; fixtures cover 80 updates, 64 batches/1,024 buckets,
and failures/exceptions for each operation. Linux 130/130 plus 40
focused, installed consumer exit 0; MSVC 40 focused passed.
PUBLIC activation in Monitor/Sampler/owner and admission pending.
F03/H04/J04 open, 57/75=76.0% closed, 18/75=24.0% open, no partial credit.
Mirror 92 confirmed, comment 90140272299452; remote history unchanged.

Checkpoint 91: [Safe shared RAW removal](LINUX_NETWORK_SHARED_RETENTION.en.md)
completed without restarting retained actors or reusing slots. Native
17->16->1 keeps provenance and eight FDs and excludes removed actors
even while they send. Fixtures cover late events, 64->63, regression,
and failures 1 and 2. Linux 130/130 without skips; MSVC 2/2 repeated 20
times each. Additions, readmission, authenticated transition, and public
integration pending; F03/H04/J04 open. 57/75=76.0% closed, 18/75=24.0%
open; no partial credit or new closure. Final Linux 130/130 plus 40
focused, installed consumer exit 0; MSVC 40 focused.
Mirror 91 confirmed, comment 90140272294875; remote history unchanged.

Priority requested by the user at this checkpoint: [Linux first](LINUX_FIRST_PLAN.en.md).
Windows work is deferred at revision 62 until Linux scope is complete,
not removed from the project. Next F03, then H04/J04 and the remaining
Linux work by dependency, one subtask at a time. Revision 63 only
changes execution order: no new code, suites, closures, or percentage
change. Run Windows regression checks only when shared changes require
them, not to advance Windows features. Mirror 63 confirmed, comment
90140271872526; historical titles and statuses were not reconciled.

**Official percentage at this checkpoint: 57/75 subtasks closed = 76.0%.**
Open: 18/75 (24.0%): 11 in progress, five pending, two blocked.
Three archived [X] rows are excluded. This is not a percentage of hours
or a guarantee of coverage on all machines. Every subtask has equal
weight; partial work does not count as closed.

The audit reopened A01/A03/J01/K01 for specific problems with availability,
the time window, session state, and service group. It excluded C04/D05
as unrequested improvements and absorbed J03 into J05: safe rejection is
not detach. The change from 50.0% to 45.3% corrects tracking and does not
represent lost code.

The earlier subjective estimate of approximately 70% is retired as a
current indicator. The historical ClickUp 60.4% (32/53) does not represent
the current state either. Do not mix those denominators with the official
inventory of 75.

After each unit: update the ID, state, criterion, evidence, and blocker;
record the move and recalculate C/(C+E+P+B) in SEGUIMIENTO.txt. Update this
summary and STATUS.md with the same numbers. Do not increase the percentage
per prompt or create already-resolved tasks to inflate it. Scope changes
require a history.

Mirror 75 confirmed: list comment 90140272037855.

Mirror 76 confirmed: list comment 90140272039515.

Mirror 77 confirmed: list comment 90140272040593.

Mirror 78 confirmed: list comment 90140272041626.

Mirror 79 confirmed: list comment 90140272042530.
Mirror 80 confirmed: list comment 90140272044050.

Latest unit at this point, revision 90: F03 E->E. Completed authenticated
SHARED windows for FIXED selection. LinuxSharedSelectedNetworkSampler:
1–64 PID/start/pidfd selections with cookie 0, owned duplicates, complete
rollback, unique kernel identities, boot/observer checks, and joins before
and after start. EVERY window joins ALL actors before AND after scanning.
Each native join uses TWO fresh kernel queries with permissions, liveness,
and procfs checks. ONE gated query session serves the entire set.
Local/kernel domains stay separate; permissions are not cached.
Shared-producer consistency, bracket <=1 s, epoch, empty metadata, new
bucket without a rate, four-lane reducer, and partial UID totals. Failure
of the last AFTER join rejects ALL and resets rates. Discard resets the
baseline without removing raw regression checks.

Fixtures cover 64 actors, 1,024 buckets, two UIDs, four lanes at 20 B/s,
512 known values per UID/lane, and 10,240 B/s per UID/lane; validated
projection, old join layout, errors/exceptions at each startup step,
duplicates, context/clock/identity/fork, quality, last AFTER failure,
and reset/recovery. The private WSL 17-actor test PASSED: owner released,
31 FDs (17 pidfds + 8 capture + 6 query), three shared traffic links and
one gated identity link. Four rates and integer UID oracle for TCP/UDP
over IPv4/IPv6. Empty metadata, first bucket without rates, main-worker-main,
parent excluded, and fork leaves parent intact. An exited actor rejected
ALL twice. Stop/start with 16 actors verified a fresh epoch, empty state,
NEW counters, 30 FDs, and discard/recovery. All children exited normally
and final FDs were clean. Start 420 ms, measured 517 ms, stop 2,262 ms
include joins, IPC, and restart; not a benchmark. Only ONE real netns.
64 × 16 is fixture coverage; no claim for 64 real actors.

Linux 130/130 without skips, three tests repeated 20 times, installed
consumer exit 0. MSVC focused 3/3, new test repeated 20 times. Initial
MSVC fixture segfault came from a range-for over a destroyed temporary;
fixed by retaining the sample. Failing final-targeted-msvc log retained;
final2-targeted-msvc and repeat-msvc passed. No new C++ warnings; expected
D9025. This does NOT fix the checkpoint 75 consumer issue, resolve N03,
or establish full compatibility. No new public API/layout, GUI,
persistence, dependencies, CLI, or configuration.
docs/LINUX_NETWORK_SHARED_WINDOWS.en.md; work/network-shared-window-*.
Pending: shared updates that retain existing capture and provenance
without unsafe gaps or slots, then public backend activation,
Monitor/Sampler, and automatic admission. Verified projection does NOT
mean the public backend is active. F03/H04/J04 open; C57/E11/P5/B2,
75 active/3 X, 76.0% closed/24.0% open, 18 open. No new closure, ID,
or partial credit. Mirror 90 confirmed, comment 90140272288882; list read,
historical titles and statuses not reconciled; TXT authoritative.

Previous unit, revision 89: F03 E->E. Completed the INTERNAL
SHARED producer for fixed selection. LinuxSharedNetworkSession accepts
1–64 declared kernel tokens, not PID attestation. HASH maps TGID to slot+1;
ARRAY holds global data, initial UID, and 105 values per actor. Three traffic
programs/links cover the whole set. Unselected actors return before context
or counter processing; collection stays disabled until all attachments
are in place. No eviction or 17th cookie; identity, read, and overflow
failures do not become zero. Sticky quality rejects the entire result.
Global and per-actor writer/sequence checks, table and metadata validation,
sequence-sum checks, eight retries, and a bracket of at most 1 s. The prior
reference is retained to detect regressions.

Fixtures cover 64 × 16 = 1,024 buckets, isolation, four lanes, IPv4/IPv6,
TCP/UDP, table checks, a full last actor, overflow, rollback on failures or
exceptions at each startup step, clock, writers, fork, and regression
recovery. Only three loads and links for 64 actors in the fixture.
The private WSL 17-actor test PASSED: pidfd/procfs tokens matched before
capture and exactly eight FDs remained (two maps, three programs, three
links) after releasing the owner and query. UDP: 64–80 bytes, twice for
IPv4 and once for IPv6. TCP: IPv4/IPv6, four times 64–80 per direction.
Four lanes, cookies, and per-actor/global sequences checked. Empty baseline,
parent excluded, fork leaves parent intact, fresh restart, clean FDs, and
normal exit for all children. Start 532 ms, measured 548 ms, stop after
restart 1,765 ms. These include joins, IPC, and workload; not a benchmark.
Only ONE real netns; 64 × 16 is fixture coverage.

The earlier PUBLIC 17-actor regression also PASSED four rates/UID and
cleanup; it did NOT use the new backend. Linux 129/129 without skips, three
tests repeated 20 times, installed consumer exit 0. MSVC focused 3/3,
no new C++ warnings, expected D9025. Checkpoint 75 consumer failure remains
under N03; no full compatibility claim. No new public API/layout, GUI,
persistence, dependencies, CLI, or configuration.
docs/LINUX_NETWORK_SHARED_PRODUCER.en.md; work/network-shared-*.
Pending: connect producer to CONTINUOUS pidfd joins, windows, rates, and
updates that retain provenance, then public integration, Monitor/Sampler,
and automatic admission. No general network coverage or app/user totals.
Authorized private tests only. F03/H04/J04 open. C57/E11/P5/B2,
75 active/3 X, 76.0% closed/24.0% open, 18 open. No new closure, ID,
or partial credit. Mirror 89 confirmed, comment 90140272276062; list read,
historical titles and statuses not reconciled; TXT authoritative.

Previous unit, revision 88: F03 E->E: gated session pidfd query
completed. Loaded resources are reused; slot 5 matches the exact
caller_pid_tgid, with 0 disarmed. No cached identity or permission.
New caller/thread clears results, with TWO queries per join and joins
before and after. Permission, liveness, and procfs checks unchanged.
Failure to disarm removes link/program and rejects. Fork closes its copy
WITHOUT writing the parent's maps. Prior injected sources and standalone
route retained. Fixtures cover 40 transactions, three loads, one attach,
thread change without reload, gate 0 or different caller not reading
context, stale data, no trigger, failures/exceptions, fork, and
disarm/recovery. Per-query-link prototype FAILED the native 17-actor
test; network-query-reuse-native17-linux log retained, not counted as
passing.
Final gated design PASSED the full native 17-actor test: start 1,593 ms,
empty 1,620 ms, first 1,647 ms, measured 1,676 ms, stop 2,986 ms. Phases
of 27–29 ms include IPC and workload; not a benchmark or universal latency.
Four rates, partial UID, bytes for 17 actors, parent excluded, owner clear,
clean FDs, and normal exit for every child. Watchdogs 55/60 unchanged.
WSL worker passed with two actors, main/worker/updater, AB->B->AB, history,
epochs, gap, and update failure. Strict TCP/UDP, rates/UID, fork leaving
parent intact, restart, and standalone mask checks also passed.
Linux 128/128 without skips, six focused tests repeated 20 times,
installed consumer exit 0. MSVC focused 6/6, no new C++ warnings and
expected D9025. Checkpoint 75 consumer failure remains in N03, no full
validation. No new public API/layout, GUI, persistence, dependencies,
CLI, or permanent configuration. docs/LINUX_NETWORK_QUERY_SESSION.en.md,
work/network-query-gated-* and prototype work/network-query-reuse-*.
Only ONE real netns, 17 real actors; 64/1,024 are fixtures. Next: shared
multi-actor producer and explicit coverage/capacity before automatic
admission; authorized private actors only, no general capture.
F03/H04/J04 open. C57/E11/P5/B2, 75 active/3 X, 76.0% closed/24.0% open,
18 open. No new closure, ID, or partial credit. Mirror 88 confirmed:
comment 90140272264999. The list was read; historical titles and statuses
were not reconciled. The TXT tracker is authoritative.

Previous unit, revision 87: F03 E->E. STRUCTURAL capacity 64,
per-session BTF, and measured cost. Strict supports 64 processes,
declared mode 64 rows, and observed mode 64 × 16 = 1,024 buckets.
Owner/bridge max 64; 65 is rejected without truncation or rotation.
A 17th cookie per actor is still rejected. Fixtures cover 64/1,024,
last-actor failure resetting ALL, retention, reorder, re-addition,
and quality. BTF is reused per session; identities are NOT cached
and checks are not omitted. Permission, liveness, pidfd, procfs, and
both endpoints are checked afresh.
The native 17-actor test FAILED TWICE; it did not pass. After adjustment:
start 11.2 s, empty 35.1 s, and first sample 53.6 s from start. The first
17 UDP counters were correct, but the second sample found an exited pidfd
at the children's 55 s lifetime. No verified 17-actor rates or UID totals,
nor normal final cleanup; RAII closed capture and reaped the children.
No third attempt or longer watchdog. Native strict regression passed:
four rates/UID, identity, parent excluded, fork/restart, clean FDs.
Linux 128/128 without skips, six focused tests repeated 20 times,
installed consumer exit 0. MSVC focused 6/6, no new C++ warnings and
expected D9025. Checkpoint 75 consumer failure remains in N03; no full
compatibility claim. Public constant changed: rebuild ALL. No existing
public layout changes or new GUI, persistence, dependencies, or CLI.
docs/LINUX_NETWORK_CAPACITY64.en.md and work/network-capacity64-*.
The 64 limit is NOT a performance guarantee or a shared producer.
F03/H04/J04 remain open. Next: reduce repeated BPF identity loads/attaches
while preserving ALL checks, before shared producer and automatic admission.
C57/E11/P5/B2, 75 active/3 X, 76.0% closed/24.0% open, 18 open; no closure
or partial credit. Mirror 87 confirmed: list comment 90140272256311;
remote statuses were not reconciled.

Previous unit, revision 86: F03 E->E. PUBLIC read-only inventory from pinned procfs
through EOF, maximum 100,000, context/boot/observer checks, and a cooperative
5 s bracket. ALL PIDs found have Metrics and verified/denied/disappeared/
unavailable reasons; observer UID is retained, with no invented zero.
Global failure yields no partial result. This is not a host census.
The bridge requires ALL candidates verified and at most 16; no filtering,
truncation, or rotation. Fixtures cover UID 0 and empty, 16/17, mixed
results, three failure states, 21 source failures, and 15 altered frames.
Read-only WSL used real /proc and two children verified over IPC, then
passed an explicit PRIVATE subset to the owner. Bridge returns all or
no result, with clean FDs. Positive denied/exited cases are fixtures,
not real hidepid or exit-race results. No BPF, root, or traffic capture.
Linux 128/128 without skips, two tests repeated 20 times, installed
consumer exit 0, final build clean. One initial warning corrected with
log retained; stat notes clarified. MSVC focused 2/2 without new C++
warnings and with expected D9025; checkpoint 75 consumer failure remains
in N03, no full validation. Additive header/types/functions/test, no
existing layout changes or new GUI, persistence, dependency, CLI, or
global capture. LINUX_NETWORK_INVENTORY.en.md. F03/H04/J04 open, count
unchanged. Next: capture architecture past 16 with explicit capacity
and quality before general automatic admission. Traffic tests are
limited to authorized private actors; ask before expanding capture.
C57/E11/P5/B2, 75 active/3 X, 76.0% closed/24.0% open, 18 open; no partial credit.
Logs: work/network-inventory-*. Mirror 86 confirmed: list comment 90140272246025.

Previous unit, revision 85: F03 E->E. PUBLIC read-only PID/start owner, 0–16 complete
declared candidates, consent, and shape/capacity checks; 17 is rejected
without querying the source, truncation, or rotation. Acquire, retain,
remove, replace, and reorder without creating a new generation for
reordering. ALL candidates are checked at both endpoints and against
context using poll/fdinfo/stat. RAII, fork/move, and empty state covered.
Runtime failure cleans owned resources; NO capture, enumeration, or
automatic update. Borrows use cookie 0; downstream duplicates the FD.
Clearing or failing the owner does NOT stop independent capture; the
consumer must stop or update it. Fixtures: 16 positives, rejection at
17, 13 runtime/quality/context/exception failures, retention and changes,
and clean FDs. Read-only WSL: two children, move/fork, stale start,
exit-to-empty. Authorized owner-to-PUBLIC BPF test: two children,
original FDs closed, clearing the owner leaves capture running, four
rate/UID oracles, A TCP 12288/12288 and UDP 768/768 with peek 24, B double,
two threads per actor, gap/exit/restart, and clean FDs. Only ONE real
netns; the OS oracle initializes cookies; 16 positives are fixtures,
not general coverage. Linux 127/127 without skips, two tests repeated
20 times, installed consumer exit 0, clean final build. Three initial
indentation warnings corrected, log retained. MSVC focused 2/2, no new
C++ warnings, expected D9025; checkpoint 75 consumer failure remains
in N03, no full validation. Additive header/types/class, no existing
layout changes or new GUI, persistence, dependency, CLI, or global capture.
LINUX_NETWORK_SELECTION_OWNER.en.md. F03/H04/J04 open, count unchanged.
Next: inventory with coverage/admission and an architecture that scales
past 16 without concealing losses or capacity. Bounded private tests
only; no general capture without broader authorization.
C57/E11/P5/B2, 75 active/3 X, 76.0% closed/24.0% open, 18 open; no partial credit.
Logs: work/network-selection-*. Mirror 85 confirmed: list comment 90140272050845.

Previous unit, revision 84: F03 E->E. Observed mode integrated into Monitor/Sampler
through three-argument opt-in setup. Two-argument setup is always strict;
old callbacks are rejected. Active/running capture does not change
mode. Setup uses a stopped Sampler and the flag; live updates preserve
mode, metadata, history/epochs, and retained provenance, reset ALL
baselines, and do not count gaps for new actors. Opt-out and stop close
capture. The flag or restart does not rearm it; failed update closes
capture but leaves the worker running. Fixtures cover both modes, quality,
exceptions, old callbacks, empty metadata, defaults, flags, running state,
and consumers. Public WSL Monitor: two children, closed caller pidfds,
move, empty-to-late data, four rate/UID oracles, A TCP 12288/12288 and UDP
768/768 with peek 24, B double, two threads per actor, parent excluded,
fork, update/removal/re-addition/gap, two exited cases, stop/restart,
default, opt-out, and clean FDs. Worker: two actors, cookie 0,
empty-to-buckets, four rates/UID in history, AB->B->AB, concurrent
updater/history, epoch reset, retained provenance, new actors without
gap accounting, failed update leaves worker alive, defaults, disarmed
restart, and clean FDs. Only ONE real netns; OS oracle initializes cookies,
no general coverage. Declared Monitor regression passed. Linux 126/126
without skips, three tests repeated 20 times, installed consumer exit 0,
no warnings. MSVC focused 3/3 without new C++ warnings and expected D9025;
checkpoint 75 consumer failure remains in N03, no full validation.
No API/layout changes beyond 83 and no new GUI, persistence, dependency,
CLI, or global capture. F03/H04/J04 open, count unchanged.
LINUX_PROCESS_NETWORK_OBSERVED_MONITOR.en.md. Next: general process
selection and coverage policy with identity, removal, and limits.
Bounded private tests only; ask for authorization if capture must expand.
No new front or global capture.
C57/E11/P5/B2, 75 active/3 X, 76.0% closed/24.0% open, 18 open; no partial credit.
Logs: work/network-observed-monitor-*. Mirror 84 confirmed: list comment 90140272049411.

Previous unit, revision 83: F03 E->E. PUBLIC selected observed provider, opt-in,
strict by default, with 1–16 actors selected using cookie 0, up to 16
cookies per actor, and 256 rows. New coverage; observed_processes retains
identities even when empty, the first bucket has no rate, repeated actor
sequences are not summed, and projection is all-or-nothing. New 0.x
snapshot/SystemSnapshot layout: rebuild ALL consumers. Monitor/Sampler
still reject the mode without a provider; integration is next.
Fixtures cover empty/mixed results, multiple cookies, 23 invalid cases,
256-row projection, and consumer use of the new types. PUBLIC WSL API:
two children, closed caller pidfds, move, empty-to-late data, four
rate/UID oracle checks, A TCP 12288/12288 and UDP 768/768 with peek 24,
and B double. Two threads per actor, parent excluded, fork, update,
removal/re-addition with a gap, epochs, two exited cases, recovery after
removal, stop/restart with mode and empty state checked, and clean FDs.
Only ONE real netns; the OS oracle initializes cookies and 256 is fixture
coverage. Native strict regression passed. Linux 126/126 without skips,
three tests repeated 20 times, installed consumer exit 0, no warnings.
MSVC focused 3/3, no new C++ warnings, expected D9025; checkpoint 75
consumer failure remains in N03, so Windows is not fully validated.
No new GUI, persistence, dependency, CLI, or global capture.
F03/H04/J04 open, count unchanged. LINUX_PROCESS_NETWORK_OBSERVED.en.md.
Next: opt-in Monitor/Sampler integration, then general selection policy.
Bounded private tests only; no global capture or new front.
C57/E11/P5/B2, 75 active/3 X, 76.0% closed/24.0% open, 18 open; no partial credit.
Logs: work/network-public-observed-*. Mirror 83 confirmed: list comment 90140272047500.

Previous unit, revision 82: F03 E->E. INTERNAL observed common window for 1–16 actors,
16 cookies each, and scope of 0–256 rows. Cookie 0 selection, fixed mode,
ALL endpoint joins, 1 s bracket, fork guard, identity record even when
empty, no rate for the first cookie, four rates and partial UID. Any
failure resets ALL, no partial output, and final record commit. Updates
retain provenance, removal/re-addition starts empty without counting
the gap, with a new epoch and group reset. Runtime failure closes all.
Fixtures cover 32 buckets, two UIDs, 256 reducer rows, limits, 20 read
failures, five start failures, and failed addition. Two WSL children
with original pidfds closed and no input cookies: empty/late data, four
rate/UID oracles, A TCP 12288/12288, UDP 768/768, peek 24, and B double;
two threads per actor, parent excluded, fork/removal/re-addition/gap,
exit rejects the group twice, removal restores operation, and clean FDs.
Only ONE shared real netns; OS oracle initializes cookies, 32–256 counts
are fixtures, not 16 real actors. Strict common and individual observed
regressions passed. Linux 126/126 without skips, three tests repeated 20
times, installed consumer exit 0, clean build. MSVC focused 3/3 without
new C++ warnings and expected D9025. Checkpoint 75 consumer failure
remains in N03; no full validation. No new public API/layout, GUI,
persistence, dependency, CLI, or global capture. F03/H04/J04 open,
count unchanged. LINUX_NETWORK_OBSERVED_MULTI.en.md. Next: selected
public observed provider with identities, empty records, quality,
brackets, and explicit limits, then Monitor/Sampler and general policy.
Bounded private tests only; no new front or global capture.
C57/E11/P5/B2, 75 active/3 X, 76.0% closed/24.0% open, 18 open; no partial credit.
Logs: work/network-observed-multi-*. Mirror 82 confirmed: list comment 90140272046531.

Previous unit, revision 81: F03 E->E. INTERNAL selected observed windows with owned
pidfd, endpoint joins, boot/context, fork guard, and 1 s bracket. No input
cookie; known empty, first bucket without rates, four rates, partial UID,
and A->A+B. Errors, context changes, and discard reset baselines without
hiding record/counter regression. Stop/restart empties the map, starts a
new epoch, and keeps modes separate. Fixtures cover 16 cookies, exclusions
only, scope 0/17/multiple actors, eight start failures, 21 read failures,
and recovery. One WSL child with original pidfd closed: TCP 12288/12288,
UDP 768/768, peek 24, two threads, four rate/UID oracle checks, others
excluded, fork leaves parent intact, discard/restart, two exited cases,
and clean FDs. Only ONE real netns; OS oracle initializes the cookie,
16-cookie coverage is synthetic. Native strict regression passed.
Linux 126/126 without skips, two tests repeated 20 times, installed
consumer exit 0, and clean build. MSVC focused 2/2 with no new C++ warnings
and expected D9025; checkpoint 75 consumer failure remains in N03, no
full validation. No new public API/layout, GUI, persistence, dependency,
CLI, or global capture. F03/H04/J04 open, C57/E11/P5/B2 unchanged.
LINUX_NETWORK_DISCOVERED_WINDOWS.en.md. Next: multiple actors with common
bracket, joins, and group reset before observed API/Monitor/Sampler.
Bounded private tests only; no new front.
C57/E11/P5/B2, 75 active/3 X, 76.0% closed/24.0% open, 18 open; no partial credit.
Logs: work/network-discovered-window-*. Mirror 81 confirmed: list comment 90140272044981.

Previous unit, revision 80: F03 E->E. PRIVATE observed engine without an input
cookie, using the actor's kernel token. ARRAY 105, 16 slots, CMPXCHG64,
no eviction, global sticky quality, whole read with eight retries,
validator, and empty/detach/stop/restart. Fixtures cover 16 cookies,
four lanes, same/different peer, capacity, quality, and rollback.
One WSL child, no producer-supplied cookie; OS oracle: TCP 12288/12288,
UDP 768/768, peek 24, two threads, others excluded. Detach/restart starts
empty, then TCP 8192/8192, UDP 512/512, peek 16, clean FDs. Only ONE real
netns; 16 cookies, different-peer race, and capacity are fixture cases;
the oracle initializes the cookie. Declared Monitor regression passed.
Linux 126/126 without skips, two tests repeated 20 times, installed
consumer exit 0, clean build. MSVC focused 2/2 with no new C++ warnings
and expected assertion D9025. Checkpoint 75 consumer failure remains in
N03; no full validation. Fixture assert macro corrected; log retained.
No new public API/layout. LINUX_NETWORK_DISCOVERY.en.md. F03 remains E
for joins, context, windows and rates, multiple actors, public provider,
and general policy. H04/J04 open, count unchanged. Next: selected-level
integration with join/reset/windows, bounded private tests only, no global
capture.
C57/E11/P5/B2, 75 active/3 X, 76.0% closed/24.0% open, 18 open; no partial credit.
The provider also does not use setns or unshare. A 17th cookie, invalid zero, failures, and overflow affect global sticky quality. The writer and sequence are tracked. Logs: work/network-discovery-*.

Previous unit, revision 79: F03 E->E. Declared buckets integrated into
Monitor/Sampler through three-argument setup with mode and consent.
Two-argument setup is always strict after stop; setting the flag or
restarting does not authorize, and active capture does not change mode.
Stopped Sampler with flag/update preserves the mode. Fixtures and WSL
Monitor/worker validated four bucket/UID oracles, removal/re-addition,
gaps, history, concurrent update, baselines, strict default, and cleanup.
Only ONE real netns; second cookie NOT observed. No two concurrent real
namespaces or discovery. Linux 126/126 without skips, three tests
repeated 20 times, installed consumer exit 0, clean build. MSVC focused
3/3 without new C++ warnings and expected assertion D9025. Consumer
failure from checkpoint 75 remains in N03; no full validation.
Additive API, no new layouts or options. F03 stays E for policy/discovery,
dynamic buckets, and general coverage; H04/J04 open, count unchanged.
LINUX_PROCESS_NETWORK_BUCKET_MONITOR.en.md. Next: dynamic buckets and
general selection, with bounded private tests only, no global capture
or new front.
C57/E11/P5/B2, 75 active/3 X, 76.0% closed/24.0% open, 18 open; no partial credit.
Fixtures also cover callbacks, enums, failures, opt-out, and defaults. No new GUI, persistence, dependency, CLI, or global capture. Logs: work/network-bucket-monitor-*.

Previous unit, revision 78: F03 E->E. Public provider with declared namespace
buckets, opt-in constructor, up to 16 process-cookie rows, and multiple
rows per process. Strict default unchanged. Exclusions before the map,
sticky quality, coherent deduplication/context/UID, new coverage, and
partial sum without duplicates. Updates change epoch and reset ALL
baselines; retention, removal/re-addition, and gaps handled. Fixtures:
two cookies, four lanes, positive and negative cases. One WSL child
declared two cookies, but only ONE real netns and no observation of the
other cookie: four oracles, other bucket zero, no duplicate sum, gap,
fork, and clean FDs. NOT two concurrent real namespaces. Strict
two-child regression passed. Linux 126/126 without skips, five tests
repeated 20 times, installed consumer exit 0, and clean build. MSVC
focused 5/5 with no new C++ warnings and expected assertion D9025.
Consumer failure from checkpoint 75 remains in N03; no full validation.
Fixture overflow protocol error corrected; log retained. Additive API
and enum extension, no new layouts. LINUX_PROCESS_NETWORK_BUCKETS.en.md.
F03 stays E for Monitor/Sampler bucket mode, policy/discovery, dynamic
buckets, and general coverage. H04/J04 open, count unchanged. Next:
opt-in bucket integration, with bounded private tests only.
C57/E11/P5/B2, 75 active/3 X, 76.0% closed/24.0% open, 18 open; no partial credit.
Rows are keyed by PID/start/cookie; foreign nonzero cookies are excluded before the map. No new GUI, persistence, dependency, CLI, setns/unshare, or global capture. Logs: work/network-buckets-*.

Previous unit, revision 77: F03 E->E. Sampler update with an active worker and
existing capture, fresh consent, serialized collection/commit/update/stop,
and priority for pending updates with protected wakeup. History, epochs,
and other metrics are preserved. Update does not rearm; failure closes
capture, not the worker. Two private WSL children AB->B->AB with worker
and updater on separate threads; four actor/UID oracles and positive
gap/baseline/provenance/failure-restart/cleanup checks. The first native
run failed on removal; a possible starvation issue was corrected, repeat
and final runs passed, and the log was retained. Linux 126/126 without
skips, three tests repeated 20 times, installed consumer exit 0, clean
build. MSVC focused 3/3 with no new C++ warnings. Consumer failure from
checkpoint 75 remains in N03; no full validation. Additive API, no new
layouts. LINUX_PROCESS_NETWORK_WORKER.en.md. F03 stays E for policy,
discovery, buckets, and general coverage; H04/J04 open; count unchanged.
Next: bounded buckets and general selection, without global capture or
another front.
C57/E11/P5/B2, 75 active/3 X, 76.0% closed/24.0% open, 18 open; no partial credit.
latest can remain historical until the next sample; do not connect epochs. CTest covered start/stop, 40 updates, and reads. No new GUI, persisted history, dependency, or global capture. Logs: work/network-worker-*.

Previous unit, revision 76: F03 E->E. Explicit update in the public provider and
Monitor. AB->B->AB and reordering keep matching captures, retire old ones
before creating new ones, require fresh consent and epoch, and reset ALL
baselines. Runtime failure closes ALL captures; in Monitor this also
applies to an invalid authorized list. Retained counters keep provenance,
and new counters do not include earlier traffic. Two WSL children, four
oracles, gap/fork/exit and recovery, and no FD leak. Linux 126/126 without
skips, two tests repeated 20 times, installed consumer exit 0, and clean
build. Additive API with no new layouts. LINUX_PROCESS_NETWORK_UPDATE.en.md.
F03 remains E for worker updates, policy/discovery, buckets, and general
coverage. Count unchanged, H04/J04 open. Next: safe worker update, not
another front or global capture. The checkpoint 75 MSVC consumer failure
remains under N03; no full validation.
C57/E11/P5/B2, 75 active/3 X, 76.0% closed/24.0% open, 18 open; no partial credit.
The initial fixture order assertion was corrected and its log preserved. MSVC focused 2/2 passed with assertions active, expected D9025, and no new C++ warnings. No global capture, GUI, history, or dependency was added.

Previous unit, revision 75: F03 E->E. [Selected Monitor/Sampler
integration](LINUX_PROCESS_NETWORK_MONITOR.en.md) completed on Linux.
Separate report, flag false by default, and separately authorized setup.
Opt-out closes capture, as do worker stop, completion, and error. Restart
or setting the flag alone does not reauthorize. Lifecycle fixtures and
legacy sentinels passed, as did two private WSL children with four oracles,
closed caller FDs, move/fork, worker, fresh epoch/baseline on rearm, and
no FD leaks. Linux 126/126 without skips, 18 network tests repeated 20
times, installed consumer exit 0, and verified clean build.
0.x LAYOUT CHANGE to SystemSnapshot/SampleOptions: rebuild ALL consumers.
Metric header extracted without semantic change; BTF shadowing fixed.
DrvFS warnings preserved; rebuild clean. MSVC focused 2/2 passed; general
consumer reproduced 0xC0000409 after new checks. No proven cause, N03
pending, NO full compatibility claim. Logs: work/network-monitor-*.
No new Windows capture, GUI, persistence, or dependency. F03 remains E
for dynamic selection, namespace buckets, and general coverage; H04/J04
open. Next: fixed manual scope and namespaces, private tests, no global
capture. C57/E11/P5/B2, 75 active/3 X, 76.0% closed/24.0% open, 18 open;
no partial credit.

Previous unit, revision 74: F03 E->E. [Selected public
provider](LINUX_PROCESS_NETWORK.en.md) completed. LinuxProcessNetworkSampler
has a movable Pimpl, opt-in start, sample and stop, an installed header,
typed coverage and estimated support, local/kernel identities by key,
counters, rates, partial initial UID, availability, brackets, and
boot/epoch/observer metadata. Pure projection, 34 negative cases, zero
baseline and UID 0, overflow, and differing ID orders. PUBLIC WSL API:
two authorized children, four actor/UID oracles over 2.307999437 s, caller
FDs closed, move while active, idle zero, parent excluded, fork/exit, and
no FD leaks. Linux 125/125 without skips, 17 network tests repeated 20
times, installed consumer built with the new header and ran with exit 0,
clean build. Fixture field name fixed; log retained. ADDITIVE API,
existing layouts and ABI unchanged; no automatic Monitor integration or
legacy totals. No new Windows work, GUI, history, or dependency.
Logs: work/network-public-*. F03 remains E for opt-in Monitor integration
and selection/coverage/bucket policy; H04/J04 open. Next: selected report
in the general flow, private tests, no global capture. C57/E11/P5/B2,
75 active/3 X, 76.0% closed/24.0% open, 18 open. Mirror 74 confirmed:
comment 90140271926244; remote titles and statuses are historical.

Previous unit, revision 73: F03 E->E. [Common nominal window and UID
totals](LINUX_NETWORK_COMMON.en.md) completed. All joins checked before and
after, ordered non-overlapping counter scan <=1 s, then commit.
selected_actor_set supports 1–16 actors deduplicated by TGID/start.
Common nominal end, estimated rates, partial initial UID with known,
missing, and overflow states, current and prior brackets, and associations
by key. Failure, exception, identity change, or mode change discards ALL
baselines. Fixtures: same or two UIDs, four oracles, 12 failures, recovery,
1 s and >60 s, overflow, modes, and fork. Two authorized private WSL
children: A 4096/4096/256/256 B and B 8192/8192/512/512 B, four actor/UID
oracles, brackets, idle zero, parent excluded, caller FDs closed, fork,
exit, and cleanup. Selected and independent native regressions exit 0.
Linux 124/124 without skips, 16 network tests repeated 20 times, installed
consumer exit 0, clean build. Existing CTests extended, no new count.
Logs: work/network-common-*. No new Windows, Monitor, API, ABI, history,
or dependency. No atomic or global snapshot; declared cookies and inherited
limits. F03 remains E for coverage contract, buckets, and public provider;
H04/J04 open. Next: coverage and selected opt-in provider, private tests,
no global capture. C57/E11/P5/B2, 75 active/3 X, 76.0% closed/24.0% open,
18 open. Mirror 73 confirmed: comment 90140271923048; remote titles and
statuses are historical.

Previous unit, revision 72: F03 E->E. [Selected multi-actor
collection](LINUX_NETWORK_MULTI.en.md) completed. Fixed list of 1–16,
one cookie per process, local/kernel deduplication, common boot/observer
context, and rollback of ALL baselines on any failure, exception, or change.
First recovery sample has no rates. Independent intervals, NOT a simultaneous
snapshot or common UID sum. Fixtures: two actors with same UID/cookie,
2 s and 3 s intervals, five startup failures, four second-row failures,
reset and fork. Two authorized private WSL children: A 4096/4096/256/256 B,
B 8192/8192/512/512 B, four oracles per actor, cross-traffic and parent
excluded, idle zero, caller FDs closed, duplicate/fork, exit, and clean
FDs. Linux 124/124 without skips, 16 network tests repeated 20 times,
installed consumer exit 0, clean final build. Initial indentation warning
fixed and log retained. Logs: work/network-multi-*. No new Windows,
Monitor, API, ABI, history, or dependency. F03 remains E for common window,
UID aggregation, coverage, buckets, and integration; H04/J04 open. Next:
bounded common window and UID aggregation, private tests, no global capture
or automatic cookies. C57/E11/P5/B2, 75 active/3 X, 76.0% closed/24.0%
open, 18 open, no new IDs, closure, or partial credit. Mirror 72 confirmed:
comment 90140271920104; remote titles and statuses are historical.

Previous unit, revision 71: F03 E->E. Revalidated [selected
windows](LINUX_NETWORK_SELECTED_WINDOW.en.md) completed. Owned duplicate
pidfd, fresh joins before/after, kernel/boot/observer/context validation,
and reducer commit only on success. Failure, exception, or change clears
the baseline, so recovery does not calculate a rate across a gap.
One-actor/cookie scope separate from SELF/global, separate actor producer,
shared epoch, four estimated rates, and partial UID. Fixtures: FD 0,
eight startup failures, 15 sample failures, reset, context, interval,
and fork. Authorized private WSL PID 402/kernel 913, UID 0: four rates
and UID oracle over 1.104113519 s, caller FD closed, parent excluded,
idle zero, fork/restart/exit, and no FD leak. Linux 123/123 without skips,
15 network tests repeated 20 times, installed consumer exit 0. Initial
make relink warnings for DrvFS timestamps preserved; verified build clean
and suite repeated without changing the clock. Logs: work/network-selected-window-*.
No new Windows, Monitor, API, ABI, history, or dependency. F03 remains E
for multiple actors, buckets, coverage, and integration. Cookie still
declared; endpoint checks are non-atomic and do not guarantee ABA
protection, future lifetime, or a hard deadline. H04/J04 open. Next:
coverage, buckets, and multiple actors, private tests, no expanded global
capture. C57/E11/P5/B2, 75 active/3 X, 76.0% closed/24.0% open, 18 open,
no new IDs, closure, or partial credit. Mirror 71 confirmed: comment
90140271917419; remote titles and statuses are historical.

Previous unit, revision 70: F03 E->E. Selected [pidfd/procfs
association](LINUX_NETWORK_PIDFD_JOIN.en.md) completed. Duplicate and poll
the pidfd, check boot/observer/target at both endpoints, require two
matching kernel tokens, project Pid through caller-thread fdinfo, and
revalidate PID/start_ticks/status/namespaces. PID/UID/tick domains remain
separate; procfs UID is in the observer's view. Fixtures cover changes and
missing data. Authorized private WSL PID 2513/kernel 6353, UID 0: parent
reads start, repeated join, stale PID/pipe/exited targets rejected,
four traffic lanes, parent excluded, detach, negative cases, mask, and
no FD leaks. Caller FD table distinct from the leader's detected, fixed,
and tested with a private CLONE_FILES worker, without new privileges or
namespaces. Linux 122/122 without skips, 14 network tests repeated 20
times each, installed consumer exit 0, and clean build. Logs: work/network-join-*.
No new Windows, Monitor, API, ABI, history, or dependency. No atomicity,
ABA protection, future lifetime, automatic external cookie, or global
coverage. F03 remains E for revalidated windows, multiple actors, buckets,
coverage, and integration; H04/J04 open. Next: selected windows/rates
with revalidation and baseline reset. C57/E11/P5/B2, 75 active/3 X,
76.0% closed/24.0% open, 18 open; no new IDs, closure, or partial credit.
Mirror 70 confirmed: comment 90140271913864; remote titles and statuses
not reconciled.

Previous unit, revision 69: F03 E->E. [Identity from
pidfd](LINUX_NETWORK_PIDFD.en.md) completed and tested. Internal opt-in
query, duplicate and liveness checks, FD -1/EBADF lookup, and FEXIT
permission probe filtered to the original caller thread, mode 0x12,
return 0. BTF validated and exactly one result required. No ptrace attach,
signals, target stop, or system-wide traversal. Temporary caller mask is
restored; handlers may be delayed, with no hard deadline. Authorized
private WSL child without a reported kernel token: PID 554/kernel 1640,
UID 0, two queries, invalid handle and exited targets rejected,
TCP 4096/4096, UDP 256/256, eight peeks, parent excluded, detach,
sticky rejection of wrong start/UID, and no FD leaks. Linux 120/120
without skips, 12 network tests repeated 20 times, installed consumer
exit 0, clean final build. Logs: work/network-pidfd-*. Missing
__pidfd_fget candidate discarded; pair warning fixed and new target
reconfigured. No new Windows, Monitor, API, ABI, public history, or
dependency. F03 remains E for pidfd/procfs join and revalidation, context,
multiple actors, buckets, coverage, and integration; H04/J04 open.
Next: selected join with private tests and bounded permissions.
C57/E11/P5/B2, 76.0% closed/24.0% open, 18 open; no new IDs, closure,
or partial credit. Mirror 69 confirmed: comment 90140271911142; remote
titles and statuses not reconciled.

Previous unit, revision 68: F03 E->E. [Selected private
actor](LINUX_NETWORK_SELECTED.en.md) captured with a declared kernel
token and cookie, using a type separate from SELF. Validation, eight
failure/exception points, rollback, FD 0, and restart tested. This does
not authenticate external identity. Authorized cooperative WSL child:
PID 2075/kernel 2600, UID 0, TCP 4096/4096 B, UDP 256/256 B, eight peeks,
parent excluded, detach, sticky rejection of wrong start/UID, pidfd exit,
and no FD leaks. Native joined SELF also passed. Linux 119/119 without
skips, 11 network tests repeated 20 times, installed consumer exit 0,
clean build. Logs: work/network-selected-*. No new Windows, Monitor,
API, ABI, public history, or dependency. F03 remains E for trustworthy
external identity, multiple actors, buckets, coverage, and integration;
H04/J04 open. Next: reliable actor discovery/revalidation without extending
private permission to global capture. GUI/CMake preference retained in
LINUX_FIRST_PLAN.en.md. C57/E11/P5/B2, 76.0% closed/24.0% open, 18 open,
no new IDs, closure, or credit. Mirror 68 confirmed: comment
90140271905836; remote titles and statuses not reconciled.

Previous unit, revision 67: F03 E->E. [Direct SELF kernel/procfs
association](LINUX_NETWORK_SELF_JOIN.en.md) completed. Local token with
PID/start_ticks/UID/procfs/namespaces, bounded reader with caller/leader
checks, two fresh BPF identities with neither left attached, and stable
boot/context. No tick/ns conversion or assumed equality across ID domains.
Missing data, changes, permission failures, exceptions, context changes
between samples, or raw/joined mode changes discard the baseline.
Linux 119/119 without skips, 11 network tests repeated 20 times, installed
consumer exit 0. Native procfs UID 1000/PID 950 stable across 20 checks,
no FD leak. Authorized WSL PID 390 versus kernel TGID 4965, UID 0/0,
TCP 4096/4096 B, UDP 256/256 B, eight peeks, four-rate/UID oracle over
793949 ns, idle zero, fork/restart, and no FD leak. Logs: work/network-self-join-*.
Indentation warning fixed, temporary WSL timestamp warnings, clean final
build. No new Windows, API, ABI, public history, Monitor, or dependency.
SELF and bracketed checks only, not atomic/general identity, apps, or
external actors. F03 remains E for multiple actors, buckets, external
identity, coverage, and integration; H04/J04 open. Next: external actor
capture and identity with private tests and bounded permissions.
C57/E11/P5/B2, 76.0% closed/24.0% open, 18 open; no new IDs, closure,
or partial credit. Mirror 67 confirmed: comment 90140271891145; remote
titles and statuses not reconciled.

Previous unit, revision 66: F03 E->E. [SELF capture connected to windows
and rates](LINUX_NETWORK_WINDOW.en.md). Sampler owns capture, scope,
epoch, and reducer. Typed kernel_initial domains, one-row scope with
declared cookie, bounded procfs boot UUID, and monotonic bracket <=1 s.
First read has no rates; intervals <=60 s, estimated rates from read
endpoints, partial UID, baseline discarded on absence, guarded restart
and fork. Linux Release 117/117, nine network tests repeated 20 times,
installed consumer rebuilt and exit 0. Authorized native WSL:
TCP 4096/4096 B, UDP 256/256 B, eight peeks, four-rate/UID oracle over
646362 ns, idle zero, rejected fork, fresh epoch on restart, and no FD
leak. Logs: work/network-window-*. Initial build clock skew 0.27 s,
verified clean afterward. No new Windows, Monitor, API, ABI, public
history, or dependencies. No global total or procfs join. F03 remains E
for multiple actors, buckets, join, coverage, and public integration;
H04/J04 open. Next: procfs identity for the producer. C57/E11/P5/B2,
76.0% closed/24.0% open, 18 open; no new IDs, closure, or partial credit.
Mirror 66 confirmed: comment 90140271884970; remote titles and statuses
not reconciled.

Previous unit, revision 65: F03 E->E. [Socket namespace
checks](LINUX_NETWORK_NAMESPACE.en.md) completed. Optional BTF
skc_net.net->net.net_cookie, declared SELF cookie, sticky rejection
of mismatches, nulls, and failed reads; positive peek cases as well.
Linux Release 116/116, eight network tests repeated 20 times each,
installed consumer rebuilt and exit 0. Authorized native WSL:
cookie 1 corroborated by SO_NETNS_COOKIE, TCP 6144/6144 B,
UDP 384/384 B, 12 peeks, child excluded, threads, detach/restart without
FD leaks; different cookie persistently rejected. Prior mode also passed.
Logs: work/network-netns-*. No setns, unshare, new configuration or
dependency, or new Windows/API/ABI/public history/Monitor. No real
multi-namespace or general capture test. F03 remains E for buckets,
multiple actors, joins, coverage, windows, and integration; H04/J04 open.
Next: F03 producer identity and scope. C57/E11/P5/B2, 76.0% closed/24.0%
open, 18 open; no new IDs, closure, or partial credit. Mirror 65 confirmed:
comment 90140271880147; remote titles and statuses not reconciled.

Previous unit, revision 64: F03 E->E. [Linux SELF session
lifecycle](LINUX_NETWORK_SESSION.en.md) completed. Authorized start,
exact identity, private map 9, loading before attach, rollback at 14
failure/exception points, FD 0, bounded coherent read, detach/stop/
destructor/restart, and rejection of estimates. Linux Release 116/116,
fixture repeated 20 times, installed consumer rebuilt and exit 0.
Authorized native WSL passed TCP 6144/6144 B, UDP 384/384 B, 12 peeks
excluded, child excluded, threads, and detach/restart without FD leaks;
no flag 77. Initial build clock skew warning of 0.0062 s; later build
clean. Wrapper handling of 77 fixed through quoting; direct call confirms
77. Logs: work/network-session-*. No Windows work or new API, ABI, public
history, or Monitor work. F03 remains E for netns, join, multiple actors,
coverage, windows, and integration; H04/J04 open. Continue F03 next.
No new IDs, closure, or partial credit: C57/E11/P5/B2, 76.0% closed/24.0%
open, 18 open. Mirror 64 confirmed: comment 90140271876915; remote titles
and statuses not reconciled.

Previous unit, revision 62: M02 E->E. [IRP correlation](WINDOWS_DISK_CORRELATION.en.md)
completed. TDH version 3 initiation/completion decoder with 32- and 64-bit
IRPs preserves the issuing identity from initiation after PID/TID exit
or reuse. Tests cover collisions, duplicates, orphan completions, epochs,
ordering, coverage, TTL, limits, 10,000 pairs, and a simulated pipeline.
Release: Windows 136/136 on both toolchains and Linux 115/115. Decoder
and correlator repeated 20 times per environment; three installed
consumers rebuilt and exited 0. Logs: work/disk-correlation-*.
Initial build failure from the SDK far macro was fixed in the fixture;
MSVC D9025 expected. No ETW, UAC, new native test, Monitor integration,
public API/ABI, persistence, or dependency. Native lifecycle/I/O
initiation producer, reconciliation, draining, loss handling, and general
disk/network attribution remain pending. Coverage is a precondition,
not certified. C57/E11/P5/B2, 76.0% closed/24.0% open, 18 open.
Mirror 62 confirmed: comment 90140271870463; remote titles and statuses
not reconciled.

Previous unit, revision 61: M02 E->E. [Temporal PID/TID index](WINDOWS_TRACE_IDENTITY.en.md)
completed. Tracks epochs, generations, and lifetimes on the QPC clock.
Looks up actual initiation or an interval, not completion time or current
IDs as substitutes. Complete coverage is a caller precondition, not
certified. Validated replacement leaves no stale results. Tests cover
limits, overlap, duplicates, containment, 100,000 records, and a 20,000-query
oracle. Release: Windows 135/135 on both toolchains and Linux 114/114,
20 repetitions per environment, three installed consumers rebuilt and
exited 0. Logs: work/trace-identity-*. No ETW, UAC, retry of canceled
MSVC elevation, or new public API/ABI, persistence, or dependencies.
M02 still needs the native lifecycle/I/O initiation producer, IRP
correlation, draining, loss handling, and general disk/network attribution.
C57/E11/P5/B2, 76.0% closed/24.0% open, 18 open; no guarantee of complete
capture. Mirror 61 confirmed: comment 90140271604191; remote titles and
statuses not reconciled.

Previous unit, revision 60: M02 E->E. [ETW decoder and private
test](WINDOWS_DISK_EVENTS.en.md) completed. TDH reads four-byte version 3
properties by name, not by header PID or offsets. uint64 totals for the
probe's own live thread within a QPC window, with valid zero, overflow
checks, and a 10,000-event oracle. Native MinGW PASS: 1048576 B read and
1142784 B written, 16 reads and 38 writes, zero reported errors and losses.
File and session removed; no ETL. The first schemaError 13 was retained;
exact cause undiagnosed. Native MSVC test did not run after elevation
was canceled; no retry and no positive result counted. Release: Windows
134/134 on both toolchains and Linux 113/113. Decoder/lifecycle repeated
20 times per Windows toolchain; three installed consumers rebuilt and
exited 0. Native Windows tdh included in the package; no new public
API/ABI or persistence. General M02 disk/network attribution, temporal
identity, draining, and loss handling remain open. No complete capture
guarantee. C57/E11/P5/B2, 76.0% closed/24.0% open, 18 open.
Mirror 60 confirmed: comment 90140271603279; remote titles and statuses
not reconciled.

Previous unit, revision 59: M02 E->E. [ETW lifecycle](WINDOWS_DISK_TRACE_LIFECYCLE.en.md)
completed. The user's “prosigue” after the revision 58 request was
treated as authorization. Owned session, no ETL, all events discarded,
no Monitor integration or default capture. Tests cover collisions,
errors, buffers, flags, idempotence, and concurrent shutdown. Elevated
MinGW/MSVC normal and watchdog shutdown passed: STOP 0, QUERY 4201,
removal confirmed, 64 KiB buffers with four reserved and max 64.
Release: Windows 133/133 on both toolchains and Linux 112/112.
Fixtures repeated 20 times per Windows toolchain; three installed
consumers rebuilt and exited 0. Logs: work/etw-lifecycle-*.
ANSI/wide, hybrid flag error 13, and UPDATE error 87 fixed with logs
retained and removal confirmed. No new API/ABI or persistence.
Event arrival, attribution, and per-PID disk/network not yet validated.
M02 remains open. C57/E11/P5/B2, 76.0% closed/24.0% open.
Mirror 59 confirmed: comment 90140271601441; remote titles and statuses
not reconciled.

Previous checkpoint, revision 58: M02 E->E. [Disk ETW decision](WINDOWS_PROCESS_DISK_DECISION.en.md).
Temporary authorization pending: 15 s, 64 MiB, no ETL. May receive
system events and require elevation, retaining only events from the
private test process. No ETW implementation or execution, new tests,
or closures; revision 57 results stand. Mirror 58 confirmed: comment
90140271600221; remote titles and statuses not reconciled.

Previous implementation, revision 57 (2026-10-21): M02 E->E. General
Windows I/O completed. [Contract and evidence](WINDOWS_PROCESS_IO.en.md):
six native uint64 counters and estimated rates by identity and dedicated
window in ProcessInfo.windows_io. They are not disk- or network-specific
and do not fill those fields. Tested errors, reuse, reset, limits, zero,
and 64-bit values. Release: Windows 132/132 on MinGW/MSVC and Linux
112/112, 20 repetitions per Windows compiler. Positive private 1024 B
pipe test and Monitor baseline/rates/off-on checks. Three installed
consumers rebuilt and exited 0. Public ProcessInfo 0.x layout changed:
rebuild the library and ALL consumers. No new vtable or persistence.
Logs: work/process-io-verified-* and install/consumer logs. C4312 in
the volume fixture fixed; MSVC test D9025 expected. M02 remains open
for specific disk/network attribution and coverage. M04 global
memory/hardware pending; M05 deferred. C57/E11/P5/B2, 76.0% closed/
24.0% open, 18 open. Mirror 57 confirmed: comment 90140271599726;
remote titles and statuses not reconciled.

Previous unit, revision 56 (2026-10-21): M04 E->E. Explicit experimental
AMD query completed. User decision on October 21; [contract and evidence](WINDOWS_AMD_RUNTIME_MEMORY.en.md).
Separate call and types, two estimated runtime quantities and OpenCL
driver version. Not global memory; no Monitor/Sampler, default call,
or history. Primary evidence clarifies size_t[2], KiB, and the format,
but scope, caches, budgets, reservations, and heaps remain limited.
Unique observed and revalidated interop match is not physical LUID
identity. Validation covers 64-bit values, zero, and overflow; extracting
the helper preserved the earlier OpenCL version provider. Positive
RX 7900 XT on MinGW/MSVC: 21435580416 B and largest block 21413273600 B
in the first sample; global AMD memory remains unknown in Monitor.
Release: Windows 131/131 on both toolchains and Linux 111/111. AMD
and interop each repeated 20 times per Windows toolchain; three installed
consumers rebuilt, relinked, and exited 0. Logs: work/amd-runtime-*.
Initial test expected four closes even on early rejection; corrected
to compare opens and closes. Logs retained; MSVC test D9025 expected.
Additive API 0.2 without changes to existing layouts, vtables, or
formats. M04 still needs global AMD/Intel memory, physical NVIDIA,
and broader coverage; M05 deferred. C57/E11/P5/B2, 76.0% closed/
24.0% open, 18 open. Mirror 56 confirmed: comment 90140271597603;
remote titles and statuses not reconciled.

Previous unit, revision 55 (2026-10-21): M04 E->E. NVML System32/legacy
loader completed. [Contract and evidence](WINDOWS_NVML_LOADER.en.md).
Fixed NVSMI location through ProgramFilesX64 Known Folder; fallback
only for absence, not access denial or bad images. No environment,
PATH, or installation. Validated absolute path and restricted flags
do not certify signatures, ACLs, reparse points, or previously loaded
modules. CUDA/KMT, 13 symbols, identity, and memory behavior unchanged.
Win32 loader diagnostics are included in the note without a fabricated
status 0. Release: Windows 130/130 on both toolchains and Linux 111/111;
loader and memory each repeated 20 times per Windows toolchain. Three
installed consumers rebuilt, relinked, and exited 0. Logs:
work/nvml-legacy-*. Initial MinGW type/narrowing error fixed; MSVC test
D9025 expected. Native AMD host also lacked legacy NVML (126), not a
positive physical NVIDIA result. AMD/Intel free memory and coverage
remain pending in M04; M05 deferred. No new API/ABI, vtables, or formats;
Linux unchanged. C57/E11/P5/B2, 76.0% closed/24.0% open, 18 open.
Mirror 55 confirmed: comment 90140271596413; remote titles and statuses
not reconciled.

Previous unit, revision 54 (2026-10-21): M04 E->E. Windows NVML free
memory completed. [Contract and evidence](WINDOWS_NVML_MEMORY.en.md):
the existing field holds unallocated framebuffer bytes reported by NVML
v1, not an allocation guarantee, budget, or DXGI-minus-PDH estimate.
Exact CUDA LUID/UUIDv2 to NVML root UUID and KMT count 1, with rereads,
uniqueness, bounds, topology, 64-bit values, and zero. Other providers
unchanged. Optional System32 drivers without SDK, CLI, download, or
library-created contexts, buffers, or kernels. cuInit can leave internal
driver state or threads; NVML init/shutdown paired and KMT/DLL resources
use RAII. Native 64-bit provider; no legacy NVSMI fallback at this point.
Release: Windows 129/129 on both toolchains and Linux 111/111; NVML
repeated 20 times per Windows toolchain. Three installed consumers
rebuilt, relinked, and exited 0. Logs: work/nvml-free-*, final-* and
release-repeat-*. AMD host confirms absence and missing-DLL probe,
NOT positive physical NVIDIA/WDDM/TCC/multi-GPU/MIG/linked/hotplug.
Injected tests do not certify hardware. No new API/ABI, vtables, or
formats; Linux unchanged. AMD/Intel free memory and coverage remain in
M04; M05 deferred. C57/E11/P5/B2, 76.0% closed/24.0% open, 18 open.
Mirror 54 confirmed: comment 90140271595547; remote titles and statuses
not reconciled.

Previous unit, revision 53 (2026-10-21): M04 E->E. Vulkan MSVC support
and uniqueness completed. Windows CMake detection accepts a private
path to existing headers and the VULKAN_SDK environment setting without
download, installation, or export to consumers. MSVC previously used
the stub; native RX 7900 XT Vulkan 1.4.349 by LUID now matches MinGW.
Explicit OFF honored and tested 1/1 with headers present; clear a
negative cached check with -U. Duplicate or empty destination IDs
remain unknown, without affecting another unique row.
[Evidence](WINDOWS_GPU_VULKAN.en.md). Release: Windows 128/128 on both
toolchains and Linux 111/111, 20 Vulkan repetitions per Windows
toolchain. Three installed consumers rebuilt, relinked, and exited 0.
Logs: work/vulkan-portable-*. No new API/ABI, vtables, or formats.
AMD free memory research was bounded; no guessed format or DXGI-minus-PDH
calculation. M04 still needs global memory and coverage; M05 deferred.
C57/E11/P5/B2, 76.0% closed/24.0% open, 18 open.
Mirror 53 confirmed: comment 90140271591287; remote titles and statuses
not reconciled.

Previous unit, revision 52 (2026-10-21): M04 E->E. Opt-in WGL/OpenGL
completed. Existing option, false by default. Reads GL_VERSION from the
thread's current desktop context only with a stable Win32 EXT LUID and
one unique DXGI row. Not maximum capability, physical-node information,
or proof of acceleration. No name fallback. System32 loading with RAII,
per-context functions, limits, contracts, and checks for versions,
duplicates, and changes. The library creates no context/window, draws
nothing, and does not call glGetError. Private AMD node-query diagnostic
returned 1280; the node-based prototype was dropped. Final documented
contract is ADAPTER scope by LUID, without querying or inferring nodes.
[Contract and evidence](WINDOWS_CURRENT_OPENGL.en.md): native RX 7900 XT
4.6.0 on MinGW/MSVC, directly and through Monitor; prior error untouched
and no new error in final checks, context/DC preserved or restored, and
private window/class removed. Release: Windows 128/128 on both toolchains
and Linux 111/111. Twenty final WGL repetitions per Windows toolchain;
three installed consumers rebuilt, relinked, and exited 0. No new
layouts, API, vtables, or formats. Opt-in Windows behavior in 0.2;
Linux EGL/GLX unchanged. Sampler does not inherit the GUI context.
Final logs: work/wgl-release-*. Known zero LUID, no write, partial write,
and negative HighPart tested. Initial GCC warning fixed; expected MSVC
D9025 from test /UNDEBUG retained. M04 not closed: remaining criterion,
global memory, and adapter coverage pending; M01 open, M05 deferred,
other IDs unchanged. C57/E11/P5/B2, 76.0% closed/24.0% open, 18 subtasks.
Mirror 52: comment 90140271590512.

Previous unit, revision 51 (2026-10-21): M01 E->E. MSVC x64 compatibility
completed. VS 18/MSVC 19.51/SDK 26100. Actual SDK rename failure fixed
using a documented internal ABI, without the WDK or switching to
path-based operations; same handle/identity/no-overwrite contract.
/utf-8 only for C++ plus a literal test. Type, ASCII, and variable
warnings fixed. Negative-time test portable by native tick, still
rejecting and preserving the store. [MSVC evidence](MSVC_VALIDATION.en.md):
Release MSVC 127/127, MinGW 127/127, Linux 111/111. 80 MSVC and 100 Linux
repetitions, three separate installed consumers rebuilt, relinked, and
exited 0. Positive native private Startup test and host CPU/firmware
reads; no real Startup/SCM changes or certified multi-group hardware.
Manual SCM probe only compiled. Final MSVC had no C warnings and kept
intentional D9025 from test /UNDEBUG; MinGW/Linux warning-free. Interrupted
and failing logs retained in work/msvc-*, with final MSVC verified/native
suite and post-clock regression. No new API, layout, vtables, or formats;
version 0.2. M01 still needs stable compression and multi-group hardware.
CI N01/A05/N03 not closed. M05 deferred by explicit user confirmation on
October 21. C57/E11/P5/B2, 76.0% closed/24.0% open, 18 open.
Mirror 51 confirmed: comment 90140271587468; no fractional credit or new IDs.

Previous unit, revision 50: J05 P->C under the previously defined
alternative of documenting unavailable capability, not because an actual
disconnect was performed. Public static API by utmp/login1 adapter:
false/available with a reason on Linux, unknown for an unknown enum or
non-Linux. Inventory propagates the result only with recognized, known
metadata. Missing metadata now yields unknown, not generic false. IDs,
Remote, Active, State, and logout tokens are not capability evidence.
No signals, locking, logout, or elevation. Native contracts explain why
utmp/login1 do not offer transport detach, not why every remote server
would lack it. [Scope and evidence](LINUX_DISCONNECT.en.md). J03 is not
counted twice; WTS M06 and logout J02 remain separate. Release Linux
110/110 and Windows 126/126, 60 repetitions per platform, both consumers
rebuilt, relinked, and run with exit 0. Private POSIX child preserved
and private utmp cleaned; real login1 read-only, one session. The first
new NUL case failed because of an expectation; corrected to require
the existing invalid_argument without changing validation. Logs
retained: work/disconnect-capability-*. New header, enum, and function;
no new layouts, vtable, history, or dependencies. C57/E11/P5/B2, 76.0%
closed/24.0% open. M05 deferred; “prosigue” did not enable experimental
opt-in. Mirror 50: comment 90140270925508.

Previous unit, revision 49: M05 E->E. Documentation research, no new
code or tests. StartupTask for the current package is not general
control, and RequestEnableAsync does not override a user's disabling
an entry. No general contract was found in the official sources
reviewed, without claiming universal absence. Reviewed revision 48
Run error 6801 evidence without new probes, configuration changes, or
an attributed cause. Asked whether to defer M05 or accept a version-limited
experimental provider based on internals; that would not authorize real
changes, UAC, or ACL changes in tests. See the [technical
decision](WINDOWS_STARTUP_APPROVAL_DECISION.en.md). Criterion unchanged,
no closure or partial credit. C56/E11/P6/B2, 74.7% closed/25.3% open.
Revision 48 suites (126/110) were NOT rerun. Mirror 49 confirmed:
comment 90140270919388.

Previous unit, revision 48: M05 E->E. HKLM32/64 reads and listings
complete; transactional control implemented and fully modeled, but
successful native mutation remains PENDING because of 6801. Separate
API per view; IDs select hive/view, while older listing and Monitor
remain HKCU. Software pinned, flags explicit, backup in the same
hive/view, no fallback, permissions and architecture constraints
preserved. Run auditing copies the request before calling the sink.
Real HKLM: one row in 32-bit and four in 64-bit, successful individual
reads without writes. Redirected private native test returned 6801
without writes. No real HKLM mutation, elevated ACL, legacy
virtualization, or MSIX coverage certified.
WINDOWS_MACHINE_RUN_CONTROL.en.md and work/machine-run-pinned-*.
Release Linux 110/110 and Windows 126/126, 60 Windows and 20 Linux
repetitions, both consumers rebuilt, relinked, and run with exit 0;
clean final builds. Additive API and enum, no new virtual method,
layouts, or formats. HKLM all-user behavior change in 0.x requires
host consent; no UAC or configuration/ACL changes. M05 still needs
successful native Run mutation, StartupApproved/approval, and generic
Windows action; A05/L05/N03 remain. C56/E11/P6/B2, 74.7% closed/25.3%
open. Mirror 48 confirmed: comment 90140270915863.

Previous unit, revision 47: M05 E->E. Common Startup file support
completed. User/common ID selects the Known Folder; common listing
is separate. Existing user listing and Monitor are not expanded.
Checks, auditing, and no-replace rename remain. 0.x behavior change:
common can affect all users using existing rights and requires host
disclosure and consent, without elevation, ACL changes, or fallback.
Private native test with two roots covered audited removal/listing/
restoration, conflicts, scopes, roots, isolation, and injected
enumeration permission failures. Real common Startup read-only;
no actual ProgramData/UAC/ACL mutation or native ACL denial tested.
WINDOWS_COMMON_STARTUP_FILE_CONTROL.en.md; work/startup-common-*.log.
Release Linux 110/110 and Windows 125/125, 60 Windows repetitions,
both consumers rebuilt, relinked, and run with exit 0, clean builds.
Additive free API, no new virtual method, layouts, or formats and
no real Startup changes. M05 still needs HKLM, approval/StartupApproved,
and generic Windows action; Run 6801 unresolved; A05/L05/N03 open.
C56/E11/P6/B2, 74.7% closed/25.3% open. Mirror 47 confirmed:
comment 90140270909066.

Previous unit, revision 46: M05 E->E. Explicit file action in
SystemActions/AuditedActions completed. Separate audit operation
preserves earlier ordinals. Request copied before callback; full
argument goes to the checked backend without fallback. Sensitive
log ID is bounded; no contents or file/folder identities. requested
fails closed; best-effort is explicit; failed completion recording
preserves the result, and exceptions propagate. Private native
audited removal/restoration and sink failure tests passed. Portable
tests cover forwarding, errors, policies, limits, and a callback
that changes the original argument. New 0.x virtual method requires
rebuilding the library and ALL consumers/backends, with no new
Snapshot/Options/AuditEvent layouts or formats.
WINDOWS_STARTUP_FILE_AUDIT.en.md; work/startup-file-audit-*.log.
Linux 110/110, Windows 125/125, 40 Windows and 20 Linux repetitions,
both installed consumers rebuilt, relinked, and run with exit 0.
Final builds clean. Initial WSL clock skew recorded without changing
the clock. Run 6801 confirmed without writes. Previous suites also
included that test even when notes said it had not been repeated
(there was no dedicated probe). No real Startup change or leftover
fixtures. M05 still needs HKLM/common, approval, and generic action;
A05/N03 open. C56/E11/P6/B2, 74.7% closed/25.3% open.
Mirror 46 confirmed: comment 90140270903626.

Previous unit, revision 45: M05 E->E. Backup reporting in
Monitor/Sampler completed, explicit option false by default, separate
Metric without mixing Startup and Run. Off skips the provider; on
preserves empty results, permissions, errors, values, and notes,
and clears stale data. Ownership and Sampler latest/history/timed_history
without new persistence, actions, or window changes. Private native
fixture passed for report population; real Monitor/Sampler read-only,
with injected gating/error/copy cases. Snapshot/Options 0.x layout
change: library and ALL consumers rebuilt and relinked. Provider
types moved without layout changes, no new virtual methods or formats.
WINDOWS_STARTUP_FILE_MONITOR.en.md; work/startup-backup-monitor-*.log.
Release Linux 109/109 and Windows 124/124, 40 Windows and 20 Linux
repetitions, both consumers exit 0. WSL clock skew in generated
dependencies recorded; additional settled pass without clock/config
changes or an attributed cause. Final settled pass exit 0 without
warnings or recompilation/relinking; suites used the same binaries.
Windows warning-free. M05 open for HKLM/common, approval, auditing,
and generic actions. Run 6801 unresolved and not reprobed in this
unit; A05/N03 unchanged. C56/E11/P6/B2, 74.7% closed/25.3% open.
Mirror 45 confirmed: comment 90140270896765.

Previous unit, revision 44: M05 E->E. Backup listing completed with
explicit list_windows_startup_file_backups API and an additive UTF-8
name/registration type. Supports discovery and restoration without
saved IDs after restarting the consumer. Two bounded handle-based
passes check identities, conflicts, and context. Estimated, not atomic;
no directory creation or writes; permission errors and other failures
yield no partial list. Private native fixture passed for multiple files,
Unicode, size zero, ordering, exclusions, and conflict. Injected errors,
records, bounds, permissions, and a between-pass change are rejected.
WINDOWS_STARTUP_FILE_CONTROL.en.md; work/startup-backup-listing-*.log.
Release Linux 108/108 and Windows 123/123, 20 Windows repetitions, both
consumers rebuilt, relinked, and run with exit 0, clean final builds.
No changes to existing layouts, virtual ABI, or history; no new CLI,
elevation, or KTM and no real Startup changes. M05 open for HKLM/common,
approval, and integration. Run 6801 unresolved and not reprobed.
C56/E11/P6/B2, 74.7% closed/25.3% open. Mirror 44 confirmed:
comment 90140270891958.

Previous unit, revision 43: M05 E->E. New explicit current-user
Startup file removal/restoration action; POSITIVE native test with
a private fixture outside real Startup. Relative NtCreateFile and
handle-based NtSetInformationFile rename without replacement, with
checked identities. Same file, bytes, and attributes retained; backup
in PCInsight.Disabled.v1. Reads do not create it. Retain ID for restore;
no full backup listing yet. Local drives only, no UNC/common/HKLM/
StartupApproved, CLI, KTM, elevation, or fallback. Additive installed
API without layout, virtual ABI, or history changes.
WINDOWS_STARTUP_FILE_CONTROL.en.md; work/startup-file-control-*.log.
Release Linux 108/108 and Windows 123/123, 20 Windows repetitions,
both consumers rebuilt, relinked, and run with exit 0, clean final
builds. Readonly preserved; conflicts, staleness, sharing, changes,
and bounds tested. Two fixtures aborted due to an incorrect readonly
expectation were cleaned by exact name; final fixtures clean.
M05 remains open for effective approval, HKLM/common, listing, and
integration. Run TxR 6801 unresolved and not reprobed. C56/E11/P6/B2,
74.7% closed/25.3% open. Mirror 43 confirmed: comment 90140270888697.

Previous unit, revision 42: M05 E->E. Fixed Startup root validation:
is_absolute accepted a stream colon (C:\Fixture:stream). Regression
failed before and passed afterward. Ordinary and extended drive/UNC
validation, rejection before enumeration, and cleanup tested.
GLOBALROOT, pipes, and incomplete UNC were already rejected by the
current runtime; no prior failure attributed. WINDOWS_STARTUP_FOLDERS.en.md
and work/startup-root-*.log. Release Linux 108/108 and Windows 122/122,
20 Windows repetitions, both installed consumers relinked and run with
exit 0, clean builds. Run TxR still 6801, no writes; no KTM repair or
real Startup changes. This implements neither actions nor approval;
M05 remains open. C56/E11/P6/B2, 74.7% closed/25.3% open.
Mirror 42 confirmed: comment 90140270883882.

Previous unit, revision 41: L03 B->C. The user replied “prosigue” to
a concrete proposal for a limited advanced option, off by default,
with PID/TID risk and no guarantees of checked operation, EcoQoS,
new-thread handling, or restoration. Generic Linux on/off is disabled
without touching the target; availability false/available. Checked
operation remains unsupported without fallback. Advanced apply/restore
require explicit ack=true and retaining the receipt after failures.
0.x BEHAVIOR change documented in LINUX_EFFICIENCY.en.md and
LINUX_EFFICIENCY_DECISION.en.md. No new signatures, layouts, virtual
ABI, or history; Windows EcoQoS unchanged. Native tests only on
private children: rejection without changes, two-thread opt-in,
full restore and mixed denial with pending flags, caller intact and
children reaped. Release Linux 108/108 and Windows 122/122, 20 Linux
repetitions, both consumers rebuilt, relinked, and run with exit 0,
clean final builds. No race solved and no new helper, elevation, ptrace,
or cgroups. L05/N02/N03 unchanged. C56/E11/P6/B2, 56/75=74.7% and
19/75=25.3%. Mirror 41 confirmed: comment 90140270880053; no new IDs
or fractional credit.

Previous unit, revision 40: E06 E->C. The user replied “prosigue”
to the specific E06 proposal: report observed transport and media,
leave them unknown without evidence, and do not infer SSD from NVMe.
This existing alternative was accepted only for E06, not L03 or a
new universally exact media claim. RPM retains permission_required
if ATA/B1 is denied and no valid report exists; a valid independent
report survives. Tests cover layers, unrelated VPD, stale SSD,
permissions, unknown, no invented zero RPM, and alias value/support/note.
LINUX_DISK_IDENTITY.en.md. WSL: four contexts, zero media results;
technology positives came from fixtures, not physical hardware.
N02/N03 unchanged. Release Linux 108/108 and Windows 122/122, ten
focused Linux repetitions, both installed consumers rebuilt, relinked,
and run with exit 0, no warnings. No new layouts, API, virtual ABI,
history, dependencies, /dev access, CLI, elevation, or disk changes.
C55/E11/P6/B3, 55/75=73.3% and 20/75=26.7%. Mirror 40 confirmed:
comment 90140270876164; no new IDs or fractional credit.

Previous unit, revision 39: C03 E->C. The user replied “prosigue” to the
specific proposal to accept the labeled, estimated difference between
installed and usable RAM, and unknown without valid data. C03 only,
not E06/L03. This was an existing alternative, not a universal BIOS
implementation or a measure of hardware-only reservations. The difference
and its alias preserve the usable-total permission status. New fixtures
cover missing, denied, contradictory, and unsupported data; even zero is
always estimated, with no stale values. LINUX_MEMORY_COMPOSITION.en.md.
Release Linux 108/108, Windows 122/122, ten focused Linux repetitions,
both installed consumers rebuilt, relinked, and run with exit 0; clean
builds. Native WSL composition passed, with zero zram devices observed
and Zswap/firmware data absent, not global zero values. Positive
compression/firmware cases came from fixtures. N02/N03 unchanged. No
existing layout, virtual ABI, history, or dependency changes, elevation,
or new zram, swap, or RAM pressure. C54/E12/P6/B3, 54/75=72.0% closed,
21/75=28.0% open. Mirror 39 confirmed: comment 90140270873160; no new IDs
or fractional credit.

Previous unit, revision 38: M03 E->C. WMI succeeded on 5/5 disks:
two HDDs and three SSDs, SATA/NVMe, without UAC, permission changes, or
changes to WMI code. Earlier failure is historical, not a cause fixed here.
Filesystem capacity 5/5, one system disk and one page-file disk, read/write
rates and latency 5/5, valid active time 1/5 in the detailed sample and
5/5 in a later diagnostic. Availability varies; no clamping or invented
zero. Send/receive rates known for 55 interfaces without generating traffic.
query_windows_network_identity uses unique revalidated LUID/GUID/PnP
matching. Manufacturer and description are Metrics; nine manufacturers,
eight Ethernet interfaces, and a positive Realtek PCIe 2.5GbE result.
WINDOWS_NETWORK_IDENTITY.en.md. Twenty identity and twenty network
repetitions. Release Windows 122/122 and Linux 108/108. Both consumers
rebuilt, relinked, and run with exit 0. Additive API without changes to
existing layouts, virtual ABI, or history. Closes supported providers
within their limits, not universal RAID/MPIO/physical hotplug, authenticated
brands, or arbitrary allocation across disks. M02/M06/F03/N01/N03 unchanged.
C53/E13/P6/B3, 53/75=70.7% closed, 22/75=29.3% open. Mirror 38 confirmed:
comment 90140270634154; no new IDs or fractional credit.

Previous unit, revision 37: M07 E->C. Read-only open_windows_service API
for the GUI without a shell, editor, or elevation. Metadata uses Metrics
and typed errors; a known empty value is distinct from inaccessible data.
Load-order group, raw SCM description, states, and PID are explicit.
WINDOWS_SERVICE_VIEW.en.md. Positive results for eight services, eight
descriptions, and eight groups. Private launch verified argv; stale token
rejected; checked EcoQoS on/off reread; checked end-task used the same
handle and returned exit 1. Private child cleaned up. Twenty repetitions
per test (40 runs). Revision 36's positive SCM result and cleanup were
retained, not repeated. Release Windows 121/121 and Linux 108/108. Both
installed consumers rebuilt, relinked, and run with exit 0. Additive API,
no existing layout, virtual ABI, SystemSnapshot, or history changes.
Closes supported providers/actions, not combined efficiency mode, power
savings, automatic restoration, localized descriptions, or svchost groups.
L05/M02/M06/N01/N03 unchanged. C52/E14/P6/B3, 52/75=69.3% closed,
23/75=30.7% open. Mirror 37 confirmed: comment 90140270623147; no new IDs,
fractional credit, or criterion change.

Previous unit, revision 36: M07 E->E. Authorized native SCM validation
with UAC and a private LocalService service. Public start/stop/restart,
idempotence, and real inventory passed for state, PID, description, and
group. Static executable copy in ProgramData with a protected ACL,
without opening the user profile or using LocalSystem. A 60 s watchdog;
registration, copy, and directory removed and absence checked again.
PASS. Manual tests/windows_service_native.cpp is outside CTest;
WINDOWS_SERVICES.en.md. Initial launch from the profile failed with
Win32=5; cleanup completed and evidence retained. No opt-in exits 2;
insufficient privileges return Win32=5 before creation. Release Windows
119/119 and Linux 108/108. No production code, API, ABI, layout, or history
changes or consumer migration. M07 still needs details and metadata
availability; production helper is L05. C51/E15/P6/B3, 51/75=68.0%
closed, 24/75=32.0% open. No overall closure or new IDs. Mirror 36
confirmed: comment 90140270598882; remote title/statuses unchanged.

Previous unit, revision 35: E07 P->C. Declared ext2/3/4 capacity without
mounting, queried by region and DiskInfo.id. Sysfs enumerates partitions
or a whole device. Expected identity is revalidated; a total is returned
only when ALL regions are known and non-overlapping. No partial totals
or counting aliases/layers. Logical metadata declaration, NOT statvfs
usable/free space or universal physical capacity. LINUX_FILESYSTEM_CAPACITY.en.md.
Native tests used real 32 MiB images and a 32 MiB filesystem in a 48 MiB
source; WSL 8:48/sdd, 1 TiB, valid checksum. Individual and per-disk tests
exited 0. No real disks or mounts modified; no new SDK CLI or dependencies.
An existing formatter was used only in the private harness. Linux 108/108
and Windows 119/119; both installed consumers rebuilt, relinked, and run
with exit 0, no warnings. Additive API without existing layout, virtual
ABI, SystemSnapshot, or history changes. Other formats, raw MBR/GPT, and
physical RAID/LVM/loop allocation remain unknown. C51/E15/P6/B3,
51/75=68.0% closed, 24/75=32.0% open. F03 deferred; N02/N03 open.
Mirror 35 confirmed: comment 90140270531513; remote title/statuses unchanged.

Previous unit, revision 34: H05 P->C. Opt-in LinuxNotificationObserver
with consent, attribution/privacy/period policy, and native capture.
Notify and replies matched by owner/sender/serial count accepted calls
and replacements, NOT notifications displayed or read. Rejected,
unacknowledged, pending, and expired calls stay separate. Declared app
hint is estimated; unknown without a hint. No proof of PID/executable
identity or CPU join. No retained content, CLI, autolaunch, elevation,
or persistence. Owner loss and errors invalidate the period; stop keeps
the last finite period and restart starts another. LINUX_NOTIFICATIONS.en.md.
Private native test: three accepted, one replacement, one rejected, one
unknown app, and one unacknowledged, with real denial and owner loss.
10/10 repetitions, direct exit 0, no skip. Linux 106/106 and Windows
118/118; both consumers rebuilt, relinked, and run with exit 0, clean
final builds. Additive API, no existing layout, virtual ABI,
SystemSnapshot, or history changes. No universal GTK/portal/vendor
coverage or retroactive data. C50/E15/P7/B3, 50/75=66.7% closed,
25/75=33.3% open; H04/M08/F03/N03 open. Mirror 34 confirmed:
comment 90140270530011; remote title/statuses unchanged.

Previous unit, revision 33: I05 E->C. Modern and legacy AppStream
provider with explicit source policy and absence conditions; criterion
unchanged. Public query and opt-in Monitor use an exact, unique desktop
ID match. Declared publisher is estimated, not a signer or executable
owner. Optional system libxml2, without helpers, downloads, or installs.
Absence, errors, and ambiguity yield unknown, not a stale label.
LINUX_STARTUP_PUBLISHER.en.md. Native XML/Monitor passed with private
files, exit 0 without skips. Tests cover rejection of DTD/XXE and
language, duplicate, permission, limit, and incomplete-reader cases.
Linux 104/104 and Windows 117/117; both installed consumers rebuilt,
relinked, and run with exit 0, clean final builds. New SampleOptions 0.2
flag: rebuild ALL; no StartupEntry, virtual ABI, or history changes.
No universal vendor/Flatpak/Snap coverage or closure of I07/K03/F03/N03.
C49/E15/P8/B3, 49/75=65.3% closed, 26/75=34.7% open. Mirror 33 confirmed:
comment 90140270529005; remote title/statuses unchanged.

Previous unit, revision 32: E05 E->C. Positive result from an authorized
REAL active 64 MiB swap file on WSL/ext4. /proc/swaps reported 65,532 KiB
and zero used; mapped to sdd, through the native reader/report, to
Monitor true/estimated. swapoff, unlink, and rmdir succeeded; previous
swap untouched. No fstab, permanent configuration, or external
utilities. LINUX_SWAP_DISKS.en.md and manual linux_swap_active_native.cpp
outside CTest, with required flag and guard 77. No API, layout, or history
change; no universal XFS/Btrfs/F2FS/RAID/LVM or kernel-inode identity
support. Linux 102/102 and Windows 116/116, both consumers exit 0,
no warnings. C48/E16/P8/B3, 48/75=64.0% closed, 27/75=36.0% open.
Mirror 32 confirmed: comment 90140270527425; remote title/statuses
unchanged. F03 integration remains deferred and N03 risks unchanged.

Previous unit, revision 31: J06 P->C. Manage user accounts integration
for the consumer GUI. AccountManagementIntegration offers explicit
query/open; capability is queried again and the receipt reports
host_invoked on errors or exceptions. No provider means nothing runs.
No native manager, universal panel, CRUD, elevation, or command
autodetection. The host supplies the view/manager as trusted code.
ACCOUNT_MANAGEMENT.en.md. Positive public API test with a private child
on both platforms, not an actual accounts UI. Linux 102/102 and Windows
116/116; installed consumers rebuilt, relinked, and run with exit 0,
no warnings. Additive API, no existing virtual ABI, layout, or history
changes. C47/E17/P8/B3, 47/75=62.7% closed, 28/75=37.3% open. Only
J06 P->C; no new IDs or fractional credit. F03/J04/N03 open.
Mirror 31 confirmed: comment 90140270525751; remote title/statuses unchanged.

Previous unit, revision 30: F03 E->E. TGID/start_boot/UID identity and
private writer, sequence, and sticky overflow guards. Authorized native
test passed with threads sharing a leader, rejection of incorrect identity
and injected overflow, child exclusion, and teardown. TCP 6144/6144,
UDP 384/384, and 12 peeks. Linux 101/101 and Windows 115/115, both
consumers exit 0, no warnings. LINUX_NETWORK_QUALITY.en.md. Overlap
rejection was tested in fixtures, not observed live (ten coherent and zero
busy reads). No integrated producer, netns, join, public API, ABI, or
history. F03 remains E, J04/H04 open, and N03 unchanged. 46/75=61.3%
closed and 29/75=38.7% open, unchanged. The user asked not to prolong
stages: defer F03 integration and move to another ID. Mirror 30 confirmed:
comment 90140270524925; remote title/statuses unchanged.

Previous unit, revision 29: F03 E->E. Private classification into four
TCP/UDP lanes by socket, family, and BTF protocol
(inet{,6}_sendmsg/sock_recvmsg), not by port or name. Read failures and
PEEK/TRUNC/ERRQUEUE receives are tracked separately. Authorized native
send/sendto and sendmsg over IPv4/IPv6: TCP 4096/4096, UDP 256/256, zero
failures, and eight excluded peeks. Child exclusion and removal of three
links checked, exit 0; legacy 1088/1088 remains positive.
LINUX_NETWORK_CLASSIFIED.en.md. Linux 101/101 and Windows 115/115, both
installed consumers exit 0, warning-free builds. Temporary root, with no
pinning, permanent configuration, or installed utilities. Manual test;
CTest never loads the probes. No public API, ABI, history, or complete
producer with UID/start_boot/netns, quality, saturation, consistency, and
integration. F03 remains E, J04/H04 open, legacy network absent, N03
unchanged. 46/75=61.3% closed, 29/75=38.7% open; no new IDs or fractional
credit. Continue F03 identity, quality, and integration. Mirror 29
confirmed: comment 90140270524004; remote title/statuses unchanged.

Previous unit, revision 28: F03 E->E. Authorized private eBPF test
passed. The 18-instruction generator's kernel TGID was checked through
unattached TEST_RUN. Filtering precedes context and map reads; fexit
uses BTF signatures for __sys_sendto and sock_recvmsg. TCP 1024 plus UDP
64 gives 1088/1088 send/receive. Phases checked; the excluded child and
traffic after links were closed do not add to the count. Temporary root,
no pinning or permanent config, RAII for FDs, and an explicit flag.
No native test in CTest. LINUX_NETWORK_PROBE.en.md. Linux 100/100 and
Windows 114/114; both installed consumers exit 0. Initial failures kept
and corrected: helper 14 was disallowed in SOCKET_FILTER, sock_sendmsg
did not see send/sendto, and __sock_sendmsg was missing from BTF. The
revised sendto hook uses six arguments and slot 48. This is NOT a
production producer: four lanes, UID/start_boot/netns, loss, coverage,
saturation, and other paths remain, as does integration with the reducer,
Monitor, legacy output, and history. F03 stays E, J04/H04 open, N03
unchanged. 46/75=61.3% closed, 29/75=38.7% open; no new IDs or fractions.
Continue producer, coverage, and identities. Mirror 28 confirmed:
comment 90140270523263; remote title/statuses unchanged.

Previous unit, revision 27: F03 E->E. Internal reducer converts counters
to TCP/UDP rates and UID totals with coverage. It preserves actor/scope
identity and windows, handles loss/reset/overflow/absence conservatively,
and uses exact integer arithmetic. 10,000 portable oracle cases, not a
capture test. LINUX_NETWORK_ACCOUNTING.en.md. Final Linux 99/99 and
Windows 113/113, both installed consumers exit 0. Initial sandboxed
Windows run was 98/113 (15 environment/access failures); repeat outside
was 113/113 without weakening tests. Both logs retained. No new public
API, ABI, or history and no producer, loader, or probes implemented.
No root, attachment, or privilege changes in this unit. On 2026-10-18
the user explicitly authorized a future temporary WSL test on private
processes only, with necessary privileges and cleanup, without permanent
configuration or installing tools. F03 remains E, J04/H04 open, legacy
network absent, N03 unchanged. C46/E17/P9/B3, 46/75=61.3% closed and
29/75=38.7% open; no new IDs or fractions. Continue the F03 producer,
loader, integration, and authorized tests. Mirror 27 confirmed:
comment 90140270522304; remote title/statuses unchanged.

Previous unit, revision 26: F03 E->E. Internal read-only BTF preparation.
Validates kernel signatures and offsets without hardcoded constants or
external utilities. No new loader, probes, maps, events, or TCP/UDP
accounting. LINUX_NETWORK_BTF.en.md. Linux 98/98 (native BTF passed) and
Windows 112/112; both installed consumers relinked and run with exit 0.
Initial fixture fixed without weakening the parser; failing and final
logs retained. No new public API, ABI, or history. Authorization for a
future private probe test was requested but not yet received; no elevation,
load, or attach. F03 remains E, J04/H04 open, legacy network absent.
C46/E17/P9/B3, 46/75=61.3% closed and 29/75=38.7% open, no new IDs or
fractions. Continue F03 loader, accounting, and authorized tests.
Mirror 26 confirmed: comment 90140270363748; remote title/statuses
unchanged. Existing N03 risks remain; passing suites do not resolve them.

Previous unit, revision 25: F03 E->E. TCP query for a selected holder
by PID/start_ticks/FD. Uses pidfd_getfd externally and F_DUPFD_CLOEXEC
for self. Checks procfs and fdinfo's Pid projection, identity and
liveness, and the slot's cookie/device/inode at both endpoints. Sampler
resets on change, absence, or counter reset; no FDs retained between ticks.
No proc-fd/inode fallback, elevation, or data/flag changes. Duplication
briefly extends the socket's lifetime. A holder does NOT prove exclusive
or causal attribution. Aliases or PIDs with the same cookie are not
independent contributions. LINUX_TCP_HOLDERS.en.md. Linux 96/96 and
Windows 111/111; both installed consumers rebuilt, relinked, and exit 0.
Native 8192 B acknowledged/received; private child pidfd_getfd passed
with the same cookie as the parent. Replaced FD, invalid token, UDP,
listener, data, and F_GETFL checked. No changes to existing layout,
virtual ABI, or history. F03 remains E for causal/exclusive attribution,
UDP/other coverage, and integration. J04/H04 open and legacy network
absent. 46/75=61.3%, no new ID or fractional credit; C46/E17/P9/B3
unchanged. Continue F03. Mirror 25 confirmed: comment 90140270363050.
Existing N03 risks unchanged.

Previous unit, revision 24: F03 P->E. Native opt-in LinuxTcpSampler
uses SOCK_DIAG/TCP_INFO directly. TCP ESTABLISHED/CLOSE_WAIT, with
counters and rates keyed by cookie, inode, family, socket UID, and
thread namespaces. Estimated partial sums by SOCKET UID, with coverage
and overflow handling. No real UID, PID, app, or total network
attribution and no UDP/QUIC. No legacy network integration or closure
of J04/H04. LINUX_TCP.en.md. Linux 94/94 and Windows 110/110 without
skipping native TCP validation; both installed consumers rebuilt,
relinked, and exit 0. Private loopback: 8192 B acknowledged and 8192 B
received, two sockets and one UID. Not a benchmark; no changes to other
connections or payload capture. No CLI, eBPF, elevation, or changes to
existing layout, virtual ABI, or history. F03 stays E for PID/app/shared
socket attribution, UDP and other coverage, and the window provider and
integration. 46/75=61.3%, no new IDs or fractions. Continue F03 next.
Mirror 24 confirmed: comment 90140270361819. Existing N03 risks unchanged.

Previous unit, revision 23: J04 P->E. Disk activity, identity, and coverage
by numeric UID, not username. LinuxUserIoDetails separates CPU/RAM scope
and types coverage for known, missing, permissions, identity, overflow,
and rows without UID. D02 retains per-process rate; disk_rate_real_uid
requires a matching UID around reads and across window endpoints without
inventing exact causal ownership. UID aliases share a total; distinct
UIDs are not mixed even if usernames match. utmp entries without UID
have no I/O result. LINUX_USER_IO.en.md. F03 network pending: J04 is NOT
closed. Linux 92/92 and Windows 109/109, installed consumers rebuilt,
relinked, and exit 0. Native UID 1000: 8779070 B/s, four known and two
missing after private file activity; one public report with process rows
hidden. New 0.x ProcessInfo/UserSession/SystemSnapshot layout: rebuild ALL.
No virtual ABI or history changes. 46/75=61.3% unchanged, no new IDs or
fractions. F03 dependency work starts in revision 24. Mirror 23 confirmed:
comment 90140270360992. Prior limits and risks remain, without implicit
scope reductions.

Last closed unit at this point, revision 22: K04. Explicit inventory,
details, and control for the effective user's systemd manager.
LinuxServiceTarget/Inventory/View API with system/current_user scope,
UID, and observed owner. Bus pinned to the effective UID, peer UID
and owner UID checked, destination/sender pinned, and owner revalidated.
No GUI, shell, elevation, or fallback to the system manager. Opening
does not load or start units. Start/Stop/Restart uses the K02 job runner
and confirmed job tracking, with unknown outcome where appropriate.
LINUX_USER_SERVICES.en.md. Linux 91/91 and Windows 108/108; both installed
consumers rebuilt, relinked, and run with exit 0. WSL read-only inventory:
199 system and 19 user units, with details for both. New positive
control tests were simulated; no real services modified. No existing
layout or virtual ABI change. From 45/75 to 46/75=61.3% closed, 29
open=38.7%. K03/L05/N01/N02 and prior limits/risks remain separate.
J04 proposal starts in revision 23. Mirror 22 confirmed:
comment 90140270360264; no new ID or fractional credit.

Previous closed unit, revision 21: G06, DRM/sysfs/NVML audit and fixes.
Bounded per-field reading, identity and driver bindings, real module and
no invented version. Estimated VRAM/headroom and separate GTT in
LinuxGpuSysfsDetails; labeled sensors, no fabricated Windows-style shared
memory or budget presented as global free memory. NVML merge requires
available PCI and a coherent complete tuple; aliases are used together.
linux_gpu_inventory_source distinguishes empty, absent, partial, and
opt-out. LINUX_GPU_TELEMETRY.en.md. Linux 89/89 and Windows 107/107;
installed consumers rebuilt, relinked, and run with exit 0. Initial
temperature test fixed to expect -0.001°C for -1 millidegree; invalid
byte and percentage values remain rejected. Logs retained. WSL: zero
rows, DRM estimated, NVML absent. Positive cases are fixtures, not
physical validation. G07/N02 and M04 separate. New 0.x GpuInfo/
SystemSnapshot layout: rebuild ALL. G06 E->C moves 44/75 to 45/75=60.0%.
No new ID, denominator change, or fractions. L03 remains B;
C03/E05/E06 and N03 risks unchanged. K04 proposal made in revision 22.
Mirror 21 confirmed: comment 90140270359672.

Closed unit, revision 20: G08, Linux driver-date source policy and
documented absence. No real date was obtained; the original criterion
allows unavailability with a reason. driver_date remains absent/
unsupported with a reason from DRM/sysfs, NVML, or PCI merging; the
merge previously lost that reason. No BIOS date, timestamp, package,
build, kernel, API, or DRM placeholder zero is presented as a release
or installation date. LINUX_GPU_DRIVER_DATE.en.md. Linux 88/88 and
Windows 106/106; installed consumers rebuilt, relinked, and run with
exit 0. WSL had zero GPU rows; fixtures are not physical positives.
No API, ABI, layout, or format change or new GPU queries/dependencies.
G08 P->C moves 43/75 to 44/75=58.7%. No new ID, denominator change, or
partial credit. G06/G07/N02/M04 remain pending and L03 remains B.
C03/E05/E06 and N03 risks open. Proposed G06 GPU field audit not started.
Mirror 20 confirmed: comment 90140270359048.

Closed unit, revision 19: G05, current EGL/GLX matched by PCI and probing
of declared profiles. LinuxOpenGlDetails separates context version from
core/compatibility maximums and flags by Metric. Direct GLX/Mesa requires
interop ABI 1, IDs, acceleration, sysfs, extensions, and before/after
context/display/screen checks. No attribution by model or order, or
attribution of software or ambiguous cases. LINUX_CURRENT_GLX.en.md.
Linux 87/87 and Windows 105/105; installed consumers rebuilt, relinked,
and run with exit 0. Four EGL/GLX tests repeated ten times each (40).
Dynamic GLX fixture and private PCI reader verified positives and
rejections. WSL had no GLX context and zero attributions. No physical
positive, headless probing, or context creation claimed. Physical
G07/N02 and Windows M04 retain scope. New GpuInfo 0.x layout: rebuild ALL.
G05 E->C moves 42/75 to 43/75=57.3%; no new ID, denominator change, or
partial credit. L03 stays B. No assumed acceptance or closure for
C03/E05/E06 or N03 risks. Proposed G08 not started. Mirror 19 confirmed:
comment 90140270358672.

Revision 18: L03 E->B pending a scope/architecture decision. See the
[Linux efficiency decision](LINUX_EFFICIENCY_DECISION.en.md). No documented
direct replacement of numeric writes with pidfd. Repeated lists do not
prove universal coverage. Explicit acceptance of a limited provider or
a choice of an additional cooperative or invasive design remains pending.
Neither is introduced without a decision. Documentation-only revision,
no new code or tests. C42/E17/P13/B3, 42/75=56.0%, no new IDs or fractional
credit. Mirror 18 confirmed: comment 90140270357868.

Unit verified in revision 17: partial L03, unchecked opt-in API for
captured threads and a receipt for exact nice/ioprio restoration.
Pending flags survive failures; observed conflicts are not overwritten.
No automatic rollback or elevation. LINUX_EFFICIENCY.en.md. Linux 85/85,
Windows 105/105, both installed consumers rebuilt and run with exit 0;
Linux repetitions 20/20. Native tests used only private two-thread
children: original I/O priority restored; CPU denial 13 explicit with
pending receipt. Caller unchanged, children reaped. L03 remained E in
revision 17 and moved to B in revision 18. No pidfd-safe identity or
tracking of future threads. Before/after comparisons do not close the
checked criterion. Additive API without changes to existing layouts
or virtual methods. Count remains 42/75=56.0%, no new ID or fractional
credit. Mirror 17 confirmed: comment 90140270357119.

Closed unit: K01 in revision 16, systemd service inventory, state, and
metadata. LinuxServiceDetails separates per-field availability, provider,
and Slice grouping. group is the Slice or an empty value, not a systemd
label or Windows/Unix group. Known MainPID 0 is distinct from permission
denial or absence; known descriptions and Slices can be empty. Unit.Id
before and after property reads and unique owner are verified. Manager
inventory remains distinct from partial file fallback; no effective
metadata fabricated. LINUX_SERVICE_METADATA.en.md. Linux 85/85 and
Windows 105/105; installed consumers rebuilt, relinked, and run with
exit 0. Native: 199 services, 117 known MainPID values including zero,
117 known Slice values (15 empty), no real service creation or control.
New 0.x ServiceInfo/SystemSnapshot layout: rebuild ALL. No guarantee of
atomic snapshot or persistent PID, and no Windows grouping claim.
K03/K04/L05/N01/N02 retain scope. K01 E->C moves 41/75 to 42/75 without
a new ID or denominator change. C03/E05/E06 and N03 CPU/history risks
remain open without assumed decisions. The revision 16 L03 proposal
started in revision 17. Checked/full-scope closure still pending without
reducing the criterion or changing the denominator.

Closed unit in revision 15: J01, session inventory/state and visible
aggregates. LinuxSessionDetails separates State/Active/Remote/UID with
Metrics. No WTS connected/disconnected equivalence or invented utmp
activity. Id/Name/User and timestamp checked before and after, final
owner verified, and fields unknown with no token if identity cannot be
verified. Public source distinguishes a known empty enumeration from
an absent or disabled source; it does not certify full host coverage.
LINUX_SESSION_STATE.en.md. Linux 85/85 and Windows 105/105; installed
consumers rebuilt and run with exit 0. WSL: one session, one known State,
and one known foreground flag. Private utmp: three records, empty,
and absent cases. No actions on real sessions. New 0.x UserSession/
SystemSnapshot layout: rebuild ALL. CPU/RAM remain estimated visible
totals by username, repeated across sessions, not unique host totals.
J04/J05/J06 and N02 retain scope. J01 E->C moves 40/75 to 41/75, no new
IDs or fractional credit. C03/E05/E06 not closed and no assumed consent;
N03 CPU/history risks remain pending. The K01 proposal from this revision
was implemented and closed in revision 16.

Closed unit in revision 12: A01, the Metric model and documented
replacement for ambiguous history. AppHistorySample/app_history_view
retain support, notes, provenance, and units. metric_is_known distinguishes
measured zero or false from absence. Conservative legacy import leaves
all counters unknown; even positive values do not prove support.
LegacyAppHistoryEntry is a compatible payload alias; v1/v2 unchanged
and no implicit conversion of measurements to legacy. See
METRIC_AVAILABILITY.en.md. Revision 12 A01 validation: Linux 83/83 and
Windows 103/103. Packages reinstalled and consumers rebuilt and run
with exit 0. Additive API, no new layout for existing structures.
A01 does not close missing providers, per-field audit, or API 1.0;
A05/H04/H05/M08/N03 remain pending.

The cause of the earlier global CPU failure remains under N03. Precise
rejection notes are now shared by calculation and guard, with diagnostic
output before the original assertion. No formula or assertion weakened.
Fixtures cover each reason and 100/100 native repetitions passed
without reproducing the failure or establishing its cause. N03 P->E
is partial, NOT closed; see LINUX_CPU_USAGE.en.md. The 83/83 and 103/103
suites and relinked installed consumers exiting 0 were checked again
in revision 13. E05 remains E pending a real active swap file; no swap
was activated to force closure. See LINUX_SWAP_DISKS.en.md.

C03 remains E, not closed: a hardware-only reservation provider or
explicit acceptance of the estimate is missing. “Prosigue” is not
interpreted as acceptance. Closing A01 moved 39/75 to 40/75 in revision
12. Revisions 13/14 retain 40/75, without new tasks, denominator changes,
or fractional credit for N03/E06.

Revision 14 unit, E06 P->E: LinuxDiskDetails separates protocol,
transport, media, and RPM. SAT 89/B1, VPD 83, and NVMe transport use
identity, bounds, and metadata comparison. Sources report values;
names and rotational are not used to infer them. See
LINUX_DISK_IDENTITY.en.md. Final Release revision 14: Linux 84/84,
Windows 104/104, installed consumers rebuilt and run with exit 0.
NEW 0.x DiskInfo/SystemSnapshot layout: rebuild ALL. WSL: four pinned
contexts, zero media results; positive SATA/SSD/HDD/SAS/NVMe results
are fixtures. E06 remains E: exact media for NVMe/SCSI non-rotating
and hidden backing, or explicit acceptance of reported/unknown limits,
is pending. Question sent, no answer assumed. CPU diagnostics prepared
for recurrence, not declared resolved. First Linux suite was 83/84:
missing history report with disks disabled. Logs retained; a note was
added before the assertion, then 3/3 and 84/84 passed without changing
logic or assertions. Cause remains pending in N03, not attributed to
CPU or E06. E05 requires an environment with an active swap file to
close its verification.

The TXT tracker includes an R01–R20 matrix mapping the request to
active tasks; R requirements are not extra tasks. It retains firmware
and kernel limits and distinguishes synthetic tests from hardware
evidence. The tracking audit does not certify an absence of bugs or
close N01/N02/G07/N03.

When ClickUp is available, synchronize from the TXT tracker while
preserving others' changes and resolving conflicts. A remote error or
quota never blocks updates to the authoritative tracker.

Revision 12 mirror confirmed 2026-10-16, comment 90140269990865,
40/75=53.3%.
Revision 13 mirror confirmed 2026-10-16, comment 90140269993090,
same 40/75.
Revision 14 mirror confirmed 2026-10-16, comment 90140270001412,
same 40/75.
Revision 15 mirror confirmed 2026-10-16, comment 90140270005439,
41/75=54.7%.
Revision 16 mirror confirmed 2026-10-17, comment 90140270356361,
42/75=56.0%.
Remote statuses were not changed and the old list was not treated as
reconciled. Its 60% title is historical, not official.
