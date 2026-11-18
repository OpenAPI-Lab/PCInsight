# K03: completion audit and handoff to L05

Update 173: [private acceptance guide](LINUX_K03_ACCEPTANCE.en.md), with 18/18
focused tests on WSL. No runtimes were found in the paths checked, and real
acceptance was not run. K03 remains E; the SysV exception is still unauthorized.

Update 159: [public OpenRC integration](LINUX_OPENRC_CONTROL_PUBLIC.en.md)
is complete: pinned files, directories, and context; per-request consent;
single-dispatch and revocable selection; and independent runner ownership.
Linux 188 tests plus 50 OpenRC repetitions, 20 MSVC runs, and 3 relinked SDK
consumers exiting 0. This used only the private model, not real OpenRC. runsv,
s6-supervise, openrc-run, and rc-service were still not found on the WSL PATH
checked (work/openrc-public-runtime-probe.txt); this does not establish global
absence. K03 remains E for isolated real acceptance and SysV control; the
SysV exception question was sent but not granted. OpenRC integration is no
longer pending. L05 deployment remains deferred. The revision 145 matrix
and earlier directions are historical.

Update 158: K03 resumes without expanding the L05 transport. The
[opt-in internal OpenRC executor](LINUX_OPENRC_CONTROL.en.md) was implemented
and verified against a private model: protected paths/files, ELF runtime
execution through a pinned FD, restricted environment and FDs, gated child
and pidfd, ACK/wait, exit/timeout, and explicitly requested TERM/KILL of the
runner only. No collection through scripts, public integration, or real
acceptance at this checkpoint. Linux 187/187, 40 OpenRC runs, 10 portable
MSVC runs, and two relinked SDK consumers exiting 0. K03 remained E: real
OpenRC testing and integration, runit/s6 acceptance, and SysV control without
an exception remained pending. L05 deployment remained E and deferred.
The revision 145 matrix below is historical, not current operating direction.

Update 148, 2026-11-02: the user explicitly approved OpenRC scripts/runtime
ONLY for requested start/stop/restart, with opt-in development and private
tests, without installation, elevation, real services, or data collection
through scripts. This removed the decision blocker, not the pending
implementation or acceptance. SysV was not authorized. See the
[current decision](LINUX_OPENRC_CONTROL_DECISION.en.md). The revision 145
matrix below is historical; its “not authorized” entry for OpenRC is no
longer current.

Revision 145, 2026-10-30. The original criterion is unchanged: native
OpenRC/runit/s6/SysV adapters, inventory and control, and tests. K03
**remains E**, deferred for the items below, not closed or removed from
the denominator.

| Provider | Implemented and verified | What is NOT complete |
| --- | --- | --- |
| runit | Inventory and 20-byte status; pinned, consent-gated FIFO u/d/tcu; one-shot observation; ServiceInfo; Monitor selection | Acceptance with real runsv; observation does not establish readiness, causation, or host PID identity |
| s6 | 35-byte record, inventory, and loggers; pinned, consent-gated FIFO u/d/cru; historical conditions; ServiceInfo; Monitor | Acceptance with real s6-supervise; files and FIFOs do not establish liveness, peer version, or completion |
| OpenRC | Catalog, references, and direct stacks; separate markers; two ServiceInfo views and Monitor selection without joins by name | Start/stop/restart through scripts/runtime not authorized at this revision; PID and liveness cannot be verified from markers; real acceptance |
| SysV | Catalog and references, bounded LSB metadata tied to a candidate, ServiceInfo, and Monitor selection | Start/stop/restart through scripts not authorized; PID and liveness not derived from files; real acceptance |

Implementing readers did not complete OpenRC/SysV control. Writing markers,
links, or pidfiles or sending signals to guessed processes is not a substitute.
Private FIFO/status fixtures establish protocol behavior, guards, and rejection
cases, not real supervisors, active services, or coverage on another distribution.
Successful delivery of bytes does not mean completed work or readiness.

## Evidence reviewed

Code, APIs, tests, and revision 128–144 documents: runit
([provider](LINUX_RUNIT.en.md), [transitions](LINUX_RUNIT_TRANSITIONS.en.md),
[services](LINUX_RUNIT_SERVICES.en.md)); s6
([record](LINUX_S6.en.md), [control](LINUX_S6_CONTROL.en.md),
[inventory](LINUX_S6_INVENTORY.en.md), [observation](LINUX_S6_OBSERVATION.en.md),
[services](LINUX_S6_SERVICES.en.md)); OpenRC
([markers](LINUX_OPENRC.en.md), [catalog](LINUX_OPENRC_CATALOG.en.md),
[services](LINUX_OPENRC_SERVICES.en.md)); SysV
([catalog](LINUX_SYSV.en.md), [LSB](LINUX_SYSV_LSB.en.md),
[described catalog](LINUX_SYSV_DESCRIBED_CATALOG.en.md),
[services](LINUX_SYSV_SERVICES.en.md)); and
[service selection](LINUX_SERVICE_SELECTION.en.md).
The last K03 regression before this audit was revision 144: Linux 170/170
plus ten selector repetitions, ten pure MSVC runs, and two rebuilt SDK
consumers exiting 0. It is not relabeled as a revision 145 run.

Revision 145 read-only check in Ubuntu WSL: command -v with literal names
runsv, s6-supervise, openrc-run, and rc-service returned exit 1 without paths.
This establishes absence on that PATH at that time, NOT global absence across
packages or disk, or support for other distros. Nothing was installed or run.
The first loop attempt had variables expanded by PowerShell and is not valid
evidence. It was corrected to four literal command -v checks; the error
was retained.

## What allows each pending item to resume

Real acceptance needs an appropriate private environment with the supervisor
already available and authorization for isolated tests, without using personal
services. “Continue” does not implicitly authorize installing an init system
or starting real services.

OpenRC/SysV control: an explicit decision is needed on an exception limited
to control actions through existing runtime/scripts, never data collection.
Design, implementation, and validation follow that decision. At revision 145,
the revision 132 exception was still ungranted. A polkit helper does not
remove this restriction or turn a script into a protocol.

No additional inventory or projection layer warrants adding duplicate adapters
before those items. Scope is not changed to close K03 artificially. More
fixtures cannot stand in for real acceptance. This work is deferred and
**L05/Linux** proceeds according to the plan at this checkpoint.

## Limited handoff

First L05 unit: pure internal validation of narrowly defined system systemd
control requests against an allowlist set by the future mechanism, not the
client. No daemon, socket, setuid, polkit rules, elevation, installation,
activation, UAC, or real operation. Consent is not authentication or
authorization. IPC identity, polkit authorization, and rejection tests
would follow and were still pending.

K03 remains E; no new subtask, ID, or partial credit. Starting L05 does not
close the criterion shared with UAC. This handoff does not increase the
completion percentage.
