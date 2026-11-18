# Shared backend in the Linux public API

Revision 97: [capacity for 1,024 processes](LINUX_NETWORK_SCALE.en.md), RAW
batches of up to 64, structural capacity for 16,384 buckets, and a native
test with 65 private processes in two batches. The limits and single-batch
costs below describe historical checkpoint 93.

Revision 94 follow-up: [standalone strict admission](LINUX_NETWORK_ADMISSION.en.md)
connects inventory, selection owner, and capture. An unchanged set preserves
rates and epoch. It rejects unknown entries and selections over 64 without
filtering. Native validation used only two private processes; full capture
was NOT authorized or validated, and Monitor/Sampler admission was not yet
automatic. The revision 93 pending items below are historical.

Revision 93, 2026-10-22. Public
`LinuxProcessNetworkSelectionMode::observed_namespace_buckets` now uses
`LinuxSharedSelectedNetworkSampler` and its shared admission batches.
No new mode, signature, public layout, dependency, or build flag. Constructing
either backend does not open pidfds or load probes.

## Connected paths

Backend selection happens inside LinuxProcessNetworkSampler's Pimpl:

- Observed mode: start/sample/update use the authenticated shared producer.
- Default strict mode: the previous per-row provider, unchanged.
- Declared-cookie mode: the previous per-row provider, unchanged.

Stop closes both branches unconditionally; only the selected branch has
active resources. Move transfers the Pimpl and its mode; the moved-from
object does not capture. Windows are published through the SAME validated
projection, without bypassing identity, domain, rate, quality, empty-actor
metadata, or UID-sum checks. Projection rejection stops the selected capture.

Monitor delegates to this sampler and publishes SystemSnapshot.linux_process_network.
Sampler retains its existing serialization across worker/update/latest/history;
there is no additional capture thread. Mode is selected during setup while
the worker is stopped and cannot change during capture. SelectionOwner
prepares input PID/start/pidfds without capturing and can release its copies
after successful start/update.

The include flag only includes the report; it does NOT authorize eBPF.
Opt-out, stop, and worker exit close capture. Restarting the worker or
re-enabling the flag does not rearm it. Explicit setup with consent is
still required.

## Semantics and cost

Public coverage remains instrumented TCP/UDP payload by actor and SOCKET
namespace. At revision 93, input cookie is zero, with 1–64 actors, 16 cookies
per actor, and at most 1,024 buckets. This is not a full socket inventory,
link-layer traffic, global user sums, or causal app attribution through a
socket owner. A positive TCP send return does not establish delivery or ACK;
uninstrumented operations do not become zero. Idle actor metadata does not
create a fabricated bucket.

Nominal windows are estimated, with per-batch consistency and an overall
sweep of at most one second, not an atomic cut across batches. Quality loss
in any batch rejects the ENTIRE report. Fresh pidfd joins cover ALL actors
before and after the sweep and each update, with no cached permissions or
PID/UID conversion across domains.

Retained actors keep their pidfds, capture, and counter origins; removed
actors are no longer admitted. New and readmitted actors use NEW batches.
Slots a running probe could still write to are never recycled. Readmitted
actors do not include gap bytes. Every successful update, even a reorder,
creates a new epoch and resets ALL rates. See
[authenticated updates](LINUX_NETWORK_SHARED_UPDATES.en.md) and
[safe removal](LINUX_NETWORK_SHARED_RETENTION.en.md).

Native FD counts after each operation: N pidfds, six query FDs, and 8G
traffic FDs, with G nonempty batches and 1 <= G <= N <= 64. Initial cost is
N+14, not one probe set per actor. Selection changes can fragment capture
into up to 64 batches; a constant three probes are not promised. Proposals
can temporarily keep up to 64 new pidfd copies alongside earlier ones.
Queries and BTF can use additional transient FDs. Empty batches close.

Rejection rules do NOT silently become uniform in this revision. Standalone
preserves capture for invalid input shape or missing consent. Monitor retains
its earlier wrapper, closing capture after a failed authorized update,
including invalid input shape. Sampler inherits that behavior, but the worker
continues. Denied consent changes no path. Runtime update failure closes all
capture. An absent sample clears rates and does not publish zero.

## Verification

Logs: `work/network-shared-public-*`. The official tracker records final
results. Public activation is not inferred merely from internal projection.
Native manual tests use existing temporary authorization, ONLY private
children and loopback, and time guards. No permanent configuration,
external collection utilities, installation, or global capture.

The 17-process capacity test validates the PUBLIC observed provider with
N+14 FDs, four lanes, a nominal interval, and UID sums. Sampler/Monitor/worker/
owner scenarios validate additions, retention, gaps, readmission, epochs,
FDs, closed input descriptors, stop/restart, denial, fork, exited processes,
and no rearming through the flag. One real network namespace was used.
The 64-actor/1,024-bucket cases are fixtures, not universal native certification.

Final revision 93: Linux Release 130/130 without skips and three focused
tests repeated 20 times each (60 runs); the installed consumer was rebuilt
and relinked and exited 0. MSVC passed 3/3 focused tests and 60 repeat runs.
No new C++ warnings; D9025 was expected. Two initial builds were mistakenly
launched by the assistant in the SAME directory and collided in ranlib,
producing a truncated or missing archive. Logs were retained. Later builds
were serialized by directory, and all final results passed. This is not a
new code issue or a resolution of the revision 75 Windows consumer
0xC0000409 failure in N03. Revision 93 mirror confirmed: comment
90140272305719. The TXT remains authoritative.

F03/H04/J04 remain open at this checkpoint. Activation does not automatically
admit all visible processes or integrate network data into per-app history
or session totals. The caller selects an explicit set. Future admission
policy must respect capacity, privileges, and identity without silent
truncation.
