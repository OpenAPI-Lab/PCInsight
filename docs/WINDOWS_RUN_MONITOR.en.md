# Run and backup report in Monitor and Sampler

Opt-in: `SampleOptions.include_windows_managed_run` (false by default).
`SystemSnapshot.windows_run_registrations` holds the Metric for the HKCU
Run and PCInsight backup listing. WindowsRunRegistration and
WindowsRunRegistrationState are declared in types.hpp and remain available
through startup.hpp as before.

```cpp
pcinsight::SampleOptions options;
options.include_windows_managed_run = true;
// Does not require include_startup_entries or signature/shortcut/CompanyName options.
pcinsight::Monitor monitor;
auto snapshot = monitor.sample(options);
const auto& report = snapshot.windows_run_registrations;
if (report.value) {
    // Render report.value, report.support, and report.note.
    // registered / disabled_by_pcinsight / conflict are not StartupApproved.
}
```

## Opt-in semantics

Monitor calls the [bounded listing](WINDOWS_RUN_LISTING.en.md) once per
sample when the option is requested. A failed read or later opt-out does
not retain a previous result. An already returned sample owns its copy;
later samples do not change it.

With the option false, the Metric has no value and a note that the option
is disabled. It does not call **this** listing provider. Other options,
such as include_startup_entries, can read Run separately. With true on
Windows, two matching enumerations produce an estimated result, including
a valid empty vector. Provider failures leave the value absent and preserve
support and note. An error is not replaced by an empty vector. With true
on Linux, the report is explicitly unsupported and does not try Win32 APIs.

The report does not append backups to generic startup_entries or change
StartupStatus, impact, signatures, or CompanyName. These are not two
atomically joined inventories. If both reports are enabled, they are
collected at different times and may differ. captured_at timestamps the
overall sample, not each individual read or registration. Report availability
does not enable the transactional action or resolve its 6801 error.
No mutation, elevation, KTM, or execution.

## Sampler and cost

Sampler already retains full SystemSnapshot values in latest/history.
The opt-in report is carried there with support and note, without
additional caching logic or write policy. It is not converted into a
numeric chart or startup impact. It can contain sensitive paths, arguments,
and native bytes; the host decides what to display, retain, and serialize.
There is no automatic disk persistence.

The listing allows 16 MiB of output values and up to 20,000 rows, plus
object overhead. A history of 60 samples can use up to roughly 960 MiB of
payload, plus overhead and temporary copies from latest/history. Do not
describe 16 MiB as a global memory cap. Use an on-demand Monitor for
Startup metadata or a separate, low-frequency sampler with limited
retention. Leave this flag false in the fast CPU/RAM sampler. The test
uses a one-hour interval and capacity 1; it does not recommend polling
every second.

The listing's input/output limits and diagnostics are retained, with no
hard registry API deadline. No new runtime dependency or change to
existing actions. The 0.x API was extended: rebuild the library and consumers.

## Verification

2026-10-15: Windows 75/75 and Linux 55/55 Release. Packages and external
consumers were rebuilt and run with exit code 0 on both. The new test
keeps assertions active and covers opt-out by default, opt-in independent
of generic inventory, disabling without retaining results, preservation of
an earlier snapshot copy, Sampler latest/history with capacity 1, and
bounded shutdown. Native Windows Monitor observed 16 real rows without
write actions. Linux returned unsupported with the flag enabled. The
Windows consumer verifies the estimated report and IDs and the default
disabled state. This integration created or deleted no keys and did not
run any entries.

The suite retains listing-provider tests for failures, permissions, and
limits. This integration does not certify native removal/restoration, a
built GUI, ClickUp updates, history persistence, or remote CI.
