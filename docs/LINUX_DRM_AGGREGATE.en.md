# Explicit aggregation of selected DRM clients

LinuxDrmGpuSampler, in pcinsight/linux_drm.hpp, consumes a batch of
read_linux_drm_client_usage results and maintains its own baselines.
It needs no subprocess, SDK, root, or new devices. Its arithmetic also
works on Windows with consumer-supplied observations; the native DRM
reader remains Linux-only. It does not scan /proc or change Monitor or GpuInfo.

```cpp
pcinsight::LinuxDrmGpuSampler selected;
std::vector<pcinsight::Metric<pcinsight::LinuxDrmClientUsage>> observations;
// The consumer selects which existing clients to observe (known PID/fd).
observations.push_back(pcinsight::read_linux_drm_client_usage(pid, fd));
auto first = selected.sample(observations); // Initial baseline; no sums yet.
// On the next tick, rebuild observations with fresh reads:
// auto next = selected.sample(observations);
```

Results are grouped by full PCI address, including domain, then by
client-id within each group. A client shared across multiple PIDs or FDs
counts **once**. Copies are ordered by timestamp and the latest complete
observation is selected. Counters are not assembled from maxima across
different engines. Engine ordering and equivalent PCI address forms are
normalized. Copies of one client must agree on driver, engines, and
capacities, with nondecreasing counters as timestamps advance. At equal
timestamps they must match exactly. Contradictions reject the entire batch
and clear baselines.

## GUI semantics

summed_client_active_percent is the **sum of observed client rates**,
not global GPU utilization. It is estimated and can exceed 100: two
clients at 80 sum to 160 without implying 160% GPU utilization. It is
not an instantaneous lower bound either; intervals can differ. Do not
use the sum directly as a 0–100 hardware utilization chart.

For busy/total cycle pairs, counter_kind identifies the basis and the
sum goes in summed_client_cycle_percent, never summed_client_active_percent.
Different bases are not mixed in a sum. Cycles do not require a CPU
interval; shortest_interval and longest_interval include only available
CPU intervals. See [cycle counters](LINUX_DRM_CYCLES.en.md) for the
formula and reset rules.

Each GPU reports input_records and unique_clients. Each engine retains
its native name, capacity, and observed_clients/measured_clients.
Only clients that publish an engine belong to that class; an unpublished
engine is not zero. If any of those clients lacks a valid delta, no
partial sum is published. Conflicting client capacities leave both
capacity and sum absent. shortest_interval and longest_interval describe
intervals that contribute deltas. earliest_observation and
latest_observation are current timestamps, not a common hardware window.
There is no coverage certification or provenance verification for
observations manually assembled by the consumer.

Rates are recalculated from raw counters. Existing input percentages and
notes are ignored. High-water behavior and validation come from the
client sampler. Changing the selected owning process resets its baseline
for safety. Absent clients are removed from state, and reappearance
requires a new baseline. reset and an empty batch clear baselines.
The class is not thread-safe.

## Limits and remaining work

Limits are 2,048 observations, 128 engines per client, and 65,536 engine
records per batch, including duplicates. IDs, PCI, names, capacity, and
uniqueness are validated before aggregation. Invalid, absent, estimated,
or permission-denied input rejects the batch without truncating or
converting errors to zero. permission_required is preserved. Copied
structures omit caller-supplied notes and percentages and retain only
raw data.

[DRM discovery](LINUX_DRM_DISCOVERY.en.md) adds bounded automatic discovery as an explicit
call; this aggregator does not perform I/O. [Monitor/Sampler integration](LINUX_DRM_MONITOR.en.md)
adds opt-in collection, and [DRM engine classes](LINUX_DRM_CLASSES.en.md)
adds typed native categories for AMDGPU/i915/Xe. The
[cycle provider](LINUX_DRM_CYCLES.en.md) adds busy/total cycle pairs.
The [maximum-frequency provider](LINUX_DRM_MAXFREQ.en.md) adds a separate
frequency-normalized sum. Other drivers and classes, global coverage,
and physical AMD/Intel/NVIDIA validation remain pending. This work does
not complete the grouped task.

Tests cover deduplication across FDs and processes, PCI domains and
aliases, temporal order, conflicts, different windows, a sum of 160
without clipping, measured zero, independent engines, capacity, added
and removed clients, high-water handling, limits, and reset.
Linux Release passed 44/44 and Windows Release passed 57/57, with
installed packages and external consumers. Positive cases are synthetic
and do not replace validation on physical DRM hardware, absent in WSL.

Identity and counter source: [DRM per-client statistics](https://www.kernel.org/doc/html/latest/gpu/drm-usage-stats.html).
