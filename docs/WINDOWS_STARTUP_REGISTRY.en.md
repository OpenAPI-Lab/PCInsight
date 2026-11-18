# Windows startup: Run registry inventory

Monitor populates startup_entries when include_startup_entries=true. This
is opt-in and runs no commands or helper processes, changes no registry
values, and does not elevate privileges. It uses only Advapi32, already
linked, and system APIs. It installs no dependencies.

## Coverage

The provider enumerates Software\Microsoft\Windows\CurrentVersion\Run in
the caller's HKCU and in HKLM. On AMD64 Windows it queries the shared HKCU
view once and the 64- and 32-bit HKLM views with explicit WOW64 flags,
without appending Wow6432Node. On native x86 Windows it queries a single
view of each hive. On ARM64 and other architectures, requesting the
provider fails with not_supported: the architecture-dependent views have
not been implemented or verified there.

There is one row per nonempty REG_SZ or REG_EXPAND_SZ value with a nonempty
name. name is the registry value name; command is the raw text converted
from UTF-16 to UTF-8. The provider does not expand variables, resolve
executables or shortcuts, or interpret quoting or shell syntax. It retains
even text that cannot be launched: inventory does not validate commands.
source distinguishes hive, view, and type and marks the data as registration
only. The ID is winrun:HIVE:VIEW:HEX, with the complete UTF-8 value name
encoded in hex to avoid collisions with separators in the name. Identical
names in different views remain separate registrations; entries are not
deduplicated by name or app.

status=unknown: presence in Run does not establish approval, effective
enablement, eligibility, execution, or success. The provider does not
interpret private StartupApproved bytes or infer enabled/disabled from
presence alone. publisher is empty (the value name does not establish
the publisher), impact=not_measured, and observed metrics and XDG fields
are absent. The [publisher guide](WINDOWS_STARTUP_PUBLISHER.en.md) adds a separate opt-in estimated
CompanyName lookup for an explicit candidate EXE. It does not verify the
signature or authorship or infer a publisher from the name.

The [Startup folders guide](WINDOWS_STARTUP_FOLDERS.en.md) describes the
file inventory added to the combined Monitor, without resolving shortcut
targets. The standalone Run reader does not include those folders.
RunOnce, shortcut resolution, scheduled tasks, policies, packaged
StartupTask entries, and other users' hives remain out of scope. This is
not a complete autorun or all-user inventory. Effective approval,
enable/disable/reset/properties, verified publisher, and Windows impact
measurement remain pending. A read ID grants no permission to modify an
entry; at this stage Windows actions had no provider for this entry type.

## Errors and limits

A missing key is skipped. Other open or enumeration failures, including
access denied and ERROR_MORE_DATA, propagate as a Win32 std::system_error
through Monitor instead of returning a partial list as complete. There
is no elevation or hidden retry. Non-text types, empty default names, and
empty commands are skipped. An empty list only means that the queried keys
reported no included registrations.

Unterminated text is accepted using its explicit byte length. Odd lengths,
embedded NUL, invalid UTF-16, an exact duplicate name during enumeration,
invalid buffers or lengths, and incomplete APIs fail closed. Limits:
10,000 values per key including ignored entries, 16,383 UTF-16 code units
per name, 65,536 bytes per value, 16 MiB of cumulative raw input, and
16 MiB of output. The raw budget includes ignored values, bounding the
name set. Long commands are observed text, not a promise that they can
be executed.

All handles close even on exceptions. Reads are not atomic: concurrent
changes can omit or alter registrations, and duplicate checks cannot
detect every race or establish consistency across an entire hive. The
limits do not impose a timeout on a blocked registry API. Returned
strings may contain sensitive paths or arguments; consumers decide what
to expose.

## Verification

Fixtures cover user and machine hives, two AMD64 views and one x86 view,
Unicode and names with separators, unexpanded REG_EXPAND_SZ, ignored types,
unterminated data, duplicates, truncated or invalid UTF-16, incomplete APIs,
permissions, registration/inventory/payload limits, missing keys, and
cleanup on late failure. The first run found accidental duplicate names
in the valid fixture; test data was corrected without weakening duplicate
rejection. Native read-only and Monitor opt-in/opt-out tests do not create,
delete, change, or run any real entry and do not require registrations to
exist. MSVC, native x86 Windows, and other machines still lack native
validation in this revision. The x86 view selection is tested by injection,
not certified by an AMD64 build.

Release: Windows 65/65 and Linux 50/50. Installed packages and external
consumers were rebuilt and run on both. The Windows consumer also exercises
Monitor with include_startup_entries. Linux checks regressions, not the
Windows registry; a Linux build does not establish native Windows coverage.

Official sources: [Run and RunOnce](https://learn.microsoft.com/en-us/windows/win32/setupapi/run-and-runonce-registry-keys),
[RegEnumValueW](https://learn.microsoft.com/en-us/windows/win32/api/winreg/nf-winreg-regenumvaluew),
[shared and redirected WOW64 keys](https://learn.microsoft.com/en-us/windows/win32/winprog64/shared-registry-keys).
