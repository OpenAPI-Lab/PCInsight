# Linux process descriptions and detail availability

`ProcessInfo.linux_details` exposes a `Metric` for name, state, username,
real UID, executable, architecture, description, and description source.
Each field keeps its optional value, support, and note. The six legacy
strings mirror their Metrics or stay empty when missing. They do not
replace checking availability. On Windows the Linux report remains
unsupported without reinterpreting Windows data. CPU, RSS, and I/O
semantics are unchanged, and this does not close A01 for the entire API.

## Description provenance

The source is the unlocalized `Comment` of a `.desktop` file, normalized as
short UTF-8 text with valid scalar values and no controls. Name,
GenericName, kernel name, ELF, and basename are not used as fabricated
descriptions. This is not signed metadata or proof of commercial identity
or publisher.

The reader enumerates XDG_DATA_HOME/applications (falling back to
HOME/.local/share/applications) and XDG_DATA_DIRS/applications (default
/usr/local/share:/usr/share). Relative IDs replace separators with hyphens;
the first directory takes precedence, and ambiguous collisions mask lower
entries. A higher-priority hidden, invalid, or unreadable entry does not
silently reactivate a lower-priority copy. Symlink directories are not
traversed. No programs are run or data installed.

The `Exec` program is matched to the D03-pinned executable through
dev, inode, size, mode, mtime, and ctime, not name similarity. The reader
accepts absolute programs and simple names resolved in the caller's PATH.
Relative or empty components before resolution fail closed. The caller's
PATH may differ from the original session's, so attribution remains
estimated, not a reconstruction of how the program launched. The sampled
executable pathname is not reopened to determine architecture.

The conservative Exec subset accepts a bare program or one in double quotes
without escapes, followed by zero or one standalone %f/%F/%u/%U code.
Wrappers, literal arguments or options, complex escapes, D-Bus activation,
other codes, and unsupported formats do not produce associations. The
reader does not guess script, terminal, or launcher identity from arguments.
It is not a launch parser.

One admissible match can produce an estimated description. Two distinct
admissible catalog entries for the same file are ambiguous even if their
text agrees. A source that changes or disappears after indexing leaves
the result absent. No admissible match, no Comment, or a localized-only
Comment yields unknown with a note, not placeholder text or another
executable's description. Individual unreadable or unsupported files are
discarded. The result describes only the accessible subset, not a complete
catalog of all apps.

## Cost and limits

`SampleOptions.include_linux_process_descriptions` defaults to true.
False skips the entire scan and returns unsupported description and source
with a disabled note, without clearing other details. When processes are
needed, including for users or history, the reader builds one index per
sample rather than scanning per process. There is no permanent cache,
download, command-line invocation, or required auxiliary service.

Limits: 64 roots, 16,384 examined entries, depth 8, 4,096 IDs, 64 KiB per
file, 16 MiB total content, 4,096 lines, 8,192 B per line, and 1,024 B per
comment. Environment variables are limited to 32,768 B and PATH to 128
components when resolving a program. Exceeding a catalog limit or hitting
an enumeration error invalidates the whole index. An individually
unsupported file is skipped while retaining its precedence. Metadata
files are pinned with O_PATH, verified regular before reading, and read
through their own descriptors with metadata checks before and after.
EINTR gets at most three retries. UTF-8 handling reuses cpu_identity_text
validation. The autostart reader is not reused to infer application comments
or IDs under different rules.

Checks are sequential, not an atomic read of catalog, process, and
executable. A-to-B-to-A changes and changes invisible to metadata are not
ruled out. Someone can write false comments in their own files, so the GUI
must retain estimated support and show the source. Do not use associations
for authorization. Byte limits do not guarantee a filesystem or NSS
deadline. The caller must not mutate its environment while the library
reads it.

## Verification

The linux-process-metadata test covers subset parsing, duplicates, flags,
wrappers, UTF-8 and escapes, and limits. The public report retains
permission denial, absence, and UID 0, clears stale legacy values, and
does not inherit defaults. Private Linux fixtures cover a positive Comment
for the test's own executable, Hidden precedence, ID collisions, multiple
matches, source changes, PATH, and Monitor opt-out. Native WSL ran under
PID 2083; no installed applications or accounts were modified. Positive
metadata is created in a private catalog, not evidence of a complete
commercial application inventory across distributions.

The public 0.x layout changed. Rebuild the library and **all** consumers
with new headers and the same compiler and runtime. Do not mix old objects
or assume stable ABI. See [installation and compatibility](PACKAGING.en.md) and
[Linux process details](LINUX_PROCESS_DETAILS.en.md).

Primary format references (the supported subset is this provider's policy):
[Exec](https://specifications.freedesktop.org/desktop-entry/latest/exec-variables.html),
[Comment and Hidden](https://specifications.freedesktop.org/desktop-entry/latest/recognized-keys.html),
and [IDs and precedence](https://specifications.freedesktop.org/desktop-entry/latest/file-naming.html).
