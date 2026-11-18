# K03: public OpenRC selection and control

Revision 159, 2026-11-02. Public integration, not real OpenRC acceptance.
Installed header: `pcinsight/linux_openrc_control.hpp`. It does not change
existing public layouts, Monitor, SystemActions, or data readers. The
[revision 148 control exception](LINUX_OPENRC_CONTROL_DECISION.en.md) stays
opt-in, with no collection tools or privilege elevation.

## Use from the caller's GUI

```cpp
#include <pcinsight/linux_openrc_control.hpp>

pcinsight::LinuxOpenrcControlConfig config;
config.enabled = user_enabled_openrc_control; // False by default.
config.runtime_path = trusted_absolute_runtime_path;
config.init_directory = trusted_absolute_init_d_directory;
// owner_uid=0 and allow_sticky_ancestors=false by default.

auto selection = pcinsight::LinuxOpenrcService::open(config, "mi-servicio");
if (selection.value && user_confirmed_restart) {
    auto request = (*selection.value)->request(
        pcinsight::ServiceAction::restart, true);
    // Retain the runner EVEN if request.result reports an uncertain error.
    if (request.runner) pending.push_back(request.runner);
}
```

`pending` belongs to the caller. From its timer or worker, `runner->wait(0)`
polls without waiting; syscalls still have no hard deadline. When the Metric
has a value and exited=true, display exit_code or signal and remove the
already-reaped handle. An observation error calls for diagnosis, not an
automatic new request. Do not turn exit 0 into “service running.” The test
model returns 3/7/11 specifically to check that separation. The GUI and
its build belong to the caller; it need not set up its GUI CMake now.
The included small consumer project only tests the installed SDK and is
not an integration requirement.

## Selection, consent, and expiration

`open` only inspects metadata and handles FDs. It never launches the runtime,
script, rc-service, or shell. The configuration is trusted and copied;
changing it afterward does not change or revoke existing selections.
init_directory must be absolute and nonempty, and name a bounded literal
ASCII basename with no slash, NUL, options, spaces, metacharacters, dot,
or dot-dot. An inventory index or started marker does not confer authority.

The selection retains runtime/script FDs and their directory guards, not
just strings. Each request requires independent consent and accepts only
three verbs. Missing consent and invalid verbs do not consume the selection.
Before releasing a child, the library revalidates the ORIGINAL files,
paths, metadata, and context. Replacement, changed mode/metadata, or a
different directory, process, namespace, or UID results in stale_target:
nothing is dispatched and the selection is permanently revoked. Restoring
the name does not revive it. No different target is silently reopened.

Returning a child consumes the selection even if the ACK is uncertain;
a second attempt is not allowed. For another action, the GUI must open a
new selection after a new explicit user intent. That alone does not resolve
the unknown outcome of a prior action. `close()` revokes future requests
without cancelling or reaping the runner or terminating the service.
Selection methods are serialized. The host follows the fork, FD, context,
and reaper restrictions in [the executor guide](LINUX_OPENRC_CONTROL.en.md).
close retains the pins until destruction; release the object if its name or
history is no longer needed. Real and effective UID/GID are pinned and
revalidated. The host still must serialize supplementary group and credential
changes; no snapshot of all privileges is promised.

## Result and resources

LinuxOpenrcControlResult retains ActionResult, runtime_may_have_executed, and
an optional runner. The dispatch message and boolean do not prove execution
or live service state. The runner owns the pidfd; process_id is diagnostic,
not permission to signal by PID. wait(0..30000ms) retains the historical
result and reaps through P_PIDFD, with no PID fallback or hidden reaper.
The caller must keep each runner until it is reaped. Destroying the selection
does not cancel it. Abandoning a live runner can leave a zombie.
terminate_runner requires confirmation and signals only the runner. It is
NOT equivalent to ServiceAction::stop and does not affect the group, tree,
or dependencies.

The public wrapper, result, and ACK messages are allocated before dispatch.
Later transfers move the handle and messages without allocation, so a
failure to reserve these objects does not abandon an already released child.
The revision 158 executor's limits, minimal environment, FD closure,
NO_NEW_PRIVS, and 2-second ACK budget are retained. There is no autodetection,
shell fallback, installation, new privilege, or host bus connection.
Platforms without a backend report unsupported, not an invented zero.
A selection does not guarantee kernel or runtime support.

An FD does not freeze content, and OpenRC reopens the script by name. There
is no atomic snapshot, ABA guarantee, or defense against a hostile owner
or root. pthread_atfork callbacks and the trusted runtime, scripts,
configuration, and dependencies remain under the host's control. This API
is experimental until tested against real OpenRC versions.

## Verification

`tests/linux_openrc_control_public.cpp` imports ONLY public headers. It
uses the private revision 158 ELF model, never real OpenRC, and covers
default-off, invalid names, consent, three verbs, copied configuration,
script/runtime/mode/directory changes, persistent revocation/close, consumed
selection, post-fork context, runner ownership, exit/timeout/explicit
termination, FD handling, and cleanup. `tests/openrc_consumer` builds that same
program against the installed SDK without private includes. Release assertions,
the 20-second alarm, and the 30-second CTest timeout remain active.

Remaining K03 work: isolated acceptance with real OpenRC/runit/s6 and a
SysV control decision (not authorized by the OpenRC exception). The completed
integration is not counted as pending, and the model is not mistaken for
acceptance on a real system. L05 deployment and Windows stay deferred, and
I07 remains observer-only.

Final revision 159 evidence: Linux 188/188, five OpenRC tests repeated ten
times (50 executions), 20 MSVC portable runs, and three reinstalled/relinked
SDK consumers exiting 0, including the new program without private includes.
FD handling and cleanup passed. Final builds had no code warnings. Initial
MSVC D9025 from /UNDEBUG overriding /DNDEBUG is expected to keep Release
assertions active. Logs: `work/openrc-public-*`, especially *-delivery.log,
build-delivery-clean.log, and final focused public entry-point runs.
A read-only PATH check, command -v runsv s6-supervise openrc-run rc-service,
returned exit 1 with no output. It does not prove global absence, and none
were run or installed. The separate SysV permission question was sent and
remained pending; there is no implicit authorization.
No remote CI or real-system acceptance. K03 remains E, with
C60/E10/P3/B2 plus 3 excluded X: 80% closed, 20% open.
