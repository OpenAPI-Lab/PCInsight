# Historical manual evidence envelope (internal, revision125)

`linux_manual_store.hpp/.cpp` persists the raw receipt and birth data of the
[manual resource session](STARTUP_DESKTOP_RESOURCES.md). Storage is explicit:
caller chooses path and when to save/load. No background write, activation,
capture resumption, login installation or public SDK API is added.

## Separate format, shared safe IO

The single-file PCIMAN v1 envelope has an eight-byte magic, little-endian
version, length-delimited embedded PCISTART v3 birth archive, optional raw manual
receipt and trailing64-bit FNV-1a checksum. Combined size is bounded to16MiB.
The checksum detects accidental corruption, NOT malicious tampering or origin.
PCISTART v1/v2/v3 and application-history formats/merge semantics are unchanged;
neither reader silently interprets the other format. No automatic migration.

`startup_store_bytes.hpp` exposes INTERNAL memory-codec and atomic-IO helpers
from the existing StartupImpactStore implementation. Its old public load/save
retain their format, validation and replacement behavior. This avoids a second
implementation of temporary-file/flush/rename handling. The envelope still uses
one atomic replacement, not a two-file receipt/birth transaction.

Linux manual saves request a STRICT private-file check: after exclusive0600
temporary creation and BEFORE writing the payload, fstat must confirm regular
file, mode0600 and current effective owner. If the filesystem ignores these
permissions, save returns not_supported, removes the empty temp and leaves the
old target untouched. This was positively tested on the WSL /mnt/c build mount;
the successful private test uses Linux /tmp. Choose a trusted private directory
on a suitable filesystem. This is not encryption, protection from root, or a
claim about Windows ACL access. Windows inherits the selected directory's ACL;
compatibility tests do not certify Windows privacy or add Windows features.

Other existing guards remain: bounded regular/no-symlink input, nonregular
destination refusal, exclusive sibling temp, flush+close+replace and Linux
directory fsync. A directory-fsync failure after rename reports replaced=true;
there is no rollback claim. Callers serialize writes/load-save cycles and must
not overwrite a corrupt load implicitly. No universal power-loss guarantee.

## Stored evidence vs derived claims

`LinuxManualLaunchArchive` contains `StartupBirthArchive` (original raw report,
window and policy) plus `Metric<LinuxDesktopLaunchReceipt>`. Presence/support/
notes of the outer receipt and raw exec metrics are preserved, as are literal
argv (including empty strings), route, full kernel/local/boot/namespace identity,
recorded file pin tuple, launch error, historical PID and quality/exit states.
Absent/denied, failed pre-exec, estimated/sticky quality and known-zero file
identifiers remain distinct. Raw birth rows remain unbound.

Derived entry_first_exec_matches and resource measurements are NOT serialized.
Input caches cannot override raw evidence. Load revalidates/canonicalizes the
receipt; use measure_linux_manual_launch_resources with the stored window/policy
to reproduce historical resource results. A deliberately wrong derived cache
is ignored and corrected, not stored as proof. Full birth validation and
receipt boot/observer namespace consistency are required. Missing child/binding
or coverage remains a gap, not fabricated app/cause/resource evidence.

Decoded caller_owned_pid is only a RECORDED NUMBER, not a currently owned child
or permission to wait/signal. No fd, BPF object, session, live pin, executable,
current app identity or automatic-startup authority is restored. Loaded structs
and checksums are untrusted historical input, not authentication tokens.

## Verification

Pure/fixture tests cover deterministic roundtrip/re-encoding, every-byte
truncation/corruption, repaired-checksum invalid version/support/presence/origin/
argument count, over16MiB input, trailing data, invalid metric/key/notes/argv,
unknown/denied/quality and known zeros. File tests check preservation on rejected
save, unrelated backup survival, nonregular/symlink refusal, 0600 and strict
permission-refusal before payload/replacement. Normal CTest loads NO probes.

`pcinsight-linux-desktop-resources-tests --authorized-private-resources-store`
additionally saves/loads the four previously authorized private cases: successful
resources after reap, two pre-exec failures and early finish while alive. Four
0600 envelopes reproject to the same retained source values/unknowns, preserve
unbound rows and manual origin, and leave no probes/FDs owned. No actual login
startup application is activated. Logs: `work/manual-store-*`.

I07 remains E for recorder integration, trusted managed-autostart context and
other reapers/descendants. N03 remains open. No task IDs/fractional credit added.

Validation: Linux153/153 without skips+30 focal manual/birth/legacy store runs;
installed SDK consumer exit0; MSVC10 manual fixture repeats+2 legacy store tests.
Initial fixture offset type error was corrected. Initial POSIX-mode assertion
failure is preserved and led to the strict pre-payload guard, not a skipped
assertion. Initial make future-timestamp warnings are preserved; final incremental
build and full153 rerun are clean, no clock/mtime adjustment. No full Windows
telemetry/Monitor test or CI completion is claimed.

Revision126 connects the existing recorder through a separate
[manual-only finalize/save path](STARTUP_RECORDER_MANUAL.md), retaining historical
scope and refusing legacy-save routing for this mode.
