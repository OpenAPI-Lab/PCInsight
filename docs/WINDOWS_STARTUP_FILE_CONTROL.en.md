# Explicit Windows Startup file control

Revision 44, 2026-10-19. The provider successfully removed and restored
private files **using native operations**, outside real Startup. This is
independent of the Run/KTM provider, which still returns 6801 on this host.
At this revision M05 remained open for effective approval, HKLM/common
scope, backup integration, and other actions. Revision 44 completes the
explicit backup listing; it does not rely on IDs retained by the host or
silently add backups to legacy inventory.
Revision 45 adds a separate [opt-in Monitor/Sampler report](WINDOWS_STARTUP_FILE_MONITOR.en.md),
independent of inventory and actions. The new 0.x SystemSnapshot and
SampleOptions fields require rebuilding consumers; see the linked contract.
Revision 46 adds the [explicit SystemActions/AuditedActions action](WINDOWS_STARTUP_FILE_AUDIT.en.md).
The free function does not emit events on its own. A new 0.x virtual method
requires rebuilding the library, consumers, and custom backends without
using older vtables.
Revision 47 adds [explicit common scope](WINDOWS_COMMON_STARTUP_FILE_CONTROL.en.md):
reads and actions by user/common prefix and a separate common listing,
using existing permissions and an ALL USERS warning. This is not HKLM
or Windows approval.

## Usage and meaning

Additional API in `pcinsight/windows_startup_file_control.hpp`:

```cpp
auto observed = pcinsight::read_windows_startup_file_registration(entry.id);
// Show the state and limitations, and obtain explicit user consent.
// If registered, remove that file from the direct Startup entries:
// auto result = pcinsight::set_windows_startup_file_registered(*observed.value, false);
// Query the SAME retained ID again before offering restoration (true).
```

Accepts inventory IDs `winfolder:user:` and `winfolder:common:` in their
respective known folders. Ordinary and extended local-drive paths are
accepted. UNC, mapped network drives, other accounts, non-direct names,
streams, desktop.ini metadata, and the reserved store name are rejected.
The API cannot create a new command. It does not change the registry,
StartupApproved, associations, permissions, processes, or shortcut targets.
It neither runs nor resolves files. It is not automatically wired to generic
enable/disable; since revision 46, auditing is opt-in through AuditedActions.
Monitor/Sampler read backups only if the host enables the explicit
revision 45 option. The GUI should say “Remove or restore a Startup file
through PCInsight,” not present this as effective Windows approval or an
execution block.

States: absent, registered (direct file only), retained_by_pcinsight
(backup file only), and conflict (both). Conflict and absence have no
selected file identity. Presence in backup does NOT authenticate who
created it. Reads do not create directories and report Support::estimated.
The result is not an atomic snapshot or evidence of eligibility, trust, or
launch. Absence here does not prevent the program from starting another way.

false moves the file to `Startup/PCInsight.Disabled.v1/<same name>`; true
restores it directly under Startup. The file is preserved; contents are
not copied and objects are not deleted. The subfolder stays even when
empty. It is created only if needed after validating the observation and
can remain empty if rename fails. There is no automatic cleanup or
destructive rollback. Removed files disappear from ordinary direct
inventory. The host can retain the ID for requery/restoration or discover
it through the listing below. The GUI does not automatically persist IDs.
Do not manually delete or move that subfolder if recovery through this
API is needed.

## Discovery after restarting the consumer

```cpp
auto backups = pcinsight::list_windows_startup_file_backups();
if (backups.value) {
    for (const auto& item : *backups.value) {
        const auto& row = item.registration;
        // Show item.name (exact UTF-8), entry_id, and state; do not restore automatically.
        // retained_by_pcinsight allows offering restoration after consent.
        // Resolve a conflict without implicitly overwriting anything.
        // auto result = pcinsight::set_windows_startup_file_registered(row, true);
    }
}
```

The listing enumerates only direct regular files in PCInsight.Disabled.v1,
not active entries without backups or other startup mechanisms. It emits
the original direct entry ID, not the backup path, with
retained_by_pcinsight or conflict. A zero-byte file has known size zero.
Conflict rows select no file identity and do not authorize restoration.
The listing does not authenticate backup authors. The host can requery an
ID before offering an action. The action checks identity again, so an old
row does not authorize a new file.

GetFileInformationByHandleEx enumerates the pinned backup handle with
FileIdBothDirectoryRestartInfo/FileIdBothDirectoryInfo. It does not use
FindFirstFile on a path that could point to another folder. Two passes
must match in names, identities, and sizes. Files are opened relative to
the same handle to check enumerated identity, detect direct-file conflicts,
and check presence again. The Known Folder root is revalidated at the end.
Detected changes leave the Metric without a value, not a truncated or
fabricated empty list. Two matching observations do not prove an atomic
snapshot, rule out ABA changes, or guarantee stability after return.
Support is always Support::estimated.

If no backup exists, a known empty list is returned after checking,
without creating it. A missing or denied root, incompatible backup
type, or enumeration failure leaves no value, with a note and
permission_required when applicable. '.', '..', desktop.ini, directories,
devices, and reparse entries are skipped without traversing or following
their contents. An empty list means no eligible regular files in this
store, not that there are no startup apps or ignored objects.

Limits: a 64 KiB buffer, 10,000 records per pass including skipped
entries, at most 10,001 calls per pass, and 16 MiB of output text (IDs and
names). Names are limited to 255 UTF-16 units and converted strictly.
Record offsets and lengths are checked before reading, and exact duplicates
are rejected. Exceeding a limit fails the entire query rather than
truncating it. Order is deterministic by exact UTF-16 name, not locale
or product identity. Listing installs no dependencies, writes no files,
starts no transactions, and runs no programs.

## Validation and operation

The observation retains the folder's volume, index, and creation time,
and the file's volume, index, creation time, last-write time, and size.
The action reopens and compares them and requires a state consistent
with the requested direction. Folder size and modification timestamps
are not object identifiers: moving files can change them. These are
filesystem tokens, not hashes, SIDs, authorization, signatures, or
permanent identities. A zero object index is not accepted as verifiable
identity. OBJ_CASE_INSENSITIVE is not forced when opening entries in
case-sensitive directories. These tokens do not protect against deliberate
recreation with all the same fields. Stay in the same user and volume
context, requery after changes, and do not reuse old observations or
transfer them to another account.

The known folder is pinned by handle; entries and the subfolder are opened
with NtCreateFile relative to that handle. Final reparse points, devices,
and incompatible types are rejected. A backup junction or final file link
is not followed. Components preceding the known root can be redirected.
This does not establish that the entire path is junction-free or is a
security boundary against someone with equivalent permissions. The action
requires the observed root identity. During the operation, directory
handles do not share delete access and the selected file does not share
write or delete access.

The commit is **one rename without replacement using the file handle**,
through NtSetInformationFile/FileRenameInformation with RootDirectory
pinned to the destination directory. No absolute path is resolved again
at commit. An existing destination is rejected, even if its data matches.
There is no cross-volume copy, hidden second attempt, KTM transaction,
deletion, or path-based MoveFile fallback. Bytes and attributes are
preserved because it is the same file. This does not promise a durable
flush on power loss, exclusion of all writes through existing mappings,
or protection against later changes.

The provider uses functions resolved from existing ntdll with
GetProcAddress, plus Win32 and system Known Folders. It does not install
tools, launch commands, load a driver, request UAC, or bypass ACLs.
Missing functions or an unsupported filesystem cause failure;
availability is not forced. Errors retain the Win32 code translated from
NTSTATUS, and permission issues are distinguished from not found and
conflict. Requery after errors rather than assuming the filesystem operation
or external state is unchanged. The host handles consent and auditing:
the free function does not record sensitive actions on its own.

## Verification

`tests/windows_startup_file_control.cpp` redirects only the Known Folder
root to a unique temporary folder; file and rename operations are
**native**, not an injected model. It does not write to actual Startup.
Tests cover successful removal/restoration, preserved bytes and identity,
fresh backup reads, conflicts without overwrite, mismatched file/folder
tokens, changed file or backup, incompatible sharing, preserved readonly,
a backup folder replaced with a file, an entry becoming a directory,
absence, Unicode, invalid IDs/paths/sizes, and extended prefixes. Arbitrary
ACLs, native junctions, FAT/exFAT/ReFS, MSVC/ARM64/x86, and other machines
lacked native validation in this component. Attribute checks alone do not
establish coverage. No hard timeout is guaranteed for native calls.

The first test incorrectly expected readonly to prevent rename on this
host. Removal/restoration preserved readonly; the library was not changed
to fabricate a permission error. The two folders from those aborted
tests were inspected and cleaned by exact name without recursive deletion.
Final runs clean their own fixture. Initial GetProcAddress cast and
indentation warnings were fixed, and the original log was retained.
Final logs: `work/startup-file-control-*-windows.log`; Linux regressions
and consumers: `work/startup-file-control-*-linux.log`.

Final Release: Windows 123/123 and Linux 108/108, 20 repetitions of the
new Windows test, and both installed consumers rebuilt, relinked, and
run with exit code 0. Final builds had no warnings. Linux tests the
unsupported stub and its own regressions, not the NT provider. The
additional API changes no existing layouts, virtual ABI, or history
serialization. This is not remote CI.

Revision 44 adds listing cases to windows-startup-file-control. Native
successes include discovery without retained IDs, restoration from a
discovered row, conflicts, multiple files and Unicode, zero size, order,
and metadata/subfolder exclusions. A query without a backup does not
create it. The enumeration API is injected only for failures: initial
and late permission denial, odd length, out-of-buffer offsets, invalid
UTF-16, success without a valid record, the 10,000 limit, and a missing
API. None yields a partial list. A private backup is changed between
passes to verify rejection, and a Known Folder context change is simulated.
Those injections do not validate other physical filesystems. Logs:
work/startup-backup-listing-*.log.

Native contract sources:
[NtCreateFile and RootDirectory](https://learn.microsoft.com/en-us/windows/win32/api/winternl/nf-winternl-ntcreatefile),
[FILE_RENAME_INFORMATION](https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/ns-ntifs-_file_rename_information).

Listing: [GetFileInformationByHandleEx](https://learn.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-getfileinformationbyhandleex),
[FILE_ID_BOTH_DIR_INFO](https://learn.microsoft.com/en-us/windows/win32/api/winbase/ns-winbase-file_id_both_dir_info).

## MSVC compatibility update (revision 51)

MSVC 19.51/SDK 26100 x64 now builds and verifies the same provider using
a documented internal ABI declaration for NtSetInformationFile/
RenameInformation, without requiring the WDK or switching to path-based
operations. Native removal and restoration succeeded in a private fixture,
with 20 control repetitions (80 total including other focused tests) and
a 127/127 suite. Real Startup was not changed. See the [evidence and
limits](MSVC_VALIDATION.en.md). The historical MSVC gap above does not
reflect this later validation. ARM64/x86, other filesystems, and other
hardware retain their limits.
