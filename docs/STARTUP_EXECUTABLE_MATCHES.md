# Current XDG filesystem candidates, not automatic attribution

Update115: [capture validation](STARTUP_CAPTURE_VALIDATION.md) reproduces a CPU
difference between two independent captures and separates same-source exact
passthrough from default-native independent clock/IO oracles. The matching logic
is unchanged; precise kernel interleaving remains unproven/N03 open.

Checkpoint114,2026-10-29. Public header `pcinsight/startup_executable_matches.hpp`:
`discover_startup_executable_matches(report, exact_child_key)` returns a read-only
list of candidates from the CURRENT CALLER's effective XDG autostart inventory.
It does not mutate the observer/bindings/entries, launch programs, elevate, install
anything or classify impact. It is NOT target-UID or launch-time inventory.
Caller must serialize environment changes; synchronous native I/O may block.

## Meaning of a match

The exact retained capture/boot/kernel key selects existing first-exec evidence.
Missing evidence or another boot/PID namespace is rejected before file matching.
For each effective desktop entry the API compares the historical first-exec
device-major/minor/inode tuple with the CURRENT regular file named by its literal
absolute Exec program. It never matches names, command substrings or a bare PID.
The comparison is estimated/non-atomic filesystem evidence, not app identity.
Arguments are NOT used to disambiguate apps sharing one binary. Disabled entries
remain candidates: current enablement does not prove their historical state.

`unique_candidate` has a value ONLY if the entire current catalog was resolved
and exactly one entry's file identifiers match. It remains Support::estimated.
Two matching entries, even one currently disabled, remain ambiguous. Any unreadable,
unsupported, changed or unresolved entry suppresses uniqueness, even if another
entry matches. Empty/no-match catalogs do not assert absence of historical startup.
`automatic_startup_origin` always stays unknown. No automatic associate() call.

A file may have been replaced or its identifiers reused BEFORE this query. No
file handle from capture time is available here. Wrappers, interpreters, shared
executables, mount/filesystem-specific stat semantics, content changes and manual
launches prevent a causal conclusion. A unique CURRENT tuple match must not be
presented as proof that this desktop entry launched the process. An independent
launch receipt/protocol is still needed for reliable attribution/startup cause.

## Explicit Exec subset

The parser follows two layers from the
[Desktop Entry Exec specification](https://specifications.freedesktop.org/desktop-entry/latest/exec-variables.html):
string unescaping before whole-argument double quoting/escaping. It validates the
whole command without expanding arguments or running a shell. Supported program
paths are literal absolute printable-ASCII paths; quoted spaces and literal %%
outside quotes are supported. No lexical path normalization across symlinks.
Invalid/unknown field codes, excessive file/URL placeholders, non-standalone
multi-argument codes, quoted field codes, invalid quoting/escapes and reserved
unquoted characters stay unresolved, not guessed.

Bare/relative programs need the LAUNCHER's PATH/CWD, which was not captured.
Unicode/control-character command lines and other unsupported valid forms are
explicitly unresolved. Type=Application is required; duplicate routing fields,
Terminal=true and DBusActivatable=true are not treated as direct Exec routing.
Eligibility and current enablement are not used to certify launch/causality.

## Read safety and cost

At most4096 effective candidates,1024 Exec tokens and32768 command bytes. Exec
targets use Linux openat2 O_PATH with RESOLVE_NO_MAGICLINKS: normal symlinks may
resolve, proc magic links do not. Only regular targets proceed; no executable
content read, device/FIFO I/O or command execution. Two pinned target/path stats
compare device/inode/mode/UID/GID/size/mtime/ctime. This cannot detect all ABA races.
If openat2/UAPI support is unavailable, return unknown without a permissive fallback.

Properties are read in a batch, not by reenumerating the entire inventory for
each entry. Before/after effective censuses plus full caller-context checks
invalidate all candidate results if inventory/context changes. Per-entry property
reads pin a regular desktop file before opening its own fd for bounded content
read, revalidate metadata/path and compare ordered fields around the executable
query. Maximum1MiB per desktop read and16MiB property accounting per query; a read
can discover that the remaining budget was exceeded, then later entries are
skipped/unresolved. The two existing inventory scans have their own bounds.
Source snapshots still are not atomic and no hostile concurrent environment or
mount changes are supported. No persistent catalog or user XDG file is created.

## Tests and acceptance

New startup-executable-matches CTest target: Linux143/143 total,30 focal repetitions,
MSVC10 pure matcher runs and installed SDK consumer exit0. Tests cover grammar,
unique/shared/disabled/unknown/no-match, changed properties/context/inventory,
permission errors, budget, exact child key and unavailable first exec. Native
filesystem fixtures verify no script execution (marker stays absent), rejection
of magic links and FIFO, plus replacement while the old inode is pinned.

The manual private exec harness creates temporary XDG catalogs ONLY in its own
test process/environment. Real captured first-exec data yields one estimated
candidate, then two ambiguous candidates sharing the same binary (one disabled).
The observer association/origin is not promoted. Original environment is restored;
only private temporary files are removed. Release fixture checks throw, not
assert-only side effects. Collection still needs only the prior private eBPF scope.

Initial warnings/fixture missing <map> on MSVC were corrected and logs retained.
One initial native run stopped on the EXISTING public/internal resource equality
assertion in mode10; that run is NOT counted as a pass. Resource diagnostic output
was added without relaxing the assertion; the four-case repeat passed, with no
reproduction. The subsequent complete13-case run also passed, including6real
exec observations each yielding a unique then shared/disabled ambiguous candidate.
Cause is not demonstrated/fixed: tracked as N03 risk. Matcher unit
and native candidate checks are separate evidence, not a claim that N03 is closed.
Logs: work/startup-file-match-*; remote CI and universal Linux coverage not claimed.

I07 remains E: reliable app/desktop/launch receipt and automatic-startup cause,
time-bounded integration/other reapers/descendants. No archive format change and
no automatic UI overlay, association, recorder activation or process-tree sums.
