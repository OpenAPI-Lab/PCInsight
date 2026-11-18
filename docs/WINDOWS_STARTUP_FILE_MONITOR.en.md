# Opt-in Startup backup report in Monitor/Sampler

Revision 45, 2026-10-19. `SampleOptions::include_windows_startup_file_backups`
(false by default) requests the explicit list of removed files.
`SystemSnapshot::windows_startup_file_backups` holds the same Metric of
WindowsStartupFileBackup entries (UTF-8 name and registration). It is not
merged with startup_entries or windows_run_registrations; the three options
are independent.

```cpp
pcinsight::SampleOptions options;
options.include_windows_startup_file_backups = true;
pcinsight::Monitor monitor;
auto snapshot = monitor.sample(options);
const auto& report = snapshot.windows_startup_file_backups;
if (report.value) {
    for (const auto& item : *report.value) {
        // Display item.name and item.registration.state.
        // Restore ONLY after consent through the separate control API.
    }
}
// For continuous sampling, pass the same options to the Sampler constructor.
```

When the option is off, the backup provider is not called: the Metric has
no value, Support::unsupported, and the note "disabled". This is not a
measured empty list. When enabled, the provider's value, support, and note
are preserved, including errors, permission_required, and a known empty
list. Previous reports are not reused. On Linux and other systems, the
provider reports unsupported/requires Windows. A failure represented by a
Metric does not by itself invalidate other snapshot fields. Allocation
exceptions and other sampling failures follow the general Monitor/Sampler
contract; they do not become successful queries.

The report does not create folders, move or restore files, request UAC,
query Run/KTM, run apps, or change StartupApproved. It is metadata from
one source, not Windows effective enabled/disabled status or a launch
guarantee. It retains the [provider's](WINDOWS_STARTUP_FILE_CONTROL.en.md)
local-drive/current-user scope, limits, estimated support, non-atomic
behavior, and restrictions. It does not implement action auditing or
generic buttons.

## Ownership, sampling, and compatibility

Each snapshot owns its vectors and strings. Disabling later samples or
changing a caller-owned copy does not alter earlier snapshots or Sampler's
internal state. latest/history/latest_timed/timed_history hold the report
in memory as part of the snapshot. The monotonic 60-second window and its
capacity limits are unchanged. No persistence is added: HistoryStore,
WindowsHistoryStore, their formats, and merge logic still do not
serialize these backups.

Enumeration can be expensive, and each sample retains its own metadata
(up to 16 MiB of provider text plus overhead). Use a sampling interval
and capacity appropriate for the GUI. There is no cache, shared
deduplication, or independent per-group refresh. Native calls have no
hard timeout. A sample combines sequential queries; captured_at does not
establish an atomic instant for all data. Sampler works on its own thread
without implicitly transferring the calling thread's impersonation
context. Do not use rows across accounts as action authorization.

**0.x layout change:** fields are appended to SystemSnapshot and
SampleOptions. Rebuild and relink the library and ALL its consumers
together. Do not mix old objects/libraries with new headers or use sizeof
as a persistence format. There is no stable 0.x binary ABI guarantee.
Provider types move to types.hpp without changing definitions or layouts;
the control header still includes them. No new virtual methods, actions,
or persisted-format changes. A05 (1.0 API/ABI review) remains pending.

## Evidence

Portable tests/windows_startup_file_monitor.cpp injects the internal
provider to verify that opt-out skips it, populated/empty/error reports
are preserved without coercion, missing providers are handled, old values
are cleared, and ownership is maintained. Native read-only Monitor tests
cover independent opt-in and opt-out. Sampler tests allow five seconds
to obtain a sample and cover latest/history/timed_history and independent
copies. They do not claim a positive result for real user backups if the
user store is empty or inaccessible.

tests/windows_startup_file_control.cpp feeds the same report-building
code with a native provider redirected to a private fixture with a real
backup. It confirms the row and identity, then disables the report
without touching the file. Real startup settings are unchanged. The
installed consumer builds and links the new fields and option and calls
Monitor without actions. Linux checks the stub and its own regressions,
not the Windows provider. Logs: work/startup-backup-monitor-*.log.
This is not remote CI or validation of other filesystems or architectures.

Release: Windows 124/124 and Linux 109/109. Twenty repetitions of each of
the two Windows tests (monitor/control, 40 runs) and twenty of the Linux
monitor test. Both installed consumers were rebuilt, relinked, and run
with exit code 0. The initial and incremental WSL builds reported Clock
skew for compiler_depend.make files with mtime 0.1–0.2 seconds in the
future. Original logs were retained without changing the clock or
configuration or hiding the warning. See the last
work/startup-backup-monitor-settled-build-linux.log as well. No internal
cause for the skew is claimed, and a make warning is not a C++ compilation
error. The final settled pass exited 0 without warnings or additional
compilation or linking; the final suites validate the same binaries.
Windows also built without warnings. The earlier warning remains in its logs.
