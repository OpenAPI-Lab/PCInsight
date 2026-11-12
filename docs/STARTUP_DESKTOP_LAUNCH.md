# Controlled direct XDG entry launch (internal, revision122)

`src/linux_desktop_launch.hpp/.cpp` connects an effective caller XDG autostart
entry to the [pre-exec gate](STARTUP_GATED_LAUNCH.md), without accepting a
separate caller-supplied command. It is an explicit MANUAL action, not login
activation, an automatic-startup receipt, or a public SDK API. RunNewTask and
existing archive/layout/association APIs are unchanged.

## Resolution and supported subset

`resolve_linux_desktop_route(properties)` is a pure bounded transformation of a
snapshot to `LinuxDesktopRoute{entry_id,request}`. A returned route is not a
file authentication token. The native launch wrapper obtains its OWN properties
using the existing effective-inventory/precedence/stat-checked reader; it never
trusts a caller-forged route or caller-provided argument vector.

The parser follows the two escaping stages and whole-token double quoting of
the [Desktop Entry Exec specification](https://specifications.freedesktop.org/desktop-entry/latest/exec-variables.html).
It produces literal arguments, including empty quoted arguments and escaped
dollar/backtick/backslash. Reserved unquoted characters, malformed escapes,
concatenated quotes and non-ASCII Exec are rejected. There is no shell parsing,
environment expansion or PATH lookup. Executable must be absolute and contain
no equals sign. Quoted field codes are rejected. Standalone file/URL codes are
omitted for this no-file-input action, with at most one file/URL code. Standalone
deprecated codes are omitted; %% becomes a literal percent. Embedded codes,
unknown codes and %c/%i/%k are explicitly unsupported, not guessed.

Type=Application, Name and Exec are required. Terminal/D-Bus/GPU-preference
launch paths are unsupported unless absent/false; they are not silently
converted to direct invocation. Path is string-unescaped, absolute or empty
(empty inherits cwd). The distinction between Exec, D-Bus and terminal routes
comes from the [recognized desktop keys](https://specifications.freedesktop.org/desktop-entry/latest/recognized-keys.html).
Hidden/disabled entries and unimplemented vendor extensions are rejected;
X-GNOME-Autostart-enabled=true is supported. Duplicate main-group keys are
ambiguous and rejected. Desktop Action groups cannot replace the main Exec.
Bounds: 65536 fields, 1MiB aggregate fields, 32768-byte values and 4096 tokens;
the existing source reader and launch primitive also enforce their own limits.

This is deliberately NOT every legal desktop command. Unsupported syntax stays
unsupported. An explicitly requested wrapper ELF (including a shell executable)
is opaque: its descendants/delegation are NOT resolved into an application.
There is no IMPLICIT shell or script fallback; this is not a command sandbox.

## Native action and checks

`launch_linux_desktop_entry(id,consent,arm)` denies absent consent BEFORE reading
files. Only Linux has a native provider. It reads the caller's effective XDG
entry, requires enabled status and exact available/true standard eligibility
AND autostart condition, then resolves its direct request.

The gate pins the requested ELF and creates the pidfd-authenticated blocked
child. Before calling `arm(route,target)`, the wrapper re-reads effective
properties and eligibility. After that callback it checks them AGAIN. Any parsed
field/order/ID change, shadowing, unknown eligibility or denial cancels and reaps
the still-gated child; no program is entered. The gate then does its existing
child/caller/credential/image-stamp revalidation before release. Its borrowed-FD,
privilege, signal, callback exception and caller waitpid contracts still apply.

The desktop reads are NOT an atomic filesystem/eligibility transaction. The
historical statement is the resolved snapshot chosen by this explicit action,
not that a mutable desktop file must still have those contents at kernel exec.
Comments/whitespace not represented by parsed fields are not a byte identity.
Host must serialize environment/credentials/cwd/FDs/reapers/forks as for the gate.
Filesystem/native condition checks may block. No external collection utility,
new dependency, service installation, startup registration or GUI activation.

## Verified behavior

Normal Release `linux-desktop-launch` CTest uses only its own helper ELF and
private XDG directories. It verifies literal arguments/no shell effects, empty
arguments and escaping in pure tests, field-code rejection/omission, optional
Path, duplicate keys, ignored Action Exec, effective shadow rejection, disabled
and unknown-condition rejection. Both a harmless parsed-field mutation and a
disable during arm cause stale-target cancellation/ECHILD/no marker; FD census
is unchanged. CTest loads NO BPF probes.

`--authorized-private-desktop` additionally arms the existing
[selected-exec provider](STARTUP_SELECTED_EXEC.md) inside that route callback.
The authorized WSL run positively verified this private desktop's controlled
child had no event before release, and had the matching pinned first executable
AFTER waitpid/reap. Capture references are removed and FD count returns to its
initial value. No real startup app or login launch was activated.

Evidence: `work/desktop-launch-*` configure/build/CTest/native/SDK-consumer logs.
Linux150/150 without skips plus20 focal repetitions, installed SDK consumer exit0;
MSVC10 pure/denial/no-Linux fallback repetitions, not Windows launch support.
An initial make warning reported a dependency timestamp 0.069s in the future.
The final incremental rebuild and full150-test rerun passed without warnings;
no clocks or file timestamps were adjusted. Both logs are preserved.
I07 remains E: managed launch receipt/cause integration, other reapers and
descendants still need work. Manual launch is NEVER automatic-startup evidence.
N03 is not fixed; no new task IDs, fractional credit or Windows features.

Revision123 composes this route with the selected-exec sensor in a one-shot
[manual receipt session](STARTUP_DESKTOP_RECEIPT.md). It still does not certify
automatic-startup cause or change the public RunNewTask API.
