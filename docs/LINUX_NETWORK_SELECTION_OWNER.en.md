# Selecting by PID and start time without managing pidfds manually

Revision 88: the [gated query](LINUX_NETWORK_QUERY_SESSION.en.md) passed the full
17-actor native test, including rates and cleanup. This does not certify a
shared producer or capture of 64 real actors.

Historical revision 87: [structural capacity for 64 and actual cost](LINUX_NETWORK_CAPACITY64.en.md).
The limit of 16 processes and 256 rows cited below is historical; the limit
of 16 cookies per actor is unchanged. The 17-actor native run failed because
of cost and bounded target lifetimes. It did NOT certify scalable capture.
Each revision's original evidence is retained.

Revision 86 follow-up: [visible inventory with per-PID quality](LINUX_NETWORK_INVENTORY.en.md)
and a strict bridge without silent filtering. It neither increases the
16-process limit nor starts capture.

Revision 85 adds LinuxProcessNetworkSelectionOwner in
pcinsight/linux_process_network_selection.hpp. It prepares and OWNS pidfds
for a complete list of PID/start_ticks candidates supplied by the caller's
policy. It does not enumerate processes, capture network traffic, or authorize
probes. It helps manage additions and removals; it is NOT a finished general
provider. F03 remains open.

## Contract

refresh(consent,candidates) accepts 0–16 distinct processes with known start
times and valid PIDs. No PID-only fallback, invented ticks, truncation to the
first 16, or hidden rotation. A list of 17 candidates is rejected BEFORE
descriptors are opened. A policy that wants all visible processes must detect
and report insufficient capacity. It cannot claim global coverage from this
limited selection.

The list is sorted by PID, and pidfds are retained for exact PID/start matches.
An identical or reordered list is revalidated but changes neither generation
nor changed, avoiding unnecessary rate resets. An addition opens a pidfd; a
removal closes the owned reference when the new list is committed. The same
PID with a different start counts as removal and addition, never reuse by PID
alone. The initial list, additions, removals, and replacements change
generation, which is NOT the collector's selection_epoch. These are distinct
tokens.

refresh validates boot and the observer's local identity before, during, and
after the operation. It inspects pinned processes and rechecks ALL of them
after acquiring the list, so it can detect the first process exiting while
the last is being prepared. The native reader verifies genuine procfs, the
pidfd's Pid projection in the thread's fdinfo table, the pinned directory,
stat PID/start, status UID, and namespaces. Poll before and after rejects
an exited pidfd. Local IDs are not translated into the kernel domain; the
downstream collector still performs BPF identity verification when it
receives the selection.

False consent, malformed input, or exceeded capacity does not mutate a
prepared list. Runtime failure, estimated or denied reads, process or context
changes, acquisition failure, or exceptions clear ALL owned references with
no partial publication. New references are managed with RAII even if a later
actor fails. A forked copy rejects valid refresh/selections calls and closes
its inherited references, not the parent's. The API is movable but not
copyable. Injected internal contexts must outlive the owner.

An empty list is VALID. It reports changes/removals and a known empty
selections result. Do not pass it to collector start/update, which require
1–16 entries at this revision; stop the collector instead. clear and
destruction release resources. Preparing a new list requires fresh consent.
Authorized preparation outside Linux returns unsupported, with no new
Windows backend.

## Borrowing and capture: ownership is not authorization

selections returns a copy of the vector, but its FDs are BORROWED until the
next refresh, clear, or destruction. Rows use cookie zero for observed mode.
Serialize owner calls and use of the borrowed references in the same FD table.
Do not close or reuse those FDs externally. The result does not guarantee
future liveness; a process can exit immediately. The collector verifies
identity and liveness again and duplicates the pidfds during start or when
adding entries on update, retaining independent references.

Consequently, clearing the OWNER or an owner failure does NOT stop capture
that already started. If policy or refresh fails, the caller must EXPLICITLY
stop or update the collector. Never present its old selection as a new complete
list. Permission to prepare pidfds is not permission to start or update BPF.
Every capture setup/update retains its existing consent rules.

## Caller reconciliation example

```cpp
#include <pcinsight/linux_process_network_selection.hpp>

bool reconcile(pcinsight::LinuxProcessNetworkSelectionOwner& owner,
               pcinsight::LinuxProcessNetworkSampler& network,
               std::span<const pcinsight::LinuxNetworkProcessCandidate> candidates,
               bool prepare_consent, bool capture_consent, bool& active)
{
    auto prepared = owner.refresh(prepare_consent, candidates);
    if (!prepared.value) { network.stop(); active = false; return false; }
    auto rows = owner.selections();
    if (!rows.value) { network.stop(); active = false; return false; }
    if (rows.value->empty()) { network.stop(); active = false; return true; }
    if (active && !prepared.value->changed) return true;
    auto status = active
        ? network.update_selection(capture_consent, *rows.value)
        : network.start(capture_consent, *rows.value);
    active = status.value.has_value();
    if (!active) network.stop();
    return active;
}
```

Construct network with observed_namespace_buckets. The list must come from
a source with known identity, availability, and coverage. Do not silently
omit an absent ProcessInfo.linux_start_ticks or pass off a partial inventory
as complete. Reconcile and sample from the caller's worker without blocking
the GUI. Linking from an IDE works without requiring CMake before the GUI.
This revision does not automatically integrate with Monitor/Sampler or
enumerate /proc.

## Evidence and limits

The new linux-network-selection test keeps checks enabled in Release. It
covers consent, malformed input, duplicates, zero start, out-of-range PIDs,
and rejection of 17 candidates without touching the source; 16 valid
candidates and valid FD 0; retention, reordering, stable generation,
removal/re-addition/replacement, empty lists, and clear. Thirteen runtime
failures cover acquisition, the second actor, the first actor on recheck,
contexts, estimated quality, exceptions, boot, and namespace, with complete
cleanup. Simulated fork, move, and missing APIs are also covered. The 16
actors are fixtures, not measurements of 16 real processes.

Manual `--private-children`, without BPF or root: two private children wait
for IPC. The test acquires two pidfds, checks retention and reordering without
a new generation, active move, fork rejection without affecting the parent,
and removal/re-addition. Stale start and actor exit invalidate the entire
owner; it recovers with the remaining actor and then an empty list. No FD
leaks. The successful path does not signal the target, duplicate sockets,
or inspect traffic. Parent/child watchdogs are 20 s/15 s; private children
are cleaned up if the test fails.

Authorized manual `--authorized-private-owned-selection` connects
preparation to the PUBLIC BPF API for two private children. It closes the
original pidfds and passes the owner's rows; start duplicates them, so
clearing the owner does not cut off capture. Revision 83 oracles are reused:
empty and late metadata, four rates and UID sum, actor A TCP 12,288/12,288,
UDP 768/768, and 24 peeks, with twice that for B. The test covers two threads
per actor, parent exclusion, fork, update, removal/re-addition and gaps,
exit, restart, and FD cleanup. Only ONE real namespace was shared. The
children initialize SO_NETNS_COOKIE for the oracle; no cookie is passed to
the producer. This does not test general capture or multiple real namespaces.
Watchdogs are 60 s/50 s.

The first build emitted three misleading-indentation warnings. They were
fixed without changing the contract, and the initial log was retained.
Logs: work/network-selection-*. Linux Release 127/127 without skips,
including the new selection test; two tests repeated 20 times; installed
consumer rebuilt, linked, and exiting 0; and a warning-free final build.
MSVC passed 2/2 focused tests with no new C++ warnings; expected D9025 keeps
assertions active. No new full Windows suite or consumer run. The revision
75 consumer failure remains N03; full compatibility is not claimed. Positive
BPF evidence is limited to the two private children. No new layouts for
existing types, GUI, persistence, dependency, CLI, namespace or permanent
configuration changes, or global capture. F03/H04/J04 remain open here.

GENERAL selection still needs an inventory source with explicit coverage,
automatic admission/removal policy, and an architecture that scales beyond
16 actors without concealing loss or capacity. Do not close F03 on the
strength of pidfd management alone.
