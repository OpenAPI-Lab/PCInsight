# Visible process inventory for network selection (no capture)

Revision 88: the [gated query](LINUX_NETWORK_QUERY_SESSION.en.md) passed the full
17-actor native test, including rates and cleanup. This does not certify a
shared producer or capture of 64 real actors.

Historical revision 87: [structural capacity for 64 and actual cost](LINUX_NETWORK_CAPACITY64.en.md).
The 16-process/256-row limit below is historical; the limit of 16 cookies per
actor does NOT change. The 17-actor native run failed because of cost and
bounded target lifetimes. It did NOT certify scalable capture. The original
evidence for each revision is retained.

Revision 86 adds pcinsight/linux_network_inventory.hpp:
read_linux_network_process_inventory(consent) and the strict bridge
linux_network_inventory_candidates. No BPF, sockets, signals, external
commands, helper processes, elevation, or persistent descriptors. F03 remains
open at this checkpoint: an inventory is not general capture and does not
remove the collector's then-current 16-process limit.

## View and availability

The inventory enumerates ONLY /proc as visible to the current caller. It pins
the directory, verifies it is procfs through fstatfs, and checks it against the
observer's device/inode. Valid numeric entries are sorted by PID, without
silently deduplicating. The limit is 100,000 entries. Exceeding the limit or a
readdir failure yields no inventory, NOT a partial vector. It does not follow
arbitrary alternative roots or procfs mounts. enumeration_completed means the
sweep reached EOF, not an atomic snapshot, a host-wide inventory, or a
guarantee that no processes are hidden.

Namespaces, mounts, and permissions can restrict the view. Invisible processes
do not produce rows and are NOT reported as nonexistent. Processes can appear
or exit during the sweep; two enumerations are not combined to fabricate a
stable census. The observer's boot/procfs/PID and user namespace identities
are checked at both endpoints, using a CLOCK_MONOTONIC bracket and a
cooperative 5-second budget. An estimated or backwards clock, observer change,
exhausted budget, incomplete source, or exception rejects the ENTIRE inventory.
There is no hard deadline or cancellation while a syscall is in progress.

## One row per PID found, including failures

For each PID, the inventory opens a temporary pidfd, reads stat/start, and
verifies them using the existing pidfd/procfs reader: the fdinfo Pid projection
in the thread's FD table, the pinned process directory, consistent stat/status
UID data, and namespaces. Polling before and after inspection detects exit.
The pidfd closes before the next row. The inventory does not duplicate sockets
or read network contents. Results do NOT guarantee future liveness.

Public row states:

- verified: the PID/start candidate and observer_real_uid are known/available.
- permission_denied: Metrics are absent with permission_required; the reason is preserved.
- disappeared: pidfd_open reports the process absent or poll confirms exit.
  Metrics are absent/unsupported; no invented ticks or UID 0.
- unavailable: another incompatible identity or source, or a failure that
  cannot be classified as exit or permission denial. The reason is included;
  not every failure means the process exited.

The real UID is in the procfs OBSERVER's domain, not the initial kernel UID,
a resolved local user, or a session. A known UID 0 is valid. Do not compare it
numerically with the BPF report. candidate contains only local PID/start.
The selection owner and collector verify liveness and identity again at
preparation or capture, without converting ticks to nanoseconds.

An available overall snapshot means the visible sweep and context checks
finished. It does NOT mean every row has a known identity. Each row has its
own Metrics. A per-PID error is not omitted and does not invalidate other
verified PIDs; a structural inconsistency in the provider DOES reject the
whole result.

## Bridge without silent filtering

linux_network_inventory_candidates requires a present, available inventory,
a completed enumeration, valid metadata and brackets, exact ordering and
identities, and verified state for EVERY row. It rejects unknown, denied, or
exited rows rather than selecting only the ones it could read. At this
revision it rejects MORE THAN 16 processes with a capacity reason, even if
the inventory can report many more. It does not return the first 16 or rotate
silently. Known empty yields an empty list: stop capture rather than passing
an empty selection to start/update.

```cpp
#include <pcinsight/linux_network_inventory.hpp>

auto inventory = pcinsight::read_linux_network_process_inventory(true);
// Display each row's state and reason. Do not substitute zero for absence.
auto candidates = pcinsight::linux_network_inventory_candidates(inventory);
if (candidates.value) {
    pcinsight::LinuxProcessNetworkSelectionOwner owner;
    auto prepared = owner.refresh(true, *candidates.value);
    // When known and nonempty, owner.selections() gives BORROWED pidfds and cookie zero.
    // BPF setup/update requires SEPARATE consent and duplicates those pidfds.
    // Do not use a temporary owner if its rows will outlive it.
}
// If the source or bridge fails, do not claim total coverage. Stop capture
// or EXPLICITLY keep a different selection policy without calling it the new inventory.
```

Neither include_linux_process_network nor this read starts capture or
automatically updates it. This revision does not integrate automatic admission
in Monitor/Sampler. The caller can define a declared subset with its own
explicit coverage; do not modify the inventory and still label it the
complete visible set. Preparation and capture always revalidate PID/start;
an old sample can be stale immediately. Call from the GUI's worker. Consumers
can link from their IDE without setting up app CMake before the GUI; the
library retains its internal build. Outside Linux, an authorized call returns
unsupported. False consent returns permission_required without inspection.

## Evidence

The new linux-network-inventory test keeps Release assertions active.
It checks canonical ordering, UID 0, metadata, empty results, a positive
bridge with 16 entries, and a positive inventory with 17 entries that the
bridge rejects. It preserves rows with denied, exited, and unavailable states,
including mixed known and unknown results. Twenty-one failures cover source,
clock/budget, context, duplicates/range, 100,001 entries before inspection,
foreign identity, estimated quality, and exceptions. Inconsistent error rows
are rejected. The bridge rejects 15 altered inventories involving metadata,
quality, incomplete results, identity, ordering/duplicates, UID, or state.
CTest does not load BPF.

Manual `--native-readonly`: a REAL sweep of visible /proc, including
read-only identity checks for unrelated processes. Two private IPC children
are verified by PID/start, supplied to the pidfd owner as an EXPLICIT private
subset, and released. The bridge returns ALL entries from the real inventory
if within capacity, or absence with a reason, never a silent subset. Both
children exit normally and FDs are cleaned up. No root, BPF, signals to the
target, or traffic capture. Parent/child watchdogs are 20 s/15 s, not hard
syscall deadlines. Positive denial and exit cases are fixtures; the test
does not claim to have forced hidepid, a real exit race, or multiple namespaces.

The first build had one warning about compact indentation. It was fixed and
the log kept. Linux Release 128/128 without skips, including the new inventory
test; two tests repeated 20 times; installed consumer rebuilt, linked, and
exiting 0; and a clean final build. MSVC passed 2/2 focused shared-code tests
without new C++ warnings (expected D9025 keeps assertions enabled). No new full
Windows suite or consumer run. The revision 75 consumer failure remains N03,
so full compatibility is not claimed. Invalid-stat reasons were corrected so
an earlier success note would not explain absence. Logs: work/network-inventory-*.
The header, types, functions, and test are additive. No existing layout,
GUI, persistence, dependency, CLI, permanent configuration, or global capture
changes.

Next at this checkpoint: automatic admission and removal, and a capture
architecture that scales beyond 16 actors with explicit capacity and quality.
Do not close F03 on the strength of a read-only inventory. H04/J04 remain
open. Do not expand traffic tests beyond authorized private processes without
new authorization.
