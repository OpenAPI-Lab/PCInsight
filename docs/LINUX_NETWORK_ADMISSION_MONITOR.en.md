# Visible process admission and coverage in Monitor/Sampler

Revision 97: [expanded capacity](LINUX_NETWORK_SCALE.en.md) supports 1,024
processes and 16,384 buckets. Scaling beyond 64 was still pending in revision 96,
as described below.

Revision 96, 2026-10-24. F03 integration: the explicit admission path from
revisions 94/95 now reaches Monitor, SystemSnapshot, and Sampler's worker and
history. This does not close F03/H04/J04: scaling beyond 64 eligible processes,
validation, and coverage were still pending.

## Public usage

```cpp
#include <pcinsight/monitor.hpp>
#include <pcinsight/sampler.hpp>
using namespace pcinsight;
SampleOptions options;
options.include_linux_process_network = true;
Monitor monitor;
// ONLY with consent for all eligible processes in the current view:
auto prepared = monitor.refresh_linux_visible_process_network(
    true, LinuxNetworkAdmissionPolicy::verified_only);
auto snapshot = monitor.sample(options);
// snapshot.linux_process_network: ONLY network data for admitted actors.
// snapshot.linux_network_admission: policy, counts, exclusions, epoch, and times.
monitor.stop_linux_process_network();

Sampler worker(std::chrono::seconds(1), 61, options);
auto setup = worker.start_linux_visible_process_network(
    true, LinuxNetworkAdmissionPolicy::verified_only); // ONLY while stopped.
worker.start();
// Each later refresh is EXPLICIT and authorized, with the worker running:
auto changed = worker.refresh_linux_visible_process_network(
    true, LinuxNetworkAdmissionPolicy::verified_only);
auto latest = worker.latest();
worker.stop();
```

Always check the returned Metrics; the example does not guarantee permissions
or capacity. Overloads without a policy use strict_all on EVERY call and do
not inherit partial mode. There is no new flag: include_linux_process_network
only includes the report and NEVER enumerates, authorizes, or rearms by itself.
Monitor setup does not require the flag, because options are passed to sample.
Sampler requires flag=true in its construction options.

## Exclusivity and lifecycle

Earlier manual selection and visible admission are separate paths. Do not
start one while the other is configured or active; stop first. Rejection of
mixed paths does NOT replace or stop existing capture. Denying consent does
not change either path. Moving Monitor transfers its mode and ownership;
sampling the moved-from object does not grant authorization.

Monitor.refresh can explicitly set up or refresh visible admission.
Sampler.start_linux_visible_process_network sets it up only while stopped.
Sampler.refresh_linux_visible_process_network requires a running worker
and successful prior preparation of visible admission, even if the known
inventory had no eligible actors. An unprepared worker cannot set it up
implicitly.

Opting out with include=false, stopping, a general Monitor exception, or
worker exit closes capture and its owner and clears the mode and preparation.
Restarting the worker or re-enabling the flag does NOT rearm capture. A failed
authorized refresh closes capture, but the worker continues to publish
absent results with reasons. Another EXPLICIT refresh can retry the previously
prepared visible path while the worker runs. The sample loop does not retry
or enumerate automatically.

## Paired reports

SystemSnapshot.linux_network_admission is a new Metric with metadata from
the LAST successful refresh. It is NOT a fresh census at sample time or
host-wide coverage. It is ABSENT on the manual path. A visible sample's
metadata and network values come from the SAME provenance envelope and
corresponding epoch.

The adapter checks policy, times, limits, consistent counts/exclusions, PIDs
and reasons, metadata equality with the stored refresh, observed coverage,
actor count, and epoch. Excluded processes cannot appear as network actors
or rows. Foreign rows or excess capacity reject both reports. The lower-level
provider retains its full projection, identity, and counter checks.

A sample failure or exception rejects BOTH reports, closes resources, and
removes stale metadata rather than displaying obsolete coverage. Resources
close before exception diagnostics are constructed. Successful publication
is prepared before assignment and moved without throwing. If a valid refresh
has no eligible actors, metadata remains available (there may still be
visible processes and exclusions), network stays absent rather than zero,
and sampling does not call the network provider.

## Worker and history

The implementation reuses lifecycle → Monitor → history lock order and the
request-before-lock handoff. A refresh announces that it is waiting before
competing with the worker, avoiding starvation under an unfair mutex. It does
not wait for Monitor while holding the history lock. The previous sample is
committed to history BEFORE refresh can return. Each entry keeps its own
paired metadata and network data. Refresh does NOT erase earlier history.

An unchanged eligible set preserves the epoch and rate baselines; only
refresh metadata changes. One epoch can therefore have different admission
times, reasons, or policies. Inspect each entry. A changed set gets a new
epoch and resets all rates; do not connect chart series across epochs.
latest/history can retain an earlier sample while the next one is pending.
Check the returned epoch when selection changes. Known metadata does not
imply known network data.

Persistence is unchanged, and this does not integrate H04 app history.

## Compatibility and verification

The 0.x SystemSnapshot layout adds linux_network_admission. REBUILD the
library and ALL consumers. The existing network report layout does not
change, and legacy process/user totals are not overwritten. The installed
consumer links the new methods and tests denied consent without full capture.

Fixtures cover adapter/Monitor lifecycle, move and opt-out. Corrupt metadata,
inconsistent epochs/coverage/selection, excluded or foreign network PIDs,
and invalid quality reject both reports. With no eligible actors, metadata
is preserved without invented network data. Tests also cover concurrent
refresh and history reads while the worker runs, failure without worker
shutdown, restart without rearming, and rejection of setup with the flag off.

Native testing used ONLY two private children and a synthetic marker that
was not queried in the OS. It used native procfs/pidfds and the public BPF
provider. Monitor checks paired snapshots, UDP, nominal intervals, an integer
oracle, move, and opt-out. Sampler checks AB → unchanged AB → B → AB,
concurrent refresh/history, paired epoch and metadata, retained origins and
readmission without gap bytes, strict default closing both reports while
the worker continues, and stop/restart without rearming. FDs were cleaned
up and all children exited normally. The 45 s/40 s watchdogs were not extended.

The test uses internal PER-OBJECT injection to restrict ONLY the enumerator.
It uses the production public methods, lifecycle, and locks. There are no
global hooks or public provider overrides. Test contexts outlive Monitor and
Sampler; the worker exits before they are destroyed. The test did NOT capture
the full /proc view or expand private eBPF authorization.

Revision 96 verification: Linux 133/133 without skips and five fixtures
repeated 20 times (100 runs); installed consumer rebuilt, linked, and exiting 0.
Two MSVC fixtures passed 20 runs each (40 runs) without relaxing assertions.
An initial MSVC assertion failure is retained in targeted-msvc.log. Its cause
was not isolated, so do not attribute it to stale objects as fact. It is distinct
from the installed revision 75 consumer failure in N03, which remains open.
The Linux build recorded transient WSL clock-skew warnings on generated files
(0.012–0.12 s); logs were retained. The clock-recheck-linux.log rebuild exited 0
without warnings. Revision 96 ClickUp mirror confirmed: comment 90140272936245.
The TXT is authoritative. Logs: work/network-admission-monitor-*. Final results
and mirror status are in the TXT. At this revision, more than 64 eligible
processes were rejected without taking the first 64, truncation, or rotation.
Do not infer universal host support or closure of F03, H04 app history, or
J04 aggregates.
