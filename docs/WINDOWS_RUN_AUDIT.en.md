# Auditing explicit Run control

Revision 48: the method also recognizes [HKLM32/64 by ID](WINDOWS_MACHINE_RUN_CONTROL.en.md).
No virtual method or operation is added; the target retains hive and view.
The observation is now copied before calling the sink so its callback
cannot change the pending request. This does not permit concurrent
mutation during the copy or establish authorization. KTM error 6801 remains
unresolved; successful native reads are not successful mutations.

`SystemActions::set_windows_run_registration_enabled(expected, enabled)`
is the explicit action for [PCInsight's Run mechanism](WINDOWS_RUN_CONTROL.en.md),
separate from generic set_startup_enabled and from StartupApproved.
Windows delegates to the same transacted provider as the free function in
startup.hpp; Linux and custom backends without an override return
not_supported. Adding the method neither authorizes nor changes an entry
when the backend is constructed.

`AuditedActions` now wraps this method. A consumer can use the decorator
to record attempts and results through its own sink. The free function
remains available and does not automatically generate audit records.
This is not global system monitoring or a requirement imposed on apps
that bypass the decorator.

## Contract and privacy

- AuditOperation::windows_run_registration is distinct from startup and
  is appended to the enum to preserve earlier ordinal values.
- requested and completed share a decorator-local ID. enabled=false
  requests moving the registration to backup and true requests restoration;
  neither means Windows approval.
- The target copies only entry_id, with the existing 4,096-byte limit and
  metadata_truncated. command, native_data, and native_type are not copied,
  nor are backend diagnostics or exceptions. ID and actor remain caller
  data and can be sensitive: the ID includes a hex-encoded value name, not
  an anonymized name. The library cannot detect secrets placed in ID or actor.
- The full request, with untruncated ID/type/bytes/command/state, goes to
  the explicit backend method. It never falls back to generic enable/disable
  or a call without the expected observation. The decorator does not itself
  reinterpret or validate whether the observation authorizes the action.
  That is the checked provider's responsibility.

The [general audit policy](AUDIT.en.md) is retained. By default, if
requested cannot be recorded, the backend is not called. best_effort
must be chosen explicitly. If completed fails, the backend's returned
result is preserved and the diagnostic counter increases, without
claiming rollback. Backend exceptions are recorded as
backend_threw/system_error and propagated; they do not prove an absence
of effects. backend_invoked indicates a provider call, not a write,
successful commit, or execution at the next login.

The sink must copy events it retains, be thread-safe, and not change
action arguments from callbacks. The caller must keep those arguments
valid and unchanged during the call. The backend is serialized per
instance, not across other processes or decorators. No automatic audit
persistence, actor authentication, destructive retries, sink timeout,
or elevated helper. The host remains responsible for consent and log
failures.

The 0.x API was extended with a new virtual method and enum value:
rebuild the library and consumers. The default not_supported method
preserves source compatibility for existing custom backends, not binary
compatibility with older vtables.

## Evidence

2026-10-15: Release Windows 76/76 and Linux 56/56. Packages and external
consumers were rebuilt and run with exit code 0 on both. The native
provider's 6801 was reconfirmed in a private fixture; auditing does not
change that availability.

Portable tests cover exact forwarding of all fields, separation from
the generic method, none/not_supported/permission_denied/stale_target/
system_error results, two stages and ID, flags, privacy, event-only
truncation, the original exception, requested/completed sink failures,
and best_effort. The real backend test uses only an object with an empty
ID: invalid_argument on Windows and not_supported on Linux, without
requesting a valid registry change. The installed consumer also uses
only that invalid action and checks both events.

This verifies the decorator and dispatch, **not** a successful native
removal or restoration. Provider error 6801 remains unresolved; an
audited operation does not bypass it or become enabled merely because
a Monitor report exists.
