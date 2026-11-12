# Public selected-creator birth observer and explicit child attribution

Update120: STARTUP_GATED_LAUNCH.md adds an INTERNAL controlled pre-exec arming
barrier. It is not a successful-exec/app/automatic-startup receipt and does not
promote associate() declarations or change this observer's public behavior.

Update116: [birth-relative impact projection](STARTUP_BIRTH_IMPACT.md) consumes
explicitly bound retained rows without inventing procfs identity/steady-clock
samples. Default60s since each birth, release within window, estimated high-only
lower bound or unknown. No recorder/overlay activation or automatic-startup cause.

Update115: [same-source/native validation](STARTUP_CAPTURE_VALIDATION.md) corrects
the cross-capture equality oracle. A published final remains immutable, including
CPU (even1ns rewrite is refused). Release-observed CPU is not a guaranteed total
after the target's final scheduler switch; no native counter formula changed.

Update113: observe_first_exec OFF by default adds historical executable file
metadata, STARTUP_FIRST_EXEC.md. Public layouts require recompilation; no app ID
or startup-origin promotion. Flag64 and private raw20-word layout extend112.

Checkpoint112,2026-10-29. Header: `pcinsight/startup_birth_observer.hpp`.
`LinuxStartupBirthObserver` exposes111's internal Linux sensor through an owning,
poll-driven SDK API. Construction/reporting do not start collection. No installed
recorder, startup entry, service, launch, hidden timer/thread or automatic overlay.

## Lifecycle

`start(consent, borrowed_creator_pidfd, creator_pid, creator_start_ticks, options)`
requires explicit authorization and a genuine selected creator. The fd must stay
owned/stable during start; the library duplicates it, never reaps/signals that
creator and never raises its own privileges. Capture IDs are1..128 bytes without
NUL; retained capacity1..64 (default16); `recover_final_resources` defaults OFF.
When ON, final totals only cover the same selected creator's direct releases,
as detailed in STARTUP_CHILD_FINALS.md. No collection before attachment is recovered.

`poll()` accepts one native snapshot transactionally. Replay cannot duplicate
records; known birth data is immutable and final data transitions only once.
An omitted publishing row is retained from the last accepted snapshot. Foreign
context, duplicate identities, invalid modes/flags/counters, backwards lifecycle
or rewritten final data stop owned capture with `failed` state; previous accepted
records and associations survive. Source loss flags remain sticky and explicit.

`close()` stops owned capture without a hidden final read. It retains records and
bindings. A used capture cannot restart/resume in that object; create a new one.
`report()` is a copy only, never a system read. Creator exit is not evidence that
all descendants exited or that startup measurement is complete. Pending fields
remain absent, not zero. Closing does not magically fill them.

Calls are serialized. Native start/poll and explicit association validate full
caller boot/namespace/process context, not numeric PID alone. Coordinate forks
outside active calls: inherited references must be closed/exec'd, and no atfork
coordination or force-detach despite references held elsewhere is promised.
Non-Linux authorized start returns not_supported, never calls a null source hook.

## Association, not an inferred executable identity

Every observation has a `StartupBirthProcessKey`: capture ID, boot ID, GLOBAL
kernel TGID, kernel real UID and kernel start_boottime. It is a historical key,
not a control token. `local_pid` separately carries observer namespace device,
inode and local TGID; it must not replace the key or authorize PID-based actions.
No conversion of kernel UID/start to local UID/procfs ticks is implied.

`associate(exact_key, startup_entry_id)` binds ONLY that retained child. Supply
the exact key from a report using independent launch evidence or an explicit user
declaration; do not select by list position, process name or bare PID. IDs are
1..32768 bytes, no NUL. A repeated identical binding succeeds without duplication;
rebinding to another ID or using a foreign capture/boot/start/UID key is refused.
It can label a retained record after child/creator reaping and after close. A
failed capture can still have valid retained historical rows, which remain labelable
by its owner. Copies of the returned report are ordinary caller-owned values.

Until association, `startup_entry_id` has no value. Explicit bindings have
Support::estimated: CALLER DECLARATION, not OS proof of executable identity,
successful exec, a desktop-file match or automatic startup. The observer never
inherits an app ID from its creator or attributes every child of a session manager
to the same app. `automatic_startup_origin` always remains unknown in this provider.

This is a usable public raw-observation/association API, NOT a conversion into
StartupImpactObserver samples. Kernel monotonic times are not host steady_clock
timestamps; no invented procfs start_ticks, synthetic60-second sample or complete
window is manufactured to feed the older classifier. CPU is group scheduler
birth-to-release time, excluding waited-child CPU; IO includes waited-child IO.
There are deliberately NO automatic tree/application sums or low/medium/high
classifications. Total process lifetime is not necessarily its first60seconds.

## Diagnostics

Report quality_flags retains native bits:1 birth read failure,2 capacity loss,
4 duplicate birth,8 final failure,16 duplicate final,32 PID-mapping failure.
They do not count exact lost processes. publishing is transient snapshot overlap,
not a guessed zero or measured loss. pid_mapping_enabled/note say whether mapping
was attempted; disabled mapping does not by itself erase CPU/IO. Known zero is
retained as an engaged metric value. Failed snapshots never silently commit half
their rows or erase previous associations.

## Verification and remaining work

Linux CTest142/142 including new startup-birth-observer;30 focal repetitions,
MSVC10 pure fixture runs (including non-Linux authorized fallback), installed SDK
consumer rebuilt with the public header and safe consent-denied construction.
Initial misleading-indent warning was fixed; the full-context fixture's braced
optional initialization compile error was corrected. Initial logs are preserved,
not represented as clean successes. Final builds/tests are the acceptance evidence.
MSVC D9025 from NDEBUG->UNDEBUG is intentional to keep Release assertions active.

Fixtures exercise explicit binding/idempotence/conflict/foreign key, PID reuse,
zero/final/partial/PID unavailable, atomic batch rejection, exceptions, context
change during read, and SAME caller PID with changed namespace/start/boot/inode.
Native manual target now checks public and internal sensors on the SAME selected
PRIVATE creator, independently attached before child creation. Across9scenarios,
15 public rows match authenticated internal birth identities,10 final CPU/IO rows
match counters and5 expected missing finals stay absent. Bindings persist after
creator reap, replay and close; conflicting IDs are refused, origin stays unknown.
Both captures close and FD census returns to baseline. Native data includes an init
with selfPID1 in a private namespace-for-children, no procfs mount/system change.
Logs: work/startup-birth-api-*. No remote CI or universal kernel/hardware guarantee.

I07 remains E: reliable app/exec/launch evidence and automatic-startup cause,
integration with time-bounded impact/recorder/history without mixed clocks or IO
double counting, further descendants and other reapers. No persisted format was
changed and these reports are not silently saved/merged into app history.
