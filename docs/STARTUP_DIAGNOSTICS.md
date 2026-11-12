# Durable startup recorder diagnostics

Update108: exit_disk_enabled retains its legacy name for the optional combined
path. CPU can now be recovered with a matching clock baseline, but exit_disk_*
counts remain disk outcomes; CPU availability/provenance is in each metric note.
Wire v1/v2 is unchanged. See STARTUP_SCHEDULER_CPU.md.

Revision107, 2026-10-24. `StartupImpactArchive::diagnostics` is an optional
`StartupCaptureDiagnostics`. LinuxStartupRecorder.save now writes it together
with observations in ONE atomic historical file. There is no sidecar race,
automatic save, merge, checkpoint resume or activation of startup entries.
This completes aggregate diagnostic persistence, not the rest of I07.

## Meaning and availability

Metadata preserves requested duration/scan interval, observed phase elapsed,
whether privileged exit-disk recovery was enabled and its concurrent bound,
scan/partial-scan/examined/ignored/unavailable/registered/rejected counters,
poll attempted/updated/failed, exit-disk attempted/attached/skipped/unavailable/
recovered/released_without_final, and automatic_startup_origin with presence,
Support and note. A legacy file has diagnostics ABSENT, not measured zeroes.

Counts describe work actually attempted by this recorder. They do NOT enumerate
all startup apps or yield a global capture percentage. No scan can count apps
that disappeared before selection. Ignored/candidate hints are not reliable
evidence of automatic startup. The native provider's origin remains unknown.
An origin value in a caller-supplied archive is a stored claim, not authentication.

`exit_disk_released_without_final` counts attached readers stopped without a
final record or reader error. It includes a process still alive when a window
ends, as well as an uncovered exit still pending. It does NOT prove an exit was
missed. Unavailable counts attachment/read/reset failures; recovered counts only
successful partial-final disk integration. Skipped means no concurrent slot,
not a false zero and not an automatic retry. Disabled recovery has no activity.

`finished_at` is a new report-only local steady-clock time. finish rejects clock
reversal; save checks interval overflow/representability using integer arithmetic.
Only relative elapsed nanoseconds are serialized, never absolute steady epochs,
next_scan/deadline, fd numbers, BPF state, live baselines or process environments.
Elapsed may be shorter than requested (explicit close), or longer (slow I/O).
It ends at capture closure, not later file-writing time. Wall recorded_at remains
the existing millisecond timestamp, not a certified clock value.

## Format and validation

PCISTART v1 is unchanged and still read. Save without optional diagnostics emits
v1. Save WITH diagnostics, including recorder.save, emits v2. All v1 observation
records keep their wire shape; v2 appends before the checksum:

1. Requested duration ms, scan interval ms, elapsed ns: three little-endian u64.
2. Exit-disk enabled u8, configured concurrent bound u32.
3. Sixteen little-endian u64 counters, in the order listed in the first paragraph
   (seven scan, three poll and six exit-disk counters).
4. Origin metric: Support u8, presence u8, optional boolean as u64(0/1), note
   length u32 and bytes. All metadata is covered by the existing FNV-1a checksum.

Validation rejects invalid duration bounds, negative/overflowing elapsed, bad
enums/booleans/notes, trailing bytes and truncation. Registered must equal the
number of archived observations; partial scans cannot exceed scans; successful
and failed polls partition attempted polls. For a successfully closed capture,
exit attempts partition into recovered/unavailable/released_without_final and,
when enabled, attempts plus skipped equal registered. Comparisons/subtractions
are checked before arithmetic; malformed UINT64_MAX counts cannot wrap.
The existing16MiB/4096-row/metadata limits and atomic file replacement still apply.
The checksum is corruption detection, NOT a signature or trusted telemetry proof.

Readers predating107 reject v2; rebuild/update consumers before new captures.
New public archive/report fields also require recompilation (no old-layout ABI
promise). Loading never rewrites a v1 file. Explicitly dropping diagnostics and
saving emits v1 and loses that metadata; this is not done implicitly.

## Verification

Store fixtures exercise unchanged deterministic v1, v2 roundtrip/determinism,
unknown versus measured zero/false, inconsistent counts/durations/flags/enums,
every truncation/byte corruption and every metadata truncation with recomputed
checksum, invalid bool, valid-checksum trailing data, file-size bounds and failed
replacement preserving the old file. Existing symlink/FIFO/reparse/locked-file
guards and temporary cleanup apply equally; native Linux checks0600.

The actual optional host captures a private late app and an empty run, with
diagnostics saved/loaded. Manual privileged testing uses the prior authorized
PRIVATE six-case target only: five recoveries, one uncovered internal auto-reap,
real native provider/recorder/observer/store and diagnostic roundtrip including
released_without_final. No global app capture or real login activation is done.

I07 remains open for compatible CPU baseline/final counters, never-preselected
short lifetimes and reliable automatic-startup attribution. Persisting diagnostics
does not make those measurements available or turn partial impact into low/medium.
