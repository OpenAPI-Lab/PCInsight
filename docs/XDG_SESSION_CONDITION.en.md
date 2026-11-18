# Startup conditions based on the GNOME session name

Verified on 2026-10-14: `AutostartCondition=GNOME3 if-session <name>` and
`GNOME3 unless-session <name>` use the name reported by the manager
on the caller's bus. They return through autostart_condition_satisfied
without changing status or xdg_standard_eligible. Name comparison is
case-sensitive; full type and predicate tokens are ASCII case-insensitive.
An invalid predicate, missing operand, empty name, control character,
NUL, or name over 1,024 bytes yields unknown. Nothing is executed.

The native source is the
[org.gnome.SessionManager SessionName property](https://gnome.pages.gitlab.gnome.org/gnome-session/re04.html),
read using already installed libsystemd/sd-bus without a CLI or new
build dependency. GetNameOwner resolves the current owner, and
Properties.Get targets that unique name, not an activatable service.
The provider does not start GNOME or substitute XDG_CURRENT_DESKTOP
or DESKTOP_SESSION for the actual name.

The transport is local. If DBUS_SESSION_BUS_ADDRESS is set, it must be
a single unix: address of at most 32 KiB. Autolaunch, unixexec, TCP, and
address lists are rejected before opening. If the variable is absent,
sd_bus_open_user uses its default local socket, not a PCInsight subprocess.
The environment, loader, and socket are trusted host context, not a
security barrier for a privileged helper. The provider does not look
for another user's graphical session or guarantee that the selected
bus belongs to the intended session.

The provider validates the unique owner, string variant, and a nonempty,
bounded, control-free name. Missing runtime, bus, owner, or property,
malformed replies, and other failures yield unknown; access denial
yields permission_required. unless-session does not turn unknown into
true. A known value is available: an observed name, not a guarantee of
launch or persistent session identity.

The query is lazy and runs once per inventory context, including failures.
All entries in the context share it under a mutex. A new inventory creates
a new context and queries again. There is no watcher or global cache.
Each of the two D-Bus calls has a two-second timeout. There is no
guaranteed overall deadline for opening, handshakes, or other providers'
filesystem calls. The owner or name can change after the query; the
caller must refresh inventory. The provider invokes no logout,
registration, or launch methods.

Linux Release passed 26/26 and Windows Release passed 37/37, with
installed packages and external consumers run. Portable parser tests
cover case-sensitive names, differences from desktop names, operands
and limits, absent providers, and permission_required. Linux uses
the private service `org.pcinsight.SessionFixture.p<PID>` on the existing
local bus. The test acquires its own name without replacement and does
not claim the real GNOME name. The same native provider with an internal
test target reads variants, observes changes, and rejects wrong variants,
access denials, oversized or control-containing strings, nonexistent
owners, and unixexec before opening. Closing the connection removes only
the test service. No daemon or CLI was launched. The fixture ran on
Ubuntu 24.04 WSL without skipping. Hosts without a bus can explicitly
skip the native part; Windows does not emulate a GNOME manager.

The provider has not been validated with a real graphical GNOME session
manager or all its versions. The fixture verifies the native transport
and property contract, not GNOME as a whole. GConf, other types and
vendor policies, and startup impact remain pending at this checkpoint.
