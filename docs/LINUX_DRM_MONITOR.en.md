# Observed DRM in Monitor and Sampler

SampleOptions.include_linux_drm_clients=true enables [DRM discovery](LINUX_DRM_DISCOVERY.en.md) and
[client aggregation](LINUX_DRM_AGGREGATE.en.md). It defaults to false and
also requires include_gpus=true.
Procfs is not scanned for DRM unless both options are enabled.
include_processes need not be enabled. The pipeline does not open GPU
devices or run utilities.

```cpp
pcinsight::Monitor monitor;
pcinsight::SampleOptions options;
options.include_linux_drm_clients = true;
options.linux_drm_limits.max_processes = 4096;
options.linux_drm_limits.max_duration = std::chrono::milliseconds(100);
auto snapshot = monitor.sample(options); // Initial baseline.
// On later ticks, reuse the SAME monitor and options:
// snapshot = monitor.sample(options);
const auto& report = snapshot.linux_drm;
// report.discovery: coverage, limits, and permissions; always observed (estimated).
// report.aggregates: engines and clients by PCI; not global GPU utilization.
```

Sampler receives the same options in its constructor and retains the
report in latest/history. Its history remains bounded. The integration
does not retain raw PID/fd vectors for each sample:
LinuxDrmDiscoverySummary contains only counters, flags, timestamps, and
scope. Raw vectors exist during the sweep and for aggregator baselines;
they are not copied into snapshot history. LinuxDrmDiscovery inherits
that summary and retains observations in its explicit API. DRM data
types live in types.hpp to avoid circular includes; linux_drm.hpp
continues to expose the readers and samplers.

## Semantics and association

The linux_drm report is separate from GpuInfo.utilization_percent and
engines. It does not replace DRM/NVML/PDH measurements or convert client
sums, which can exceed 100, into 0–100 utilization charts. It retains
discovery errors, permissions, and limits, plus the aggregator's
per-engine counters and windows. Missing data is not zero. A sweep
without errors does not certify global coverage.

A group can have inventory_gpu_id only if its full PCI address matches
exactly one available location in current inventory and that GPU's ID
is unique and nonempty. It does not join by index, name, model,
estimated location, or a partial PCI domain. Unmatched groups remain
in the report with PCI identity and no inventory ID. Association
does not make the metric global.

Baselines are kept per Monitor. Opt-out, include_gpus=false, limit
changes, inventory identity changes (not reordering), discovery
errors, conflicting batches, and exceptions discard the corresponding
baselines. Reenabling requires another sample before rates are
calculated. Backend exceptions also reset DRM state before propagating;
Sampler keeps its usual error handling. Opting in on Windows yields
unsupported and does not touch PDH. Concurrent use of the same Monitor
is not guaranteed.

Sweep cost is part of the sample call. Its budget is cooperative,
not a hard kernel or driver timeout. The snapshot's captured_at does
not imply atomic collection. The DRM summary has its own started_at
and finished_at.

## Verification

The integration fixture checks that two reads of a duplicate client
produce 10, not 20. It covers model independence from name, distinct
PCI domains, duplicate or estimated PCI, absent inventory, duplicate
IDs, inventory order and changes, opt-out, disabled GPU collection,
limits, permissions, failure or exception, conflicts, and baseline
recovery. It checks that global utilization 17 and the original
overall engine remain unchanged. Native Monitor and background Sampler
latest/history were tested on Linux and Windows. Positive DRM
correlations were injected; they do not certify Linux hardware.

[DRM engine classification](LINUX_DRM_CLASSES.en.md) adds engine_class
for known AMDGPU/i915/Xe names without renaming engines or
reinterpreting rates. [Cycle counters](LINUX_DRM_CYCLES.en.md) add busy/total pairs,
with counter_kind and summed_client_cycle_percent separate from active
time. [Maximum-frequency normalization](LINUX_DRM_MAXFREQ.en.md) adds summed_client_frequency_percent for the
cycles/maximum-frequency basis. Other classes and drivers, global
hardware coverage and utilization, and physical AMD/Intel/NVIDIA
validation remain pending. This closes the integration, not all work
on engines. No new dependencies or installations were added. The
0.x struct changes require rebuilding the library and consumers.
Linux Release passed 46/46 and Windows Release passed 59/59. Libraries
and headers were installed, and rebuilt external consumers ran. The
installed consumer also tested default opt-out. Positive physical
Linux validation remains pending.
