# Recorder-owned manual capture and persistence (internal, revision126)

`StartupRecorderAccess` now connects the EXISTING LinuxStartupRecorder to the
[manual resource session](STARTUP_DESKTOP_RESOURCES.md) and
[PCIMAN historical store](STARTUP_MANUAL_STORE.md). This is a private integration
path, not new public SDK methods/layouts, an autostart host or a Windows feature.

## Separate mode, same lifecycle owner

`start_manual(recorder,entry,launch_consent,capture_consent,options)` requires both
consents before context reads/source construction. Options freeze capture ID,
birth-relative resource window and impact policy. Window/policy/IDs are bounded
and validated; native entry paths are lexically normalized. Each recorder has
one capture/mode; an existing legacy recorder cannot be switched into this mode.

The recorder owns a private source whose native implementation owns the existing
LinuxDesktopResourceSession. Construction does not capture. Native start arms
the selected creator before child launch and retains existing gate/receipt
contracts. A released PID is returned without later allocations, clock reads or
failure paths, so its waitpid responsibility cannot be lost. Failed launch can
still leave a recording session for explicit collection of failure evidence;
caller must finish or close it. No unobserved fallback or privilege elevation.

This mode does NOT request acceptance of estimated GIO hints, call Monitor,
discover candidates, scan other processes or register legacy observations.
Its creator scope still requires host serialization of unrelated forks/reapers
and retains the one-child capacity and kernel/permission requirements.

## Finalize, abandon and save

`finish_manual` verifies context/clock, explicitly finalizes the owned source,
checks source support, expected capture/entry/PID/outcome and native observer
identity, then validates/canonicalizes raw evidence through PCIMAN's in-memory
codec. Context/clock are checked again before an all-or-nothing commit. Cached
source resource/link claims are not trusted. No file is written during finish.

The public recorder report exposes closed state and raw unbound birth capture.
Its observations and birth_impacts remain EMPTY: manual measurements are not
inserted into unlabeled legacy startup tables. automatic_startup_origin remains
unknown. `manual_archive` returns a historical copy, from which the existing
manual-resource projection reproduces values under the frozen window/policy.

The recorder's public poll checks context without scans before the scheduling
deadline; an explicit poll at/past that deadline finalizes the manual source.
The resource window remains BIRTH-relative, not converted into the recorder's
global scheduling interval. Explicit early finish is allowed; reap first for
final counters, otherwise unknowns remain. No hidden worker/timer/reaper.

Public close is ABANDONMENT in this mode: stop without a final sensor read and
without manufacturing a saveable archive. Destructor, move-assignment cleanup
and failure detach the owned source once, without killing/reaping released
children. Move transfers ownership; moved-from calls fail safely. Report copies
are historical, not live control authority.

`save_manual` requires successful finalization and unchanged context, refuses
corrupt/foreign existing files, then explicitly replaces PCIMAN using its
atomic/private-file guard. Save never finalizes, launches or loads a capture.
The public legacy save REFUSES this mode rather than writing an empty PCISTART
that loses its receipt. watch_births is likewise refused while the manual mode
owns its separate birth source; declare-association APIs are not promoted.

## Evidence and limits

Private injected-source fixtures test consent/no reads, invalid options, no GIO
scans, explicit/deadline finish, abandon without sample, move/destruction,
context/clock/capture/source-quality failures, snapshot isolation, frozen policy,
historical projection, corrupt-file preservation and safe save routing.
Callbacks run under the recorder mutex, must not reenter, and their fixtures
must outlive the recorder. There is no global/native provider override.

Manual native verification:

```
pcinsight-linux-desktop-resources-tests --authorized-private-recorder-store
```

Four private children use the REAL default recorder context/native owned source:
positive resources after reap, missing-cwd and invalid-ELF failures, and early
finish of a pipe-blocked live child. Recorder finish/save and reload/reprojection
pass in all four cases; public legacy tables stay empty, raw rows stay unbound,
files are0600 and all probe/FD references close. The live child is released and
reaped only by its caller. No real login/startup application is activated.

Logs: `work/recorder-manual-*`. Normal CTest loads no BPF probes. Existing GIO
recorder and PCISTART behavior remain separate/unchanged. I07 still needs trusted
managed-autostart context and remaining reaper/descendant coverage; this manual
recorder is NOT automatic-startup cause. N03 remains open, no fractional credit.

Validation: Linux154/154 without skips+30 focal manual-recorder/legacy-recorder/
manual-store repetitions; installed SDK consumer exit0. MSVC20 injected manual/
legacy recorder repetitions, no native Windows telemetry claim. Initial make
future-dependency timestamp warnings remain in logs; final incremental rebuild
and full154 rerun pass without warnings, no clocks/mtimes adjusted.
