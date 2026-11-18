# L05/Linux: internal preflight for a fixed set of operations

Revision 145. This starts L05; **there is no operational privileged helper**.
The [K03 audit](LINUX_K03_AUDIT.en.md) came first: K03 remains open or deferred
because of unauthorized control paths and real-world acceptance, not because
it needs another inventory projection.

The internal linux_privileged_preflight.hpp/.cpp code is not an installed
header or a new public API. It is a pure function: no callbacks, IPC, file
reads, daemon, socket, polkit, rules, setuid, elevation, installation, service,
or execution. It does not connect to Monitor/SystemActions or change formats
or layouts. It does not start work on UAC/Windows.

## First supported surface

Only planning for start/stop/restart on the **system** systemd manager, using a
canonical .service name literally allowed by the future mechanism. The request
does not accept commands, argv, PATH, environment, cwd, files, client-supplied
PID/UID, startup/reset/logout/efficiency actions, or OpenRC/SysV scripts. It
does not use a shell. This does NOT remove those requirements from the project;
they are outside this first surface and are not added through a fallback.

PrivilegedServiceRequest contains a unit, a ServiceAction, and declared explicit
intent. False yields no plan; true is NOT identity, authentication, or
authorization. The name is checked by the shared service validator and the
action by a closed enum, with no mask calculated from an arbitrary enum value.
Inventory IDs are not treated as authority to execute.

Rules come from the **mechanism's** configuration, not a client-supplied list.
Up to 256 exact names are allowed. The bits are start=1, stop=2, restart=4;
zero denies, and unknown bits invalidate configuration. An invalid or duplicate
name, even for a different unit, rejects the entire preflight. Bad rules are
not ignored. An empty list, missing unit, or ungranted operation yields no
plan. There are no wildcards, regexes, prefixes, or authorization of another
template instance just because it shares a base name. There is no configuration
loader yet; tests supply private rules.

Success returns an owned copy of the unit and operation and a fixed policy ID
per verb, provisional for the future mechanism:
org.pcinsight.helper.systemd.start, org.pcinsight.helper.systemd.stop, or
org.pcinsight.helper.systemd.restart. No action is registered with polkit.
An available Metric means **syntax and scope preflight**, not OS permission,
a grant, or proof of user identity. Internal structs can be constructed in C++;
they are not a security boundary or a token an executor should accept as
authorization. There is no executor at this checkpoint. The host serializes
access to strings and rules; concurrent copying is not guaranteed atomic.

## Next security boundary (not implemented in revision 145)

Obtain requester identity from authenticated transport, never from a UID, PID,
or bus name claimed in the payload. Bind the subject to that connection and
a valid owner. Freeze the full request before authorization, check for changes
and disconnection, require replies from the correct Authority, and reject any
ambiguity or error. Do not reuse results across requests or turn a plan into
a ticket.

Polkit separates subject, action_id, and result. is_authorized is authorization;
is_challenge says authorization might be obtained with more information, not
that it succeeded. User interaction is a separate option, not implicit
elevation. Primary reference:
[Authority API](https://polkit.pages.freedesktop.org/polkit/eggdbus-interface-org.freedesktop.PolicyKit1.Authority.html).

Pending design: IPC backend, peer and Authority checks, per-request queries
without a local cache, interaction only on explicit events, cancellation and
timeouts, rejection and retention, local policy, and separate final native
results. The mechanism, subject, and agent architecture is documented in
[PolkitAuthority](https://polkit.pages.freedesktop.org/polkit/PolkitAuthority.html).
A client boolean or a call to pkexec is not enough.

Also required before deployment: root-owned configuration not writable by the
client, explicit installation and activation, payload and concurrency limits,
manager and target revalidation, opt-in auditing, handling dependency effects,
rejection tests, and real acceptance. User and system polkit rules can affect
scope and retention; this does not promise to override root policy. This unit
does not add or install a generic root helper.

## Evidence and status

tests/linux_privileged_preflight.cpp covers all three verbs and fixed IDs,
default deny, missing intent or target, denied operation, zero mask, unknown
bits, invalid enums, invalid names/paths/NUL/control characters, duplicates,
invalid unrelated configuration, the 256/257 boundary, exact instances, and
an independent copy unaffected by later changes. These are pure tests, not
authenticated polkit/daemon tests, real actions, or privilege tests.

Revision 145 validation: Linux 171/171 plus ten preflight repetitions, ten
pure MSVC runs, and two relinked SDK consumers exiting 0. Initial and final
Linux builds were clean. Logs: work/privileged-preflight-*. No polkit
authentication, remote CI, or root-helper integration.

L05 moves P → E, not C. IPC identity, authorization, restricted execution,
and acceptance remain pending. The shared UAC work is deferred under the
Linux-first priority. Next: an identity and authorization boundary that fails
closed, with private fixtures, before connecting an executor or installing
policy. K03/OpenRC revision 132/I07 and other criteria remain unchanged.

Revision 146 adds [internal authorization coordination](LINUX_PRIVILEGED_AUTHORIZATION.en.md):
snapshotting, binding, revalidation, and denials with fixtures, still without
a native adapter, real peer authentication, installed policy, or privileged
executor.
