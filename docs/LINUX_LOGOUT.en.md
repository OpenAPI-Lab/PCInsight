# Linux logout: an explicit action separate from Disconnect

The user confirmed on 2026-10-14 that Log out should be a separate action
that can terminate the session's processes. The implementation is
`SystemActions::terminate_user_session(UserSession::logout_token)`.
Unsaved work may be lost. The GUI must request explicit confirmation
and must not invoke logout as a fallback for Disconnect, Lock, or
another action's failure.

```cpp
// Only after an explicit request to log out THIS session.
if (!session.logout_token.empty()) {
    auto result = actions->terminate_user_session(session.logout_token);
    // An error after acceptance does NOT prove that nothing happened.
}
```

## Inventory and availability

Monitor prefers org.freedesktop.login1 when available, using ListSessions
over native D-Bus. J01 separates State, Active, and Remote into Metrics;
Id, Name, and User are checked against ListSessions, with TimestampMonotonic
checked before and after. See [session state](LINUX_SESSION_STATE.en.md) for
semantics and coverage, including the utmp fallback. Row IDs use login1:<id>.
The token combines the unique bus owner, Session object path, and
TimestampMonotonic, not a PID, TTY, or invented number. If an identity
property cannot be read or does not match, the row has no token and its
State, Active, and Remote remain unknown. logout_available is estimated:
a token does not establish permission for Terminate or that the session
still exists. Resource metrics remain visible sums by username, not by session.

SampleOptions::prefer_logind_sessions=false explicitly selects utmp.
Without libsystemd, a bus, or login1, the provider falls back to utmp
records; those rows have no logout token. An available login1 instance
that fails enumeration does not return a partial list as complete.
PCInsight does not add logind to distributions that lack it or install
libraries. dlopen of libsystemd.so.0 is optional at runtime, with no
external headers, SDK, or CLI. elogind can work only if it exposes this
same API and the required sd-bus transport is installed; it is not
assumed verified.

## Control and result

1. Validate the bounded token: a numeric unique owner, Session path, and
   positive uint64 timestamp. Reject TTY and PID identifiers, NUL or invalid
   trailing data, and auto/self/parent paths.
2. GetNameOwner confirms the same login1 instance. A change yields stale_target.
3. Read TimestampMonotonic using the **unique owner**, not by resolving
   the well-known name again for each request. A change yields stale_target
   without logging out.
4. Call Session.Terminate once on that path and unique owner. Interactive
   authorization is disabled; there is no sudo/pkexec or polkit prompt.
5. Query until the identity disappears. UnknownObject, NoSuchSession,
   or a different timestamp confirms that the previous identity is no
   longer published. A replacement object is not terminated. Other
   failures or a timeout after acceptance leave the result unknown or
   unconfirmed, with no Cancel or rollback.

Object removal does not establish an exit code or that a process which
escaped the session scope terminated. The session manager applies its own
permissions and semantics. This does not close other sessions for that
user through TerminateUser/TerminateSeat or signal groups inferred from
utmp. Log out does not preserve a session for reconnection. Disconnect
remains not_supported in the current Linux provider.

## Limits

The token limit is 1,340 bytes; paths 1,024, owner 255, and D-Bus values
4,096. Inventory is bounded to 10,000 rows and 16 MiB of payload.
D-Bus calls have a five-second limit each. Enumeration has a five-second
iteration budget; logout rechecks have a five-second budget with 50 ms
pauses. This is not a total deadline: a call or opening the bus can extend
beyond that budget. The library does not automatically create threads
or persistent storage.

Use a token from a fresh sample on the same host, boot, and bus. The
token is not secret or authorization, does not globally identify a
session, and does not survive a D-Bus restart. Checking properties and
calling Terminate are not atomic. A compatible implementation could
remove and reuse the path between calls. The unique owner prevents
redirection to another service instance, but does not guarantee
immutable D-Bus object references universally. Public 0.x structs,
the virtual interface, and the audit enum were expanded; rebuild all
consumers and wrappers. Windows and other backends inherit not_supported
for now. AuditedActions records terminate_user_session with the token
and result. The token can be sensitive and needs the same access and
retention policy as other targets.

## Completion evidence

On 2026-10-14, Linux Release passed 21/21 and Windows Release passed
32/32. Packages were installed and rebuilt external consumers ran.
A native read-only sample found one login1 session. It was not sent a
logout request.

Injected tests cover changed owner or timestamp, permissions, nonexistent
targets, absent providers, confirmation success/timeout/failure, parsing,
and auditing.

Later native destructive validation on 2026-10-14 used the test executable
`pcinsight-linux-logout-native-tests --native-fixture` in an explicitly
selected root context on Ubuntu 24.04 WSL. Only the test loads PAM; it
is not a new library dependency. The test used pam_start_confdir and
pam_open_session with already installed pam_systemd, private /tmp
configuration, and a process in its own transient service. It did not
write /etc/pam.d or call CreateSession directly.

Before logout, the test checks that the ID did not appear in initial
inventory, Leader is the PID of that service, and Service matches the
test's unique PAM marker. The PID is pinned with pidfd. A token with
the wrong timestamp returns stale_target and leaves the child alive.
The correct token passes through AuditedActions and the public action:
logind removes the session, and pidfd confirms the leader exited.
Preexisting session IDs remain. The transient service is unregistered
and the private PAM files are removed. No preexisting session was closed.

The test succeeded twice; the second run explicitly verified absence
of the service and files. CTest does not run it automatically. It needs
a selected privileged context and an existing UID 1000. It creates no
accounts, does not modify system PAM, scheduling, or settings, and uses
native APIs only for its fixture. All require/exception checks remain
active in Release. Linux 21/21, Windows 32/32, and installation and
consumers for both were verified.

The login1 provider was functionally verified in the tested environment.
This does not certify permissions for all consumers, all distributions
or elogind, or detach that preserves apps. The identity and timing
limits above still apply. [Disconnect safeguards](LINUX_DISCONNECT.en.md)
are documented separately.

Primary reference: [login1 API and Terminate semantics](https://github.com/systemd/systemd/blob/main/man/org.freedesktop.login1.xml).
