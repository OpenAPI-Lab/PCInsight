# End-to-end reference startup capture

examples/startup_capture.cpp connects Monitor, native candidates, the
observer, and StartupImpactStore into an executable flow. It builds as
pcinsight-startup-capture when PCINSIGHT_BUILD_EXAMPLES=ON (the default).
This is an example consumer included with the project, **not** an external
app the library needs to run or install. The GUI can use the same APIs
and its own scheduler without this executable.

```text
pcinsight-startup-capture FILE CAPTURE_ID [DURATION_MS]
pcinsight-startup-capture --help
```

Explicit invocation accepts **estimated** associations from GIO markers
for the current UID. It does not enable .desktop entries, interpret Exec,
launch applications, or change login configuration. It is not a daemon,
service, or installed startup recorder. Linux uses existing native sources.
Windows builds and displays help, but capture returns code 3 (provider
unavailable) without writing a file.

## Flow and window

The example validates arguments and the destination and directory, and
refuses to overwrite an existing file that StartupImpactStore cannot load.
The directory must exist. A valid existing file is replaced with the new
capture; there is no merge, backup, or retention. That preliminary check
is not compare-and-swap against other writers. Use a private path and
serialize invocations as described in [startup store](STARTUP_STORE.en.md).

It takes a snapshot with process and startup data and its own identity
context, discovers candidates, registers baselines, and begins the timed
phase **after** that initial scan. The initial identities thus receive
a full window. The default is 60 seconds, with a configurable integer
duration from 10 ms to one hour. Every 250 ms it discovers only identities
not yet registered and polls. Closed identities are not registered again
and CPU is not counted twice. Late processes have shorter windows at
phase end. Unknown, partial, or a high lower bound is retained rather
than inventing complete low or medium.

At the end it polls once more, closes, and saves a closed capture with
explicit recorded_at and capture_id. Native calls can be slow. Warmup,
discovery, I/O, and save do not have cancellation or an absolute deadline.
Missed polls are not accumulated; the next scan is scheduled from the
current clock. No data is written automatically before the final save.
The example does not interpret SIGINT as safe closure or saving, or alter
signals in the consuming process.

## Interpretation

Output summarizes observations, individual complete windows, partial
scans, inaccessible attempts, and rejected registrations. Those discovery
counts diagnose the executable run; they are not a persisted certification
of global boot coverage. In the file, window_complete means the process
was sampled for its window without gaps, not that every app was found or
launched automatically. The GUI should display the file's context,
historical date, confidence, and scope.

GIO markers can come from a manual launch and are not present on every
desktop. The .desktop version and launch cause are not verified. An empty
capture means no acceptable candidates were found, not zero impact.
Changed PID, token, or context is not attributed to the old observation.
If a process disappears between polls, earlier samples remain with a
gap; its missing usage is not reconstructed. This executable is not the
parent of observed apps and does not use waitid or reap other processes.
[Child exit sampling](STARTUP_CHILD_EXIT.en.md) is for supervisors that
own those children.

Exit codes: 0 capture saved; 2 invalid arguments; 3 native provider or
context unavailable; 4 path, previous load, or save failed (the new file
may already have been published if durability confirmation failed);
5 exception or execution failure, with no completeness claim. A capture
can save despite partial scans. Its limits still apply when the process
returns 0.

## Verification

Linux Release passed 35/35 and Windows Release passed 45/45, with
installed packages and consumers run. The Linux test launches **only**
this consumer and its own C++ fixture with markers set before exec,
using private XDG, directory, and file paths. The real 300 ms flow finds
the exact identity, obtains an estimated delta, closes the window,
saves, and loads the file. The .desktop remains identical and its Exec
never runs. Invalid durations and empty IDs do not overwrite the
capture; a corrupt file is preserved; a missing directory is rejected;
and a new empty capture replaces without accumulating rows. The test,
not the consumer, reaps all test children. Help works on both systems.
Windows also returned code 3 without writing a file.

Remaining work includes early recorder startup through explicit
consumer-chosen integration, reliable provider-specific autostart cause,
and capture of other short-lived processes and descendants. This example
does not complete startup impact.
