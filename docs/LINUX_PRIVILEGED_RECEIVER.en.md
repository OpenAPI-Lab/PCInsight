# L05 — restricted receiver, authorization observation only

Revision 150, 2026-11-02. Internal, not installed or connected to SystemActions,
Monitor, or a permanent helper. L05 remains E: this unit does not establish
secure deployment or authorize system actions.

`receive_native_privileged_request` in src/linux_privileged_identity.cpp takes a
borrowed sd_bus_message from the current dispatch and the mechanism's
configuration and allowlist. It accepts only path `/org/pcinsight/Helper`,
interface `org.pcinsight.Helper1`, member `AuthorizeSystemService`, and exact
signature `uusb`: version 1, operation 1/start–2/stop–3/restart, unit, and
boolean explicit intent. It does not accept client fields for UID, requester
name, action ID, authority, commands, arguments, environment, PID, or extra paths.

It checks the current message, type, and metadata, rewinds and decodes once,
and requires the end of the body, valid version/operation/boolean values, and
a unit of up to 255 bytes. It copies the request and applies
[revision 145 preflight](LINUX_PRIVILEGED_PREFLIGHT.en.md) before querying
identity or authorization. Identity comes from the
[revision 147 adapter](LINUX_PRIVILEGED_IDENTITY.en.md) and its authenticated
broker queries, not from those fields. The
[revision 149 adapter](LINUX_PRIVILEGED_NATIVE_AUTHORIZATION.en.md) freezes the
request and binds the sole CheckAuthorization call to the subject, action,
unit, verb, owner, and cookie. The
[revision 146 coordinator](LINUX_PRIVILEGED_AUTHORIZATION.en.md) revalidates and
applies strict rejection rules. Interaction remains OFF by default; query
autostart and the bus's interactive flag stay zero. There is no executor,
production IPC reply, object registration, or connection opening.

Intent is a client declaration, NOT proof of a click or human consent and not
a substitute for secure policy. The mechanism's gate for interaction is
separate. An `authorization_observed` result is not a ticket, capability,
guarantee of later validity, or reusable permission to execute. Do not retain
messages or adapters after the handler or dispatch the same connection
concurrently. Future execution needs its own boundary and checks.

## Test with real transport

tests/linux_privileged_receiver.cpp uses REAL libsystemd and an already available
dbus-daemon solely as development infrastructure. CMake adds the private test
only if it finds that executable in /usr/bin or /bin. It downloads and installs
nothing. The library does not run dbus-daemon or other utilities to collect data.

The test creates its own configuration and socket in a private mkdtemp directory,
without service directories, activation, or host-bus access. Three connections
to the private broker represent the client, receiver, and SIMULATED Authority.
SO_PEERCRED credentials, daemon UID/ID/owner queries, messages, senders, serials,
and libsystemd serialization are REAL. The test does not run polkitd or test its
policy, agent, prompts, host acceptance, or service control. It retains the
current UID without elevation and starts or stops no user services.

Each run tests 16 requests: four accepted observations (start, start with a
forged sender header, stop, and restart), eleven rejections without querying
Authority (version, operations, intent, unit, path, interface, member, and an
extra field attempting to add a UID), and one Authority denial. Exactly five
CheckAuthorization calls are made. Authority checks that the subject is the
client's REAL unique name even when header forgery is attempted, and that action
ID, unit, verb, and flags match. Before/after FD counts are equal. A parent
supervises the time-bounded worker and removes broker, configuration, and socket
even if the worker fails. The broker also receives a parent-death signal.
It is launched without a shell; successful tests leave no persistent processes
or files.

Revision 150 validation: Linux 178/178; eight L05 tests repeated ten times
(80 runs, including ten with the private broker); ten MSVC argument/fallback
runs; and two reinstalled and relinked SDK consumers exiting 0. Initial
clock-skew logs were preserved. The final incremental build was clean without
changing clocks or mtimes. Logs: work/privileged-receiver-*. No public API,
layout, format, or action changes, and no remote CI claim.

## Remaining L05 work

Update 151: [protected policy](LINUX_PRIVILEGED_POLICY.en.md) adds loading from
a verified anchor and generations without fallback. The test uses the managed
receiver and now runs 21 requests and eight checks, including invalidation
during a positive reply. Revision 150 evidence is historical; bootstrap is
still pending.

Remaining work includes protected bootstrap and configuration (dynamic loader
and environment, ownership, permissions, paths, and generations), connection
and concurrency isolation, message/FD/queue limits before decoding, an overall
deadline and rate limiting, real polkit policy and consent, a restricted
executor with revalidation before acting, and real acceptance under specific
authorization. The 255-byte unit limit does NOT bound memory for messages
already received by the transport. Bounded individual queries do NOT provide
an aggregate deadline or guarantee remote cancellation.
UAC/Windows remains deferred. Next at this checkpoint: protected configuration
and mechanism lifecycle without an executor, installation, activation, or
elevation. OpenRC revision 148 remains authorized only for explicit control.
SysV is not included; K03/I07 and collection restrictions are unchanged.

Primary sources consulted for the ABI and test isolation:
[sd-bus.h](https://raw.githubusercontent.com/systemd/systemd/main/src/systemd/sd-bus.h),
[callback registration](https://www.freedesktop.org/software/systemd/man/latest/sd_bus_add_object.html),
and [dbus-daemon](https://dbus.freedesktop.org/doc/dbus-daemon.1.html).
The restricted protocol and bounds are PCInsight decisions, not a public
polkit API or a replacement for pending deployment checks.
