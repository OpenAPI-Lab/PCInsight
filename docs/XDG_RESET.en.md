# Restore the system startup entry

Completed on 2026-10-14: `reset_startup_for_current_user(entry.id)` removes
only a selected personal copy from autostart and retains a recoverable
backup. This is distinct from enabling: **it withdraws the personal
Name, Exec, and flag customizations from future logins**. It does not
launch apps or stop processes.

```cpp
auto result = actions->reset_startup_for_current_user(personal_entry.id);
// Reload inventory after success or an error stating that reset was applied.
// Retain result.startup_backup_path to offer manual recovery to the user.
```

The action accepts only the absolute personal `.desktop` path in the
caller's XDG context, not a system ID or external path. It rejects a
shared personal/system context. A system counterpart is required: the
first existing file in XDG_CONFIG_DIRS priority order is used, with no
fallthrough from a malformed or inaccessible file. The system file must
be regular, without a final symlink, and at most 1 MiB to read. The
provider validates Desktop Entry and unambiguous flags and revalidates
identity, dev, inode, size, mtime, and ctime. No counterpart yields
not_found: **a personal-only app is not removed**. The resulting state
is the system configuration. This does not force enabled or remove
desktop or TryExec restrictions. Reload inventory to see the result.

The personal directory is opened with O_NOFOLLOW, must be owned by the
effective UID, and must not be group- or world-writable. It uses a
nonblocking flock shared with the other editors. The personal file must
be regular, owned by the caller, have nlink=1, and not be a final symlink.
Data is fsynced and the descriptor and path revalidated before moving.
The personal contents need not be valid: a broken customization can
be backed up to return to a valid counterpart.

`renameat2(RENAME_NOREPLACE)` atomically moves the file within the same
directory to `.pcinsight-reset-<PID>-<counter>.backup`. It does not end
in .desktop, so inventory does not treat it as autostart. At most 128
collisions are allowed. No existing backup is overwritten, followed,
or purged. Inode, contents, ownership, mode, and xattrs are retained;
ctime may change on rename. No directories are created and the system
file is not modified. Unsupported kernel or filesystem behavior yields
not_supported, with no link/unlink fallback. No CLI, external service,
or privilege elevation is used.

After moving, the directory is fsynced. Failure returns system_error
with a **reset applied** message and startup_backup_path populated.
It does not promise rollback or absence of effects. For explicit
recovery, the consumer inspects the backup and moves it back only if
the original destination is still free. No automatic restore or purge
API is included, and tests do not touch real files.

The provider trusts the environment, ancestor paths, and coordination
with external writers. flock is advisory. Checks and rename are not
compare-and-swap against peers with the same access who ignore locks;
there is also no global snapshot of all system directories. Consumers
must coordinate environment and configuration changes and reload after
changes. Do not use this as a security barrier for a privileged helper.
Reading the system file can update atime. The backup retains original
permissions, without promising encryption or hiding it from administrators.
Other backends return not_supported.

AuditedActions records startup_reset_current_user, target, and result
without Exec or backup contents, and preserves startup_backup_path in
ActionResult. The enum is append-only. The expanded 0.x API and result
structure require rebuilding the library, wrappers, and consumers together.

Linux Release passed 23/23 and Windows Release passed 34/34, with installed
external consumers on both. A Linux /tmp fixture uses private
XDG_CONFIG_HOME/DIRS without changing HOME. It verifies native reset,
an intact read-only counterpart, preserved backup inode/mode/owner,
an untouched decoy backup, manual restore, Monitor precedence, and
auditing. It rejects system, relative, or orphan paths, symlinks,
hardlinks, insecure directories, lock conflicts, shared contexts,
and malformed or symlink counterparts without fallthrough. A disabled
lower-priority counterpart remains disabled even when backing up a
broken personal copy. Windows verifies unsupported behavior and auditing,
not an XDG provider.
