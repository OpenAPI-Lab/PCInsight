# HKLM Run: explicit views and transactional control

Revision 48, 2026-10-19. Native HKLM32/64 reads and listings succeeded.
Control is implemented and tested with an injected transaction model, but
**successful native mutation is still pending**: the private test returns
6801/ERROR_RM_NOT_ACTIVE without writes. Real HKLM, StartupApproved, KTM
configuration, and permissions are not changed to force availability.
This component does not close M05.

## API and scope

```cpp
#include <pcinsight/startup.hpp>
auto machine32 = pcinsight::list_windows_machine_run_registrations(
    pcinsight::WindowsMachineRunView::registry32);
auto machine64 = pcinsight::list_windows_machine_run_registrations(
    pcinsight::WindowsMachineRunView::registry64);
// Rows contain a winrun:hklm:32:HEX or winrun:hklm:64:HEX ID.
// Query the ID again before offering an action:
// auto observed = pcinsight::read_windows_run_registration(row.entry_id);
// Show HKLM / ALL USERS, the view, and the limitations; obtain consent.
// auto result = audited.set_windows_run_registration_enabled(*observed.value, false);
```

list_windows_run_registrations() and the Monitor/Sampler
include_windows_managed_run report still cover ONLY HKCU/shared. They do
not automatically include machine data. The new listing selects one view
per call. It does not mix errors from one view with supposedly complete
rows from another. An invalid enum leaves the Metric without a value.
AMD64 supports both 32 and 64; native x86 supports only 32 and does not
reinterpret 64 as 32. ARM64 remains unsupported under provider policy.
Linux and other platforms report requires Windows, not a measured empty list.

read_windows_run_registration and the existing explicit action recognize
HKCU/shared and HKLM/32/64 IDs. This is a 0.x behavior change: a previously
rejected HKLM ID can now request mutation if the caller already has
permission and KTM works. The host must disclose the ALL USERS impact
and obtain consent. This is not effective Windows approval, eligibility,
publisher authentication, proof of who created a backup, or a launch
guarantee. Generic Windows set_startup_enabled does not route to this provider.

## Hive, view, and backup

The provider opens HKLM Software with an ordinary read-only handle in
the selected view. Later operations are relative to that handle. It does
not use an empty open of a predefined key as though it returned a new
handle. Each child open/create explicitly retains KEY_WOW64_32KEY or
KEY_WOW64_64KEY. It does not construct Wow6432Node paths or change process
redirection. See the [view contract](https://learn.microsoft.com/en-us/windows/win32/winprog64/accessing-an-alternate-registry-view)
and [RegOpenKeyEx](https://learn.microsoft.com/en-us/windows/win32/api/winreg/nf-winreg-regopenkeyexw).

Run is HKLM/Software/Microsoft/Windows/CurrentVersion/Run and backup is
HKLM/Software/PCInsight/StartupDisabled/v1/Run, both in the SAME view.
HKCU is not used as backup for an HKLM entry, or vice versa. HKCU keeps
its prior RegOpenCurrentUser mechanism. The ID's hive and view labels
select the requested target; they are not cryptographic tokens or
authorization. If another ID is fabricated for a different entry with
identical state, text, and bytes, the content comparison does not prove
the observation's historical provenance. Do not use untrusted caller data
as permission for an elevated helper (L05).

The reader takes two observations of Run and backup; the listing
enumerates both twice. REG_SZ and REG_EXPAND_SZ bytes are preserved
without expansion or execution. registered, disabled_by_pcinsight,
conflict, and absent retain the same [limited meaning](WINDOWS_RUN_CONTROL.en.md).
Invalid data, permission failures, or detected changes leave the Metric
without a value. If both keys are absent after opening Software, a known
empty list is possible. The result is estimated and non-atomic: it does
not rule out ABA changes or prove immutability after return. Reading does
not create keys or require KTM.

## Actions, permissions, and auditing

The same transactional provider checks state, text, type, and bytes
within the transaction, writes the destination without replacement, and
deletes the source with a single commit. Failures abandon, roll back, and
close; a failed commit requires a new query without claiming confirmed
rollback. Limits remain 64 KiB per value, 10,000 rows per pass, 16 MiB
of listing input/output, and 260 UTF-16 code units for commands that can
be changed. HKLM does not relax limits or add a nontransacted fallback.

The provider requests KEY_READ on Software, QUERY_VALUE to read children,
and QUERY_VALUE/SET_VALUE for actions. Creating a backup key also depends
on the parent ACL. A writable HKLM root is not required and privileges
are not enabled. An ACL can deny a process that can read.
AccessDenied/PrivilegeNotHeld retain a permission-required or
permission_denied result, not zero or disabled. RegCreate can check the
parent descriptor independently of the handle mask; see
[RegOpenKeyEx permissions](https://learn.microsoft.com/en-us/windows/win32/api/winreg/nf-winreg-regopenkeyexw).

No UAC, automatic impersonation, other profiles, RunOnce/packages,
external tools, ACL/reflection/KTM changes, elevated retries, or resource
manager repair. Successful enumeration does not establish action availability.

SystemActions and AuditedActions use the existing explicit method with
no new virtual method or AuditOperation value. The target identifies
hive/view; command and native_data stay out of the event. The ENTIRE
request is now copied before the requested callback, including hive,
view, and content, so later changes to the original object from that
callback do not change it. This does not prevent races during copying.
Other methods retain their own contracts. Fail-closed and best-effort
policies, sink errors, and exceptions are preserved; rollback is not
invented. ID and actor remain sensitive and unauthenticated. No automatic
persistent log.

The free API and enum are additive. This component changes no snapshot,
option, or payload layouts, vtables, or persisted formats. Update and
rebuild consumers for the new symbol. This does not finalize API/ABI 1.0.

The scope describes the view requested from Win32 in the host context,
not universal proof of physical or global storage. Legacy 32-bit hosts
with registry virtualization and MSIX are not certified; they can present
other layers or merged views. The host controls its manifest and context.
The library does not change those settings or attribute backup authorship.
See [compatibility virtualization](https://learn.microsoft.com/en-us/windows/win32/sysinfo/registry-virtualization).

## Evidence

tests/windows_machine_run.cpp redirects only HKLM32/64 Software to two
distinct private HKCU branches outside Startup. Reads and enumerations
are native. Tests cover distinct data/bytes, backup-only entries, view
flags on each open, no user/KTM API in those reads, injected initial and
late errors and privilege failures, invalid views, simulated x86/ARM64,
and handle cleanup. Private branch separation is not mutation of real
HKLM views. The private transaction probe retains error 6801 with no
writes in both branches. If a host supports transactions, the test
validates removal/restoration only in those private branches. Cleanup
uses exact keys and values, without recursive deletion or administrator rights.

tests/windows_run_control.cpp also runs the full transaction model for
HKLM32/64: exact bytes, failed set/delete/commit, rollback after prepared
writes, stale observations and conflicts, limits, and handles. It requires
the correct view on EVERY open/create. This is a model, not successful
native mutation validation. windows_run_audit adds a callback regression
that attempts to change the ID to HKLM and alter bytes; the backend
receives the earlier copy.

Read-only real HKLM results on this host: one row in the 32-bit view,
four in the 64-bit view, and successful individual reads in both without
KTM. Counts can vary by machine and time. No real HKLM values were changed,
and no real write ACLs, UAC, physical MSVC/ARM64/x86 configurations, or
other systems were tested. Installed consumers link the new listing
without acting on valid entries. Logs: work/machine-run-*; final results
in work/machine-run-pinned-*.log.

Final Release: Windows 126/126 and Linux 110/110. Twenty repetitions
each of machine-run, run-control, and run-audit on Windows (60 runs) and
twenty run-audit repetitions on Linux. Both installed consumers were
rebuilt, relinked, and run with exit code 0. Final builds had no warnings
or leftover fixture keys. Linux validates the stub and auditing, not the
Windows registry. This is not remote CI and does not fix 6801.
