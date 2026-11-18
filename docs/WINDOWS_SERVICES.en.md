# Windows services: inventory and explicit control

Implemented on 2026-10-13. Enable it with `SampleOptions::include_services=true`.
It defaults to off to avoid unnecessary enumeration.

The provider queries the local Service Control Manager through Win32 APIs.
It does not run `sc.exe`, PowerShell, or other tools. It requests only
connection, enumeration, and configuration-read permissions; the inventory
provider does not start, stop, restart, delete, or modify services.

## Fields and scope

- `id`: internal service name; `name`: Windows display name.
- `status`: running, stopped, paused, start_pending, stop_pending,
  continue_pending, pause_pending, or unknown.
- `pid`: present only when Windows returns a nonzero PID in a state where
  it is valid. No PID is published for stopped, starting, or stopping services.
- `description`: configuration description when accessible. Indirect
  references to language resources are kept as returned by Windows; no DLL
  is loaded to resolve them.
- `group`: **load-order** group (`lpLoadOrderGroup`), not the svchost hosting
  group. The command line is not used to infer a group.

The inventory includes active and inactive Win32 services in their own or
shared processes, not kernel drivers. Windows can omit services when the
caller lacks permission to query their status; administrative visibility is
not promised. A deleted service or inaccessible configuration leaves the
basic inventory row with empty description/group fields. Legacy inventory
does not distinguish an empty string from a string unavailable for permission
reasons. The new [open_windows_service query](WINDOWS_SERVICE_VIEW.en.md)
exposes per-field availability without changing the legacy layout.

## Resilience

Enumeration uses 256 KiB pages, deduplicated identifiers, and checks bounds
and cursor progress. A global failure throws `std::system_error` instead of
presenting a partial list as successful. Limits are 100,000 records and 1,024
pages. Configuration and description queries use 8 KiB buffers; insufficient
space or failure leaves the field unavailable. Handles close on all paths,
including exceptions. There is no cache: every sample queries state and
configuration again. Concurrent changes are not captured atomically, and
native RPC timeouts are controlled by Windows.

## Start, stop, and restart actions

`make_system_actions()->control_service(service.id, ServiceAction::restart)`
uses the internal UTF-8 name, not the display name. Actions are explicit and
separate from sampling. They run no commands and do not elevate privileges;
they use the caller's rights. The SCM connection requests CONNECT, and the
service handle requests QUERY_STATUS and START/STOP as appropriate. The GUI
must authorize each action and run it off the UI thread. The UAC helper remains
pending.

Start and stop are idempotent if the target state is already observed.
Restart starts a stopped service. The implementation waits for transitions
before acting and verifies running/stopped afterward. Acceptance does not
mean completion. Start does not resume a paused service; restart can stop
and start it. Dependents are not automatically stopped, and the host process
is not terminated. A start failure after a stop reports a partial restart.
There is no rollback, and timeout does not cancel a request Windows accepted.

Polling shares a 30-second budget per operation with 100 ms intervals.
Native RPC calls may exceed that budget. Success confirms only the observed
state, which can change immediately. ActionError distinguishes permission
denial and a missing service; other errors include the Win32 code in message.
Empty input, NUL, slashes, invalid UTF-8, names longer than 256 UTF-16 code
units, and invalid enums are rejected before opening SCM. A disabled service's
startup type is not changed.

## Tests and remaining work

Injected fixtures cover two pages, SCM and configuration permissions,
failure after the first page, invalid counts or pointers, a stalled cursor,
states/PIDs, and handle cleanup. Read-only integration observed 323 services
on the development machine. Windows Release passed 9/9 and Linux WSL 5/5.
Those tests created or changed no services.

Injected action tests cover stop/start order, minimal permissions, errors,
handle cleanup, invalid input, pauses, transitions, idempotency, active
dependents, failed startup, and the shared budget. The manual native harness
below was also run on 2026-10-19 with explicit authorization and UAC
confirmation. No existing services were touched.

Revision 37 completes the dedicated detail query with typed availability
for description and group: see [Windows service details](WINDOWS_SERVICE_VIEW.en.md).
Together with native actions on a private child, this closes M07 within its
documented scope. The production UAC helper remains L05. Indirect descriptions,
svchost hosting groups, combined efficiency mode, and coverage of more
versions or machines are not claimed.

## Limited manual native validation (2026-10-19)

The `pcinsight-service-native-test` target is NOT registered with CTest.
Without `--run-private-service-test` it returns 2 without making changes.
Without privileges it returns 1 and records Win32=5 before creating anything.
It does not elevate itself. Running it requires explicit operator authorization
and elevation.

It uses a unique `PCInsight_Private_SCM_<pid>_<tick>` name, exclusive
CreateService, manual startup, an own-process service, the LocalService
account, and no dependencies. It changes no existing services or policies.
It copies the static MinGW executable into an exclusive ProgramData directory
with a protected DACL: SYSTEM/Administrators have full control and LocalService
has read/execute access. It does not open up the user profile or run the service
as LocalSystem. Both executables are pinned against ordinary replacement
during the test. This harness is not a production helper or a security boundary
against other administrators.

The private service accepts only stop/interrogate, uses no network, and shuts
down within 60 seconds if it loses the controller. The controller tests the
public API: stop while stopped, start, idempotent start preserving PID, restart
while running, stop, restart while stopped, and final stop. Each phase checks
SCM and the library's real inventory: internal name, state, valid/absent PID,
description, and empty load-order group. Restart need not produce a different
PID because Windows may reuse it. Pauses, dependents, and failures remain
fixture cases and are not induced in unrelated real services.

Cleanup uses only the handle from ITS successful CreateService: wait for stop,
call DeleteService, close the handle, and verify ERROR_SERVICE_DOES_NOT_EXIST.
It removes its copy and directory non-recursively. Incomplete cleanup produces
RESULT FAIL with a diagnostic. Power loss or forced controller termination may
leave registration or files for administrative cleanup. The watchdog bounds
the live process; it does not guarantee cleanup after a controller crash.

Native result: service `PCInsight_Private_SCM_18672_36777171`, all phases
PASS, registration/copy/directory removed, RESULT PASS. An independent query
confirmed the registration and directory were absent. Evidence:
`work/build-release/pcinsight-service-native-test.exe.result_18672_36777171.txt`.
The first elevated attempt from the user profile failed at StartService with
Win32=5. Registration was removed and the failure retained in
result_2664_36728984.txt. The restricted ACL on the copy resolved startup
without modifying existing ACLs.

Regression: Windows Release 119/119 and Linux 108/108. Only the harness, CMake,
and documentation changed in that unit; no production code, API/ABI/layout,
or history changes required consumer migration. Revision 36 did not close all
of M07, L05, remote CI, or coverage of other versions, VMs, or hardware.

Sources: [SCM enumeration](https://learn.microsoft.com/en-us/windows/win32/api/winsvc/nf-winsvc-enumservicesstatusexw),
[configuration](https://learn.microsoft.com/en-us/windows/win32/api/winsvc/nf-winsvc-queryserviceconfigw),
and [description](https://learn.microsoft.com/en-us/windows/win32/api/winsvc/nf-winsvc-queryserviceconfig2w).
Control: [StartServiceW](https://learn.microsoft.com/en-us/windows/win32/api/winsvc/nf-winsvc-startservicew),
[stopping services](https://learn.microsoft.com/en-us/windows/win32/services/stopping-a-service),
and [querying state](https://learn.microsoft.com/en-us/windows/win32/api/winsvc/nf-winsvc-queryservicestatusex).
