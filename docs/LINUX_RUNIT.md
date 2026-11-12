# Explicit runit service protocol provider (K03, revision128)

`pcinsight/linux_runit.hpp` adds a separate Linux provider; it does not overload
systemd unit names or auto-select a manager in Monitor. The caller selects a
trusted runit service tree/directory. No `sv`, shell, service script, external
collector, SVDIR/PATH lookup, package installation or elevation is performed.

## Query and inventory

`LinuxRunitService::open(absolute_directory)` pins the service directory,
supervise directory and real control/ok FIFO nodes. Ordinary service/supervise
directory symlinks are supported; magic links and FIFO-node symlinks are not.
Linux openat2 is required; unsupported kernels fail without unsafe fallback.
Only our own validated O_PATH pins are reopened for IO, avoiding a path swap
that could open a device instead of the selected regular file/FIFO.

`read()` requires a reader on the pinned ok FIFO and two matching20-byte status
observations, with metadata/endpoint/caller checks. A stale status file without
an ok reader is UNKNOWN, not running/down. This is point-observation consistency,
not an atomic snapshot or ABA detection. Boot/procfs/PID/start/UID/namespace and
effective-UID caller context is guarded; coordinate forks and FD lifetimes.

The20-byte format and state semantics follow the upstream
[runsv implementation](https://github.com/void-linux/runit/blob/master/src/runsv.c#L160-L196):
raw12-byte TAI64N change stamp, little-endian reported PID, pause/wanted-state/
TERM flags and down/run/finish mode. A finish transition with PID0 is valid.
Malformed widths, flags, nanoseconds and impossible run/down PID combinations
are rejected. TAI is NOT converted to Unix time/uptime. Reported PID belongs to
the supervisor's namespace, not necessarily the observer; never use it as an
identity token for process details/signaling.

`query_linux_runit_inventory(root,max_entries)` enumerates a pinned explicit
root, including ordinary symlink service entries. Hidden entries/non-directories
are skipped; directories lacking usable supervision remain rows with unknown
status, never asserted running services. No standard description/group metadata
is invented. Results are sorted and bounded1..4096 rows(default1024),16384 visits
and8MiB names/paths. Enumeration/bound/membership changes expose incomplete with
diagnostics; per-row status absence is distinct from an unavailable root.

## Explicit requests, not completion certificates

`handle->request(action,true)` sends the native FIFO request: start=u, stop=d,
restart=tcu, matching [sv's action mapping](https://github.com/void-linux/runit/blob/master/src/sv.c#L304-L318)
and [documented control interface](https://smarden.org/runit/runsv.8).
These are literal protocol bytes, not commands passed to a shell. The supervisor
may itself run its configured/custom control hooks as part of servicing them.

Absent explicit authorization returns before control IO. Changed pinned endpoint
identity, caller context, missing ok reader or unavailable control endpoint is
refused. Selected handles stay tied to the original directory/FIFO objects, not
whatever a later pathname replacement names. Filesystem tree trust is required:
FIFO protocol has no authenticated peer credentials or per-request job IDs.

The result reports requested/written byte counts. Success means FULL FIFO WRITE,
not that the supervisor consumed the command, restarted a process or passed a
readiness check. `completed` deliberately stays unknown; GUI can refresh status
but must not equate delivery with confirmed completion. No check/run/finish
script is executed by the library. Partial/failed writes are not retried as a
new operation; outcome/byte count is retained. Return data is prepared before
write, with no post-delivery allocation that loses the result.

Writes are nonblocking and small/atomic. SIGPIPE is blocked only in the calling
thread; newly generated EPIPE SIGPIPE is consumed while preexisting pending
SIGPIPE and the original mask are preserved. No global handler/SIG_IGN change.
Concurrent external SIGPIPE delivery must be coordinated by the host. General
filesystem/native operations can still block; no hard wall-time bound promised.

## Verification and remaining K03 scope

Release CTest uses private directories,20-byte fixtures and FIFO readers, not a
real supervisor. It covers decoder errors/known down0/finish0, alias inventory,
unmanaged/unknown rows, bounds, u/d/tcu delivery, denied action, replaced FIFO,
stale-file refusal, deterministic EPIPE, existing pending SIGPIPE, mask and FD
cleanup. No real service was started/stopped, no BPF probe or utility executed.
This is native Linux PROTOCOL IO evidence, NOT real runsv/service acceptance.

The installed SDK is also compiled/linked from a separate C++ consumer. Logs:
`work/runit-*`. K03 stays E for confirmed transitions/real-supervisor acceptance,
OpenRC/s6/SysV providers and integration. No universal init/control claim.

Revision128 follows the user's "prosigue" conservatively along the recommended
observing-library path. I07's unverified automatic cause remains deferred/open;
no new startup launcher is authorized or developed, no criteria erased.

Validation128: Linux155/155 without skips+20 repeats; installed SDK existing and
new-header C++ consumers both exit0. Linux final build clean. MSVC10 decoder/
unsupported-contract repeats, not Windows runit support. runsv was not found
in this WSL PATH (read-only lookup); none was installed or executed. K03 remains
open, C60/E9/P4/B2,3X:80% closed/20% open, no fractional credit or new task ID.
# Observación posterior a solicitudes (revisión129)

`LinuxRunitTransition` añade una solicitud única con consentimiento y polling
explícito hasta objetivo/deadline. restart exige época TAI posterior, no solo
otro PID. Objetivo observado NO equivale a readiness o causal completion;
`delivery.completed` sigue UNKNOWN. Sin reintento/hilo/script/cancelación.
Contrato y evidencia: [LINUX_RUNIT_TRANSITIONS.md](LINUX_RUNIT_TRANSITIONS.md).
La aceptación con runsv real continúa pendiente.

Revisión141 añade [consulta explícita ServiceInfo](LINUX_RUNIT_SERVICES.md):
etiquetas reported-*, PID común desconocido y evidencia original conservada.
No cambia Monitor ni los contratos de control/observación anteriores.

