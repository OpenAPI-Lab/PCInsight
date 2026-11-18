# SysV in the common ServiceInfo model

Revision 140/K03. An explicit query that does not change Monitor's automatic
selection or existing systemd inventory and control. It does not run scripts
or external tools and does not require an active SysV service to read files.

```cpp
#include <pcinsight/linux_sysv_services.hpp>

auto result = pcinsight::query_linux_sysv_service_inventory(
    "/etc/init.d", "/etc", 1024, 8 * 1024 * 1024);
if (pcinsight::metric_is_known(result)) {
    for (const pcinsight::ServiceInfo& service : result.value->services) {
        // service.name is the candidate filename, NOT an authenticated daemon.
        if (service.linux_details.value) {
            const auto& description = service.linux_details.value->description;
            if (pcinsight::metric_is_known(description)) {
                // Declared LSB text; use *description.value in the caller's view.
            }
        }
    }
}
```

## Projection contract

`LinuxSysVServiceInventory.source` retains the full [revision 139 described
catalog](LINUX_SYSV_DESCRIBED_CATALOG.en.md): explicit roots, candidates,
runlevel references, availability, diagnostics, budget, and attempts.
`services[i]` corresponds to `source.catalog.scripts[i]` ONLY within this
result. External or unresolved references do not add rows. The private
projection is pure: it does not reread files or join by name. Misalignment,
duplicate or invalid names, or more than 4,096 candidates yield no result.
Descriptions are not shifted to another row, and truncation does not hide
the problem.

`id` is `sysv-file:` followed by the name's bytes in hexadecimal. It is
opaque, ASCII, lossless, and collision-free within a catalog. It is scoped
to the roots of THIS inventory and is NOT a global key across hosts, roots,
or snapshots, a path, a command, a systemd name, or an action token. Apps
combining inventories must retain their scope and provider. `name` preserves
filename bytes, which may not be UTF-8 or safe terminal text; the GUI must
escape or represent them appropriately. LSB descriptions undergo the
[revision 138 parser's](LINUX_SYSV_LSB.en.md) validation and normalization.

`linux_details.provider` is `sysv-file-catalog`. Its Support is estimated
because it projects file candidates rather than enumerating services confirmed
by a manager. `linux_details.name` is the observed filename: available in a
consistent inventory, estimated in a partial one. This does NOT identify a
live daemon.

`description` uses the declared Short-Description if available, falling back
to the declared Description if available. The common field and the Metric
contain the same text. Both original descriptions remain in source. Permission
denial or absence and their notes propagate; there is no fallback to the name,
command, or invented text. A partial or unstable catalog or an unavailable
source leaves the description UNKNOWN even if a fixture supplied fields.
Previous query results are not reused.

Runtime state: `status="unknown"`, with no value in the status Metric. PID is
absent, main_pid is UNKNOWN rather than a substitute zero, and group and
group_kind are empty. The slice/load_state/active_state/sub_state/unit_file_state
metrics are UNKNOWN, and the common strings are empty. Running/stopped or
enabled/disabled are not inferred from S/KNN, executable bits, missing links,
headers, pidfiles, or process names. A runlevel does not become a
Windows/systemd group or effective policy.

Outer Support describes filesystem inventory consistency, NOT active service
coverage or availability of every description. A partial catalog yields
estimated. A complete catalog can still lack descriptions due to budget,
permission, or a missing header. Attempting all reads does not guarantee a
field. The revision 139 limits, reservations, and identity guards still apply.
There are no additional body reads, cache, worker, persistence, or hard
deadline. Refresh to get a new observation; there is no atomic snapshot
or permanent identity.

## Control and integration

This view adds no SysV control. Its IDs must NOT be passed to
control_linux_service, which remains an explicit-selection systemd API.
A filename resembling .service does not authorize control of a systemd unit
with the same name. The GUI should select actions by provider and capability,
not the name's text. The library does not install an init system or execute
scripts for status or control. At this checkpoint, OpenRC revision 132 and
SysV control scripts had no authorized exception. The I07 observer is
unchanged, as are the common ServiceInfo/LinuxServiceDetails layouts.

## Evidence and next step

`tests/linux_sysv_services.cpp` covers portable pure projection, preference
for Short-Description with Description as fallback, unavailable fields,
permissions, contradictory Support, partial inventory, alignment/duplicates,
IDs from non-UTF-8 names, and empty catalogs. Native Linux tests cover private
trees, association with a differently suffixed reference, a zero budget, a
broken symlink, a skipped FIFO, a sentinel to detect execution, and no FD leaks.
MSVC validates pure projection, arguments, and fallback, NOT the Linux
collector. The installed SDK consumer includes the public header and links
the new symbol.

Revision 140 validation: Linux 166/166 and 40 SysV repetitions, 10 MSVC
pure projection and fallback runs, and two installed consumers exiting 0.
Logs: work/sysv-services-*. Initial /mnt/c clock-skew warnings below 1 s
were retained. The final incremental build and full suite were clean,
without changing clocks or mtimes. These are local tests, not remote CI
or acceptance with a real init system.

K03 remains E for other providers' integration, control/liveness, and real
acceptance. The next functional unit is [explicit runit projection](LINUX_RUNIT_SERVICES.en.md)
into the same model, keeping reported PID, unproven liveness, and control
through its own pinned selection separate. Monitor is not enabled
automatically, and a failed explicit systemd query does not switch managers.
