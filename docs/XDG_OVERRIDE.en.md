# Startup changes for the current user only

Completed work: `SystemActions::set_startup_enabled_for_current_user`
creates or edits a personal XDG override without modifying the system
source file. The earlier `set_startup_enabled` action still edits the
selected path directly; its contract was not silently changed. Use the new
variant for the GUI's personal toggle, especially for system inventory entries.

```cpp
auto result = actions->set_startup_enabled_for_current_user(entry.id, false);
// Reload startup_entries: the effective ID becomes the personal copy's path.
// This does not launch or stop processes; it changes future login settings.
```

## Target and precedence

The source must be an absolute .desktop path without NUL and a direct
child of an autostart directory in the process's XDG context. The
destination is `XDG_CONFIG_HOME/autostart/<same filename>` or
HOME/.config/autostart under the existing resolver. Invalid HOME falls
back to the effective account through NSS. A context without a personal
directory or with a personal directory matching a system XDG_CONFIG_DIRS
directory is rejected. Arbitrary sources outside those directories do
not accidentally create a new autostart program.

If a personal destination already exists, its Name, Exec, comments,
actions, and metadata take precedence. Only Hidden and the GNOME flag
are changed by the verified editor. It does not refresh from system or
overwrite customizations, even if the GUI still sends a system source ID.
The system source may have disappeared; an existing personal copy
remains editable in its own context.

If the destination is absent, the provider performs a bounded source read,
validates it with the existing reader, and copies its bytes while editing
main-group flags. It does not run Exec, a shell, or a CLI or elevate
privileges. Disabled writes Hidden=true and GNOME=false; enabled does
the reverse. The personal copy becomes effective by filename and XDG
priority, masking lower-priority copies. Enabled does not remove
OnlyShowIn, NotShowIn, TryExec, or vendor conditions. It is still
configuration and can have xdg_standard_eligible=false or unknown.

## Publication and safety

Files are capped at 1 MiB. The final source is opened with
O_NOFOLLOW/O_NONBLOCK and must be regular. Identity/dev/inode/size/mtime/ctime
are compared during the read and before publication, including the source
path. The final personal directory uses O_NOFOLLOW, must be owned by the
effective UID, and must not be group- or world-writable. A newly created
autostart directory is set to mode 0700. Missing parents may be created,
so a later error can leave empty directories. Existing directory modes
are not changed to force permission.

Temporary files use O_EXCL with at most 128 collisions. A new file uses
mode 0600 and caller/filesystem ownership and GID. The system file's
owner, mode, xattrs, and labels are not copied; this is a user file
with its native creation context. Inherited ACLs are bounded by mode
0600, and uniform SELinux policy across distributions is not assumed.
Existing overrides preserve metadata through the earlier editor.
Symlinks, hardlinks, and destinations owned by another user are
rejected rather than followed to edit another file.

Data and permissions are synced, the temporary file is closed, and
linkat publishes without replacement. A destination that appears
concurrently yields stale_target without truncation or overwrite.
Only the provider's own temporary file is removed, followed by
directory fsync. Failure after publication reports that the override
was applied or the result was partial. Do not retry blindly as though
nothing changed. Filesystems without hardlinks or sufficient permissions
return an explicit error, with no non-atomic fallback. The advisory lock
is shared with the direct editor; readers can still read.

The provider trusts caller HOME/XDG, NSS, and ancestor paths, which
can include symlinks. It is not a sandbox or protection for a root helper
with hostile inputs, and does not automatically select another user's
graphical session. The consumer must coordinate environment changes and
external writers that ignore flock. Checks and publication are not a
universal compare-and-swap against hostile directory or temporary-file
replacements by a peer with the same access. Mode 0600 does not hide data
from an administrator. Reading the system source may update atime; its
content, mode, inode, and mtime are not written. Personal copies do not
automatically receive later system-package updates. See the separate
[reset action](XDG_RESET.en.md).

## Auditing and verification

On 2026-10-14, Linux Release passed 22/22 and Windows Release passed 33/33.
Both packages were installed and rebuilt external consumers ran.

AuditedActions records startup_current_user, source, and enabled, and
preserves errors and results without retaining the file's Exec payload.
The destination appears in a successful ActionResult and is available
from subsequent inventory. The 0.x virtual API and audit enum expanded;
rebuild the library, wrappers, and consumers together. Other backends
inherit not_supported and do not emulate XDG on Windows.

Portable tests cover destination and context resolution, invalid paths,
and conflicting roles. Native Linux tests use **only the test process's**
XDG environment pointing to /tmp. They cover an intact read-only source,
initial setup with directory mode 0700 and file mode 0600, flags and
precedence observed by Monitor, an enabled entry still excluded from
GNOME by OnlyShowIn, preserved custom Exec, rejected symlinks and
hardlinks, concurrent locking, retention of a preexisting temporary
file, no leftover temporary files, rejected insecure directory and shared
context, and a malformed source without creating configuration. Auditing
was verified. No real autostart was edited, no app launched, and HOME
was not changed.

Reference: [XDG personal overrides and Hidden](https://specifications.freedesktop.org/autostart/latest/).
