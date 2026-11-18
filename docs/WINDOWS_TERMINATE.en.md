# Windows: forced End task

Implemented on 2026-10-13. `SystemActions::terminate_process_checked(pid, creation, true)`
uses OpenProcess, IsProcessCritical, TerminateProcess, and WaitForSingleObject.
It does not run taskkill, PowerShell, or other programs, or enable privileges
or UAC elevation.

## Contract and safety

- `force=false` returns `not_supported` without opening or terminating the
  process. There is no universal cooperative shutdown: WM_CLOSE only applies
  to windows, can prompt for confirmation, and does not guarantee process
  exit. A normal close never escalates automatically to a forced one.
- PIDs 0–4 and the caller's own process are rejected. The action requests
  only PROCESS_TERMINATE, PROCESS_QUERY_LIMITED_INFORMATION, and SYNCHRONIZE,
  with no handle inheritance. Access denial is reported without trying to
  bypass it.
- IsProcessCritical is resolved from the already loaded kernel32. If it is
  missing, fails, or reports a critical process, termination is not requested.
  This check is not atomic with termination and does not protect against all
  concurrent changes in critical status. It does not identify every important
  application.
- One handle is used from opening through final verification and closed
  through RAII. Children, sessions, and process trees are not traversed.
  Terminating a shared host can affect all its services; use control_service
  for services instead of terminating their PID.
- Use the checked call with `ProcessInfo::windows_creation_time_100ns` from
  the selected sample. After OpenProcess, GetProcessTimes is queried on the
  same handle. A timestamp mismatch returns `stale_target` before waiting or
  requesting termination. A failed query or missing permission rejects the
  action. Zero is invalid; there is no unchecked PID fallback.
- `terminate_process(pid, true)` remains for compatibility **without
  protection against PID reuse before OpenProcess**. Do not use that variant
  for stale selections or unattended termination.
- Forced termination can lose data and gives no opportunity to save. The
  GUI must obtain explicit authorization and call it off the UI thread.
  The library does not replace that authorization policy.

## Identity from Monitor

```cpp
// selected is the chosen ProcessInfo; the GUI has authorized forced termination.
if (selected.windows_creation_time_100ns.value) {
    const auto result = actions->terminate_process_checked(
        selected.pid, *selected.windows_creation_time_100ns.value, true);
    // On stale_target, refresh the list and ask for a new selection.
    // Do not automatically retry against the process now occupying the PID.
}
```

The timestamp is native FILETIME (100 ns since 1601 UTC), not Unix time or
milliseconds. Keep it unrounded with the PID from the same sample and machine.
Do not use a double or persist it as authorization. An inaccessible process
has no value and retains the corresponding support status. At this historical
checkpoint, Linux and other backends returned `not_supported` for the checked
call without delegating to the older action. The PID and timestamp check does
not authenticate the program, protect against privileged manipulation, or
replace authorization.

ProcessInfo gained a field and SystemActions gained a virtual method with a
safe default implementation. Rebuild the library and consumers together:
there is no stable 0.x ABI. Existing subclasses need not implement the new
method but report it unsupported until they do so explicitly.

## Results

After identity verification, an already signaled handle returns success
without calling TerminateProcess. A concurrent exit during TerminateProcess
is confirmed on the same handle. Otherwise the action requests termination
with exit code 1 and waits up to 5,000 ms. Only a signaled handle confirms
success.

A timeout or wait error returns `system_error` with an unconfirmed-outcome
message; it **does not cancel** the request already sent. Permission errors
and missing PIDs are distinguished as `permission_denied` and `not_found`.
Failure to open a process that already disappeared is not reported as
idempotent success. The wait does not bound the duration of other native calls.

## Verification

Revision 37 update, 2026-10-19: the checked action terminated a private child
of the harness. A stale token was rejected without terminating it, and the
same signaled handle and exit code 1 were verified. All 20 repetitions passed.
See [Windows service view](WINDOWS_SERVICE_VIEW.en.md). No unrelated processes
were terminated. The evidence below describes the initial tests on 2026-10-13.

Injected tests cover operation order, requested rights, rejection of self,
reserved and critical processes, missing or failed criticality APIs, missing
processes, concurrent exit, termination failure, timeout, unexpected wait
states, and handle closure. The public API test uses only PID 0 or a zero
timestamp, rejected before OpenProcess. Identity fixtures cover matches,
changes in the high or low 32 bits, zero timestamps, GetProcessTimes errors,
and mismatches even after exit. A real read of the test process compares
Monitor data with GetProcessTimes and checks stability across samples.
Those initial tests terminated no real process.

Release: Windows 11/11 and Linux WSL 5/5. Pending at that checkpoint: disposable
process testing in a VM, verified Linux identity, GUI-specific cooperative
shutdown, a UAC helper, and validation on more compilers and machines.

Sources: [TerminateProcess](https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-terminateprocess),
[IsProcessCritical](https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-isprocesscritical),
[OpenProcess](https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-openprocess),
and [GetProcessTimes](https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-getprocesstimes).
