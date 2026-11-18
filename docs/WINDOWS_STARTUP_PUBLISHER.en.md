# Windows startup company label (VERSIONINFO)

Independent opt-in: include_startup_entries=true and
include_windows_startup_publisher=true (false by default). Reads CompanyName
from the candidate executable's VERSIONINFO without running application code,
verifying signatures, elevating privileges, or installing dependencies. It
uses Version.dll, which is already linked. This opt-in DOES query the candidate
file, unlike the .lnk/.url reader, which reads only the shortcut, not its target.

## GUI data and trust

StartupEntry.publisher_label contains CompanyName with estimated support and
a provenance warning. publisher_source_executable retains the candidate path,
also marked estimated. The legacy publisher string mirrors the label only
when a value exists. Without the opt-in, Windows entries still lack a publisher.

CompanyName is self-declared resource text, NOT an Authenticode publisher,
a valid signer, or proof of integrity, trust, or payload authorship. If the
EXE is a launcher or interpreter, the label describes that file, not its
arguments or the DLL, script, or app it loads. The GUI should show the source
and estimate status, not add a verified-publisher badge. Startup status and
impact are unchanged.

The path is a candidate for resource lookup, not verified launch identity.
A replaced file, stale link, alias, or concurrent change can affect the
result. The provider does not validate existence or version at launch time
or enable any execution or control action.

## Conservative selection

- Run: an EXE prefix in closed quotes followed by whitespace or end of text,
  or the first unquoted token if it is already an absolute .exe path without spaces.
- Direct .exe file in Startup: its source path.
- .lnk: available stored_local_target from an available windows_shortcut
  result. Also requires the metadata opt-in and supported local LinkInfo.
  Does not call Resolve.

Only explicit drive-letter paths and the .exe extension (ASCII
case-insensitive) are accepted. No PATH lookup, variable or percent
expansion, UNC, extended prefixes, drive-relative paths, alternate
slashes, streams, wildcards, or ambiguous commands. No scripts, DLLs,
.url files, or handler lookup. Uncovered cases have no label; the binary
is not guessed from the app name.

This does not guarantee local-only I/O. A drive can be mapped, a path
can traverse reparse points, and the version API can consult MUI resources.
The provider does not explicitly request signature, revocation, or network
checks and does not load the executable to run it. The host should consider
path policies and I/O cost before opting in. There is no API deadline.

## Errors and limits

Up to 50,000 rows, 128 distinct textual paths queried per sample, and
16 MiB of output values (counting the label twice when mirrored in publisher).
Caching applies only within the call and includes missing results and errors.
CompanyName is not retained across samples because files may change. Path
case differences and aliases are not canonicalized or deduplicated by file
identity. Paths are limited to 32,768 UTF-8 bytes and labels to 16,384 bytes,
without embedded NUL.

Native resource limits: VERSIONINFO buffer up to 4 MiB, Translation up to
4,096 bytes (1,024 language/code-page pairs), and values up to 32,768 wchar_t
units with a terminator inside the resource and strict UTF-8 conversion.
Declared translations are examined; English is not forced, and ProductName
or FileDescription is not treated as CompanyName. These are buffer and
query limits, not strict I/O byte quotas.

Missing resources, fields, access, or supported formats yield no value.
The reused native reader reports absence without classifying its cause;
it does not assume that every absence is a permissions issue. An injected
provider's permission_required is preserved. A label failure does not
remove the inventory entry. Other providers and platforms are unchanged.
The 0.x API was extended: rebuild consumers.

## Verification

Portable tests cover quoted and unquoted paths, ambiguity, .exe/.lnk/.url
and scripts, variables/UNC/streams/wildcards, metadata trust, caching,
permissions and absence, invalid values, 128/50,000/16 MiB limits, and
preservation of unrelated rows. The Windows test uses a Unicode CompanyName
in the second resource language of its own executable. It covers native
reads, injected Run selection without registry writes, Monitor opt-in and
opt-out, and preservation of FileDescription behavior. No real Startup
entries were changed or run.

Release: Windows 69/69 and Linux 53/53. Libraries and headers were installed
and external consumers rebuilt and run on both. The installed consumer
verifies opt-out by default and publisher/label/source consistency with
Windows opt-in. Linux runs portable selection, caching, budget accounting,
and regression checks, not native VERSIONINFO reads. MSVC, other hosts,
and other resource variants need validation. This does not complete
signature verification or establish a trusted publisher. The subsequent
Authenticode query is independent and does not change the trust level of
CompanyName: [local-policy signature](WINDOWS_STARTUP_SIGNATURE.en.md).

Official sources: [CompanyName in StringFileInfo](https://learn.microsoft.com/en-us/windows/win32/menurc/stringfileinfo-block),
[GetFileVersionInfoW and MUI resources](https://learn.microsoft.com/en-us/windows/win32/api/winver/nf-winver-getfileversioninfow).
