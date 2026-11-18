# Explicit Startup file action and auditing

Revision 47 extends reads and actions to common IDs without adding a
virtual method or operation. See [all-user impact and permissions](WINDOWS_COMMON_STARTUP_FILE_CONTROL.en.md).
The host must show that scope before requesting consent. Neither ID nor
actor authorizes an elevated helper to publish other parties' common files.

Revision 46, 2026-10-19. SystemActions adds
`set_windows_startup_file_registered(observed, registered)` and AuditedActions
wraps it. Windows delegates to the free function's [checked provider](WINDOWS_STARTUP_FILE_CONTROL.en.md).
Linux, other platforms, and custom backends without an override report
not_supported. Neither generic set_startup_enabled nor the Run/KTM action
is redirected. Constructing either object changes no files and records no requests.

```cpp
pcinsight::AuditedActions actions(pcinsight::make_system_actions(),
    [](const pcinsight::AuditEvent& event) {
        // Copy or store structured data with the guarantees the host requires.
    }, "GUI local");
auto backups = pcinsight::list_windows_startup_file_backups();
// Show names and states and obtain explicit consent BEFORE acting.
// After selecting a retained_by_pcinsight row and querying it again if needed:
// auto result = actions.set_windows_startup_file_registered(row.registration, true);
```

The empty sink in the example is NOT durable storage. The host must
replace it with a real sink and handle diagnostics(). The free function
remains available without auditing. The decorator does not capture calls
that bypass it or actions taken elsewhere. It is not authorization, an
authenticated identity, a UAC helper, automatic consent, global logging,
or a tamper-proof record. It does not change StartupApproved or effective
Windows eligibility.

## Event and policies

AuditOperation::windows_startup_file_registration is appended to the
enum, preserving earlier ordinals. requested and completed share a
wrapper-local ID. enabled=false requests moving the direct file to
backup; true requests restoration. The flag is the request, not Windows
approval or proof of a write. backend_invoked confirms only that the
backend was called, not that a rename occurred.

The target contains only entry_id, limited to 4,096 bytes with
metadata_truncated. actor retains its existing limit. The event does
not include file or folder identity, contents, diagnostics, or exception
text. **The ID contains the full hex-encoded path, which can reveal a
username or other sensitive names. It is not anonymous or redacted.**
The host controls access, retention, serialization, and sanitization.
Do not concatenate untrusted strings into log lines. Truncation may
split a UTF-8 sequence, and strings can contain arbitrary bytes.

The full observation is copied before requested is emitted. The backend
receives that untruncated copy (ID, state, folder identity, and optional
file identity) even if a callback later modifies the original object.
This neither authorizes the target nor replaces filesystem revalidation.
The caller must avoid races during copying; it is not permission to
mutate shared objects without synchronization. The change is limited
to this method. Other audited methods that keep references to caller
arguments retain their previous contract. Allocation failure before
invocation may propagate with no events and no backend call.

The [general audit policies](AUDIT.en.md) are unchanged:

- require_request_record is the default. If requested fails, the backend
  is not called; the result is system_error and completed is attempted
  with backend_invoked=false.
- best_effort is explicit opt-in: execution can proceed even if requested fails.
- If completed fails, the backend's returned result is preserved and
  failed_completed_records increases. No rollback is claimed and no retry occurs.
- On a backend exception, completed is attempted with system_error and
  backend_threw=true, and the original exception propagates. This does
  not prove an absence of effects.

The sink is synchronous and thread-safe, with no timeout. Backend calls
are serialized per instance, not across processes or wrappers. No elevation,
new log files, retries, queues, or configuration changes. The host remains
responsible for consent and for handling unknown outcomes or lost records.

## 0.x compatibility

A virtual method is appended to SystemActions, AuditedActions overrides
it, and the enum is extended. **Rebuild and relink the library and ALL
consumers and custom backends together.** The default not_supported
preserves source compatibility for existing subclasses, not binary
compatibility with old vtables. Do not mix older plugins or objects.
This component changes no AuditEvent/SystemSnapshot/SampleOptions layouts
or history formats. It does not declare a stable 1.0 ABI or close A05.

## Verification

Portable tests/windows_startup_file_audit.cpp verifies forwarding of all
fields, both directions and every ActionError, the separate operation
without generic fallback, both stages and their shared ID, limits only
on event metadata, result/message and exception preservation, requested/
completed/best-effort sink policies, and copying before a callback mutates
the caller object. The real factory receives only an invalid observation:
no actual Startup change is requested.

In tests/windows_startup_file_control.cpp on Windows, the test backend
redirects only Known Folder to a temporary fixture and the real decorator
wraps the native provider. Tests verify successful removal/restoration
with real files and correlated events, a sink rejection without calling
the provider or creating a backup, and a failed completed record after
a successful rename with the result preserved and diagnostics increased.
No real startup entries are changed and no files are executed. The
installed consumer calls the new audited virtual method only with an
invalid observation and checks both events. Logs: work/startup-file-audit-*.log.

Release: Windows 125/125 and Linux 110/110. Twenty repetitions each of
audit and control on Windows (40 runs) and twenty audit repetitions on
Linux. Both installed consumers rebuilt, relinked, and ran with exit
code 0. Windows and the final incremental Linux build had no warnings.
The initial WSL log retains Clock skew warnings for compiler_depend.make
(0.023–0.047 seconds in the future), without clock or configuration changes
or any attributed cause. No fixture residue or changes to real Startup.
This is not remote CI or validation of other filesystems or architectures.
M05 remains open for HKLM/common coverage, effective approval, and generic
actions. This auditing work does not resolve Run/KTM error 6801.
