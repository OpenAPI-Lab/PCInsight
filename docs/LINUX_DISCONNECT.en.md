# Disconnect on Linux: provider capabilities and justified limits

Revision 50, 2026-10-19. J05 closes under the alternative already in its
official criterion: **documented unavailability** for the included Linux
utmp/login1 adapters. This does not implement an actual disconnect or claim
that no Linux remote server can disconnect a session. Windows WTS remains
under M06. Rejecting the action alone did not close J05. This work defines
the provider-specific capability contract and rationale, propagates it to
inventory, and tests that no destructive substitute is used. It does not
change the criterion or count the J03 remediation again.

## Capability query without sessions or side effects

```cpp
#include <pcinsight/linux_session_control.hpp>
auto capability = pcinsight::query_linux_disconnect_capability(
    pcinsight::LinuxDisconnectProvider::login1);
if (pcinsight::metric_is_known(capability) && !*capability.value) {
    // Disable Disconnect and display capability.note.
    // Do NOT call terminate_user_session as a fallback.
}
```

The query is static. It describes built-in adapters without inspecting
a session, bus, remote server, or permissions. It works with no sessions
or with collection disabled. On Linux it returns false/available and
the reason for a recognized provider (utmp or login1). An unknown enum
or non-Linux build returns an absent/unsupported Metric rather than
inventing a measured false.

UserSession.disconnect_available uses the same reason only when provider
details are known and recognized. Missing, unsupported, denied, or
future provider metadata stays unknown. Providers are not classified
by ID prefix. Active, Remote, State, and logout_token do not establish
detach capability either. A logout token is exclusively for the separate
destructive action and is not used by Disconnect.

## Rationale and coverage

- utmp records logins. It is neither a control protocol nor a remote
  transport identifier. Its documentation warns that the inventory may
  be incomplete. [utmp(5)](https://man7.org/linux/man-pages/man5/utmp.5.html).
- The reviewed login1 interface documents inventory, activation, locking,
  termination, and device control, but not remote transport detachment.
  ReleaseControl releases device control; it does not disconnect while
  preserving a session. ReleaseSession is part of the PAM lifecycle
  and should not be called by clients. Neither is substituted for
  Disconnect. [login1 contract](https://www.freedesktop.org/software/systemd/man/latest/org.freedesktop.login1.html).

These contracts justify unavailable capability **for these adapters**,
not impossibility for every future extension or distribution. The Remote
property does not identify or authorize a particular RDP, VNC, or SSH
controller. Cutting a connection while preserving processes requires
the relevant transport server's semantics and identity. Those cannot
be obtained from a PID, TTY, or logind row. No dependencies, daemon,
CLI, or experimental remote protocol are added.

The GUI should show this limitation and not present the option as
operational with these Linux providers. A future transport adapter
needs its own contract, identity and authorization, and positive
validation. It cannot reuse this false as detection of every OS capability.

The additive 0.x API introduces a header, enum, and free function
without changing layouts, vtables, or history format. Rebuild and relink
consumers to use the new symbol. An aggregated row without provider
metadata now reports unknown rather than false/available. Recognized
native utmp/login1 rows retain false and gain a provider-specific note.

## Revision 50 validation

Windows Release passed 126/126 and Linux Release passed 110/110.
disconnect-safety, users, and session-state were each repeated 20 times
per platform (60 runs on Windows and 60 on Linux). Windows verifies stubs
and portable policy, not a Linux session. Both installed consumers were
rebuilt and relinked and exited 0 with the new function and header.
Builds had no warnings. Evidence is under work/disconnect-capability-*.
The final rebuild, suites, installation, and consumers are in
verified-{build,suite,install,consumer}-{windows,linux}.log; repetitions
are under repeat-*, and native reads under native-linux. Earlier final-*
logs were retained.

The native Linux test creates its own child in a private POSIX session
and uses a private utmp file. After audited rejection of TTY and login1
IDs and a logout token, the child replies, receives no SIGHUP, and
remains alive. Channel reads use poll with a two-second wait, and CTest
limits the full test to 20 seconds. Cleanup targets only that child
and the exact file and directory; real utmp is unchanged. Read-only
login1 inventory on this host retains one session, one known State, and
one known Active value. Nothing is disconnected or terminated.
Fixtures cover both providers and all Active/Remote/State combinations,
unknown enums, absent/future/denied providers, and invalid NUL.

The first suites failed on the new NUL case because the aggregator
already rejects such metadata with invalid_argument, whereas the test
expected an absent Metric. The expectation was corrected to require
the exception; the validator was neither changed nor weakened. Initial
logs were retained as work/disconnect-capability-suite-{windows,linux}.log.
That failure is not attributed to the native action or historical
N03 CPU/app-history failures.

## Background: safety remediation

Completed work: removal of legacy `kill(-getsid(ut_pid), SIGHUP)`
control. This was not implementation of a new real Disconnect API.

## Problem corrected

The old code looked up ut_line in utmp, obtained a PID, and used its
session ID as a negative process-group ID. A session ID, process-group
ID, and TTY/ut_line are not interchangeable identities. Membership can
change, utmp can go stale, and the PID can be reused. SIGHUP can also
terminate applications; sending it does not establish a disconnect
that preserves processes.

Linux `SystemActions::disconnect_user` now validates input and fails
closed. Empty input, NUL/control bytes, or over 4,096 bytes yields
invalid_argument; other identifiers yield not_supported. It does not
read utmp, call getsid, kill, or a session manager, or change privileges.
A “logind:” prefix is not implicit authorization to log out.

`UserSession::disconnect_available` reports false with Support::available
for recognized Linux providers. This is availability **of the implemented
backend**, not a claim that Linux cannot support a remote disconnect.
The GUI can hide or disable the button and display the note. Providers
that do not calculate it leave it unsupported; absence is not measured
false. AuditedActions still records the request and rejection without
inventing an action. The field was added earlier in 0.x; revision 50
does not change its layout again.

## Keep the operations separate

Disconnecting a remote session can preserve apps for reconnection.
Linux has no universal graphical or TTY detach across providers.
An implementation needs that provider's native protocol and appropriate
identity and permissions, not CLI calls or guessed process groups.

logind Session.Terminate/TerminateSession **terminates** the session
and its processes. It must not silently replace Disconnect. The decision
at this earlier checkpoint was whether to add an explicitly destructive
Log out action and then implement and verify its provider. A later
decision confirmed the separate action, implemented and verified with
a real private PAM session in [Linux logout](LINUX_LOGOUT.en.md). This
does not establish universal logind/elogind compatibility or remote
disconnect support.

## Verification

On 2026-10-14, the original safety fix passed Linux 20/20 and Windows
31/31. Final suites with the separate action passed 21/21 and 32/32.
Installation and external consumers were verified on both systems.

Portable logic covers bounds, control bytes, Unicode, TTY and numeric
IDs, and valid-looking prefixes without triggering actions or reporting
false success. The Linux test creates its own child in an isolated POSIX
session, installs a SIGHUP detector, and registers a **private** utmp
row pointing to that child. After disconnect_user, the child confirms
that it did not receive SIGHUP and remains alive. Monitor exposes false
availability and auditing reports not_supported. Only that child is
reaped and the temporary file is removed, restoring the test process's
utmp selector. System utmp is not changed, and the user's session is
not disconnected or terminated. Windows tests only portable policy.

References: [POSIX sessions](https://man7.org/linux/man-pages/man2/setsid.2.html),
[signals to process groups](https://man7.org/linux/man-pages/man2/kill.2.html),
and [logind Terminate is not detach](https://github.com/systemd/systemd/blob/main/man/org.freedesktop.login1.xml).
