# SysV: LSB descriptions without executing scripts

Revision 138/K03. Public API in `pcinsight/linux_sysv.hpp`:

```cpp
auto info = pcinsight::read_linux_sysv_lsb_metadata("/etc/init.d/ejemplo");
if (info.value) {
    // short_description and description have independent availability.
    // These are declared comments, not authenticated publisher/state/process data.
}
```

`parse_linux_sysv_lsb_metadata(text, input_complete)` is pure. The native
reader selects an explicit regular file, reads a bounded prefix twice, and
verifies bytes, metadata, and context before parsing. It does not invoke
a shell, CLI, exec, variable expansion, status/start/stop commands, or
subprocesses, and does not install anything or modify the file. Reading
the prefix can include body bytes, but **they are not evaluated** and
descriptions are not taken from executable code. The revision 137 catalog
still reads only filesystem metadata, with no automatic content reads.

## Supported comment subset

The parser looks for one `### BEGIN INIT INFO`/`### END INIT INFO` block
in the leading comment preamble. Delimiters must start at column zero;
trailing spaces/tabs and CRLF are allowed. Blank lines and comments,
including indented comments, can appear before or after. The first
non-comment text ends the preamble. Headers are not discovered in heredocs,
strings, or code beyond that point. A header after a command such as set or
source is not guessed to be metadata.

Inside the block, comments start at column zero and keywords are preceded
by one space after #. Only `Short-Description` and `Description` produce
fields. Other syntactically recognizable keywords are ignored:
Provides/Required/Default-Start/Stop, extensions, and startup policy are
NOT evaluated. Description continuations start with # followed by a tab
or at least two spaces, and are joined with a space. Orphaned text
continuations or continuations of another keyword are not accepted. Blank
continuation comments do not erase Description. Spaces/tabs in values are
normalized; quotes, variables, backticks, $(...), and other visible text
characters are not interpreted.

Duplicate description fields, duplicate or nested blocks in the preamble,
an unmatched end, an incomplete block, non-comment lines inside, invalid
text, or exceeded bounds yield UNKNOWN without arbitrarily picking a value.
Description values must be strict UTF-8 scalar values with no C0/DEL/C1;
corrupt bytes are not repaired. Values of ignored keywords are not published.
Missing or empty fields remain individually UNKNOWN. A valid block without
description fields can yield available info with both fields UNKNOWN. There
is no fallback to a name or shell variable.

Limits: 64 KiB input/prefix, 1,024 preamble lines, 4,096 bytes per line,
1,024 bytes per description line's value after removing indentation and
trimming, and 4,096 bytes for joined Description including separators.
Excess input is not silently truncated. The reader accepts files larger than
64 KiB if a valid block and the beginning of code are both visible in the
prefix. A truncated prefix still containing only comments is not treated
as EOF even after an end marker: another block could follow. This is a
conservative subset, not a full LSB validator or shell parser.

## Identity, I/O, and scope

Paths must be absolute, contain no NUL, and be at most 32,768 bytes. The
reader uses openat2 NO_MAGICLINKS/O_PATH and allows ordinary aliases.
fstat confirms a regular file before reopening ONLY its own FD through
/proc/self/fd for NONBLOCK/CLOEXEC reading. FIFO, device, and directory
targets are rejected without opening them for I/O. It uses pread twice
from offset 0 up to min(size, 64 KiB). Early EOF, errors, differing bytes,
or changed mode/size/dev/inode/mtime/ctime metadata yield UNKNOWN. It
revalidates the original path and boot/procfs/PID/start/UID/namespaces/eUID
context. Permission denial is distinguished and tested only as non-root.
There is no cache, worker, timer, atomic snapshot or ABA guarantee, or hard
I/O deadline. The host serializes fork/setns/credentials/FD-context changes.

This is a fresh observation of the given path, NOT a guaranteed association
with an earlier catalog index. It does not merge with old snapshots by
basename. Comments can be false or stale. Available means declared text was
read and validated, not verified publisher, a live daemon, a description
of an in-memory binary, or enablement. No PID, current service, Windows
group, effective defaults, or control capability is added.

## Evidence and remaining work

`tests/linux_sysv_lsb.cpp` covers format, CRLF, continuations, missing and
empty values, literal shell text and Unicode, invalid UTF-8/C0/C1,
duplicate and nested blocks, heredoc/body text not treated as metadata,
truncated input, 64 KiB/1,024-line/4,096-byte line/1,024-byte value limits, and
a joined description of exactly 4,096 bytes versus excess. The first fixture
failed because it counted indentation toward the 1,024-byte value limit.
It was corrected and the initial logs were kept, without attributing the
failure to an external cause.

Private native tests cover a file larger than 64 KiB with a sentinel to
detect execution, aliases, missing files without creation, FIFO/directory/
magic link, truncated headers, permissions when non-root, and no FD leaks.
No real script, init, or service is run. MSVC tests the parser and fallback;
the SDK consumer links the new symbols. No real SysV acceptance or fixtures
for every race are claimed. HISTORY/formats, Monitor, and existing models
are unchanged.

At this revision, K03 remains open for projection/integration, control/
liveness, and real acceptance. Revision 139 adds an opt-in
[described catalog](LINUX_SYSV_DESCRIBED_CATALOG.en.md) tied to the pinned
candidate within the same query. It does not change this fresh-read API
or interpret defaults as enablement. At this checkpoint, OpenRC revision
132 and SysV control scripts still had no exception. The I07 observer is
unchanged.

Primary reference:
[LSB Comment Conventions](https://refspecs.linuxfoundation.org/LSB_5.0.0/LSB-Core-generic/LSB-Core-generic/initscrcomconv.html).
The example script is not copied, and no LSB installation tools are run.
