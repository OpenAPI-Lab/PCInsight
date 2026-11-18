# DRM counters in the GPU clock domain

The DRM pipeline accepts drm-cycles-N / drm-total-cycles-N pairs in addition
to drm-engine-N times in ns. This supports records published by Intel Xe
while retaining PID/start_ticks, descriptor, client, and PCI checks.
It does not open a GPU, create workloads, or require external programs.

## Public data and GUI

LinuxDrmEngineUsage.counter_kind distinguishes nanoseconds from cycles.
For cycles, busy_cycles and total_cycles hold the raw values and their
availability. The legacy busy_nanoseconds field is **not** valid in this
case: its initial zero is neither a time measurement nor a conversion.
Always check counter_kind. If the driver reports both ns and a
busy/total cycle pair for one engine, ns is preferred and the cycle
counters are retained as metadata without mixing them.

The per-client percentage based on cycles is:

    100 * delta(busy_cycles) / delta(total_cycles) / capacity

capacity is the reported value, or one if absent. Deltas are computed
before converting to floating point, avoiding overflow in uint64
subtraction. The calculation does not depend on CPU time between
samples; it can run even if two steady_clock timestamps are equal.
A backward CPU clock still invalidates that comparison for safety.

Aggregates explicitly separate the counter bases:

| counter_kind | Field containing the observed sum |
|---|---|
| nanoseconds | summed_client_active_percent |
| cycles | summed_client_cycle_percent |
| cycles_max_frequency ([maximum frequency](LINUX_DRM_MAXFREQ.en.md)) | summed_client_frequency_percent |

The other field is not published as zero. Clients with the same engine
but different bases or capacities are not summed. The sum remains an
estimate for visible clients, not global GPU utilization, and can exceed
100. shortest/longest_interval describe available CPU intervals, not
cycles or a shared window. They can be absent in a valid cycle sample
with equal CPU timestamps.

## Incomplete samples and resets

A first sample or change in counter basis, capacity, or identity creates
a new baseline without a percentage. If the denominator does not advance,
the value stays absent and the previous busy-cycle anchor is retained so
that increases observed before the GPU clock advances are not lost.
A decreasing denominator creates a new epoch and baseline rather than
inventing continuity or unwrapping the counter. If busy cycles decrease
while the GPU clock advances, retain the high-water mark and wait for
recovery. Deltas above capacity times denominator stay absent rather than
being capped at 100. A zero busy delta with a positive denominator does
produce a measured zero.

The bounded parser retains the same uint64, name, line, and record rules
and 128-engine limit. It rejects incomplete pairs, duplicates, units on
cycle fields, signs, overflow, orphan denominators, and total-cycles
together with maxfreq for one engine. The
[drm-maxfreq alternative](LINUX_DRM_MAXFREQ.en.md) uses a separate typed
basis, explicit unit conversion, and frequency-normalized percentages.
Deduplication compares both counters when cycles is the selected basis.

## Verification

Tests cover the parser, injected identity reader, sampling,
deduplication, aggregation, classification, and Monitor matching by PCI.
Cases include capacity 2, a GPU clock independent of the CPU clock,
stationary and reset denominators, high-water handling, values near
UINT64_MAX, conflicts, and mixed bases. Existing ns regressions remain.
Linux Release passed 48/48 and Windows Release passed 61/61. Packages
were installed and their consumers ran. Automated approval initially
interrupted Windows verification; a retry through the same approval
mechanism completed successfully.

Positive physical Linux validation on Xe, AMD, and NVIDIA remains
pending. The available WSL environment does not expose physical DRM.
Fixtures do not certify driver or hardware compatibility. Other
providers and GPU-global coverage also remain open. The expanded 0.x
API requires rebuilding the library and consumers together.

Sources: [DRM specification](https://www.kernel.org/doc/html/latest/gpu/drm-usage-stats.html),
[Xe format](https://www.kernel.org/doc/html/latest/gpu/xe/xe-drm-usage-stats.html),
and [Xe v6.12 capacity and clock reporting](https://raw.githubusercontent.com/torvalds/linux/v6.12/drivers/gpu/drm/xe/xe_drm_client.c).
