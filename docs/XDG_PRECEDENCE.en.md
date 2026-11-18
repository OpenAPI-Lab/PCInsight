# XDG startup directories and effective overrides

Completed on 2026-10-14: resolve XDG directories and choose one entry
per filename according to precedence in the process's context.
`Monitor::sample(include_startup_entries=true)` uses this native Linux
resolver.

## Context and precedence

The resolver reads the caller's environment: XDG_CONFIG_HOME for personal
configuration and XDG_CONFIG_DIRS for system directories, in order.
Only absolute paths are used; relative or empty components are ignored.
Without a valid configuration home, it uses HOME/.config. If HOME is
invalid or missing, it falls back to the home for geteuid through
passwd/NSS. With no valid system directory, it uses /etc/xdg.
It appends /autostart and removes duplicate paths without changing priority.

The first entry with a given `.desktop` filename wins. A personal copy
with Hidden=true masks system copies, which do not reappear as enabled.
An inaccessible or malformed winning file remains unknown; the resolver
does not fall back to a lower-priority copy as if the override did not
exist. Linux filenames are case-sensitive. Entries are not deduplicated
by Name or Exec command.

`StartupEntry.id` remains the selected path, compatible with the action
that edits a specific file. `source` indicates whether the effective
entry is personal or system and adds a diagnostic when precedence or
readability cannot be established. Output order is stable by filename.

**Scope change:** system profiles are no longer mixed indiscriminately.
The result describes this process's XDG context, not the assumed
environment of every user or necessarily the graphical session if
Monitor runs in an elevated helper. The consumer must run it in the
context it wants to inspect. Other sessions' environments are neither
captured nor changed. The host must coordinate updates to its own
environ and avoid changing it while it is being read.

## Errors and limits

A missing directory does not stop the resolver from checking later
directories. Other enumeration errors invalidate confidence in
lower-priority entries, leaving them unknown rather than enabled.
Winning entries already read from higher-priority directories remain
valid. Internal enumeration diagnostics are retained.

Directories, FIFOs, and other nonregular objects are not app entries.
Linux reads use O_NONBLOCK and verify the descriptor type to avoid
blocking if a path changes to a FIFO. Symlinks to regular files can
be read. Broken symlinks keep an unknown entry and do not activate a
lower-priority copy. This is not a sandbox for reading manipulated
paths from a privileged helper.

Limits are 64 directories, 32 KiB paths, 100,000 inspected entries,
and 1 MiB files. Output metadata is capped at 64 MiB of string bytes,
not RSS. Exceeding the limit raises an error instead of publishing
an unbounded payload. Oversized directory environment settings also
raise errors. Sampler catches these exceptions. The filesystem
snapshot is not atomic.

`XdgStartupInventory.complete` is internal and reports only enumeration,
not validity of every file. The current public API exposes no global
diagnostic if no directory can be listed. An empty list does not prove
complete coverage. Returned entries preserve uncertainty in their states.

## Completion criteria and evidence

Linux Release passed 14/14 and Windows Release passed 26/26. Fixtures
cover defaults and customization, relative paths, duplicates, priority
among system directories, personal Hidden entries, malformed or
inaccessible overrides, missing or nondirectory paths, limits,
directory and file permissions, broken symlinks, and nonblocking FIFOs.

Normal Linux integration found three effective native entries without
duplicate filenames. Another test changes **only** the test process's
XDG_CONFIG_HOME and XDG_CONFIG_DIRS to its own temporary directories.
Monitor returns only the disabled personal override and hides the
system copy. The environment is restored afterward. No real autostart
files or system configuration were edited. The false/true/false sequence
for include_startup_entries was verified. Packages were installed and
an external CMake consumer ran successfully on both systems after the
final changes.

This completion does not evaluate OnlyShowIn, NotShowIn, TryExec,
desktop-specific conditions, publisher, startup impact, or personal
override creation through enable/disable. Status describes flag
configuration, not proof that the app will run at the next login.

Later update: OnlyShowIn, NotShowIn, and TryExec were implemented as a
separate field in [XDG eligibility](XDG_ELIGIBILITY.en.md).
Desktop-specific conditions, publisher, impact, and override creation
were still pending at that checkpoint.

Primary references: [XDG Autostart precedence](https://specifications.freedesktop.org/autostart/latest/)
and [XDG base directories and variables](https://specifications.freedesktop.org/basedir/latest/).
