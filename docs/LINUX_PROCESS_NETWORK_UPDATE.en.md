# Changing the explicit Linux network selection

Revision 76 allows adding, removing, and reordering processes without recreating
captures whose local PID, start_ticks, and socket namespace cookie are unchanged.
The input is a full replacement list, not a list of differences or automatic
discovery. The limit of 1–16 processes and one declared cookie per process still
applies at this revision.

```cpp
// network already has an active capture. Each call needs fresh consent.
auto changed = network.update_selection(consent, new_selection);
// Equivalent in Monitor, serialized with sample/start/stop:
auto other = monitor.update_linux_process_network_selection(consent,
                                                           new_selection);
```

These are ADDITIVE methods on LinuxProcessNetworkSampler and Monitor. This
revision does not change public layouts. Consumers using layouts from before
revision 75 still need to rebuild. Link the current library to use the new
methods; the consumer project does not need CMake.

## Identity, resources, and failures

The selection must contain 1–16 distinct PIDs, nonnegative pidfds, and nonzero
start_ticks and cookies. Retained rows use their OWN previously duplicated
pidfd, not the caller's FD number, which may have been closed after startup.
For additions and replacements, the borrowed pidfd must stay valid during the
call. It is duplicated and validated with the same kernel/procfs checks as
start. Changing start_ticks or the cookie closes the old capture and starts a
new one. Removed rows are not revalidated, so a process that has exited or become
inaccessible can still be removed. Retained rows are revalidated; identity
continuity is not assumed.

False consent leaves resources unchanged. The standalone provider rejects a
malformed list before modifying capture. Once reconciliation begins, any
denial, inconsistent identity or context, error, or exception closes ALL
captures. It does not silently revert to the previous selection. Monitor is
stricter: ANY authorized update failure, including a malformed list or absent
provider, stops capture and preserves the failure note in the report. Do not
use update to start a stopped session. For an empty selection, use stop rather
than presenting an empty report as available.

Removed captures close before replacements are created. This is NOT an atomic
swap and does not guarantee gap-free capture. Retained captures keep counting
during reconciliation; new ones count only from their own start. The span and
borrowed pidfds are not retained. No new thread, automatic elevation, CLI,
package, service, or capture of other processes.

## Windows and counter origin

Every successful update creates a new selection_epoch and discards ALL
baselines and brackets, even when reordering or repeating an identical list.
The first following sample has counters but NO rates or interval. The second
can produce rates again. Do not calculate deltas across epochs or data gaps.

Retained actors' payload_bytes_since_start counters keep the origin of THEIR
capture; changing selection_epoch does not reset them to zero. New actors start
their own captures, with no retroactive traffic from the gap. The UID sum
remains a partial, estimated sum of the current selection, not the account's
total traffic. Legacy process/app/session fields are not populated.

Monitor calls must be serialized. Revision 77 adds a
[worker-safe update](LINUX_PROCESS_NETWORK_WORKER.en.md) to Sampler under the
same update_linux_process_network_selection name; start does not reconfigure.
The stopped-worker, explicit setup, and restart-without-reauthorization
contract is unchanged. At this checkpoint, F03 still needs discovery and
dynamic policy, namespace buckets, and broader coverage; H04/J04 remain open.

## Evidence

The existing linux-network-multi and linux-process-network-monitor CTests keep
Release assertions enabled. They cover AB→B→AB, retention without recreating
FDs, reordered and identical lists, cookie changes, denial and invalid lists
without callbacks, failures involving additions/revalidation/kernel duplicates/
context/fork/exceptions, mandatory restart after failure, and removal of an
inaccessible row. The reducer sorts by key; associations preserve selection
order. An initial assertion that assumed reducer order was corrected without
changing that contract. The initial targeted log was retained.

Manual `--authorized-private-update-window` on WSL, NOT CTest: only two
previously authorized private children, neither reporting a kernel token.
Monitor uses the public API to exercise AB→B→AB and reordering, closure of
removed FDs, retained counters, exclusion of A's traffic during the gap, four
actor/UID payload and rate oracles, new epochs and baselines, denial without
changes, rejection of updates after fork, recovery of A after removing exited
B, and cleanup without FD leaks. The initial measured interval was 2.215693931 s,
not a cadence guarantee. Parent/child watchdogs of 60 s/50 s are not hard
deadlines for BPF operations. See work/network-update-native-linux.log and
build/test logs with the same prefix. This does not validate other kernels,
distributions, hardware, or global capture.

Linux Release 126/126 without skips, two modified tests repeated 20 times
each, a clean full build, and an installed external consumer rebuilt, linked,
and exiting 0. No new CTests or partial closures: 57/75 = 76.0%, with
18 open = 24.0%. The revision 75 general MSVC consumer's native failure
remains unexplained and N03 stays open; full Windows compatibility is not
claimed. Shared MSVC compatibility at revision 76: the library and two
focused tests built, and 2/2 passed with no new C++ warnings (D9025 from
/UNDEBUG retaining assertions is expected). The full Windows suite and
general Windows consumer were not rerun for this revision.
