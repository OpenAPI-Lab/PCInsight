# Windows startup: known Startup folders

With include_startup_entries=true, Monitor now combines Run registrations
with files in the calling user's and the system's Startup folders. Paths
come from FOLDERID_Startup and FOLDERID_CommonStartup, not hardcoded English
names or environment variables. It does not create folders
(KF_FLAG_DONT_VERIFY, without KF_FLAG_CREATE), change files, or run apps.

## Data and interpretation limits

There is one row per direct file entry that is not a directory, device,
or reparse point. desktop.ini is excluded case-insensitively. Regular
hidden files are retained; being hidden does not mean enabled or disabled.
The provider does not recurse or open file or shortcut contents or targets.
The [Shell Link guide](WINDOWS_SHORTCUT.en.md) describes a separate opt-in read of supported .lnk contents,
never the target; the basic inventory still does not open the file.
Windows resolves the root, which can be redirected or traverse a system
junction. The reparse-point filter applies to entries and does not rule
out redirects in root path components. Enumerating a UNC location can
require network I/O; no timeout is imposed on that native operation.

name is the full filename, not the app's product name. source is its
full UTF-8 path. id is winfolder:user:HEX or winfolder:common:HEX,
with the full UTF-8 path hex-encoded. Identical names in different folders
remain separate. If both folders are redirected to the same destination,
two records can appear for their separate scopes; this does not establish
that they are distinct apps.

command stays empty. A .lnk, .url, or script is not presented as an
executable and no arguments are invented. The provider does not use
ShellExecute, Resolve, shell extensions, shortcut COM, or file extension
associations. publisher is empty, status=unknown, impact=not_measured,
observed metrics are absent, and
impact_scope=windows-startup-folder-registration-only. XDG fields are
unused. The GUI should show that these are observed files with unresolved
targets. Do not treat id/source as authorization to launch or delete.

The provider does not determine effective approval, StartupApproved
state, validity, or execution. Revision 43 adds [explicit file
control](WINDOWS_STARTUP_FILE_CONTROL.en.md) for the current user: removal
and restoration by rename without replacement, with a backup in a
subfolder. This does not change the inventory's unknown status or
implement effective shell approval; at that point there was no backup
listing. Revision 44 adds list_windows_startup_file_backups as a separate
explicit query to discover backups and conflicts and restore after the
host restarts. It is not automatically merged into Monitor or presented
as Windows approval. Scheduled tasks, packaged StartupTask entries,
RunOnce, and other accounts are not included. Shortcut resolution,
publisher, impact, and enable/disable/reset/properties remain pending in
the effective shell sense. [Shell Link support](WINDOWS_SHORTCUT.en.md) provides only saved
metadata for a subset of .lnk files; [Internet Shortcut support](WINDOWS_INTERNET_SHORTCUT.en.md) adds
the saved .url target without navigation. Actual resolution and opening,
and the other sources, remain pending. This is not a universal autorun
inventory.

## Failures and bounds

A missing folder or a folder without matching entries is skipped. Failure
to obtain a known path, permission denial, or interrupted enumeration
throws system_error rather than returning a partial list. Known Folder
errors retain HRESULT; Find errors retain the Win32 code. Failure in
either the Run or folder provider fails the combined sample. An empty list
does not prove there are no startup apps on the machine. The provider
does not elevate, query another user token, or retry silently.

The root path must be an absolute literal path, including supported
extended Win32 prefixes. Device paths, relative paths, wildcards, and
invalid UTF-16 are rejected. Revision 42 fixes a reproduced issue:
std::filesystem::is_absolute wrongly accepted a path with stream syntax
(C:\Fixture:stream). An isolated test of the old predicate confirms that
case. The current runtime already rejected GLOBALROOT, pipes, and incomplete
UNC paths; no prior vulnerability is claimed for those cases. The provider
now requires an absolute drive path (C:\...) or nonempty UNC server and
share, including their extended forms. A prefix alone does not authorize
other namespaces. GLOBALROOT, pipes, Volume GUID namespaces, UNC without
a share, drive-relative paths, and stream colons are rejected. Volume GUID
paths are excluded by conservative policy, not because they are claimed
not to refer to filesystems. The original path is kept for Win32, and
extended UNC/unc is accepted. Rejected paths are not opened or enumerated.
This validates syntax; it does not prevent root junctions or redirection
or authenticate the UNC server.

Limits: 32,765 UTF-16 code units for the root, a name terminated within
cFileName, 10,000 entries per folder including ignored ones, and 16 MiB
of output text. Exact duplicate names, separators or colons in names,
missing paths, incomplete APIs, or exceeded limits fail without truncation.
Name sets are bounded by entry count and native name size. There is no
atomic snapshot or guarantee against concurrent renames or changes; this
does not certify persistent identity.

CoTaskMemFree frees paths even when SHGetKnownFolderPath fails after
allocation; FindClose frees searches even on exceptions. The provider
does not initialize or change the caller's COM apartment. Shell32/UUID/Ole32
are system libraries linked at build time, not packages users must download.
The combined Run and folder output is bounded by both providers: up to
50,000 entries and 32 MiB of text on AMD64, without semantic deduplication
between registry and file entries.

## Verification

Injected tests cover two folders, Unicode, identical names in different
scopes, hidden files, metadata/directory/reparse exclusion, relative,
device, wildcard, oversized, extended, and invalid Unicode paths,
unterminated and duplicate names, permission and late failures, absent
folders, incomplete APIs, entry and payload limits, and cleanup even
when the second folder or allocation fails. The extended-path regression
initially failed and was corrected by normalizing only the root check;
the path passed to Win32 keeps its prefix. Native read-only and combined
Monitor opt-in/opt-out testing was performed with process output hidden.
No real folder was written to, target loaded, or file executed.

2026-10-19, revision 42: the new regression failed before the fix and
passed afterward. New cases check rejection before FindFirstFile and
path cleanup. Positive UNC and extended cases preserve inventory and
cleanup. Windows Release 122/122, 20 repetitions of windows-startup-folders,
and the installed consumer relinked and run with exit code 0. Positive
and negative special-path cases are injected fixtures; the native read
uses only existing known folders. This fix does not change StartupApproved
or implement enable/disable. M05 remains open; the separate
windows-run-control test confirms 6801 with no writes. Logs:
work/startup-root-*-windows.log. This is neither CI nor a native
UNC/Volume GUID test.

Release: Windows 66/66 and Linux 50/50. Installed libraries and headers
and external consumers were rebuilt and run on both. The Windows consumer
accepts both registration types in Monitor. Linux tests its own regressions,
not Known Folders. MSVC, ARM64, x86, and real UNC redirection remain
unverified natively in this revision. The extended path was checked with
an injected API, not on a filesystem with long paths.

Official sources: [SHGetKnownFolderPath](https://learn.microsoft.com/en-us/windows/win32/api/shlobj_core/nf-shlobj_core-shgetknownfolderpath),
[Known Folder IDs](https://learn.microsoft.com/en-us/windows/win32/shell/knownfolderid),
[FindFirstFileW](https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-findfirstfilew).
