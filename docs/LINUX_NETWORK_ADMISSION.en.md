# Strict admission of the visible Linux inventory

Revision 97: [expanded capacity](LINUX_NETWORK_SCALE.en.md) supports 1,024
processes in RAW batches of up to 64. The earlier limits below are historical.

Revision 95: [opt-in partial policy and provenance](LINUX_NETWORK_ADMISSION_POLICY.en.md)
adds verified_only, explicit exclusions, a planner, and a sample envelope.
Strict remains the default. At that revision, more than 64 eligible processes
still caused rejection without truncation. The expanded 0.x
LinuxNetworkAdmissionChange layout requires rebuilding the library and
consumers. The partial-policy limitations below describe revision 94.

Revision 94, 2026-10-22. New standalone public path:
`LinuxVisibleProcessNetworkSampler` in `pcinsight/linux_network_admission.hpp`.
It connects inventory, strict policy, pidfd ownership, and the shared observed
provider. There is no manual PID selection or execution of external utilities.
This revision does not close F03/H04/J04 or enable automatic admission in
Monitor/Sampler.

## Usage and consent

```cpp
#include <pcinsight/linux_network_admission.hpp>

pcinsight::LinuxVisibleProcessNetworkSampler network;
// ONLY with consent to capture the ENTIRE visible inventory found:
auto admission = network.refresh_visible(true);
if (admission.value && admission.value->capture_active) {
    auto frame = network.sample(); // Metric<LinuxProcessNetworkSnapshot>
}
network.stop();
```

Construction does not enumerate processes, open pidfds, or load eBPF. Every
refresh needs fresh consent; `false` neither queries sources nor changes
existing capture. Sampling NEVER enumerates, authorizes, or rearms. There is
no thread or automatic polling: the caller decides when to refresh membership
and when to sample. Stop closes the owner and capture. Move transfers ownership;
the moved-from object cannot sample.

## All-or-nothing policy

The native source sweeps the observer's visible /proc. The inventory preserves
the reasons for unverified rows. The strict bridge rejects the ENTIRE set if
any row is denied, disappeared, or unavailable. More than 64 actors is rejected
at this checkpoint without filtering, truncation, or rotation. No PID is
retained without a known start time. A known empty inventory successfully stops
capture with epoch zero. It does NOT establish that there are no hidden
processes, processes in other namespaces, or processes anywhere on the host.

A failed authorized refresh, including capacity failure, stops ALL capture.
It does not present an older selection as the new view. The implementation
checks observer, boot, procfs, and namespace changes and PID reuse. The owner
authenticates pidfds at both endpoints, and the provider authenticates kernel
associations. Initial UID is not inferred from local UID. Context and clock
are checked before and after admission. Inventory and commit must fit within
five seconds from inventory start. This is a cooperative budget, not syscall
interruption, a hard deadline, or asynchronous cancellation.

## Preserve rates when a refresh changes nothing

The owner compares PID/start pairs in canonical order and revalidates processes
and contexts. If the set is unchanged, there is NO hidden provider update or
sample. The epoch and all baselines are preserved even if input order changes.
The provider still checks current eBPF/ptrace access at sample time; the
owner's procfs revalidation is not treated as cached eBPF permission.

A changed set uses the public shared update for additions, removals, and
readmission. Retained actors keep their counter origins; new and readmitted
actors start fresh without gap bytes. A new epoch resets all rates. The same
PID with a different start time is a replacement, not retention by PID alone.
The owner costs N additional pidfds: after each native operation, 2N+6+8G FDs
(initially 2N+14), where G is the number of live batches. Proposals, queries,
and BTF may require additional transient FDs.

Sample validates the epoch and coverage and returns the estimated public
report. Any sample absence or error stops EVERYTHING in this strict path,
unlike the low-level sampler's recovery from some transient absences. Another
consented refresh can start a NEW capture. Sample and the flag never rearm
capture, and old counters are not silently continued.

## Coverage and evidence

The /proc view is dynamic and may be restricted. A process born during the
sweep or after commit is not guaranteed to be included until another refresh.
The 64-process limit does not guarantee full-machine performance or coverage.
A desktop with more processes or insufficient permissions gets a strict error,
not zero traffic. Sampling remains instrumented TCP/UDP payload with 16 cookies
per actor and partial initial-UID sums, not causal app/session/user totals or
link-layer traffic.

Fixtures cover start, unchanged and reordered sets, changed sets, PID reuse,
empty results, restart, 64 accepted and 65 rejected, denied/disappeared/unknown
rows, changed boot/procfs/namespace/caller, stale inventory, failure or exception
in EVERY start/update operation, and sample quality/epoch/exception/fork.
No consent means no queries; an unchanged set triggers no hidden update or sample.

Native testing deliberately limits the test enumerator to TWO private children,
while using native procfs/pidfds and the PUBLIC eBPF provider. AB → unchanged AB
→ B → AB checks that an unchanged refresh preserves rates and epoch, retained
actors keep their origin, and readmission excludes the gap. Owned FD counts
were 18 → 16 → 26 with the owner included. Tests cover fork rejection without
affecting the parent, a synthetic unavailable row stopping EVERYTHING, explicit
fresh restart, empty inventory stopping capture, normal child exits, and FD
cleanup. SO_NETNS_COOKIE initializes only the oracle for the single real
namespace. The unavailable row is an injected marker that does NOT query an
unrelated OS PID. Positive capture admits only the private children.

`refresh_visible(true)` was NOT run against the user's full session. Earlier
eBPF authorization covered private processes ONLY. The full-view public path is
tested with injection and fixtures, and the installed consumer tests denied
consent. This is NOT global capture validation and does not expand permission.
No installations, daemon, permanent configuration, or new dependencies.

Logs: `work/network-admission-*`. Final Linux Release 131/131 without skips,
four focused tests repeated 20 times each (80 runs), and an installed consumer
rebuilt and relinked with the new header/API, exiting 0. The new MSVC fixture
passed 20 repetitions; the final private native run passed. No new C++ warnings;
D9025 is expected. No full Windows compatibility or N03 fix is claimed. Resources
close BEFORE formatting exception diagnostics. Revision 94 mirror confirmed:
comment 90140272312863; SEGUIMIENTO.txt remains authoritative. The initial build
requested the target before regenerating CMake and had no rule; configuration
fixed it. An initial build detected 0.75 s of WSL timestamp skew. Its log was
retained and a later final build verified, without attributing it to code or N03.

At this checkpoint, F03 still needs a useful policy for large or inaccessible
views without hiding coverage, authorized real-view validation, and
Monitor/Sampler admission integration if appropriate. H04/J04 retain separate
history and session criteria; this standalone path does not close them.
