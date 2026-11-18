# User services and opening service details (K04)

Additional API in `pcinsight/linux_services.hpp`. It does not change existing
ServiceInfo/SystemSnapshot layouts or the virtual ABI of SystemActions.
Rebuild and relink the static library to use the new symbols.

```cpp
#include <pcinsight/linux_services.hpp>
using namespace pcinsight;
auto inventory = query_linux_service_inventory(LinuxServiceScope::current_user);
LinuxServiceTarget target{LinuxServiceScope::current_user, "example.service"};
auto view = open_linux_service(target); // Data for the caller's GUI; does not launch it.
// Only after an explicit decision by the application or user:
// auto result = control_linux_service(target, ServiceAction::restart);
```

## Target and identity

system and current_user are separate scopes even when they share a unit name
or D-Bus owner name; owner names are local to each bus. Inventory and detail
views retain scope, user_uid, and manager_owner. Do not infer scope from a PID,
name, or description. user_uid is the current **effective** UID, not a presumed
desktop user, process owner, or arbitrary UID. It is absent in system scope.
Monitor include_services and SystemActions control_service retain system scope;
this separate API does not mix rows into their snapshots.

current_user constructs only `unix:path=/run/user/<euid>/bus` using
sd_bus_new/set_address/set_bus_client/start. It does not use
DBUS_SESSION_BUS_ADDRESS, XDG_RUNTIME_DIR, sd_bus_open_user, remote transport,
shell, or autolaunch. It checks the socket's SO_PEERCRED against euid and uses
GetConnectionUnixUser to verify the unique owner of org.freedesktop.systemd1.
It pins that owner for properties, calls, and signals, and verifies ownership
again at the end. It trusts the local broker and its registry: it does not
cryptographically authenticate the systemd executable or protect against a
malicious broker or library under the same user. A nonstandard bus or broker
with another UID is reported as unavailable or denied; no alternative target
is guessed and there is no system-manager fallback.

This is an observation, not a lasting authorization token. Each new
current_user call selects the euid in effect for that call. The caller must
not change credentials concurrently or treat a view as process identity.
MainPID is not a pidfd or a checked token for other actions.

## Inventory and opening a service

This reuses the two complete enumerations and Unit.Id checks from
[K01](LINUX_SERVICE_METADATA.en.md). provider=systemd-user-manager identifies
the rows; Slice remains native grouping, not a Windows or Unix group. Known
MainPID zero and per-field availability are preserved. The API does not call
LoadUnit or start services to resolve a view.

open_linux_service validates the .service name before opening the bus and
returns a copy of details for the caller's GUI. It does not open an editor,
file, URL, or external application. Only observed names are resolved. A missing
name carries a note and does not prove it never exists. Installed, unloaded
aliases and templates remain unresolved with unknown runtime state, not stopped.

Metric distinguishes a completed empty inventory from an absent bus or manager,
permission denial, or a parsing or identity failure. There is no fallback file
scan. The free-function API returns errors as absence with a note; Monitor's
historical internal path still propagates errors after opening the bus. This
is not an atomic snapshot. Inherited limits: 10,000 entries per enumeration,
64 KiB strings, 16 MiB payload, and a 5-second timeout per call, with no global
timeout guarantee.

## Explicit control

control_linux_service(target,start/stop/restart) validates scope, name, and enum
before opening the bus. system delegates to the [K02 controller](LINUX_SERVICE_ACTIONS.en.md);
current_user opens and verifies its target and uses the same job broker with
the destination and filter pinned to the unique owner. There is no elevation,
interactive authorization, sudo, remote UID, scope fallback, or launch of a
missing manager or broker. Starting a specific unit can load it, start
dependencies, and replace jobs (`replace`). That is an explicit mutating
action, distinct from querying or opening details.

The API registers for JobRemoved before Subscribe and before requesting the
change. It waits for the returned job path, including early signals. Only
result=done is success, not proof of readiness or permanent running state.
Denial, NoSuchUnit, and job failure retain their categories. An owner change,
invalid job, disconnect, limit, or timeout after attempting the action may
leave an **unknown outcome**. The API neither cancels nor retries; a failed
restart may already have stopped the application. Limits are a 30-second wait,
5 seconds per call, 100,000 messages, 1,024 early signals, and 1,024 bytes per
field. Opening and authenticating the bus have no strict global deadline.

## Evidence (2026-10-17)

- Release: Linux 91/91 and Windows 108/108. Installed external consumers on both
  systems were rebuilt, relinked, and run to exit 0 with the new header and symbols.
- Portable checks: incompatible scopes, incomplete or duplicate identity,
  absence versus denial, invalid names and enums, and detail selection.
- Isolated Linux loader: simulated libsystemd and a local socketpair for
  credentials. Checks fixed address, UID, bus-client mode, timeout, and disabled
  interactive authorization. Deliberately unrelated environment variables are
  ignored. Tests cover empty and populated inventory, missing manager, wrong
  UID, denial, parse failure, and owner changes; start/stop/restart, early and
  late signals, job failed, NoSuchUnit, denial, invalid jobs, and disconnect.
  They verify unique destination and sender and never connect to the host's D-Bus.
- Read-only native WSL: 199 system units and 19 user units, details opened in
  both scopes, and user UID verified. Owner :1.2 on both buses does not mean
  shared identity. No real service was controlled. New positive control tests
  are simulated; historical native system evidence remains in K02.
  --native-fixture was not run for this unit.
- Logs: work/user-services-build-{linux,windows}.log,
  work/user-services-suite-{linux,windows}.log, and
  work/user-services-native-linux.log. Builds had no compiler warnings.

This completes the systemd user provider and opening service details, not K03
alternative init systems, all host users, an external service manager, the L05
helper, N01 CI, or universal N02 distribution validation. Passing this suite
does not resolve historical N03 CPU/history risks. No new IDs or fractional
credit were added.

Primary references: [sd-bus address](https://www.freedesktop.org/software/systemd/man/253/sd_bus_set_address.html),
[bus client](https://www.freedesktop.org/software/systemd/man/253/sd_bus_set_server.html),
[D-Bus and Unix identity](https://dbus.freedesktop.org/doc/dbus-specification.html),
[sd-bus declarations](https://github.com/systemd/systemd/blob/main/src/systemd/sd-bus.h).
