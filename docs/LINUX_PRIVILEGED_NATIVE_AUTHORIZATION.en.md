# L05/Linux: native CheckAuthorization, without an executor

Revision 149, 2026-11-02. Implements the native query and connects it to
the [revision 146 coordinator](LINUX_PRIVILEGED_AUTHORIZATION.en.md). It does
NOT install or activate a helper, receiver, policy, prompt, or privileged
action. Isolated validation uses a simulated libsystemd ABI and private Unix
sockets, not a real Authority. No new public API, layout, format, or
Monitor/SystemActions integration.

## Input and fixed request construction

check_native_privileged_authorization requires a borrowed incoming message
during dispatch, trusted mechanism configuration, expected context, an explicit
request, and mechanism rules. It first repeats [revision 145 preflight](LINUX_PRIVILEGED_PREFLIGHT.en.md).
It makes no query without allowed intent, target, and operation. It repeats
the [revision 147 identity observation](LINUX_PRIVILEGED_IDENTITY.en.md) and
requires an exact match to the pinned context before sending CheckAuthorization.
No arbitrary subject UID or name, client-supplied polkit ID, or root default.

The function builds a CheckAuthorization message for the already verified
UNIQUE Authority owner, using the PolicyKit1 Authority path and interface.
The system-bus-name subject uses the original message's sender name. The
action_id is fixed for each start/stop/restart verb, and details contain
only unit and verb from the validated, copied request. No arguments, commands,
paths, environment variables, scripts, or text expansion. The D-Bus message's
autostart and interactive-authorization flags are zero.

The polkit AllowUserInteraction API flag defaults to zero. It becomes one
only when explicit intent is validated AND the mechanism allows interaction.
Tests with one use only the simulated ABI, with no system prompt. The
function does not automatically activate an authentication agent or check
permission during data sampling.

## Correlation, parsing, and quality

One synchronous sd_bus_call with a five-second timeout, no retry, and no
fallback. The library transports the message and assigns its cookie. The reply
must be a method return from the pinned owner, with exact (bba{ss}) signature
and a nonzero call cookie matching the reply cookie. The body contains two
booleans represented as 0/1 and an array of string pairs. Missing data,
incorrect type/sender/signature/cookie, an invalid container, or trailing text
yields UNKNOWN.

Limits: 64 details, 1–128 bytes per key, 0–4,096 bytes per value, and up to
16 KiB across keys and values. Duplicate keys and exceeded limits invalidate
the entire result without picking a value or truncating. Dismissal, retention
after a challenge, and temporary ID are preserved based on nonempty presence;
other keys are omitted only after validation and bounds checks. D-Bus/UTF-8
decoding relies on the trusted libsystemd parser. A reply is not reinterpreted
from arbitrary client-supplied bytes.

The function then repeats revision 147 identity observation. Bus, caller, UID,
Authority, and generations must match the expected context. Changes or failures
discard the entire reply. An available reply is valid adapter data, **not a
grant or execution**. The revision 146 coordinator conservatively rejects
denial, challenges, dismissal, retention, and temporary authorization. Do
not turn a raw is_authorized=true into reusable permission.

## Per-request adapter

native_privileged_authorization_api freezes configuration, request, and rules.
Its observe/check callbacks reuse the revision 147 native observation and
the query above. The coordinator's plan must match the exact snapshot
(unit, operation, policy_action), or no query is sent. Each coordinator call
queries again without a cache. Message and bus remain borrowed. The entire
adapter lifetime and use must stay within the valid, serialized handler.
Do not retain callbacks afterward or share them across requests.

Frozen configuration does not implement a reload watcher. The mechanism
must serialize rule, credential, namespace, and FD changes, use correct
generations, and construct a new adapter per request. This path does not
yet have a real receiver or payload parser. C++ structs a fixture can
fabricate do not replace them.

## What this does not establish

No deployed helper, execution, elevation, rules or root-owned configuration
loader, secure bootstrap/loader, acceptance with a real bus/Authority/agent,
or guaranteed remote cancellation. cancellation_id is empty. Local timeout
does not guarantee cancellation of remote authentication work. Interaction
defaults to zero but does not replace identity or authorization requirements.
Administrative polkit policy can authorize without prompting; fresh password
verification is not promised.

The query has its own timeout, but revision 147 native observation makes
several calls, with no hard overall deadline, atomicity, or ABA guarantee.
Revalidating selection, identity, and policy before future execution and
consuming authorization in the same flow remains pending. No executor
consumes this unit's reports. Configuration, endpoint, and libsystemd must
be protected before privileged use; the fixture's LD_LIBRARY_PATH is NOT
an acceptable deployment configuration.

## Evidence and next step

tests/linux_privileged_native_authorization.cpp uses a simulated ABI to check
the full message, subject, target, verb, and flags. It uses a real private
socket and SO_PEERCRED without connecting to host D-Bus or polkit. Cases cover
accept/deny/challenge/dismissed/retained/temporary/unknown; type, sender,
signature, cookie, booleans, truncation, and struct/array/entry shape;
duplicates, 64/65 bounds, key/value limits, exact and excessive 16 KiB
payloads, trailing data, changed owner, timeout or construction failure
without retry, simulated interaction 0/1, mismatched context or plan, no
send without intent, and no FD leaks. MSVC covers arguments and fallback only.

Revision 149 validation: Linux 176/176 plus 60 L05 repetitions, ten MSVC
argument/fallback runs, and two relinked SDK consumers exiting 0. Initial
clock-skew warnings were preserved. The final incremental build and 176-test
suite were clean without changing clocks or mtimes. Logs:
work/privileged-native-auth-*. No real broker or Authority, or remote CI.

Primary sources:
[Authority API](https://polkit.pages.freedesktop.org/polkit/eggdbus-interface-org.freedesktop.PolicyKit1.Authority.html),
[sd_bus_call](https://www.freedesktop.org/software/systemd/man/latest/sd_bus_call.html),
and [sd-bus ABI](https://raw.githubusercontent.com/systemd/systemd/main/src/systemd/sd-bus.h).
The limits, unit/verb details, and rejection policy are PCInsight contracts,
not replicas of all polkit policy or implementation behavior.

Update 150: the [restricted receiver](LINUX_PRIVILEGED_RECEIVER.en.md) was
implemented and tested with REAL private libsystemd and broker and a simulated
Authority. Revision 149 evidence remains historical. No executor, deployment,
or real polkit policy; L05 remains E. Next: protected configuration and
lifecycle without installation or elevation.

Historical revision 149 plan: an isolated receiver with a fixed payload
and real transport identity, without an executor, installation, or activation
of user services. Secure bootstrap/configuration and execution integration
were still pending. OpenRC revision 148 permission stays limited to explicit
control; SysV is not included. K03/I07/UAC and data collection restrictions
are unchanged.
