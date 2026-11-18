# Live systemd service inventory

K04 update: [Linux user services](LINUX_USER_SERVICES.en.md) adds a separate API
for the effective user's manager, opening details, and explicit control.
include_services retains system scope. The K04 exclusions below describe
the earlier historical unit.

K01 update: [Linux service metadata](LINUX_SERVICE_METADATA.en.md) adds per-field
availability, a unique D-Bus owner, Unit.Id verification, and Slice as native
grouping. It replaces group=systemd and adds an inventory source. The
2026-10-14 evidence below predates that update.

Completed and verified on 2026-10-14: a read-only systemd inventory with
names, descriptions, states, and MainPID. Enable it with
`SampleOptions::include_services=true`. It does not change, start, or stop
services.

```cpp
pcinsight::SampleOptions options;
options.include_services = true;
auto snapshot = monitor.sample(options);
for (const auto& service : snapshot.services) {
    // service.status: running, active/exited, inactive/dead, failed/failed, ...
    // service.pid: Nonzero MainPID, if it could be queried.
    // service.active_state/sub_state/load_state/unit_file_state: Raw metadata.
}
```

## Sources and meaning

The provider loads `libsystemd.so.0` and uses sd-bus over D-Bus. It does not
require an SDK to build or run systemctl, ps, or another application. If
systemd is not the active manager (`/run/systemd/system` is absent),
libsystemd is unavailable, or the bus cannot be opened, the previous
provider's partial file inventory remains in use. This fallback does not
infer running state or a main PID from cgroups.

- `ListUnits`: currently loaded units, retaining `.service` units.
  This includes transient and generated units without installed files.
- `ListUnitFiles`: installed files, including templates, aliases, and masked
  units. Results are combined by unit name. A file not in ListUnits retains
  `status=unknown`, `load_state=not_loaded`, an empty description, and an
  absent PID. Disabled does not mean stopped. An alias not loaded under
  that name does not establish the target service's state.
- `status=running` only when ActiveState=active and SubState=running.
  Other combinations are shown without losing information, such as
  `active/exited`: a finished oneshot can remain active with no live process.
- `pid` uses `Service.MainPID`, not the first process in the cgroup.
  Zero, an inaccessible property, or a vanished unit yields absence.
- Since K01, group reflects a known Service.Slice or is empty. The provider
  is separate in linux_details.provider; Slice is not equivalent to a
  Windows or Unix group. Names and descriptions are those reported by the
  manager.

ServiceInfo exposes four new strings: load_state, active_state, sub_state,
and unit_file_state. Other providers leave them empty when unavailable.
This change requires rebuilding consumers of the 0.x static library;
binary compatibility with older structs is not promised.

## Errors and limits

Both enumerations must finish before publishing the inventory. A D-Bus or
parsing failure after the bus opens throws an exception rather than reporting
a partial inventory as complete. Sampler records it in last_error. Since K01,
individual MainPID/Slice failures preserve per-field availability. A known
MainPID of zero remains a typed value, never a valid legacy PID.

Limits are 10,000 entries per enumeration, strings up to 64 KiB, and a
5-second timeout per D-Bus call. This is not a global timeout: many PID
queries can extend sampling. Interfaces and messages are released through
RAII. The caller's permissions apply, with no elevation or policy changes.
The snapshot is not atomic; state, PID, and files can change between queries.

## Completion evidence

- Fixtures cover running, oneshot active/exited, inactive, failed, activating,
  templates, aliases, masked, zero/inaccessible PID, duplicates, contradictions,
  invalid paths/strings, bounds, and enumeration failure.
- Real read-only Ubuntu WSL integration: 199 services, 12 running, 16
  active/exited, and 82 unloaded files. No service was created or modified.
- include_services false/true/false was tested through Monitor.
- Release: Linux 10/10 and Windows 22/22. Windows tests portable logic, not
  native access to Linux D-Bus.
- Packages were installed, and an external CMake consumer was rebuilt and run
  on Linux and Windows with the new ServiceInfo. Old headers and objects were
  not mixed with the new ones.

This closes **systemd inventory and state**, not all services across all
distributions. Start/stop/restart control, authorization, alternative init
managers, and sessions/services of the user's systemd manager are outside
this unit. It does not complete the combined services/startup task.

Primary references: [systemd D-Bus API](https://wiki.freedesktop.org/www/Software/systemd/dbus/),
[sd-bus declarations](https://github.com/systemd/systemd/blob/main/src/systemd/sd-bus.h),
[ListUnitFiles in systemd](https://cgit.freedesktop.org/systemd/systemd/tree/src/systemctl/systemctl.c).
