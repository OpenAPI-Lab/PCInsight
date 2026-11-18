# Observed Linux network traffic by user (J04)

Revision 101, 2026-10-24. J04 network data is published in
`SystemSnapshot::linux_user_network`, independently of session inventory.
Disk data stays in `UserSession::linux_io_details`; see
[user I/O](LINUX_USER_IO.en.md).

`pcinsight/linux_user_network.hpp` exposes the pure function
`summarize_linux_user_network(frame)` and the `LinuxUserNetworkReport`
and `LinuxUserNetworkSample` types. It projects an observed-mode F03 sample
without starting capture, installing tools, running commands, querying
NSS, or accessing the bus. It introduces no libsystemd/logind or runtime
package requirement.

## Identity: an observed UID is not a session

Each row represents **one** real UID in the F03 **observer's** user
namespace, not the initial kernel UID. The domain key includes the boot
token and the observer user namespace's device and inode. Do not use
a UID alone across hosts, boots, or namespaces. The report retains the
F03 observer, producer, epoch, and sweep brackets. In Monitor/Sampler it
comes from the **same** frame as linux_process_network. For inventory
coverage, consult the paired linux_network_admission when present.
Manual selection does not establish coverage of all host processes.

The projection uses `row.process.real_uid`, whose procfs/kernel
association F03 verifies through pidfds and current joins before and
after the sweep and across windows. `initial_real_uids` retains the
distinct initial UIDs mapped to that observed UID. There can be more
than one, for example in a restricted view or with an overflow UID.
The group is not necessarily one globally unique account. The
projection does not automatically translate F03 `users` totals from
the initial domain.

The users vector is sorted and unique by UID. Two sessions with the
same UID do not create two rows or double-count their bytes. A
registered session is not required: root, services, and tasks without
a login can appear. The report does not identify active or connected
sessions or the causal or exclusive owner of sockets.

Legacy network fields in UserSession/linux_io_details remain absent.
login1 reports a UID and utmp a name, but neither proves the same
domain as procfs, for example when a host bus is exposed to a container.
The provider does not join by username or numeric match without that
evidence. The new report **is** the authoritative per-user network API.
The GUI should display UID, domain, and coverage separately, rather
than presenting an unverified session association as verified. A
GUI-supplied account name or label does not change the numeric key or
justify adding session rows.

## API and units

```cpp
#include <pcinsight/monitor.hpp>
#include <pcinsight/linux_user_network.hpp>

pcinsight::Monitor monitor;
// Configure observed F03 capture FIRST, with explicit consent.
// Use observed_namespace_buckets manually or authorized visible admission.
pcinsight::SampleOptions options;
options.include_linux_process_network = true; // Read-only; never authorizes BPF.
options.include_linux_user_network = true;   // Does not require session inventory.
options.include_processes = false;
options.include_user_sessions = false;
auto snapshot = monitor.sample(options); // Call periodically from the GUI.
if (pcinsight::metric_is_known(snapshot.linux_user_network)) {
    for (const auto& user : snapshot.linux_user_network.value->users) {
        // user.real_uid, partial_send/receive/total_bytes_per_second,
        // selected_processes, idle_processes, observed_buckets, known/missing.
        // Check Metric/Support and coverage; do not convert absence to zero.
    }
}
```

`include_user_sessions=true` also requests the projection if F03 reads
are enabled, without filling each session's network fields. Use the
dedicated flag to avoid the session inventory and its optional
dependencies. Neither flag starts BPF or elevates privileges. Stopped
capture, a disabled flag, or F03 failure leaves the report absent; it
does not retain an old rate as current. Turning off only this report
does not stop F03 capture requested for other uses.

With a standalone `LinuxProcessNetworkSampler`, pass its result directly
to `summarize_linux_user_network`. This is a projection, not authentication
of caller-fabricated data; use a trustworthy native F03 frame. The strict
and declared modes explicitly do not provide this per-user projection.

The four lanes are TCP send, TCP receive, UDP send, and UDP receive.
Each value is payload bytes/s from instrumented operations, not on-wire
bytes, ACKs, active time, or universal traffic. Known bucket rates are
summed after F03 has **already** rounded them down. Adding rounded rates
can differ from rounding a sum of bytes; no extra precision is claimed.
Send plus receive counts both loopback endpoints and does not remove
causal duplicates between different processes.

Each user row publishes:

- selected_processes: distinct actors, not bucket or session count.
- idle_processes: actors in the complete registry that do not yet have
  buckets. Metadata without traffic does not become a zero rate.
- observed_buckets: distinct process/cookie buckets, each added once.
- known/missing[4]: partition observed_buckets by lane. permission_denied
  is a subset of missing. These do not quantify invisible or unselected
  processes.
- partial_payload_bytes_per_second[4]: the known sum for each lane,
  with overflow[4].
- partial_send/receive/total_bytes_per_second: independent partial sums
  of known lanes, with send/receive/total_overflow. An absent lane is
  not filled with zero. If none is known, the aggregate rate is absent.

A **measured** F03 zero stays known. A first bucket or baseline, an actor
without buckets, or missing permission is not measured zero. Missing
rates leave only the known partial sum and its coverage, without
extrapolation. Overflow leaves the affected sum absent without wrapping
or saturation, retains coverage counters, and does not clear valid
independent results. UID 0 is valid.

## Validation and limits

Before publishing, the projection verifies observed mode, boot, observer,
epoch, producer, bounded and ordered brackets, unique actor identities,
PID/start/UID/inode, complete kernel association, unique cookies,
bucket counts and sequences, counter and rate presence and Support, and
a nominal interval matching the previous bracket's interval. Global
malformation rejects the whole frame rather than publishing partial
groups. An absent frame's permission status is retained.

F03 structural capacity is 1,024 actors, 16 cookies each, and 16,384
buckets. That is not a latency or frequency guarantee. Aggregation has
bounded memory and adds no baselines or persistent state. Replaying
a frame does not accumulate rates. A new epoch shows the new frame's
coverage and baseline, not old rates. F03 validates UID, process, and
context changes across windows. Failures are not filled with adapter
rates, socket_uid, or counters from another namespace.

The new 0.x SystemSnapshot/SampleOptions layout requires rebuilding
the library and **all** consumers. The SDK can be linked from the IDE
without setting up the GUI's CMake early. History files, the action
virtual ABI, and Windows functions are unchanged.

## Verification

`tests/linux_user_network.cpp` covers UID 0, distinct UIDs, many-to-one
mapping from the initial domain, measured zero versus unknown and idle,
partial rates and permissions, independent send/receive/total, overflow,
two cookies for one actor, repeated sessions without double counting
or fabricated joins, replay, 15 invalid frames, capacity at 1,024/16,384,
and flags without authorization or implicit capture. Release retains assertions.

`tests/linux_network_admission_native.cpp --authorized-private-admission`
uses only two private UDP children and real Monitor/Sampler, joins,
pidfds, and BPF. It compares the observer UID, two actors, two buckets,
four lanes, and send/receive/total rates against an exact oracle for
rounded rates. It verifies paired epoch and observer, first bucket
and idle without invented zero, and a report available **without**
session inventory. Sampler history keeps metadata and actor counts
across AB-to-B-to-AB, concurrent refresh, failure, stop, and restart.
No global capture was used; probes were removed, children exited
normally, and FDs were restored. The test also retains the H04
regression for real bytes and explicit persistence.

The final result and official count are in SEGUIMIENTO.txt, with logs
under work/linux-user-network-*. This does not certify multiple real
namespaces or every distribution. Mapping and domain variants are
covered by fixtures, not presented as hardware validation. MSVC
compatibility covers the pure projection, not a Linux Monitor on
Windows. An initial trial also ran Windows Monitor and aborted with
0xC0000409 after the pure fixtures passed, as recorded in the diagnostic
log. No cause or fix is claimed. The fixture's native integration checks
are now limited to __linux__, as intended. No Linux assertions were
removed, and Windows code did not change. N03 retains that risk
separately from this portable pure function.
