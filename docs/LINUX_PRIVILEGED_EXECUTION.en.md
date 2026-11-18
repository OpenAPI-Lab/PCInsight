# L05 — restricted execution, opt-in and without retries

Revision 153, 2026-11-02. Internal flow implemented and tested with a simulated
manager. OFF by default, not installed or registered, and not connected to
SystemActions or Monitor. L05 remains E: there is no real polkit, root, or
helper acceptance, and no hardened deployment.

## Two separate methods

The restricted decoder is shared without expanding the payload: path
/org/pcinsight/Helper, interface org.pcinsight.Helper1, signature uusb, version 1,
operation 1–3, unit, and intent. There are no identity, UID, actionID, or command
fields. AuthorizeSystemService still only observes authorization: it NEVER
calls the manager, even when execution is enabled. The new
execute_owned_privileged_request accepts only ControlSystemService from the
current dispatch. It neither accepts an earlier report as a grant nor reuses
saved authorization.

PrivilegedExecutionGate is INTERNAL mechanism configuration, not payload:
enabled=false, permit_interaction=false, and no configured Manager UID by
default. An atomic flag rejects overlapping or reentrant execution. It does
not replace bus serialization or make it safe to use or close the same object
from multiple threads at once. The guard is released on errors too. No shell,
argv, environment, or arbitrary client method is accepted; only StartUnit,
StopUnit, and RestartUnit are supported.

## Per-request flow

1. Check the switch, UID, and reentrancy, then decode and run preflight with a
   copy of the current policy and rules.
2. Require the owned connection and exact native identity context, including
   the policy generation.
3. Query the broker for the Manager's owner with GetNameOwner and for its UID.
   Require the configured UID and a unique name distinct from the requester
   and Authority. No activation or alternate lookup.
4. Obtain NEW authorization with the transport subject and frozen request.
   The coordinator applies strict rejection rules; callbacks observe context,
   gate, policy, and connection.
5. Reobserve the Manager and caller and check current validity before sending.
6. Build the message for the verified UNIQUE OWNER, with a fixed method for
   the operation, the literal unit, and fixed replace mode. Autostart and the
   message's interactive flag are both zero.
7. Check switch, configuration, policy, and connection one last time. Make
   ONE sd_bus_call with a timeout of up to five seconds.
8. Check reply type, sender, exact o signature, matching cookie, complete
   body, and a numeric job path under /org/freedesktop/systemd1/job/ up to
   1,024 bytes. Revalidate context.

A correlated job receipt means job_queued, NOT that the service started or
stopped, that the action succeeded, or that the job completed. An authorization
observation is not a reusable ticket, and the intent bit is not proof of
human consent.

## Outcome reporting and concurrency limits

Before dispatch_attempted, rejection or failure means this flow did not try
to send the request to the Manager. From immediately before sd_bus_call onward,
any error, invalid reply, or context change is reported as dispatch_unknown:
the operation MAY have run. The flow does not guarantee absence of effects,
retry, or automatically roll back or cancel. The GUI must show uncertainty
as such, not treat it as a definite failure that permits a hidden retry.

The final check is NOT a distributed atomic transaction with polkit, policy
changes, client disconnection, or systemd. A change after that boundary does
not cancel an operation in flight. Post-checks can detect changes but cannot
undo them. Immediate atomic revocation/invalidation, a hard aggregate
deadline, rate and queue limits, job completion tracking, and process
hardening remain pending before a deployed helper can be called secure.
Unit dependencies and Conflicts are not isolated: the allowlist restricts
the literal target, not every effect of an administrator-configured systemd job.

The private implementation in src/linux_privileged_identity.cpp reuses the
validated ABI and queries. The contract is in src/linux_privileged_execution.hpp.
No SDK headers are exported, layouts or formats changed, or existing paths
enabled.

## Revision 153 evidence

Update 154: [asynchronous job tracking](LINUX_PRIVILEGED_JOBS.en.md) adds a match
rule and Subscribe before dispatch, a canonical uint32 receipt, results that
can be queried without touching the bus from another thread, and bounded
deadlines and cleanup. job_queued is still a receipt, not completion; the job
object supplies the observed result. Revision 153 evidence is historical.

tests/linux_privileged_execution.cpp checks mechanism and decoder defaults
and portable fallback. tests/linux_privileged_receiver.cpp extends the REAL
private broker and libsystemd setup with a fourth connection and a thread
running a SIMULATED Manager alongside the SIMULATED Authority. No real
service is started, stopped, or restarted. The Manager only verifies
method, unit, and mode, increments counters, and returns replies.

Each run uses 41 receiver requests, 20 Authority checks, and eight calls to
the simulated Manager. New cases: default off, unconfigured UID, busy state,
invalid operation, unlisted unit, missing intent, extra UID field, wrong
Manager UID, denial without dispatch, queued start/stop/restart, an
authorization-only endpoint that does not dispatch even when enabled,
invalidation during authorization without action, invalid job path,
incorrect signature, policy invalidated by the Manager after simulated
acceptance, D-Bus error after simulated acceptance, and final recovery.
Every unknown-reply case records exactly one Manager increment and
dispatch_attempted, with no receipt or retry. Revision 152 anchor revocation
and private FD and cleanup checks remain.

Linux 181/181 and 110 L05 runs (11 tests repeated ten times, including ten
broker runs); 20 MSVC defaults/arguments/fallback runs; and two reinstalled
and relinked SDK consumers exiting 0. Initial full-build clock-skew warnings
were retained. The final incremental build and 181-test suite were clean
without changing clocks or mtimes. Logs: work/privileged-execution-*.
No polkitd, host bus, prompts, elevation, real services, installation,
activation, real-world acceptance, or remote CI.

Next at this checkpoint: receiver limits and job tracking, keeping dispatch
separate from completion. Real deployment, loader, and polkit work remain
pending. Do not install, activate, elevate, or act on real services without
specific authorization. OpenRC revision 148 stays limited to explicit
control; SysV/K03/I07/UAC are unchanged. Official count: C60/E10/P3/B2 plus
three excluded X; 60/75 = 80% closed and 15/75 = 20% open.

Primary sources:
[systemd Manager](https://www.freedesktop.org/software/systemd/man/latest/org.freedesktop.systemd1.html)
and [sd_bus_call](https://www.freedesktop.org/software/systemd/man/latest/sd_bus_call.html).
The gate, helper protocol, and conservative uncertainty policy are PCInsight
contracts, not replicas of all systemd or polkit policies.
