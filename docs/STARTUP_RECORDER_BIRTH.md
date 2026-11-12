# Owned selected-creator birth capture in LinuxStartupRecorder

Update118: [birth-aware PCISTART v3](STARTUP_BIRTH_STORE.md) supersedes the117 save
refusal below. Explicit save now preserves the raw capture/window/policy with
legacy views still separate; v1/v2 remain readable. Rebuild SDK and consumers.

Checkpoint117,2026-10-29. ONE explicitly selected creator capture can now be owned
by the recorder. Construction/start/poll do not silently enable BPF. Existing GIO
consent remains required at recorder.start. Public StartupRecorderReport layout
is extended: rebuild SDK and consumers. No archive format changed.

## Lifecycle and limits

After recorder.start, call watch_births(consent, borrowed_creator_pidfd, pid,
start_ticks, birth_options). Recorder must still be recording. Birth capture ID
must be1..128 bytes and DIFFERENT from recorder ID; capacity1..64. One attachment,
no replacement/restart. The native observer duplicates the borrowed pidfd after
validating identity and permission. No elevation, child launch/reap, configuration
change, retroactive capture or permanent installation is performed.

The default factory owns a REAL LinuxStartupBirthObserver. Recorder.poll polls it
on EVERY call, even before the next legacy scan. Context checks, transactional
publication, immutable retained rows and sticky quality flags remain enforced.
The recorder additionally rejects changed context, omitted/rewritten rows and
malformed projections before replacing its accepted views. A source failure
stops owned collection and preserves confirmed results. An attachment denial
before ownership can leave legacy recording active.

Explicit close does NOT poll. Deadline polling samples once then closes; a slow
early birth poll crossing the deadline also closes. Failure/destruction/move
cleanup stop owned capture. Moved-from recorders are inert. No worker/timer is
added. Caller scheduling determines actual stop: this is NOT a kernel-enforced
global deadline. Delayed polls can retain births outside the intended host phase.
Do NOT label birth data as complete/global recorder-phase measurements.

## Separate views and binding

report().birth_capture retains historical source keys, namespace-qualified PIDs,
first-exec metadata, flags and lifecycle. report().birth_impacts projects ONLY
explicitly bound rows through measure_startup_birth_impact. Recorder duration is
each child's BIRTH-RELATIVE window, not the legacy steady-clock phase.

associate_birth(exact_key,entry_id) is explicit, also after successful close with
no hidden sample. Candidates, matching executables, parent or PID do not create
bindings. Conflicts are refused. Source caller checks still apply. Projection/
copy errors fail the recorder rather than silently returning inconsistent views.

Legacy observations stay in their own vector. SAME app/local PID does not merge
or add CPU/disk between sources. No fabricated procfs ticks/steady-clock identity,
process-tree sum, automatic cause or Monitor/overlay update. Group IO may already
include waited children. Birth classification remains high-only/unknown, binding
declared/estimated, as documented in STARTUP_BIRTH_IMPACT.md.

## Safe refusal of incomplete saves

After watch_births SUCCEEDS, save refuses with not_supported even for an empty
birth capture: PCISTART v2 cannot represent those keys/time axes. Refusal occurs
BEFORE file inspection/replacement; replaced=false. Active/failed saves retain
their existing rejection rules. No silent legacy-only save or coercion occurs.
A birth-aware durable format remains pending. Other history stores are unchanged.

## Verification

Pure fixtures cover consent/bounds/ownership, pending/bound/final data, early poll,
same-app/local-PID separation, failure/exception cleanup, CPU rewrite/omitted rows/
context/quality rejection, close without poll, slow/deadline stop, move and save
refusal. Release assertions stay active. Installed SDK consumer checks the new
symbol and denied-call behavior without enabling collection.

Private native harness uses the REAL birth factory under recorder lifecycle and
empty private legacy census callbacks (no root-wide process scan). Attachment
precedes child execution; child PIDs reach the controller ONLY AFTER reaping.
Each capture uses the same independent CPU-clock bounds and exact private IO
oracle, not cross-sensor CPU equality. Exact binding, separate views, creator-reap
replay, close retention, save refusal and FD census remain checked. Previous raw/
public/default-independent/first-exec/candidate/projection checks remain active.
No login/desktop/service or permanent recorder is activated.

Linux145/145 without skips plus20 focal repetitions; MSVC10 pure repetitions;
installed SDK consumer rebuilt/relinked exit0. Native13 scenarios passed with23
recorder rows (18 release-resource rows,5 absent finals), confirmed by the final
native-confirm run exit0. Existing checks and FD census stayed active. C++ builds
clean; MSVC assert-enabling D9025 intentional. Execution evidence: SEGUIMIENTO.txt
and work/startup-recorder-birth-*.
I07 still needs reliable launch receipt/app/cause, birth-aware persistence/overlay
and other reapers/descendants. N03 remains open. Local evidence is not universal
Linux/kernel or Windows coverage.
