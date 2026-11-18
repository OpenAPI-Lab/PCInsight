# Windows: EcoQoS efficiency control

Implemented on 2026-10-13 with GetProcessInformation and SetProcessInformation,
resolved from the already loaded kernel32. No external utilities are required.

## Reading the policy

ProcessInfo reports `efficiency_mode_available` when the power policy API can
be queried and `efficiency_mode_requested` when the EXECUTION_SPEED bit is
explicitly controlled and enabled. Read availability does not guarantee write
permission. Missing APIs, an unsupported information class, failures, or an
unknown structure leave the metrics absent. Access denial is reported as
permission_required.

The requested field represents the explicit request. A Windows-managed policy
can apply QoS according to its own heuristics even when the field is false.
The provider does not measure savings, actual frequency, or core selection.
Windows versions that cannot query ProcessPowerThrottling may leave the read
absent even when SetProcessInformation accepts the change.

## Action

```cpp
// With explicit user authorization and the selected sample:
if (selected.windows_creation_time_100ns.value) {
    auto result = actions->set_efficiency_mode_checked(
        selected.pid, *selected.windows_creation_time_100ns.value, true);
}
```

The checked variant verifies PID and creation time on the same handle used
to change the policy. An identity mismatch returns stale_target. A zero
creation time is rejected, and there is no fallback. The older
set_efficiency_mode(pid,enabled) retains the risk that a PID is reused before
it is opened. Linux and other backends do not yet implement the checked variant.

The action requests PROCESS_SET_INFORMATION, PROCESS_QUERY_LIMITED_INFORMATION,
and SYNCHRONIZE without enabling privileges. It rejects PIDs 0–4, the caller's
own process, exited processes, and critical processes. If IsProcessCritical
is absent or its query fails, the modification is rejected. These checks are
sequential: they do not replace authorization or rule out every concurrent
process change.

Enabling sets ControlMask=EXECUTION_SPEED and StateMask=EXECUTION_SPEED.
Disabling retains that explicit control and sets StateMask=0. It **does not
return the mechanism to automatic management or restore the earlier policy**.
Only the speed mechanism is selected. Timer policies and the CPU priority
class are unchanged.

Success means Windows accepted the policy, not that its effect or persistence
was verified. EcoQoS is the energy component. Task Manager's combined mode can
also include CPU priority, which this action does not implement. The library
does not promise exact equivalence with that GUI. It does not manage process
trees, thread QoS, restarts, or persistence.

## Verification

Revision 37 update, 2026-10-19: native checked EcoQoS on/off passed on a
private child of the test harness. The requested bit was read back after
each change, and a stale token was rejected; 20/20 repetitions passed. The
child was terminated and cleaned up afterward. No parent or unrelated
process policies were changed. See [Windows service view](WINDOWS_SERVICE_VIEW.en.md).
This does not establish actual savings or implement combined priority.
The following is the original evidence from 2026-10-13.

Injected fixtures cover on/off masks, handle rights, identity, missing API,
critical or exited processes, query and modification errors, and handle closure.
The only additional native query reads the caller's own process; that initial
test changed no real process policy. Windows Release passed 14/14 and Linux
WSL passed 6/6. Rebuild consumers because ProcessInfo and the virtual interface
were extended.

Pending: combined CPU priority with defined restoration, automatic policy if
exposed as a separate action, integration testing in a VM, and more Windows
versions, hardware, and compilers. The broader process subtask remains in progress.

References: [SetProcessInformation](https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-setprocessinformation),
[GetProcessInformation](https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-getprocessinformation),
and [Quality of Service](https://learn.microsoft.com/en-us/windows/win32/procthread/quality-of-service).
