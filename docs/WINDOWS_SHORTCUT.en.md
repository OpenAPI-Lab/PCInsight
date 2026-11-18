# Saved .lnk shortcut metadata

Opt-in: include_startup_entries=true and
include_windows_startup_shortcut_metadata=true (false by default). This reads
.lnk files from winfolder: rows, not Run commands. The [Internet Shortcut
provider](WINDOWS_INTERNET_SHORTCUT.en.md) adds .url files to the same pipeline
and budget. The library's parser does not use ShellExecute, IShellLink/Resolve,
COM, extensions, tracking, variable expansion, or external programs. It reads
the shortcut file and never opens or checks its target.

## API and meaning

StartupEntry.windows_shortcut is a Metric<WindowsShortcutInfo> with
stored_local_target, arguments, and working_directory metrics. These contain
saved text, NOT a resolved or verified path, a safe argument list, or launch
authorization. If HasArguments is absent, the argument metric contains an
available empty string: StringData has no argument text, but that does not
certify the effective invocation. An absent working directory has no value.

stored_local_target joins the local base path and suffix from supported
LinkInfo with an absolute drive root. It prefers UTF-16 and accepts legacy
ASCII. ForceNoLinkInfo suppresses the field. IDList, UNC/network, environment,
Darwin, properties, and other mechanisms are not resolved. Local text may
be stale or differ from what Windows would select at launch. The empty command,
unknown status, publisher, and not_measured impact remain unchanged. The
parser does not normalize relative paths, interpret quotes, or execute
arguments. The GUI should label and escape untrusted text.

UTF-16 validation includes surrogate pairs. Non-ASCII legacy data is left
absent; the source system's code page is not guessed.

## Limits and errors

The parser checks the header and CLSID, used ranges and offsets, bounded
IDList, local LinkInfo, counted StringData, Unicode, and bounded ExtraData
blocks with a final terminator. Embedded NULs, truncation, trailing data,
and range overflow are rejected. This is NOT a complete semantic validator:
IDList, VolumeID, and ExtraData payloads are not fully decoded and do not
establish target legitimacy or selection.

Limits: 1 MiB per file, 128 attempts per sample, 16 MiB of accepted payload,
16 MiB of retained text, 1,024 ExtraData blocks, and 260 StringData units
except for arguments (uint16 count). A depleted budget leaves metadata
absent with a note. A read or parser failure affects the metric without
removing the file from inventory. permission_required is preserved. Failed
or discarded reads do not count as accepted payload: the worst case is
128 native read attempts of 1 MiB each, not a strict 16 MiB I/O cap.
There is no timeout for blocked system calls or UNC reads.

CreateFileW uses read-only access and OPEN_REPARSE_POINT. Non-disk handles,
directories, reparse points, and oversized files are rejected. The read
uses the same handle and compares size and last-write time before and
after. This is not atomic or protection against every concurrent change;
the Startup root can be redirected. It does not change the COM apartment
or elevate privileges.

## Verification and remaining work

Fixtures cover Unicode local targets, arguments and directory, supplementary
Unicode, legacy ASCII and non-ASCII, ForceNoLinkInfo, network links without
resolution, every truncation of one fixture, invalid offsets/sizes/Unicode/
NUL/trailing data, the 1 MiB/128-file/16 MiB limits, opt-in and opt-out, and
injected permission errors. Windows reads actual bytes of a temporary
TMP/work fixture and runs Monitor against real inventory. Neither Startup
nor targets are written to. This does not certify all shell formats. The
fixture is removed by exact file and empty directory, without recursive
deletion. The 0.x API was extended: rebuild library and consumers together.

Release: Windows 67/67 and Linux 51/51. Libraries and headers were installed
and external consumers rebuilt and run on both. Linux tests the portable
parser; native .lnk reads and Monitor opt-in are tested only on Windows.
The installed consumer verifies that the flag defaults to false and uses
the Windows opt-in. Metadata adds memory and I/O to sampling and memory
to snapshot history if retained by the consumer. The budget resets on
each sample, not over a time window.

Remaining work: additional formats and code pages, effective resolution,
enabled/disabled approval, publisher and impact, actions, and other
autorun sources.

Sources: [MS-SHLLINK](https://learn.microsoft.com/en-us/openspecs/windows_protocols/ms-shllink/747629b3-b5be-452a-8101-b9a2ec49978c),
[LinkInfo](https://learn.microsoft.com/en-us/openspecs/windows_protocols/ms-shllink/6813269d-0cc8-4be2-933f-e96e8e3412dc),
[StringData](https://learn.microsoft.com/en-us/openspecs/windows_protocols/ms-shllink/17b69472-0f34-4bcf-b290-eccdb8de224b),
[LinkFlags](https://learn.microsoft.com/en-us/openspecs/windows_protocols/ms-shllink/ae350202-3ba9-4790-9e9e-98935f4ee5af).
