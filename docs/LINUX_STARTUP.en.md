# Enable or disable XDG startup: editing and reading flags

Completed and verified on 2026-10-14: edit flags in a selected
`.desktop` file with `set_startup_enabled(path, enabled)` and read its
state without confusing the main group with Desktop Action groups.

The previous implementation replaced keys in any group and appended
flags at the end of the file, which could belong to another group.
It also truncated a fixed-name temporary file. These paths were
replaced with a group-aware editor and replacement using an exclusive
temporary file.

## Editing contract

- Only Hidden and X-GNOME-Autostart-enabled in `[Desktop Entry]` change.
  Enabled=true writes Hidden=false/GNOME=true; disabled does the reverse.
- Missing keys are inserted before the next group, not in Desktop Action.
  Exec, other groups, comments, and localized names are unchanged.
  LF/CRLF and untouched lines are preserved; insertion may add a newline.
- Desktop Entry must be the first group and occur exactly once. Duplicate
  flags, NUL, malformed group structure, and exceeded limits are rejected.
  This is not a full Desktop Entry validator.
- The action takes **the file to edit**. It does not automatically create
  a personal override for a system entry or change who uses it. The later
  set_startup_enabled_for_current_user API covers that personal scope;
  see [personal overrides](XDG_OVERRIDE.en.md). Editing a system-wide
  file may affect multiple users when the caller has permission. The GUI
  must select and authorize the target.
- It does not run Exec or other applications, use a shell, or elevate
  privileges.

## File, metadata, and commit

The file limit is 1 MiB. The editor opens the directory and then the
file with openat/O_NOFOLLOW. It rejects final symlinks, hardlinks,
directories, devices, and oversized files. Symlink ancestors are not
rejected; the directory is anchored by its descriptor once opened.
This is not an authorization barrier for a privileged helper with
hostile paths.

An advisory directory lock serializes operations from this implementation.
A concurrent operation receives stale_target. External writers that
ignore the lock are not blocked. Device, inode, size, mtime, and ctime
are compared after reading and before rename, but there is no atomic
compare-and-swap against external changes between the last check and
rename. Use trusted directories and coordinate writers. The editor
does not guarantee absence of all races.

Temporary files use O_EXCL/O_NOFOLLOW in the same directory. A collision
does not truncate or remove an existing temporary file. Readable UID/GID,
mode, and xattrs are preserved; additional inherited attributes not
present on the original are removed. Failure to preserve ownership,
ACLs, a security label, or another attribute aborts the commit. Additional
SELinux or ACL configurations require verifying preservation permissions.
The library does not strip metadata to force success.

Data is written, synced, and closed; the target is revalidated and
renameat is used. Errors detected before rename do not replace the
original. Only the editor's own temporary file is removed. The directory
is then synced. Failure of that last step explicitly reports that
**the flags were already applied**. An error does not always mean no
effects, so do not retry blindly. Final atomicity and durability depend
on the filesystem and device. There is no automatic backup or recovery
of data modified by other processes.

Metadata limits are 64 KiB for xattr names and for each value, and
1 MiB for all values. Attributes changing during the read invalidate
the action. Prior timestamps are not preserved; editing changes
timestamps and inode.

## Reported state

The reader uses only Name, Exec, Hidden, and the GNOME flag in the
main group. Inaccessible, duplicate, or malformed entries are unknown,
not enabled. Hidden=true or GNOME=false means disabled. Valid flags
that do not disable mean enabled as **configuration**, not that the app
actually launched. Invalid boolean values yield unknown.

This original scope did not evaluate TryExec, OnlyShowIn/NotShowIn,
desktop conditions, or session eligibility. Custom directories and
filename precedence were later addressed by [XDG precedence](XDG_PRECEDENCE.en.md)
for the calling process's context. Publisher and startup impact were
still pending at this checkpoint. These closures do not complete all
startup work.

## Completion evidence

Linux Release passed 13/13 and Windows Release passed 25/25. Windows
tests the portable parser, not editing XDG entries. Installation and
the external CMake consumer were verified on Linux and Windows after
the final changes.

Native Linux tests run as a normal user in private directories under
`/tmp`. They cover enabling, disabling, idempotence, consistent state,
CRLF and no newline, Desktop Actions, comments, Exec, localization,
duplicates and limits, symlink and hardlink rejection, permission
denial, concurrent locking, and original preservation on tested errors.
Mode, UID/GID, and a user xattr were preserved. A default ACL with an
additional user on the test directory was not introduced into the
replacement file. The preexisting colliding temporary file was kept
and no editor-created temporary files remained.

No real autostart entries were edited and no startup app was launched.
This verifies editing and reading flags, not graphical login behavior.

Primary references: [XDG Autostart](https://specifications.freedesktop.org/autostart/latest/)
and [Desktop Entry format](https://specifications.freedesktop.org/desktop-entry/latest/basic-format.html).

2026-10-19 update: the declared AppStream publisher is available by
opt-in; see [Linux startup publisher](LINUX_STARTUP_PUBLISHER.en.md). The
original status paragraphs are historical. Publisher is not a verified
signer and does not change flags.
