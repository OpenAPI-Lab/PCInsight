# Declared publisher for Linux startup entries

I05 provides an optional AppStream provider integrated into Monitor.
It does not run programs, package managers, appstreamcli, shells, or
external utilities. It does not install software or download metadata.
The library still works without libxml2.

## Usage and meaning

```cpp
#include <pcinsight/monitor.hpp>
#include <pcinsight/linux_startup_publisher.hpp>

pcinsight::Monitor monitor;
pcinsight::SampleOptions options;
options.include_startup_entries = true;
options.include_linux_startup_publisher = true; // Separate opt-in.
auto snapshot = monitor.sample(options);
for (const auto& entry : snapshot.startup_entries) {
    // Check entry.publisher_label.value and show support/note.
    // publisher is only shorthand; an empty string does not mean no publisher exists.
}

// Explicit alternative: query and apply to inventory the GUI already holds.
auto declared = pcinsight::read_linux_startup_publishers();
pcinsight::apply_linux_startup_publishers(snapshot.startup_entries, declared);
```

The only join is an exact, case-sensitive match between the effective XDG
entry's basename and an AppStream launchable of type desktop-id. Name,
Exec, PID, username, package name, and XML filename are not matching keys.
The provider does not run or resolve the command to establish who published
it. An unknown or malformed entry receives no label from such a match.
A valid disabled entry can still have a declared publisher.

The result is estimated. developer_label is the declared developer or team,
not necessarily a publishing company, signer, or owner of the current
executable. A customized .desktop file can keep its basename and change
Exec. The label does not authenticate that entry. publisher_source_executable
remains absent on Linux. publisher_label.note and LinuxStartupPublisher
retain component and source information. Do not use this data to authorize
execution, elevation, or trust in a binary.

The reader accepts modern developer/name and legacy developer_name. It selects
only the default form without xml:lang, accounting for inherited language.
Translations are not chosen implicitly by locale. Duplicate, empty, or
control-containing labels, or conflicting modern and legacy labels, are
rejected. Identities and labels are bounded to 4,096 B. This is not a full
AppStream schema validator.

## Sources, conditions, and missing data

The provider resolves the calling process's XDG_DATA_HOME (fallback
HOME/.local/share) and XDG_DATA_DIRS (fallback /usr/local/share:/usr/share).
An absent or invalid HOME falls back to geteuid/passwd. It does not obtain
another session's environment. Relative or empty components are ignored;
normalized directories are deduplicated. Each base is checked for metainfo
and legacy appdata directories and .metainfo.xml/.appdata.xml extensions.
This is PCInsight's explicit source policy, not a promise to reproduce
every AppStream cache or precedence rule.

Each XML document must contain a single upstream component, without a
namespace, of type desktop-application. The provider does not read components
catalogs, DEP-11 YAML, compressed files, caches, remote data, or internal
package-manager databases. XML with namespaces, DTDs, entity declarations,
or processing instructions yields absence, not an inferred publisher.
Other component types contribute no rows. Flatpak or Snap is visible only
when it publishes regular metadata files in the queried directories with
an explicit match. Private installations are not traversed, and symlink
exports are not followed as fallback.

Supported startup conditions remain separate from publisher: flags,
OnlyShowIn/NotShowIn/TryExec, and AutostartCondition based on files, GSettings,
or a GNOME3 session. See [XDG eligibility](XDG_ELIGIBILITY.en.md),
[file conditions](XDG_FILE_CONDITION.en.md), [GSettings](XDG_GSETTINGS.en.md),
and [GNOME session conditions](XDG_SESSION_CONDITION.en.md). The provider
does not invent support for new vendor conditions. Other startup mechanisms
(cron @reboot, shell profiles, KDE session scripts, jobs from other init
systems, user units, or custom extensions) are not treated as equivalent
XDG apps. Services belong to K03/K04 inventory; automatic attribution and
early impact capture remain under I07. This work extends metadata and
documents absence. It does not close those tasks or claim universal
inventory of Linux startup mechanisms.

When multiple documents declare the same desktop-id, the overlay does not
pick a winner or take the first publisher. The match stays unknown. A scan
with parsing, reading, or limit errors rejects the whole report rather than
publishing a partial set as unique. Under this conservative policy, an
unrelated defective file can leave the whole inventory without labels for
that sample. A missing directory is a source with no documents. EACCES
and EPERM retain permission_required. An empty list describes only these
sources and does not prove apps have no publisher. An old label is not
retained when the new report is absent, ambiguous, or invalid.

## XML library and read limits

The provider dlopens the system's libxml2.so.2, falling back to libxml2.so,
only when queried. It does not require development headers or packages at
build time. Missing library or symbols yields unsupported rather than
installing a replacement. The host manages its library and trusted loader
paths. PCInsight does not change callbacks or global options or call
xmlCleanupParser on another user's library state.

Input is bounded UTF-8 without NUL. DTD and ENTITY are rejected before
parsing. The in-memory reader uses no base URL and sets NONET, NOERROR,
and NOWARNING. It does not enable NOENT, DTDLOAD, DTDATTR, DTDVALID,
XINCLUDE, HUGE, or RECOVER. It does not follow release, screenshot, icon,
or include URLs. PCInsight imposes its own limits and guards; parser
correctness and maintenance depend on the system library.

Limits are 128 directories, 8,192 visited entries, 1 MiB per file, 16 MiB
of input bytes, 16 MiB of output strings, 8,192 rows, 100,000 nodes per
file, and depth 64. These are not an exact RSS cap or syscall deadline.
There is no persistent cache; each query resamples. The loader is resolved
once per process.

Directories are pinned by fd. Files are opened with openat/O_NOFOLLOW/
O_NONBLOCK and must be regular. A final symlink is not followed, and a
FIFO does not block. Relative fd and pathname metadata are revalidated:
dev, inode, mode, nlink, size, mtime, and ctime. Enumeration is not atomic.
Additions, removals, directory or ancestor replacement, and ABA changes
can go undetected. Estimated is not an identity guarantee. This is not
a sandbox for elevated helpers with hostile paths. Query in the user's
context and coordinate changes to the process's own environ.

## Compatibility and verification

The additional API is in linux_startup_publisher.hpp. The new flag at the
end of SampleOptions in development version 0.2 changes its layout, so
rebuild **all** consumers. StartupEntry layout is unchanged: the label
Metric and note are reused. The virtual ABI and history format are
unchanged. Windows retains VERSIONINFO/Authenticode and does not query
AppStream implicitly.

tests/linux_startup_publisher.cpp keeps assertions active in Release.
Private fixtures cover modern and legacy XML, predefined and numeric
entities, CDATA, inherited language, duplicates and conflicts, invalid
XML, rejection of DTD/XXE, limits, permissions, symlinks, and FIFOs.
The public scan and Monitor opt-in/opt-out use the test process's private
XDG context. No app is started and real autostart is not changed.
A separate XML-library test with incomplete symbols yields unsupported
without scanning or producing labels. Windows tests portable joins and
context and provider absence, not native Linux XML collection. If
libxml2 is missing, the positive XML test explicitly exits 77; a skip
is not counted as native success.

On 2026-10-19, native Ubuntu WSL with already installed libxml2 passed
the public query and Monitor tests on real private regular files, with
exit 0 rather than a skip. Linux Release passed 104/104 and Windows
passed 117/117. Final builds had no warnings. Packages were reinstalled,
and rebuilt, relinked external consumers on both systems exited 0 with
the new header and API. Logs: work/startup-publisher-* (native-linux
and build/suite/install/consumer per platform). A preexisting duplicate
NOMINMAX warning in tests/windows_app_history.cpp was fixed with a
macro guard without changing test logic or assertions.

I05 completion covers the expanded modern and legacy AppStream sources,
explicit source and unsupported-condition policy, positive public
verification, and rejections. It does not change I07/K03 criteria or
claim universal vendor support. Historical N03 risks remain open; passing
suites do not establish their cause.

Primary sources: [AppStream upstream developer and launchable metadata](https://www.freedesktop.org/software/appstream/docs/chap-Metadata.html),
[libxml2 XMLReader](https://gnome.pages.gitlab.gnome.org/libxml2/html/xmlreader_8h.html),
and [parser options](https://gnome.pages.gitlab.gnome.org/libxml2/html/parser_8h.html).
