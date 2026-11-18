# Runit in ServiceInfo: reported state, not process identity

Revision 141/K03. This explicitly queries a runit tree trusted by the caller.
It does not change Monitor or systemd/SysV queries. No init autodetection,
SVDIR, PATH, shell, external utilities, scripts, or control requests are used.

```cpp
#include <pcinsight/linux_runit_services.hpp>
auto result = pcinsight::query_linux_runit_service_inventory("/var/service");
if (pcinsight::metric_is_known(result)) {
    for (const auto& service : result.value->services) {
        // Status: reported-run, reported-down, reported-finish, with optional
        // /paused, or unknown. Do not call reported-run "verified running."
        const auto& report = service.linux_details.value->status;
        // A label is estimated; check availability.
        (void)report;
    }
}
```

The path is an example. The host selects its actual tree without assuming a
universal location. `source` retains LinuxRunitInventory, its original rows,
and diagnostics. `services[i]` corresponds only to `source.services[i]` from
the same call. The private projection is pure: it does not repeat reads or
join snapshots by name.

## What the projection exposes

`linux_details.provider` is `runit-reported-inventory`. Its Support is
estimated: it contains protocol reports, not authenticated kernel process
enumeration. The name is the observed filename, available for a consistent
inventory or estimated for a partial one. IDs are `runit-report:` followed by
the name's bytes in hexadecimal. They are opaque and collision-free within
that inventory, scoped to its root; they are not global IDs, paths, commands,
or tokens. Names may contain non-UTF-8 bytes or control characters. The caller
must escape them for display.

With a consistent inventory and a valid available record:

- State down maps to `reported-down`.
- State run maps to `reported-run`.
- State finish maps to `reported-finish`.
- Paused adds `/paused` to the reported state label.

The status Metric is estimated, and its note explicitly identifies the source.
`running`, `stopped`, `enabled`, and `ready` are not used as conclusions about
liveness, policy, or readiness. Raw want_up, term_sent, and TAI64N remain in
source without conversion to enablement, uptime, or Unix time. Invalid TAI
nanoseconds, PID, enum, or inconsistent down/run/PID combinations leave status
UNKNOWN. A source without an available record, denied permission, or a partial
or unstable inventory also yields unknown status. The Metric preserves the
reason or permission status. Previous state is not reused.

**The common PID is absent and main_pid is UNKNOWN**, even when reported_pid
is positive. The format reports a number in the supervisor's namespace, not
a host process identity. The original number, including valid zero in
down/finish, remains in source. The provider does not search /proc by name or
use that number to signal or terminate processes. Finish does not imply a live
daemon.

Description and group are empty. The description/slice/load_state/active_state/
sub_state/unit_file_state metrics are UNKNOWN, and group_kind is empty. The
provider does not invent descriptions from names, script metadata, groups
from the root, or systemd/Windows properties. Outer Support describes inventory
consistency, not the availability of every report. An unmanaged row can be
UNKNOWN in a complete inventory.

## I/O, scope, and actions

This reuses query_linux_runit_inventory unchanged: 1–4,096 rows, 16,384 visits,
8 MiB payload, pinned endpoints, context and metadata checks, and two 20-byte
status reads. The reader requires a reader on supervise/ok; a status file
alone is not enough. Opening the write end of ok to check for a reader does
NOT authenticate the peer or verify the reported process. No control bytes
are written. There is no new worker, cache, persistence, read of run/check/finish,
or hard deadline. There is no atomic snapshot or guarantee of pathname
identity after the query.

The projection rejects inconsistent roots, names, duplicate rows, or
directories and oversized payloads. It does not match by basename across
trees. It preserves evidence without retroactively improving the guarantees
of the original inventory.

The view contains no control handle or capability. An explicit runit action
uses [LinuxRunitService](LINUX_RUNIT.en.md) with its own pinned selection,
consent, and delivery contract. Do NOT pass this ID to systemd actions or
reported_pid to process actions. Refresh and pin a new selection; an old
pathname does not authorize acting on any replacement. Delivery and observed
state still do not imply completion, readiness, or causation. The revision
128/129 contracts are unchanged.

## Evidence and next unit

tests/linux_runit_services.cpp covers portable pure projection, states and
paused, PID always absent in the common model, retained flags and records,
invalid ranges and enums, permissions/Support/partial coverage, out-of-scope
directories, duplicates, raw-byte IDs, and empty inventory. Private Linux
tests cover a directory alias, a 20-byte status with an ok reader, an unmanaged
row, bounds, a stale file without an ok reader, a control FIFO with no bytes
written, a sentinel script never run, and no descriptor leaks. No real runsv
or services are used. MSVC tests projection, arguments, and fallback only,
not the Linux backend. A separate SDK consumer includes and links the installed
header and symbol.

Revision 141 validation: Linux 167/167 plus 30 runit repetitions; 10 pure and
fallback MSVC runs; two installed SDK consumers exited 0. Initial and final
Linux builds were clean. Logs: work/runit-services-*. These were local tests,
not remote CI or acceptance against a real runsv.

K03 remains E for other providers, integration, and acceptance with real
supervisors. The next functional unit is explicit s6 projection into the
common model, without treating a persistent status file as proof of a live
supervisor or process. At this checkpoint, OpenRC revision 132 and SysV script
control had no authorized exception. The I07 observer is unchanged.
