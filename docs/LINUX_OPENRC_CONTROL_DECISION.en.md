# OpenRC: control decision

## Checkpoint 159 — 2026-11-02

The [public OpenRC API](LINUX_OPENRC_CONTROL_PUBLIC.en.md) was integrated and
tested from an installed SDK, with the original selection pinned, consent,
and an independent runner. No real OpenRC testing or newly authorized
actions. A separate SysV question was sent and remains unanswered; do NOT
extend the revision 148 permission. K03 remains E: real acceptance and SysV
are pending, and public OpenRC integration is complete. L05 deployment is deferred.

## Checkpoint 158 — 2026-11-02

Work resumes on the K03 route authorized in revision 148 without extending
L05 to another transport. An [opt-in internal executor](LINUX_OPENRC_CONTROL.en.md)
now exists with path/FD validation, a closed environment, a gated child and
pidfd, and ACK/wait/exit kept separate from live state. Tests use only a
private model runtime, not real OpenRC. Public integration and isolated
acceptance are still pending; K03 remains E and L05 deployment is deferred.
The revision 148 permission remains in force; it does not grant real actions
or SysV control. Implementation gaps below describe checkpoint 148 and
do not negate this progress.

## Current decision — 2026-11-02, revision 148

The user explicitly answered, “Yes, allow OpenRC scripts/runtime only for
explicit control.” Option 1 is approved exclusively for explicitly requested
start/stop/restart, never for data collection. Development is opt-in and
off by default, with private tests only: no dependency installation,
privilege elevation, or changes to real services. This is not permission
for SysV, a root helper, polkit/rules/daemon, automatic execution during
sampling, or an autostart launcher.

This approval removes the revision 132 OpenRC decision blocker but does NOT
implement the executor, validate safety or real acceptance, or close K03.
At this checkpoint, the design and validation described below were still
needed. Every future action requires explicit execution consent. It is not
enabled by development approval or by include_services/Monitor. The
restriction against external applications for collection remains intact.

Official record: revision 148 in SEGUIMIENTO.txt. L05/Linux remains the
current operational unit. Do not request the same authorization again when
resuming K03. The revision 132 paragraphs below are historical, predating
the reply; they do not revoke the current permission.

## Original audit and question (historical revision 132)

Revision 132, 2026-10-30. K03 audit, with no new executor code or real-service
tests. The latest implementation tests at that point remained those of
revision 131: Linux 158/158, 20 OpenRC repetitions, and SDK consumers exiting 0.

## Evidence reviewed

The [rc-service manual](https://raw.githubusercontent.com/OpenRC/openrc/master/man/rc-service.8)
describes service lookup and execution. The [rc-service.c code](https://raw.githubusercontent.com/OpenRC/openrc/master/src/rc-service/rc-service.c)
resolves the script, prepares the environment, and ends in execv. The
[openrc-run runtime](https://raw.githubusercontent.com/OpenRC/openrc/master/src/openrc-run/openrc-run.c)
and its [shell wrapper](https://raw.githubusercontent.com/OpenRC/openrc/master/sh/openrc-run.sh.in)
participate in executing scripts and their hooks and dependencies. They are
not a simple per-service message equivalent to the runit FIFO already implemented.

The [reviewed public librc header](https://raw.githubusercontent.com/OpenRC/openrc/master/src/librc/rc.h.in)
exposes query, marking, and configuration functions, but the review did not
find a public per-service start/stop/restart function equivalent to the
executor. Marking started/stopped does not run or stop the service and is
not a substitute. This describes the reviewed sources, not every version
or fork.

The [openrc-init dispatcher](https://raw.githubusercontent.com/OpenRC/openrc/master/src/openrc-init/openrc-init.c)
handles global requests such as shutdown, reboot, single-user mode, and
reexec. It is not used, and init.ctl is not opened to try to control an
individual service. PIDs are not inferred from names or markers, and
unidentified processes are not signaled.

## Choice presented to the user

The existing restriction prohibits external applications for information
collection. To avoid reinterpreting it as permission for a script executor,
an explicit decision was requested between:

1. Allow existing OpenRC scripts/runtime **only** for explicitly requested
   start/stop/restart, never for data collection. Development is opt-in and
   off by default, with private tests only: no dependency installation,
   elevation, or changes to real services.
2. Defer that control and advance other K03 adapters (s6/SysV).

Historical revision 132, before the revision 148 reply: the exception was not
to be implemented or tested until an answer arrived. Another “continue”
without an answer was not consent to run it. s6/SysV could proceed while
keeping OpenRC control open. K03 remained E; it was not closed and the
denominator was unchanged. This did not block all Linux work.

Even with approval, the design still needed script/runtime selection and
validation, environment/FDs/credentials, path and file-change policy, child
ownership, exit/errors/deadline, and separation between executor outcome and
live state. No generic shell fallback would be added to the existing ELF
launcher, and no catalog index would be treated as authority to execute.
Private-script tests would not equal real OpenRC acceptance. Hooks can
affect dependencies, so effects would not be promised to stay within
one process.

This is GUI-requested service control, not an app autostart launcher. I07
remains observer-only with UNKNOWN cause deferred by explicit decision.
The OpenRC revision 130/131 readers and their read-only contract are unchanged.
