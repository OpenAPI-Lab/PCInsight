# PCISTART v3: historical birth captures without identity/clock coercion

Update119: STARTUP_BIRTH_OVERLAY.md adds an explicit historical presentation helper
with accepted-context guards/declared-association consent; not automatic live/app
attribution. The pending-overlay note below is historical.

Checkpoint118,2026-10-29. StartupImpactArchive adds optional `births` containing
StartupBirthArchive: the original StartupBirthObserverReport, birth-relative
window in nanoseconds and all four StartupImpactPolicy thresholds. Recompile
SDK/consumers for the extended public layout. No collection is started by load.

| Version | Content | Missing birth data |
| --- | --- | --- |
| v1 | Legacy closed observations | Absent/unknown |
| v2 | Same observations + recorder diagnostics | Absent/unknown |
| v3 | Same observations + optional diagnostics + birth section | Present, possibly empty |

Readers accept all three. Writers choose v3 ONLY when births is present; otherwise
existing v1/v2 selection and byte layout stay unchanged. Loading does not migrate
or rewrite files. Old readers reject v3; upgrade readers before explicitly saving
a birth-aware capture. Different captures are replaced, not merged/accumulated.

## Preserved evidence and interpretation

All public birth-report metadata is retained: capture/boot/namespace, lifecycle,
error/message, publishing/loss flags, mapping modes/notes and unknown origin.
Each row retains its complete logical key, explicit binding metric, namespace PID,
kernel birth/release times, CPU/read/write metrics and first-exec metadata. Every
metric keeps presence, Support and note; measured zero identifiers/counters remain
present, absent/protected metrics remain absent/protected. Pending/unbound rows
are stored too. Closed captures may be incomplete; explicit failed historical
captures are supported with a non-none error. No source handles are serialized.

Key capture ID and boot are encoded once in the birth header and restored into
every row after validation that they agree. Birth ID must differ from the outer
legacy recorder ID. No conversion to procfs start_ticks/steady-clock epochs or
sum across apps/PIDs/trees. Legacy diagnostics count legacy rows, not birth rows.

Derived impact vectors are NOT serialized as independent claims. Recompute with
measure_startup_birth_impact(capture,key,window,policy) using the saved window and
thresholds. High-only lower-bound/unknown semantics and declared association remain;
files do not prove executable/app identity, automatic startup cause or current
permission. Loading a historical key never authorizes process control. A live
recorder/observer is not resumed and no Monitor/overlay is silently changed.

## Wire shape and validation

PCISTART magic/version and legacy row encoding are unchanged. In v3, legacy rows
are followed by a1-byte diagnostic-presence flag, the existing v2 diagnostic body
when present, then the birth header/rows. All integers are little-endian, strings
u32 length+bytes, flags/enums/presence1 byte. IDs and counters use their declared
u32/u64 widths. CPU/window/policy CPU durations are nonnegative int64 nanoseconds
encoded in8 bytes. Disk thresholds/counters are u64. Compound metric payloads:
binding length+bytes; local PID u64 device/u64 inode/u32 TGID; executable u64
observation/u64 inode/u32 major/u32 minor/u32 UID. Each ends in its note string.
The existing8-byte FNV-1a trailer covers the WHOLE v3 body; accidental-corruption
detection only, not authentication against deliberate edits.

Existing16MiB file limit and bounded regular/non-symlink/non-reparse IO remain.
Birth limits:64 rows, capture ID128 bytes, binding32768, metadata/metric notes4096,
window >0..1 hour, ordered positive thresholds. Duplicate kernel TGID+start is
rejected even with another UID. Validate full key/header agreement, boot/namespace,
known flags, closed/failed state/error, metric shapes, PID namespace mapping,
first-exec mode/time/device widths, birth/release chronology and deadline overflow.
Final rows require requested final recovery and known read/write; CPU overflow
may stay unknown. Unknown read+write SUM remains a projection issue, not a reason
to erase valid individually stored counters. Origin must stay unknown.

Any invalid header/checksum/version/enum/flag/count/length/semantic field,
truncation or trailing byte rejects the ENTIRE file without partial rows. Invalid
save preserves the previous file. Existing exclusive sibling temp+flush+atomic
replacement and Linux directory-fsync/replaced flag semantics are reused.
Callers serialize writes; file checksums do not implement concurrency control.

## Recorder and verification

LinuxStartupRecorder.save now writes v3 after watch_births succeeds, including
empty captures; the revision117 refusal is superseded. Legacy-only recorders
still write v2. Existing invalid/foreign archive checks and closed/context guards
remain. No helper/host/login activation, GUI, external utility or app-history
format migration occurs.

New fixture checks deterministic roundtrip/reprojection, exact key/metrics/notes/
policy, known zero/unknown/protected/pending/unbound/empty/failed states; every-byte
truncation/corruption, repaired-checksum structural/semantic failures and invalid
save preservation. Existing v1/v2 tests remain. Private native recorder checks
save-load-reprojection after reaping, separate legacy data, file0600 and cleanup.
Evidence and final counts: SEGUIMIENTO.txt, work/startup-birth-store-*.

Initial MSVC file tests failed at replace with Windows error5/access denied under
isolation. The same tests passed with the isolated rename restriction removed;
logs retain BOTH results and no assertion was relaxed. Stage/native-error output
was added without changing write/flush/close/replace behavior. This is NOT proof
that historical N03 Monitor/CPU failures share that cause or are fixed.

I07 still needs reliable app/launch receipt/automatic-startup cause, guarded overlay
and other reapers/descendants. Local tests are not remote CI/universal hardware
or total Windows support. Final results: Linux146/146 without skips+20 focal
executions, installed SDK/consumer exit0, native13 scenarios23 rows (18 releases,
5 absent finals) with v3 save/load/reprojection/0600/FD cleanup. MSVC3/3 final
file/recorder fixtures passed outside isolation; initial failing logs retained.
