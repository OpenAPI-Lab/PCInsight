# Startup Properties: read-only query

Implemented work: get_startup_properties(StartupEntry.id), exposed by
pcinsight/startup.hpp. The query returns StartupPropertiesResult with
value, ActionError, and message. It does not open an OS dialog or run
or resolve Exec. It lets the GUI build its own Properties panel without
requiring a framework.

Cross-platform extension: source_file_size_bytes is the Metric for the
.desktop file's size and is available on Linux. file_size_bytes remains
a legacy Linux field. Windows Run has no source file and leaves that
Metric absent, not zero. See [Windows Run Properties](WINDOWS_STARTUP_PROPERTIES.en.md).
Rebuild for the 0.x API change.

```cpp
#include <pcinsight/startup.hpp>
auto properties = pcinsight::get_startup_properties(entry.id);
if (properties) {
    // properties.value->entry: state and conditions computed from these bytes
    // properties.value->file_size_bytes: .desktop size, not executable size
    // properties.value->fields: group/key/raw_value, in original order
}
// stale_target: reload inventory rather than silently selecting another app.
```

On Linux, the ID must be an absolute .desktop path, at most 32 KiB
and without NUL, within the caller's XDG directories and effective
under precedence. A system ID masked by a personal copy, even a
malformed one, returns stale_target. A missing entry returns not_found;
an out-of-context ID returns invalid_argument. If inventory access
errors prevent verification of priority, the query returns system_error
without fabricated partial Properties.

Native reads use O_RDONLY/CLOEXEC/NONBLOCK and a regular file capped at
1 MiB, with RAII descriptor cleanup. The query compares dev, inode, size,
mtime, ctime, and the byte count read, including verification of the
path. It revalidates priority and the file after calculating conditions.
A detected change returns stale_target. A symlink to a regular file is
allowed for reading, as in inventory. It does not grant permission to
edit links. FIFOs and directories are not opened as valid apps. Denial
on open returns permission_denied. No files are modified, though a read
can change atime depending on the filesystem.

group/key/raw_value fields preserve order, Desktop Action groups,
localized names such as Name[es], vendor keys, and duplicates the basic
reader does not consider ambiguous. The query does not pick an arbitrary
winner among repeated keys. Values retain Desktop Entry escapes and
are not decoded or parsed as shell. Surrounding spaces and tabs and
the CRLF line delimiter are removed. This is not a lossless copy for
editing and does not include comments. Use the flag editor rather than
reconstructing the file from this list. Limits are 4,096 bytes for
group/key, 32 KiB per value, and 65,536 fields. Lines without equals,
empty keys, invalid structure, or exceeded limits return invalid_argument.

entry contains Name, Exec, and status from the same bytes, with filename
as fallback if Name is absent. Standard conditions and AutostartCondition
use the current native context with the previously documented limits
and optional sources. Fields can display Comment, Icon, Path, Terminal,
localizations, and actions. They do not execute Icon or Path or interpret
relative Exec paths. The query does not automatically translate
localizations or certify full Desktop Entry validity. Publisher stays
empty and impact stays not_measured; they are not inferred from
arbitrary names or vendor metadata. The query does not read the Exec
binary's properties, signature, version, or package data.

This is opt-in and can read two inventories of up to 100,000 entries and
64 MiB each in addition to the selected file. Use it to open or refresh
Properties, not as a required GUI frame poll. It is not a universal
snapshot or compare-and-swap against writers that change and restore
files or directories between checks. It trusts the XDG environment,
ancestor paths, and loader; the host coordinates changes. Filesystems
or native providers can block, with no universal deadline. This is
neither a privileged helper nor an audit of a mutating action. Other
backends return not_supported. Empty, NUL-containing, or oversized
parameters are rejected first. The reader creates no files or threads.

Verification on 2026-10-14: Linux Release passed 27/27 and Windows
Release passed 38/38. The new header was installed, and external
consumers built and ran on both. A private XDG fixture under /tmp checks
that the read-only source stays intact; fields and raw escapes,
localizations, groups, and duplicates; separate flags and eligibility;
personal customization taking precedence; a stale system ID even with
a malformed personal copy; symlink reads; permissions for UID 1000;
nonblocking FIFO behavior; invalid paths; and uncertain priority.
Portable tests cover the parser, limits, and API without a backend.
No real startup configuration was changed and no app was run.
