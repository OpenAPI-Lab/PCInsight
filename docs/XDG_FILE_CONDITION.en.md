# Startup conditions based on file existence

Verified on 2026-10-14: StartupEntry::autostart_condition_satisfied
evaluates AutostartCondition in the main group of the effective
`.desktop` file. It supports if-exists and unless-exists without external
programs or new dependencies. The result is separate from status and
xdg_standard_eligible. No key yields true, meaning this requirement is
absent, not that the app will launch. GConf is not implemented and yields
unknown. Later optional providers for GSettings and GNOME3-session are
documented in [GSettings](XDG_GSETTINGS.en.md) and
[GNOME session conditions](XDG_SESSION_CONDITION.en.md).

Absolute paths are queried directly. Relative paths use the caller's
configuration directory from the existing XDG resolver, not the autostart
directory or working directory. These semantics were checked against
[GNOME](https://mail.gnome.org/archives/commits-list/2020-June/msg03823.html).
Evaluating a predicate does not establish that the desktop recognizes it,
that all GNOME versions evaluate it the same way, or that the caller is
in the relevant graphical session.

The parser accepts full types with ASCII case-insensitive comparison,
not abbreviated prefixes. A duplicate key, malformed or unreadable
entry, invalid escape, or empty, invalid-control, or oversized operand
yields unknown. Only the main group is read, not Desktop Action groups.
String escapes are decoded. Paths are literal, with no shell, variable
or tilde expansion, globbing, or quote interpretation. Spaces are part
of the filename. The filesystem resolves `..`; the parser does not
normalize it in a way that changes symlink semantics.

Linux uses stat, never opens the candidate. Directories and FIFOs can
exist, symlinks are followed, and dangling links yield false.
ENOENT/ENOTDIR yields false; EACCES/EPERM yields unknown with
permission_required; ELOOP and other failures yield unknown.
unless-exists inverts only known values, not errors. A relative path
without a configuration directory, or an absent provider, yields
unknown. No file is modified or executed.

Each Monitor.sample queries again. An enabled entry can have a false
or unknown condition even if xdg_standard_eligible is true. The provider
does not install watchers or start or stop apps when conditions change.
It does not cover other vendor policies. stat can wait on a remote
filesystem or automount, with no universal timeout or atomic snapshot.
Limits are 1 MiB per entry and 32 KiB per string or resolved path. The
new note counts toward the 64 MiB inventory metadata limit. Other backends
leave the field unsupported.

Linux Release passed 24/24 and Windows Release passed 35/35. Packages
were installed and rebuilt external consumers ran. Windows validates the
parser with an injected provider, not the XDG backend. Linux runs as UID
1000 with /tmp and a private XDG context without changing HOME. Monitor
observes file appearance and disappearance, spaces, directories, valid
and broken symlinks, nonblocking FIFO checks, ELOOP, and permission
denial distinguished from nonexistence. Parser tests cover groups,
duplicates, unknown types, limits, escapes, literal paths, and absent
context or providers. No app was launched and real startup was untouched.

A clean-first build regenerated a Linux object with invalid format.
Windows SCM/PDH tests failed due to access restrictions inside the
sandbox and passed in the approved run outside it. Errors were not
hidden and services were not modified. The new public 0.x field requires
rebuilding the library and consumers together.
