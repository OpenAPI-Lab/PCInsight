# systemd service metadata and grouping (K01)

K04 update: [Linux user services](LINUX_USER_SERVICES.en.md) implements the
separate API for user services, opening details, and control. The K04 exclusions
in the historical evidence below do not describe that unit's current state.

The systemd system manager provides the name, description, state, MainPID,
Slice grouping, and installation state, with explicit availability.
The provider does not run systemctl/ps or install dependencies. No service
is started, stopped, or restarted for enumeration or validation of this
unit. K02 control, K03 alternative init systems, and K04 user managers and
opening service details remain separate.

## Public contract

ServiceInfo.linux_details is Metric<LinuxServiceDetails>. provider identifies
the source; group_kind defines the grouping semantics, not the group value.
Check each Metric: a present structure does not mean every field is known.

| Field | Source and meaning |
|---|---|
| name/description | Primary name and description reported by ListUnits; an empty description can be known |
| load_state/active_state/sub_state | Native ListUnits strings, not installation state |
| status | Estimated rendering of ActiveState/SubState: active+running gives running; otherwise active/sub |
| main_pid | Service.MainPID property; known zero means no main PID was reported |
| slice | Service.Slice: native resource-control grouping |
| unit_file_state | ListUnitFiles: installation policy, not running/stopped |

ServiceInfo.group reflects the slice or is empty; it no longer uses systemd
to identify the provider. group_kind=systemd-slice and provider=systemd-system-manager
are separate. A Slice is NOT a Windows load-order group, svchost group, or
the Unix credential group Service.Group. The GUI should label it “systemd
Slice” without assuming cross-platform equivalence. No default system.slice
is invented if the property is unavailable or invalid. A Slice property read
as an empty string stays known empty (no slice name reported), distinct from
an inaccessible query.

The optional legacy PID is published only when MainPID is available and positive
within Linux's signed 32-bit range. main_pid=0 remains known in the typed field
without being presented as a valid process. An inaccessible query or out-of-range
value remains unknown, not zero. MainPID is not the first process in a cgroup
or historical ExecMainPID.
[Manager API and MainPID](https://github.com/systemd/systemd/blob/main/man/org.freedesktop.systemd1.xml),
[native Slice property](https://github.com/systemd/systemd/blob/main/src/core/dbus-unit.c),
[publication through the Service interface](https://github.com/systemd/systemd/blob/main/src/core/dbus.c).

active/exited can mean an active oneshot without a main process.
Enabled/disabled/masked/alias does not prove execution. Future states are
retained as valid ASCII tokens up to 128 B, not arbitrarily translated.
An invalid or absent state leaves its Metric without a value. Legacy status
is unknown unless both ActiveState and SubState are available.

## Identity and coverage

The provider obtains systemd's unique D-Bus owner and directs both enumerations
and every property query to that instance. It verifies the owner again at the
end. Enumeration, parsing, or limit errors and owner changes prevent the list
from being reported as successful. There is no silent fallback after such a
failure.

Unit.Id must match the primary name before and after MainPID/Slice. If it
cannot be verified, PID and Slice remain unknown, preserving the permission
note where applicable. If the first Id check fails, those properties are not
requested. Earlier ListUnits metadata is retained as what the enumeration
reported, not as certified current state after the failure.

Id does not identify a service invocation or PID generation. A restart or
replacement that preserves the name between queries may go undetected. There
is no atomic snapshot or guarantee that the PID is still alive. These data
are not a process termination token or authorization to act. K02 retains its
separate contract for name-based control and job confirmation.

Installed files absent from ListUnits retain unknown runtime state.
load_state=not_loaded is an estimated label for that view, NOT a native
LoadState value or proof the unit is stopped. Aliases and templates are not
deliberately resolved or loaded to obtain PID/Slice. LoadUnit and StartUnit
are not called. Queries against enumerated objects may interact with internal
manager caches; absence of all internal daemon effects is not promised.

SystemSnapshot.linux_service_inventory_source distinguishes:

- systemd-system-manager/available: ListUnits and ListUnitFiles completed
  against the same owner. It can be a valid empty inventory, not coverage
  of all init systems or managers.
- partial-file-scan/estimated: the earlier filename fallback, with possibly
  partial coverage or errors, not runtime state.
- No value: collection disabled or non-Linux backend; see the note.

In fallback, each row's provider is systemd-file-scan or sysv-file-scan.
Group and effective description stay empty with unknown typed availability.
The simplistic Description= line parser was removed; it did not resolve
sections, precedence, aliases, or effective overrides. Finding a file does
not prove that systemd/OpenRC/SysV manages or runs it. This does not
implement K03.

## Validation, errors, and compatibility

.service names use the project's shared canonical validator: up to 255 B,
ASCII and hexadecimal escapes. Object paths are under
/org/freedesktop/systemd1/unit/ with one valid component and no self.
Duplicate loaded entries, repeated paths, and contradictory file states
are rejected, even if one state was empty. Slice is validated as a bounded
.slice name: paths, @ instances, NUL, and incomplete escapes are rejected.
General strings are limited to 64 KiB without NUL, with 10,000 rows per
enumeration and 16 MiB of combined payload. The native reader also limits
unpublished ListUnits fields. This is not an exact process-memory or
internal sd-bus buffer limit; an injected callback may already have created
its vector.

Observed EACCES/EPERM/AccessDenied/InteractiveAuthorizationRequired errors
are preserved per field as permission_required; permission failure is not
guessed from absence. Interactive authorization is disabled, with no elevation.
RAII releases messages, errors, bus, and library. The dynamic ABI was checked
against [sd-bus.h](https://github.com/systemd/systemd/blob/main/src/systemd/sd-bus.h).
The timeout is 5 s per call, not global: four queries per loaded unit can
extend sampling. Universal latency on large or slow managers is not guaranteed.

The 0.x API changes the ServiceInfo/SystemSnapshot layouts. Rebuild the
library and ALL consumers. The fallback's legacy group/description strings
change to correct their meaning. Consumers relying on group==systemd must
migrate to provider. Windows retains its SCM fields and semantics, with
linux_details absent. The virtual action interface and history format are
unchanged.

## Tests

tests/linux_systemd.cpp covers running/oneshot/inactive/failed/activating,
installed but unloaded units, per-field metadata, a known empty description,
zero PID versus denied/absent/invalid, valid/absent/denied/malformed Slice,
invalid or changed identity before or after the query, duplicates, excess
payload, and fallback without invented groups or descriptions. Real integration
is read-only and tests Monitor with include_services false/true/false; it
creates or controls no services.

On 2026-10-17, final Release suites passed: Linux 85/85 and Windows 105/105,
with no failures in those runs. Packages were reinstalled, and both installed
consumers rebuilt, relinked, and exited 0, including access to the new fields
through installed headers. The final incremental rebuild emitted no warnings.
Real Ubuntu WSL enumeration: 199 services, 11 running, 16 active/exited, 82
not observed as loaded, 117 MainPID reports (including zero), and 117 known
Slice values (15 empty, 102 named). These are observations from that dynamic
enumeration, not permanent values or guarantees that processes remain alive.
No host services were created or controlled. Windows ran portable provider
fixtures and SCM regressions, not native Linux D-Bus. Workspace logs:
work/service-metadata-verified-suite-linux.log and
work/service-metadata-verified-suite-windows.log. Passing this suite does not
resolve the earlier intermittent CPU/history risks in N03.

K01 closes this provider and contract within explicit limits. It does not
close K03 alternative init systems, K04 user managers or opening service
details, L05 authorization, N01 CI, or N02 distribution coverage. These tests
do not add subtasks to the count.
