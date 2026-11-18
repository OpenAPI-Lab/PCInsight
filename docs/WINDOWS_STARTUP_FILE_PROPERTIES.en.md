# Windows Startup source-file properties

`get_startup_properties(entry.id)` now accepts `winfolder:user:...` and
`winfolder:common:...` as well as Run entries. It returns data about the
**Startup file**, not the executable it might point to. This explicit
read-only query does not open system UI, use shell or COM, resolve targets,
load icons, query signatures or CompanyName, expand variables, launch
anything, elevate privileges, or change enablement.

## Identity and fields

The query selects the exact ID from a fresh inventory of the caller's
known Startup folders. It does not decode an ID as permission to open
an arbitrary path. It checks scope, canonical hex, and exact ID/source
matching. An unknown ID yields not_found. A different folder or user
context can invalidate it. The inventory is queried again before
completion; an absent or changed entry yields stale_target. Inventory
failure is not mistaken for an empty folder.

`StartupProperties.source_file_size_bytes` returns the observed logical
size, including zero, not the target size or physical space used. The
legacy file_size_bytes remains Linux-only. fields has a
`Windows Startup file` group: path, scope, size_bytes, attributes_raw,
creation_filetime_100ns, and last_write_filetime_100ns. Attributes are the
Win32 bitmask in decimal. Dates are the raw reported FILETIME values
(100 ns intervals since 1601), not a guarantee of uniform precision or
availability across filesystems. Last access is not returned because
reading the file can update it.

The entry retains its current name/path/ID, empty command, unknown
status, and not_measured impact. Registration does not imply approved
autorun or authorized action. The function does not reuse optional
metadata from another sample or handle.

## Saved shortcut metadata

For `.lnk` and `.url` files up to 1 MiB, it reads bytes through the same
handle used to query properties and fills `entry.windows_shortcut` with
the existing [Shell Link](WINDOWS_SHORTCUT.en.md) and
[InternetShortcut](WINDOWS_INTERNET_SHORTCUT.en.md) parsers. Properties is
an explicit query and does not require Monitor's SampleOptions flags.
An invalid or unsupported format leaves windows_shortcut without a value
and with a note; valid file properties are retained. Files larger than
1 MiB retain size, attributes, and dates without reading content. Other
types never have their content read. Target metadata, arguments, and URLs
are untrusted text, not execution or navigation permission or proof of
a current target. The target is not opened.

## Consistency, errors, and limits

CreateFileW uses GENERIC_READ, FILE_SHARE_READ, OPEN_EXISTING, and
FILE_FLAG_OPEN_REPARSE_POINT. A disk file is required; final-component
reparse points, directories, and devices are rejected. Data-read permission
is required even if the file type does not need a content read. Restrictive
sharing can fail if a writer is already open. The query compares metadata
before and after (attributes, size, creation/write times, and the volume/
64-bit file-index pair). It then reopens the path and compares again while
keeping the first handle open. RAII closes handles on success and errors.

This detects changes but is not an atomic snapshot, historical or future
identity, universal immutability, or a control token. The 64-bit file
index depends on the filesystem and is not a guaranteed unique identity
on ReFS; it is not authorization. The query cannot detect every ABA change
or a change with identical metadata. The host can replace the file after
the query. A redirected known folder, mapped drive, or intermediate reparse
point can cause network I/O; local-only storage is not promised.

Access denied yields permission_denied; an initially absent file or ID
yields not_found. Detected changes, a short read, or a path that cannot
be revalidated yield stale_target. Unclassified native errors yield
system_error with a code. Inconsistent syntax or identity yields
invalid_argument; an incomplete API or non-disk type yields not_supported.
A content I/O failure does not return a partially refreshed result.
An unsupported shortcut format, by contrast, is not an I/O failure.

Limits: 20,000 inventory rows per selection query (two observations),
262,160 B for the ID, 131,068 UTF-8 bytes and 32,765 wchar_t units for
source, and 32,768 B for the name. The native inventory retains its own
folder and text limits. Content is limited to 1 MiB, reads to 64 KiB
each, and ReadFile calls to 256. Large-file metadata can return 64-bit
sizes without reading content. There is no hard API deadline or guarantee
of blocking every filesystem change. Consumers of the 0.x API must
rebuild. No new runtime dependency.

## Verification

2026-10-15: Windows 72/72 and Linux 54/54 Release; installed consumers
ran with exit code 0. On Windows, the public query read three real
Startup entries. A private temporary `.url` fixture was read through
the native file provider with injected inventory, without adding it to
Startup or opening its URL. Only that fixture was removed afterward.
Injected tests cover 64-bit sizes and no reads for other types, invalid
parsing, large files, permissions and I/O, reparse points, directories,
devices, pipe types, changes, disappearance, replacement, duplicate or
failed inventory, IDs, read-call limits, and resource cleanup. Linux
tests XDG regressions, not these Win32 APIs. MSVC, more hosts, filesystems,
and network paths need further validation.

Official sources:
[GetFileInformationByHandle](https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-getfileinformationbyhandle),
[BY_HANDLE_FILE_INFORMATION identity and date limits](https://learn.microsoft.com/en-us/windows/win32/api/fileapi/ns-fileapi-by_handle_file_information),
[CreateFileW and flags](https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-createfilew).
