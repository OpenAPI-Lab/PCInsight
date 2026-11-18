# Listing Run registrations and PCInsight backups

Revision 48 adds a [separate HKLM32/64 listing](WINDOWS_MACHINE_RUN_CONTROL.en.md)
through list_windows_machine_run_registrations(view). The listing described
here remains HKCU/shared ONLY; Monitor's scope is not silently expanded.

Completed component: `list_windows_run_registrations()` in pcinsight/startup.hpp.
It reconstructs the list from the registry, including values removed from
ordinary Run and retained in the PCInsight store. It needs no IDs saved
by an earlier process and does not trigger writes.

```cpp
auto listing = pcinsight::list_windows_run_registrations();
if (listing.value) {
    for (const auto& row : *listing.value) {
        // entry_id, state; command/native_type/native_data if there is no conflict.
        // Display provenance and listing.support (estimated).
    }
}
// Without a value, show the error or provider unavailability, not a fabricated empty list.
```

## Semantics and scope

Only the current context's HKCU Run and HKCU Software/PCInsight/StartupDisabled/v1/Run.
Uses RegOpenCurrentUser(KEY_READ), RegOpenKeyExW(KEY_QUERY_VALUE), RegEnumValueW,
and RAII cleanup. No KTM, key creation, set/delete, execution, elevation,
external utilities, or Windows approval queries. Linux returns unsupported.

One row per value name included in either location:

- registered: Run only; does not mean Windows has enabled it or guarantee execution.
- disabled_by_pcinsight: PCInsight store only; does not prove authenticity or authorship.
- conflict: both locations; no overwrite and no command selected for restoration.

The listing emits no absent rows. A vanished ID can be rechecked through the
individual reader. Ordinal Unicode comparison ignores case via
CompareStringOrdinal. For example, Same and sAME are grouped as a conflict,
not two apps. The ID uses the name reported by Run when present and the
backup name otherwise. It uses the compatible winrun:hkcu:shared:HEX form,
with UTF-8 encoded as uppercase hex. Ordering is ordinal and case-insensitive,
not execution order, product name order, or localized GUI order.

For registered/disabled rows, the reader preserves the unexpanded UTF-8
command, native type, and exact source bytes, including a terminator if one
existed. Conflict rows leave those fields empty or unselected but still
report the conflict. Ordinary Run skips unnamed values, empty commands,
and nontext types, as the current inventory does. In backups those cases
are errors, not silent skips. Malformed text or an unsupported backup
type rejects the entire listing.

## Consistency and limits

Run and backup are enumerated twice (four passes). Maps are compared by
exact name, type, and bytes before returning Support::estimated. The
comparison detects even case changes between passes. It provides no atomic
snapshot, ABA protection, or guarantee against changes during or after
the reads. It does not establish stability of ignored values or completeness
of all autoruns. A working listing does not establish that the transactional
action is available.

Up to 10,000 visited values per key per pass, including skipped entries;
16,383 wchar_t units per name and 65,536 bytes per value. The four passes
share a 16 MiB raw budget (UTF-16 names and data, including skipped values)
and a 16 MiB output budget (IDs, commands, and native bytes). The union
has up to 20,000 rows. UTF-16/UTF-8 is strict, with no embedded NUL.
Duplicates or case-equivalent names within a pass, changes, ERROR_MORE_DATA,
permission failures, or exceeded limits leave the Metric without a value,
not a partial result. permission_required and native diagnostics are
preserved where applicable. A nonexistent key is empty; a failed query
is not. Registry APIs have no hard deadline.

Data can contain sensitive paths and arguments. The PCInsight store is
user-editable configuration, not a signature or security boundary.
The GUI should reread before acting and not present the text as an authorized
command. The listing is available on demand and as a [Monitor/Sampler
report](WINDOWS_RUN_MONITOR.en.md) with opt-in off by default, so it is not
scanned on every tick by default. The explicit action can be [audited through
an opt-in decorator](WINDOWS_RUN_AUDIT.en.md) without changing write availability.
The 0.x API was extended: rebuild consumers against the updated installation.

## Verification

2026-10-15: Windows 74/74 and Linux 54/54 Release. Installed packages and
external consumers were rebuilt and run with exit code 0 on both. The
Windows consumer verifies estimated listing and HKCU scope without
mutation; Linux verifies unsupported.

A native test used private keys outside Startup: Run values, a Unicode
backup without Run, and a Same/sAME conflict. Three correct rows and two
independent calls, without previous IDs or in-memory provider state.
Windows was not rebooted and no other process was run to claim a reboot test.
The read-only real HKCU listing reported 16 rows. Only the fixture's own
values and keys were removed by exact names, without recursive deletion
or changes to real Startup.

The injected provider has no KTM or write callbacks. Tests cover states,
case-equivalent duplicates, case changes between passes, late failures,
permissions, MORE_DATA, invalid fields, ignored types versus invalid backups,
a valid empty list, 10,000 and 16 MiB limits, incomplete APIs, and handle
cleanup. This does not resolve successful native removal/restoration
validation (6801) or implement other Startup sources.

Official sources:
[RegEnumValueW](https://learn.microsoft.com/en-us/windows/win32/api/winreg/nf-winreg-regenumvaluew),
[CompareStringOrdinal](https://learn.microsoft.com/en-us/windows/win32/api/stringapiset/nf-stringapiset-comparestringordinal).
