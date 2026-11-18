# L05 — verified transport boundary and deployment decision

Revision 157, 2026-11-02. The contract test is complete. This does NOT
implement a new production transport, install a helper, or protect the
host bus. L05 remains E. Production gains no dependencies and does not
enable execution.

## Finding

The [revision 156 limits](LINUX_PRIVILEGED_ADMISSION.en.md) act in the handler,
after sd-bus has received the message. A dbus-daemon profile can limit message
size and broker resources, but those limits are not configured per destination
or helper. A small maximum on a shared bus can break legitimate large messages
from other apps. Do not install this profile under /etc/dbus-1 or change or
restart the system bus to satisfy a local PCInsight requirement.

This unit does not demonstrate a byte limit BEFORE receipt in the production
helper. It also does not obtain or attest to the user's effective broker limits
through credentials or identity: SO_PEERCRED and UID do not prove configuration.
A local queue threshold is not a hard memory guarantee.

## Reproducible private test

tests/fixtures/privileged_transport_limits.xml is a TEST-ONLY FRAGMENT, not
a policy to install on the host. The runner inserts it into its temporary
broker configuration, without service directories or activation. It uses an
already available dbus-daemon executable solely as development infrastructure.

Profile: 32,768 bytes per message, 65,536 bytes for each incoming and outgoing
queue, one FD per message and four in each queue, 16 complete connections and
16 per user, four incomplete connections, 128 matches, 32 pending replies, and
auth_timeout of 2,000 ms. Queues can exceed their threshold by up to one allowed
message. This is not an exact RAM/RSS ceiling because there is overhead and
there are other resources. Not all these limits were stress-tested here.

The first profile, with zero FD quotas, blocked authentication in the tested
runtime. The failure is retained in work/privileged-transport-focal.log, and
the fixture was corrected to use positive quotas. This result is not
generalized to every broker. PCInsight's owned connection still uses
negotiate_fds=0. The profile is not claimed to prohibit all FDs at the broker.

`--transport` mode in tests/linux_privileged_receiver.cpp:

1. A small legitimate request is accepted with a SIMULATED Authority.
2. A valid D-Bus message contains a 65,536-byte string field. Appending it
   locally succeeds, but the call fails and the handler entry count does NOT
   increase. Authority checks do not increase, and the Manager is not called.
3. Another client can still make a legitimate request. The receiver and
   broker remain operational after rejecting the offending client.

The private broker and libsystemd are REAL; Authority and Manager are
SIMULATED. Two calls reach the handler, there are two Authority checks, and
there are zero Manager operations. This is three client attempts, not three
admitted requests. No real services are run, and there is no connection to
the host bus or polkit. The parent removes the broker and files; the worker
checks for FD leaks. The test measures delivery to the handler, not RSS or
latency or rejection for every byte, FD, or queue variation.

Linux 186/186 plus 160 L05 runs (16 tests repeated ten times, including 30
broker runs across regression, admission, and transport); ten MSVC argument
and fallback runs; and two existing SDK consumers exiting 0. Production
library code was unchanged in this unit. Initial, full, and final builds
were clean. See work/privileged-transport-* for the passing focal-final run
and full suite. No new public API, layouts, formats, Monitor/SystemActions
changes, or remote CI.

## Decision pending at this checkpoint

Do not add a private broker as a runtime dependency. It conflicts with the
intent to avoid additional applications and expands installation, supervision,
and security requirements. Do not install the fragment globally to claim
per-helper protection.

The user was asked to choose between:

- Defer L05 deployment and pending acceptance and continue K03/OpenRC. This
  was recommended to stay within the existing Linux scope; L05 remains E
  with its criteria open.
- Design a private Unix channel with its own limits before decoding. This
  expands transport, protocol, and the link between identity and authorization.
  It is not a simple sd-bus toggle or authorization to install, elevate, or
  perform real actions.

Silence is not a choice and does not trigger an architecture change. At this
checkpoint, revision 157 closes as evidence and a contract test, while the next
L05 step awaits direction. A trusted process and loader, real polkit policy
and consent, deployment, and acceptance also remain pending. Private tests do
not close those items or Windows/UAC.

OpenRC revision 148 remains authorized only for explicit control in private
development and tests. It does not extend to collection, SysV, or real services.
K03 and I07 remain unchanged. C60/E10/P3/B2 plus three excluded X:
60/75 = 80% closed, 15/75 = 20% open, with no partial credit.

Primary source: [dbus-daemon limits](https://dbus.freedesktop.org/doc/dbus-daemon.1.html).
The values and scenario are PCInsight test choices, not recommended settings
for a shared desktop bus.
