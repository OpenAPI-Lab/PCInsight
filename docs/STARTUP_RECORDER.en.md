# Optional PCInsight recorder for an early host

Update 117: [capture of recorder-owned children](STARTUP_RECORDER_BIRTH.en.md) is
opt-in for a selected creator. Keys and windows are separate, binding is
explicit, and there are no invented sums or clocks. At this revision, save
rejects birth captures until a format can preserve them, avoiding silent data
loss. Rebuild the SDK and its consumers.

Update 108: Compatible final CPU accounting is integrated into the existing
opt-in path; see [scheduler CPU accounting](STARTUP_SCHEDULER_CPU.en.md). The
historical exit_disk flag name is retained, but additional CPU time is available
only with a compatible scheduler baseline. The procfs fallback remains a lower
bound. Sample adds the counter type: rebuild consumers. The v1/v2 format is
unchanged; notes distinguish the CPU source, and no capture resumes from a file.

Update 107: Save writes both observations and historical diagnostics in v2;
see [startup diagnostics](STARTUP_DIAGNOSTICS.en.md). V1 remains readable with
metadata absent. New archive/finished_at/counter fields require recompiling
consumers; the old ABI is not supported.

Update 106: Optional disk recovery for preselected targets is integrated into
this API and is OFF by default; see [startup disk tail](STARTUP_DISK_TAIL.en.md).
Final CPU is not mixed with procfs. The CLI retains its non-BPF mode; the
privileged option is exposed only through StartupRecorderOptions in the library.
Options/Report add fields: rebuild consumers. There is no binary ABI promise
for prior layouts. The StartupImpactStore format is unchanged in this update.

Revision 103, 2026-10-24. The user authorized development of the second
alternative in [early startup decision](STARTUP_EARLY_DECISION.en.md) by replying
“continue” to the specific proposal, WITHOUT installing or enabling it yet.
The component is implemented; I07 is NOT closed.

## Library and first-party executable

`pcinsight/startup_recorder.hpp` provides `LinuxStartupRecorder`, a reusable
controller for a capture. The optional host `tools/startup_recorder.cpp` uses
the SAME API; it is neither a third-party app nor a tool invoked by the library.
The GUI can also host the API with its own scheduler.

The SDK includes the controller. The Linux-only executable is build-time
opt-in via `PCINSIGHT_BUILD_STARTUP_RECORDER=ON` (OFF by default). It has no
install rule, service, .desktop entry, or login task. In this revision it is
built and run only in private tests, without installing or enabling the host.
Installing the SDK under work/install-linux for the test consumer does NOT
install the executable or modify actual login. Enabling it at login requires
separate authorization. The default mode requires no root, BPF, third-party
daemon, or new packages. The optional recovery from revision 106 requires
appropriate privileges and a compatible kernel; it does not elevate privileges
or install dependencies automatically.

```cpp
#include <pcinsight/startup_recorder.hpp>

pcinsight::LinuxStartupRecorder recorder;
pcinsight::StartupRecorderOptions options;
options.capture_id = "sesion-elegida-por-el-host";
options.accept_estimated_gio_associations = true; // informed consent
auto result = recorder.start(options);
// If result is valid, the host scheduler checks report().next_scan and
// calls poll(). Once closed, save(path) is a separate EXPLICIT operation.
// There is no hidden thread/timer and no writes just because time passes.
```

Constructing the recorder or calling start without accepting the hints does
not start the backend. There is one capture per instance; repeated start calls
are rejected rather than resetting the capture or adding another phase. Moving
transfers the capture; the moved-from instance cannot start or save. The object
holds a mutex; native readers and saving are serialized. Do not run reentrant
callbacks in the private test seam.

## State, timing, and coverage

States: idle -> recording -> closed or failed. ID: 1–256 bytes without NUL;
duration: 10 ms–1 h; scan interval: 10 ms–60 s. These bounds are validated
before converting durations to native ticks. Since revision 106, after checking
context, the timed phase starts BEFORE the first scan, poll, or probe setup:
slow work does not buy extra time. start can return closed if that work consumed
the time budget. There is no hard I/O cancellation. Late apps retain partial
windows, not certified low/medium ratings. This replaces revision 103's warmup
outside the timed phase.

A tick that is not due does not read processes. A due tick scans and polls, then
schedules the next tick from the current clock time without accumulating
overdue scans. At or beyond the deadline it allows a final poll, closes, and
does not read again. A slow scan or reader can overrun the deadline; there is
no I/O cancellation or hard deadline. `close()` closes immediately without
a final read or implicit save; incomplete observations remain unknown or a
high lower bound according to the observer.

The recorder's native context is verified before scans, the final poll, and
save: the sampler's boot/PID namespace, plus the current caller's procfs
identity (PID, start, UID, namespaces) and euid. Forking or a change in caller,
euid, or context is rejected rather than continuing attribution in a different
domain. `close()` uses only a cheap PID/euid guard, with no I/O; save revalidates
fresh context. Endpoint checks do not trace ABA changes.

The controller discovers candidates for the effective UID through the existing
[startup candidate source](STARTUP_CANDIDATES.en.md), filters already registered
identities, and automatically registers only estimated candidates, with no
numeric claim of automatic origin. An identity is not registered twice and
does not switch apps because of a later hint. Batch-wide errors prevent the
batch's candidates from being applied; partial scans without errors may
register their declared subset and increment partial_scans. A read error is
not hidden behind a zero. Context changes, exceptions, a backwards clock,
limit violations, and overflows fail closed and preserve confirmed observations.
There is no atomic rollback of registrations already accepted when an exception
occurs mid-processing: they are closed and exposed as partial evidence from a
failed phase, which the recorder's save method refuses to store as successful.

Inherited limits: 4,096 observations/8 MiB in the observer, 4,096 PIDs visited
and 8 MiB of environment per discovery scan, and 100,000 snapshot rows. Report
limits and uint64 diagnostic sums are also checked, without wraparound or eviction.

`report()` returns scans/partial_scans/examined/ignored/unavailable/registered/
rejected, poll_attempted/updated/failed, exit_disk_attempted/attached/skipped/
unavailable/recovered, phase, context, times, and observations. Since revision
107, exit_disk and other counters are saved in the v2 file's optional diagnostics.
exit_disk_released_without_final counts probes closed without a final record,
including processes still alive at closure; it does not prove a failure.
finished_at is local steady-clock time held only in memory. The file stores
relative elapsed time, not a steady-clock epoch or absolute next_scan/deadline.
`automatic_startup_origin` remains ABSENT: GIO hints can come from manual launches
or be forged. The controller does not treat them as proof of startup cause.
It does not automatically sum processes or descendants or apply a hidden overlay.

## Saving and optional CLI

```text
pcinsight-startup-recorder --accept-estimated-gio FILE CAPTURE_ID [DURATION_MS]
pcinsight-startup-recorder --help
```

This explicitly runs a foreground host. It does not daemonize, install anything,
elevate privileges, open a GUI, or execute a .desktop Exec entry. The CLI
announces readiness only after setup and indicates whether the phase is still
active or closed during setup. It samples while active and saves explicitly
at closure. It does not install special signal handling; abrupt termination
can lose the in-memory capture. There are no hidden periodic writes or
checkpoint resumption.

The caller chooses a private file and directory; the directory must already
exist. `save` requires a closed phase without failure, fresh context, and a
destination that can be loaded if it exists. It rejects corruption, foreign
formats, and invalid destinations. It uses [StartupImpactStore](STARTUP_STORE.en.md)
to replace the capture, NOT merge it. A durability failure can report
replaced=true: do not retry as if nothing had been published. Preflight/load
is not compare-and-swap; the caller serializes writers.

IMPORTANT (original revision 103 behavior, superseded for diagnostics by
update 107): the existing format saved ONLY closed historical observations,
context/ID/recorded_at. It did NOT retain recorder report diagnostics or certify
discovery coverage. The CLI announced that limitation. The GUI must not infer a
complete census or resumable state from the file. An empty capture means “no
acceptable candidate observed,” NOT zero overall impact. CLI errors: 2 for
arguments/consent, 3 for unavailable provider/start, 4 for destination/save
(may report replaced), and 5 for failure during capture/execution.

## Tests and pending work

A deterministic fixture uses a private seam PER OBJECT: denial without I/O,
bounds, timing/no work before due, deduplication, late arrivals, final/overshoot,
partial/failed batches, context changes/denial/exceptions, gaps, closure without
reading, clock reversal/overflow, and move. There are no global hooks or public
provider of fabricated data. MSVC tests the controller with simulated sources
and native denial without building the Windows Monitor; it does not certify
Windows support.

A real Linux test of the optional executable uses private XDG data, a private
file, and a private C++ child. The host announces readiness BEFORE creating
the candidate, then discovers its identity, measures actual CPU, closes its late
partial window, and saves/loads a 0600 file. It checks that the .desktop entry
remains intact and Exec never runs, invalid or nonconsenting requests do not
replace the file, a corrupt destination is preserved, the empty capture is
reported honestly, and both children are reaped. No real login, services, or
apps were touched. The native source enumerates visible processes and reads
hints only for the same euid; this is not proof of capture limited exclusively
to private PIDs or of capture for other UIDs.

I07 is NOT closed: reliable provider-specific autostart cause, lifecycle
coverage of unrelated short-lived processes and descendants, early end-to-end
coverage, and a durable diagnostics policy remain pending as of this revision.
This host lets the library run before the GUI, but it cannot measure before ITS
OWN startup or recover processes that have already disappeared. The development
permission does not authorize enabling real login startup or expanding global
capture. The next unit must address that evidence and lifecycle, not install
this binary or count the previous example as full capture.

Revision 104 completes an INTERNAL last-thread event sensor for a preselected
target, retained even after the parent reaps it; see
[selected process exit](STARTUP_SELECTED_EXIT.en.md). That event remains separate.
Revision 106 connects ONLY disk data from the revision 105 release provider to
the opt-in recorder; it does not add final CPU or full coverage of short-lived
processes. See [startup disk tail](STARTUP_DISK_TAIL.en.md).

Revision 103 results: Linux 139/139 without skips plus 30 focused repetitions;
10 MSVC simulated-controller/denial tests without a backend; relinked installed
consumer exited 0; final build without warnings. The executable is NOT in the
installed SDK prefix. The initial fixture expected the not_measured enum for an
unknown metric; the expectation was corrected to absence (without changing the
classifier), and the initial log was preserved. Logs: work/startup-recorder-*.
Previously undeclared historical N03 risks were corrected.
