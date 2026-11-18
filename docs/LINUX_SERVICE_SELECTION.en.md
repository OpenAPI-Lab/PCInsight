# Explicit service selection from Monitor

Revision 144/K03. `SampleOptions.linux_service_selection` also requires
`include_services=true`. Linux only; Windows ignores this selection. No new
actions, init autodetection, installation, CLI, shell, or scripts.

```cpp
#include <pcinsight/monitor.hpp>

pcinsight::Monitor monitor;
pcinsight::SampleOptions options;
options.include_services = true;
options.linux_service_selection.provider = pcinsight::LinuxServiceProvider::s6;
options.linux_service_selection.root = "/service"; // Chosen by the host.
auto snapshot = monitor.sample(options);
if (pcinsight::metric_is_known(snapshot.linux_service_inventory_source)) {
    // snapshot.services uses ServiceInfo and the Metrics/notes in linux_details.
    // An available inventory does not prove each row is live or controllable.
}
```

Paths are explicit, not universal locations. Selection is disabled when
include_services=false, even if selection options are set or invalid.

## Providers and selection arguments

| Provider | Selection arguments used | Source |
| --- | --- | --- |
| legacy_default | None | Previous behavior: systemd system manager and partial file fallback |
| systemd_system | None | systemd system manager only |
| systemd_current_user | None | Current effective UID's manager only, with no system fallback |
| sysv | init_directory, runlevel_directory, max_entries, sysv_prefix_budget | SysV catalog with opt-in LSB comments |
| runit | root, max_entries | runit inventory and protocol reports |
| s6 | root, max_entries | s6 inventory, historical records, and loggers |
| openrc_configured | init_directory, runlevel_directory, max_entries | Configured candidates and references only |
| openrc_marked | root, max_entries | Runtime markers only; no catalog or scripts |

`legacy_default` is the default. It retains its prior behavior, including
the historical fallback. It does NOT silently become explicit selection or
change legacy reads to include SysV content. The other seven modes never
invoke a different provider or fall back on errors, permission issues, or
missing data.

Unused fields are ignored: they do not open other roots or trigger another
provider's validation. Used paths must be absolute, contain no NUL, and be
at most 32,768 bytes. max_entries is 1–4,096 for filesystem providers.
sysv_prefix_budget is 0–16 MiB with an 8 MiB default and applies only to
SysV; zero skips description reads. It does not affect systemd/runit/s6/OpenRC.
Each provider's native limits and guards remain unchanged; the dispatcher
does not strengthen them.

## Result and quality

Only rows from the selected provider or view are published. IDs, provider
labels, Metrics, and projection labels are preserved. The snapshot does not
retain each typed API's raw structures. For raw PIDs, flags, relationships,
or detailed diagnostics, use the corresponding explicit typed query and
preserve its scope. Properties are not fabricated to fill common columns.

`linux_service_inventory_source.value` identifies the source when the result
is known, including a known empty inventory. Its Support reflects inventory
quality, and its notes include selection, scope, and absence of fallback.
Errors, denied permission, exceptions, and invalid arguments produce no rows
or source value, with an explicit reason and no reuse of the last result.
Partial results yield partial rows and estimated Support, not full coverage.
Disabled yields no rows or value, with a disabled note. No new cache or
persistence.

OpenRC selects ONE view: configured does not query runtime, and marked
does not query init.d/runlevels. Markers and candidates are not joined by
basename, and an unrequested source's quality does not downgrade a complete
one. The views stay distinct. To observe both independently, use the
[revision 143 typed API](LINUX_OPENRC_SERVICES.en.md).

runit's reported-*, s6's record-*, and OpenRC's marked-* labels do not
mean confirmed running state, a live process, readiness, or control.
The common PID remains unknown where identity is unproven; raw PIDs are
not turned into process targets. Selecting systemd does not change its
property, manager, or reported PID contract.

## Actions, compatibility, and limits

Selecting a provider for Monitor does NOT select an action provider.
Do not pass IDs or filenames from these views to systemd or process actions.
Each control API keeps its own pinned selection, authorization, and contract.
This unit adds no capabilities, runs no scripts, and confirms no jobs.
The OpenRC exclusion in revision 144 is historical: in revision 148 the
user authorized explicit control only, and revision 159 added
[the separate API](LINUX_OPENRC_CONTROL_PUBLIC.en.md). Read selection stays
unchanged; SysV still has no exception, and the I07 observer is unchanged.

A field is added at the end of SampleOptions, with new public types in
types.hpp. **Rebuild ALL 0.x clients, bindings, and plugins** against matching
headers and library. Do not mix binaries built with older sizes. The
ServiceInfo/SystemSnapshot layouts and history formats are unchanged. This
is the public C++ contract; the GUI need not configure CMake to use it now.

There is no atomicity across samples, permanent path identity, ABA guarantee,
or hard I/O deadline. The host coordinates namespace and credential changes
and forks, and updates its explicit selection. This option loads no new
providers by default.

## Evidence and next step

tests/linux_service_selection.cpp covers seven routes with a private per-call
callback, exactly one provider invoked, used and ignored fields, invalid
enums/roots/bounds/budgets without queries, error/permission/exception
without fallback, and known empty or partial results without old values.
A real Linux Monitor test uses private SysV/runit/s6/OpenRC configured and
marked trees, changes provider, checks failures and switches without stale
data, verifies the disabled flag and that unrelated runtime is not queried,
and checks a sentinel and FD leaks. MSVC covers only the pure dispatcher,
NOT Windows Monitor or the Linux backend. Two installed SDK consumers were
rebuilt; the new one uses selection in Linux Monitor.

Revision 144 validation: Linux 170/170 plus ten selector repetitions,
ten pure MSVC dispatcher runs, and two installed SDK consumers rebuilt,
relinked, and exiting 0, including the new Linux Monitor sample. Initial
/mnt/c clock-skew warnings were retained; the final incremental build and
170-test suite were clean without changing clocks or mtimes. Logs:
work/service-selection-*. This does not verify Windows Monitor, an N03 fix,
remote CI, or acceptance with real init managers.

K03 remains E for real control and acceptance and the remaining scope. Next:
audit K03 by provider against actual evidence and blockers, separate completed
work from deferred or unauthorized work, and continue L05/Linux when appropriate.
Do not claim universal coverage or prolong tests that require unavailable
hardware.
