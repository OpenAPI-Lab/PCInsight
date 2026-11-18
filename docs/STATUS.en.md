# Actual implementation status

For the scope of the public release, start with [Support status](SUPPORT_STATUS.en.md).
The checkpoints below are historical evidence. They do not establish that
all fields or providers are complete, and they do not represent remote CI.

This document distinguishes fields defined by the API from metrics that
are actually implemented.

Checkpoint 172: [Linux hardware verifier](LINUX_HARDWARE_EVIDENCE.en.md)
tested against the installed 0.2.0 SDK. Synthetic and WSL tests each
passed 10/10, and the Linux regression suite passed 200/200. N02/G07
remain blocked pending physical hardware evidence. Official count:
60/75=80%. ClickUp mirror: 90140278768612.

Checkpoint 171: [Firmware evidence](LINUX_FIRMWARE_VIRTUALIZATION_EVIDENCE.en.md),
B08 E→E. WSL had none of the OEM/KVM paths checked. Focused tests
passed 10/10, with no positive physical result or new code.
Official count: 60/75=80%. ClickUp mirror: 90140278765329.

Checkpoint 170: [Linux CI](LINUX_CI.en.md), N01 P→E. Local rehearsal of
the Release verification stage: Linux 200/200, delivery 12/12, marker
1/1, and controlled rejection cases. Windows unchanged; no push or
remote CI. Official count: 60/75=80%. ClickUp mirror: 90140278762897.

Checkpoint 169: [SDK policy](SDK_VERSION_POLICY.en.md) and the 54-file
Linux manifest verified. Modified digest rejected; marker 1/1, baseline
3/3, Linux 200/200. A05 stays E with no declaration of a 1.0 ABI or
library or versioning changes. Next: N01 Linux CI review. Official
count: 60/75=80%. ClickUp mirror: 90140278761934.

Checkpoint 168: [Linux baseline](LINUX_ABI_BASELINE.en.md), A05 E→E.
The installed SDK probe checked 15 signatures/types and 28 sizes and
alignments under GCC 13.3.0/libstdc++ 20240904. Three tests, 30 runs,
and Linux 200/200. No library change or 1.0 ABI declaration. Official
count: 60/75=80%. ClickUp mirror: 90140278760856.

Checkpoint 167: [SDK header marker](SDK_HEADER_COMPATIBILITY.en.md),
A05 P→E. The Linux/MSVC SDK checks matched and exited 0, and the
alternate symbol was rejected after compilation. The older Linux
archive was rejected. Direct g++ build exited 0; Linux passed 200/200.
SDK 0.2.0: no layout, vtable, or format changes or 1.0 ABI guarantee.
Official count: 60/75=80%. ClickUp mirror: 90140278759496.

Checkpoint 166: [Linux delivery audit](LINUX_DELIVERY_AUDIT.en.md)
against R01–R20. Dedicated SDK: 53 files, exact 0.2.0 accepted and 0.2.1
rejected, six examples with 12/12 checks, and direct g++ linking exit 0.
Linux 200/200. N03 remains open overall. Next: Linux A05 compatibility
review, not a 1.0 ABI declaration. Official count: 60/75=80%.
ClickUp mirror: 90140278755570.

Checkpoint 165: [Linux Services requirements](LINUX_SERVICES_REQUIREMENTS.en.md)
traced per provider. Read-only SDK example uses systemd inventory
without fallback and queries details. Linux 200/200 plus 20 focused
checks, ten portable MSVC checks, and 25 links. No library/ABI change.
N03 stays E; Linux delivery audit is next. K03 and L05 remain open;
SysV control still needs authorization. Official count: 60/75=80%.
SDK consumer self-test and sample exited 0, with 199 systemd rows
and no fallback. ClickUp mirror: 90140278753524.

Checkpoint 164: [Linux Users and Sessions](LINUX_USERS_REQUIREMENTS.en.md)
traced with separate CPU/RAM, disk, and partial network scopes. SDK
example performs no actions or capture. Linux 198/198 plus 20 focused
checks, ten portable MSVC checks, and 20 links. No library/ABI change.
N03 stays E; Services is next. Official count: 60/75=80%. SDK consumer
self-test and sample exited 0. ClickUp mirror: 90140278752093.

Checkpoint 163: [Linux Startup Apps](LINUX_STARTUP_REQUIREMENTS.en.md)
traced with configuration, eligibility, and impact kept separate.
Inventory and Properties example does not display raw values or perform
actions. Linux 196/196 plus 20 focused checks, ten portable MSVC checks,
and 29 links. I07 remains open and ABI unchanged. N03 stays E; Users
is next. Official count: 60/75=80%, 15/75 open. SDK consumer self-test
and sample exited 0. ClickUp mirror: 90140278750348.

Checkpoint 162: [Linux App History](LINUX_APP_HISTORY_REQUIREMENTS.en.md)
traced with an SDK example that separates CPU, network, and notifications
and does not treat legacy counters as measured. It does not activate
BPF or the observer, or write history. Linux 194/194 plus 20 focused
checks, ten portable MSVC checks, and 16 links verified. N03 stays E;
Startup is next. No library, ABI, or format changes. Official count:
60/75=80%, 15/75 open. Installed SDK consumer self-test and sample
exited 0. ClickUp mirror: 90140278748341.

Checkpoint 161: [Linux Processes/Details and actions](LINUX_PROCESS_REQUIREMENTS.en.md)
traced with a read-only SDK example, SELF output, and a separate partial
network report by bucket/lane. Run new task, End task, and Efficiency
are documented, not invoked by the example. Linux 192/192 plus 40 focused
checks and 20 portable MSVC checks. N03 remains open; App History is next.
No ABI/library change or inferred SysV permission. Official count:
60/75=80%, 15/75 open. SDK consumer self-test and SELF sample exited 0.
ClickUp mirror: 90140278746290.

Checkpoint 160: [Linux Performance contract](LINUX_PERFORMANCE_REQUIREMENTS.en.md)
completed: public fields, units, support, tests, and CPU/RAM/disk/network/GPU
limits, with the 60 s window and Windows differences documented explicitly.
The SDK-only linux_performance.cpp example shows help by default, has a
pure --self-test and a two-read --sample, and performs no actions or
capture. Verified unknown versus 0/false, NaN, permissions, parent
relationships, empty zram, and engines. Linux 190/190 plus 20 focused
checks, MSVC 10; installed SDK consumer relinked, self-test/sample exit 0,
65 links checked, no code warnings. Logs: work/performance-contract-*.
Production library and layouts unchanged. N03 stays E for traceability
of the remaining blocks/actions, delivery and audit, and historical risks
NOT fixed here. G07/N02/B08 and real K03 acceptance remain pending; no
explicit SysV permission. L05/I07/Windows unchanged. No installation,
elevation, real services, eBPF, or CI claim. C60/E10/P3/B2 plus 3 X:
80% closed/20% open. TXT authoritative; no new IDs or partial credit.
Mirror 160 confirmed: comment 90140276895356; remote 60% title is historical.

Checkpoint 159: [Public OpenRC API](LINUX_OPENRC_CONTROL_PUBLIC.en.md)
installed: LinuxOpenrcService open/request/close and Runner wait,
terminate_runner, and diagnostic PID. Selection retains FDs, metadata,
directories, and UID/GID context, is revoked when they change, requires
consent, and is consumed when the child is returned. No reopening a
different target or automatic retry. Exit/ACK is NOT service state.
Off by default; reader, Monitor, and SystemActions unchanged. Linux
188/188 plus 50 OpenRC checks, 20 portable MSVC checks, three SDK consumers
relinked with exit 0 (new consumer used only the installed SDK), verified
FD handling, cleanup, and final builds. Initial D9025 from UNDEBUG was
expected; no final code warnings. Private model, NOT real OpenRC. No
tool installation, elevation, real services, or CI claim. K03 stays E:
public integration complete; real acceptance pending. Read-only PATH
probe did not find runit/s6/OpenRC; this does not establish global absence.
SysV permission requested, NOT yet granted. L05 deployment deferred,
I07/Windows unchanged. C60/E10/P3/B2 plus 3 X: 80% closed/20% open;
TXT authoritative. Mirror 159 confirmed: comment 90140276888517;
remote 60% title is historical.

Checkpoint 158: [Internal OpenRC control](LINUX_OPENRC_CONTROL.en.md)
implemented, opt-in and OFF by default, only for explicit start/stop/restart.
Pinned ELF runtime, protected and revalidated script and paths, closed
environment and FD set, gating/pidfd, ACK and waitid(P_PIDFD), and exit,
timeout, and explicit cancellation of the runner only. It does not
establish live service state or stop the service on cancellation. Tests
used a private model, never real OpenRC or services. Linux 187/187 plus
40 OpenRC runs, ten portable MSVC checks, two SDK consumers relinked with
exit 0, verified FDs, cleanup, and final builds. Initial compilation
failures and clock skew retained in logs; final delivery clean. No new
public API, Monitor/SystemActions integration, dependency installation,
elevation, or CI claim. K03 stays E: next public selection and consent
integration, then real acceptance as environment and permissions allow.
L05 deployment remains E and deferred, no new Unix channel; SysV/I07
unchanged. TXT authoritative. C60/E10/P3/B2 plus 3 X: 80% closed/20% open,
no new IDs or partial credit. Earlier checkpoints are historical.
Mirror 158 confirmed: comment 90140276877977; remote 60% title is historical.

Checkpoint 157: [Transport boundary](LINUX_PRIVILEGED_TRANSPORT.en.md)
tested. A TEST-ONLY profile on a real private broker limits messages
to 32 KiB, rejects a 64 KiB string before the handler, and continues
serving another client. Three attempts, two handler entries, two
authorization checks, and zero Manager calls. Limits are broker-wide,
NOT per helper. Do not install on a shared bus. Initial zero FD quotas
blocked authorization; failure retained and positive fixture corrected.
Linux 186 plus 160 L05 checks across 30 brokers, MSVC ten argument checks,
two SDK consumers exit 0, FD cleanup, and clean Linux builds. Production
library, public API/layout/formats, Monitor, and SystemActions unchanged.
No new production transport, installation, elevation, real services,
real polkit, acceptance, or CI claim. L05 stays E: pending choice between
deferring L05 deployment and continuing K03/OpenRC (recommended), or
extending a private Unix channel. No extra runtime broker or host-wide
change inferred. Process/loader, policy and consent, acceptance, and UAC
remain pending. K03/I07 unchanged; OpenRC 148 still covers explicit
control only. C60/E10/P3/B2 and 3 X: 80% closed/20% open, no new IDs or
credit. Mirror 157 confirmed: comment 90140276862936; remote 60% title
is historical.

Checkpoint 156: [Admission and queue pressure](LINUX_PRIVILEGED_ADMISSION.en.md)
completed. Token bucket per receiving connection and broker sender:
defaults 64/16 s and 16/4 s, 64-entry table, 300 s idle timeout, integer
credit under a mutex, TTL at least the full refill period, no eviction
of active entries. Read/write queue thresholds of 64 reject work before
RPC. No per-UID, fairness, or global DoS guarantee; the guard does not
bound queue bytes, pre-receipt work, or replies. Both endpoints share
admission; OFF and busy return earlier. Tests use real private broker
and libsystemd with simulated Authority/Manager. The five-request case
with two authorization checks, one Manager call and one Subscribe
verifies rejection before budget use, checks, or dispatch; a new sender
cannot bypass the shared quota. Regression with 56 requests, 32
authorization checks, 18 Manager calls, and two Subscribe calls uses an
explicit high-limit profile, not changed defaults. Linux 185 plus 150
L05 checks across 20 brokers, MSVC 20 admission-argument checks, two SDK
consumers exit 0, FD cleanup, and clean Linux builds. L05 stays E for
transport contract, deployment, and real acceptance. No installation,
helper activation, elevation, real services or polkit, public API/layout/
format changes, Monitor/SystemActions integration, or CI claim. OpenRC
148 still covers explicit control only; SysV/K03/I07/UAC unchanged.
C60/E10/P3/B2 and 3 X: 80% closed/20% open, no new IDs or partial credit.
TXT authoritative. Mirror 156 confirmed: comment 90140276857313; remote
60% title is historical.

Checkpoint 155: [Per-request budget](LINUX_PRIVILEGED_BUDGET.en.md)
completed. Steady clock and mutex; 1 ms–30 s and 1–128 instrumented RPCs
(default 5 s/96). Identity, daemon, Authority, Manager, match, Subscribe,
and dispatch share one budget. RAII restores the default timeout, and
explicit calls cap their timeout at the time remaining. Owned auth-only
also uses 5 s/96; unbudgeted legacy raw retains its prior contract. This
is cooperative, NOT an absolute deadline or a transport rate, queue, or
byte limit. A timeout after an attempt is UNKNOWN and MAY have acted;
no retry. Real private broker/libsystemd with simulated Authority/Manager:
56 requests, 32 authorization checks, 18 Manager calls, two Subscribe
calls. Holds before and after send show the call returned before the
holds were released. The native 2 s timeout was restored per request.
Linux 183 plus 130 L05 checks across ten brokers, MSVC 20 budget-argument
checks, two SDK consumers exit 0, FD cleanup. Initial clock skew retained;
final build clean and final suite 183 without changing clock or mtime.
L05 remains E: next admission and transport queues. Deployment, process
and loader, real polkit, acceptance, and UAC pending. No public API,
layout or format changes, Monitor/SystemActions integration, installation,
helper activation, elevation, real services, or CI claim. OpenRC 148
still covers explicit control only; SysV/K03/I07 unchanged.
C60/E10/P3/B2 and 3 X: 80% closed/20% open, no new IDs or partial credit.
TXT authoritative. Mirror 155 confirmed: comment 90140276845885;
remote 60% title is historical.

Checkpoint 154: [Asynchronous job tracking](LINUX_PRIVILEGED_JOBS.en.md)
completed. Match and Subscribe use the owner before dispatch; cache by
connection/owner, with the slot owned by the connection. Shared data and
snapshots are thread-safe and make no bus calls. Checks cover uoss,
body, sender, bus, uint32 ID, path, and unit. Limits: 64 early events,
4,096 events, 64 pending watches, 1 ms–30 s deadline. Handles duplicates,
conflicts, future results, and pending/completed/failed/unknown states.
Timeout and revocation do NOT cancel; completed does NOT establish
current service state. Real private broker/libsystemd with simulated
Authority/Manager: 50 requests, 29 authorization checks, 16 Manager calls,
two Subscribe calls. Spoofed events ignored, subscription failure prevents
dispatch, FD cleanup verified. Linux 182 plus 120 L05 repetitions across
ten brokers, MSVC 20 FSM-argument checks, two SDK consumers exit 0, clean
Linux builds. No public API/layout/format changes, Monitor/SystemActions
integration, installed helper, activation, elevation, real services,
real polkit, acceptance, or CI claim. L05 remains E: next receiver-wide
limits and deadline; deployment, process/loader, and real polkit pending.
OpenRC 148 still covers explicit control only; SysV/K03/I07/UAC unchanged.
C60/E10/P3/B2 and 3 X: 80% closed/20% open, no new IDs or partial credit.
TXT authoritative. Mirror 154 confirmed: comment 90140276834738;
remote 60% title is historical.

Checkpoint 153: [Closed internal execution](LINUX_PRIVILEGED_EXECUTION.en.md),
OFF by default. ControlSystemService is separate from auth-only and
revalidates fresh authorization, context, gate, policy, connection,
and Manager owner/UID. Fixed start/stop/restart method, unit, and replace
sent to the unique owner. One dispatch with autostart and interaction
off, 5 s, no retry. A receipt with exact cookie/sender/signature/path
means job_queued, NOT completion. Failures or changes after a send
attempt yield dispatch_unknown (it MAY have acted). No rollback, retry,
or atomic revocation. Real private broker/libsystemd, simulated Authority
and Manager. Per run: 41 requests, 20 authorization checks, eight Manager
calls, none to a real service. Linux 181 plus 110 L05 repetitions across
ten brokers, MSVC 20 default/argument checks, two SDK consumers exit 0,
FD cleanup. Initial clock skew retained; clean incremental build and
final suite 181 without changing the clock. No public API/layout/format
changes or new public actions, installed helper, elevation, real polkit,
acceptance, or CI claim. L05 remains E: next job limits and tracking;
deployment, loader, and real polkit pending. OpenRC 148 still covers
explicit control only; SysV/K03/I07/UAC unchanged. C60/E10/P3/B2 and 3 X:
80% closed/20% open, no new IDs or partial credit. TXT authoritative.
Mirror 153 confirmed: comment 90140276823908; remote 60% title is historical.

Checkpoint 152: [Owned anchor and connection](LINUX_PRIVILEGED_CONNECTION.en.md).
Absolute anchor without symlinks or unsafe ancestors; calling thread's
PID/EUID and mount/user namespaces checked. Sticky ancestors OFF by
default, explicitly ON only for /tmp tests. New connection checks endpoint,
node UID and peer separately, with Hello, Unix stream peer credentials,
CLOEXEC, and readiness. FDs, anonymous access, interaction, and
exit-on-disconnect set to 0; timeout <=5 s. No default bus, environment
bus, or CLI. Per-connection generation and persistent revocation.
Receiver bound to its own bus with revalidation afterward. Real private
broker/libsystemd and simulated Authority: 22 requests and nine checks.
Directory change during positive authorization yields UNKNOWN and revokes
the connection. Linux 180/180 plus 100 L05 repetitions across ten
brokers, MSVC 20 argument/fallback checks, two SDK consumers exit 0,
FD cleanup. Initial indentation warnings fixed and retained in logs;
verification and final builds clean. No hardened process/loader,
operational helper, real polkit, executor, installation, elevation,
real services, API/layout/format changes, or CI claim. L05 remains E:
next limits, closed execution flow, and deployment before acceptance;
no authority to activate or install. OpenRC 148 still covers explicit
control only; SysV/K03/I07/UAC unchanged. C60/E10/P3/B2 and 3 X:
80% closed/20% open, no new IDs or partial credit. TXT authoritative.
Mirror 152 confirmed: comment 90140276816812; remote 60% title is historical.

Checkpoint 151: [Protected L05 policy](LINUX_PRIVILEGED_POLICY.en.md).
Closed parser with limits of 96 KiB and 256 rules. openat reads from
a trusted anchor with trusted UIDs, permission/type/link/path checks,
and revalidation; no shell. Store tracks generations, invalidates on
refresh without fallback, and the managed receiver discards invalidated
policy during identity/authorization. Real private broker/libsystemd
and simulated Authority: 21 requests and eight checks, including
discarding a positive reply after invalidation and then recovering.
Linux 179/179 plus 90 L05 repetitions across ten brokers, MSVC 20
parser/argument/fallback checks, two SDK consumers exit 0, FD and private
fixture cleanup, clean Linux builds. No new API, layouts, formats, or
actions. No trusted process/anchor/loader bootstrap, watcher, executor,
real polkit, installation, elevation, acceptance, or CI claim. L05
remains E: next bootstrap and connection. OpenRC 148 still covers
explicit control only; SysV/K03/I07/UAC unchanged. C60/E10/P3/B2 and
3 X: 80% closed/20% open, no new IDs or partial credit. TXT authoritative.
Mirror 151 confirmed: comment 90140276807772; remote 60% title is historical.

Checkpoint 150: [Closed L05 receiver](LINUX_PRIVILEGED_RECEIVER.en.md)
completed. Exact method, path, interface, and signature; version,
operation, intent, unit, and preflight; no client-provided identity
fields. Real libsystemd/broker transport, UID, owner, and serials on a
private bus without activation. Simulated Authority, no polkitd.
Per run: 16 requests and five checks. Spoofed sender does not impersonate
the client; 11 rejections without Authority; start/stop/restart observed
and denial conservative. Linux 178/178 plus 80 L05 repetitions across
ten brokers, MSVC ten argument/fallback checks, two SDK consumers
relinked with exit 0, FD and private fixture cleanup. Initial build
clock skew retained; final build clean. No operational helper, executor,
demonstrated human consent, real policy, installation, elevation, new
actions or API/layout/format changes, or CI claim. L05 remains E: next
protected configuration and lifecycle, then executor and acceptance.
OpenRC 148 still covers explicit control only; SysV/K03/I07/UAC unchanged.
C60/E10/P3/B2 and 3 X: 80% closed/20% open, no new IDs or partial credit.
TXT authoritative. Mirror 150 confirmed: comment 90140276799432;
remote 60% title is historical.

Checkpoint 149: [Native CheckAuthorization](LINUX_PRIVILEGED_NATIVE_AUTHORIZATION.en.md)
implemented and connected to the coordinator with a frozen factory.
Preflight, subject/owner, before/after identity, sender/cookie,
type/signature, bounded parser (64/128/4,096/16 KiB), and rejection of
duplicates and trailing data. Autostart 0, interaction OFF; no retry,
cache, or executor. Linux 176/176 plus 60 L05 repetitions, MSVC ten
argument/fallback checks, two SDK consumers relinked with exit 0, clean FDs.
Simulated libsystemd ABI and real private kernel socket, NOT real
Authority/broker, authenticated client, helper, receiver, dialogs,
execution, installation, or acceptance. Initial clock skew retained;
final incremental build clean and suite 176 without changing the clock.
No new API, layouts, formats, Monitor/SystemActions integration, Windows
feature, N03 fix, or CI claim. L05 remains E: next isolated receiver,
closed payload, and real transport identity, then bootstrap, secure
configuration, executor, and acceptance. OpenRC 148 still authorizes
explicit control ONLY, no SysV extension; K03/I07/UAC unchanged.
C60/E10/P3/B2 and 3 X: 80% closed/20% open, no new IDs or partial credit.
Mirror 149 confirmed: comment 90140276792674; TXT authoritative.

Checkpoint 148 (2026-11-02): [Current OpenRC authorization](LINUX_OPENRC_CONTROL_DECISION.en.md).
The user approved existing scripts/runtime ONLY for explicit
start/stop/restart, opt-in and OFF by default, with private tests.
Never for data collection, installation, elevation, or real services.
No SysV, helper, polkit, or autostart authorization. Resolves decision
132, but does not implement the executor, validate acceptance, or
close K03. Earlier notes about no OpenRC exception are historical.
L05 remains active; isolated native CheckAuthorization is next.
Do not ask again for development permission when resuming K03, but
each action requires consent. No new code, execution, or tests from
this record. Evidence 147 remains historical. C60/E10/P3/B2 and 3 X:
80% closed/20% open, no new IDs, partial credit, or closures.
Mirror 148 confirmed: comment 90140276786160; TXT authoritative.

Checkpoint 147: [Native L05 identity](LINUX_PRIVILEGED_IDENTITY.en.md)
from current message, ready client connection, address, destination,
and sender; Unix endpoint, router UID from SO_PEERCRED, and FD;
daemon queries for ID, caller UID, Authority owner and UID, with
revalidation and context checks. No identity from payload or fixed
root UID. Native code validated with a SIMULATED ABI and real private
kernel socket: NOT a real broker, polkit authorization, receiver,
helper, or CheckAuthorization. Linux 174/174 plus 40 L05 repetitions,
MSVC ten argument/fallback checks, two SDK consumers relinked with exit
0, clean FDs and initial/final Linux builds. No new API, layouts,
formats, actions, dialogs, installation, activation, elevation, CI
claim, Windows feature, or N03 fix. L05 remains E: next native
CheckAuthorization with pinned subject, correlation, and bounded
parsing in isolated tests, without an executor. Configuration,
bootstrap, receiver, integration, acceptance, and UAC pending.
K03/OpenRC 132/I07 unchanged. C60/E10/P3/B2 and 3 X: 80% closed/20% open,
no new IDs or partial credit. Mirror 147 confirmed: comment
90140275959685; TXT authoritative.

Checkpoint 146: Internal [L05 coordinator](LINUX_PRIVILEGED_AUTHORIZATION.en.md),
fixtures only: copy, context, one check, revalidation, cookies, denial,
challenge, retention, and errors without caching. Expected Authority
UID comes from mechanism configuration, NOT hardcoded 0. Corrected
in review and tested for non-root, root, mismatch, and absence. The
boolean means conservative acceptance, not raw is_authorized or a
ticket. NO native adapter, real authenticated identity, real polkit,
helper, IPC, executor, elevation, or installation. Linux 172/172 plus
20 repetitions, MSVC ten pure checks, two SDK consumers relinked with
exit 0, clean initial/final Linux builds. No new API, layouts, formats,
actions, CI claim, Windows feature, or N03 fix. L05 remains E: next
native identity/Authority adapter with verified transport and private
rejection tests, before connecting an executor/helper. K03 deferred;
OpenRC 132/I07 unchanged and UAC deferred. C60/E10/P3/B2 and 3 X:
80% closed/20% open, no new IDs or partial credit.
Mirror 146 confirmed: comment 90140275958832; TXT authoritative.

Checkpoint 145: [K03 audit](LINUX_K03_AUDIT.en.md) complete. K03 remains E
and was deferred because script control was not authorized and real
acceptance was unavailable. Inventory, views, and Monitor were tested
against their contracts, not certified universally. Literal command -v
checks found no paths for four supervisors in WSL PATH. Nothing was
installed or run, and this does not establish global absence.
[L05/Linux](LINUX_PRIVILEGED_PREFLIGHT.en.md) moves P->E with a pure
private preflight for system systemd start/stop/restart against a
mechanism-owned allowlist: exact target, declared intent, limit 256,
invalid config denied, and an owned copy. No authentication,
authorization, peer identity, IPC, polkit grant, helper, or execution.
No new public layouts, formats, or API. Linux 171/171 plus ten
repetitions, MSVC ten pure checks, two SDK consumers relinked and
exited 0, initial and final Linux builds clean. Next: identity and
authorization boundary with private rejection fixtures, before
installing, activating, or executing anything. K03/OpenRC 132/I07
unchanged, UAC deferred. C60/E10/P3/B2 and 3 X: 80% closed/20% open;
no new IDs, partial credit, Windows feature, N03 fix, or CI claim.
Mirror 145 confirmed: comment 90140275957563; TXT authoritative.

Checkpoint 144: [Monitor service selection](LINUX_SERVICE_SELECTION.en.md).
Seven explicit routes/views without fallback. Selected arguments are
saved before I/O; errors, permission failures, and switches do not retain
old rows. Known empty and partial remain distinct. legacy_default keeps
the old route; disabled performs no query. OpenRC reads only the chosen
view without a name join or querying another source. No new actions
or raw source in the snapshot. SampleOptions ABI extended: REBUILD ALL
0.x clients. ServiceInfo/SystemSnapshot layouts and formats unchanged.
Linux 170/170 plus ten repetitions, MSVC ten pure dispatcher checks
(NOT Windows Monitor), two SDK consumers rebuilt/relinked and exited 0,
including the new Linux Monitor path. Private FD and sentinel checks
passed. Initial /mnt/c clock skew retained; final incremental build
clean, suite 170 passed without changing clock or mtime. No execution,
scripts, real acceptance, or CI claim. K03 remains E: next audit closure
per provider, evidence, and blockers, then L05 as appropriate. Do not
wait indefinitely for unavailable hardware. OpenRC 132/I07 unchanged.
C60/E9/P4/B2 and 3 X: 80% closed/20% open; no new IDs, partial credit,
Windows feature, or N03 fix.
Mirror 144 confirmed: comment 90140275956990; TXT authoritative.

Checkpoint 143: [OpenRC to ServiceInfo](LINUX_OPENRC_SERVICES.en.md).
Separate configured and marked views by source and index, without
joining identities by name. IDs are distinct and scoped. Estimated
marked-* Metrics do NOT establish liveness or readiness. PID,
description, group, and policy are UNKNOWN. Partial results, permissions,
and failures are independent; original source Metrics are preserved.
An incomplete source does not suppress another complete source or
produce an invented stopped state. Linux 169/169 plus 30 OpenRC
repetitions, MSVC ten pure/fallback checks, two SDK consumers exit 0,
FD/sentinel checks passed, initial and final Linux builds clean.
No librc, CLI, execution, scripts, creation, or real acceptance.
Monitor, defaults, layouts, and other providers unchanged. K03 remains
E: next explicit Monitor provider selection, no silent fallback or
new actions. Integration, control, and acceptance pending. OpenRC
132/I07 unchanged. C60/E9/P4/B2 and 3 X: 80% closed/20% open; no new
IDs, partial credit, Windows feature, N03 fix, or CI claim.
Mirror 143 confirmed: comment 90140275956062; TXT authoritative.

Checkpoint 142: [s6 to ServiceInfo](LINUX_S6_SERVICES.en.md). Explicit
query preserves source, evidence, logger, and index. Estimated
record-pid-present/zero/finishing/paused states do NOT establish
liveness, readiness, control capability, or authenticated peer.
uint64 PID, wait word, and ready are source-only; common PID/main_pid
is UNKNOWN. Ready with PID 0 is valid source data. Partial, invalid,
and permission cases remain UNKNOWN. Logger and shape guards and
scoped IDs; no inferred group, description, or policy. Linux 168/168
plus 50 s6 repetitions, MSVC ten pure/fallback checks, two SDK
consumers exit 0. Private FD, sentinel, and no-control-bytes checks
passed. Initial /mnt/c clock skew under 1 s retained; final incremental
build clean and suite 168 passed without changing clock or mtime.
No changes to Monitor, layouts, or contracts 133–136; no execution,
scripts, or real acceptance. K03 remains E: OpenRC next, integration
and acceptance pending. OpenRC 132/I07 unchanged. C60/E9/P4/B2 and
3 X: 80% closed/20% open; no new IDs, partial credit, Windows feature,
N03 fix, CI claim, or persistence.
Mirror 142 confirmed: comment 90140275955591; TXT authoritative.

Checkpoint 141: [runit to ServiceInfo](LINUX_RUNIT_SERVICES.en.md).
Explicit query returns aligned source, evidence, and rows from the
same call. Estimated reported-down/run/finish/paused Metrics do NOT
establish kernel liveness, readiness, or authenticated peer. Raw PID
is source-only; common PID/main_pid UNKNOWN. Partial census, invalid
record, or permission failure yields unknown status. No invented
description, group, or policy. Scoped hex IDs do not enable actions.
Linux 167/167 plus 30 runit repetitions, MSVC ten pure projection/
fallback checks, two SDK consumers exit 0. Private FD, sentinel, and
no-control-bytes checks passed; initial and final Linux builds clean.
No changes to Monitor, systemd, layouts, persistence, or contracts
128–129; no execution, CLI, scripts, or real acceptance. K03 remains
E: s6 projection next; other providers, integration, and acceptance
pending. OpenRC 132/I07 unchanged. C60/E9/P4/B2 and 3 X: 80% closed/
20% open; no new IDs, partial credit, Windows feature, N03 fix, or
CI claim. Mirror 141 confirmed: comment 90140275955125; TXT authoritative.

Checkpoint 140: [SysV to ServiceInfo](LINUX_SYSV_SERVICES.en.md).
Explicit query returns revision 139 evidence and aligned rows from
the same call. LSB Short/Long description with availability,
permission, and partial-result information. Scoped hex IDs do not
enable actions. PID, liveness, group, and enablement remain UNKNOWN.
No changes to Monitor, systemd, or shared layouts; no execution or
CLI. Linux 166/166 plus 40 SysV repetitions, MSVC ten pure projection/
fallback checks, two SDK consumers exit 0; private FD/sentinel checks
passed. Initial /mnt/c clock skew under 1 s retained; final incremental
build clean and suite 166 passed without changing clock or mtime.
No acceptance on a real init or control. K03 remains E: runit projection
next; other providers, integration, control, liveness, and acceptance
pending. OpenRC 132/I07 unchanged. C60/E9/P4/B2 and 3 X: 80% closed/
20% open; no new IDs, partial credit, Windows feature, N03 fix, CI
claim, or persistence.
Mirror 140 confirmed: comment 90140275954616; TXT authoritative.

Checkpoint 139: [SysV catalog with descriptions](LINUX_SYSV_DESCRIBED_CATALOG.en.md).
Opt-in metadata binds to the pinned candidate and identity from the first
census. A second census and root/context checks guard publication.
Swaps before pinning or after reading, root changes, wrong identity,
and partial results yield UNKNOWN; no join by an old basename.
Budget 0–16 MiB (default 8), conservatively reserving two prefixes,
not disk usage. Catalog 137 does not read script bodies. Linux 165/165
plus 30 SysV repetitions, MSVC 20 portable/fallback checks, two SDK
consumers exit 0 and clean FDs. Initial /mnt/c clock skew under 1 s
retained; final incremental build clean and suite 165 passed without
changing clock or mtime. No execution, CLI, control, liveness, enablement,
or real acceptance. K03 remains E: explicit projection into the common
model next; integration, control, liveness, and acceptance pending.
OpenRC 132/I07 unchanged. C60/E9/P4/B2 and 3 X: 80% closed/20% open;
no new IDs, partial credit, Windows feature, N03 fix, CI claim, or
persistence. Mirror 139 confirmed: comment 90140275953534; TXT authoritative.

Checkpoint 138: [LSB descriptions](LINUX_SYSV_LSB.en.md). Pure parser
and type-checked reader of a pinned 64 KiB prefix, with independent
declared Short-Description and Description fields. Invalid UTF-8,
exceeded bounds, duplicates, truncation, and control characters yield
UNKNOWN. No shell, eval, execution, CLI, or publisher, liveness, or
default-policy claim. No unsafe join with an earlier catalog.
Focused Linux/MSVC checks initially failed because indentation counted
toward the 1,024 limit. Trimming and the optional-value test were
corrected; logs retained. Linux 164/164 plus 20 SysV/LSB repetitions,
MSVC ten portable checks, two SDK consumers exit 0, private FDs clean.
Relink clock skew under 1 s on /mnt/c retained; final incremental build
clean and suite 164 passed without changing clock or mtime. K03 remains
E: next bind metadata to the pinned candidate. Integration, control,
liveness, and acceptance pending. At this checkpoint OpenRC decision
132 and SysV script control had no exception; observer-only I07 unchanged.
C60/E9/P4/B2 and 3 X: 80% closed/20% open; no new ID, partial credit,
N03 fix, Windows feature, CI claim, or format.
Mirror 138 confirmed: comment 90140275952208; TXT authoritative.

Checkpoint 137: [SysV catalog](LINUX_SYSV.en.md). One explicit init.d
and one parent for rcN.d, with eleven levels (0–9 and S). Includes .sh
candidates and execute-bit metadata without checking access or
launching them. S/KNN and suffix naming conventions do NOT establish
identity. Device/inode index must be unique in a stable scope.
Hard links, external targets, and partial data yield UNKNOWN.
A missing runlevel differs from a broken alias. Two metadata scans,
root/context checks, bounds, and visible partial results. No script
body reads, execution, CLI, PID, liveness, status, control,
enable/disable, current state, or effective boot policy. Linux 163/163
plus 20 repetitions, MSVC ten pure fallback checks, two SDK consumers
exit 0, clean FDs and final build. Private fixtures, NOT real SysV
acceptance. K03 remains E: bounded LSB description comments without a
shell next. Control, integration, real acceptance, and other pending
work unchanged. OpenRC script decision 132 remained unauthorized at
this point; observer-only I07 unchanged. C60/E9/P4/B2 and 3 X: 80%
closed/20% open; no new ID, partial credit, N03 fix, Windows feature,
CI claim, or persisted format.
Mirror 137 confirmed: comment 90140275949924; TXT authoritative.

Checkpoint 136: [s6 observation](LINUX_S6_OBSERVATION.en.md). read_record()
is tied to the pinned control target. LinuxS6Observation uses one-shot
consent, a baseline, polling, and a deadline. RAW conditions only: no
liveness, readiness, freshness, or causal completion. Ready is ignored;
restart requires a later changed TAI value, not just a PID or readiness
timestamp change. Finish and gaps are excluded. Delivery counts are
retained and completion stays UNKNOWN even when a condition is observed.
No retry, worker, timer, undo, or final read. Late values do not confirm
the window, timeout does not mean failure, and the report is frozen.
Linux 162/162 plus 40 s6 repetitions, ten portable MSVC checks, two SDK
consumers exit 0, clean build and private FDs. Initial indentation warning
fixed. No real s6 acceptance. K03 remains E for integration, liveness,
real acceptance, and other init systems. Read-only SysV inventory is next.
I07 and OpenRC script decision 132 remain deferred and unchanged.
C60/E9/P4/B2 and 3 X: 80% closed/20% open; no new ID, credit, Windows
feature, N03 fix, CI claim, or persisted format.
Mirror 136 confirmed: comment 90140275948612; TXT authoritative.

Checkpoint 135: [s6 inventory](LINUX_S6_INVENTORY.en.md). Explicit
ScanRoot, visible directories and aliases, and one logger level with
the parent name. No recursion, manager autodetection, or global installed
inventory. Raw record and FIFO type stay separate; neither establishes
liveness, active status, peer identity, permissions, delivery, or an
action token. readAtPin reuses the reader from checkpoint 133. Service
and logger share a parent. Bindings, context, and root are guarded, with
caps and visible partial results. A broken logger is not absent.
No CLI, execution, scripts, locks, writer probes, writes, mkdir,
dependencies, or real services. Linux 161/161 plus 30 s6 record/control/
inventory repetitions, ten MSVC fallback checks, two SDK consumers
exit 0, clean FDs and final build. No real s6 acceptance. K03 remains E
for observation, integration, real acceptance, and other init systems.
Next: observe the pinned control target. OpenRC script decision 132
was still unauthorized and deferred; observer-only I07 unchanged.
C60/E9/P4/B2 and 3 X: 80% closed/20% open; no new ID, credit, N03 fix,
or Windows feature. Mirror 135 confirmed: comment 90140275946721;
TXT authoritative.

Checkpoint 134: Public [s6 FIFO control](LINUX_S6_CONTROL.en.md) with
open/request, consent, target pins, context checks, and u/d/cru.
Delivery counts do NOT establish completion, readiness, liveness, or
peer authentication/version. Completion stays UNKNOWN. No CLI, libs6,
execution, scripts, persistent down state, worker, retry after positive
delivery, or wait. Private fixtures cover EPIPE, EAGAIN, pending SIGPIPE
and mask handling, fork staleness, replacement, and FD cleanup.
Linux 160/160 plus 20 s6/control repetitions, ten pure/fallback MSVC
checks, two SDK consumers exit 0, clean final build. No real s6
acceptance. K03 remains E: s6 inventory next; observation, integration,
real acceptance, and other init systems pending. OpenRC script control
was still unauthorized and deferred; observer-only I07 unchanged.
C60/E9/P4/B2 and 3 X: 80% closed/20% open; no new ID, partial credit,
Windows feature, N03 fix, or CI claim.
Mirror 134 confirmed: comment 90140275943430; TXT authoritative.

Checkpoint 133: Public [s6 reader](LINUX_S6.en.md) decodes and reads
the raw 35-byte record. It does not establish supervisor or process
liveness or effective readiness. Two raw TAI values, uint64 PID
without narrowing, wait word, and reported flags. PID 0 with ready=true
is valid. openat2 pins, context, two reads, metadata and type/size
checks, and permission_required. Missing, invalid, or changed data
yields UNKNOWN. No execution, CLI, libs6, skalibs, scripts, lock probes,
FIFO I/O, dependencies, autodetection, or ServiceInfo join.
Linux 159/159 plus 20 repetitions, ten pure/fallback MSVC checks,
two SDK consumers exit 0, and clean private FDs. No real s6 acceptance.
Next: s6 handle/control; inventory and integration pending. A
“prosigue” advances the s6 alternative without assuming an OpenRC
script exception under decision 132, still deferred and open.
Observer-only I07 unchanged. K03 remains E. C60/E9/P4/B2 and 3 X:
80% closed/20% open; no new ID, partial credit, Windows feature,
N03 fix, or CI claim.
Mirror 133 confirmed: comment 90140275941116; TXT authoritative.

Checkpoint 132: [OpenRC control audit](LINUX_OPENRC_CONTROL_DECISION.en.md)
ONLY. The reviewed official route runs scripts/runtime. A librc
marker does not replace control, and the global init FIFO is not opened
to control an individual service. Decision requested: allow an opt-in
exception ONLY for explicit start/stop/restart, never collection, with
private tests and no installation, elevation, or real services; or
defer OpenRC control and continue with s6/SysV. Without a response, do
not implement or execute the exception; “prosigue” alone is not enough.
No new code or tests in 132. Latest results from 131 (Linux 158 plus 20,
MSVC 10, two SDK consumers exit 0) were not rerun. K03 stays E with
criteria unchanged; observer-only I07 unchanged. C60/E9/P4/B2 and 3 X:
80% closed/20% open, no credit.
Mirror 132 confirmed: comment 90140275939136; TXT authoritative.

Checkpoint 131: [OpenRC catalog](LINUX_OPENRC_CATALOG.en.md). One
explicit init.d and one runlevels directory. Regular candidates and
executable_mode bits do not grant launch permission. Service links
match unique device/inode pairs; ambiguous hard links yield UNKNOWN.
Direct stacks use inode matches without recursion, expansion, or
unbounded cycles. No elevation, CLI, librc, script-body reads, script
execution, or mkdir. Two metadata scans with root/context guards and
bounds; partial remains UNKNOWN and local indices are not authority.
No boot-enabled status, effective state, live PID, control, or real
acceptance. Linux 158/158 plus 20 OpenRC reader repetitions, ten
pure/fallback MSVC checks, two SDK consumers exit 0, clean private FDs.
Millisecond-scale /mnt/c relink clock skew retained; final incremental
build clean and suite 158 passed without changing clocks or mtime.
K03 remains E for control, liveness, integration, real OpenRC/runit,
s6, and SysV. Observer-only I07 stays deferred; N03 remains open.
C60/E9/P4/B2 and 3 X: 80% closed/20% open; no new IDs or partial credit.
Next: review native OpenRC control.
Mirror 131 confirmed: comment 90140275938588; TXT authoritative.

Checkpoint 130: [OpenRC markers](LINUX_OPENRC.en.md). Explicit filesystem
observation API for seven categories. Its own decoder does not use
RC_SERVICE. Two passes, pins, context, bounds, and UNKNOWN for partial
results. Symlink metadata without following targets. No librc, shell,
scripts, CLI, or mkdir. A started marker does NOT establish running state
or PID; absence does NOT mean stopped; an empty root does NOT mean zero
installed services. Linux 157/157 plus 20 repetitions, ten pure/fallback
MSVC checks, general SDK and header consumers exit 0, clean private FDs.
Initial millisecond-scale /mnt/c clock-skew log retained; final incremental
build clean and suite 157 passed without changing clocks. K03 remains E
for installed services/runlevels, control, liveness, real OpenRC acceptance,
integration, real runit, s6, and SysV. No I07 scope change, Windows feature,
N03 fix, or CI claim. C60/E9/P4/B2 and 3 X: 80% closed/20% open; no new ID
or partial credit. Next: OpenRC installed services and runlevels.
Mirror 130 confirmed: comment 90140275936748; TXT authoritative.

Checkpoint 129: [runit observation](LINUX_RUNIT_TRANSITIONS.en.md).
K03 one-shot observation with consent, exact baseline, explicit polling,
and steady-clock deadline. Restart requires a new TAI epoch; stop
excludes down with want-up. An observed target condition does not establish
readiness or causal completion; delivery.completed is always UNKNOWN.
Late values do not confirm the window. No retries, worker, scripts, or
undo. Linux 156/156 plus 20 repetitions, ten MSVC transition checks,
general SDK and runit new-symbol consumers exit 0, clean final build and
private FDs. No real runsv acceptance. K03 remains E for real runsv,
OpenRC, s6, SysV, and integration. I07 cause remains UNKNOWN and deferred;
observer only, no new launcher. N03 open. C60/E9/P4/B2 and 3 X: 80%
closed/20% open, no new IDs or partial credit.
Mirror 129 confirmed: comment 90140275461997; TXT authoritative.
On 2026-10-30 the user explicitly chose to keep observing and move to
K03: do not develop a custom launcher or close I07 when cause cannot
be verified. Decision mirrored: comment 90140275539280.

Checkpoint 128: [runit](LINUX_RUNIT.en.md). K03 explicit open/read/
inventory/request API, 20-byte status and FIFO, reader checks on ok,
pins/context, and two reads. Raw TAI and reported PID are not identity;
description and group unknown. Start u, stop d, and restart tcu report
delivery only; completion is UNKNOWN. No CLI, scripts, elevation, manager
autodetection, or existing ServiceInfo/layout changes. PRIVATE native
Linux protocol I/O, aliases, unknown data, bounds, replaced FIFO, EPIPE,
pending SIGPIPE/mask, and FD cleanup passed. NO real runsv acceptance;
real services untouched. runsv not found in WSL PATH, query only.
Linux 155/155 plus 20 repetitions, general SDK and new-header consumer
exit 0, ten MSVC decoder/unsupported checks, clean build. K03 remains E
for transition confirmation, real runsv, OpenRC, s6, SysV, and integration.
“Prosigue” continues the recommended observer-only scope without a new
launcher. I07 cause stays UNKNOWN and deferred, no closure and criteria
unchanged; N03 open. C60/E9/P4/B2 and 3 X: 80% closed/20% open; no new ID,
partial credit, or Windows feature.
Mirror 128 confirmed: comment 90140275459137; TXT authoritative.

Checkpoint 127 AUDIT: [Causal attribution decision](STARTUP_EARLY_DECISION.en.md).
Session metadata, GIO, unit names, proximity to login, and a host we
control do not establish automatic OS cause. A SCOPE decision separate
from the already authorized recorder 103 was requested: keep observing,
defer I07 cause, and continue K03 (recommended); or develop an opt-in
launcher privately without installing or activating it or launching
real apps. Even a managed host does not prove the OS started it.
Keep the result Unknown. Do not assume a UI default, close the task,
or redirect work before a reply. No new code, tests, or probes in 127.
Revision 126 evidence (Linux 154 plus 30, MSVC 20, SDK exit 0) is
historical. I07 stays E and N03 open. C60/E9/P4/B2 and 3 X:
80% closed/20% open; no new ID or partial credit.
Mirror 127 confirmed: comment 90140275454763; TXT authoritative.

Checkpoint 126, 2026-10-29: The INTERNAL [manual-only recorder bridge](STARTUP_RECORDER_MANUAL.en.md)
is integrated into the EXISTING LinuxStartupRecorder. It uses an owned
source, start/finish, canonical archive, and safe PCIMAN save, with two
consent checks and no acceptance of GIO data or Monitor scans. Legacy tables
stay empty, raw data stays unbound, automatic origin is UNKNOWN, and the
public legacy save rejects it. Guards cover context, clock, support, capture,
entry, PID, outcome, and native observer. Move/abandon does not perform a
final read, kill, or reap; PID ownership is returned unchanged. Explicit
polling against the public deadline finishes observation. Resource windows
are relative to birth. Four private cases with the REAL recorder cover a
positive result, failure, live UNKNOWN, finish/save/reload/reprojection,
0600 permissions, and FD cleanup. Linux 154/154 plus 30 checks, MSVC 20
manual/legacy checks, SDK exit 0. Initial make future-timestamp warnings
retained; final rebuild and suite 154 clean. I07 remains E for a trusted
managed-autostart context, other paths and reapers, and descendants.
N03 open; no activation, new public API/format, or Windows feature.
C60/E9/P4/B2 and 3 X: 80% closed/20% open.
Mirror 126 confirmed: comment 90140275453225; TXT authoritative.

Checkpoint 125, 2026-10-29: INTERNAL [PCIMANv1](STARTUP_MANUAL_STORE.en.md)
completed. Raw manual receipt and embedded PCISTART3 birth records,
window, and policy, with 16 MiB limit, checksum, and strict validation.
HISTORICAL only: no live ownership, FDs, authorization, or capture.
Links and resources are recalculated; caches are not serialized. The shared
memory codec and legacy I/O were extracted without changing PCISTART1/2/3,
app history, or public API. Linux manual save checks 0600 and current
ownership BEFORE writing payload. Filesystems that ignore the mode are
rejected and the old file is preserved; /tmp positive. Windows inherits
ACLs; no POSIX claim. Four private 0600 envelope round trips tested
same-source reprojection, UNKNOWN, raw unbound data, and FD cleanup.
Linux 153/153 plus 30 manual/birth/legacy repetitions, SDK exit 0, MSVC
ten manual and two legacy I/O compatibility checks. Initial fixture offset
type and privacy-failure mode corrected, guard tested. Initial make
future-timestamp logs retained; final rebuild and suite 153 clean without
adjusting the clock. I07 remains E for recorder, managed-autostart context,
other paths, and descendants. No activation; manual is not automatic
cause. N03 open. C60/E9/P4/B2 and 3 X: 80% closed/20% open, no new ID
or partial credit.
Mirror 125 confirmed: comment 90140275450450; TXT authoritative.

Checkpoint 124, 2026-10-29: INTERNAL [receipt-to-resources bridge](STARTUP_DESKTOP_RESOURCES.en.md)
completed. A one-shot operation arms the creator's birth/final/exec
observations BEFORE launch, then obtains the receipt. Explicit finish
polls, closes, samples, and detaches; no hidden wait or signal, and no
final read on stop. Exact join by boot, kernel TGID/UID/start boot,
capture, namespace PID, and first file. Full read-only validator and
staging copy; no bitwise comparison of timestamps across sensors or
changed binding. MANUAL resource lower bounds, 60 s birth window,
pre-exec data, I/O including waited-for children, and high-quality
lower bounds or unknown. No summing, autostart, or proof of app, content,
or current identity. Four private cases: CPU 22,418,570 ns and disk
1,155,072 B AFTER reap (1 MiB fsync and 20 ms CPU); two unattributed
pre-exec errors; live finish UNKNOWN with WNOHANG 0, caller release and
wait with exit 7, raw unbound data, and FD/probe cleanup. Linux 152/152
plus 20 checks, MSVC ten pure checks, SDK exit 0. Initial make
future-timestamp warnings retained; final rebuild and suite 152 clean.
I07 remains E: recorder, persistence, trustworthy managed-autostart
route without activation, other paths, and descendants. Manual is not
automatic cause; N03 open. No new public API, file, format, RunNewTask,
associate path, or Windows feature. C60/E9/P4/B2 and 3 X: 80% closed/20%
open, no new ID or credit.
Mirror 124 confirmed: comment 90140275447569; TXT authoritative.

Checkpoint 123, 2026-10-29: INTERNAL [controlled manual receipt](STARTUP_DESKTOP_RECEIPT.en.md)
completed. The session combines desktop route, gate, and selected first
exec, with two consent checks before I/O, child creation, or probes.
One-shot, no fallback or relaunch after stop. The HISTORICAL receipt
records fixed manual_request, full kernel/local/boot/namespace identity,
request and file pin, outcome, exec quality, and exit. A true/false link
requires known exec, exact quality, and a released PID. Absence, EOF,
exit, estimated quality, or flags leave it UNKNOWN; contradictions fail
closed. No allocation after release in start. Four private native cases:
one match AFTER reap, two pre-exec errors yielding UNKNOWN/ECHILD, and
one live match. Stop does not kill or reap; caller releases and waits.
Copies, replay, one-shot behavior, and FD/probe cleanup passed.
Linux 151/151 plus 20 focused checks, MSVC ten pure denial/fallback
checks, SDK exit 0. Initial make future-timestamp warnings retained;
final rebuild and suite 151 clean without adjusting clock or mtimes.
I07 remains E for receipt/recorder/resource integration, trustworthy
managed-autostart route without activation, other paths, and descendants.
Manual is not automatic cause. N03 open; no new public API, archive,
RunNewTask, associate path, or Windows features. C60/E9/P4/B2 and 3 X:
80% closed/20% open.
Mirror 123 confirmed: comment 90140275444937; TXT authoritative.

Checkpoint 122, 2026-10-29: INTERNAL [direct XDG route](STARTUP_DESKTOP_LAUNCH.en.md)
completed. Wrapper obtains its OWN effective snapshot and requires
eligible condition exactly true. Supported Exec subset with two escaping
stages, whole-argument quoting, literal argv, and Path; no alternative
command supplied by the caller. Terminal, D-Bus, unknown vendor options,
and unsupported field codes are rejected. No implicit shell or PATH
lookup. Rechecks fields, ID, order, precedence, and eligibility BEFORE
and AFTER callback; stale data cancels and reaps the still-gated child.
No atomic transaction or proof of current file, content, or app.
Linux 150/150 plus 20 focused checks, MSVC ten pure denial/non-Linux
checks, SDK exit 0. Private native desktop-to-gate-to-selected-first-exec
matched the file pin AFTER reap; FDs cleaned, probe removed. Two
mutations during arming canceled with ECHILD; shadowed, disabled, and
unknown-condition entries rejected. MANUAL action is not autostart
cause. Next: managed receipt with manual/automatic distinction,
integration, other paths, and descendants. No new public API, archive,
RunNewTask, or associate path; no host/login/GUI activation, global
capture, or Windows features. N03 open. C60/E9/P4/B2 and 3 X:
80% closed/20% open, no new IDs, fractions, or estimated hours.
Mirror 122 confirmed: comment 90140275443077; TXT authoritative.

Checkpoint 121, 2026-10-29: INTERNAL [selected first exec](STARTUP_SELECTED_EXEC.en.md)
completed. Duplicates the pidfd and image pin inside the arming
callback. Full kernel/local/boot/namespace identity and context checked
before release; attach with an empty allowlist and enroll LAST, for
one TGID/start boot. No release_task or CPU dependency, no invented
birth or counters: raw fields 4–14 are UNUSEDZERO. First exec filesystem
tuple is compared with the pin, not proof of content, app, or cause.
Re-exec does not replace it; sticky first failure, publication rules,
and unknown state are preserved. Post-reap, own replay, and caller
guards; stop detaches and cleans FDs without signals or reaping.
Public API, RunNewTask, store, and associate unchanged. Linux 149/149
plus 20 focused checks, SDK exit 0, MSVC 20 pure decoder/denial/non-Linux
checks. Five private native children: three successful matches including
symlink/re-exec to a DIFFERENT inode, two pre-exec errors UNKNOWN, and
five ignored sentinels. Post-reap samples, immutability, FDs/mask/NNP,
and probe removal verified. Logs: selected-exec-*. I07 remains E:
validated desktop route, managed receipt, reliable cause, other paths,
and descendants. N03 open; no host/login/GUI activation or Windows
features. C60/E9/P4/B2 and 3 X: 80% closed/20% open, no new IDs,
fractions, or estimated hours.
Mirror 121 confirmed: comment 90140275441257; TXT authoritative.

Checkpoint 120, 2026-10-29: [Internal pre-exec gate](STARTUP_GATED_LAUNCH.en.md)
completed. Pins a regular ELF with O_PATH/NO_MAGIC, atomic clone3/PIDFD,
gate and arming callback, and full context, identity, credentials, and
stamp recheck before execveat on the pinned object. Cancels its own
held child via pidfd and reaps it. For released success/unknown the
caller uses waitpid; no hidden post-exec kill. Child NNP, raw syscalls,
kernel mask restored, no libc atfork, literal argv and environment.
No shell, PATH lookup, script, elevation, or fallback. The gate does
not prove exec success, app identity, or autostart and is not a sandbox.
Linux 148/148 plus 20 focused checks: positive self ELF, symlink,
denial/throw/stop cancellation, error FIFO, FDs/mask/NNP. Four manual
private kernel pidfd joins before release, probes removed; SDK consumer
exit 0. MSVC ten denial/non-Linux fallback checks; no Windows features.
Final C++ build clean, initial indentation warning fixed, intentional
D9025 for assertions. No new public API, layout, format, host, login, or
GUI. I07 remains E: selected SUCCESSFUL exec and reliable file-pin/desktop
route next, then receipt, managed autostart, other paths, and descendants.
N03 open. C60/E9/P4/B2 and 3 X: 80% closed/20% open, no new IDs,
fractional credit, or estimated hours.
Mirror 120 confirmed: comment 90140275436885; TXT authoritative.

Checkpoint 119, 2026-10-29: [Historical birth overlay](STARTUP_BIRTH_OVERLAY.en.md)
completed. Pure apply_startup_birth_impact API, consent false by default,
exact accepted capture ID/boot/PID namespace context, shared v3 validator,
and rollback before noexcept commit. IDs are declared, not proof of app
or cause or identity in the CURRENT snapshot. Single-group CPU/disk
estimated high or unknown; multiple groups are not summed, only a
high-quality lower bound or unknown. Legacy interval unknown. HISTORICAL
scope with capture, boot, namespace, source, quality, and window. Current
and unrelated fields and source are unchanged. No OS/file I/O, automatic
Monitor integration, or GUI. Linux 147/147 plus 20 focused checks,
MSVC ten pure checks, SDK exit 0. Thirteen native cases with 23 rows
from v3, 0600, FDs, and cleanup passed. Custom 1 ms high threshold used
ONLY in tests; defaults and previous assertions intact. No new layouts
or changes to existing formats. I07 remains E: receipt, reliable app and
cause attribution, other paths, and descendants next. N03 open; Windows
deferred. C60/E9/P4/B2 and 3 X: 80% closed/20% open, no new IDs, fractional
credit, or hour estimate.
Mirror 119 confirmed: comment 90140275428976; TXT authoritative.

Checkpoint 118, 2026-10-29: [Birth-aware PCISTARTv3](STARTUP_BIRTH_STORE.en.md)
completed. Archive.births preserves the complete raw report, keys, metric
presence/support/notes, exec evidence, window, and all four policy
thresholds, including pending, unbound, empty, and failed states.
Recorder.save v3 resolves the rejection in 117 without duplicate projections,
clock coercion, sums, or action authorization. Reads v1/v2/v3; writes without
birth data retain v1/v2. Historical absence is not empty or zero. No automatic
migration or app history changes. SDK and consumers rebuilt for ABI changes.
Linux 146/146 plus 20 focused checks, SDK exit 0. Thirteen native cases with
23 rows (18 finals, five absent) passed v3 save/load, reprojection, 0600,
FD, and cleanup checks. Three final MSVC fixtures passed outside isolation;
two initial runs aborted on assertions with replace error 5/access denied.
Stage/native-error diagnostics and logs retained without changing the
assertion or write/flush/close/replace sequence. This does not establish
the cause of, or fix, historical N03. Clean C++ builds; intentional D9025
for assertions. I07 remains E for receipts, reliable app/cause attribution,
guarded overlay, other paths, and descendants. N03 open; Windows deferred,
no host/login/GUI activation. C60/E9/P4/B2 and 3 X: 80% closed/20% open,
no partial credit. Mirror 118 confirmed: comment 90140275426329; TXT authoritative.

Checkpoint 117, 2026-10-29: [Child capture in the recorder](STARTUP_RECORDER_BIRTH.en.md)
completed. watch_births is opt-in for one creator, with its own native
factory and polling on each call. associate_birth uses the exact key even
after close; cleanup, failures, and retention handled. birth_capture and
birth_impacts are separate from legacy views. Windows are relative to each
birth, not a global phase; no clock coercion, summing, or automatic cause.
After attachment, save rejects even an empty capture before touching the
filesystem, with replaced=false: v2 cannot represent these identities.
A new format is pending; no silent loss. Report layout extended; SDK
consumer rebuilt. Linux 145/145 plus 20 focused checks, MSVC 10, SDK exit 0.
Thirteen native scenarios with 23 rows (18 finals, five absent) and private
CPU/I/O oracles; lifecycle, binding, replay, FD, and source checks retained.
No host/login/GUI activation or Windows work. I07 remains E for receipts,
reliable app/cause attribution, a birth-aware format, overlay, other paths,
and descendants. N03 open. C60/E9/P4/B2 and 3 X: 80% closed/20% open;
no new IDs or fractional credit. Caller-scheduled stop does NOT certify
global coverage. Mirror 117 confirmed: comment 90140275422485; TXT authoritative.

Checkpoint 116, 2026-10-29: [Short-lived child impact](STARTUP_BIRTH_IMPACT.en.md)
completed as the pure measure_startup_birth_impact API, using the report,
exact key, and prior binding. The 60 s window starts at EACH BIRTH, not
boot, login, or a global timestamp. Checked kernel deadline and CPU/disk
deltas when release is within the window yield estimated lower bounds.
Missing or late data yields unknown without clipping or prorating. High
requires exceeding the policy threshold; low values NEVER establish
low/medium impact or complete coverage. Measured zero, permission, and
overflow are explicit. No invented legacy clock or PID/start_ticks and
no tree totals. Cause is always unknown; binding is declared.
Linux 144/144 without skips plus 20 focused checks, MSVC 10, SDK consumer
exit 0. Thirteen native scenarios with 23 rows passed post-reap/binding
checks, with 18 finals and five absent. High was verified using a CUSTOM
1 ms test policy only; defaults unchanged. Independent source/default
behavior, candidates, replay, FDs, and cleanup preserved. No format,
activation, GUI, or Windows implementation changes. I07 remains E for
receipts, reliable app/cause attribution, recorder/persistence/overlay
integration, other paths, and descendants. N03 open. C60/E9/P4/B2 and
3 X: 80.0% closed/20.0% open, no new IDs or partial credit.
Mirror 116 confirmed: comment 90140275370635; TXT authoritative.

Checkpoint 115, 2026-10-29: [Same-capture validation](STARTUP_CAPTURE_VALIDATION.en.md)
corrected. Prior mode 8 reproduction: CPU 10,441,812 versus 10,581,094 ns
(139,282 ns difference), equal I/O. The harness treated TWO independent
sensors as one source. Now it checks exact passthrough of the SAME real
capture with native context, then a second DEFAULT instance against a
private clock with the same CPU bounds, exact I/O/fstat checks, and its own
immutable replay. A new regression rejects CPU 0 rewritten to 1 ns and
preserves CPU 0 and binding. No formula, probe, or threshold change.
Release-observed CPU is not guaranteed to be full post-switch lifetime CPU.
Linux 143/143 plus 20 focused checks, MSVC 10, SDK consumer exit 0.
Native 13 plus 20 positive scenarios. Exact kernel interleaving remains
unproven; do not claim a CPU fix or N03 closure. I07 remains E for receipts,
reliable app/cause attribution, integration, other paths, and descendants.
C60/E9/P4/B2 and 3 X: 80.0% closed/20.0% open, no new IDs or fractional
credit; Windows deferred. Mirror 115 confirmed: comment 90140275365822;
TXT authoritative.

Checkpoint 114, 2026-10-29: [XDG filesystem candidates](STARTUP_EXECUTABLE_MATCHES.en.md)
completed. Read-only API uses the current caller's catalog, exact key,
boot, and PID namespace. Validated literal absolute Exec subset with
string, quote, and field-code checks; O_PATH/openat2 NO_MAGICLINKS and
stat rechecks, pinned properties, batching, 16 MiB budget, and censuses.
A unique match requires exactly one match and a complete catalog and is
always estimated. Shared or disabled ambiguity and unknown results
preclude uniqueness. No binding, execution, autostart cause, or impact;
no guarantee of historical catalog or target UID. Linux 143/143 plus 30
focused checks, ten MSVC matcher checks, SDK consumer exit 0. Native 13
passed with six real exec results, including unique-to-shared ambiguity,
without promoting provenance or bindings. First Native4 failed resource
equality in mode 10; diagnostic added without weakening checks. Four
plus 13 repeats passed without reproducing it. Cause not established
or fixed; N03 open. Indentation warnings and fixture <map> fixed, logs
retained, Release checks active, final Linux clean, intentional MSVC D9025.
I07 remains E for receipts, reliable app/cause attribution, temporal
integration, other paths, and descendants. C60/E9/P4/B2 and 3 X: 80.0%
closed/20.0% open; Windows deferred, no persisted format or activation.
Mirror 114 confirmed: comment 90140275359283; TXT authoritative.

Checkpoint 113, 2026-10-29: [First successful exec](STARTUP_FIRST_EXEC.en.md)
completed. observe_first_exec is OFF by default. first_executable Metric
contains historical inode, device major/minor, process UID, and kernel
monotonic time. The allowlist includes only authenticated births, keyed by
TGID/start; only task/start is used to validate hits, with no unrelated
file or credential reads. Anonymous BTF MM depth eight and budget 256;
raw20, CAS for first record, state published last, time bracket, failure
state 3, and flag 64. Re-exec does not replace the first evidence or make
it the current executable, path, hash, app, or cause. Estimated caller
bindings and unknown autostart origin unchanged. CPU, I/O, and PID are
independent. Thirteen native cases, 23 rows, 18 finals, five absent, and
six exec records checked against own fstat after reap. Direct, failure,
re-exec, non-leader, replay, and FD cases covered. Initial anonymous-BTF
failure and proc self-exe fixture after leader exit fixed; logs retained.
Linux 142/142 plus 30 focused checks, MSVC 30 fixtures, installed SDK
consumer exit 0. Extended public layouts require rebuilding; persisted
format unchanged. Clean Linux build and intentional MSVC D9025 for
assertions. I07 remains E for reliable app/desktop/launch attribution,
temporal cause, integration, other paths, and descendants. C60/E9/P4/B2
and 3 X: 80.0% closed/20.0% open. N03 open; Windows deferred, no activation
or global capture. Mirror 113 confirmed: comment 90140275341609; TXT authoritative.

Checkpoint 112, 2026-10-29: [Public API and per-child association](STARTUP_BIRTH_OBSERVER.en.md)
completed. LinuxStartupBirthObserver start/poll/associate/close/report is
opt-in without worker, automatic elevation, launch, or installation.
Key combines capture, boot, and kernel TGID/UID/start, with separate PID
namespace. Caller binding is estimated and idempotent; conflicts rejected.
No app inferred from creator, name, or PID. Full caller context, not just
PID; transaction, replay, immutable final records, flags, and modes.
Errors retain accepted data. Close does no final read; report is a copy.
No tree I/O sums, classification, or synthetic ticks/steady time. Public
API usable, but executable evidence and cause still unknown. Nine native
cases with 15 public rows compared to the internal sensor, ten finals and
five absent. Binding/replay after reap, close, and FD checks passed.
Linux 142/142 plus 30 focused checks, ten MSVC fixture/non-Linux fallback
checks, installed SDK and new-header consumer exit 0. Initial indentation
warning and optional fixture corrected, logs retained, final clean.
MSVC D9025 from NDEBUG to UNDEBUG is intentional. No persisted format
or automatic integration. I07 remains E for app/exec/launch evidence,
autostart cause, temporal integration, other paths, and descendants.
C60/E9/P4/B2 and 3 X: 80.0% closed/20.0% open. N03 open; Windows deferred.
Mirror 112 confirmed: comment 90140275314523; TXT authoritative.

Checkpoint 111, 2026-10-29: INTERNAL [local PID at birth](STARTUP_BIRTH_PID.en.md)
completed. Independent BTF lookup searches upid from 0 through level
(at most 32) by inode of the authenticated OBSERVER namespace, not a
global PID or innermost namespace. local_pid Metric is independent of
kernel identity/start/UID. Flag 32 preserves CPU and I/O if mapping
fails. Fifteen private uint64 fields before birth publication. Missing
schema disables the feature with a note; invalid namespace, range, or
pairs rejected. No public API/format, worker, extra FD, or activation.
Nine native cases, 15 post-reap PIDs, ten finals, and five expected
missing finals. Private init self PID 1 differs in the observer's view;
no mounts or global changes. Two-child fixture EOF retained without a
proven syscall cause. Corrected one-init case passed alone and in suite
9 without collector changes. VM cases cover 0/32, global 88 to local 12,
faults, retained CPU zero, and adversarial BTF. Linux 141/141 plus 30
focused checks, MSVC 20, SDK consumer exit 0. Intentional MSVC D9025
from NDEBUG to UNDEBUG keeps assertions active. N03 not fixed; Windows
deferred. I07 remains E for app/exec/launch attribution, integration,
other paths, descendants, and reliable cause. C60/E9/P4/B2 and 3 X:
80.0% closed/20.0% open; TXT authoritative. Suite 9 repeated with exit 0:
self PID 1, observer PID 450, kernel TGID 1568, namespace 4026532221.
Mirror 111 confirmed: comment 90140275296541. Temporary probes removed.

Checkpoint 110, 2026-10-29: INTERNAL [birth-to-final records bridge](STARTUP_CHILD_FINALS.en.md)
completed. Opt-in OFF. release_task is installed before the birth hook;
early filter uses the same authenticated creator/reaper. Child matching
uses TGID/start/UID, live=0, and nr=1. Atomic publication, bracket, checked
deltas, and flags 8–16. Scheduler CPU uses task and signal accounting;
I/O uses task and signal and includes waited-for children. Do not sum trees.
Observer does not preselect, enumerate, or obtain pidfds for each child.
Eight native scenarios with nine finals and five expected missing finals,
private CPU oracle and exact I/O. Auto-reap unknown, known zero, quota,
thread, sentinel, fork, replay, and FDs checked. Kernel IDs still lack local
mapping, app attribution, or autostart cause. No Monitor/recorder/history
automation or global other-reaper/descendant coverage. Linux 141/141 plus
30 focused checks, MSVC ten birth-final and ten exit-BTF checks, SDK
consumer exit 0; recheck clean after initial retained clock skew. No new
API, format, or global activation. C60/E9/P4/B2 and 3 X: 80.0% closed/
20.0% open. I07 remains E for mapping, attribution, integration, and
reliable cause. N03 not fixed. TXT authoritative; ClickUp only a mirror.
Mirror 110 confirmed: comment 90140275274389. Eight native scenarios
passed again.

Checkpoint 109, 2026-10-29: [Internal birth sensor](STARTUP_BIRTH.en.md)
completed. FENTRY wake_up_new_task, authenticated creator TGID/UID/start,
and new leader. ARRAY quota 1–64 with explicit reservation, publication,
and failures. Records kernel IDs and CPU/I/O AT BIRTH, not finals, local
PID, app, real_parent, or autostart. No user-space census or preselection
of each child. Not yet integrated with recorder or history. Private
native creator plus thread and two children reaped BEFORE first child
PID read: capacity four gave two quality-0 records; capacity one gave
one quality-2/estimated record. Sentinel and thread ignored, fork copy
denied, creator reaped, replay retained, clean FDs; measured zero birth
counters. Linux 141/141 plus 30 focused checks, MSVC ten birth and ten
exit-BTF checks, SDK consumer exit 0. Indentation warning fixed and
initial clock skew retained; clean recheck. No new public API/layout/
format, activation, or global capture. Inherited fork references must
be closed or released through exec; stop releases owned references,
with no atfork guarantee. I07 remains E for child finals, local mapping,
attribution and integration, more descendants, and reliable cause.
C60/E9/P4/B2 and 3 X: 80.0% closed/20.0% open; N03 not fixed.
Delayed mirror 108 confirmed: 90140275243922. TXT authoritative.
Mirror 109 confirmed: 90140275259142.

Checkpoint 108: [Compatible final CPU](STARTUP_SCHEDULER_CPU.en.md)
integrated for selected, covered targets. Sample.cpu_counter identifies
the source. The POSIX process CPUCLOCK_SCHED reader requires a live pidfd,
identity checks before and after, and checked conversion to ns. Ticks are
not presented as scheduler time. With the legacy disk flag opted in, the
recorder attempts a new baseline. Reader kinds survive limits and failed
attachment; use a compatible final count or fall back to a lower bound,
without mixing sources, resetting counters, or converting unknown to 0.
Defaults OFF; no BPF for the live reader. Partial and gap states retain
their limitations; observed high impact does not prove autostart.
Wire v1/v2 unchanged with CPU source notes. exit_disk_* still describes
DISK diagnostics. New Sample layout requires rebuilding.
Native worker released before baseline, plus seven private BPF cases:
six CPU/disk recoveries and one uncovered unknown. CPU alone produced
high at 1,241,171,380 ns with 262,144 B of disk I/O. Archive, metadata,
flags, 0600, and FDs verified. Linux 140/140 plus 30 focused checks,
MSVC ten recorder and ten impact checks, installed SDK consumer exit 0,
clean build. I07 remains E for lifetimes not preselected, descendants,
and reliable cause. Compatible CPU and durable diagnostics for selected
targets are complete. No login activation, global capture, or new Windows
support. C60/E9/P4/B2 and 3 X: 80.0% closed/20.0% open.

Checkpoint 107: [Durable diagnostics](STARTUP_DIAGNOSTICS.en.md) completed.
Optional Archive.diagnostics preserves settings, elapsed time, 16 counters,
provenance, and presence in the same atomic v2 file. v1 remains readable
with metadata ABSENT, not invented zero. Counts, duration, booleans, enums,
overflow, truncation, and checksum checks are all-or-nothing.
released_without_final includes processes still alive at close; it does
not prove a missed exit. finished_at is in memory; elapsed time uses
checked integer arithmetic. Close rejects a backward clock and save rejects
an extreme interval. Archive/Report layout changes require rebuilding;
older readers reject v2. No migration on load or app-history changes.
CLI reports v2. Linux 140/140 without skips plus 30 focused checks;
MSVC one store and ten recorder checks. Private late/empty host cases and
six private cases using actual BPF, provider, recorder, observer, and store
verified diagnostics, flags, notes, 0600, and clean FDs. Installed SDK
updated; rebuilt consumer exit 0. No login activation or global capture.
I07 remains E for compatible CPU, never-selected lifetimes, and reliable
cause. Aggregate persistence is complete, not global coverage or I07
closure. C60/E9/P4/B2 and 3 X: 80.0% closed/20.0% open without partial
credit; N03 not fixed. Mirror 107: comment 90140272944220.

Checkpoint 106: [Final disk recovery](STARTUP_DISK_TAIL.en.md) integrated
into LinuxStartupRecorder/Observer/Store. Opt-in OFF, concurrent cap 1–32.
Last procfs CPU remains a LOWER BOUND and is not mixed with scheduler ns.
finish_partial_process marks gap, closure, and incomplete coverage.
Notes and flags persist in the existing format. A pending record after
procfs disappears waits until the deadline instead of producing false
zero. No late tail, reset, or automatic privileges. The phase includes
setup; start can close during scan/attach, and the CLI reports actual
state without BPF by default. New Options/Report fields require rebuilding;
prior ABI/layout not promised, format unchanged. Private native test used
an injected census (NOT an autostart/GIO test) and real provider, recorder,
observer, and store. Five recoveries after reaping, one auto-reap UNKNOWN,
six round trips with 0600/flags/CPU note/cleanup/FD checks, and 4 MiB
classified as high with partial coverage. Linux 140/140 plus 30 focused
checks, MSVC ten recorder and ten observer checks, SDK consumer exit 0.
Initial fixture tick was not due; corrected without weakening the guard.
No host activation or real global capture; N03 remains. I07 stays E for
compatible CPU, unselected lifetimes, reliable cause, and durable diagnostics.
C60/E9/P4/B2 and 3 X: 80.0% closed/20.0% open, no partial credit.
Mirror 106 confirmed: comment 90140272943424; TXT remains authoritative.

Checkpoint 105: INTERNAL [direct-release counters](STARTUP_FINAL_RESOURCES.en.md)
completed. FENTRY release_task targets the released task, not the reaper.
BTF, identity, leader, live=0, nr_threads=1, overflow, publication, context,
fork, and cleanup checks. Scheduler runtime ns stays separate from procfs.
Group-accounted I/O includes already-waited-for children; it is not
exclusive I/O or an automatic tree total. Five private native cases: four
positives with CPU clock/procfs I/O oracle and one repeated internal
auto-reap case correctly UNKNOWN. Before reaping, an exited pidfd does not
invent totals. After parent ACK and procfs absence, data and replay are
retained. FDs clean. Linux 140/140 without skips plus 40 focused checks
(four times ten), MSVC pure checks repeated ten times, SDK consumer exit 0,
clean incremental recheck. Initial fixture and clock-skew failures retained;
no weakening of schema/decoder or fix for N03. No new public API, layout,
vtable, or format. No installation, host activation, or global capture.
I07 remains E for compatible baseline, integration, unselected lifetimes,
reliable cause, and durable diagnostics. C60/E9/P4/B2 and 3 X: 80.0%
closed/20.0% open, unchanged.
Mirror 105 confirmed: comment 90140272941957; TXT remains authoritative.

Checkpoint 104: INTERNAL [selected-exit sensor](STARTUP_SELECTED_EXIT.en.md)
completed. BTF/FENTRY/taskstats_exit for the last thread, pidfd and joins,
and a one-shot event retained after reaping. No certified CPU, disk, or
autostart cause. Native private grandchildren, ECHILD, absent procfs,
single and multiple threads: a zombie leader with a live worker is not
final, but the last worker is. Sentinel, fork, parent exclusion, and FD
cleanup verified. F03/Monitor/Sampler/UID/H04 real-file regression passed
with the extracted shared BTF reader. Linux 140/140 plus 30 focused
checks, MSVC fixture ten repetitions, installed consumer exit 0, clean
recheck. Initial fixture index, cleanup-write, and clock-skew logs retained;
schema not weakened, N03 not fixed. No new public API, layout, vtable,
or format, and no host installation or activation. I07 remains E for
final counters, lifetimes not preselected, descendants, reliable cause,
and durable integration. 80.0% closed/20.0% open, unchanged.
Mirror 104 confirmed: comment 90140272941185; TXT remains authoritative.

Checkpoint 103: [Optional recorder host](STARTUP_RECORDER.en.md)
completed. Development of the second option was authorized, NOT
installation or activation. Reusable LinuxStartupRecorder API with
start/poll/close/report/save and a separate executable. Opt-in OFF by
default, no install rule or startup entry. Consent, context, EUID, fork,
deduplication, bounds, deadline, gaps, close, and late windows handled.
Explicit historical archive, with no persisted census diagnostics or
claim of autostart from GIO. Native host ready BEFORE launching a
private candidate: real CPU, partial coverage, 0600 file, Exec unchanged,
invalid/corrupt inputs preserved, and honest empty result. Real login
untouched. Linux 139/139 plus 30 focused checks, private MSVC controller
repeated ten times, installed consumer exit 0. Executable verified NOT
installed in the private SDK prefix. Initial unknown-versus-enum fixture
fixed; classifier and absence assertions unchanged, log retained.
Additive API without layout, vtable, or format changes. I07 remains E
for reliable cause, other short-lived processes, descendants, early
coverage, and durable diagnostics. 60/75=80.0% closed and 15/75=20.0%
open, unchanged; no new IDs or partial credit. Continue I07.
Mirror 103 confirmed: comment 90140272940396; TXT remains authoritative.

Checkpoint 102: I07 audited, no new code, capture, or suites. Still need
to choose the [early-capture host](STARTUP_EARLY_DECISION.en.md): future
GUI at login or a small optional recorder before opening the GUI. No
real startup entry installed or activated, and no additional architecture
accepted by silence. Neither option alone proves autostart or covers
other short-lived processes. I07 remains E. 60/75=80.0% closed and
15/75=20.0% open, unchanged; results from 101 below are historical.
Mirror 102 confirmed: comment 90140272939947; TXT remains authoritative.

Checkpoint 101: [Linux network by UID](LINUX_USER_NETWORK.en.md), J04
E->C. Revision 23 disk reporting preserved. New network report counts
each observer UID once, with domain, lanes, partial send/receive/total,
coverage and overflow, and paired F03 metadata. No duplicated sessions
or joins by bus UID or username. Legacy session network remains absent;
the new report is authoritative. Dedicated flag without session inventory.
Native two private children with UID 0 and two buckets: oracle
4164+4164=8328 B/s, worker AB->B->AB, history and clean FDs, real H04
258->516 B, replay and store unchanged. Linux 136/136 plus 40 focused
checks, installed consumer exit 0. Pure MSVC projection passed ten times.
Windows Monitor trial aborted with 0xC0000409 after the pure phase;
logs retained, cause under N03. Fixture native integration restricted to
Linux without weakening Linux assertions or modifying or validating
Windows functionality. 0.x layout: rebuild ALL. C60/E9/P4/B2, 75 active
and 3 X, 80.0% closed/20.0% open. Next: I07, not started.
Mirror 101 confirmed: comment 90140272939846; TXT remains authoritative.

Checkpoint 100: [Linux network history](LINUX_NETWORK_APP_HISTORY.en.md),
H04 E->C. Observed F03 integrated into Monitor/Sampler with executable
verified by pidfd, procfs, boot, namespaces, PID/start, UID, inode, and
path/stamp before and after reads and between samples.
linux_network_app_history is separate from CPU; deltas by bucket, no
retroactive counting, double counting, or automatic I/O. The exec-ABA
limitation is documented as estimated. Two private native children,
258->516 B, 50 replays, real archive, clean FDs, and probes removed.
Linux 135/135 plus 30 focused checks, installed consumer exit 0; new
MSVC fixture passed ten times. Earlier CPU units/boot-changed repeat
failure retained under N03; final run passed without changing assertions.
New SystemSnapshot 0.x layout: rebuild ALL consumers.
C59/E10/P4/B2, 75 active and 3 X, 78.7% closed/21.3% open. Next: J04,
not started. Mirror 100 confirmed: comment 90140272939021; TXT remains
authoritative.

Checkpoint 99: [Linux history store with availability](LINUX_HISTORY_STORE.en.md)
ready. H04 P->E, NOT closed: F03 does not yet supply per-app network_bytes.
LinuxHistoryStore saves CPU/network/notification rows with value, Support,
and note in a separate format. Cumulative merge avoids double counting
and validates strictly; reset does not remove directories. No automatic
I/O, new capture, or change to existing ABI. Linux 134/134 plus 30 focused
checks, installed consumer exit 0, and a REAL CPU round trip without
invented network zero. Twenty focused MSVC checks passed outside the
sandbox. Inside, temporary replacement returned access denied (5), also
with the older store. Logs retained; no new Windows code. Initial fixtures
corrected, assertions active, N03 not fixed. Executable attribution and
windows, F03 integration, and REAL network persistence remain pending.
C58/E11/P4/B2, 75 active and 3 X, 77.3% closed/22.7% open. Next: H04.
Mirror 99 confirmed: comment 90140272938633.

Checkpoint 98: [F03 acceptance and real view](LINUX_NETWORK_ACCEPTANCE.en.md),
E->C. The OPTIONAL native source meets the existing criterion, not
universal coverage, a wire format, or async support. SELF fixed with full
context and two fresh kernel reads; external actors retain permission
and join checks. REAL WSL: 30/30 strict, zero excluded, three Monitor
snapshots with paired network, epoch, and metadata, UDP delta 97, rates,
and clean FDs. Temporary permission confirmed, trials under 10 s total,
probes removed, no permanent authorization. Initial EINVAL and non-live
actor failures retained; churn fails closed and requires authorized
refresh. Linux 133/133 plus 30 focused checks, ten MSVC join checks,
installed consumer exit 0. Private native 17-actor TCP/UDP, IPv4/IPv6,
update, gap, and fork regression PASS. H04/J04/N02/N03 open.
58/75=77.3% closed, 17/75=22.7% open; C58/E10/P5/B2. Next: H04;
read [History](HISTORY.en.md). Linux build recheck exited 0 without warnings
after clock skew on generated files; logs retained. Mirror 98 confirmed:
comment 90140272937903; TXT authoritative.

Checkpoint 97: [Capacity 1,024 and batching](LINUX_NETWORK_SCALE.en.md)
completed. RAW64 per batch, 16 cookies per actor and 16,384 buckets.
Retained actors continue; additions start fresh without counting gaps.
No truncation or 1 Hz guarantee; no RLIMIT/config changes. Rebuild ALL
for the constexpr change. Fixtures cover 65/129/1,024 actors, 16,384 buckets,
and failures in batches 2 and 16. Native PUBLIC test used ONLY 65 private
actors: two batches and 87 FDs, restricted admission with 152 FDs, UDP,
rates, UID, unchanged refresh, and cleanup. 17-actor regression passed.
Linux 133/133 plus 70 focused checks, MSVC 70 focused, installed consumer
exit 0. Old consumer static_assert updated to 16,384; log retained, no
checks weakened. No new C++ warnings, expected D9025, N03 open. Real-view
validation and coverage pending. WSL authorization requested but not
received in this revision; NO global capture. F03/H04/J04 open, 76.0%
closed/24.0% open, no partial credit. Mirror 97 confirmed:
comment 90140272937319.

Checkpoint 96: [Monitor/Sampler admission and provenance](LINUX_NETWORK_ADMISSION_MONITOR.en.md)
integrated. SystemSnapshot pairs metadata and network data, not a new
census or global total. Consent EVERY time, strict default, exclusive
modes. Failure closes both reports, the worker continues, and flag/restart
does not rearm. Concurrent refresh/history, move, gaps, and provenance
tested. SystemSnapshot 0.x layout extended: rebuild ALL. Linux 133/133
plus 100 focused checks, installed consumer exit 0, MSVC 40 focused.
Native test passed with ONLY two private actors, a synthetic marker,
and object-restricted enumeration. Full view neither authorized nor
validated. First MSVC assertion failure retained; cause not isolated.
Rebuild and repeats passed without weakening assertions; N03 consumer
issue from checkpoint 75 unresolved. Temporary WSL clock skew logged;
later rebuild exit 0 without warnings. F03/H04/J04 open for scale,
coverage, authorized validation, and later integrations. 76.0% closed/
24.0% open, no partial credit. Mirror 96 confirmed: comment 90140272936245;
remote history unchanged; TXT authoritative.

Checkpoint 95: [Opt-in partial policy and provenance](LINUX_NETWORK_ADMISSION_POLICY.en.md).
Pure plan, strict default, explicit verified_only. PID/state/reason
exclusions accompany the sample; no hidden malformed data or fabricated
rows, rates, or UID totals. More than 64 eligible actors rejects ALL,
not first 64. all_encountered_verified describes quality, NOT readiness
or host-wide coverage. Qualified snapshot retains the last refresh,
not a new census. AdmissionChange 0.x layout extended: rebuild ALL.
Linux 132/132 plus 100 focused checks, installed consumer exit 0, MSVC
40 focused. Native positive used ONLY two private actors and a synthetic
marker. Monitor/Sampler, scale beyond 64, and authorized real-view
validation pending; F03/H04/J04 open. 57/75=76.0% closed, 18/75=24.0%
open, no partial credit. Mirror 95 confirmed: comment 90140272317289;
remote history unchanged.

Checkpoint 94: [Standalone strict admission](LINUX_NETWORK_ADMISSION.en.md).
Connects inventory, owner, and public observed provider. ALL must be
verified and at most 64, or reject ALL. Consent on each refresh;
unchanged set retains epoch/rates. No capture on construction, polling,
or rearm on sample. Native test used ONLY two private children, restricted
enumeration, and native procfs/pidfd/BPF. Gap/provenance, unchanged refresh,
FD counts 18->16->26, fork, unknown, restart, empty, and cleanup passed.
NO full-view capture. Linux 131/131 plus 80 focused checks, installed
new-header/API consumer exit 0, 20 MSVC fixture runs, no new C++ warnings,
expected D9025. F03/H04/J04 open: practical policy for large or
inaccessible views, authorized validation, and Monitor/Sampler admission
integration pending. 57/75=76.0% closed, 18/75=24.0% open, no partial
credit. Mirror 94 confirmed: comment 90140272312863; TXT authoritative.

Checkpoint 93: [Public shared observed backend](LINUX_NETWORK_SHARED_PUBLIC.en.md)
activated for standalone, Monitor, Sampler, and owner. Native 17 with
N+14 initial FDs and AB->B->AB passed retained provenance, excluded
gaps, FD counts 16->15->24, rates/UID/epoch, move, fork, concurrent worker,
opt-out without rearm, and cleanup. Native strict and declared modes
passed, without API, layout, coverage, or consent changes. Final 93:
Linux 130/130 without skips plus 60 focused checks, installed consumer
exit 0; MSVC 3/3 plus 60 focused passed. No new C++ warnings; expected
D9025. The assistant's initial concurrent builds in one directory
collided at ranlib; logs retained, rebuild serialized. F03/H04/J04 open
for general admission and later integrations, no global capture.
57/75=76.0% closed, 18/75=24.0% open, no closure or partial credit.
Mirror 93 confirmed: comment 90140272305719; remote history unchanged.

Checkpoint 92: [Authenticated internal shared update](LINUX_NETWORK_SHARED_UPDATES.en.md)
complete; public backend NOT activated. Retained actors do not restart;
additions and readmissions start fresh without counting gaps. New epoch
and reset of all rates. Bounded live batches, no recycled slots, no
constant three probes after churn, no atomic cut across batches. Native
16->15->16; Linux 130/130 plus 40 focused checks, installed consumer
exit 0, MSVC 40 focused. Public integration, Monitor/Sampler/owner,
and admission pending. F03/H04/J04 open; 57/75=76.0% closed,
18/75=24.0% open, no closure or partial credit.
Mirror 92 confirmed: comment 90140272299452; remote history unchanged.

Checkpoint 91: Safe shared RAW removal complete, NOT a public update.
Native 17->16->1 retained provenance and eight FDs, excluded removed
actors even while sending, and cleaned up. Fixtures cover suspended
events, 64->63, regressions, failures, and fork. Linux 130/130 without
skips; MSVC 2/2 repeated 20 times. Additions, readmission, authenticated
transition, and public integration still pending.
[Contract and limits](LINUX_NETWORK_SHARED_RETENTION.en.md).
F03/H04/J04 open; 57/75=76.0% closed, 18/75=24.0% open. Linux 40 focused
checks and installed consumer exit 0. Mirror 91 confirmed:
comment 90140272294875; remote history unchanged.

Internal reference tracker: SEGUIMIENTO.txt (not distributed here),
revision 95. 57/75 subtasks closed (76.0%), 11 in progress, five pending,
and two blocked; 18 open (24.0%). ClickUp is only a mirror. Summary:
[Progress](PROGRESS.en.md). This is not a percentage of effort.
The earlier estimated 70% and historical 60.4% are no longer current
indicators. Update after every unit with evidence.
User-requested revision 63 priority: [finish Linux before Windows](LINUX_FIRST_PLAN.en.md).
Next F03, then H04/J04, one subtask at a time. Windows implementation
deferred at checkpoint 62. No scope removed or states/count changed.
This revision only reorders work: no new code, capture, UAC, suites,
or closures. Windows regression only when shared compatibility requires it.
Mirror 63 confirmed: comment 90140271872526; historical title and
statuses not reconciled.
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

Previous unit, revision 86:
F03 remains E for automatic admission and capture that scales past 16 actors. H04/J04 remain open.
C57/E11/P5/B2, 76.0% closed/24.0% open, 18 open; no partial credit.
Logs: work/network-inventory-*. Mirror 86 confirmed: list comment 90140272246025.

Previous unit, revision 85:
F03 remains E for inventory with coverage, automatic admission, and scaling past 16 actors. H04/J04 remain open.
C57/E11/P5/B2, 76.0% closed/24.0% open, 18 open; no partial credit.
Logs: work/network-selection-*. Mirror 85 confirmed: list comment 90140272050845.

Previous unit, revision 84:
F03 remains E for general selection and coverage policy. H04/J04 remain open.
C57/E11/P5/B2, 76.0% closed/24.0% open, 18 open; no partial credit.
Logs: work/network-observed-monitor-*. Mirror 84 confirmed: list comment 90140272049411.

Previous unit, revision 83:
F03 remains E for observed Monitor/Sampler integration and general policy. H04/J04 remain open.
C57/E11/P5/B2, 76.0% closed/24.0% open, 18 open; no partial credit.
Logs: work/network-public-observed-*. Mirror 83 confirmed: list comment 90140272047500.

Previous unit, revision 82:
F03 remains E for a public observed provider, Monitor/Sampler integration, and general policy. H04/J04 remain open.
C57/E11/P5/B2, 76.0% closed/24.0% open, 18 open; no partial credit.
Logs: work/network-observed-multi-*. Mirror 82 confirmed: list comment 90140272046531.

Previous unit, revision 81:
F03 remains E for multiple actors, a common bracket, observed API/Monitor/Sampler, and general policy. H04/J04 remain open.
C57/E11/P5/B2, 76.0% closed/24.0% open, 18 open; no partial credit.
Logs: work/network-discovered-window-*. Mirror 81 confirmed: list comment 90140272044981.

Previous unit, revision 80: F03 E->E. [Private observed-cookie engine](LINUX_NETWORK_DISCOVERY.en.md).
No input cookie; uses the actor's kernel token, 16 slots, and CMPXCHG64.
No eviction. Sticky global quality, whole reads with eight retries,
validation, and empty/detach/stop/restart behavior. Fixtures cover
16 cookies, four lanes, same/different peer, capacity, quality, and
rollback. One WSL child with no producer-supplied cookie: OS oracle,
TCP 12288/12288, UDP 768/768, peek 24, two threads, and other traffic
excluded. Detach and restart begin empty, then TCP 8192/8192, UDP 512/512,
peek 16, and clean FDs. Only one real netns. The 16-cookie, different-peer
race, and capacity cases are fixtures; the oracle initializes the cookie.
Declared-mode Monitor regression passed. Linux 126/126 without skips,
two tests repeated 20 times, installed consumer exit 0, clean build.
MSVC focused 2/2 with no new C++ warnings and expected assertion D9025.
Checkpoint 75 consumer issue remains under N03; no full validation.
Fixture assert macro fixed and log retained. No new public API or layout,
GUI, persistence, dependencies, CLI, setns/unshare, or global capture.
F03 remains E for joins, context, windows and rates, multiple actors,
provider, and general policy; H04/J04 remain open. Logs: work/network-discovery-*.
C57/E11/P5/B2, 76.0% closed/24.0% open, 18 open; no partial credit.

Previous unit, revision 79: F03 E->E. [Buckets in Monitor/Sampler](LINUX_PROCESS_NETWORK_BUCKET_MONITOR.en.md).
Three-argument setup takes mode and consent; two-argument setup is always
strict after stop. Flag or restart does not reauthorize; setup cannot
change mode during active capture. Sampler setup requires a stopped worker
and the flag; update preserves mode. Fixtures and WSL Monitor/worker
tests passed four bucket/UID oracles, removal/re-addition, gaps, history,
concurrent updates, baselines, strict defaults, and cleanup. Only one
real netns; second cookie NOT observed. No concurrent two-netns test or
discovery. Linux 126/126 without skips, three tests repeated 20 times,
installed consumer exit 0, clean build. MSVC focused 3/3 without new C++
warnings; expected assertion D9025. Revision 75 consumer failure remains
under N03; no full validation. Additive API, no new layouts, options, GUI,
persistence, dependency, CLI, or global capture. F03 remains E for policy,
discovery, dynamic buckets, and general coverage; H04/J04 open.
Logs: work/network-bucket-monitor-*. C57/E11/P5/B2, 76.0% closed/
24.0% open, 18 open; no partial credit.

Previous unit, revision 78: F03 E->E. [Declared buckets](LINUX_PROCESS_NETWORK_BUCKETS.en.md)
in the individual provider. Opt-in constructor, up to 16 PID/start/cookie
rows, with multiple disjoint cookies per process. Filtering precedes the
map; strict default unchanged. Sticky quality, coherent deduplication,
context and UID, new coverage, and partial sums without duplicates.
Update changes epoch and resets ALL baselines; retention, removal, and
re-addition do not count gap traffic. Fixtures cover two cookies, four
lanes, and negatives. One WSL child declared two cookies, but only ONE
real netns and no observation of the other cookie. Four oracles, other
bucket zero, no duplicate sum, gap, fork, and clean FDs. No simultaneous
two-real-netns test; strict two-child regression passed. Linux 126/126
without skips, five tests repeated 20 times, installed consumer exit 0,
clean build. MSVC focused 5/5 without new C++ warnings; expected
assertion D9025. Revision 75 consumer failure remains under N03, no full
validation. Fixture overflow protocol error corrected, log retained.
Additive API and enum extension; no new layouts, GUI, persistence,
dependency, CLI, setns/unshare, or global capture. F03 remains E for
Monitor/Sampler bucket mode, policy, discovery, dynamic buckets, and
general coverage; H04/J04 open. Logs: work/network-buckets-*.
C57/E11/P5/B2, 76.0% closed/24.0% open, 18 open.

Previous unit, revision 77: F03 E->E. [Updating an active Sampler](LINUX_PROCESS_NETWORK_WORKER.en.md).
Serializes collection, commit, update, and stop, with priority and protected
wakeup for pending updates. History retains epochs; latest does not indicate
current capture state. Requires existing capture and fresh consent. Update
does not rearm; failure closes capture, not the worker. Concurrent CTest:
40 updates, start/stop, and reads; three tests repeated 20 times. Two
private WSL children AB->B->AB with worker and updater on separate threads:
four actor/UID oracles, gap, provenance, baseline, failure/restart without
rearming, and clean FDs. First native removal failed; a possible starvation
issue was removed. Repeat and final passed, log retained. Linux 126/126
without skips, installed consumer exit 0, clean build. MSVC focused 3/3
without new C++ warnings. N03 retains the revision 75 consumer failure;
no full compatibility claim. Additive API, no new layouts, persistence,
GUI, dependency, or global capture. F03 remains E for policy, discovery,
buckets, and general coverage; H04/J04 open. Logs: work/network-worker-*.
C57/E11/P5/B2, 76.0% closed/24.0% open, 18 open; no partial credit.

Previous unit, revision 76: F03 E->E. [Updatable selection](LINUX_PROCESS_NETWORK_UPDATE.en.md)
in the public provider and Monitor. AB->B->AB and reordering retain captures
by PID/start/cookie. Removed captures close before additions; fresh consent
and epoch required, ALL baselines reset. Retained counters keep provenance;
new ones do not count gap traffic. Runtime failure closes ALL; Monitor
also closes capture on any failure in an authorized update. Fixtures and
two private WSL children passed four oracles, gap, fork, exit/recovery,
and no FD leaks. Linux 126/126 without skips, two tests repeated 20 times,
installed consumer exit 0, clean build. Initial fixture order assertion
corrected, log retained. Additive API, no new layouts. F03 remains E for
worker update, policy, discovery, buckets, and general coverage; H04/J04
open. Revision 75 MSVC consumer failure remains under N03; no full
compatibility validation. MSVC 76 focused 2/2 passed, expected D9025
with assertions active and no new C++ warnings. Logs: work/network-update-*.
C57/E11/P5/B2, 76.0% closed/24.0% open, 18 open; no partial credit.

Previous unit, revision 75: F03 E->E. [Selected Monitor/Sampler](LINUX_PROCESS_NETWORK_MONITOR.en.md)
integrated on Linux. Flag defaults false, with separate explicit setup.
Opt-out or worker stop closes capture; restart or flag does not reauthorize.
Two private WSL children: four oracles, worker lifecycle, move, fork,
rearm, and cleanup passed. Linux 126/126 without skips, 18 network tests
repeated 20 times, final installed Linux consumer exit 0, verified clean
build. New 0.x SystemSnapshot/SampleOptions layout: rebuild ALL. Metric
header extracted without semantic change. MSVC focused 2/2 passed;
general consumer returned 0xC0000409 after new checks. Cause NOT proven;
N03 risk pending, NO full compatibility validation. Logs: work/network-monitor-*.
No new Windows capture, GUI, persistence, or dependency. F03 remains E
for dynamic selection, buckets, and general coverage; H04/J04 open.
C57/E11/P5/B2, 76.0% closed/24.0% open, 18 open; no partial credit.

Previous unit, revision 74: F03 E->E. [Selected public provider](LINUX_PROCESS_NETWORK.en.md)
completed. New LinuxProcessNetworkSampler with movable Pimpl, opt-in start,
sample/stop, typed coverage and estimated support, local/kernel IDs by key,
counters, rates, quality, partial initial UID, brackets, and context.
Pure projection, 34 negative cases, zero baseline, UID 0, overflow, and
different ordering. PUBLIC WSL API with two children and four actor/UID
oracles, caller FDs closed, move while active, idle zero, parent excluded,
fork/exit, and cleanup. Linux 125/125 without skips, 17 network tests
repeated 20 times, installed header and external consumer built and run
with exit 0, clean final build. Fixture error fixed, log retained.
ADDITIVE API; existing layouts, ABI, and legacy totals unchanged.
No automatic Monitor integration, new Windows work, GUI, history, or
dependency. Logs: work/network-public-*. F03 remains E for opt-in
Monitor integration and selection/coverage/bucket policy; H04/J04 open.
Next: selected report in the general flow without false totals, private
tests, no global capture. C57/E11/P5/B2, 76.0% closed/24.0% open, 18 open.
Mirror 74 confirmed: comment 90140271926244; remote titles and statuses are historical.

Previous unit, revision 73: F03 E->E. [Common nominal window and UID totals](LINUX_NETWORK_COMMON.en.md)
completed. All joins before/after, scan <=1 s, then commit. Selection 1–16,
dedup by TGID/start, common nominal end, estimated rates, partial UID
with availability and overflow checks, brackets, and associations by key.
Failure, exception, change, or mode change resets all baselines. Fixtures:
same/two UIDs, 12 failures, 1 s/>60 s, overflow, modes, and fork. Two
private WSL children passed four actor/UID oracles, caller FDs closed,
idle zero, parent excluded, fork, exit, and cleanup. Native selected and
independent regressions exit 0. Linux 124/124 without skips, 16 network
tests repeated 20 times, installed consumer exit 0, clean build. Existing
CTests extended. Logs: work/network-common-*. No new Windows work, Monitor
integration, API, ABI, history, or dependency. No atomic/global snapshot
or dynamic cookies. F03 remains E for coverage contract, buckets, and
public provider; H04/J04 open. Next: coverage and selected opt-in provider,
private tests only, no global capture. C57/E11/P5/B2, 76.0% closed/24.0%
open, 18 open; no partial credit. Mirror 73 confirmed: comment
90140271923048; remote titles and statuses are historical.

Previous unit, revision 72: F03 E->E. [Selected multi-actor collection](LINUX_NETWORK_MULTI.en.md)
completed. Fixed list of 1–16, local/kernel deduplication, shared observer
and boot context, per-actor capture and pidfd duplication, and rollback
of ALL baselines on failure, exception, or change. Independent intervals,
not a simultaneous snapshot or shared UID sum. Fixtures: two actors,
2 s/3 s intervals, same UID/cookie, five startup failures, four second-row
failures, reset, and fork. Two authorized private WSL children:
A 4096/4096/256/256 B, B 8192/8192/512/512 B, four oracles per actor,
cross-traffic and parent excluded, idle zero, input FDs closed,
duplicate/fork, exit, and cleanup without FD leaks. Linux 124/124 without
skips, 16 network tests repeated 20 times, installed consumer exit 0,
clean final build. Initial indentation warning fixed, log retained.
Logs: work/network-multi-*. No new Windows work, API, ABI, Monitor
integration, history, or dependency. F03 remains E for common window,
UID aggregation, coverage, buckets, and integration; H04/J04 open.
Next: bounded common window and UID aggregation, private tests, no global
capture. C57/E11/P5/B2, 76.0% closed/24.0% open, 18 open.
Mirror 72 confirmed: comment 90140271920104; remote titles and statuses are historical.

Previous unit, revision 71: F03 E->E. [Revalidated selected windows](LINUX_NETWORK_SELECTED_WINDOW.en.md)
completed. Owned duplicate pidfd, fresh endpoint joins, kernel/boot/
observer/context checks, reducer commit only on success, and baseline
discard on failure, exception, or change. One-actor/cookie coverage is
separate from SELF/global, with a separate actor producer, shared epoch,
four estimated rates, and partial UID. Fixtures cover FD 0, eight startup
failures, 15 sample failures, reset, and fork. Authorized private WSL
child PID 402/kernel 913, UID 0: four rates/UID oracle, caller FD closed,
idle zero, fork/restart/exit, no FD leaks. Linux 123/123 without skips,
15 network tests repeated 20 times, installed consumer exit 0. Initial
make relink warnings about DrvFS timestamps; rebuild clean and suite
repeated without changing the clock. Logs retained: work/network-selected-window-*.
No new Windows work, API, ABI, Monitor integration, history, or dependency.
F03 remains E for multiple actors, buckets, coverage, and public integration.
Cookie still declared. No atomicity, ABA protection, future-lifetime
guarantee, or hard deadline. H04/J04 open. Next: coverage, buckets, and
multiple actors, private tests, no global capture. C57/E11/P5/B2,
76.0% closed/24.0% open, 18 open. Mirror 71 confirmed: comment
90140271917419; remote titles and statuses are historical.

Previous unit, revision 70: F03 E->E. [Selected pidfd/procfs join](LINUX_NETWORK_PIDFD_JOIN.en.md)
completed. Duplicates and polls the pidfd, checks stable boot and
observer/target context at both endpoints, requires two matching kernel
tokens, and revalidates caller-thread fdinfo, PID, start, status, and
namespace. Domains remain separate; procfs UID is observer-relative.
No tick-to-ns conversion. Authorized private WSL child PID 2513/kernel
6353, UID 0: parent read of start, repeated join, negative cases, four
capture lanes, and cleanup without FD leaks. An unprivileged worker
with an FD table distinct from the leader's correctly distinguished a
pipe and pidfd with the same number. Linux 122/122 without skips,
14 network tests repeated 20 times each, installed consumer exit 0,
clean build. Logs: work/network-join-*. No atomicity, ABA protection,
future-lifetime guarantee, automatic external cookie, or global
coverage. No new Windows work, API, ABI, Monitor, history, or dependency.
F03 remains E for revalidated windows, multiple actors, buckets, coverage,
and integration; H04/J04 open. Next: revalidated selected windows and
rates with private tests. C57/E11/P5/B2, 76.0% closed/24.0% open, 18 open,
no partial credit. Mirror 70 confirmed: comment 90140271913864; remote
titles and statuses are historical.

Previous unit, revision 69: F03 E->E. [Token from pidfd](LINUX_NETWORK_PIDFD.en.md)
completed. Duplication and liveness checks, FD -1/EBADF query with a
permission FEXIT probe scoped to the caller thread, mode 0x12, return 0;
BTF and one result required. No ptrace attach, signals, stopping the
target, or global traversal. The caller's temporary mask is restored
but can delay handlers. Authorized WSL child without a reported kernel
token: PID 554/kernel 1640, UID 0. Two queries, negative cases, TCP
4096/4096, UDP 256/256, eight peeks, parent excluded, detach, sticky
wrong start/UID rejection, removal, mask, and no FD leaks. Linux
120/120 without skips, 12 network tests repeated 20 times, installed
consumer exit 0, clean final build. Logs: work/network-pidfd-*.
Missing __pidfd_fget candidate dropped, pair warning fixed, new target
reconfigured. No new Windows work, Monitor, API, ABI, public history,
or dependency. F03 remains E for procfs join/revalidation, context,
multiple actors, buckets, coverage, and integration; H04/J04 open.
Next: selected pidfd/procfs join, private tests. C57/E11/P5/B2,
76.0% closed/24.0% open, 18 open; no new IDs, closures, or credit.
Mirror 69 confirmed: comment 90140271911142; remote titles/statuses
not reconciled.

Previous unit, revision 68: F03 E->E. [Selected actor capture](LINUX_NETWORK_SELECTED.en.md)
completed with declared kernel token and cookie, not authenticated
external identity. Explicit type separate from SELF; eight failure/
exception points, rollback, FD 0, and restart. Authorized cooperative
WSL child PID 2075/kernel 2600, UID 0: TCP 4096/4096 B, UDP 256/256 B,
eight peeks, parent excluded, detach, sticky rejection of wrong
start/UID, pidfd exit, and cleanup without leaks. Native joined SELF
also passed. Linux 119/119 without skips, 11 network tests repeated
20 times, installed consumer exit 0, clean build. Logs:
work/network-selected-*. No new Windows work, Monitor, API, ABI, public
history, or dependency. F03 remains E for reliable external identity,
multiple actors, buckets, coverage, and integration; H04/J04 open.
Next: reliable actor discovery/revalidation, private tests and bounded
permission. GUI/CMake preference in LINUX_FIRST_PLAN.en.md.
C57/E11/P5/B2, 76.0% closed/24.0% open, 18 open; no new IDs, closures,
or partial credit. Mirror 68 confirmed: comment 90140271905836;
remote titles/statuses not reconciled.

Previous unit, revision 67: F03 E->E. [SELF kernel/procfs association](LINUX_NETWORK_SELF_JOIN.en.md)
through a direct caller. Local PID/start_ticks/UID/procfs/namespace
token, bounded pinned caller/leader reader, two fresh BPF identity
checks, and stable boot. No tick-to-ns conversion or assumed equality
across ID domains. Rejection, changes, permission, context, or a raw/
joined transition invalidates the baseline. Linux 119/119 without skips,
11 network tests repeated 20 times, installed consumer exit 0. Native
procfs UID 1000/PID 950 stable over 20 checks. Authorized WSL PID 390/
kernel TGID 4965, UID 0/0, TCP 4096/4096 B, UDP 256/256 B, eight peeks,
four-rate/UID oracle over 793949 ns, idle zero, fork/restart, and no
FD leaks. Logs: work/network-self-join-*. Indentation warning fixed,
WSL timestamp logs retained, clean final build. No new Windows work,
API, ABI, public history, Monitor, or dependency. SELF with endpoint
checks only, not an atomic snapshot or attribution of external actors
or apps. F03 remains E for multiple actors, buckets, external identity,
coverage, and integration; H04/J04 open. Next: external actors with
private tests and bounded permissions. C57/E11/P5/B2, 76.0% closed/
24.0% open, 18 open; no new IDs, closures, or partial credit.
Mirror 67 confirmed: comment 90140271891145; remote titles/statuses
not reconciled.

Previous unit, revision 66: F03 E->E. [SELF capture windows and rates](LINUX_NETWORK_WINDOW.en.md)
connected. Sampler owns capture, scope, epoch, and reducer. Typed initial
kernel domains, one row/cookie, procfs boot UUID, and monotonic bracket
<=1 s. Estimated rates between read endpoints, maximum 60 s, partial
UID. Absence discards baseline, restart changes epoch, and fork rejects
the copy. Linux Release 117/117, nine network tests repeated 20 times,
installed consumer rebuilt and exit 0. Authorized WSL: TCP 4096/4096 B,
UDP 256/256 B, eight peeks, four-rate/UID oracle, 646362 ns, idle zero,
fork/restart, and no FD leaks. Logs: work/network-window-*. Initial build
clock skew 0.27 s; verified without warnings. No new Windows work,
Monitor, API, ABI, public history, or dependency. F03 remains E for
multiple actors, buckets, procfs/app/UID joins, coverage, and public
integration; H04/J04 open. Next: procfs identity for the producer.
C57/E11/P5/B2, 76.0% closed/24.0% open, 18 open; no new IDs, closures,
or partial credit. Mirror 66 confirmed: comment 90140271884970;
remote titles/statuses not reconciled.

Previous unit, revision 65: F03 E->E. [Socket namespace](LINUX_NETWORK_NAMESPACE.en.md)
checks completed. Optional BTF skc_net.net->net.net_cookie and declared
SELF restriction. Mismatches, nulls, and failed reads invalidate
persistently; positive peek cases also verified. Linux Release 116/116,
eight network tests repeated 20 times, installed consumer rebuilt and
exit 0. Authorized WSL cookie 1 corroborated by SO_NETNS_COOKIE,
TCP 6144/6144 B, UDP 384/384 B, 12 peeks, child excluded, threads,
detach/restart, and no FD leaks. Different cookie rejected persistently;
prior mode also passed. Logs: work/network-netns-*. No setns, unshare,
new config or dependency, or new Windows work, API, ABI, public history,
or Monitor. No real multi-namespace test. F03 remains E for buckets,
multiple actors, joins, coverage, windows, and integration; H04/J04
open. Continue F03 next. C57/E11/P5/B2, 76.0% closed/24.0% open, 18 open;
no new IDs, closures, or partial credit. Mirror 65 confirmed: comment
90140271880147; remote titles/statuses not reconciled.

Previous unit, revision 64: F03 E->E. [Private Linux SELF session](LINUX_NETWORK_SESSION.en.md)
lifecycle completed. Opt-in, exact identity, map 9, load before attach,
rollback at 14 failure/exception points, FD 0, bounded coherent read,
detach/destructor/restart. Linux Release 116/116, fixture repeated
20 times, installed consumer rebuilt and exit 0. Authorized native WSL
PASS: TCP 6144/6144 B, UDP 384/384 B, 12 excluded peeks, child excluded,
threads, detach/restart, no FD leaks; no-flag exit 77 confirmed.
Initial clock skew 0.0062 s, later clean. Wrapper 77 fixed through
quoting. Logs: work/network-session-*. No Windows progress or new API,
ABI, public history, or Monitor work. F03 stays E for netns, joins,
multiple actors, coverage, windows, and integration; H04/J04 open.
Continue F03. C57/E11/P5/B2, 76.0% closed/24.0% open, 18 open; no new
IDs or partial credit. Mirror 64 confirmed: comment 90140271876915;
remote titles/statuses not reconciled.

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
a caller precondition, not certified. C57/E11/P5/B2, 76.0% closed/24.0% open, 18 open.
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
exited 0. Logs: work/etw-disk-events-*. Native Windows tdh exported by the package; no new public
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
retained and removal confirmed for every session created. No new API/ABI or persistence.
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
112/112, 20 repetitions per Windows compiler. Positive private pipe test with 1024 B read and 1024 B written and Monitor baseline/rates/off-on checks. Three installed
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
so the unknown format is no longer a blocker. Runtime scope, heaps, caches, budgets, and reservations remain limited.
Unique observed and revalidated interop match is not physical LUID
identity. Validation covers 64-bit values, zero, and overflow; extracting
the helper preserved the earlier OpenCL version provider. Positive
RX 7900 XT on MinGW/MSVC: 21435580416 B and largest block 21413273600 B
in the first sample; global AMD memory remains unknown in Monitor.
Release: Windows 131/131 on both toolchains and Linux 111/111. AMD
and interop each repeated 20 times per Windows toolchain; three installed
consumers rebuilt, relinked, and exited 0. Logs: work/amd-runtime-*.
Initial test expected four closes even on early rejection; corrected
to compare opens and closes. Logs retained; MSVC test D9025 expected and no final C warnings.
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
D9025 expected. Native AMD host found no DLL in either location (126), not a
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
uniqueness, and physical count checks before and after. Bounds, errors, missing or partial output, MIG, and linked adapters can yield unknown; valid zeros and 64-bit values are preserved. Integrated after PDH; usage, capacity, shared memory, engines, and other providers unchanged. Optional System32 drivers without SDK, CLI, download, or
library-created contexts, buffers, kernels, or workers. cuInit can leave internal
driver state or threads; NVML init/shutdown paired and KMT/DLL resources
use RAII. Native 64-bit caller and 13 symbols; no legacy NVSMI fallback at this point.
Release: Windows 129/129 on both toolchains and Linux 111/111; NVML
repeated 20 times per Windows toolchain. Three installed consumers
rebuilt, relinked, and exited 0. Logs: work/nvml-free-*, final-* and
release-repeat-*. MSVC D9025 expected; no new C warnings. AMD host confirms absence and missing-DLL probe,
NOT positive physical NVIDIA/WDDM/TCC/multi-GPU/MIG/linked/hotplug.
Injected tests do not certify hardware. No new API/ABI, vtables, or
formats; Linux unchanged. AMD/Intel free memory and coverage remain in
M04; M05 deferred. C57/E11/P5/B2, 76.0% closed/24.0% open, 18 open.
Mirror 54 confirmed: comment 90140271595547; remote titles and statuses
not reconciled.

Previous unit, revision 53 (2026-10-21): M04 E->E. Vulkan MSVC support
and uniqueness completed. Windows CMake detection accepts a private
PCINSIGHT_VULKAN_INCLUDE_DIR pointing to existing headers and the VULKAN_SDK environment setting for private library/test use, without download, installation, export to consumers, or loader linkage. MSVC previously used
the stub; native RX 7900 XT Vulkan 1.4.349 by LUID now matches MinGW.
Explicit OFF honored and tested 1/1 with headers present; clear a
negative cached check with -U or use a fresh build. Duplicate or empty destination IDs
remain unknown, without affecting another unique row.
[Evidence](WINDOWS_GPU_VULKAN.en.md). Release: Windows 128/128 on both
toolchains and Linux 111/111, 20 Vulkan repetitions per Windows
toolchain. Three installed consumers rebuilt, relinked, and exited 0.
Logs: work/vulkan-portable-*. No new API/ABI, vtables, or formats. MSVC test D9025 intentional.
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
duplicates, changes, and sentinels. The library creates no context/window, draws
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
Windows previously ignored this option; it is now opt-in. Linux EGL/GLX unchanged. Sampler does not inherit the GUI context.
Final logs: work/wgl-release-*. Known zero LUID, no write, partial write,
and negative HighPart tested. Initial GCC warning fixed; expected MSVC
D9025 from test /UNDEBUG retained. M04 not closed: remaining criterion,
global memory, and adapter coverage pending; M01 open, M05 deferred,
N01/A05/N03 unchanged. C57/E11/P5/B2, 76.0% closed/24.0% open.
Mirror 52: comment 90140271590512.

Previous unit, revision 51 (2026-10-21): M01 E->E. MSVC x64 compatibility
completed. VS 18/MSVC 19.51/SDK 26100. Actual SDK rename failure fixed
using a documented internal ABI with checked offsets and static_assert, retaining the same symbol without the WDK or switching to
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
suite, post-clock regression, and consumer logs (work/msvc-verified-* and msvc-native-suite). No new API, layout, vtables, or formats;
version 0.2. M01 still needs stable compression and multi-group hardware.
CI N01/A05/N03 not closed. M05 deferred by explicit user confirmation on
October 21. C57/E11/P5/B2, 76.0% closed/24.0% open, 18 open.
Mirror 51 confirmed: comment 90140271587468; no fractional credit or new IDs.

Previous unit, revision 50: J05 P->C under the previously defined
alternative of documenting unavailable capability, not because an actual
disconnect was performed. New query_linux_disconnect_capability by utmp/login1 adapter, without I/O:
false/available with a reason on Linux, unknown for an unknown enum or
non-Linux. Inventory propagates the result only with recognized, known
metadata. Missing metadata now yields unknown, not generic false. IDs,
Remote, Active, State, and logout tokens are not capability evidence.
No signals, locking, logout, or elevation. Native contracts explain why
utmp/login1 do not offer transport detach, not why every remote server
would lack it. [Scope and evidence](LINUX_DISCONNECT.en.md). J03 is not
counted twice; WTS M06 and logout J02 remain separate. Release Linux
110/110 and Windows 126/126, 60 repetitions per platform, both consumers
rebuilt, relinked, and run with exit 0. Private POSIX child and utmp fixture left intact; read-only real login1 yielded one session with State and Active. The first
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
Windows action; A05/L05/N03 remain. Real Startup and HKLM were not modified. C56/E11/P6/B2, 74.7% closed/25.3%
open. Mirror 48 confirmed: comment 90140270915863.

Previous unit, revision 47: M05 E->E. Common Startup file control completed, scoped by ID/Known Folder, with a separate common listing. The user and Monitor scopes remain per-user. The 0.x behavior now affects all users within existing permissions; the host must warn and obtain consent. No elevation, ACL changes, or fallback. Checked operations, auditing, and rename semantics preserved. A private two-root native fixture passed; conflicts, scopes, root validation, and isolation verified, with an injected enumeration permission error. Real Common Startup was read-only; no ProgramData writes, UAC, ACL changes, or native ACL denials tested. WINDOWS_COMMON_STARTUP_FILE_CONTROL.en.md; logs in work/startup-common-*. Linux 110/110 and Windows 125/125 Release; 60 Windows repetitions. Both consumers rebuilt, relinked, and ran with exit 0; no warnings. Additive free API, no new virtual methods, layouts, or formats. Real Startup untouched; no fixtures left. M05 still needs HKLM, approval/StartupApproved, and a generic Windows action; Run 6801 unresolved, A05/L05/N03 open. C56/E11/P6/B2, 74.7% closed/25.3% open. Mirror 47 confirmed: comment 90140270909066.

Previous unit, revision 46: M05 E->E. Explicit Startup file action integrated with SystemActions/AuditedActions as a distinct operation; enum ordinals preserved. Request copied before the callback; checked backend without fallback. Sensitive ID kept bounded in the log, without contents or identities. Requested/completed/best-effort and exception policies unchanged. Private native auditing passed for removal, restoration, and sink failures; portable tests cover forwarding, errors, limits, and a mutating callback. New 0.x virtual method requires rebuilding ALL consumers and backends. No changes to Snapshot/Options/AuditEvent layouts or formats. WINDOWS_STARTUP_FILE_AUDIT.en.md; work/startup-file-audit-*.log. Linux 110/110, Windows 125/125 Release, 40 Windows and 20 Linux repetitions; both consumers rebuilt, relinked, and ran with exit 0. Final builds warning-free; initial WSL clock skew recorded. Run 6801 confirmed without writes; prior suites included the test despite notes saying “not repeated” (no dedicated probe). Real Startup untouched and no fixtures left. M05 still needs HKLM/common Startup/approval/generic Windows action; A05/N03 remain. C56/E11/P6/B2, 74.7% closed/25.3% open. Mirror 46 confirmed: comment 90140270903626.

Previous unit, revision 45: M05 E->E. Opt-in backup reporting for Monitor/Sampler completed, separate from Startup/Run. Off does not call the provider; on preserves empty/error/permission/note states without stale data. Reports have their own snapshots exposed through latest/history/timed_history, with no new persistence or actions and no change to the 60-second window. Private native fixture populated the report; real Monitor/Sampler read-only, with injected gate/error/copy cases. The 0.x Snapshot/Options layouts DO change: rebuild the library and ALL consumers. Provider types moved unchanged; no new virtual ABI or persistent formats. WINDOWS_STARTUP_FILE_MONITOR.en.md; work/startup-backup-monitor-*.log. Linux 109/109, Windows 124/124 Release, 40 Windows and 20 Linux repetitions; both consumers exit 0. WSL clock skew in compiler_depend.make recorded, followed by a settled pass ending with exit 0 and no warnings, rebuilds, or relinks. Windows warning-free. No clock or configuration changes and no cause attributed. M05 still needs HKLM/common Startup/approval/audit/generic actions; Run 6801 unresolved and not repeated; A05/N03 open. C56/E11/P6/B2, 74.7% closed/25.3% open. Mirror 45 confirmed: comment 90140270896765.

Previous unit, revision 44: M05 E->E. Explicit backup listing completed, with UTF-8 names and registration data for restore without previously saved IDs. Handle-based enumeration with two passes and identity/context/conflict checks; bounded, estimated, and non-atomic. No backup creation or writes; errors and permission failures return no partial list. Private native fixture passed with multiple files, Unicode, zero size, ordering, exclusions, and conflicts. Injected failures/records/limits/permissions and rejected changes verified. WINDOWS_STARTUP_FILE_CONTROL.en.md; work/startup-backup-listing-*.log. Linux 108/108, Windows 123/123 Release, 20 Windows repetitions; both consumers rebuilt, relinked, and ran with exit 0; final builds warning-free. Additive API and type only; no changes to existing layouts, virtual ABI, or history. No CLI, elevation, KTM, or real Startup changes. M05 still needs HKLM/common Startup/approval/integration; Run 6801 unresolved and not repeated. Not fully closed. C56/E11/P6/B2, 74.7% closed/25.3% open. Mirror 44 confirmed: comment 90140270891958.

Previous unit, revision 43: M05 E->E. New explicit current-user Startup file removal/restoration action passed a native test in a private fixture outside real Startup. Directory/file identity verified and no-replace rename by handles via NtCreateFile/NtSetInformationFile; PCInsight.Disabled.v1 backup preserves the same file, bytes, and attributes. Reading creates nothing; caller keeps the ID for restoration; complete backup listing not yet integrated. Local drives only; no UNC, common Startup, HKLM, StartupApproved, or automatic SystemActions/Monitor/audit integration. No CLI, KTM, elevation, fallback, or changes to existing layouts, virtual ABI, or history. WINDOWS_STARTUP_FILE_CONTROL.en.md; work/startup-file-control-*.log. Linux 108/108, Windows 123/123 Release, 20 Windows repetitions; both consumers rebuilt, relinked, and ran with exit 0; final builds warning-free. Read-only attribute preserved; conflicts, stale identity, sharing, changes, and limits tested. Two fixtures aborted due to an incorrect read-only expectation were cleaned by exact name; final fixtures clean. M05 remains open; Run TxR 6801 unresolved and not repeated. C56/E11/P6/B2, 74.7% closed/25.3% open. Mirror 43 confirmed: comment 90140270888697.

Previous unit, revision 42: M05 E->E. Fixed acceptance of a Startup root with a stream colon by is_absolute (regression failed before and passed after). Normal/extended drive and UNC paths, rejection before enumeration, and cleanup checked. GLOBALROOT, pipes, and incomplete UNC were already rejected by the current runtime; no prior failure attributed to them. WINDOWS_STARTUP_FOLDERS.en.md; work/startup-root-*.log. Linux 108/108, Windows 122/122 Release; 20 Windows repetitions. Both installed consumers relinked and ran with exit 0; builds warning-free. Run TxR still returned 6801 without writes. No changes to KTM or real Startup. Actions/approval pending; M05 not closed. C56/E11/P6/B2, 74.7% closed/25.3% open. Mirror 42 confirmed: comment 90140270883882.

Previous unit, revision 41: L03 B->C. Limited advanced option accepted after a specific proposal warning about PID/TID reuse, without checked semantics, EcoQoS, coverage for new threads, or guaranteed restoration. Generic Linux SystemActions on/off now returns unsupported without touching the target, with availability false/available; checked remains unsupported, without fallback. Advanced apply/restore require explicit ack=true and retain the receipt on failure. This 0.x BEHAVIOR change is documented in LINUX_EFFICIENCY.en.md and LINUX_EFFICIENCY_DECISION.en.md. No new signatures, layouts, virtual ABI, or history; Windows EcoQoS unchanged. Native private child tests covered rejection without changes, opt-in for two threads, complete restore, and mixed denied restore with pending flags. Caller untouched and children reaped. Linux 108/108, Windows 122/122, 20 Linux repetitions; both installed consumers rebuilt, relinked, and ran with exit 0; final builds warning-free. No race resolution, helper, elevation, ptrace, or new cgroup behavior. L05/N02/N03 unchanged. C56/E11/P6/B2, 56/75=74.7%, 19/75=25.3%. Mirror 41 confirmed: comment 90140270880053; no new IDs or fractions.

Previous unit, revision 40: E06 E->C. The user replied “prosigue” to the specific E06 proposal, accepting reported identity or unknown without inferring SSD from NVMe. No universal exact media detection implemented, and no L03 acceptance. RPM preserves permission_required when ATA/B1 is denied and there is no valid report; independent data survives. Tests cover layers, unrelated VPD, absence/permissions, stale SSD classification, no zero RPM, and alias value/support/note. LINUX_DISK_IDENTITY.en.md. WSL: four contexts and zero known media types (correctly unknown); positive technology cases are fixtures, not physical hardware. N02/N03 unchanged. Linux 108/108, Windows 122/122 Release, ten focused Linux repetitions; both consumers rebuilt, relinked, and ran with exit 0, warning-free. No new layouts, API, virtual ABI, history, dependencies, /dev access, CLI, elevation, or disk changes. C55/E11/P6/B3, 55/75=73.3%, 20/75=26.7%. Mirror 40 confirmed: comment 90140270876164; no new IDs or fractions.

Previous unit, revision 39: C03 E->C. The user accepted the estimated limit by replying “prosigue” to the specific C03 proposal. The installed-minus-usable difference is labeled estimated; unknown without valid data. It is not an exclusive hardware/BIOS reservation. This does not accept E06/L03. Usable-total permission status propagates to the difference and alias; fixtures cover absence, denial, contradictory values, unsupported, and zero ALWAYS estimated, with no stale data. LINUX_MEMORY_COMPOSITION.en.md. Linux 108/108, Windows 122/122 Release, ten focused Linux repetitions; both installed consumers rebuilt, relinked, and ran with exit 0; builds warning-free. Native composition positive; zero zram devices observed, Zswap/firmware absent, not global zero values. Positive compression/firmware cases are fixtures; N02/N03 unchanged. No changes to existing layouts, virtual ABI, history, dependencies, elevation, zram/swap, or memory pressure. C54/E12/P6/B3, 54/75=72.0%, 21/75=28.0%. Mirror 39 confirmed: comment 90140270873160.

Previous unit, revision 38: M03 E->C. WMI passed on 5/5 disks (two HDDs and three SATA/NVMe SSDs), without UAC or changes to permissions or WMI code. Earlier failure remains historical; no underlying cause was fixed. Filesystem 5/5; one system disk and one pagefile disk; read/write/latency 5/5. Active time valid on 1/5 in detailed sample and 5/5 in a later diagnostic (variable availability, no clamping or substitution with zero). Network: 55 known send/receive rates without generating traffic. query_windows_network_identity correlates a unique, revalidated LUID/GUID/PnP match; manufacturer/description Metric: nine manufacturers, eight Ethernet entries, positive Realtek PCIe 2.5GbE. WINDOWS_NETWORK_IDENTITY.en.md; 20 identity and 20 network repetitions. Windows 122/122, Linux 108/108 Release; both consumers rebuilt, relinked, and ran with exit 0. Additive API without changes to existing layouts, virtual ABI, or history. Closure covers supported providers, topologies, and explicit absence; no authenticated physical branding, universal RAID/MPIO/physical hotplug, or arbitrary attribution across layers. M02/M06/F03/N01/N03 unchanged. C53/E13/P6/B3, 53/75=70.7%, 22/75=29.3%. Mirror 38 confirmed: comment 90140270634154; no new IDs or fractions.

Previous unit, revision 37: M07 E->C. Read-only open_windows_service for GUIs, with typed errors and Metric metadata distinguishing known empty from inaccessible, without CLI/UAC. Status, PID, UTF-16, buffers, and handles verified; raw SCM load-order group/description. WINDOWS_SERVICE_VIEW.en.md; positive for eight services, eight descriptions, and eight groups. Own launch with argv confirmed; stale token rejected; checked EcoQoS on/off read back; checked end used the same handle and exited 1. Private child cleaned up. Twenty repetitions per new test (40 executions). Positive SCM validation from revision 36 retained and cleaned up, not repeated. Windows 121/121, Linux 108/108 Release; both consumers rebuilt, relinked, and ran with exit 0. Additive API without changes to existing layouts, virtual ABI, or history. No combined mode, savings, automatic restore, localized text, or svchost group promised. L05/M02/M06/N01/N03 open. C52/E14/P6/B3, 52/75=69.3%, 23/75=30.7%. Mirror 37 confirmed: comment 90140270623147; no new IDs or fractions and no change in criteria.

Previous unit, revision 36: M07 E->E. Authorized real SCM control with UAC, using a dedicated manual LocalService test service without dependencies. Public start/stop/restart/idempotency and status/PID/description/group inventory passed. Copy in ACL-protected ProgramData; no profile permission changes or LocalSystem. 60-second watchdog; registry entry, copy, and directory removed, and absence rechecked. RESULT PASS. Manual harness tests/windows_service_native.cpp, NOT CTest; WINDOWS_SERVICES.en.md. Initial Win32=5 from the profile cleaned up and evidence preserved. Guards without opt-in or privileges verified. Windows 119/119, Linux 108/108 Release. No new production code, API, ABI, layouts, or history; no consumer migration needed. M07 still needs opening service details and metadata availability; production UAC in L05. C51/E15/P6/B3, 51/75=68.0%, 24/75=32.0%; no overall closure or new IDs. Mirror 36 confirmed: comment 90140270598882; remote title/statuses unchanged.

Previous unit, revision 35: E07 P->C. Read-only ext2/3/4 query by region or disk ID without mounting. Covers sysfs partitions and whole devices; checks expected major:minor and revalidates identity, layout, and diskseq. Reports totals only if ALL regions are known and non-overlapping, without partial results, aliases, or stacked layers. Declared capacity includes metadata; it is NOT usable/free statvfs capacity or a universal physical total. LINUX_FILESYSTEM_CAPACITY.en.md. Native tests passed with real 32 MiB images, a 32 MiB filesystem in a 48 MiB source, WSL 8:48/sdd at 1 TiB, checksum validation, and disk-level query. No real disks or mounts changed; no new SDK CLI or dependencies (formatter only in the harness). Linux 108/108, Windows 119/119; both installed consumers rebuilt, relinked, and ran with exit 0; builds warning-free. Additive API; no changes to existing layouts, virtual ABI, or history. Other formats, raw MBR/GPT, and physical attribution across RAID/LVM/loop remain unknown. N02/N03 open, F03 deferred. C51/E15/P6/B3, 51/75=68.0%, 24/75=32.0%. Mirror 35 confirmed: comment 90140270531513.

Previous unit, revision 34: H05 P->C. Opt-in LinuxNotificationObserver, consent, privacy/attribution/period policy, and a positive native test. Correlates Notify and replies using owner/sender/serial; counts accepted calls and replacements, not notifications seen or read. Rejected, unacknowledged, pending, and expired calls tracked separately. Declared app hint is estimated; unknown without a hint, with no PID/exe/CPU join. No content retention, CLI, downloads, autolaunch, elevation, or persistence. LINUX_NOTIFICATIONS.en.md. Private native test: three accepted, one replacement, one rejected, one unknown app, one unacknowledged, plus actual denial and owner loss. Ten out of ten repetitions and direct execution exit 0, no skip. Linux 106/106, Windows 118/118; both installed consumers rebuilt, relinked, and ran with exit 0; builds warning-free. Additive API without new existing-type layouts, virtual ABI, SystemSnapshot layout, or history. No universal GTK/portal/vendor protocol or retroactive coverage. H04/M08/F03/N03 open. C50/E15/P7/B3, 50/75=66.7%, 25/75=33.3%. Mirror 34 confirmed: comment 90140270530011.

Previous unit, revision 33: I05 E->C. AppStream provider, source policy, and handling of unavailable data; closure criteria unchanged. Declared publisher is estimated from an explicit, unique desktop-ID match; not the executable's signer or owner. Public query and opt-in Monitor; optional system libxml2, no install, helper, or download. Errors, absence, or ambiguity clear stale labels and leave the value unknown. LINUX_STARTUP_PUBLISHER.en.md. Native XML/Monitor with private files exited 0 without skipping; DTD/XXE rejection, permissions, duplicates, languages, limits, and incomplete reads tested. Linux 104/104, Windows 117/117; both installed consumers rebuilt, relinked, and ran with exit 0; no final warnings. New SampleOptions 0.2 flag: rebuild ALL consumers; no change to StartupEntry, virtual ABI, or history. No universal source/vendor/Flatpak/Snap inventory, and no closure of I07/K03/F03/N03. C49/E15/P8/B3, 49/75=65.3%, 26/75=34.7%. Mirror 33 confirmed: comment 90140270529005.

Previous unit, revision 32: E05 E->C. Authorized test of a REAL active private 64 MiB swap file on WSL/ext4. /proc/swaps (65,532 KiB, zero used) -> sdd -> native reader/report -> Monitor.contains_page_file=true, estimated. swapoff/unlink/rmdir completed; existing swap untouched, no changes to fstab or permanent configuration and no tools installed. LINUX_SWAP_DISKS.en.md. Manual linux_swap_active_native.cpp test (NOT CTest), mandatory flag/guard exit 77. Linux 102/102, Windows 116/116; both consumers exit 0; no warnings. No new API, layouts, or history. Scope: partitions and ext2/3/4 with conservative topology handling; no universal kernel-inode mapping or XFS/Btrfs/F2FS/RAID/LVM coverage. C48/E16/P8/B3, 48/75=64.0%, 27/75=36.0%. Mirror 32 confirmed: comment 90140270527425. Remote statuses/title unchanged. F03 deferred, N03 open.

Previous unit, revision 31: J06 P->C. Explicit Manage user accounts integration through the host GUI. AccountManagementIntegration query/open rechecks capability and returns a host_invoked receipt even on errors or exceptions; without a provider it runs nothing. No bundled native manager, universal panel, CRUD, elevation, or guessed commands. Host supplies the UI/manager; callbacks are not sandboxed. ACCOUNT_MANAGEMENT.en.md. Public API passed with a private child, not the real accounts UI. Linux 102/102, Windows 116/116; both installed consumers rebuilt, relinked, and ran with exit 0; no warnings. Additive API without new virtual ABI or changes to existing layouts/history. C47/E17/P8/B3, 47/75=62.7%, 28/75=37.3%; only J06 P->C, no new IDs or fractions. F03/J04/N03 open. Mirror 31 confirmed: comment 90140270525751.

Previous unit, revision 30: F03 E->E. Private identity and quality: TGID/start_boot/UID plus active/sequence/sticky-overflow guards. Authorized native test passed: threads with the same leader, negative UID/start/overflow cases, child and detach checks, TCP 6144/6144, UDP 384/384, and 12 peeks. LINUX_NETWORK_QUALITY.en.md. Linux 101/101, Windows 115/115; both consumers exit 0; no warnings. Overlap rejected in fixtures but not observed live (10 consistent reads, zero busy). No integrated producer, netns, join, public API, ABI, or history changes. F03 E, J04/H04 open, N03 unchanged. 46/75=61.3%, 29/75=38.7%, unchanged. At the user's request not to prolong this, defer F03 integration and move to another open ID. Mirror 30 confirmed: comment 90140270524925.

Previous unit, revision 29: F03 E->E. Private four-lane TCP/UDP IPv4/IPv6 classification by socket/BTF, with read-failure handling; positive results flagged PEEK/TRUNC/ERRQUEUE are excluded. LINUX_NETWORK_CLASSIFIED.en.md. Authorized native send/sendto+sendmsg test: TCP 4096/4096, UDP 256/256, zero failures, eight peeks excluded; child and removal of three links verified, exit 0. Legacy 1088/1088 still passes. Linux 101/101, Windows 115/115; both installed consumers exit 0, builds warning-free. Temporary root, no pinning, permanent configuration, or installed utilities; native test is manual, never run with probes under CTest. No public API, ABI, history, or complete producer for UID/start_boot/netns, durable identity, saturation, consistency, or integration. F03 E, J04/H04 open, legacy network metric absent, N03 unchanged. 46/75=61.3%, 29/75=38.7% open, no new IDs or fractions. Continue F03 identity/quality. Mirror 29 confirmed: comment 90140270524004; remote statuses/title unchanged.

Previous unit, revision 28: F03 E->E. Authorized private eBPF test passed for __sys_sendto/sock_recvmsg with correct BTF signatures and slots. Kernel TGID obtained via unattached TEST_RUN; filter precedes context/map access. TCP 1024 + UDP 64 = 1088/1088; phases, excluded child, and detach verified. Temporary root, RAII, no pinning or permanent configuration. LINUX_NETWORK_PROBE.en.md. Linux 100/100, Windows 114/114; both installed consumers exit 0. Initial failures recorded: helper 14 in SOCKET_FILTER, sock_sendmsg 0/1088, and __sock_sendmsg absent from BTF. No new public API, ABI, or history. NO real four-lane producer/UID/start_boot/netns/loss/coverage or reducer/Monitor/legacy networking integration. F03 E, J04/H04 open, N03 unchanged. 46/75=61.3%, 29/75=38.7% open, no new IDs or fractions. Continue F03 producer/coverage/identities. Mirror 28 confirmed: comment 90140270523263; remote statuses/title unchanged.

Previous unit, revision 27: F03 E->E. Internal TCP/UDP/UID reducer for rates, windows, losses, identity, coverage, and overflow, without treating absent data as zero. LINUX_NETWORK_ACCOUNTING.en.md. Fixtures and an oracle for 10,000 cases, no real capture. Final Linux 99/99 and Windows 113/113; both installed consumers exit 0. Initial Windows sandbox 98/113 (15 environment/access failures); 113/113 outside the sandbox without weakening tests. Logs preserved. No new public API, ABI, history, producer, loader, or probes. F03 E while real provider, coverage, integration, and positive tests are pending; J04/H04 open and legacy network metric absent. On 2026-10-18 the user explicitly authorized a temporary WSL test limited to private processes with necessary privileges and final cleanup, without permanent configuration or tool installation. No loading in this unit. N03 remains open. C46/E17/P9/B3, 46/75=61.3%, 29/75=38.7% open, no new IDs or fractions. Continue F03 producer/loader/integration. Mirror 27 confirmed: comment 90140270522304; remote title/statuses unchanged.

Previous unit, revision 26: F03 E->E. Read-only internal BTF preparation with compatible signatures, types, and offsets, without external utilities or kernel constants. LINUX_NETWORK_BTF.en.md. Linux 98/98 (native BTF passed), Windows 112/112; both installed consumers relinked and ran with exit 0. Initial fixture corrected without relaxing the parser; failure and final logs preserved. No new public API, ABI, or history. NO loader, probes, maps, events, or TCP/UDP accounting implemented here. Future private-probe test pending explicit authorization; no load, attach, or elevation. F03 E, J04/H04 open, legacy network metric absent, N03 unresolved. 46/75=61.3%, 29/75=38.7% open, C46/E17/P9/B3, no new IDs or fractions. Continue F03 with loader, accounting, and authorized tests. Mirror 26 confirmed: comment 90140270363748; remote statuses/title unchanged.

Previous unit, revision 25: F03 E->E. TCP holder query by PID/token/FD, using pidfd_getfd for external processes and F_DUPFD_CLOEXEC for self. Checks procfs and the projected Pid from fdinfo, PID/start_ticks/pidfd liveness, and slot cookie/dev/inode before and after. No proc-fd/inode fallback, elevation, or changes to data or flags; duplication briefly extends socket lifetime. A holder is not the exclusive or causal source; shared cookies are not independent traffic. LINUX_TCP_HOLDERS.en.md. Linux 96/96, Windows 111/111; both installed consumers rebuilt and relinked, exit 0. Native: 8192 B acknowledged and received, with positive private-child pidfd_getfd and the same cookie as the parent. FD replacement, token, UDP, listener, flags, and data verified. F03 still E pending causal/exclusive attribution, UDP/other coverage, and integration. J04/H04 open, legacy network metric absent. No new existing-type layouts, virtual ABI, history, IDs, or fractions. 46/75=61.3%, count unchanged. Continue F03. Mirror 25 confirmed: comment 90140270363050. N03 risks unchanged.

Previous unit, revision 24: F03 P->E. Opt-in LinuxTcpSampler with SOCK_DIAG/TCP_INFO, no CLI/eBPF/elevation. Counters and rates by cookie/family/inode/socket_uid and thread namespaces; estimated partial sum by SOCKET UID, not real UID, PID, app, or total network use. TCP ESTABLISHED/CLOSE_WAIT only; UDP/other coverage, PID/app/shared-socket attribution, and accounting/integration pending. LINUX_TCP.en.md. Linux 94/94, Windows 110/110 with no native TCP skip; both installed consumers rebuilt and relinked, exit 0. Own loopback: 8192 B acknowledged and 8192 B received, two sockets/one UID. No benchmark, capture of other traffic, or changes to existing layouts, virtual ABI, or history. J04/H04 remain open, legacy network metric absent. 46/75=61.3%, no new IDs or fractions. Continue F03. Mirror 24 confirmed: comment 90140270361819.

Previous unit, revision 23: J04 P->E. Visible disk I/O by numeric UID, with LinuxUserIoDetails and coverage; usernames are not I/O keys. Observed real UID must match around reads and at both ends of the D02 interval; absent, changed, or denied UID discards interval attribution even if the process rate is still known. Explicit estimated coverage and partial sums; repeated UID shares its total, utmp without UID gets no I/O. F03 networking pending: J04 NOT closed. LINUX_USER_IO.en.md. Linux 92/92, Windows 109/109; installed consumers rebuilt and relinked, exit 0. Native UID 1000: 8,779,070 B/s, four known and two absent after using a private temporary file; not a benchmark. One public report with hidden processes. New ProcessInfo/UserSession/SystemSnapshot 0.x layout: rebuild ALL consumers. 46/75=61.3%, no new ID or fraction. Next dependency F03 started in revision 24. Mirror 23 confirmed: comment 90140270360992; previous limitations and risks unchanged.

Last closed unit at this point, revision 22: K04. Inventory, detail view, and control of the effective user's systemd manager through separate LinuxServiceTarget/Inventory/View APIs with system/current_user scope, UID, and observed owner. Fixed local bus without environment redirection; peer and owner UIDs verified, destination and sender pinned, and owner revalidated. No GUI, shell, elevation, or fallback. Confirmed jobs for Start/Stop/Restart, with errors and unknown outcomes. LINUX_USER_SERVICES.en.md. Linux 91/91, Windows 108/108; installed consumers rebuilt and relinked, exit 0. WSL read-only: 199 system services and 19 user services, with details opened in both scopes. New control tests passed with a simulated loader; no real services changed. No changes to existing layouts or virtual ABI and no persistent token. K04 P->C: 46/75=61.3%. J04 proposal started in revision 23. Mirror 22 confirmed: comment 90140270360264. K03/L05/N01/N02 and prior risks remain separate.

Previous closed unit, revision 21: G06. DRM/sysfs/NVML audit and fixes. Bounded reads per field, FD, and binding; PCI subsystem and actual module version rather than a name passed off as a version. LinuxGpuSysfsDetails reports VRAM/GTT and labeled sensors; headroom is estimated, not exact free memory, inferred Windows-style shared memory, or a process budget. NVML exposes the complete, consistent tuple by PCI when available and the dedicated_used alias. linux_gpu_inventory_source distinguishes opt-out, empty, absent, and partial inventory. LINUX_GPU_TELEMETRY.en.md. Linux 89/89, Windows 107/107; installed consumers rebuilt, relinked, and ran with exit 0. WSL: zero DRM rows, with estimated coverage, and NVML absent; reader/field positives come from fixtures. G07/N02 physical validation and M04 remain separate. New GpuInfo/SystemSnapshot 0.x layout: rebuild ALL consumers. 45/75=60.0%, without a new ID or denominator. L03 B and other limits/N03 risks were not implicitly resolved. K04 proposal made in revision 22. Mirror 21 confirmed: comment 90140270359672.

Closed unit, revision 20: G08. Linux GPU driver-date source policy and reasoned absence, NOT a source for an actual date. Original criteria allow reasoned absence: no verified release/install date source in DRM/sysfs/NVML; driver_date is absent/unsupported, with the reason by source or merge. No inference from BIOS, mtime, builds, packages, kernel, or API, and no DRM0 placeholder. LINUX_GPU_DRIVER_DATE.en.md. Linux 88/88, Windows 106/106; installed consumers rebuilt, relinked, and ran with exit 0. WSL: zero GPU rows; policy/merge positives come from fixtures. No API, ABI, layout, or format changes; no new GPU queries or dependencies. G08 P->C: 44/75=58.7%, 31 open. G06/G07/N02/M04 and L03 B still pending; G06 audit proposal not started. Mirror 20 confirmed: comment 90140270359048.

Closed unit, revision 19: G05. EGL/GLX context by PCI and declared GLX profile capabilities, kept distinct from GL_VERSION in LinuxOpenGlDetails/Metric. Direct GLX/Mesa with interop ABI 1; IDs, acceleration, sysfs, context, display, and screen verified. Changes, ambiguity, and duplicates rejected. LINUX_CURRENT_GLX.en.md. Linux 87/87, Windows 105/105; installed consumers rebuilt and ran with exit 0; four EGL/GLX tests repeated ten times each. Dynamic GLX and private sysfs fixture; native WSL had no GLX context and zero attributions, not a positive hardware or headless test. G07/N02 and M04 remain separate. New GpuInfo 0.x layout: rebuild ALL consumers. 43/75=57.3%, no new IDs or denominator. L03 remains B without acceptance of the limit; G08 proposal not started. Mirror revision 19 confirmed: comment 90140270358672.

Revision 18: L03 E->B. Decision pending on whether to accept the current limited provider or choose an additional cooperative/invasive design. LINUX_EFFICIENCY_DECISION.en.md. No helper, tracing, or target-app integration without a decision. Numeric syscalls and sampled lists do not provide checked semantics or universal coverage. Diagnostics/documentation only; no new code or tests, with revision 17 evidence preserved. Count unchanged at 56.0%. Mirror 18 confirmed: comment 90140270357868.

Unit, revision 17: L03 partial. Opt-in unchecked Linux API for captured threads, with a receipt containing original nice/ioprio values and flags for outstanding writes. Exact restore by field, observed conflicts, full receipt validation, and retries. No automatic rollback, elevation, or false restoration to defaults 0/BE4. Linux 85/85, Windows 105/105; installed consumers rebuilt and ran with exit 0; 20/20 Linux repetitions. Native private two-thread children: I/O restored and explicit CPU denial 13 with partial receipt. LINUX_EFFICIENCY.en.md. L03 remained E at revision 17 (B at revision 18) because checked identity and full coverage, including new threads, were missing. A stat check cannot eliminate reuse between syscalls. No existing layout or virtual method changed. Count unchanged: 42/75=56.0%. Mirror revision 17 confirmed: comment 90140270357119.

Task K01 closed in revision 16. systemd metadata/status through Metric, with group=Slice or empty, and separate provider/group_kind fields; not a Windows or Unix group. Known MainPID 0 and empty strings distinguished from absence or denial. Unit.Id and owner verified, manager source distinguished from partial fallback, and effective metadata not invented from files. LINUX_SERVICE_METADATA.en.md. Linux 85/85, Windows 105/105; installed consumers rebuilt and relinked, exit 0. WSL: 199 services, 117 known MainPID and 117 known Slice values (15 empty Slice values); no real services created or controlled. New ServiceInfo/SystemSnapshot 0.x layout: rebuild ALL consumers. K03/K04/L05/N01/N02 still pending. L03 proposal started in revision 17; checked semantics and full scope still pending.

Unit, revision 15: J01. LinuxSessionDetails reports State/Active/Remote/UID and per-field sources; verified metadata and identity with explicit inventory coverage for login1/utmp/empty/absent. No WTS equivalence or global resource total; sums still reflect visible resources by username. LINUX_SESSION_STATE.en.md. Linux 85/85, Windows 105/105; installed consumers rebuilt, exit 0. WSL: one session, one known State, and one known foreground flag; private utmp tests covered three records, empty, and absent. No real sessions controlled. New UserSession/SystemSnapshot 0.x layout: rebuild ALL consumers. J04/J05/J06 and N02 were not closed. K01 subsequently closed in revision 16.

Unit, revision 12: A01. Metric contract and common AppHistorySample view with availability and origin, explicit legacy alias, and import without promoting zeros or positive values to measurements. See METRIC_AVAILABILITY.en.md. Does not remove compatible raw types or close backend coverage, per-field audit, or API 1.0. A01 validation: Linux 83/83, Windows 103/103; installed consumers rebuilt, exit 0. Additive API without changes to existing layouts or v1/v2 formats. C03/E05 open. N03 P->E: CPU diagnostics completed by reason, including regression state/values, with shared evaluation. Integration test prints the note before the original assertion; formula and guards unchanged. Fixtures and 100 native repetitions passed; historical failure not reproduced and cause not demonstrated. N03 remains open; LINUX_CPU_USAGE.en.md. Suites 83/83 and 103/103 and relinked installed consumers with exit 0 repeated in revision 13.

Revision 14: E06 P->E. Reported SAT/SCSI/NVMe identity validated with separate protocol, transport, medium, and RPM. LINUX_DISK_IDENTITY.en.md. E06 open pending exact media detection for remaining cases or explicit acceptance of the unknown limit (no answer assumed). Final Linux 84/84, Windows 104/104; installed consumers rebuilt, exit 0. New DiskInfo/SystemSnapshot 0.x layout: rebuild ALL consumers. WSL: four contexts and zero known media; physical positives from fixtures only. First suite 83/84 due to missing history with disks disabled; logs preserved, followed by 3/3 and 84/84. Test note added, logic/assertions unchanged. Cause pending under N03 and not attributed to CPU.

## Implemented on Linux

| Area | Status | Source without subprocesses |
|---|---|---|
| Static and dynamic CPU | Implemented with limits; validated utilization, deduplicated online caches/topology, and separate current/base frequencies without partial averages | procfs, cpufreq, sysfs, SMBIOS; see LINUX_CPU_USAGE.en.md, LINUX_CPU_CACHE.en.md, LINUX_CPU_TOPOLOGY.en.md, LINUX_CPU_FREQUENCY.en.md, and LINUX_COUNTS.en.md |
| Linux virtualization | Explicit unknown firmware status; separate KVM API query without inferring BIOS enablement from a file | sysfs misc + lstat/open/fstat + KVM_GET_API_VERSION; LINUX_VIRTUALIZATION.en.md |
| Kernel memory | Validated reads; total with availability, missing values not replaced by zero, estimated derived fields, and checked sums | `/proc/meminfo`; LINUX_MEMORY.en.md |
| Composition/compression/reservation | C03 closed with accepted limits: estimated Linux breakdown, non-additive categories, separate zram/zswap, and installed-minus-usable estimated/unknown; NOT an exclusive hardware reservation | meminfo/sysfs/SMBIOS; LINUX_MEMORY_COMPOSITION.en.md; no equivalence to Windows pools/composition; physical validation separate under N02 |
| Physical RAM | Implemented if firmware is accessible | Built-in SMBIOS parser |
| Process CPU/RAM/I/O/user/ELF | D01..D04 closed with limits: RSS, I/O, details, and estimated per-process CPU; per-app networking pending | procfs, NSS, XDG; LINUX_PROCESS_CPU.en.md, LINUX_PROCESS_DETAILS.en.md, LINUX_PROCESS_DESCRIPTION.en.md, LINUX_PROCESS_MEMORY.en.md, and LINUX_PROCESS_IO.en.md |
| Block devices, mounts, and rates | Estimated performance from independent baselines; validated visible capacity and explicit rotational hint; deduplicated mounted capacity estimated, with no zero substitution | sysfs stat/dev/diskseq/size/rotational, mountinfo, fstatvfs; LINUX_DISK_RATES.en.md / LINUX_DISK_PROPERTIES.en.md / LINUX_FORMATTED_CAPACITY.en.md |
| ext capacity outside mounts | E07 closed: explicit region/disk-ID queries, declared ext2/3/4 capacity including metadata, sysfs/partitions/expected identity, and total only if all regions are known and non-overlapping. No universal physical total or statvfs replacement | LINUX_FILESYSTEM_CAPACITY.en.md; opt-in read-only raw access, no SDK CLI; other formats/layers not inferred |
| Disk identity | E06 closed with accepted reported/unknown scope: protocol, device, transport, and medium separated, with per-field source/support. No SSD inference from name, rotational flag, or NVMe | SAT89 ATA IDENTIFY, SCSI B1/VPD83, NVMe transport in sysfs; LINUX_DISK_IDENTITY.en.md; no universal exact media detection, physical validation separate under N02 |
| Per-adapter network | Implemented; estimated rates with name/ifindex continuity, handling resets and pauses without false spikes | getifaddrs and sysfs; LINUX_NETWORK_RATES.en.md |
| Network identity | F02 closed: PCI/USB/model/vendor from optional sources, separate driver/bus/IDs, and evidence-based type; no universal retail name or default to Ethernet | sysfs/optional local pci.ids; LINUX_NETWORK_IDENTITY.en.md; additional physical validation under N02 |
| General GPU | Partial, depending on driver | DRM, PCI, hwmon |
| PCI GPU names | Numeric DRM IDs and estimated local labels implemented; commercial model name not guaranteed | Optional existing pci.ids, no utilities/downloads; LINUX_GPU_NAMES.en.md |
| Per-GPU Vulkan/OpenCL | Versions matched by unique PCI identity tested with synthetic APIs/loaders; physical Linux validation pending | LINUX_GPU_VULKAN.en.md / LINUX_GPU_OPENCL.en.md; no loader/platform fallback |
| Sessions and per-user usage | J01 closed with limits: status/foreground/source/UID through Metric, with selected source known/empty/absent; utmp does not prove activity. Visible CPU/RAM grouped by username, not session; other resources under J04 | utmpx/login1 with identity and owner verified; LINUX_SESSION_STATE.en.md / LINUX_USERS.en.md / LINUX_LOGOUT.en.md |
| Desktop startup | Verified flags, precedence, OnlyShowIn/NotShowIn/TryExec, and file/GSettings/GNOME3-session conditions; opt-in AppStream publisher with explicit, unambiguous match and source/absence policy. Other mechanisms are not presented as XDG apps; automatic impact pending under I07 | XDG + stat + optional GIO/sd-bus/libxml2; XDG_PRECEDENCE.en.md, XDG_ELIGIBILITY.en.md, XDG_FILE_CONDITION.en.md, XDG_GSETTINGS.en.md, XDG_SESSION_CONDITION.en.md, and LINUX_STARTUP_PUBLISHER.en.md |
| Services | K01/K02/K04 systemd verified: Metric/Slice metadata, separate system and effective-user inventory, details and explicit control by scope; other init systems under K03 and helper under L05 pending | D-Bus sd-bus; LINUX_SERVICE_METADATA.en.md, LINUX_SYSTEMD.en.md, LINUX_SERVICE_ACTIONS.en.md, and LINUX_USER_SERVICES.en.md |
| Windows services/actions | M07 verified: SCM inventory/control, open_windows_service details with availability, launch/checked end/EcoQoS; private native positives. Raw description/load-order group, not svchost grouping or combined mode; L05 UAC helper pending | WINDOWS_SERVICES.en.md, WINDOWS_SERVICE_VIEW.en.md, WINDOWS_LAUNCH.en.md, WINDOWS_TERMINATE.en.md, WINDOWS_EFFICIENCY.en.md |
| Per-app CPU history | H01 closed: estimated observed linux_app_history, exe/PID continuity and conversion with remainder, opt-out/reset; no new persistence, networking, or notifications in that report | procfs, CLK_TCK; LINUX_APP_HISTORY.en.md; H04 open, independent H05 source in LINUX_NOTIFICATIONS.en.md, A01 closed later |
| Observed Linux notifications | H05 closed: explicit observer with consent, accepted/replaced/rejected/unacknowledged Notify calls and periods, declared and unauthenticated app hint; no displayed-notification count or CPU join | D-Bus/optional libsystemd, BecomeMonitor permission; no retained content or persistence, partial FDO source; LINUX_NOTIFICATIONS.en.md |
| Dynamic 60 s window | A03 closed: steady-clock filter by start, start/end timestamps, and expiry on read even when stopped; automatic capacity by interval, explicit capacity may truncate | `pcinsight::Sampler`; SAMPLER_WINDOW.en.md; no exact cadence or atomic snapshot |
| Run new task | Validation and chdir/exec errors verified; structured PID, caller reaps children | fork/execve + CLOEXEC pipe; LINUX_LAUNCH.en.md |
| End task | PID+start_ticks, pidfd signaling, and exit confirmation verified; token limits documented | procfs + pidfd, kernel 5.3+; LINUX_TERMINATE.en.md |
| Linux efficiency preset | L03 closed with accepted limited advanced scope: apply/restore on captured threads with acknowledgement/receipt; generic and checked actions unsupported with no fallback. NO guaranteed stable identity, coverage of future threads, savings, or restore | Native nice/ioprio; LINUX_EFFICIENCY.en.md and LINUX_EFFICIENCY_DECISION.en.md; 0.x behavior change |
| Enable/disable/reset startup | Direct editing, personal override, and recoverable reset verified; vendor conditions/impact pending | XDG by group + personal publication/archiving; XDG_OVERRIDE.en.md, XDG_RESET.en.md, and LINUX_STARTUP.en.md |
| Startup Properties | Verified read-only query for effective file, size/fields/groups, and conditions; not properties of the Exec binary | XDG + native reads; XDG_PROPERTIES.en.md |
| Disconnect user on Linux | Availability limitation justified for utmp/login1 adapter; static API with reason and inventory. No actual detach implemented, nor a claim of universal impossibility | LINUX_DISCONNECT.en.md; J05 closed under alternative criterion, without implicit SIGHUP/logout; Windows under M06 |
| Explicit logout | login1 provider verified with a real private PAM session: identity, termination, audit, and cleanup | LINUX_LOGOUT.en.md; Terminate does not replace Disconnect |

## Verified Windows D3D12 capability

`GpuInfo.direct3d12_feature_level`: queries an explicit DXGI adapter without
creating a device, candidates 11_0..12_2; not the installed DirectX version.
A runtime that does not recognize higher candidates yields an estimated lower bound.
See WINDOWS_GPU_D3D12.en.md. Windows 50/50 and Linux 35/35; packages and installed
consumers verified. Other Windows runtimes and temperature remain pending.

## Partial or pending

A01 closed through the documented replacement: AppHistorySample/app_history_view
and metric_is_known distinguish measured zero from absence; import_legacy_history
sets all counters to unknown, and LegacyAppHistoryEntry is the explicit raw alias.
Existing v1/v2 formats, symbols, and layouts remain unchanged. No modern shared
or Linux-specific storage added. METRIC_AVAILABILITY.en.md. Linux 83/83, Windows
103/103, and installed consumers exit 0. This does not close A05/N03 or capture
work H04/H05/M08.

E05 E->E (LINUX_SWAP_DISKS.en.md): checks sizes in KiB/bytes and FD/path metadata
before and after resolution; detected changes leave the result unknown. The new
linux-swap-path test forces races. Native regular-file pipeline 8:48->sdd uses
SYNTHETIC inventory, NOT an active swap file. Real WSL had only the /dev/sdc
partition. An active-file test for supported topologies was still needed; swap
was not modified to force closure. Linux 82/82, Windows 102/102, relinked consumers
exit 0; no new public layout. That unit stayed at 39/75=52.0%; A01 closed later
in revision 12.

F02 closed (LINUX_NETWORK_IDENTITY.en.md): per-adapter LinuxNetworkDetails with
verified ifindex/type/directories and PCI/USB sources with labels and explicit
missing fields. Legacy model/connection_type carry a value or are empty. No
assumption about physical medium, no universal exact retail name, and no invented
virtual classes. Final Linux 81/81, Windows 101/101, rebuilt consumers exit 0.
Native WSL loopback/vmbus; positive PCI/USB/Wi-Fi cases are fixtures. New
NetworkAdapter 0.x layout. The isolated CPU failure recurred in the first suite;
logs were preserved, then integration 3/3 and suite 81/81 passed without changing
assertions. N03 remains open. Next proposal: E05.

C03 E->E: new MemoryInfo.linux_details with MemTotal/MemAvailable/MemFree breakdown,
Zswap/Zswapped, and per-device zram, with explicit absence, limits, and identity.
The legacy reservation proxy is now also exposed as installed_minus_usable_bytes.
This is not an implementation of exclusive hardware reservation. An exclusive
provider or explicit user acceptance of the limit was still needed. See
LINUX_MEMORY_COMPOSITION.en.md. Linux 80/80, Windows 100/100, rebuilt installed
consumers exit 0. New MemoryInfo/SystemSnapshot 0.x layout: rebuild ALL consumers.
Native WSL had no zram, Zswap, or firmware difference; positives for those
providers come from fixtures, not hardware. Count still 38/75=50.7%. This does
not close N02, CI, or the earlier N03 risk.

A03 closed (SAMPLER_WINDOW.en.md): inclusive [steady_now-60s,steady_now] interval
by collection start, with expiration on queries and a separate older latest
sample. SamplerSample/latest_timed/timed_history, and automatic capacity when set
to 0: 61 at 1 s or 601 at 100 ms. A new run clears the graph and retains Monitor;
explicit capacity caps points and may shorten the window. Synthetic 60-second,
native, and lifecycle tests. Linux 79/79, Windows 99/99, and installed consumers
exit 0. 0.x API defaults and behavior change; no new SystemSnapshot layout in A03.
C03 started later in revision 9.

H01 closed (LINUX_APP_HISTORY.en.md): observed CPU duration, not a historical total
or percentage. Validates exe/path/PID/start/stamp, utime/stime, an independent clock,
context/boot, and units; exact conversion with remainder, with gaps and overflow
handled without partial commit. Legacy app_history is empty; new linux_app_history
uses Metric per field. HistoryStore does NOT serialize it; unknowns are not
converted to 0. New public SystemSnapshot 0.x layout: rebuild ALL consumers.
Linux 78/78, Windows 98/98, and installed consumers exit 0. H04/H05/A05 still open
at this checkpoint; A01 closed later. The cause of the earlier isolated global
CPU failure remains pending under N03. A03 closed later in revision 8.

D04 closed (LINUX_PROCESS_CPU.en.md): independent clock after stat, separate
utime/stime, and positive CLK_TCK. Validates PID/start/global context/counters and
the 0..100 range without clamping. Normalized by visible host capacity, not
quota/affinity or frequency-weighted utilization. Guest time is not double
counted; child cutime/cstime is excluded. No promise that windows or sums match
global CPU. Permission issues, absence, and opt-out reset baselines. Linux 77/77,
Windows 97/97, reinstalled/relinked consumers exit 0. No new public layout in D04.
H01 closed later in revision 7 and A01 in revision 12.

D03 closed: bounded stat/status, PID/start before and after, four strictly
validated UIDs and NSS, pinned regular executable and validated ELF.
ProcessInfo.linux_details exposes per-field Metric and an estimated XDG Comment
description/source. Matching uses the file stamp, not the name; missing eligible
metadata or ambiguity remains unknown. Conservative Exec subset without
localization; no certified commercial identity or coverage of all apps. No atomic
snapshot or universal exec generation. LINUX_PROCESS_DETAILS.en.md and
LINUX_PROCESS_DESCRIPTION.en.md detail sources, limits, and tests. Linux 76/76,
Windows 96/96, reinstalled/rebuilt consumers exit 0. New 0.x API layout: rebuild
library and consumers. This does not close A01, process CPU, or history. The
isolated CPU absence from an earlier run remains recorded in N03 and is not
claimed fixed by this change. Official count 35/75=46.7%.

Linux per-process I/O fixed (LINUX_PROCESS_IO.en.md): separate read_bytes and
write_bytes, validation on each reset, stat/io/stat identity through a pinned
directory, bounded reads, and an independent clock. Absence is not zero; the
first sample and pauses reset the baseline. Estimated bytes/s, not physical
device throughput. Linux 74/74 and Windows 94/94 Release; native self-test observed
a 131072 B write delta. No new public layout. Installation/consumer checks were
not repeated in this unit. CPU/history/UID/State/ELF and whole-row consistency
remained pending. ClickUp blocked by quota.

Linux per-process RSS fixed (LINUX_PROCESS_MEMORY.en.md): exact uint64/kB VmRSS
with checked multiplication by 1024; absence is not zero. Rejects duplicates,
PID/text/bounds errors. Support::estimated because accounting is asynchronous.
RSS includes shared pages: not private memory/PSS and not identical to Windows.
Pinned O_PATH directory, bounded stat/status/stat via openat, with matching
PID/start_ticks and required Pid in status. Missing or changed identity is not
assigned memory. Linux 73/73, Windows 93/93, installation/consumers exit 0. Recorded
native self-test PID 1539/start 5173: RSS 4538368 B and Monitor 5976064 B estimated;
incorrect token rejected without forcing actual PID reuse. No changes to API,
layouts, Windows, other processes, permissions, or memory pressure. This does not
close PSS/smaps, UID/State reads, CPU/I/O/history, or atomic consistency for the
whole row. Token limited to boot/namespace and tick resolution. Actual permissions
were not changed for testing.

CPU identity fixed (LINUX_CPU_IDENTITY.en.md): bounded procfs, unique processor
records, and a common, complete text summary per field, not an arbitrary first
row. Uses model name/cpu model/cpu keys and vendor_id; no board names or assumed
implementer/part lookup. SMBIOS Type 4 Manufacturer/Version uses string sets and
indices, only for populated central processors with common values. Estimated
fallback when proc lacks a name, without hiding observed mixtures or incomplete
data. UTF-8, control characters, text bounds, and reset tested. New
CpuInfo.reported_vendor/reported_model Metric<string> fields. Empty legacy fields
mean unknown and reflect the Metric values. Windows reports the CPU0 registry
entry as estimated/representative, not a summary of all packages. Linux 72/72,
Windows 92/92, rebuilt consumers exit 0. WSL: 32 records with AuthenticAMD /
AMD Ryzen 9 7950X 16-Core Processor. ARM/MIPS/PowerPC and positive SMBIOS covered
by fixtures; native non-x86 hardware and positive real firmware pending. No
universal commercial catalog or atomicity with online CPU state. Firmware cached
at Monitor construction. 0.x API: new CpuInfo/SystemSnapshot layout; rebuild the
library and all consumers. This does not close all CPU work or the project.

Linux virtualization fixed (LINUX_VIRTUALIZATION.en.md): removed the assumption
that exists(dev/kvm) means firmware virtualization is enabled.
virtualization_enabled remains unknown. Separate Metric<unsigned>
CpuInfo.linux_kvm_api_version confirms only an API 12 query on a character-device
FD matching the sysfs KVM entry before and after. Prior lstat, read-only open with
no symlink following, and fstat on the same FD. Wrong files, directories, FIFOs,
and devices are rejected before ioctl. Explicit permission errors, without
invented false/0 or retained previous state. Linux 71/71 and Windows 91/91,
installation and rebuilt consumers exit 0. WSL missing KVM registration (ENOENT),
with native absence verified; positive API 12 is a classification fixture only,
not a hardware validation. No VM created, MSR or BIOS access, module changes, or
state changes. Direct firmware reads and other evidence/architectures remain
pending; this does not close all virtualization work. 0.x API: new
CpuInfo/SystemSnapshot layout; rebuild all consumers. Windows firmware behavior
unchanged and the new field is absent there.

Linux uptime fixed (LINUX_UPTIME.en.md): procfs read bounded to 256 B, exactly two
fields with two decimal digits, and checked integer conversion to chrono::seconds.
No stod, prefixes, NaN/Inf, rounding, or out-of-range conversions. Zero is valid;
cumulative idle can exceed uptime and is never used as boot time. Missing data
and errors clear old values and do not produce zero. Linux 70/70, Windows 90/90,
installed consumers exit 0 with uptime present and nonnegative. WSL returned 52 s
within CLOCK_BOOTTIME 52..52 in a recorded run: the environment's boot time, not
the host's physical power-on time. No forced suspension, reboot, clock changes,
or namespace changes; no fallback without procfs. No public layout or Windows
backend changes. Virtualization still needed separate review; this unit does
not close the whole CPU section or the project.

Linux CPU frequencies fixed (LINUX_CPU_FREQUENCY.en.md): bounded cpufreq reader,
positive uint64 kHz, no permissive stod, NaN/Inf, or overflow. Covers all online
CPUs, averaging per logical processor from cpuinfo_cur_freq or scaling_cur_freq
without mixing sources or ignoring missing values. A note distinguishes hardware
readings from driver values. Base frequency requires a common value across all
CPUs and matching results from two reads. No mandatory CPU0, no averaging of
heterogeneous base frequencies, and no SMBIOS Current Speed/max used as base.
Linux 69/69, Windows 89/89, installed consumers exit 0. WSL with 32 online CPUs
does not expose the sources; absence was verified end-to-end without inventing
frequencies. Fixtures cover present values and shared policy; positive physical
Linux validation pending. Estimated, sequential, non-atomic reads with no
guarantee of persistent policy/driver identity. ACPI CPPC and cycle-based
effective frequencies not implemented in this unit. No public layout change or
CPU writes. Uptime and virtualization still need separate review; not a closure
of all CPU work.

Linux CPU topology fixed (LINUX_CPU_TOPOLOGY.en.md): sockets/cores/logical CPUs
from sysfs online and package/core groups. Removed the one-socket and
cores=logical fallbacks, cpuinfo line counting, and reliance on physical/core ID
order. Complete, consistent groups with nonnegative package IDs, non-overlapping
partitions even offline, and cores contained within packages. Old ABI aliases
used only when the modern path is absent; errors do not hide missing data. Double
reads and bounds, previous values cleared, and partial absence tracked per
metric. Linux 68/68, Windows 88/88, installed consumers exit 0. WSL: 32 logical
CPUs, 16 core groups, one package, estimated and limited to kernel reports. No
physical audit, hotplug, affinity changes, BIOS, cgroup quotas, or offline-only
sockets. No public layout change. Frequencies, uptime, and virtualization are
outside this unit and need separate review; not a closure of all CPU work.

Linux CPU caches fixed (LINUX_CPU_CACHE.en.md): covers all online CPUs, not just
cpu0, with identity by level/type/shared_cpu_list, consistent sizes, and full
coverage per level. L1 Data/Instruction added together; instances shared across
SMT threads or sockets are deduplicated. Matching double reads, bounds on
text/rows/IDs, and overflow-safe sums. Missing or inconsistent data does not
produce partial totals or zero. Estimated within online sysfs scope, not hidden
or offline hardware, affinity, or cgroups. Linux 67/67, Windows 87/87, installed
consumers exit 0. WSL: 32 CPUs, 128 rows, exposed L1=1 MiB/L2=16 MiB/L3=32 MiB;
not an independent physical audit. No public layout change or hotplug testing.
This does not close heterogeneous or unrepresentable topologies, BIOS, data
hidden by a VM, or the rest of CPU inventory.

Linux dynamic memory hardened (LINUX_MEMORY.en.md): bounded meminfo with
validated units, numbers, and duplicates; absence is not zero, relationships
checked before subtraction, and no cache overflow. MemAvailable, usage,
percentage, and cache are labeled estimates. Compressed is a proxy limited to
Zswap; Windows-style pools are absent. New public Metric
MemoryInfo.reported_total_bytes on Linux/Windows; legacy zero does not establish
a total. Hardware reservation is not computed if total is absent. 0.x API:
rebuild. Linux 66/66, Windows 86/86, installed consumers exit 0; recorded WSL
run showed 16557035520 B total and 4.35461% usage. This does not close universal
compression or composition or cgroup coverage.

Linux CPU utilization hardened (LINUX_CPU_USAGE.en.md): bounded /proc/stat
reads with validated states, schema, CPU IDs, and btime, nondecreasing counters,
and checked sum. Guest is not double-counted; iowait goes into idle and steal
into the remainder as an explicit convention. Invalid source clears the
baseline; no advance does not yield a fictitious zero. Process denominator
uses the same interval, with no percentage if global CPU is invalid. Linux
65/65, Windows 85/85, installed consumers exit 0. Native 32 CPUs, with global/own
test CPU at 3.52113% in the recorded run. This does not close cgroup quotas,
physical hotplug, or complete identity/accumulation/conversion for legacy Linux
history.

Linux active swap fixed (LINUX_SWAP_DISKS.en.md): includes paths for ext2/3/4
files and devices, /proc/swaps parsing with escapes and limits, two matching
configurations (Used may vary), O_PATH/fstat, and direct mapping to block
devices. Known positives are estimated; inaccessible entries or layers
prevent a blanket false result. Linux 64/64, Windows 84/84, installed consumers
exit 0. Native existing swap: one true and 27 false; regular file reference
tested only through metadata. No swapon/swapoff, content reads, or new swap.
Native end-to-end active swap files, other filesystems/topologies, and the
kernel swap inode token were not closed by this change.

Linux system disk fixed (LINUX_SYSTEM_DISK.en.md): O_PATH FD for / plus
st_dev, cross-check with mountinfo and direct mapping to /sys/block. Estimated
true/false only when root is identifiable and one inventory row matches.
Unknown, mismatch, or ambiguity does not become false for all disks. No new
layouts. Linux 63/63, Windows 83/83, installed consumers exit 0. WSL root sdd:
one true, 27 false. Does not identify EFI/bootloader, the host outside the
namespace, or every RAID backing device; topologies outside the policy remain
pending. Swap has a separate later provider in LINUX_SWAP_DISKS.en.md, with
its own limits and no universal physical mapping.

Linux mounted capacity fixed (LINUX_FORMATTED_CAPACITY.en.md): no fictitious
zero when association is missing; bounded mountinfo parsing and unescaping,
aliases deduplicated with retry, no partial sums on conflict or error, and
overflow-safe product and sum. Reads through an O_PATH reference validated
with st_dev and fstatvfs from the same FD. Estimated sum of visible mounted
filesystems, NOT universal total formatted capacity. Conservative type list;
others require a provider or topology support. Linux 62/62, Windows 82/82,
installed consumers exit 0. Native: one disk measured, 27 absent. No new
public API. This does not close unmounted formats, additional topology/coverage,
or system_disk/page-file semantics. Separate later providers:
LINUX_SYSTEM_DISK.en.md / LINUX_SWAP_DISKS.en.md.

Linux visible capacity and media hint fixed (LINUX_DISK_PROPERTIES.en.md):
complete decimal size parsing and checked 512 B conversion. Invalid reads
neither invent byte counts nor abort via stoull. The public DiskInfo.rotational
Metric exposes the kernel hint; media_type is rotational/non-rotational/unknown
instead of inferred HDD/SSD. E06 later adds reported_media_type from reported
identity, NOT derived from that hint (LINUX_DISK_IDENTITY.en.md). Bus is not
inferred from nvme* names. Expanded 0.x API: rebuild. Linux 61/61, Windows 81/81,
28 rows compared natively, and installed consumers exit 0. Exact physical
classification and volumes/formatted capacity/system disk/page file were not
closed by this unit.

Linux disk performance correction completed (LINUX_DISK_RATES.en.md): independent
interval, opt-out clears baseline, continuity by name/dev/optional diskseq,
resets without invalid uint64 subtraction, and rates and sums without integer
wraparound. Average response time is absent when there are no operations, not a
synthetic zero. Native reads on 28 devices and fixtures; Linux 60/60, Windows
80/80, installed consumers exit 0. Does not close physical hotplug, exact types,
layers/mounts, or per-process I/O.

Linux network rate correction completed (LINUX_NETWORK_RATES.en.md): independent
clock per interface; opt-out clears baseline; disappearance, visible reuse,
and counter resets do not generate invalid deltas; rate overflow handled
without unsafe casts. Native UDP loopback and fixtures; Linux 59/59, Windows
79/79, installation and external consumers exit 0. Does not add per-process/app
network data or validate physical hotplug.

Observed Windows CPU history per executable implemented: include_app_history
and windows_app_history separate from legacy. GetProcessTimes deltas by PID,
creation time, and path; visible path groups and estimated sums. Network and
notifications have no value. Separate WindowsHistoryStore persistence completed:
preserves values/support/notes, rejects corrupted or legacy destinations, merges
accumulatively, and requires explicit retention/reset without automatic I/O.
WINDOWS_HISTORY_STORE.en.md: Windows 78/78, Linux 58/58, installed consumers
exit 0. Gaps, reuse, overflow, bounds, resets, and native CPU for the test itself
verified: WINDOWS_APP_HISTORY.en.md. Windows 77/77, Linux 57/57, installed
consumers exit 0. Not complete OS history and no activity from before capture.

Reversible PCInsight-managed HKCU Run control: provider and transaction model
implemented (WINDOWS_RUN_CONTROL.en.md), without touching StartupApproved or
mapping it to inventory enabled status. **Positive native validation pending**:
the host returns 6801 ERROR_RM_NOT_ACTIVE; refusal without writes verified.
Removal/restoration, conflicts, and rollback verified with an injected API,
not through successful native execution. Folder/HKLM actions pending at this
checkpoint. Backup enumeration was implemented separately without closing that
native validation gap. Unit regression: Windows 73/73 and Linux 54/54, installed
consumers exit 0. Do not mistake injected tests or safe refusal for validated
native enable/disable or closure of the functional subtask.
Update: native reader for PCInsight-managed HKCU Run/backup states verified
without KTM, with two matching observations and Support::estimated, not an atomic
snapshot. Private query and real entry succeeded without writes. The action
still requires TxR; available reads do not establish availability of the action.
Changes between reads and a provider without KTM callbacks were tested; see
WINDOWS_RUN_CONTROL.en.md.
Native HKCU Run/backup listing completed (WINDOWS_RUN_LISTING.en.md): rebuilds
IDs without prior caller state, includes backup-only entries and case-insensitive
conflicts, and uses estimated, non-atomic double enumeration. Private fixture
and real read of 16 rows verified. Windows 74/74, Linux 54/54, installed consumers
exit 0. Does not close removal/restoration or change StartupApproved status.
Listing integration with Monitor/Sampler completed (WINDOWS_RUN_MONITOR.en.md):
include_windows_managed_run defaults to false, independent of generic inventory;
Metric windows_run_registrations, opt-out without retained cache, and independent
snapshots plus latest/history verified. Windows 75/75, Linux 55/55, installed
consumers exit 0; native Monitor returned 16 rows, Linux unsupported. No GUI and
no validation of the pending transactional action. Retained-report memory cost
documented.
Explicit Run control audit completed (WINDOWS_RUN_AUDIT.en.md): new SystemActions
virtual method separate from the generic one and distinct AuditedActions
operation, logging request/outcome without command, bytes, or diagnostics.
Checked forwarding and failure policies preserved. Windows 76/76, Linux 56/56,
installed consumers exit 0. Does not establish successful native writes
(6801 reconfirmed), actor authentication, consent, or tamper-proof log storage.

Windows Run Properties implemented: get_startup_properties queries the value
by ID/view and returns raw command and registry fields with double observation,
without execution or expansion. File size Metric is absent for registry entries
and available for Linux .desktop files. Startup source-file Properties also
implemented: size/attributes/FILETIME, .lnk/.url metadata from the same handle,
and file/inventory revalidation. Windows 72/72, Linux 54/54, and installed
consumers verified. See WINDOWS_STARTUP_PROPERTIES.en.md and
WINDOWS_STARTUP_FILE_PROPERTIES.en.md. This is not the target executable's
Properties and does not implement enable/disable, approval, or impact.

DXGI budgets for the calling process/node 0 implemented in separate fields
(WINDOWS_GPU_PROCESS_BUDGET.en.md), Windows 55/55 and Linux 39/39. Includes usage,
reservation, and budget headroom, NOT global free memory, which remains pending.

Windows KMT GPU temperature implemented (WINDOWS_GPU_TEMPERATURE.en.md), by exact
LUID and only with physical count 1. Native RX7900XT at 43°C; Windows 54/54 and
Linux 39/39. Other drivers, linked adapters, and 32-bit callers pending; no
hotspot or VRAM temperature.

Windows OpenCL by LUID implemented and tested (WINDOWS_GPU_OPENCL.en.md).
Windows 53/53 and Linux 39/39. The local OpenCL driver does not advertise
cl_khr_device_uuid. WINDOWS_OPENCL_INTEROP.en.md adds an estimated one-to-one
D3D11 fallback and returned OpenCL 2.0 AMD-APP (3679.0) on a real GPU. This does
not validate physical LUID identity.

Windows Vulkan by DXGI LUID verified (WINDOWS_GPU_VULKAN.en.md) without copying
the loader version to each GPU. Windows 51/51, Linux 35/35, and build without
optional headers verified. Linux correlation corrected to require unique PCI
identity in LINUX_GPU_VULKAN.en.md; Linux 37/37, Windows 51/51. Physical Linux
validation pending.

| Field | Reason |
|---|---|
| Per-process/user/app networking | Not available in procfs; requires eBPF/cgroup accounting with privileges and kernel support. |
| GPU 3D/copy/compute engines | Opt-in Monitor/Sampler pipeline (LINUX_DRM_MONITOR.en.md), exact AMDGPU/i915/Xe taxonomy (LINUX_DRM_CLASSES.en.md), busy/total cycle pairs (LINUX_DRM_CYCLES.en.md), and cycles/maxfreq alternative (LINUX_DRM_MAXFREQ.en.md), with separate sums by basis. Separate linux_drm report, not global GPU load. Linux 49/49 and Windows 62/62, installed consumers run. Other drivers/classes, non-PCI platform GPUs, hardware coverage, and physical validation remain pending. |
| NVIDIA GPU | NVML provider implemented (model/PCI/utilization/memory/temperature/driver). Physical NVIDIA validation pending: available machine is AMD. No 3D/copy/MIG engine coverage. See GPU_NVML.en.md. |
| Linux DRM/sysfs GPU and memory | G06 audited and fixed: separate LinuxGpuSysfsDetails VRAM/GTT, estimated headroom, labeled sensors, real module/version, and inventory diagnostics (LINUX_GPU_TELEMETRY.en.md). No fabricated Windows-style shared memory, budget as global free memory, or mixing tuples with NVML. G07/N02 physical validation pending. |
| GPU driver date | Linux: absent/unsupported with reason from DRM/sysfs, NVML, or merge; G08 policy tested (LINUX_GPU_DRIVER_DATE.en.md). Obsolete or placeholder DRM dates and API versions are not verified release/install dates. No positive Linux source; do not infer from BIOS/mtime/build. Windows retains a separate provider. |
| OpenGL version | Linux: opt-in desktop EGL/GLX context by PCI, with declared maximum GLX profiles separate from GL_VERSION via Metric (LINUX_CURRENT_GLX.en.md, LINUX_CURRENT_OPENGL.en.md). Does not create contexts or validate physical/headless maximum. Other vendors without identity remain unknown. G07/N02 physical and M04 Windows remain open. |
| Per-app notifications | Requires observing the bus/portal from the time PCInsight starts; the OS does not keep a universal counter. |
| History across reboots | Legacy `HistoryStore` and `WindowsHistoryStore` for the Windows report with per-field availability use separate files. The consumer must coordinate loading, merging, and saving. No automatic background logging. See WINDOWS_HISTORY_STORE.en.md. |
| Startup impact | Core/observer/candidates plus explicit overlay, deadline on failures, child exits (STARTUP_CHILD_EXIT.en.md), and persistence (STARTUP_STORE.en.md). Reference capture tool verified end-to-end (STARTUP_CAPTURE.en.md). Automatic cause/early capture and short-lived external processes remain pending. Monitor without overlay remains not_measured. |
| Friendly publisher/description | Linux startup has a declared, opt-in, estimated AppStream publisher, never the executable signer/owner. No match, ambiguity, or error leaves it unknown; not inferred from username or Exec. LINUX_STARTUP_PUBLISHER.en.md. XDG/procfs do not guarantee metadata for each app. |
| Oneshot service state | Resolved for systemd through ActiveState/SubState: active/exited is not reported as running. Other init managers pending. |
| OpenRC/runit/s6 control | No universal API is shared; specific adapters are missing at this checkpoint. |
| Windows sessions | Opt-in WTS inventory: account/domain/status plus estimated visible CPU/RAM by token session ID and exact account, even with hidden processes. See WINDOWS_USERS.en.md and WINDOWS_USER_RESOURCES.en.md. Per-process disk/network and actions pending; no invented zeros. |
| Windows startup | HKCU/HKLM Run inventory with WOW64 views and known Startup-folder files; WINDOWS_STARTUP_REGISTRY.en.md / WINDOWS_STARTUP_FOLDERS.en.md. Opt-in .lnk/.url metadata with budgets (WINDOWS_SHORTCUT.en.md / WINDOWS_INTERNET_SHORTCUT.en.md), without shell resolution or opening URLs. Estimated CompanyName separate from opt-in Authenticode query, with local/cache-only policy and no revocation (WINDOWS_STARTUP_PUBLISHER.en.md / WINDOWS_STARTUP_SIGNATURE.en.md). Windows 70/70, Linux 54/54, installed consumers; native explorer.exe LONG 0, Microsoft Windows. Registry/file does not establish enabled state: status unknown, impact not_measured. Catalogs/multiple signatures/current revocation, effective resolution/tasks/packages, approval, and actions pending. |
| Windows backend | CPU/RAM/SMBIOS, processes/services/network/disks, WTS sessions with visible CPU/RAM sums, DXGI/PDH/KMT GPU, and observed app CPU history with separate persistence (WINDOWS_APP_HISTORY.en.md / WINDOWS_HISTORY_STORE.en.md) implemented with documented limits. Stable compression, multi-group hardware, other GPU fields, startup, per-process network and disk, session control, and Windows history network/notifications remain pending. |

The pending items cannot be covered correctly by a single “universal” read.
They will use optional providers included in PCInsight, selected at runtime
without requiring command-line programs.
