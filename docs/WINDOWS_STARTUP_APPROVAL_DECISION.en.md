# M05: pending decision on effective Windows startup approval

Revision 49, 2026-10-19. Documentation research and review of existing
evidence only. This component changes no code and runs no new tests.
M05 remains open, with unchanged completion criteria and official
denominator.

## Research findings

The [StartupTask class](https://learn.microsoft.com/en-us/uwp/api/windows.applicationmodel.startuptask?view=winrt-26100)
requires a startup declaration in the package manifest.
GetForCurrentPackageAsync enumerates the current package's tasks. It does
not enumerate other apps' Run entries or Startup files.
[RequestEnableAsync](https://learn.microsoft.com/en-us/uwp/api/windows.applicationmodel.startuptask.requestenableasync?view=winrt-26100)
also cannot override a user disabling the app in Task Manager; the user
must re-enable it manually.

Implementing StartupTask for the host package therefore does NOT satisfy
effective approval or general enable/disable for M05. Presence in Run or
in a Startup folder is not enough either. PCInsight's explicit providers
retain their registration/removal semantics and are not renamed as
effective Windows approval.

Microsoft documents startup configuration through [Settings, Task Manager,
and the Startup folders](https://support.microsoft.com/en-us/windows/experience/startup-boot/configure-startup-applications-in-windows).
Opening that interface could be a GUI integration, but it is not a telemetry
or control provider in this library and does not complete M05. The research
did not open or automate the interface.

The official sources reviewed did not reveal a general public
StartupApproved contract equivalent to Task Manager's controls. This is
a bounded research finding, NOT proof that no such API exists in any
version. The meaning of private registry bytes is not inferred, and no
experimental provider is presented as certified universal support.

## Separate pending item: Run transactions

Revision 48's final evidence, work/machine-run-pinned-native-windows.log,
records ERROR_RM_NOT_ACTIVE (6801) when opening a transacted key in both
private redirected branches used to test the HKLM32/64 views. Nontransacted
queries work; the provider performed no writes during those attempts.
The [Win32 error catalog](https://learn.microsoft.com/en-us/windows/win32/debug/system-error-codes--6000-8199-)
describes 6801 as transaction infrastructure not started in the specified
resource manager. This does NOT by itself identify the local cause or
establish that elevation would resolve it.

The probe is not repeated, KTM is not reconfigured, ACLs are unchanged,
and no nontransacted fallback is added. The model test is still not
equivalent to a successful native mutation. Resolving StartupApproved
would not automatically resolve this separate item, or vice versa.

## Decision requested

Revision 50 continuation: the next "prosigue" was interpreted as moving
to another subtask and deferring M05, not as consent for the experimental
provider. M05 remains open with all completion items pending. On
2026-10-21 the user explicitly confirmed "Defer M05 and continue with
another subtask"; revision 51 records that decision without enabling
the experimental provider.

The user was presented with two alternatives; the assistant did not
change scope on its own:

- Defer this part of M05 and continue with another official subtask.
  M05 stays open without partial credit. Do not prolong an investigation
  without evidence.
- Accept an experimental, version-limited provider based on undocumented
  internal structures. Acceptance would authorize only development, not
  certify the format or authorize changes to other apps' real entries,
  privilege elevation, or system configuration changes during tests.

If the second option is chosen, define an explicit contract before
offering writes: opt-in, verified scope and versions, source and view
identification, unknown status for unrecognized formats, stale observations
and conflicts, byte preservation, and recovery policy. Initial tests
must stay isolated in private fixtures. An experimental interpretation
must not promise eligibility or guaranteed launch. Keep M05's remaining
limits visible; do not weaken the criteria to close it artificially.

## Tracking

No new implementation or hardware validation. Latest implemented and
verified suites: Windows 126/126 and Linux 110/110 Release (revision 48),
not rerun here. C56/E11/P6/B2; 75 active and 3 X excluded: 56/75=74.7%
closed and 19/75=25.3% open. This is not an estimate of remaining hours.
