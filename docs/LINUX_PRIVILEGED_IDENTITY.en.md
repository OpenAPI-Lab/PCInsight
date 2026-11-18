# L05/Linux: native message and bus identity observation

Revision 147. Internal native code, **still without an operational receiver
or helper, or native CheckAuthorization**. No installation or activation of
daemons, rules, prompts, elevation, or actions. No Monitor/SystemActions
integration or new public API, layout, or format. This unit completes
identity observation, not authorization.

## Trusted input and connection

observe_native_privileged_identity receives a borrowed sd_bus_message from
the trusted receiver, valid and serialized during the call, and mechanism
configuration. It does not take a requester name or UID from the payload.
It requires THE message currently being dispatched by that bus: a method call
with a unique sender and a destination equal to the bus connection's own
unique name. Locally constructed messages or messages retained outside dispatch
do not qualify. The body is not read or interpreted as identity.

The bus must be ready and in client mode, have an address matching the
configured unix:path, and have a nonzero method timeout of at most 5 seconds.
The function does not change the borrowed bus settings or open another
connection through environment, default, session, or autolaunch settings.
It requires a Unix stream socket. SO_PEERCRED supplies connection peer
credentials, which must match the mechanism's bus_uid. getpeername must
resolve to exactly the configured filesystem endpoint, not an abstract
socket or redirected path. The FD device/inode and peer PID/UID/GID are
recorded for final revalidation.

The Unix peer is the router, NOT the forwarded requester. The router is
queried for the requester's identity by unique sender name. The router UID
is not assumed to be zero. The mechanism configures bus_uid and authority_uid,
never the client or autodiscovery that accepts any UID. Missing or unknown
settings prevent queries. The default endpoint is /run/dbus/system_bus_socket.
Choosing a different one is a mechanism decision, not a client redirect feature.

## Queries limited to the D-Bus daemon

The function loads optional libsystemd and requires the exact symbols. It calls
only org.freedesktop.DBus at /org/freedesktop/DBus: GetId, GetConnectionUnixUser
for the sender, GetNameOwner for org.freedesktop.PolicyKit1, and the UID query
for that owner. Authority must have a unique name distinct from the caller's
and a UID matching authority_uid. Bus ID, owner, both UIDs, and socket peer
are checked again. Changes, absence, or errors discard the result. Each
reply must be a method return from the daemon, with the exact s or u signature
and a present value. Strings are limited to 255 bytes; the accepted formats
are a 32-character lowercase hexadecimal GUID and conservative
:decimal.decimal unique names. Other formats are not guessed. UID 0 is a
valid value, not a fallback.

Before and after, the function checks native boot/process/namespace context
and eUID. The current message, sender, destination, and bus state, address,
and name must remain consistent. Mechanism data and generations are copied.
An error yields a Metric without a value, not default UID 0, and does not
trigger a retry or another bus query. Native query errors are conservatively
reported as unsupported with a reason; no more specific permission category
is invented.

GetNameOwner does not start the requested service. This unit does NOT send
CheckAuthorization, register or activate Authority or a helper, or change
names, owners, or services. An absent Authority does not trigger autolaunch.

## Boundary still missing

The function borrows the supplied object; it does not install a receiver that
authenticates external messages on its own. The receiver must pass messages
from actual dispatch on the appropriate bus and validate the fixed method and
payload. Internal pointers and structs can be fabricated within the process.
They are not an authorization API or ticket.

Configuration, the endpoint, and libsystemd resolution must be protected from
clients, environment variables, LD_LIBRARY_PATH, and writable libraries BEFORE
deploying a privileged mechanism. This library is not a secure root bootstrap
and does not yet load root-owned configuration. Tests deliberately redirect
the loader only in their isolated process. A malicious trusted router or
library can fabricate metadata; there is no protection against that.

There is no atomic snapshot, ABA guarantee, lasting proof of liveness, or
global replay protection. Timeouts apply per call: up to eight 5-second
queries plus local operations, NOT an overall deadline. External callbacks
or I/O may block. The host coordinates forks, setns, credentials, bus/message/FD
lifetimes, and all configuration changes. The mechanism provides generations;
this unit does not implement reloads or a global counter.

## Evidence

tests/linux_privileged_identity_fixture.cpp implements an isolated libsystemd
ABI; tests/linux_privileged_identity.cpp loads it in a process with CTest's
LD_LIBRARY_PATH. It does not connect to the host bus. It does create a real
private Unix stream socket with a real endpoint and kernel SO_PEERCRED
credentials, and verifies cleanup and FD counts. GetId/UID/owner replies and
the incoming message are SIMULATED. They are not evidence of a real broker,
real client authentication, or polkit acceptance.

Cases include a positive contract result; a noncurrent message; invalid
sender/destination/type/address/client/ready/timeout; absent Authority;
changed ID/owner/UID; replies from another sender, wrong signatures, or
truncation; mismatched peer UID/Authority UID/endpoint; and unconfigured
arguments. MSVC covers arguments and fallback only. The previous SDK still
links. No scripts, processes, or real services are launched for this test.

Revision 147 validation: Linux 174/174 plus 40 L05 repetitions, ten MSVC
argument/fallback runs, and two relinked SDK consumers exiting 0. Initial
and final Linux builds were clean. Logs: work/privileged-identity-*.
No real broker, polkit authorization, or remote CI.

Primary sources reviewed:
[sd-bus API](https://raw.githubusercontent.com/systemd/systemd/main/src/systemd/sd-bus.h),
[message metadata](https://www.freedesktop.org/software/systemd/man/latest/sd_bus_message_get_sender.html),
and the [D-Bus specification](https://dbus.freedesktop.org/doc/dbus-specification.html).
This is PCInsight's conservative contract, not a complete replica of every
implementation.

L05 remains E. Next at this revision: a native CheckAuthorization query with
the subject bound to the message, reply correlation, and bounded parsing.
Protocol tests stay isolated, without activating a helper or prompt or
connecting an executor. A real receiver, secure bootstrap/configuration,
integration with the coordinator, and polkit acceptance remain open.
K03/OpenRC revision 132, the I07 observer, and UAC deferral are unchanged
at this checkpoint.

Update 149: [native CheckAuthorization](LINUX_PRIVILEGED_NATIVE_AUTHORIZATION.en.md)
is implemented and connected to the internal coordinator, with isolated ABI
tests, not real polkit acceptance or a receiver/helper/executor. Update 148
approved explicit OpenRC control in private development and tests. Revision
147 notes about the lack of an exception are historical and do not revoke it.
