# I07: decision needed on early startup execution

Revision 128 update: after “continue” following the revision 127 options,
work proceeds conservatively with the recommended observer scope and
Linux K03. That reply was not interpreted as authorization for a new
launcher. I07 and its original criterion remain open; unverifiable
causation is unknown or deferred. This does not claim that the user
authorized managing their apps or mark the task complete because its
priority changed. The first K03 unit is the explicit protocol in
[runit provider](LINUX_RUNIT.en.md).

## Revision 127: causal limits and a new scope decision

The recorder architecture authorized in revision 103 is already implemented,
including the manual end-to-end integration in revision 126. It does not
need authorization again. The new question is different: observing launches
initiated elsewhere versus becoming their launcher.

The [XDG Autostart specification](https://specifications.freedesktop.org/autostart/latest/)
defines location, precedence, and conditions for the desktop to launch
entries. That document does not define an authenticated causal execution
record. [systemd Desktop Integration](https://systemd.io/DESKTOP_ENVIRONMENTS/)
describes opt-in XDG integration and application units. Those include
ordinary apps, not just autostart. Unit names, active sessions, proximity
to login, and GIO markers therefore are not elevated to automatic proof.
This is a conclusion about the reviewed sources' scope, not a claim that
all desktops lack more specific sources.

A custom host that also selects and launches configured applications can
verify its own managed pass and connect receipts to resource readings.
That changes the product's role from the approved observer recorder.
Even such a host can be invoked manually. Its presence alone does not
prove that the OS started it automatically, and that case will not be
labeled automatic-origin.

Decision requested from the user, without choosing the UI's preselected option:

1. Recommended: keep the library an observer, retain unknown causation when
   unverifiable, defer that part of I07, and continue Linux K03. Do not close
   I07 or change its criterion or percentage before the response.
2. Develop an opt-in custom launcher and its managed pass, using private
   tests. Do not install it, enable login startup, replace the desktop,
   launch real apps, or automatically claim that the OS started the host.
   Those actions would need later specific authorization. This also would
   not guarantee coverage of other apps or descendants.

Revision 127 is an audit and decision checkpoint, not a new implementation.
No tests or probes were run, and code, formats, and real startup were not
changed. The latest build and suite evidence remains revision 126
(Linux 154/154 + 30, MSVC 20, SDK 0). I07 remained E; C60/E9/P4/B2 with
3X meant 80% closed and 20% open, without partial credit or a new ID.

---

Revision 103 update: the user replied “continue” to the specific proposal
to develop the second option without installing or enabling it.
The development architecture was decided: an optional host and reusable API
were implemented; see [startup recorder](STARTUP_RECORDER.en.md). This did
not authorize real login startup or close I07. The revision 102 text
below is retained as evidence of that decision checkpoint.

Revision 102, 2026-10-24. Audit of the existing implementation, without
capture, installation, app execution, or changes to the user's login.

## Evidence at this checkpoint

- StartupImpactObserver measures deltas from an existing baseline.
  The caller supplies its association. It does not begin work before
  construction and invocation.
- discover_startup_process_candidates reads GIO markers for the caller UID.
  These are estimated declared evidence, not proof of automatic startup causation.
- examples/startup_capture.cpp coordinates scans, polling, and storage while
  running. It is not an installed recorder or active before invocation.
- read_startup_child_exit_sample recovers final counters for the caller's
  own unreaped children with an earlier baseline. It cannot recover other
  apps that have disappeared.
- apply_startup_impact_observations explicitly links results; it is not
  early capture.

Reviewed sources: [reference capture](STARTUP_CAPTURE.en.md),
[candidates](STARTUP_CANDIDATES.en.md), [observer](STARTUP_OBSERVER.en.md),
[child exit](STARTUP_CHILD_EXIT.en.md), [overlay](STARTUP_OVERLAY.en.md),
and the corresponding headers and linux_startup_candidates.cpp/startup_observer.cpp.

## User choice pending at this checkpoint

A library that is not running cannot retrospectively get counters for
processes that have already disappeared. To capture before the GUI opens,
a host must be active in time, or an already active source must retain
the required identities and counters. At this checkpoint PCInsight had
no such early producer, and the distribution was not assumed to keep
that data.

Architecture alternatives, neither enabled nor selected automatically:

1. The future consumer executable or GUI starts with the session and
   initializes the library before showing a window. This needs no extra
   executable. It cannot measure activity before its own launch or
   guarantee priority over every app.
2. An optional minimal recorder from the same project starts with the
   session and stores data for the later GUI. This would be a separate
   PCInsight process, not a third-party app or external telemetry package.
   The architecture requires approval, and enabling real startup requires
   separate approval.

Neither alternative alone proves autostart causation, captures all apps
or descendants, observes other short-lived processes, or supports all
desktops. After selecting a host, the evidence provider or launcher,
coverage, consent, and private native tests must be defined. A marker
wrapper or installation of a .desktop file does not close I07.

The current restriction is Linux first, with no external collection
utilities or automatic dependency installation. Do not create services
or login tasks, edit the user's XDG configuration, launch real apps,
capture other UIDs' environments, or extend prior private eBPF permission
to global monitoring without authorization.

## Status at this checkpoint

I07 remained E. No new IDs or partial credit were added. The official
count was 60/75 (80.0%) closed and 15/75 (20.0%) open. This revision was
an audit and decision checkpoint, not a new implementation. Suites were
not repeated and earlier tests were not relabeled as current results.
An early-host choice was still needed before integration that would
change the consumer's architecture or dependencies.
