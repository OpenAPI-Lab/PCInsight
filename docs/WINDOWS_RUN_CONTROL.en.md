# Reversible Run registration control through PCInsight

Status: the provider is implemented and the flow was tested with an injected
transaction model; **successful native validation is still pending**. In the
current Windows environment, RegOpenKeyTransactedW returns 6801
(ERROR_RM_NOT_ACTIVE). The test confirmed that the action returns
not_supported without writing. The system resource manager was not repaired
or reconfigured to force availability. These tests do not close the Windows
enable/disable subtask.

Update: the **native status reader is complete** and independent of KTM.
Writes still lack successful native validation; no write fallback was added.
Error 6801 was also confirmed when querying a real HKCU entry through the
transactional path, not just the private key. The new ordinary reader obtained
that entry's status without modifying it. This narrows the observed failure to
the transactional path, but does not diagnose the internal cause or establish
that KTM is inactive on all Windows systems.

## Explicit API, not Windows approval

Revision 48 adds [explicit HKLM32/64 support](WINDOWS_MACHINE_RUN_CONTROL.en.md)
with a separate listing and successful native reads. Native mutation remains
pending due to error 6801. This does not change Windows approval or turn
injected model tests into evidence of a successful native write.

pcinsight/startup.hpp exposes read_windows_run_registration(id) and
set_windows_run_registration_enabled(observed, enabled). It supports the
current user's HKCU/shared/Run and HKLM32/64 via explicit inventory IDs.
Startup folders, packages, RunOnce, other users, and ARM64 are out of scope.
The provider does not execute commands, use a shell, elevate privileges, or
modify StartupApproved. It installs no utilities and uses system Advapi32
and KtmW32. Rebuild consumers for the 0.x API.

The GUI should label this “Remove or restore a Run registration through
PCInsight,” not “Windows approval.” Generic SystemActions::set_startup_enabled
does not silently route to this mechanism. The explicit
SystemActions::set_windows_run_registration_enabled method delegates to the
same provider and can be wrapped by [AuditedActions](WINDOWS_RUN_AUDIT.en.md).
The host is responsible for consent and the sink; the free function does
not record audit events by itself.

```cpp
auto observed = pcinsight::read_windows_run_registration(entry.id);
// Show observed.value->state, provenance, and the warning to the user.
// After explicit confirmation and in the same user context:
// auto result = pcinsight::set_windows_run_registration_enabled(*observed.value, false);
// Query the retained ID again before offering restoration (true).
```

WindowsRunRegistration retains entry_id, the unexpanded UTF-8 command,
native_type (REG_SZ 1 or REG_EXPAND_SZ 2), and exact native_data, including
any existing terminator. The action compares state, text, type, and bytes
with the current read inside a transaction. A fabricated object does not
authorize creation of an arbitrary command: the source must match. The
observation is not a cryptographic token, SID, historical identity, or
protection against same-content ABA changes. Do not pass it across
accounts or impersonation contexts.

## PCInsight states and recovery

The reader observes two locations without creating keys. It uses
RegOpenCurrentUser(KEY_READ), RegOpenKeyExW(KEY_QUERY_VALUE), and bounded
queries, without creating, starting, committing, or aborting transactions.
It reads Run and backup twice and compares presence, type, and bytes.
Only matching observations yield a value, with Support::estimated and
a non-atomicity warning. Detected changes leave the Metric without a value
and with a note to reload. Permission failures retain permission_required.
The reader does not detect ABA, later changes, or every consistent-looking
intermediate state across locations. A read result does not establish
write availability.

- registered: a value in Run, without a backup of the same name.
- disabled_by_pcinsight: backup present, Run absent.
- conflict: both present; no action overwrites or chooses one.
- absent: neither present; no registration is fabricated to “enable” it.

These states show observed presence, not who created the backup. The store
is user-editable configuration and does not authenticate authorship, trust,
or safety. registered does not prove that Windows will run the entry.
Removing this source cannot prevent another mechanism or app from recreating
it or launching the same program. The inventory's StartupEntry.status does
not change from unknown to enabled through this API.

Disabling moves the selected value to HKCU\Software\PCInsight\StartupDisabled\v1\Run,
preserving name, type, and bytes, and deletes only that value from Run in the
same transaction. Restoration reverses the move and removes the backup only
as part of the committed restoration. An existing destination value is never
overwritten, even if it matches. Repeating a call with an old observation
returns stale_target, not fabricated idempotent success. Empty keys remain;
application keys are not deleted.

After removal, the entry no longer appears in the ordinary Run inventory.
The caller can retain the ID for rechecking or restoring it, or recover
backup IDs through [list_windows_run_registrations](WINDOWS_RUN_LISTING.en.md).
Monitor/Sampler offer a separate [opt-in report](WINDOWS_RUN_MONITOR.en.md),
and the explicit action has [auditing](WINDOWS_RUN_AUDIT.en.md); the GUI
remains pending. This is not a complete Startup manager. The registry store
persists across runs rather than relying on process memory. There is no
export, bulk deletion, or automatic backup cleanup.

## Transactions and limits

For the action, RegOpenCurrentUser pins the hive for the current call's
context. RegOpenKeyTransactedW queries both fixed keys. When needed,
RegCreateKeyTransactedW creates the destination inside the transaction.
RegSetValueExW and RegDeleteValueW operate on transacted handles, and
CommitTransaction is the sole success point. Failures abandon and close the
transaction, using explicit RollbackTransaction and RAII cleanup of keys,
hive, and transaction handle. There is no nontransacted fallback, destructive
retry, or hidden second attempt after commit failure.

If commit fails, the result is unconfirmed and the caller must requery.
Do not infer from that error that everything was restored. Transactional
conflict or changed content/state yields stale_target; access denial yields
permission_denied; unsupported transactions or an inactive resource manager
yield not_supported. Other errors preserve the Win32 code. Read failures
return a Metric with no value, not fabricated absent or disabled states.
The action revalidates the estimated observation in its own transaction;
it does not downgrade to the reader's two ordinary reads.

Limits: 131,128 bytes for the ID; a canonical hex name encoding strict
UTF-8, up to 16,383 wchar_t units with no NUL; and up to 64 KiB per location
with a supported text type. UTF-16 is strict, without embedded NUL or
empty commands. Reads preserve unterminated strings using their explicit
length, and actions preserve those bytes. Actions are limited to 260 UTF-16
code units per command under the documented Run limit; longer commands
can be observed but are not modified. The 5,000 ms KTM timeout is not a
hard deadline for API calls or commit. No protection against later changes.

## Test evidence

Release: Windows 73/73 and Linux 54/54. Packages and external consumers
were installed, rebuilt, and run with exit code 0 on both. The consumer
tests only invalid calls that cannot mutate the system and leaves real
Startup untouched. Linux validates API linkage and the unsupported result,
not the Win32 provider. These suites do not resolve native transaction
validation.

2026-10-15 injected model: removal/restoration of REG_SZ and REG_EXPAND_SZ,
unterminated bytes, Unicode, literal variables, conflict on both sides,
stale observation, changed backup, existing destination, query/set/delete/
commit failures, rollback after prepared writes, limits, invalid strings,
incomplete APIs, and handle cleanup. **This does not certify successful
real transactions.**

The native mutation test uses only a private temporary key outside Startup
with internal test path redirection. The fixture creates its test value
through an ordinary write. The transacted action returns 6801 before any
write and leaves that value unchanged. The ordinary reader observes both
the fixture and a real HKCU entry with Support::estimated. If a host
supports transactions, the same suite exercises them only in that private
key. The two empty keys from initial aborted tests were checked for emptiness
and removed by exact name. The final test cleans up its private key without
recursive deletion. No real Run or StartupApproved entry was changed and
no command was launched.

Reader regressions verify that an injected API without begin/commit/rollback
still permits reads, writes are rejected without those callbacks, and
PCInsight states and changes between passes are handled. The query never
calls set/delete/create or requires KTM. The installed Windows consumer
queries HKCU read-only when an entry exists.

Official sources:
[multiple registry operations in one transaction](https://learn.microsoft.com/en-us/windows/win32/sysinfo/writing-and-deleting-registry-data),
[RegOpenKeyTransactedW](https://learn.microsoft.com/en-us/windows/win32/api/winreg/nf-winreg-regopenkeytransactedw),
[CreateTransaction: timeout and abandonment](https://learn.microsoft.com/en-us/windows/win32/api/ktmw32/nf-ktmw32-createtransaction),
[CommitTransaction](https://learn.microsoft.com/en-us/windows/win32/api/ktmw32/nf-ktmw32-committransaction),
[Run command limit](https://learn.microsoft.com/en-us/windows/win32/setupapi/run-and-runonce-registry-keys).

Independent reads:
[RegOpenKeyExW](https://learn.microsoft.com/en-us/windows/win32/api/winreg/nf-winreg-regopenkeyexw).
