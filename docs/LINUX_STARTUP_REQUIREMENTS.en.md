# N03: Linux startup apps for the GUI

**Language:** [Español](LINUX_STARTUP_REQUIREMENTS.md) · English

Revision 163, 2026-11-06. Maps R15 and Run new task to I01–I07/L01.
Keep configuration, eligibility in the observed context, and observed impact
separate. An enabled entry may not be eligible; an eligible entry may not
launch. Historical impact does not prove the current login launched it.

## Inventory and fields

Read through `Monitor::sample` or `Sampler` with
`include_startup_entries=true`. XDG inventory preserves the effective file
by name and precedence. A personal copy that masks the system copy does
not authorize falling back to the system copy. The inventory uses the
caller's context and does not cover other users or all startup mechanisms.
The vector has no completeness Metric; an empty vector does not prove
there is no autostart. [Precedence](XDG_PRECEDENCE.en.md),
[tests](../tests/linux_startup_inventory.cpp).

| Data | API / unit | Scope and evidence |
| --- | --- | --- |
| Name / ID / source | `StartupEntry.name/id/source`, text | Inventory label and ID of the effective `.desktop` source. Do not use a commercial name as a key or treat an old ID as authorization. `command` is metadata, not `argv`. [Precedence](XDG_PRECEDENCE.en.md). |
| Enabled / disabled | `status`, enum | Configuration flags; `unknown` is not disabled. This does not show running processes or certify desktop behavior. [Flags](LINUX_STARTUP.en.md), [tests](../tests/linux_startup.cpp). |
| Eligibility | `xdg_standard_eligible`, bool Metric | Hidden, Type, OnlyShowIn/NotShowIn, and TryExec in the caller's context, among the standard checks. It does not execute TryExec or Exec. Distinguish known false from absence. [Contract](XDG_ELIGIBILITY.en.md), [tests](../tests/linux_startup_eligibility.cpp). |
| Additional condition | `autostart_condition_satisfied`, bool Metric | Separate predicate, not proof that the manager supports it or launched the entry. Missing key means true; unsupported syntax or source means unknown. [File](XDG_FILE_CONDITION.en.md), [GSettings](XDG_GSETTINGS.en.md), [session](XDG_SESSION_CONDITION.en.md), [tests](../tests/linux_startup_condition.cpp). |
| Publisher | `publisher_label`, text | AppStream-declared publisher or developer, not a verified signer. Opt in with `include_linux_startup_publisher` alongside inventory; local metadata and libxml2 are optional. Do not substitute UID or the legacy `publisher`. [Contract](LINUX_STARTUP_PUBLISHER.en.md), [tests](../tests/linux_startup_publisher.cpp). |
| Startup impact | `observed_startup_impact`, category; `impact_scope`, text | Show support and scope. The `impact` shorthand does not replace the Metric. Not measured is not none; measured none does not prove zero impact across the login. [Measurement](STARTUP_IMPACT.en.md), [tests](../tests/startup_impact.cpp). |
| Impact resources | `observed_startup_cpu_time` in ns, `observed_startup_disk_bytes` in B, `observed_startup_interval` as a monotonic duration | Deltas for an observed window. CPU is not a percentage, disk is not physical throughput, and the interval is not a date. A historical overlay may leave these absent. Do not add groups to manufacture a total. [Overlay](STARTUP_OVERLAY.en.md), [historical overlay](STARTUP_BIRTH_OVERLAY.en.md), [tests](../tests/startup_birth_overlay.cpp). |

The library remains an observer. Explicit measurement and overlay APIs
require accepted associations and contexts. Do not infer launch causation
from name, PID, path, proximity to login, or a matching historical ID.
Automatic early capture without verifiable causation remains open in I07.
Neither the example nor reading inventory installs or activates a launcher
or recorder. [I07 decision](STARTUP_EARLY_DECISION.en.md).

## Actions and Properties

| Action | API | What to tell the user |
| --- | --- | --- |
| Properties | `get_startup_properties(entry.id)` | Queries only the effective source, not the executable or an OS dialog. `source_file_size_bytes` measures the `.desktop` file; `fields` preserves order, duplicates, groups, and raw values. Do not interpret the values as shell or log them by default. On `stale_target`, reload inventory rather than selecting another target. [Contract](XDG_PROPERTIES.en.md), [tests](../tests/linux_startup_properties.cpp). |
| Enable / Disable for the current user | `set_startup_enabled_for_current_user(id, enabled)` | An explicit action that creates or edits the personal override, not the system source. An existing override keeps its customizations. Reload inventory because the effective ID may change. This action does not itself launch, stop, or make the application eligible. [Contract](XDG_OVERRIDE.en.md), [tests](../tests/linux_startup_override.cpp). |
| Direct edit | `set_startup_enabled(path, enabled)` | Edits exactly the named file. With permission to edit a system file, it can affect several users. Do not use it as an automatic fallback for the personal toggle. [Contract](LINUX_STARTUP.en.md). |
| Reset to system | `reset_startup_for_current_user(personal_id)` | Explicit action distinct from enabling. Requires a valid system counterpart, withdraws the personal copy from future logins, and preserves a backup in `startup_backup_path`. It does not reset a personal-only entry or force enabled. [Contract](XDG_RESET.en.md), [tests](../tests/linux_startup_reset.cpp). |
| Run new task | `launch_process(LaunchRequest)` | Separate, confirmed action. Do not pass `command` or `raw_value` directly as shell input or `argv`. A manual launch does not prove an autostart origin. [Contract](LINUX_LAUNCH.en.md). |

For mutations, an error after the change does not always mean it was not
applied. Respect diagnostics and backups, reload inventory, and do not
retry blindly. Reading or rendering these rows does not perform actions.

## SDK example

[linux_startup.cpp](../examples/linux_startup.cpp) uses the public SDK.
Without arguments it shows help. `--sample` reads Linux inventory without
enabling optional publisher lookup or measurement. `--properties ID` reads
an effective ID and prints keys and groups, without raw values or the command.
`--self-test` covers unknown, enabled but ineligible, known none impact
and zero CPU, legacy size not promoted, duplicate keys, and redaction.
The [external consumer](../tests/startup_consumer/CMakeLists.txt) uses the
installed SDK. Its use of CMake does not require CMake in the GUI.
No ABI or production-library changes.

Verification: Linux Release passed 196/196 tests. Both example tests were
repeated ten times, ten portable MSVC runs passed, and 29 local links were
checked. The installed SDK consumer exited 0 in self-test and sample modes.
Logs: `work/startup-contract-*`. This does not validate a new graphical
login or close I07.
