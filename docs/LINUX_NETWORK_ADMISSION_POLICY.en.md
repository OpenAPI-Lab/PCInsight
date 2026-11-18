# Explicit partial policy and admission provenance

Revision 97: [expanded capacity](LINUX_NETWORK_SCALE.en.md) supports 1,024
processes and 16,384 buckets. The limits of 64 and the scaling work below
are historical.

Revision 96: [Monitor/Sampler integration](LINUX_NETWORK_ADMISSION_MONITOR.en.md)
now carries admission and exclusions alongside network data in SystemSnapshot
and worker history, with explicit refresh and no rearming by flag or restart.
The pending integration below describes revision 95; scaling beyond 64,
validation, and coverage were still pending at revision 96.

Revision 95, 2026-10-22. This F03 unit integrates a pure planner and the opt-in
`LinuxNetworkAdmissionPolicy::verified_only` policy into standalone admission.
The earlier overload uses `strict_all` on EVERY call. It does not inherit
partial mode from a previous refresh. F03/H04/J04 remain open at this checkpoint.

## Decide without capturing

`plan_linux_network_admission(inventory, policy)` makes no syscalls and does
not modify capture. It validates the ENTIRE inventory before deciding: metadata,
exact quality, PID/start/UID, ordering, and consistent states, support, and
error notes. Partial policy cannot conceal a corrupt row or estimated or
incomplete inventory.

An available plan Metric means the DECISION is valid, NOT that capture is
possible. Also check `plan.value->candidates.value`:

- strict_all: every encountered identity must be verified.
- verified_only: admits every row with verified procfs identity and explicitly
  excludes permission_denied/disappeared/unavailable, preserving each PID,
  state, and original reason in `excluded`. It does not ignore other failures.
- More than 64 eligible processes at this revision: candidates is ABSENT and
  the set is rejected in full. No first 64, rotation, preference ordering, or
  silent removal. Counts and reasons remain available so the GUI can explain
  the rejection alongside the original inventory.

`visible_processes` counts all rows; `verified_processes` counts the verified
ones. `all_encountered_verified` describes identity quality only, not capture
availability. It can be true for 65 verified processes with candidates absent.
None of these fields turns a restricted procfs view into a host-wide census.
A plan is a decision about the supplied inventory, not a reservation or
authorization. A later refresh reads the current view again.

## Capture with explicit coverage

```cpp
#include <pcinsight/linux_network_admission.hpp>
pcinsight::LinuxVisibleProcessNetworkSampler network;
// Requires consent for ALL eligible processes in the current view.
auto changed = network.refresh_visible(
    true, pcinsight::LinuxNetworkAdmissionPolicy::verified_only);
if (changed.value && changed.value->capture_active) {
    auto frame = network.sample_with_admission();
    // frame.value->admission.excluded: PID, state, and reason for each exclusion.
    // frame.value->network: ONLY data from actors actually admitted.
}
network.stop();
```

`LinuxNetworkAdmissionChange` includes policy, counts, exclusions, epoch, and
refresh timestamps. `complete_encountered_set` is true only when a successful
refresh excludes no encountered rows. It does not claim coverage of hidden
processes, later births, or the entire host. If there are exclusions but no
eligible actors, it returns capture_active=false with all exclusions and stops.
It does NOT present the visible inventory as empty or generate zero rates.

`sample_with_admission()` pairs the LAST successful refresh report with the
network snapshot at the corresponding epoch. It does NOT enumerate again or
update exclusions at sample time. The earlier `sample()` still returns only
selected network data; its note indicates PARTIAL coverage and the exclusion
count when applicable and points to the envelope with provenance. Excluded
rows receive no invented buckets, payload, rates, or UID sums.

An unchanged eligible PID/start set preserves epoch and baselines even if
exclusion reasons or policy change. Stored provenance DOES update. Membership
changes retain revision 94's counter retention and fresh readmission behavior.
Report copies are prepared before commit, and context and clock are checked
afterward. Copy and sampling exceptions close resources.

The policy addresses ONLY inventory quality. A verified procfs identity does
NOT guarantee eBPF/pidfd_getfd permission. Runtime owner/provider failures,
counter quality, clocks, context, and callback failures still close
EVERYTHING, with no retry that silently filters capture actors. There is no
elevation, new permission, global test capture, or new default policy for
arbitrary selection.

## Compatibility and evidence

The 0.x `LinuxNetworkAdmissionChange` layout was expanded. REBUILD the library
and ALL consumers of that API. SystemSnapshot and LinuxProcessNetworkSnapshot
layouts do not change. The installed consumer tests overloads, the envelope
with provenance, and the planner symbol with denied consent and no capture.

Fixtures cover 64 and 65 verified processes, 67 rows with 64 verified and
three excluded, all excluded, strict/partial/invalid enum, and 20 corrupt
inventories that are not hidden. They cover mixed/unknown/PID-reuse refreshes,
changed exclusion reasons with the same epoch, samples with provenance,
strict default not inheriting partial mode, and controller failures and exceptions.

Native testing uses ONLY two private children and a synthetic disappeared
row. The children use native procfs/pidfds and public BPF; the marker does not
query an unrelated PID. Partial refresh preserves epoch, rates, and the count
of 26 owned FDs. The envelope reports three visible, two verified, one excluded,
and ONLY two network actors. Real UDP data retain their origin and omit gap
bytes for the readmitted actor. A later default strict refresh rejects the same
row and closes EVERYTHING. Earlier counts were 18 → 16 → 26. Fork leaves the
parent intact; restart, empty results, FD cleanup, and child cleanup are checked.
The deliberately restricted enumerator is NOT proof or authorization of full
/proc capture or capture of other users. One real namespace was tested, not
coverage of 64 real actors.

Logs: `work/network-policy-*`. Linux Release 132/132 without skips, five
focused tests repeated 20 times each (100 runs), and the installed consumer
relinked with the new API/header and exiting 0. Two focused MSVC tests ran
20 times each (40 runs); no full Windows support or N03 resolution is claimed.
Final builds had no new C++ warnings; D9025 is expected. Revision 95 mirror
confirmed: comment 90140272317289; SEGUIMIENTO.txt remains authoritative.
Initial warnings about new fields without explicit defaults were fixed and
the first build was preserved. No new packages, CLI, daemon, configuration,
or GUI.

At this checkpoint, the next work is Monitor/Sampler admission and provenance
integration and scaling past 64 eligible processes. This unit diagnoses the
limit but does NOT remove it. Authorized real-view validation and physical
and cross-distribution coverage remain pending. H04/J04 retain separate
app-history and user criteria.
