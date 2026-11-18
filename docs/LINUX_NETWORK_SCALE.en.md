# Linux selections larger than 64 processes

Revision 97, 2026-10-24. The public structural limit increases to 1,024
processes (`LinuxProcessNetworkSampler::max_processes`). Each process's
registry retains a limit of 16 observed namespaces, for up to 16,384 buckets.
This is not unlimited support or a promised refresh rate. Rebuild the library
and consumers because a public constexpr changes; report layouts do not.

## Batches and continuity

In observed mode, each RAW producer still accepts at most 64 actors. The
complete selection is split into disjoint batches of up to 64. For N
processes, startup uses ceil(N/64) batches, with three programs, three links,
and two maps per batch, plus one shared gated identity query. The public
backend owns N pidfds, for an initial count of N+6+8*ceil(N/64) persistent FDs.
Visible admission also retains N pidfds in the selection owner, for a total
of 2N+6+8*ceil(N/64). The library does not change RLIMIT_NOFILE, memlock,
capabilities, or configuration to accommodate the selection.

An update removes old routes BEFORE adding replacements. Retained actors
keep their producers and counters without restarting. New and readmitted
actors use new maps, never retired slots or counters from the excluded
interval. Additions are also split into batches of up to 64. After selection
changes, there can be G fragmented batches with 1 <= G <= N, not always
ceil(N/64). The persistent cost is N+6+8G (or 2N+6+8G with admission), NOT a
constant three probes regardless of updates. A changed selection creates a
new epoch and resets all rates, not retained counters. An admission refresh
with the same set preserves the epoch and rates. Each program still runs
its batch filter on a network operation, so cost grows with G. Sampler's
in-memory history also grows with the number of buckets and retained samples;
size it in the caller. A full 16,384 buckets at 1 Hz is not promised to be
suitable for every machine.

Per-batch consistency and a complete nominal sweep of at most one second
are retained, NOT an atomic snapshot across batches. Visible admission keeps
its cooperative five-second budget, not a hard deadline or an automatic
extension for larger sets. Failure to admit a later batch closes ALL batches
and pidfds rather than publishing a prefix. Inconsistent, denied, or
budget-exhausted reads do not invent zeros or rates. Invalid shape or excess
capacity in manual selection leaves the prior capture intact. A failed
authorized visible refresh closes EVERYTHING under the existing contract.

Strict and declared modes retain per-row producers, with much higher FD and
probe costs, even though the shared structural limit is 1,024. They do not
inherit the observed mode's costs or native validation. Declared mode counts
PID/start/cookie rows, not unique processes. No mode uses external CLIs.

## Admission and consumers

The selection owner, strict inventory bridge, strict_all/verified_only
planner, public projection, and Monitor/Sampler use the same capacity.
More than 1,024 eligible processes rejects the ENTIRE set, without taking
the first 1,024, truncating, or rotating. The inventory can still describe
up to 100,000 entries. Explicit exclusions do not consume capture slots,
but remain visible with PID, state, and reason. Raising the limit, creating
objects, or setting flags does not automatically start capture.

## Verification

Bounded fixtures cover 65/129/1,024 actors in batches of 64, up to 16,384
buckets, four rates per bucket and UID sums, removal/readmission, retained
counter origin, gaps, epoch reset, and rejection of 1,025 without truncation.
Failure or an exception when creating a later batch during start/update must
close earlier resources. Owner, inventory, controller, policy, projection,
and Monitor/worker tests verify the new limit with Release checks active.

The native harness accepts the explicit flag `--authorized-private-capacity65`
for manually selected private children only, with no global /proc enumeration
or unrelated processes. It uses 60 s/55 s watchdogs and cleans up children
and FDs. The flag does not itself constitute user authorization. The harness
also tests native admission for the same private set by injecting ONLY the
enumerator. It checks the existing budget, combined owner/capture FD count,
paired metadata and network results, and unchanged refresh preserving rates
and counter origin. Final results and `work/network-scale-*` logs are recorded
in the TXT. Fixtures do not certify 1,024 real processes or heterogeneous
namespaces.

Revision 97 results: Linux 133/133 without skips, seven fixtures repeated
ten times each (70 runs), and seven MSVC fixtures repeated ten times each
(70 runs). The installed consumer was rebuilt, linked, and exited 0 with
assertions for 1,024 processes and 16,384 buckets. Its first build found an
old 1,024-bucket assertion; it was updated to the new contract without
weakening the check, and the initial log was preserved. Final builds had
no new C++ warnings; D9025 was expected.

Native private 65-actor PUBLIC capture and restricted admission PASSED:
87 FDs for capture and 152 with the owner, in two batches. The test checked
per-actor UDP counters and rates, the UID sum, matching epoch and metadata,
unchanged refresh retaining the baseline, parent exclusion, clean FD counts,
and normal child exits. The 60 s/55 s watchdogs and 1 s/5 s budgets were NOT
extended. Final harness timestamps: public capture started at 1,242 ms and
stopped at 2,679 ms; admission started at 4,206 ms and stopped at 5,678 ms.
These are not latency guarantees. The native 17-actor regression also passed
for windows, updates, TCP/UDP, IPv4/IPv6, gaps, and fork. Logs: native65-final
and native17-regression. This does not test the full /proc view or imply
broader authorization. At this revision F03 remains open for live-view
validation and coverage; temporary WSL permission was requested and not yet
received. The TXT takes precedence over ClickUp. Revision 97 mirror confirmed:
comment 90140272937319; historical title and states were unchanged.
