# Execution priority: Linux first

Revision 63, 2026-10-21. The user explicitly requested finishing the Linux
scope before resuming Windows implementation. This does not remove requirements
or close tasks. SEGUIMIENTO.txt remains the authoritative tracker.

## Working rule

Unit 172: The [N02/G07 verifier](LINUX_HARDWARE_EVIDENCE.en.md) was tested in
WSL; that does not replace physical evidence. Next: review K03 Linux service
acceptance without touching real services or extending permission to SysV.

Unit 171: B08 remains unknown until OEM mapping and a positive physical
result are available. Next: review verifiable N02/G07 coverage without
changing the BIOS or loading modules.

Unit 170: The Linux CI stage was rehearsed locally; N01 remains in progress.
No push or remote run. Next: B08 firmware virtualization evidence, without
turning indicators into firmware status or changing the BIOS.

Unit 169: Versioning policy and 1.0 gates defined; the local manifest was
verified. A05 remains open for the ABI and acceptance matrix. Next: local
review of the N01 Linux CI workflow; remote CI has not run.

Unit 168: Linux SDK compatibility baseline established in A05. Next:
versioning policy and ABI freeze review. A05 and N03 remain open.

Unit 167: A05 is in progress. An optional SDK header-set marker does not
declare a 1.0 ABI; see SDK_HEADER_COMPATIBILITY.en.md. Next: review
compatibility contracts. Overall N03 and previous authorizations are unchanged.

Unit 166: N03 Linux delivery audit verified, without closing N03 overall.
Next: Linux A05 compatibility review without declaring a 1.0 ABI. Do not
infer SysV permission or change I07's observer-only scope.

Unit 165: Linux Services contract and read-only SDK example completed.
Next: N03 Linux delivery audit, without automatically closing K03, L05,
or other pending validation. No new SysV permission.

Unit 164: Linux Users/Sessions contract and read-only SDK example completed.
Next: Linux Services. N03 remains open; no new SysV permission.

Unit 163: Linux Startup contract and read-only SDK example completed.
Next: Linux Users. N03 and I07 remain open; no new SysV authorization.

Unit 162: Linux App History contract completed, with three separate sources
and separate persistence. See LINUX_APP_HISTORY_REQUIREMENTS.en.md.
Next: Linux Startup. N03 remains E for other blocks, auditing, and historical
risks. No new SysV permission.

Unit 161: Linux Processes/Details and process actions contract completed.
See LINUX_PROCESS_REQUIREMENTS.en.md and the SDK example that performs no
actions or capture. Next: Linux App History. N03 remains E for other blocks,
auditing, and historical risks. SysV still needs explicit permission, and
real K03 acceptance remains unchanged.

Unit 160: Continue N03/Linux while SysV authorization and an environment
for real init acceptance are unavailable. Performance contract completed:
LINUX_PERFORMANCE_REQUIREMENTS.en.md and an SDK-only example for installed
consumers, with explicit unknown/0/false, quality, parent relationships,
units, and limits. Linux 190 + 20 focused checks, MSVC 10, consumer
self-test/sample exit 0, and 65 links/builds successful. No new library or
layouts. Next: process/details and action traceability, one unit at a time.
N03 stays E for other blocks, auditing, and historical failures. G07/N02/B08
remain open. This “continue” does NOT authorize K03 SysV. L05/I07/Windows
unchanged. No real services, elevation, installation, capture, eBPF, or
CI claim. C60/75=80%; 20% open, no partial credit.

Unit 159: Public OpenRC integration completed: linux_openrc_control.hpp,
with Service.open/request/close and a separate Runner. The original selection
is pinned; credentials, directories, and metadata are revalidated. Staleness
is permanent, consent is independent, dispatch occurs once, the feature is
off by default, and exit does NOT mean live state. Linux 188 + 50 OpenRC,
MSVC 20, three SDK consumers relinked with exit 0, FD cleanup, and successful
final builds. Private model, NOT real acceptance. No installation, elevation,
real services, CI claim, or Monitor/SystemActions integration.
LINUX_OPENRC_CONTROL_PUBLIC.en.md and work/openrc-public-*.
SysV control next requires an answer to the explicit request already issued;
permission 148 does not extend to it. Isolated acceptance with real runtimes
is pending. A current PATH probe with no matches does not establish global
absence. Do not list the public integration as pending again. K03 stays E,
L05 deployment is deferred, and I07/Windows are unchanged. C60/75=80%; 20% open.

Unit 158: K03 resumed through the recommended path, with no new transport or
L05 expansion. Internal opt-in OpenRC executor under permission 148, with
target/FD validation, closed environment, gated child and pidfd, ACK/wait and
results that do not invent live state. Private model, not the real OpenRC
runtime. Linux 187 + 40 OpenRC, MSVC 10, two SDK consumers relinked with exit 0,
FD cleanup, and clean final builds. LINUX_OPENRC_CONTROL.en.md and
work/openrc-control-*-delivery* logs. K03 remains E: next, public integration
of protected selection and consent, and isolated real acceptance when an
authorized environment is available. L05 deployment remains E and deferred.
No installation, activation, elevation, real services, or SysV exception.
I07 unchanged; Windows deferred. C60/75=80%, 20% open, no new IDs or partial
credit. Earlier units are historical.

Unit 157: LINUX_PRIVILEGED_TRANSPORT.en.md and private XML fixture. A broker
test with a 32 KiB limit rejects a 64 KiB string before the handler; client 2
remains valid. Three attempts, two handler entries, two authorization checks,
and zero Manager calls. The broker-wide limit is NOT per helper. Do not install
on a shared bus or add a broker as a runtime dependency. Linux 186 + 160 L05
checks, 30 brokers with simulated roles, MSVC 10 argument checks, two SDK
consumers exit 0, FD cleanup, and clean Linux builds. Initial FD0 authorization
failure preserved; fixture corrected. L05 stays E: the test and contract unit
is complete. The next direction depends on the user: defer L05 deployment and
continue with K03/OpenRC (recommended), or extend the private Unix channel.
Do not decide based on silence or install, activate, elevate, or touch real
services. Production unchanged; deployment, process/loader, real polkit,
acceptance, and UAC pending. OpenRC 148 still covers explicit control only;
SysV/K03/I07 unchanged. C60/75=80%, 20% open, no new IDs or partial credit.

Unit 156: LINUX_PRIVILEGED_ADMISSION.en.md. Admission before RPC at both
endpoints: token bucket per receiver and broker sender, bounded table, safe
TTL, mutex and clock, and a local read/write queue threshold. No per-UID or
fairness guarantees, hard queue-byte or pre-receipt cap, or reply limit.
OFF and busy return cheaply before RPC. The native case (five requests,
two authorization checks, one Manager call, one Subscribe) verifies shared
quota, no bypass with a new sender, and rejection without queries. Linux
185 + 150 L05 checks with 20 private brokers and simulated roles, MSVC 20
admission-argument checks, two SDK consumers exit 0, FD cleanup, and clean
Linux builds. Regression with explicit higher limits: 56 requests, 32
authorization checks, 18 Manager calls, two Subscribe calls. L05 remains E
for transport contract, deployment and process/loader, real polkit,
acceptance, and UAC. No installation, activation, elevation, real services,
public API/layout/format changes, Monitor/SystemActions integration, or
CI claim. OpenRC 148 still covers explicit control only; SysV/K03/I07
unchanged. C60/75=80%, 20% open, no new IDs or partial credit.

Unit 155: LINUX_PRIVILEGED_BUDGET.en.md. Shared handler budget for identity,
daemon, Authority, Manager, match, Subscribe, and dispatch RPCs, using a
steady clock and mutex. Limits: 1 ms–30 s and 1–128 calls (default 5 s/96).
The default timeout lease is restored and calls explicitly cap time to the
remainder. Owned authorize-only mode uses 5 s/96; legacy raw mode remains
unbudgeted. This is COOPERATIVE, not a hard process, transport, or cleanup
deadline. No cancellation or retry. An UNKNOWN result after send MAY have
acted. Linux 183 + 130 L05 checks, ten real private brokers with simulated
Authority/Manager, 56 requests, 32 authorization checks, 18 Manager calls,
two Subscribe calls, retention preserved and native 2 s timeout restored.
MSVC 20 budget-argument checks, two SDK consumers exit 0, and FD cleanup.
Initial clock skew preserved; clean final build and suite 183 without
changing the clock. L05 stays E: next admission and transport queues.
Deployment and process/loader, real polkit, acceptance, and UAC pending.
No installation, activation, elevation, real services, public API/layout/
format changes, or CI claim. OpenRC 148 still covers explicit control only;
SysV/K03/I07 unchanged. C60/75=80%, 20% open, no new IDs or partial credit.

Unit 154: LINUX_PRIVILEGED_JOBS.en.md. Match/Subscribe with pinned owner
before dispatch. Correlation uses uoss, a uint32 ID, path and unit, up to
64 early events, 4,096 events, 64 pending watches, and a deadline.
Asynchronous data is thread-safe with no native calls from the consumer.
Connection slot ownership and cleanup are tracked; completed is historical.
Timeout or revocation does NOT cancel, and completed does NOT establish
current service state. Linux 182 + 120 L05 checks with ten real private
brokers and simulated Authority/Manager: 50 requests, 29 authorization checks,
16 Manager calls, and two Subscribe calls. MSVC 20 FSM-argument checks,
two SDK consumers exit 0, FD cleanup, and clean Linux builds. L05 stays E:
next receiver-wide limits and deadline, followed by deployment, loader,
real polkit, acceptance, and UAC. No installation, activation, elevation,
real services, public API/layout/format changes, or CI claim. OpenRC 148
still covers explicit control only; SysV/K03/I07 unchanged. C60/75=80%,
20% open, no new IDs or partial credit.

Unit 153: LINUX_PRIVILEGED_EXECUTION.en.md. Internal ControlSystemService
executor, OFF by default, explicit Manager UID and busy handling. No reusable
authorization report and no change to the authorize-only endpoint. Fresh
authorization and context, Manager owner, policy, and connection checks
before dispatch. Start/Stop/Restart sends the unit and replace to the unique
owner, with autostart and interactive both 0, a 5 s limit, and one attempt.
A queued receipt is NOT completion. Failure after send is UNKNOWN and MAY
have acted; no retry, rollback, or atomicity. Linux 181 + 110 L05 checks
with ten real private brokers and SIMULATED Authority/Manager: 41 requests,
20 authorization checks, and eight Manager calls per run. MSVC 20 default/
argument checks, two SDK consumers exit 0, and FD cleanup. Initial clock
skew preserved, final incremental build clean, final suite 181. L05 stays
E: next job limits and tracking. Deployment/loader, real polkit, and
acceptance remain pending. No installation, activation, elevation, real
services, API/layout/format changes, or CI claim. OpenRC 148 still covers
explicit control only; SysV/K03/I07/UAC unchanged. C60/75=80%, 20% open,
no new IDs or partial credit.

Unit 152: LINUX_PRIVILEGED_CONNECTION.en.md. Validated absolute anchor
using openat/no-follow checks for UID, mode, thread namespace, PID, and
effective UID. Sticky ancestors are off by default and explicitly enabled
for /tmp tests. A new owned connection uses a closed endpoint, node and
peer UID, Unix peer credentials, and CLOEXEC, with FDs, anonymous access,
interactive auth, and exit-on-disconnect set to 0; timeout <=5 s, with
generation and revocation. The receiver binds to its own bus and
revalidates afterward. Across 22 requests and nine checks, a positive
authorization was discarded after the anchor changed. Linux 180 + 100 L05
checks, ten real private brokers with simulated Authority, MSVC 20 argument/
fallback checks, two SDK consumers exit 0, and FD cleanup. Initial indentation
warnings fixed; verified and final builds clean. L05 stays E for limits,
execution flow, and hardened deployment before real acceptance. No
installation, activation, elevation, real services, real polkit, API/layout/
format changes, or CI claim. OpenRC 148 remains explicit-control-only;
SysV/K03/I07/UAC unchanged. C60/75=80%; 20% open, no new IDs or partial credit.

Unit 151: LINUX_PRIVILEGED_POLICY.en.md. Closed parser (96 KiB/256) and native
reader using openat and an anchor FD, trusted UIDs, permission and type
checks, symlink/hard-link and path checks, and revalidation. Refresh
invalidates the generation without fallback; the managed receiver discards
a policy invalidated during a positive CheckAuthorization. Linux 179 + 90
L05 repetitions with ten real private brokers and simulated Authority,
MSVC 20 parser/argument/fallback checks, two SDK consumers exit 0, FD cleanup,
and clean Linux builds. L05 remains E: bootstrap and connection are next.
Trusted environment, loader, and anchor, watcher, deadlines, executor, real
polkit, and acceptance remain pending. No installation, elevation, real
services, API/layout/format changes, or CI claim. OpenRC 148 remains
explicit-control-only; SysV/K03/I07/UAC unchanged. C60/75=80%; 20% open,
no new IDs or partial credit.

Unit 150: LINUX_PRIVILEGED_RECEIVER.en.md. Closed authorize-only receiver
with signature uusb. Neither identity nor argv comes from the payload;
the transport supplies the identity and subject. Tested with real private
libsystemd and broker, simulated Authority, and no polkitd, executor,
installation, elevation, or real services. Linux 178 + 80 L05 repetitions
across ten brokers, MSVC 10 argument checks, two SDK consumers relinked
with exit 0, FD and private-fixture cleanup, and clean final build. Initial
clock skew was retained without changing the clock. L05 remains E: next,
protected configuration and lifecycle. Real executor, policy, and consent,
acceptance, and UAC remain pending. OpenRC 148 still covers explicit control
only; SysV scope did not expand, and K03/I07 are unchanged. C60/75=80%;
20% open, no new IDs or partial credit.

Unit 149: LINUX_PRIVILEGED_NATIVE_AUTHORIZATION.en.md. Native
CheckAuthorization and a coordinator with a fixed factory, request scope,
subject and owner identity, cookie, and bounded parser. No autostart or
interaction by default, cache, retry, or executor. Linux 176 + 60 checks,
MSVC 10 argument/fallback checks, two SDK consumers relinked with exit 0,
and FD cleanup. Simulated ABI and private kernel socket, NOT real polkit,
receiver, helper, dialogs, privileges, installation, or acceptance.
Initial clock skew retained; final build clean without changing the clock.
L05 remains E: next, an isolated receiver with a closed payload and real
transport identity, then bootstrap, configuration, executor, and acceptance.
OpenRC 148 stays explicit-control-only, SysV scope is unchanged, and
K03/I07/UAC are unchanged. C60/75=80%; 20% open, no new IDs or partial credit.

Unit 148, DECISION in force as of 2026-11-02: the user approved the OpenRC
132 exception solely for existing scripts/runtime used by explicit
start/stop/restart, opt-in and OFF by default, with private tests. It does
NOT authorize collection, installation, elevation, real services, or
extension to SysV, helper, polkit, or autostart. LINUX_OPENRC_CONTROL_DECISION.en.md
records the permission. K03 remains E: the decision blocker was removed,
not the implementation or acceptance requirements, and the task was not
closed. Earlier notes about no exception are historical for OpenRC. L05/Linux
remains active; isolated native CheckAuthorization is next. Do not request
development permission again when resuming OpenRC. Each future action
requires explicit consent to execute. No new code or tests. C60/75=80%;
20% open, no new IDs or partial credit.

Unit 147: LINUX_PRIVILEGED_IDENTITY.en.md. Native observer using the current
message and connection, Unix endpoint and peer UID, caller ID/UID, and the
Authority owner's UID, with revalidation. The mechanism supplies
configuration, not the payload or a fixed root UID. Test used a SIMULATED
ABI and a real private kernel socket, NOT a real broker, polkit
authorization, receiver, helper, or CheckAuthorization. Linux 174 + 40
checks, MSVC 10 argument/fallback checks, two SDK consumers relinked with
exit 0, FD cleanup, and clean build. L05 remains E: next native
CheckAuthorization with pinned subject, correlation, and bounded parsing
in isolated fixtures, without dialogs, helper, or executor. Configuration,
bootstrap, receiver, integration, acceptance, and UAC are pending.
K03/OpenRC 132/I07 unchanged. C60/75=80%; 20% open, no new IDs or partial
credit and no API/layout/format changes, actions, or installation.

Unit 146: LINUX_PRIVILEGED_AUTHORIZATION.en.md. Internal coordinator with
fixtures for snapshots, before/after context, one check, sender cookie,
challenge rejection, and retained errors. The mechanism configures the
Authority UID rather than assuming 0. No native adapter, real authorization
or polkit, helper, or executor. Linux 172 + 20 checks, MSVC 10 pure tests,
two SDK consumers relinked with exit 0, and clean build. No new API,
layouts, formats, or actions. L05 remains E: next a native identity/Authority
adapter over verified transport with private rejection tests, without
installing, activating a helper, or executing actions. K03 deferred;
OpenRC 132 and I07 unchanged, UAC deferred. C60/75=80%; 20% open, no new
IDs or partial credit.

Unit 145: LINUX_K03_AUDIT.en.md completes the audit, NOT K03. K03 remains E
and deferred because OpenRC/SysV control had no exception and real acceptance
was not available in the WSL PATH checked. Inventory, views, and Monitor meet
their contract; do not add more projections to work around the blocker.
Move to L05/Linux: LINUX_PRIVILEGED_PREFLIGHT.en.md documents a pure private
preflight with a closed operation set, mechanism-owned allowlist, intent and
bounds checks, rejection of invalid input, and an owned copy. Linux 171 + 10,
MSVC 10 pure checks, two SDK consumers relinked with exit 0, and a clean
build. L05 moves P to E. No authentication, peer verification, polkit grant,
daemon, executor, root, installation, activation, or UAC. Next: IPC identity
and authorization with private rejection fixtures. OpenRC 132 and I07
unchanged. C60/E10/P3/B2, plus 3 excluded X: 60/75=80%; 20% open, no new IDs
or partial credit.

Unit 144: LINUX_SERVICE_SELECTION.en.md. Linux Monitor explicitly selects
seven provider/view routes without fallback. legacy_default is unchanged
and disabled performs no query. Includes guards, switching and quality
checks, and one OpenRC view without joining. SampleOptions ABI extended:
REBUILD ALL 0.x clients. Two SDK consumers rebuilt with exit 0, including
the new Linux Monitor path. Linux 170 + 10, MSVC 10 pure checks, and FD
sentinel verified. Initial clock skew retained; final build clean without
changing the clock. K03 stays E: audit closure per provider, evidence and
blockers, then continue L05 as appropriate. Do not wait indefinitely for
unavailable hardware or invent acceptance. No actions, scripts, format
changes, or ServiceInfo/SystemSnapshot layout changes. OpenRC 132/I07
unchanged. C60/75=80%; 20% open, no new IDs or partial credit.

Unit 143: LINUX_OPENRC_SERVICES.en.md. Explicit OpenRC projection to
ServiceInfo, with separate configured and marked lists, source Metrics,
and indices. Identity is NOT based on name. marked-* is estimated, not
liveness or readiness. PID, description, group, and policy remain UNKNOWN.
Partial results and permissions are independent. Linux 169 + 30, MSVC 10,
two SDK consumers exit 0, FD sentinel, and clean build. No librc, CLI,
execution, scripts, or creation. K03 stays E: next, explicit provider
selection in Monitor with the default unchanged and no silent fallback
or new actions. Integration, control, and acceptance pending. OpenRC 132/I07
unchanged. C60/75=80%; 20% open, no new IDs or partial credit.

Unit 142: LINUX_S6_SERVICES.en.md. Explicit s6 projection to ServiceInfo
with source, index, and logger. The estimated record-* Metric is NOT
liveness, readiness, or control capability. The uint64 PID, wait word,
and ready flag appear only in source evidence; the common PID is UNKNOWN.
Guards cover partial or invalid data, permissions, and shapes. Linux 168 + 50,
MSVC 10, two SDK consumers exit 0, FD sentinel, and control wrote no bytes.
Initial clock skew under one second retained; final build clean without
changing the clock. K03 stays E: next OpenRC without scripts. Integration
and real acceptance pending. Monitor, layout, and contracts 133–136 unchanged.
No execution or inferred exception. OpenRC 132/I07 unchanged. C60/75=80%;
20% open, no new IDs or partial credit.

Unit 141: LINUX_RUNIT_SERVICES.en.md. Explicit runit projection to
ServiceInfo, with evidence by index and scoped IDs. reported-* Metrics
are estimated, NOT liveness, host PID, or authenticated peer identity.
PID is source-only and the common PID is UNKNOWN. Partial results, records,
and permissions can remain UNKNOWN. No invented description, group, or
policy. Linux 167 + 30, MSVC 10, two SDK consumers exit 0, FD sentinel,
control wrote no bytes, clean build. K03 stays E: next s6 projection;
other providers, integration, and acceptance pending. No changes to Monitor,
layouts, or contracts 128–129; no execution, scripts, or inferred exception.
OpenRC 132/I07 unchanged. C60/75=80%; 20% open, no new IDs or partial credit.

Unit 140: LINUX_SYSV_SERVICES.en.md. Explicit SysV query projected to
ServiceInfo, preserving evidence from unit 139, Short/Long fallback,
permissions, partial results, and scoped IDs. PID, liveness, group, and
enablement remain UNKNOWN. No changes to Monitor, systemd, layouts, or
execution. Linux 166 + 40, MSVC 10, two SDK consumers exit 0, FD sentinel
passed, and a clean final build. Initial clock skew under one second retained
without changing the clock or mtime. K03 stays E: next explicit runit
projection, then other providers, integration, control, liveness, and real
acceptance. No SysV script exception; OpenRC 132 and observer-only I07 unchanged.
C60/75=80%; 20% open, no new IDs.

Unit 139: LINUX_SYSV_DESCRIBED_CATALOG.en.md. Opt-in LSB catalog bound
to a pinned candidate and initial identity. Two censuses, root/context
checks, and UNKNOWN on swaps. No joining old snapshots by name. A 0–16 MiB
budget with reservation does not represent disk usage. Linux 165 + 30,
MSVC 20, two SDK consumers exit 0, FD cleanup, clean final build; initial
clock skew under one second retained. K03 stays E: next explicit projection
to the common service model. Integration, control, liveness, and real
acceptance remain open. No execution, CLI, scripts, or inferred exception.
OpenRC 132 and observer-only I07 unchanged. C60/75=80%; 20% open.

Unit 138: LINUX_SYSV_LSB.en.md. Reads declared LSB metadata with a parser,
64 KiB prefix, type/context pins, UTF-8 checks, and bounds. No execution,
eval, or CLI. Linux 164 + 20, MSVC 10, two SDK consumers exit 0, and FD
cleanup. Initial indentation failure at the 1,024 limit was fixed; logs
retained and final build clean. K03 stays E: next bind metadata to a pinned
candidate without joining by basename, then integration, control, liveness,
and real acceptance. No inferred script exception. OpenRC 132 and
observer-only I07 unchanged.

Unit 137: LINUX_SYSV.en.md. Read-only SysV catalog of init.d, the parent
of runlevel directories, and eleven levels, using the S/KNN naming
convention, unique inode association, and type-only fields. Context checks,
two passes, bounds, and explicit unknown/partial results. Linux 163 + 20,
MSVC 10, two SDK consumers exit 0, and private FD cleanup. No execution,
script body reads, CLI, control, or real acceptance. K03 stays E: next LSB
comments for descriptions without a shell. Control, liveness, integration,
and acceptance remain pending. OpenRC scripts decision 132 does not
authorize SysV scripts. At this stage neither had a script-control exception.
Observer-only I07 remains deferred and unchanged.

Unit 136: LINUX_S6_OBSERVATION.en.md. Raw conditions bound to a pinned
control target, with one-shot consent, baseline, polling, and deadline.
No liveness, readiness, or causal completion claim. Retained bytes do
not change completion from UNKNOWN. Linux 162 + 40, MSVC 10, two SDK
consumers exit 0, private FD cleanup, and clean build. No real s6 acceptance,
CLI, execution, or scripts. K03 stays E: next read-only SysV inventory
without inferring permission to run scripts. Integration, real acceptance,
and other pending items unchanged. OpenRC scripts decision 132 remains
deferred; observer-only I07 unchanged.

Unit 135: LINUX_S6_INVENTORY.en.md. Explicit ScanInventory for directories,
aliases, and one logger. Pinned parent, readAtPin, context/root guards,
bounds, and visible partial results. A raw record or FIFO type is NOT
liveness, peer identity, active state, or an action token. A broken logger
is UNKNOWN, not absent. Linux 161 + 30, MSVC 10, two SDK consumers exit 0,
and private FD cleanup. No CLI, execution, locks, writes, or real services.
K03 stays E: next s6 observation tied to pinned control, then integration,
real acceptance, and other init systems. OpenRC scripts decision 132 still
has no authorization at this stage; observer-only I07 unchanged.

Unit 134: LINUX_S6_CONTROL.en.md. Native s6 FIFO control with consent,
pins/context, and u/d/cru. Bytes counted do NOT mean completion. Private
tests cover EPIPE, EAGAIN, SIGPIPE mask and pending state, stale fork,
replacement, and FD cleanup. Linux 160 + 20, MSVC 10, two SDK consumers
exit 0, clean final build. No execution, scripts, CLI, dependencies,
real services, or real acceptance. K03 stays E: next s6 inventory, then
observation, integration, acceptance, and pending SysV and other work.
OpenRC script control remains unauthorized and deferred at this stage;
observer-only I07 unchanged.

Unit 133: LINUX_S6.en.md. Native reader and decoder for the 35-byte record.
Binary history is NOT effective liveness or readiness. Linux 159 + 20,
MSVC 10, two SDK consumers exit 0, and private FD cleanup. No execution,
locks, libs6, CLI, dependencies, or real acceptance. A “continue” without
an answer advances the s6 alternative; it does NOT grant an OpenRC script
exception. K03 stays E: next s6 handle and protocol control with consent,
then inventory, integration, acceptance, and SysV. Observer-only I07
remains deferred and unchanged. OpenRC control remains open and unauthorized
at this stage.

Unit 132: LINUX_OPENRC_CONTROL_DECISION.en.md. Audit without new code or
tests. At this point the decision was pending on existing scripts/runtime
for explicit opt-in OpenRC control ONLY, never collection; private tests
without installation, elevation, or touching real services. The alternative
was to defer this control and continue with s6/SysV. Without an answer,
do not enable the executor or interpret “continue” as an exception.
K03 stays E and observer-only I07 remains unchanged.

Unit 131: LINUX_OPENRC_CATALOG.en.md. Explicit catalog with one init.d
and one runlevels directory. Metadata and links matched by device/inode;
direct stacks with cycles are not expanded. No CLI, librc, script body
read, execution, mkdir, or claim of active or boot-enabled state.
Linux 158 + 20, MSVC 10, two SDK consumers exit 0, private fixtures and FD
cleanup. No real OpenRC acceptance. K03 stays E: next review native OpenRC
control without substituting marker writes for start/stop. Liveness,
integration, real OpenRC/runit, s6, and SysV remain pending. Observer-only
I07 remains deferred and unchanged.

Unit 130: LINUX_OPENRC.en.md. Native reader for OpenRC runtime markers,
without librc, CLI, shell, scripts, or mkdir. Markers do NOT establish
liveness or API state; absence does NOT mean stopped. Linux 157 + 20,
MSVC 10, two SDK consumers exit 0, private fixture and FD cleanup. No real
acceptance. K03 stays E: next OpenRC installed services and runlevels.
OpenRC control, liveness, and acceptance, real runit, s6, SysV, and
integration remain pending. Do not install anything to force a test.

Explicit decision on 2026-10-30: the user chose to keep observing and move
to K03. Do not develop a custom launcher. I07 stays open and deferred
when no cause can be verified, with its criteria unchanged. This confirms
the conservative course in update 128.

Unit 129: LINUX_RUNIT_TRANSITIONS.en.md. Explicit observation after a runit
request, with a new TAI epoch for restart, polling and a deadline, no retries,
and no readiness or causal completion claim. Linux 156 + 20, MSVC 10,
two SDK consumers exit 0, private FIFOs, and no real runsv acceptance.
K03 remains open. Next: real acceptance when a supervisor is available
and the remaining OpenRC, s6, SysV, and integration work. Do not install
runit or expand autostart/I07 to obtain validation.

Update 128: “continue” after decision 127 is conservatively interpreted
as keeping the observer-only scope and advancing the recommended K03,
NOT authorizing a new autostart launcher. I07 and its criteria remain
open and deferred; an unverifiable cause stays UNKNOWN. Do not reduce
scope or mark it closed. Working order: K03, then L05 and Linux validation;
resume I07 when a suitable source or decision is available.
Unit 128: LINUX_RUNIT.en.md, explicit runit open/read/inventory/request
protocol. Linux 155 + 20, MSVC 10, two SDK consumers exit 0, private
FIFO only. Real runsv acceptance, transition confirmation, OpenRC, s6,
SysV, and integration remain in K03. No premature closure.

Consumer workflow confirmed by the user: develop the GUI first and leave
its CMake setup until the end. The library can be linked from the IDE
during development. PCInsight's internal CMake remains for building and
testing the library independently. Do not start a GUI or its build on
the basis of this preference alone.

Work on one functional unit of one subtask at a time: implement, verify
on Linux, record evidence and limits, and update the TXT tracker before
moving on. Do not open new Windows work or split activity across multiple
fronts. Check Windows compatibility only when shared changes require it.
That does not authorize new Windows features or retrying canceled UAC tests.

M02 pauses at revision 62: decoder, correlator, and index tested, with
no complete native ETW producer or public integration. Revision 63 only
reviewed documentation and code to order the remaining work. It added no
Windows lifecycle code and started no capture or elevation. M05 is also
deferred by the user's earlier decision. M01/M04/M06/M08 wait for the
Windows phase.

## Linux execution order for existing IDs

1. F03: CLOSED in revision 98 as an optional native network accounting
   provider with explicit per-process attribution, coverage, and windows.
   Evidence and limits: LINUX_NETWORK_ACCEPTANCE.en.md. Next implementation
   priority: H04. Shared sockets or partial TCP coverage do not establish
   total, causal, or exclusive network attribution. Ask for authorization
   when a live test requires broader privileges or scope; prior approvals
   were temporary and bounded, not global.
2. H04: CLOSED in revision 100. The F03 source is integrated into
   linux_network_app_history, with verified executable identity, cookie
   deltas, separate periods, and real persistence without double counting.
   Evidence: LINUX_NETWORK_APP_HISTORY.en.md. Next priority: J04.
   The original criterion remains: integrate F03 into per-app history with
   identity, availability, and persistence without double counting. Read
   HISTORY.en.md before changing semantics. Revision 99 completes
   LinuxHistoryStore (availability, merge, and retention), not network
   attribution by itself. Revision 100 checks F03/executable identity
   before and after, not PID or path in isolation; native positive and real
   round trip without double counting or gaps.
3. J04: CLOSED in revision 101. Unique network aggregation by F03 observer
   UID, with explicit namespace, coverage, and windows and no unverified
   session join. Earlier disk reporting is retained. See
   LINUX_USER_NETWORK.en.md. Missing data does not become zero, and
   repeated sessions for the same UID are not counted twice. Next: I07.
4. I07: Complete early capture and attribution of startup impact,
   short-lived processes, permissions, and end-to-end evidence. Do not
   present manual associations as verified automatic correlation.

   Revision 103 completes the optional host authorized for development,
   WITHOUT activation: STARTUP_RECORDER.en.md. That host does not close
   I07. Continue with reliable cause, external and descendant lifecycles,
   and early-capture coverage and diagnostics.

   Revision 104: the INTERNAL selected-exit sensor produced positive
   results after reaping in single- and multithreaded cases.
   STARTUP_SELECTED_EXIT.en.md. Counters, cause, coverage, and integration
   remain pending. Do not activate the recorder or close I07 on the
   strength of a lifecycle event.

   Revision 105 completes internal counters at direct release:
   STARTUP_FINAL_RESOURCES.en.md. Partial coverage, separate CPU, and I/O
   that includes waited-for children; five private cases verified. I07
   still needs baseline, integration, reliable cause, and coverage. This
   component does not change priority or authorize host activation.

   Revision 106, before K03, integrates selected final disk data into the
   recorder, observer, and file, opt-in OFF: STARTUP_DISK_TAIL.en.md.
   CPU lower bound is separate. Compatible CPU, unselected lifetimes,
   reliable cause, and durable diagnostics remain pending. Continue I07
   without activating the host.

   Revision 107 completes durable aggregate diagnostics v2; v1 remains
   unknown. STARTUP_DIAGNOSTICS.en.md. Private host/BPF cases cover round
   trips and limits. Persistence does not close I07. Continue compatible
   CPU, unselected lifetimes, and reliable cause. Do not activate the host
   or resume Windows on the strength of this work.

   Revision 108 completes and integrates compatible initial/final CPU
   for selected processes. STARTUP_SCHEDULER_CPU.en.md. POSIX clock,
   pidfd, kind guard, and fallback without mixing ticks. Native CPU-only
   high-quality data in seven private cases. I07 still needs never-selected
   lifetimes, descendants, and reliable autostart cause. No Windows work
   or host activation.

   Revision 109 completes the INTERNAL birth sensor for a selected TGID
   before first execution. Native positive after reaping and before the
   first child PID read. STARTUP_BIRTH.en.md: explicit quota/read flags
   and counters AT BIRTH only. Continue child final data, mapping,
   integration, descendants, and reliable cause, without host activation
   or global capture.

   Revision 110 completes INTERNAL access to final CPU/disk data for the
   same creator/reaper without selecting each child PID before it
   disappears. STARTUP_CHILD_FINALS.en.md: eight native scenarios, nine
   final records, and five expected missing finals. CPU uses scheduler
   group accounting and I/O includes waited-for children, without summing
   trees or inventing zero. Opt-in OFF, no public API/format or automation.
   Continue local mapping, attribution, safe integration, other reaper
   paths, descendants, and reliable cause. I07 remains E; no host activation.

   Revision 111 completes local TGID at birth qualified by the OBSERVER's
   namespace: STARTUP_BIRTH_PID.en.md. BTF upid/ns/level bounded to 0–32;
   no global PID heuristic. Flag 32/unknown preserves CPU and I/O. Nine
   native cases, 15 post-reap PIDs, and a translated private init PID 1.
   I07 remains E: continue app/exec/launch attribution and safe integration,
   other reaper paths, descendants, and reliable cause. No host activation
   or resumed Windows work.

   Revision 112 completes the PUBLIC LinuxStartupBirthObserver API and
   explicit association by each child's exact key:
   STARTUP_BIRTH_OBSERVER.en.md. No inherited app identity from the parent,
   tree I/O sums, or invented clocks/start_ticks. Nine native cases,
   15 public rows, ten final records, and five without finals. I07 remains E:
   reliable app/exec/launch and cause evidence, temporal integration with
   recorder/classifier without mixing domains or duplicating I/O, and other
   paths/descendants. Do not activate the host or resume Windows.

   Revision 113 captures the first filesystem evidence from a SUCCESSFUL
   exec, opt-in OFF. STARTUP_FIRST_EXEC.en.md. Birth/TGID+start allowlist
   and validated anonymous MM, with immutable first record and flag 64.
   Thirteen native cases, six exec results after reap, plus failure, re-exec,
   and non-leader cases. No automatic current executable, desktop, or cause.
   Continue reliable app/desktop/launch matching, cause, temporal integration,
   other paths, and descendants. I07 remains E; no host activation.

   Revision 114 completes XDG candidates from the current caller's
   filesystem, not automatic binding: STARTUP_EXECUTABLE_MATCHES.en.md.
   A unique match requires exactly one candidate and complete coverage
   and is estimated; shared, disabled, ambiguous, or unknown results
   prevent uniqueness. Literal absolute-path parser and batching. Thirteen
   native cases and six successful real exec results after an initial
   resource mismatch that did not reproduce (cause pending in N03).
   Continue receipts, reliable app/cause attribution, temporal integration,
   other paths, and descendants. I07 remains E; no host activation or Windows.

   Revision 115 fixes the exact oracle across independent captures, not
   the probe or formula. STARTUP_CAPTURE_VALIDATION.en.md: exact checks
   within the same capture, plus a second DEFAULT capture with the same
   CPU bounds, exact I/O fstat checks, and OWN replay. A 1 ns CPU rewrite
   is rejected. Native 13 + 20 positive, Linux 143 + 20, MSVC 10, SDK exit 0.
   CPU at release is not guaranteed to be the full post-switch total;
   kernel interleaving is unproven and N03 stays open. Continue I07 receipts,
   reliable cause, and temporal integration. No Windows or host activation.

   Revision 116 completes a pure impact bridge for retained children and
   explicit binding: STARTUP_BIRTH_IMPACT.en.md. A 60 s window from EACH
   birth, not boot or login. Kernel deadline and release within the window;
   high-only lower bound or unknown. No legacy coercion, tree aggregation,
   cause, or activation. Linux 144 + 20, MSVC 10, SDK exit 0, 13 native cases
   with 23 rows. I07 remains E: continue receipt and reliable app/cause
   attribution, recorder/durable/overlay integration, other paths, and
   descendants. No Windows or host activation; N03 remains open.

   Revision 117 completes recorder-owned child capture through watch_births
   and associate_birth. STARTUP_RECORDER_BIRTH.en.md. One opt-in creator,
   polling on each call, separate views, and cleanup. Save rejects before
   filesystem I/O because a birth-aware format is missing; no legacy coercion
   or silent loss. Linux 145 + 20, MSVC 10, SDK exit 0, 13 native cases with
   23 rows and positive FD checks. I07 remains E: next birth-aware format
   and overlay, receipts and reliable app/cause attribution, other paths,
   and descendants. No Windows or host activation. N03 remains open.

   Revision 118 completes birth-aware PCISTARTv3 and recorder.save:
   STARTUP_BIRTH_STORE.en.md. Raw report, window, policy, and availability
   without legacy clocks, sums, or automatic authorization. v1/v2 stay
   compatible. SDK rebuild exit 0, Linux 146 + 20, 13 native cases with
   23 rows and positive round trips. Three final MSVC checks outside
   isolation after an initial replace error 5; logs retained, N03 open.
   I07 remains E: next guarded overlay, reliable receipt/app/cause, other
   paths, and descendants. No Windows features, host activation, or
   partial credit.

   Revision 119 completes the HISTORICAL birth overlay with consent,
   context, and v3 validation: STARTUP_BIRTH_OVERLAY.en.md. No verified
   app cause, current usage, summing, or legacy clocks. Linux 147 + 20,
   MSVC 10, SDK exit 0, 13 native cases with 23 rows from v3 and positive
   FD checks. I07 remains E: NEXT receipt and reliable app/cause attribution,
   other paths, and descendants. Do not extend the projection framework
   to satisfy that criterion. N03 open; no Windows features, host or GUI
   activation, or partial credit.

   Revision 120 completes the INTERNAL pre-exec gate: file pin,
   clone3/pidfd, arming callback, NNP, and safe cancellation.
   STARTUP_GATED_LAUNCH.en.md. Linux 148 + 20, positive ELF case and four
   earlier kernel joins, SDK exit 0, MSVC 10 fallback checks. No app receipt
   or proven autostart. Next: observe SUCCESSFUL exec of the authenticated
   child, compare the file pin, and validate the desktop route. Then
   receipts, managed autostart, other paths, and descendants. Further
   projections do not satisfy the causal criterion. I07 stays E and N03
   open; no Windows features, host/login/GUI activation, or partial credit.

   Revision 121 completes INTERNAL selected successful exec: authenticated
   pidfd and file pin before release, attach with an empty allowlist and
   enroll last, keyed by TGID and boot-relative start. Compare the first
   executable tuple with the pin; no proof of app, content, or cause.
   UNUSEDZERO does not fabricate birth or CPU data. Evidence survives
   reaping and remains immutable on re-exec; absence is UNKNOWN. Five
   private cases (three successes, two failures), five ignored sentinels,
   clean FDs, Linux 149 + 20, MSVC 20 pure checks, SDK exit 0.
   STARTUP_SELECTED_EXEC.en.md. I07 stays E: next validated desktop route,
   managed receipt and reliable app/cause, other paths, and descendants.
   Do not extend projections as causal proof. No host/login/GUI activation,
   global capture, or Windows features. N03 open; no partial credit.

   Revision 122 completes the INTERNAL desktop route: the caller's own
   effective XDG snapshot, a direct Exec subset, literal argv and Path,
   and a callback gate that rechecks fields, precedence, and eligibility
   exactly true before and after. Stale data cancels and reaps the still
   gated child. Positive private desktop-to-first-exec pin after reaping,
   two canceled mutations, clean FDs, Linux 150 + 20, MSVC 10 pure checks,
   SDK exit 0. STARTUP_DESKTOP_LAUNCH.en.md. A MANUAL launch does not prove
   automatic cause, current file, app identity, or contents. Next: managed
   receipt honestly distinguishing manual and automatic causes, integration,
   other paths, and descendants. I07 E/N03 open; no new public API/format,
   host/login/GUI activation, global capture, or Windows features.

   Revision 123 completes the INTERNAL LinuxDesktopLaunchSession and
   composite MANUAL receipt: double consent, one-shot, no fallback, full
   route key, pin and outcome, exec quality and UNKNOWN; no allocation
   after release. Four private cases cover matching after reap, two
   failures with unknown results, and stopping observation with the child
   still alive (no kill/reap; caller releases and waits), plus replay and
   clean FDs. Linux 151 + 20, MSVC 10 pure checks, SDK exit 0.
   STARTUP_DESKTOP_RECEIPT.en.md. Next: integrate receipt, recorder, and
   resources, and establish a trustworthy managed-autostart route without
   activating it. Never promote manual to automatic cause. I07 E; other
   paths and descendants pending, N03 open, no new API/format/Windows features.

   Revision 124 completes the INTERNAL bridge from manual receipt to birth
   resources. Creator hook BEFORE launch, exact kernel key, boot/capture,
   namespace, first file, staged copy, and read-only validator.
   finish/poll/close/detach without waiting. Four private cases with
   CPU 22,418,570 ns and disk 1,155,072 B after reaping, two unattributed
   pre-exec errors, live finish UNKNOWN, caller wait, raw unbound data,
   and clean FDs. Linux 152 + 20, MSVC 10 pure checks, SDK exit 0.
   STARTUP_DESKTOP_RESOURCES.en.md. Next recorder, durability, and trusted
   managed autostart without activation; manual is not automatic cause.
   I07 E; other paths and descendants pending, N03 open, no new format,
   public API, or Windows feature.

   Revision 125 completes INTERNAL atomic PCIMANv1: raw manual receipt,
   births, window, policy, history only, and reprojection. Shared legacy
   I/O without changing the public format. Strict Linux 0600/current-owner
   checks BEFORE payload; rejection when the mount ignores mode preserves
   the old target. Four private cases with 0600 round trips, raw unbound
   data, UNKNOWN, and clean FDs; Linux 153 + 30, SDK exit 0, MSVC 10 manual
   and two legacy checks. STARTUP_MANUAL_STORE.en.md. Next: recorder
   integration and trustworthy managed autostart without activation.
   Manual is not automatic cause. I07 E; other paths and descendants
   pending, N03 open. No new public API, Windows features, or IDs.

   Revision 126 completes the INTERNAL bridge to the EXISTING recorder:
   manual-only, own source, canonical finalize/save to PCIMAN, no GIO scan
   or mixing of legacy tables; raw data stays unbound. Four private cases
   with the REAL recorder: positive, failure, live UNKNOWN, finish/save/
   reload, 0600, and clean FDs. Linux 154 + 30, MSVC 20 manual/legacy checks,
   SDK exit 0. STARTUP_RECORDER_MANUAL.en.md. Next: trusted managed-autostart
   context without activation and other paths, reapers, and descendants.
   I07 E/N03 open; no new API, format, Windows feature, or ID.

   Revision 127 AUDIT: automatic OS cause cannot be established from
   metadata, login, or a host we control. STARTUP_EARLY_DECISION.en.md.
   Role decision pending at this point: stay observer-only and defer I07
   cause while proceeding with K03, or develop an opt-in launcher privately
   without activating it. Do not change order, criteria, or closure before
   an answer. No new tests, code, percentage, or IDs. Revision 126 remains
   historical evidence; I07 E and N03 open.
5. K03: Complete native adapters for alternative init systems OpenRC,
   runit, s6, and SysV one at a time, with inventory, control, and tests.
   Audit 145: inventory, views, and Monitor selection implemented. K03
   was deferred in E because OpenRC/SysV control was not authorized at
   that point and real acceptance was unavailable. Do not add projections
   to work around that blocker; advance L05/Linux without closing K03.
   LINUX_K03_AUDIT.en.md retains the matrix and evidence. Do not count
   discovered files as running services or execute external tools to
   collect data.
6. L05: Complete the Linux side of the helper and polkit authorization,
   including rejection tests, a closed scope, and consent, without
   automatically installing or activating privileged components. Omitting
   the UAC side does not close the shared criterion.
7. B08/G07/N02: Run remaining validation when the environment or hardware
   can supply real firmware, GPU, and distribution evidence. Items remain
   pending or blocked where appropriate. Fixtures or Windows results do
   not validate physical Linux. Ask the user for evidence or an environment
   when that is the real obstacle.
8. A05/N01/N03: Complete the Linux side of the contract, packaging, CI,
   and field/action audit. Do not claim remote CI from a local build or
   close shared criteria that still require Windows. Document external
   dependencies.

This is an execution order, not a new list of subtasks or parallel work.
If F03 needs an external decision or permission before it can proceed,
report the blocker and ask for direction rather than spending prompts on
unchanged tests. State real limits plainly instead of hiding them or
substituting proxies.

## Scope and tracking

Linux-first keeps the original constraint: use the kernel, firmware, or
driver ABI, or libraries already present. Do not require external telemetry
apps or install packages except as an authorized last resort. Report exact
availability per field; do not promise identical data across kernels,
firmware, or drivers.

75 active tasks, 3 excluded X. C57/E11/P5/B2: 57/75=76.0% closed,
18/75=24.0% open. The priority change does NOT change states or the
denominator or grant partial credit. Do not invent an estimated Linux
percentage incompatible with this tracker. Windows tasks remain in the
final scope but are deferred.

Latest implementation and tests at this point: revision 62, Linux 115/115
and Windows 136/136 per compiler. Revision 63 runs no new suites and does
not reinterpret those results as Linux progress. ClickUp mirrors this
checkpoint; it is not the authoritative tracker.

Checkpoint 64: F03 private SELF lifecycle completed and tested on Linux
with no new Windows implementation. Linux 116/116, successful private
native test, and installed consumer exit 0. See LINUX_NETWORK_SESSION.en.md.
F03 remains in progress for netns, joins, multiple actors, coverage,
windows, and integration. Count unchanged. Continue F03 next without
opening another front.

Checkpoint 65: Socket namespace checks completed for SELF with a declared
cookie and sticky rejection of mixed namespaces. Linux 116/116 and positive
native WSL result, without setns, unshare, or Windows work.
LINUX_NETWORK_NAMESPACE.en.md records the limits. F03 remains E for dynamic
buckets, multiple actors, joins, coverage, windows, and integration. Count
unchanged. Next: identity and scope for the F03 producer.

Checkpoint 66: SELF capture connected to the reducer and rates through an
internal sampler, with typed initial kernel domains and a one-row/cookie
scope. Linux 117/117 and positive native WSL results for four rates and UID,
idle zero, fork, and restart. LINUX_NETWORK_WINDOW.en.md. F03 remains E for
multiple actors, buckets, joins, coverage, and public integration. No
Windows work or count change. Next: procfs identity for the F03 producer.

Checkpoint 67: Direct SELF kernel/procfs association completed as a
separate method, with two fresh BPF identities and stable local context at
both endpoints. Linux 119/119 and positive WSL PID 390/kernel 4965,
with a rate oracle, fork/restart, and no FD leak.
LINUX_NETWORK_SELF_JOIN.en.md records limitations. No Windows work or
count change. F03 remains E for multiple actors, buckets, external actors,
coverage, and public integration. Next: capture and identity for external
actors, using private tests and bounded permissions. Do not extend prior
authorization to system-wide capture.

Checkpoint 68: Private start_selected mode with declared kernel token
and cookie, distinct from SELF. Positive private cooperative child on WSL,
parent excluded, sticky rejection of wrong identity, pidfd exit, and
cleanup. Linux 119/119, no Windows work. LINUX_NETWORK_SELECTED.en.md
notes that it does not authenticate arbitrary actors. F03 remains E;
count unchanged. Next: reliable actor discovery and revalidation, with
private tests and bounded permission, not system-wide capture.

Checkpoint 69: Kernel identity obtained from pidfd without a token
reported by the target, using a temporary probe filtered to the thread
making the FD -1/EBADF permission query. Positive private WSL result
PID 554/kernel 1640, with capture, negative cases, cleanup, and mask
checks; Linux 120/120. LINUX_NETWORK_PIDFD.en.md records limitations, a
temporary effect on the caller's mask, and no ptrace attach, signals, or
target stop. F03 remains E; count unchanged, no Windows work. Next: join
and revalidate pidfd against the selected procfs process and context.

Checkpoint 70: Selected pidfd/procfs/boot/context association completed
with checks at both endpoints. Two fresh kernel tokens, Pid projected from
the calling thread's FD table, and PID/start_ticks/status/namespace
revalidation with separate domains. Authorized private WSL child:
PID 2513/kernel 6353, positive and negative cases, no FD leaks. An
unprivileged CLONE_FILES worker verifies a table distinct from the leader's.
Linux 122/122 without skips, 14 network tests repeated 20 times, installed
consumer exit 0, and no build warnings. LINUX_NETWORK_PIDFD_JOIN.en.md
documents no atomicity, ABA protection, future-lifetime guarantee,
automatic external cookie, or global coverage. F03 remains E; count
unchanged, no Windows work. Next: connect the associated actor to windows
and rates with revalidation and baseline resets. Do not open another
front or extend authorization to global capture.

Checkpoint 71: Selected windows completed with fresh endpoint joins and
an owned duplicate pidfd. Baselines reset on failure, exception, or
change. One-actor/cookie coverage stays separate from SELF/global, with
a separate actor producer, shared epoch, estimated rates, and partial UID
totals. Authorized private WSL PID 402/kernel 913: four rates and UID,
closed caller FD, idle zero, fork/restart/exit, and no FD leaks. Linux
123/123 without skips, 15 network tests repeated 20 times, installed
consumer exit 0. Initial relink warnings from make about DrvFS timestamps
were preserved; the repeated build had no warnings.
LINUX_NETWORK_SELECTED_WINDOW.en.md retains endpoint, ABA, future
lifetime, declared cookie, hard deadline, and same-FD-table limits. F03
remains E; count unchanged, no Windows work. Next: coverage/buckets and
multiple actors with clear limits before public integration. Private
tests only; no broader global capture.

Checkpoint 72: Fixed multi-actor collection for 1–16 actors completed.
Deduplication in local and kernel domains, common observer and boot, and
rollback of ALL baselines on failure, exception, or change. Windows are
independent, not a simultaneous snapshot or common UID sum. Fixtures:
two actors, 2 s/3 s intervals, second-row failures, and recovery. Two
private WSL children: four positive oracles per actor, cross-traffic and
parent excluded, fork, exit, and cleanup. Linux 124/124 without skips,
16 network tests repeated 20 times, installed consumer exit 0, and clean
final build after fixing initial indentation (log retained).
LINUX_NETWORK_MULTI.en.md records limits. F03 remains E; count unchanged,
no Windows work. Next: bounded common time window and UID aggregation
before coverage, buckets, and public integration. Private tests only;
do not expand global capture.

Checkpoint 73: Common nominal window and partial UID sums completed.
All joins checked before and after, scan <=1 s, commit afterward, 1–16
selected actors deduplicated by TGID/start, retained brackets, estimated
rates, availability and overflow checks. Not an atomic snapshot.
Failure, exception, identity change, or mode change resets all baselines.
Fixtures cover one/two UIDs, 12 failures, overflow, limits, and modes.
Two private WSL children yielded four positive actor/UID oracles, and
native selected/independent regression also exited 0. Linux 124/124
without skips, 16 network tests repeated 20 times, installed consumer
exit 0, clean build, and the same CTests extended.
LINUX_NETWORK_COMMON.en.md retains limits. F03 remains E; count unchanged,
no Windows work. Next: coverage contract and selected public opt-in
provider without fabricated totals. Private tests only; no broader capture.

Checkpoint 74: Selected public provider completed. LinuxProcessNetworkSampler
is available in an installed header, with movable Pimpl, opt-in start,
sample/stop, and a report with typed coverage, estimated support, separate
local/kernel IDs, partial initial UID, availability, and time brackets.
Pure projection, 34 negative cases, and a PUBLIC API WSL test with two
private children, four positive actor/UID oracles, move/fork, closed
caller FDs, and cleanup. Linux 125/125 without skips, 17 network tests
repeated 20 times, installed consumer using the new header exit 0, and
clean build. ADDITIVE API: existing layouts and ABI unchanged. No
automatic Monitor integration, legacy totals, new Windows work, GUI,
history, or dependency. LINUX_PROCESS_NETWORK.en.md records limits.
F03 remains E; count unchanged. Next: opt-in selected report in the
general Monitor flow and coverage/bucket policy without fabricated totals.
Private tests only; do not extend authorization to global capture.

Checkpoint 75 (2026-10-22): Selected report integrated into Monitor/Sampler.
Flag defaults false, setup is authorized separately, opt-out/stop/worker
shutdown close it, and restart does not reauthorize. Two private WSL
children and four oracles; positive worker lifecycle, move, fork, rearm,
and cleanup. Linux 126/126 without skips, 18 network tests repeated 20
times, final installed consumer exit 0, and verified clean build.
SystemSnapshot/SampleOptions 0.x layout changes: rebuild ALL consumers.
Focused MSVC tests passed 2/2, but the general consumer returned
0xC0000409 after the new checks without a proven cause. N03 risk remains
pending; no claim of full compatibility or a new Windows front.
LINUX_PROCESS_NETWORK_MONITOR.en.md retains details. F03 remains E; count
unchanged, with dynamic selection, namespace buckets, and general
coverage pending. Next: remove fixed manual scope without fabricating
totals. Private tests only; no global capture.

Checkpoint 76 (2026-10-22): Explicit update in the public provider and
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

Checkpoint 77 (2026-10-22): Sampler update with an active worker and
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

Checkpoint 78 (2026-10-22): Public provider with declared namespace
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

Checkpoint 79 (2026-10-22): Declared buckets integrated into
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

Checkpoint 80 (2026-10-22): PRIVATE observed engine without an input
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

Checkpoint 81 (2026-10-22): INTERNAL selected observed windows with owned
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

Checkpoint 82 (2026-10-22): INTERNAL observed common window for 1–16 actors,
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

Checkpoint 83 (2026-10-22): PUBLIC selected observed provider, opt-in,
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

Checkpoint 84 (2026-10-22): Observed mode integrated into Monitor/Sampler
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

Checkpoint 85 (2026-10-22): PUBLIC read-only PID/start owner, 0–16 complete
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

Checkpoint 86 (2026-10-22): PUBLIC read-only inventory from pinned procfs
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

Checkpoint 87 (2026-10-22): Revision 87, F03 E->E: STRUCTURAL capacity 64,
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

Checkpoint 88 (2026-10-22): Revision 88, F03 E->E: gated session pidfd query
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

## Checkpoint 99: H04 P->E, LinuxHistoryStore complete, 2026-10-24

Persistence is ready for LinuxAppHistoryEntry, including value, Support,
and note, in a separate format. Cumulative merge does not add replays;
periods are unique at millisecond precision. Decreases, loss of known
values, corruption, exceeded bounds, and other formats are rejected.
Save replaces rather than implicitly merging. Retention keeps whole
periods; reset unlinks the file, not directories. No automatic I/O.
Linux 134/134 plus 30 focused checks, installed consumer exit 0, and
a REAL CPU round trip with network and notification fields still absent.
Twenty focused MSVC checks passed outside the sandbox. Inside, temporary
replacement failed with AccessDenied 5 for new and existing files; logs
retained. Initial fixtures corrected, assertions active, N03 unresolved.
No existing layout/ABI changes, new CPU or Windows work, or eBPF capture.
LINUX_HISTORY_STORE.en.md; logs work/linux-history-store-*.
H04 remains open: executable attribution and windows before and after,
F03 integration, native positive, and a REAL network round trip. Do not
merge independent periods by path, reconstruct gaps, or sum absolute frames.
C58/E11/P4/B2, 75 active/3 X, 77.3% closed/22.7% open, no partial credit.
Next: H04. Mirror 99 confirmed, comment 90140272938633; TXT authoritative.

## Checkpoint 98: F03 accepted, real view and SELF, 2026-10-24

F03 E->C: the existing criterion for an optional native provider with
attribution, windows, and declared support was met. SELF fixed the
PID==caller case that does not emit the external hook: full local
identity, verified pidfd, and two fresh kernel reads, without bypassing
external permissions. REAL WSL /proc: 30/30 strict, zero excluded, three
paired snapshots, UDP delta 97, rates and clean FDs, PASS. Temporary
permission confirmed, trials under 10 s, probes removed, no permanent
changes. Linux 133/133 plus 30 focused checks, MSVC 10 join checks,
installed consumer exit 0. Private 17-actor TCP/UDP, IPv4/IPv6, updates,
gap, and fork regression passed. EINVAL and non-live actor failures were
retained. Churn still fails closed and requires authorized refresh.
No universal total or closure of N02/N03/H04/J04.
LINUX_NETWORK_ACCEPTANCE.en.md; work/network-visible-*.
C58/E10/P5/B2, 75 active/3 X, 77.3% closed/22.7% open. Next H04: read
HISTORY.en.md before modifying semantics or persistence. Linux build
recheck exited 0 without warnings after a clock-skew warning. Mirror 98
confirmed, comment 90140272937903; TXT authoritative.

## Checkpoint 97: Capacity 1,024, batching, and native 65, 2026-10-24

Scaling unit complete: structural capacity of 1,024 actors and 16,384
buckets, RAW64 per batch, disjoint startup and admission batches, retention
without restart, and readmission without counting gaps. Owner, inventory,
policy, controller, projection, Monitor, and worker share a limit.
More than 1,024 eligible actors rejects ALL. No unlimited support, 1 Hz
guarantee, RLIMIT change, or automatic capture. FDs: N+6+8G, plus N for
the owner; G can reach N with fragmentation after churn. Rebuild ALL
for the constexpr change; no new layouts. Fixtures cover 65/129/1,024
actors, 16,384 buckets, and failures in batches 2 and 16. Native 65 used
ONLY private actors through the PUBLIC API and restricted admission:
87/152 FD counts, UDP, rates, UID, unchanged refresh, cleanup, and
17-actor regression passed. Linux 133/133 plus 70 focused checks,
MSVC 70 focused checks, installed consumer exit 0. Initial consumer
assertion expected the old 1,024 buckets; updated to 16,384, log retained,
checks active. No new C++ warnings; expected D9025; N03 still open.
LINUX_NETWORK_SCALE.en.md; work/network-scale-*.
Next F03: validate coverage and a real view with appropriate permission.
Temporary WSL permission was requested but NOT received in this revision.
Do not use the earlier private permission to capture unrelated processes.
H04/J04 follow. C57/E11/P5/B2, 75 active/3 X, 76.0% closed/24.0% open,
no partial credit. Mirror 97 confirmed, comment 90140272937319;
remote history unchanged, TXT authoritative.

## Checkpoint 96: Monitor/Sampler admission and provenance, 2026-10-24

Completed Monitor refresh and Sampler start/refresh with consent EVERY
time, strict default, and mutually exclusive modes. SystemSnapshot
pairs last-refresh metadata with network data, epoch, and exclusions;
no new census or global host claim. Failure closes both, the worker
continues, and flag/restart does not rearm. Concurrent refresh/history,
retention/readmission without counting gaps, move, and opt-out verified.
SystemSnapshot 0.x layout extended; REBUILD ALL. Linux 133/133 plus 100
focused checks, installed consumer exit 0; MSVC 40 focused checks.
Native pass used ONLY two private actors plus a synthetic marker,
object-restricted enumeration, and public procfs/pidfd/BPF. A complete
/proc view was NOT authorized or validated. Initial MSVC assertion failure
retained, cause not isolated; rebuild and repetitions passed without
weakening assertions. N03 consumer issue from 75 remains open. Temporary
WSL clock-skew warnings recorded; verification rebuild exit 0 without
warnings. LINUX_NETWORK_ADMISSION_MONITOR.en.md; work/network-admission-monitor-*.
Next: scale beyond 64 eligible actors, coverage, and authorized real
validation. H04/J04 criteria unchanged and follow. C57/E11/P5/B2,
75 active/3 X, 76.0% closed/24.0% open, no closure, ID, or partial credit.
Mirror 96 confirmed, comment 90140272936245; remote history unchanged,
TXT authoritative.

## Checkpoint 95: Opt-in partial policy and provenance, 2026-10-22

F03 E->E. Pure plan validates the ENTIRE inventory, separating candidate
readiness from all_encountered_verified quality and consent. strict_all
defaults on EVERY refresh. Explicit verified_only excludes ONLY
well-formed unverified rows with PID, state, and reason. Malformed,
estimated, or incomplete input is not hidden. More than 64 eligible
actors rejects ALL; no first 64, truncation, or rotation.
Change reports counts, policy, exclusions, complete_encountered_set,
epoch, and times. sample_with_admission pairs the LAST refresh with
same-epoch network data, not a new census or host-wide view. Raw sample
notes PARTIAL, without invented rows, rates, or UID totals. All excluded
means visible >0 and capture off, not an empty visible set or measured
network zero. Same PID/start preserves epoch/rates even when reasons
or policy change; provenance updates. Runtime owner/BPF/quality/context
failure closes ALL, without retrying with filters.
AdmissionChange 0.x layout extended: REBUILD library and consumers;
SystemSnapshot and network snapshot unchanged. Fixtures pass for
64/65/67 with three excluded, all excluded, 20 corruptions, controller,
failures, and exceptions. Native ONLY two private actors plus a marker
without another party's PID: visible 3, verified 2, excluded 1, two
captured actors, epoch/baseline, 26 FDs, gap/provenance, later strict
default closes ALL, fork/restart/empty/cleanup passed. NOT global.
Linux 132/132 without skips plus 100 focused checks, installed consumer
with new API exit 0; MSVC 40 focused, no new C++ warnings, expected
D9025, N03 unresolved. Initial missing-initializer warning corrected,
log retained. LINUX_NETWORK_ADMISSION_POLICY.en.md; network-policy logs.
Next: Monitor/Sampler admission/provenance, scale beyond 64 eligible
actors, and real-view validation with permission; H04/J04 criteria
unchanged. C57/E11/P5/B2, 75 active/3 X, 76.0% closed/24.0% open,
no closure, ID, or partial credit. Mirror 95 confirmed, comment
90140272317289; list read, history unchanged, TXT authoritative.

## Checkpoint 94: Standalone strict admission, 2026-10-22

F03 E->E. New LinuxVisibleProcessNetworkSampler with refresh_visible(bool),
sample, stop, and move connects inventory, owner, and the PUBLIC shared
observed provider. Consent on EVERY refresh. Construction does not
enumerate or capture; sample does not enumerate or rearm; no polling or
thread. ALL verified and at most 64, or reject ALL without hidden filtering,
truncation, or rotation. Empty stops capture with epoch 0; it does not
mean the host is empty. Failed authorized refresh or sample error stops
ALL, with no old or partial selection. Current boot/procfs/namespace/caller
checked before and after. Inventory plus commit has a cooperative 5 s
budget, not a hard deadline. Identical or reordered PID/start set preserves
rates/epoch without hidden update or sample. Changes and PID reuse use
shared update, retaining provenance and excluding gaps. Owner adds N FDs;
2N+6+8G persistent. Fixtures pass for 64/65, unknown/stale/context, PID reuse,
failures/exceptions at EVERY operation, sample, and fork.
Native test used ONLY two private children with restricted enumeration
and native procfs/pidfd/BPF: AB->unchanged AB->B->AB, unchanged rates/epoch,
gap and provenance, FD counts 18->16->26, fork leaves parent intact,
unknown closes ALL, fresh restart, empty, and cleanup. Do NOT call
refresh_visible(true) for the whole session on the earlier BPF permission:
it was for private actors ONLY. No positive full-view validation.
Final Linux 131/131 plus 80 focused checks; installed consumer with new
header/API exit 0. New MSVC fixture repeated 20 times; no new C++
warnings, expected D9025, N03 unresolved. Initial target without CMake
regeneration and WSL skew of 0.75 s retained; final build passed.
LINUX_NETWORK_ADMISSION.en.md; work/network-admission-*.
Next: practical policy for large or inaccessible views, coverage,
authorized real validation, and Monitor/Sampler admission; then H04/J04.
C57/E11/P5/B2, 75 active/3 X, 76.0% closed/24.0% open, no closure, ID,
or partial credit. Mirror 94 confirmed, comment 90140272312863;
list read, history unchanged, TXT authoritative.

## Checkpoint 93: Public shared observed backend, 2026-10-22

F03 E->E. PUBLIC observed_namespace_buckets branch now uses authenticated
shared capture. Default strict and declared modes keep the prior provider.
Stop covers both branches; move transfers Pimpl/mode. Same validated
projection, no new signature, layout, API, dependency, or config, and
no capture on construction. Monitor/SystemSnapshot, Sampler worker,
and SelectionOwner use the existing public paths. Native PUBLIC 17:
N+14 FDs, rates/UID, empty metadata, owner clear, parent excluded, and
cleanup passed. Standalone/Monitor/owner AB->B->AB: 16->15->24 owned FDs,
excluded gaps, retained provenance, epoch/rates, move, fork, exit/recovery,
stop/restart, and opt-out/flags not rearming passed. Worker concurrent
update/history/epoch/absence/final FDs passed, as did native PUBLIC strict
and declared modes. Final Linux 130/130 without skips plus 60 focused,
installed consumer exit 0. MSVC 3/3 plus 60 focused checks passed, no new
C++ warnings, expected D9025. Initial builds collided because the
assistant used the same directory, leaving truncated or missing ranlib
output; logs retained, rebuild serialized. Not a code failure or N03 fix.
LINUX_NETWORK_SHARED_PUBLIC.en.md; work/network-shared-public-*.
Next: general visible-process policy/admission without truncating beyond
64 or mixing identity with unknown, then H04/J04. No authorization for
global capture. F03/H04/J04 open, C57/E11/P5/B2, 75 active/3 X, 76.0%
closed/24.0% open, no partial credit. Mirror 93 confirmed, comment
90140272305719; list read, history unchanged, TXT authoritative.

## Checkpoint 92: Authenticated shared update, 2026-10-22

F03 E->E. Internal LinuxSharedSelectedNetworkSampler::update_selection
complete. Consent every time; shape 1–64 checked without mutation;
ALL joins before and after, unique kernel identities, and boot/observer
checks. Runtime failure or exception closes ALL; new pidfds managed by
RAII. LinuxSharedNetworkSet uses disjoint batches, no recycled slots,
and removals before additions. Retained actors do not restart and keep
provenance. Additions/readmissions start fresh without counting gaps.
Epoch and ALL baselines reset even on reorder. Empty batches close;
1<=G<=N<=64, native cost N+6+8G FDs. Not a constant three probes after
churn or an atomic cut across batches. Total scan <=1 s, per-batch
consistency and sticky quality preserved. Native 17/restart 16/update
16->15->16 with removed actor still sending: TCP/UDP4/6 oracles, four
rates and UID sum, reorder, exactly eight additional FDs for new batch,
children and FDs cleaned. Fixtures pass for 80 updates, 64 batches/1,024
buckets, rejection of 65, 64->1, failure/exception of every operation,
quality and span. Linux 130/130 plus 40 focused, installed consumer exit 0;
MSVC 40 focused passed without new C++ warnings, expected D9025, N03
unresolved. LINUX_NETWORK_SHARED_UPDATES.en.md; work/network-shared-update-*.
Next: activate and verify PUBLIC observed backend, Monitor, Sampler,
and owner; automatic admission remains pending. Positive internal
projection does NOT establish activation. F03/H04/J04 open,
C57/E11/P5/B2, 75 active/3 X, 76.0% closed/24.0% open, no partial credit.
Mirror 92 confirmed, comment 90140272299452; list read, history unchanged,
TXT authoritative.

## Checkpoint 91: Safe shared removal, 2026-10-22

F03 E->E. LinuxSharedNetworkSession::retain accepts only exact active
subsets. Removes HASH entries without clearing or recycling ARRAY
slots or restarting probes. An invocation already admitted can only
finish in a retired record that is not published. Global writer/sequence
includes retired actors, and sticky GLOBAL quality still governs.
Retained checkpoints keep identity. Deletion failure or exception closes
all, as does an empty set; invalid shape/consent leaves state intact.
Fork does not mutate the parent's map. Fixtures pass for 64->63, full
capacity, suspended event, regression, and failures 1 and 2. Native
17->16->1 with all still sending: retained actors keep TCP/UDP provenance
and eight FDs, retired actors excluded, clean restart/children/FDs.
Linux 130/130 without skips; MSVC 2/2 repeated 20 times each.
LINUX_NETWORK_SHARED_RETENTION.en.md; work/network-shared-retain-*.
Not an authenticated update: additions/readmission, epoch/rate reset,
and composition without restarting retained actors are missing; then
public backend and automatic admission. F03/H04/J04 open, C57/E11/P5/B2,
75 active/3 X, 76.0% closed/24.0% open, no partial credit. Final Linux
130/130 plus 40 focused, installed consumer exit 0; MSVC 40 focused.
Mirror 91 confirmed, comment 90140272294875; list read, history unchanged,
TXT authoritative.

Checkpoint 89 (2026-10-22): Revision 89, F03 E->E. Completed the INTERNAL
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

Checkpoint 90 (2026-10-22): Revision 90, F03 E->E. Completed authenticated
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
