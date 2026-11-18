# OpenRC in ServiceInfo: separate candidates and markers

Revision 143/K03. An explicit, read-only query that does not change Monitor
or the systemd/SysV/runit/s6 providers. No librc, CLI, shell, installation,
or script execution.

```cpp
#include <pcinsight/linux_openrc_services.hpp>
auto result = pcinsight::query_linux_openrc_service_inventory(
    "/etc/init.d", "/etc/runlevels", "/run/openrc");
if (pcinsight::metric_is_known(result)) {
    // Two ServiceInfo views from independent sources, NOT a combined identity.
    const auto& configured = result.value->configured_services;
    const auto& marked = result.value->marked_services;
    (void)configured;
    (void)marked;
}
```

The paths are examples, not universal configuration precedence. The caller
chooses all three scopes explicitly. Each collector retains its existing
context, pin, metadata, and two-pass inventory checks and limits. The host
serializes credential changes, setns, and fork. These are independent,
successive observations, not a combined or atomic snapshot or proof that
they describe the same installation or boot.

## No identity joins by name

`catalog` retains the original Metric from the [revision 131 catalog](LINUX_OPENRC_CATALOG.en.md);
`markers` retains the Metric from the [revision 130 marker reader](LINUX_OPENRC.en.md).
`configured_services[i]` corresponds to catalog.scripts[i], and
`marked_services[i]` to markers.entries[i], only within this call. The
view of an unavailable source stays empty, preserving its error or
permission status in the source Metric.

A file named alpha and a marker named alpha yield TWO separate rows, never
one merged row. Marker symlinks are not followed and their targets are not
authenticated. A name does not establish that a marker describes that
candidate or its current version. A ghost marker with no candidate and a
beta candidate with no marker remain separate. Missing markers do not imply
stopped state or erase an observation.

Opaque IDs use `openrc-file:` or `openrc-mark:` followed by the name's
bytes in hexadecimal. They are collision-free within each view and distinct
between roles. Their scope is the two-root catalog or the runtime root,
respectively; they are not global IDs, paths, commands, or action tokens.
The GUI must escape or represent names with non-UTF-8 bytes or control
characters. Do not merge by the text in name.

## Fields

The provider is `openrc-file-catalog` or `openrc-runtime-marks`.
linux_details is estimated: it describes filesystem observations, not
an inventory from an authenticated active manager. The observed name
is available when its source is complete and consistent, or estimated
when partial.

Configured entries: status is unknown and the status Metric has no value,
even with execute bits or runlevel references. Description and group are
empty, PID is absent, and main_pid is UNKNOWN. There is no shell parser for
description or evaluation of dependencies or defaults.

Markers: a single primary marker consistent with the mask yields
`marked-started`, `marked-stopping`, `marked-starting`, or `marked-inactive`,
with an estimated Metric. These are marker names, NOT running/stopped,
liveness, readiness, or crashed state. failed/hotplugged/wasinactive and
the original mask remain in source. Failed is not turned into an exit
cause, nor hotplugged into a group. A missing or conflicting primary
marker, unknown bits, or inconsistent state yields UNKNOWN. Permission
information is preserved. There is no PID: targets are not followed,
pidfiles are not read, and /proc is not searched by name.

Both views: group_kind and common strings are empty; slice/load_state/
active_state/sub_state/unit_file_state are UNKNOWN. Runlevels, stacks,
and directories do not become groups or effective enablement. No systemd
properties are invented.

## Quality, limits, and control

Each source is evaluated separately. A partial or unavailable catalog does
NOT invalidate an independent complete marker view, nor does a partial
marker view invalidate names from a consistent catalog. This does not
authorize associating the views. Outer Support is available only when both
sources are available and complete, and estimated when at least one source
is known but one is incomplete or unavailable. If neither has known data
there is no result; permission is required if either source reports it.
Available does NOT mean active-service coverage or authorization to control.

`max_entries_per_source` is 1–4,096 (default 1,024) and applies independently
to catalog scripts and marker names, yielding up to 8,192 common rows in
total rather than one shared global limit. Each collector's visit, payload,
runlevel, and reference limits remain in place. The pure projection rejects
inconsistent roots, names, duplicates, and bounds. There are no body reads,
joins, or additional I/O.

There are no handles or capabilities. Do not pass IDs or names from this
view to systemd, process actions, or scripts. This API does not create runtime
directories, run librc, query status through scripts, or call start/stop/restart.
At this checkpoint, the OpenRC revision 132 exception was not yet authorized;
“continue” did not grant it. The I07 observer is unchanged. There are no common
layout changes, persistence, workers, cache, ABA guarantees, or hard deadlines.
Refreshing renews the observation, not the permanent identity of a pathname.

## Evidence and next unit

tests/linux_openrc_services.cpp covers separate views with identical names
and distinct IDs, four markers and their flags, conflicts/invalid bits/
mismatches, permissions, independent partial or unavailable sources,
duplicates, raw-byte IDs, and empty results. Private Linux tests cover
candidates with and without a marker, a marker without a candidate,
broken or nonmatching targets not followed, preservation of a runlevel
reference matched by identity, bounds, rejection of a regular-file marker,
a missing runtime without creation, a sentinel script never executed, and
no FD leaks. MSVC tests projection, arguments, and fallback, NOT the Linux
backend. The installed SDK consumer includes and links the new header and
symbol. No real OpenRC init or service control is used.

Revision 143 validation: Linux 169/169 plus 30 OpenRC repetitions, 10 pure
and fallback MSVC runs, and two installed SDK consumers exiting 0. Initial
and final Linux builds were clean. Logs: work/openrc-services-*. These are
local tests, not remote CI or acceptance on real OpenRC.

K03 remains E for selection/integration, acceptance with real managers,
and unavailable control. The next functional unit is explicit service
provider selection in Monitor, reusing these views while preserving the
systemd default and each provider's limits, with no new actions or silent
fallback.
