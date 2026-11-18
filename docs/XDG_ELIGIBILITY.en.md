# Standard XDG startup conditions

Completed work: `StartupEntry::xdg_standard_eligible` is a Metric<bool>
independent of `status` (enabled/disabled flags). Linux inventory
evaluates it in the calling process's context. It does not run TryExec
or Exec.

An enabled entry can be ineligible because OnlyShowIn excludes the
current desktop. Changing enablement flags does not remove those
conditions. Missing eligibility is not false; the GUI should display
unknown and the Metric's note.

## Criteria evaluated

- Hidden=true yields false. Hidden=true tombstones still mask lower
  files even without complete app metadata.
- A Type other than Application yields false. Missing required Type
  or Name, or missing Exec without DBusActivatable=true, yields
  unknown. The complete Exec grammar is not validated.
- OnlyShowIn and NotShowIn are read from the main group, never a Desktop
  Action. Both present or duplicates yield unknown. Comparison is
  case-sensitive, with `;`-separated lists, string/list escapes, and
  `:`-separated XDG_CURRENT_DESKTOP.
- With desktop conditions, a missing or empty context yields unknown;
  GNOME or KDE is not assumed. A known condition excluding the caller
  yields false.
- A nonempty TryExec is an absolute path or a simple name searched in
  PATH. It is not parsed as a command or split into arguments. String
  escapes are decoded. A relative path containing `/` is invalid and
  unknown. A semicolon belongs to the filename, not shell code.
  `$VAR` and `~` are not expanded.

Native reads check for a regular file and faccessat(X_OK, AT_EACCESS)
under effective credentials. A symlink to an executable file can
satisfy TryExec; a directory or lack of access or permission cannot.
This is an access check, not validation of ELF or script format or
a successful exec guarantee. Files can change after the query.

PATH is captured for each inventory evaluation. Empty or relative
components are resolved against the captured working directory.
Without PATH, the reader does not invent default directories.
Inconclusive failures yield unknown. An accessible candidate can
yield true even if another directory could not be evaluated. A known
false condition can decide the result even if desktop context is
missing. An unknown TryExec preserves its support and note.

Limits: 1 MiB per file using the existing reader, 32 KiB decoded
strings, 4,096 elements per list or PATH, and 32 KiB of desktop
context. Oversized or malformed inputs are not presented as eligible.
List lookups use sets rather than comparing all identifier combinations.

## Exact scope

True means that the **checked criteria** pass in this context, not
that the app will actually launch or is running. The provider does not
evaluate special GNOME/KDE conditions, desktop policy, availability of
D-Bus services, NoDisplay as a menu filter, complete Desktop Entry or
Exec validity, or startup impact. NoDisplay alone does not disable
autostart. X-GNOME-Autostart-enabled=false still affects the existing
configuration status, but is not a universal XDG exclusion. Other
desktops can ignore that extension. The new field covers only standard
criteria.

The later autostart_condition_satisfied API separately evaluates vendor
file-existence conditions; see [file conditions](XDG_FILE_CONDITION.en.md).
It does not change the xdg_standard_eligible contract.

Process environment and credentials can differ from the graphical
session's. WSL without XDG_CURRENT_DESKTOP does not establish GNOME
conditions from the distribution name. Collection does not elevate
or change the consumer's environment. Other platforms leave the
new field unsupported when their provider does not calculate it.
This is a new field in a public 0.x struct; rebuild the library and
consumers with compatible headers.

## Completion verification

On 2026-10-14, Linux Release passed 15/15 and Windows passed 27/27.
Windows tests portable logic with injected context and queries, not
native Linux permissions or desktops. Both Release packages were
installed and rebuilt external consumers ran after the final GNOME
extension fix.

Tests cover inclusion and exclusion, multiple desktops, case, empty
fields, escapes, invalid lists, duplicates or both fields, ignored
conditions in other groups, missing context, disabled state, unknown
queries, and preserved TryExec parameters. Linux creates private
files with and without X_OK and checks absolute paths, PATH, missing
files, directories, and invalid relative paths without running candidates.

Full Monitor integration uses the test process's private environment
and temporary autostart. It keeps status=enabled and
xdg_standard_eligible=false when querying OnlyShowIn=KDE with
XDG_CURRENT_DESKTOP=GNOME. Test environments are restored afterward.
No real startup entry is changed and no app is launched for this test.

Primary references: [XDG Autostart conditions](https://specifications.freedesktop.org/autostart/latest/),
[Desktop Entry keys](https://specifications.freedesktop.org/desktop-entry/latest/recognized-keys.html),
and [value types and escapes](https://specifications.freedesktop.org/desktop-entry/latest/value-types.html).
