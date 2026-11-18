# Windows Run registration properties

`get_startup_properties(entry.id)` lets the GUI build its Properties panel
for `winrun:...` entries. It does not open system dialogs, execute commands,
expand variables, query binaries, or change the registry. It uses system
Advapi32 without external utilities. Files in Startup folders (`winfolder:`)
are handled separately: [source file properties](WINDOWS_STARTUP_FILE_PROPERTIES.en.md).
Do not confuse registry fields with file fields or shortcut size with target size.

```cpp
#include <pcinsight/startup.hpp>
auto result = pcinsight::get_startup_properties(entry.id);
if (result) {
    // entry: name, literal command, source; status unknown and impact not_measured
    // fields: hive, view, key, value_name, value_type, command_unexpanded
    // source_file_size_bytes: no value, because the registry entry is not a file
}
// stale_target: reload the inventory and Properties; do not run an alternative.
```

## Selection and data

Only inventory scopes are accepted: hkcu:shared, hklm:64, and hklm:32.
The suffix is the UTF-8 name encoded in canonical uppercase hex, not
an arbitrary registry path. UTF-8 decoding is strict, with no NUL and
a name limit of 16,383 wchar_t units. IDs are limited to 131,128 bytes
to allow hex-encoded Unicode names; Linux keeps its 32 KiB limit. RunOnce,
other keys or accounts, and folder IDs are not accepted. HKCU is queried
in the caller's Windows context. The ID does not contain a SID and is not
a persistent token across accounts or impersonation contexts.

It opens only `Software\Microsoft\Windows\CurrentVersion\Run` with
KEY_QUERY_VALUE and an explicit WOW64 view for HKLM on AMD64. Native x86
accepts the 32-bit view without flags; the 64-bit view does not exist.
ARM64 remains not_supported, consistent with the current inventory;
coverage of its additional views is not claimed.

The result contains a freshly queried StartupEntry, not a saved copy
of inventory. The UTF-8 fields in the `Windows Run` group are hive, view,
key, value_name, value_type, and command_unexpanded. REG_SZ and REG_EXPAND_SZ
retain the literal command. There is no shell interpretation, PATH lookup,
or percent expansion. Registration is not startup approval: status is
unknown, impact is not_measured, and publisher, signature, and shortcut
metadata are absent. Those Monitor opt-ins are separate.

`source_file_size_bytes` is a Metric with no value for registry entries,
not a known zero size. On Linux it contains the .desktop file size;
`file_size_bytes` remains a legacy Linux-only field. New consumers should
use the Metric. The 0.x API was extended: rebuild against the installed
library and headers.

## Errors and consistency

The same value is read twice through the same open handle with a fixed
64 KiB buffer per read. Type, length, and bytes must match; there are no
unbounded retries. Unterminated REG_SZ is accepted within the declared
size. If there is a final terminator, only one is removed. Odd or excessive
byte counts, embedded NUL, invalid UTF-16, or malformed IDs yield
invalid_argument. Other types yield not_supported. An empty command
yields not_found because it is not an inventoried app registration.

ACCESS_DENIED yields permission_denied. A key or value initially absent
yields not_found. Growth/ERROR_MORE_DATA, disappearance on the second
read, or changed bytes, type, or size yields stale_target. Other native
errors yield system_error with the Win32 code. No partial result is
returned. RAII releases the handle even on errors.

Two matching observations are not a transaction: they do not detect ABA
changes, case-equivalent renames, recreation of a key under the same name,
or later mutations. The ID identifies a name and view, not the historical
instance of the value or executable. It does not authorize enabling,
disabling, or execution. The query can show a new command if it changed
before both reads. No hard API deadline or claim of complete inventory
across all startup sources.

## Verification

2026-10-15: Release Windows 71/71 and Linux 54/54. Packages were installed
and external consumers rebuilt and run with exit code 0 on both.
Native Windows tests observed three real Run registrations through the
public query, without registry changes. Injected tests cover HKCU/HKLM32/
HKLM64/x86/ARM64 views, Unicode, unterminated data, literal variables,
invalid NUL/UTF-16/sizes, the 64 KiB limit, changes and disappearance,
errors and permissions, and cleanup. Linux verifies the new size Metric
and retains the XDG properties regression tests. MSVC and other hosts
need additional validation.

Official sources:
[RegQueryValueExW and strings without guaranteed termination](https://learn.microsoft.com/en-us/windows/win32/api/winreg/nf-winreg-regqueryvalueexw),
[RegOpenKeyExW](https://learn.microsoft.com/en-us/windows/win32/api/winreg/nf-winreg-regopenkeyexw),
[WOW64 registry views](https://learn.microsoft.com/en-us/windows/win32/winprog64/registry-redirector).
