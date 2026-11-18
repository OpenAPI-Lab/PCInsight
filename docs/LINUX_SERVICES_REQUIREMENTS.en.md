# N03: Linux services for the GUI

**Language:** [Español](LINUX_SERVICES_REQUIREMENTS.md) · English

Revision 165, 2026-11-06. Maps R17 and service actions to K01–K04 and L05.
**Inventory, service liveness, and control capability are separate.**
The selected read source does not enable or select a control API.

## Reading and presentation

`Monitor::sample` with `include_services=true` returns `services` and
`linux_service_inventory_source`. Check value, support, and note before
showing rows. A known empty list is not an unavailable source. A partial
inventory does not establish coverage. `legacy_default` retains its historical
file fallback; explicit modes do not switch providers on failure.
There is no universal init detection. [Selection](LINUX_SERVICE_SELECTION.en.md),
[tests](../tests/linux_service_selection.cpp).

| Field | API | Interpretation |
| --- | --- | --- |
| Name | `ServiceInfo.id`, `linux_details.name` | A label and key within the observed scope, not authorization. With systemd, use the canonical `.service` name and explicit scope. Do not pass a filename, PID, or ID from another provider. |
| PID | `linux_details.main_pid` | MainPID reported by systemd. A known `0` means no main PID is reported; absence means unknown. Even a positive value is not a pidfd or a process-termination token. Do not promote a raw supervisor PID to a verified PID. |
| Description | `linux_details.description` | Provider-reported description or metadata. A known empty string is valid. Do not substitute the name for missing data or claim to authenticate a commercial source. |
| Status | `linux_details.status`, `load_state`, `active_state`, `sub_state`, `unit_file_state` | Native state or provider representation. Installation state is not runtime state. A candidate, record, or marker can exist without a live process. Do not normalize all states to running or stopped. |
| Group | `linux_details.group_kind` and `slice` | In systemd this is Slice grouping, not Windows load-order grouping or a Unix credential group. A known empty group is distinct from an unknown one. The `provider` field is separate. |

Check the outer `linux_details` Metric first, then each field. Avoid
inferences from legacy strings or PIDs. [systemd metadata](LINUX_SERVICE_METADATA.en.md),
[tests](../tests/linux_systemd.cpp).

| Explicit provider | What it does and does not report |
| --- | --- |
| `systemd_system` / `systemd_current_user` | Separate managers; the latter belongs to the caller's effective UID. Do not join by unit name or use ownership as lasting authorization. [API](../include/pcinsight/linux_services.hpp), [scopes](LINUX_USER_SERVICES.en.md). |
| `sysv` | Catalog and, within the collection budget, LSB comments as metadata. No scripts are executed; a file or symlink does not establish liveness. [Projection](LINUX_SYSV_SERVICES.en.md), [tests](../tests/linux_sysv_services.cpp). |
| `runit` / `s6` | Supervisor reports or records with their scopes, not proof of current liveness or readiness. [runit](LINUX_RUNIT_SERVICES.en.md), [s6](LINUX_S6_SERVICES.en.md), [runit tests](../tests/linux_runit_services.cpp), [s6 tests](../tests/linux_s6_services.cpp). |
| `openrc_configured` / `openrc_marked` | Two independent views: configured candidates and references versus runtime markers. Do not join by basename. `marked-started` does not prove liveness, PID, or permission to control. [Projection](LINUX_OPENRC_SERVICES.en.md), [tests](../tests/linux_openrc_services.cpp). |

The host explicitly chooses filesystem roots; there are no universal paths.
Use each provider's typed queries when you need raw records, relationships,
or diagnostics. Do not force them into the systemd model.

## Separate actions

| Action | Contract |
| --- | --- |
| Open service | `open_linux_service(LinuxServiceTarget)` queries details for an observed name in its systemd scope. It does not open applications, load units by name, or guess aliases. The GUI displays the view. [Contract](LINUX_USER_SERVICES.en.md), [tests](../tests/linux_services.cpp). |
| Start / Stop / Restart systemd | `control_linux_service(target, action)` uses the explicit manager; `SystemActions::control_service` retains system scope. An explicit request and native permissions are required, with no elevation or fallback. Success means that request's job returned `done`, not readiness or permanent state. A timeout does not cancel it. [Contract](LINUX_SERVICE_ACTIONS.en.md). |
| Control runit / s6 | Select the adapter explicitly (`LinuxRunitService` or `LinuxS6Control`) and use `request(action, authorization)`. Delivery to the channel is not service success. Later observation retains its own limits and does not prove causation. [runit API](../include/pcinsight/linux_runit.hpp), [s6 API](../include/pcinsight/linux_s6.hpp). |
| Control OpenRC | `LinuxOpenrcService::open(config, name)` pins the selection; `request` requires separate consent. This is experimental and opt-in. Keep the runner until collected, even after an uncertain error. The runner result is not the service state. `close` revokes future requests, not a runner already dispatched. [Contract](LINUX_OPENRC_CONTROL_PUBLIC.en.md), [tests](../tests/linux_openrc_control_public.cpp). |
| Control SysV | Not implemented or authorized under the OpenRC exception. Do not run a script from the catalog as a fallback. |
| Run new task | If the GUI offers this action, `launch_process(LaunchRequest)` is an independent launch. Do not pass commands derived from metadata or present it as service control. [Contract](LINUX_LAUNCH.en.md). |

Do not infer authorization from a prior read. An uncertain reply does not
permit blindly retrying, canceling, or assuming nothing happened.
Acceptance on real alternative runtimes is still open in K03, and deployment
of the L05 privileged helper remains deferred. [K03 audit](LINUX_K03_AUDIT.en.md).

## SDK example

[linux_services.cpp](../examples/linux_services.cpp) queries the system
systemd manager without fallback in `--sample` mode; `--details UNIT.service`
queries metadata in the same scope. Neither calls control APIs.
`--self-test` covers unavailable sources without displaying rows, a known
empty inventory, MainPID 0, known empty strings, separate installation
state, and markers that are not promoted to PIDs. The
[external consumer](../tests/services_consumer/CMakeLists.txt) validates the
installed SDK without imposing a GUI build. No ABI or library changes.

Verification: Linux Release passed 200/200 tests. Both example tests were
repeated ten times, and ten portable MSVC runs passed. Twenty-five local
links were checked. The installed SDK consumer passed self-test and sample
modes; the latter observed 199 system systemd rows on WSL without fallback.
That observation neither guarantees universal coverage nor validates real
control. Logs: `work/services-contract-*`.
