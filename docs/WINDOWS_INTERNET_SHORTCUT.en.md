# Saved Internet Shortcut target (.url)

The include_windows_startup_shortcut_metadata opt-in now accepts .lnk
and .url files from winfolder: rows. It also requires include_startup_entries.
A .url file supplies windows_shortcut.value->stored_url; stored_local_target,
arguments, and working_directory remain absent. command stays empty, status
remains unknown, and publisher and not_measured impact are unchanged. This
is not a local executable.

The reader recognizes a conservative subset of INI text: one InternetShortcut
section with one URL key, compared ASCII case-insensitively. It ignores other
sections and unused properties. It does not load IconFile, remote icons,
handlers, a browser, or COM. It does not use GetPrivateProfileString or its
possible registry mappings; it parses only bytes from the observed file.
It does not execute, visit, resolve DNS for, or validate the availability
or safety of the target.

## Accepted text and values

Supports ASCII without BOM, strict UTF-8 with BOM, and UTF-16LE with BOM.
It does not guess code pages for legacy non-ASCII or accept UTF-8 without
BOM beyond ASCII. UTF-16BE is unsupported. Malformed Unicode, invalid
surrogates, overlong UTF-8, and NUL anywhere are rejected. LF and CRLF,
line comments starting with ; or #, and spaces or tabs around keys,
sections, and values are accepted. Trailing comments are not stripped
from URLs: ; and # can be part of the value. Quotes are not removed and
percent sequences or variables are not expanded.

Repeated InternetShortcut sections or URL keys are rejected even if
identical. Malformed sections, lines without '=' inside the relevant
section, and a missing or empty URL yield no value, not an available
empty string. This does not replicate the full grammar or precedence
of a shell version or INI API.

An ASCII scheme prefix of the form letter[letter/digit/+.-]*: and
nonempty content without unescaped ASCII whitespace or ASCII controls/DEL
are required. This is a format check, NOT full URL/URI/IRI validation or a
scheme allowlist. Unicode text and arbitrary protocols such as file: and
custom-app: are retained without decoding, normalization, target checking,
or permission to open them.

stored_url is UNTRUSTED text. The GUI should escape it as metadata, not
automatically turn it into a safe or clickable link or pass it to a shell.
It can contain credentials, tokens, misleading characters, or dangerous
schemes. The consumer owns navigation and authorization policy; the library
does not implement it.

## Budgets and errors

Limits: 1 MiB per file, 4,096 lines, 64 KiB per line, and 32,768 UTF-8 URL
bytes. The 128 attempts, 16 MiB of accepted payload, and 16 MiB of retained
text per sample are shared with .lnk, not separate budgets. Up to 128 MiB
of content may be read if reads are discarded or fail. There is no I/O
deadline. See the [native shortcut reader](WINDOWS_SHORTCUT.en.md) for
OPEN_REPARSE_POINT, same-handle reads, and size/last-write revalidation
without atomic guarantees. The URL target is not followed.

Parser, permission, or budget errors leave only windows_shortcut absent,
with a note, and retain the inventoried file. Inventory without the opt-in
does not read file content. The extended 0.x API requires rebuilding the
library and consumers.

## Verification

Fixtures cover ASCII, UTF-8 BOM, UTF-16LE and supplementary Unicode,
case-insensitivity, comments, preserved queries/fragments, ignored remote
icons, and arbitrary schemes stored without opening them. Rejection cases
include duplicates, missing/empty URLs, quotes/controls/unescaped spaces,
unknown code pages, malformed Unicode/UTF-8, oversized files/lines/URLs,
truncated prefixes, and permissions. Pipeline tests cover the 128-attempt
limit and preserve .lnk and shared-budget regressions. Windows reads a
temporary .url fixture in TMP/work and samples Monitor with opt-in and
opt-out. Only the exact fixture file and empty directory are removed.
No real Startup entries are changed, icons loaded, or URLs opened.
This does not certify all shell .url files.

Release: Windows 68/68 and Linux 52/52. Libraries and headers were installed
and external consumers rebuilt and run on both. The Windows consumer
checks that stored_url is separate from command and local path. Linux
tests the portable parser and regressions, not native Windows file reads.
The 0.x API was extended: rebuild consumers with the library.

Official references: [Internet Shortcuts and the URL property](https://learn.microsoft.com/en-us/windows/win32/lwef/internet-shortcuts),
[INI sections and keys, and differences in the Windows API](https://learn.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-getprivateprofilestringw).
