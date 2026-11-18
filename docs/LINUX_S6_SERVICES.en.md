# s6 in ServiceInfo: file evidence, not current liveness

Revision 142/K03. This explicitly queries an s6 scan tree trusted by the host
without changing Monitor or systemd/SysV/runit queries. There is no automatic
init detection, CLI, shell, script execution, installation, or control request.

```cpp
#include <pcinsight/linux_s6_services.hpp>
auto result = pcinsight::query_linux_s6_service_inventory("/service");
if (pcinsight::metric_is_known(result)) {
    for (const auto& service : result.value->services) {
        const auto& status = service.linux_details.value->status;
        // Check the Metric; record-* labels do NOT establish running or readiness.
        (void)status;
    }
}
```

The path is an example, not a universal location. `source` retains the
original s6 inventory, its records and diagnostics, and logger/FIFO metadata.
`services[i]` corresponds only to `source.entries[i]` from the same call.
The private projection is pure: it does not reread files or join snapshots
by name.

## Fields and availability

The provider is `s6-status-record-inventory`. The linux_details Metric is
estimated because it reports files rather than authenticated enumeration
from an active supervisor. The name is the observed relative entry, available
in a consistent inventory or estimated in a partial one. Opaque IDs are
`s6-record:` followed by the relative name's bytes in hexadecimal, including
the logger separator. They are collision-free within the inventory, NOT
global identities, paths, commands, or action tokens. The GUI must represent
or escape filenames with non-UTF-8 bytes or control characters. Logical scope
does not imply physical confinement: directory aliases can resolve outside
the tree.

Projected status, only with an available record and consistent inventory:

- Nonzero raw PID: `record-pid-present`.
- Raw PID zero: `record-pid-zero`.
- Finishing appends `/finishing`; paused then appends `/paused`.

The status Metric is estimated with a note identifying the historical source.
No label asserts supervisor or process liveness, running/stopped, readiness,
or an authenticated peer. Ready is not published as a conclusion:
`ready_reported` with PID 0 remains a valid record. want_up is not interpreted
as enabled. Both TAI64N stamps remain raw, not Unix time or uptime. Invalid
nanoseconds make status UNKNOWN. A large u64 PID is NOT truncated to 32 bits
or rejected solely because it does not fit in a local PID.

The common PID is always absent and main_pid always UNKNOWN. The full u64
PID and u16 wait word remain in source: neither is a verified host number,
process token, or exit code. The provider does not follow /proc entries or
pidfiles or signal that number. Description and group are empty. The
description/load_state/active_state/sub_state/unit_file_state/slice metrics
are UNKNOWN, and group_kind is empty. Logger/parent_name is a one-level
association in source, NOT a Windows/systemd group, credential, or script
metadata.

With no available record, denied permission, or a partial or unstable
inventory, status is unknown and the Metric has no value but preserves the
reason or permission status. The last value is not reused. Outer Support
describes filesystem inventory consistency, not active coverage. A complete
inventory can contain unmanaged entries and unavailable records.

## Scope, guards, and actions

This retains the [query_linux_s6_inventory contract](LINUX_S6_INVENTORY.en.md):
visible direct directories and aliases plus ONE log level, bounds of 1–4,096
rows/16,384 visits/8 MiB payload and 64 diagnostics, double reads of 35-byte
records, and pin/context/metadata guards. The projection rejects inconsistent
names/paths/duplicates, a nested logger or one without a direct parent in the
result, and oversized payloads. It does not repair or reassign a misplaced row.

`real_control_fifo` in source indicates ONLY type/presence, not permission,
a reader, version, live supervisor, or capability. It does not open the
write end or write to check liveness or control. A status file without a
supervisor remains historical; projection does not strengthen its guarantee
or turn it into a fresh heartbeat. There is no new I/O, worker, cache,
persistence, atomic snapshot, ABA guarantee, or hard deadline.

The view returns no control handles. Its IDs do not go to systemd, and its
PID does not go to process actions. LinuxS6Control retains pinned selection,
consent, and its separate delivery contract. LinuxS6Observation preserves raw
conditions, NOT completion, readiness, or causation. An old path does not
authorize acting on any replacement. Revisions 133–136 and the common
ServiceInfo layout are unchanged.

## Evidence and next unit

tests/linux_s6_services.cpp covers pure projection, preserved parent/logger
and ordering, maximum u64 without truncation, valid PID 0 with ready,
finishing/paused, invalid timestamps, permission/Support/partial coverage,
raw-byte IDs, logical aliases, duplicates, orphaned and nested loggers,
out-of-scope directories, and empty results. Common PID is always absent.
Private Linux tests cover a directory alias, ONE log level with log/log
ignored, an unmanaged row, bounds, a broken logger link, a FIFO receiving no
bytes and then with no reader, records without a real supervisor, a sentinel
script never run, and FD counts. MSVC tests projection, arguments, and
fallback, NOT the Linux backend. The installed SDK links the symbol.

Revision 142 validation: Linux 168/168 plus 50 s6 repetitions; 10 MSVC pure
and fallback runs; two installed SDK consumers exited 0. Logs: work/s6-services-*.
Initial /mnt/c clock-skew warnings below 1 s were retained; the final
incremental build and 168-test suite were clean without changing clocks or
mtimes. These are local tests, not remote CI or real s6 acceptance.

At this revision, K03 remains E for OpenRC/providers, integration, and
acceptance with real supervisors. The next functional unit projects the
OpenRC catalog and observations explicitly into the common model, without
treating markers or directories as liveness or executing scripts. At this
checkpoint, OpenRC revision 132 and SysV script control had no authorized
exception. The I07 observer is unchanged.
