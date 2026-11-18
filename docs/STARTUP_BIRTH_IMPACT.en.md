# Impact projection for retained short-lived children

Checkpoint116,2026-10-29. Public header `pcinsight/startup_birth_impact.hpp` adds
`measure_startup_birth_impact(report, exact_key, window, policy)`. It consumes an
already retained `LinuxStartupBirthObserver` report, including after reaping or
close. It never reads the system, starts collection, associates an entry, launches,
elevates, saves or updates Monitor/recorder/legacy observers. No external program
or new runtime dependency is used. Pure fixtures also compile on non-Linux; that
does not add a Windows capture provider.

## Identity and scope

The exact capture/boot/kernel TGID/real UID/start-boottime key must identify ONE
retained row. A current PID, executable candidate, name or parent association is
not accepted instead. The row must already have an explicit startup-entry binding
(for example, a successful observer.associate call). Missing binding is not_found,
foreign/missing key stale_target, malformed data/options invalid_argument.

The output carries that declaration as estimated, even if an input claims higher
confidence. `automatic_startup_origin` stays unknown regardless of input claims.
This measures a single process-group observation, not the complete app, process
tree or causal cost of an actual automatic startup. Multiple children bound to
one app are NOT summed: group IO can already include waited-child IO. Captures,
receipts and explicit bindings are not process-control authorization tokens.

## Birth-relative window, not a clock conversion

Default window:60 seconds, configurable >0 and <=1 hour. The start is the retained
kernel birth-monotonic timestamp; deadline is checked uint64 start+window. This is
a PER-PROCESS birth-relative window, NOT60 seconds from boot/login/recorder start.
No fabricated procfs start_ticks, local PID, std::steady_clock time point or
time-namespace conversion is passed to the legacy impact observer. Kernel
start_boottime is part of the identity only, not a substitute for the time axis.

A release timestamp <=deadline (including equality) exposes the retained CPU
delta and a checked read+write disk sum as estimated observed lower bounds. CPU
remains the release-observed group scheduler counter, not guaranteed post-final-
scheduler-switch lifetime CPU. IO includes waited-child accounting, not exclusive
physical IO. No before/after subtraction is repeated: birth rows ALREADY contain
the birth-to-release deltas. Known zero stays a present zero.

Absent release, disabled recovery or a release AFTER the deadline leaves BOTH
window resources unknown, even if a late lifetime counter is known. No clipping,
proportional interpolation or rate-based estimate assigns late work to the window.
The release timestamp is still exposed as historical metadata. Unknown CPU stays
unknown; read OR write unavailable makes disk unknown. Read+write overflow makes
disk unknown, not wrapped or saturated, while other valid evidence can survive.

## Classification is deliberately high-only

The existing StartupImpactPolicy defaults apply: high CPU strictly >1 second OR
disk strictly >3000000 bytes (SI). Equality is not high, matching the existing
classifier. A high measured lower bound can dominate an unknown other resource.
Everything else stays unknown: incomplete coverage cannot certify medium/low,
even when both observed values are zero or fall below medium thresholds. Policy
ordering/positive thresholds are validated, but medium thresholds do not enable
medium classification. No window_complete flag is manufactured from release.

Accepted historical rows may still yield a high lower bound after capture failure
or global loss flags; that does not restore complete coverage. The pure function
validates the fields it consumes, not live capture authenticity or current boot.
Callers must obtain trusted reports; it is not a fresh identity/permission check.

```cpp
#include <pcinsight/startup_birth_impact.hpp>
// key is the exact retained child key; associate(key, entry_id) was explicit.
const auto report = observer.report();
const auto result = pcinsight::measure_startup_birth_impact(report, key);
if (result && pcinsight::metric_is_known(result.value->impact)) {
    // Only an estimated high observed lower bound is returned here.
    // This is not proof that entry_id automatically launched this process.
}
```

## Verification and remaining work

Pure fixture tests cover exact identity/binding, default/custom window, deadline
equality/late/pending, zero, strict thresholds, one unavailable resource, overflow,
permission propagation, malformed metrics/options, no mutation, no app/tree sum
and failure/loss-retained data. The native private harness projects real retained
rows AFTER reaping and explicit binding, checks exact resource correspondence and
unknown uncovered finals. A CUSTOM1ms CPU high threshold provides positive short
workload evidence; the default threshold is unchanged and no low/medium is claimed.
All existing native identity/source/candidate/replay/cleanup checks remain.

Linux144/144 without skips plus20 final focal repetitions; MSVC10 pure fixture
repetitions; installed Linux SDK/consumer exit0. The native13-case harness passed
23 retained projections (18 with release resources,5 absent finals kept unknown),
with all previous exact-source/default-independent/candidate/replay/FD/cleanup
checks still active. Linux/MSVC C++ builds clean; MSVC D9025 for asserts intentional.
Evidence: SEGUIMIENTO.txt and work/startup-birth-impact-*.
This is a bridge from birth capture to bounded resource classification, not a
complete recorder integration: reliable launch receipt/app/automatic-startup cause,
other reapers/descendants and durable projection/overlay still need work. I07 and
N03 remain open. No archive format change, early-host activation, GUI or Windows
implementation was performed.
