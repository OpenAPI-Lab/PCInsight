# L05/Linux: internal authorization coordinator

Revision 146. The second component, after [revision 145 preflight](LINUX_PRIVILEGED_PREFLIGHT.en.md).
**Internal coordination and contract fixtures only:** no native adapter, real
authentication, helper, daemon, public socket, polkit rule, elevation, or
privileged execution. Nothing is installed or activated. No new public API,
layout, or format; Monitor/SystemActions are unchanged.

## Implemented flow

observe_privileged_authorization runs preflight on the request and mechanism
rules. Scope, syntax, or intent rejection invokes zero callbacks. Missing
callbacks or Authority UID configuration prevent an authorization query.
There is no global or default backend; an empty API always rejects.

The trusted adapter supplies an exact available context: bus ID, unique
requester and Authority names, observed UIDs, connection generation, and
mechanism policy generation. Only the conservative unique-name format
:decimal.decimal and a 32-character lowercase hexadecimal GUID are accepted.
Other formats are rejected, not guessed. Well-known names such as
org.freedesktop.PolicyKit1 do not replace the unique owner. An unknown UID,
generation zero, or missing configuration causes rejection.

The Authority UID must match the **mechanism's expected_authority_uid**, not
an arbitrary client UID or hardcoded zero. The original root-only check was
fixed during review before delivery. polkitd can run as an unprivileged system
user; see the [official polkitd manual](https://polkit.pages.freedesktop.org/polkit/polkitd.8.html).
Tests cover explicitly configured non-root UIDs and UID 0, mismatches, and
missing values. Resolving and protecting this configuration in the real
adapter remains pending.

The request, plan, and context are copied before checking. Exactly one check
uses the fixed subject, unit, and per-verb action_id. Interaction is allowed
only after preflight confirms declared explicit intent, not for data samples.
The adapter MUST obtain the subject from authenticated transport and bind the
reply to THAT call. It must not accept a UID, PID, bus name, or cookie from
the payload. The coordinator cannot authenticate data fabricated by a
malicious or incorrect adapter.

The context is then observed again. Changes to bus, peer, UID, owner,
connection generation, or policy generation, disconnection, or a non-exact
source cause the result to be discarded. The reply sender must equal the
pinned owner, and the nonzero call serial must match the reply-to serial.
This is an adapter contract, not a D-Bus parser or cryptographic replay
protection. The transport must supply real serials and causal correlation.

Only is_authorized=true with is_challenge=false and no dismissal, retention,
or temporary ID yields authorization_observed. Not authorized, challenge,
dismissal, or retention yields an observed false. Missing data, invalid
binding, errors, or exceptions yield UNKNOWN/rejection, not fabricated
false. All paths preserve check_attempted. No retries or cross-request cache.

`authorization` means the coordinator's conservative acceptance, NOT a copy
of polkit's is_authorized boolean. For example, true with retention is
rejected here and yields false. Notes explain that distinction.

Polkit distinguishes authorization from a challenge and defines the
system-bus-name subject and AllowUserInteraction for user events. Primary
reference: [Authority API](https://polkit.pages.freedesktop.org/polkit/eggdbus-interface-org.freedesktop.PolicyKit1.Authority.html).
Rejecting retained or temporary results is THIS coordinator's conservative
policy, not a universal polkit requirement or guarantee of a fresh prompt.
Administrative policy can authorize implicitly without interaction.

## Security limits and no deployment

All structs and callbacks are internal and can be constructed by C++ tests;
they are not a privilege boundary. A true report is adapter evidence, **not a
ticket**. No executor consumes it and it must not be reused. It also does
not protect against disconnection or changes after the last observation, or
against ABA. This unit does not implement real message authentication, policy
installation, loading root-owned configuration, IPC deadlines or cancellation,
or action results.

The future adapter must trust only the appropriate system connection and bus,
credentials from the transport, the actual Authority name owner, and protected
configuration. It must handle truncated and malformed messages, bounds,
serials, timeouts, cancellation, and owner changes. Do not fill gaps with
client booleans or pkexec calls. Test callbacks do not prove these requirements.
The host serializes access to request, rules, and context. The snapshot is not
atomic against concurrent writes. Callbacks are synchronous, with no hard
deadline if they block.

Before connecting any executor, revalidate selection, policy, and context
and consume authorization bound to a single operation within the same flow.
That design and integration remain pending; no object in this unit authorizes
real services. No operations outside the closed list or OpenRC/SysV exceptions
are added.

## Evidence and next unit

tests/linux_privileged_authorization.cpp covers valid simulated context, no
cache, missing intent/adapter/configuration without a check, invalid names,
UIDs, bus and generations, changes to each field after a check, disconnection,
wrong sender or cookies, deny/challenge/dismissed/retained/temporary, estimated
replies, exceptions, and external mutation of request/rules without changing
the copy. Positive results are fixtures, NOT authenticated polkit.

Revision 146 validation: Linux 172/172 plus 20 preflight/coordinator repetitions,
ten pure MSVC runs, and two relinked SDK consumers exiting 0. Initial and final
Linux builds were clean. Logs: work/privileged-authorization-*. No native
Authority or helper integration.

L05 remains E without closing the shared criterion or UAC. Next: a native
identity and Authority adapter over verified transport, with isolated private
tests and protocol rejection, still without installing a helper or connecting
an executor. K03 remains deferred, OpenRC revision 132 is unchanged, and I07
remains observer-only.

Revision 147 adds [native identity observation](LINUX_PRIVILEGED_IDENTITY.en.md)
from a borrowed dispatch message, with native socket and credential checks
and identity queries. Validation uses a simulated ABI and a real private
socket, not a real receiver or native CheckAuthorization. Do not retroactively
upgrade revision 146 evidence.
