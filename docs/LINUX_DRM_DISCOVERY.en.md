# Bounded discovery of Linux DRM clients

discover_linux_drm_clients, in pcinsight/linux_drm.hpp, discovers visible
PID/fd pairs without requiring the consumer to enumerate them manually.
It does not change Monitor, start threads, run programs, install
dependencies, or open GPU devices.

```cpp
pcinsight::LinuxDrmGpuSampler rates; // Retain between consumer ticks.
pcinsight::LinuxDrmDiscoveryOptions limits; // 4,096 processes, 65,536 FDs, 512 reads.
auto found = pcinsight::discover_linux_drm_clients(limits);
if (found.value) {
    auto selected_rates = rates.sample(found.value->observations);
    // Display scope, errors, and limits with the result.
    // selected_rates does NOT represent total GPU utilization.
} else {
    rates.reset(); // Do not retain a baseline if the sweep could not start.
}
```

## Sweep and coverage

The reader enumerates numeric entries in /proc and /proc/PID/fdinfo.
For each descriptor, stat on /proc/PID/fd/N checks whether it is a
character device; sysfs identifies its subsystem. Only DRM candidates
reach the [full DRM client reader](LINUX_DRM_CLIENT.en.md), which
revalidates process, client, and PCI. Pipes and sockets are not treated
as GPUs, and /dev/dri is not opened to create a test client. Accepted
reads retain duplicates so LinuxDrmGpuSampler can deduplicate by
PCI/client-id.

The result is **always estimated**, even without errors or reached
limits. Procfs can hide processes, namespaces and containers restrict
visibility, and the driver may omit counters. This is not an atomic
snapshot of processes or hardware.

Sweep counters:

- processes_seen: processes whose descriptor directories the sweep tried to list.
- descriptors_seen: entries for which it attempted classification.
- drm_candidates: descriptors classified as DRM, including later failures.
- directory_failures: fdinfo listing failures, including process exit and access errors.
- unclassified_descriptors: subsystem or type could not be determined.
- unreadable_drm_clients: candidate without valid ns timing or identity data.
- permission_denials: denied operations, not a count of distinct users.

Not every enumerated entry is read if a limit is reached. An initial
/proc error returns absence. Later failures yield a partial sweep with
counters. Observations contain only available reads. An empty list does
not mean GPU inactivity or zero utilization. Consumers can aggregate
the accessible subset and display its coverage, never as global load.

## Limits

Defaults: 4,096 processes, 65,536 descriptors, 512 observations, and a
250 ms cooperative budget. The respective maxima are 65,536, 262,144,
2,048, and 60,000 ms, with a minimum of one each. An additional limit of
65,536 engine records, 128 per read, lets the batch fit the aggregator
without later truncation. limit_reached and time_limit_reached indicate
interruption or enumeration limits. started_at and finished_at expose
the actual interval. The result retains an explicit partial selection,
without promising the lowest PIDs or the busiest processes.

Time is checked between operations. A blocked opendir/readdir, stat,
or kernel/driver read is not interrupted. The budget is not a hard
timeout. Windows returns unsupported without scanning. The API is
explicitly opt-in; general Monitor/Sampler use does not incur this cost
automatically.

## Verification and remaining work

Injected discovery-to-aggregation tests cover two clients with a
duplicated descriptor: the second sample yields 20, not 30. Cases
also cover permissions, missing driver stats, exit or invalid identity,
invalid directories, backward clocks, process/FD/observation/engine/time
limits, and invalid parameters. A bounded native WSL sweep attempted
eight processes, read zero FDs, recorded eight denials, and reported
limit_reached=true. This is partial coverage, not absence of GPU use.
Linux Release passed 45/45 and Windows Release passed 58/58; packages
were installed and external consumers ran. The final build had no
initializer_list assignment warnings from the fixture.

[Monitor integration](LINUX_DRM_MONITOR.en.md) adds opt-in Monitor/Sampler
collection and coverage summaries without raw vectors in history.
[DRM engine classes](LINUX_DRM_CLASSES.en.md) classifies known
AMDGPU/i915/Xe pairs. The [cycle provider](LINUX_DRM_CYCLES.en.md) adds
busy/total pairs to the same reader and pipeline, and the
[maximum-frequency provider](LINUX_DRM_MAXFREQ.en.md) adds the maxfreq
alternative. Other drivers and classes, global coverage, and positive
physical AMD/Intel/NVIDIA DRM/sysfs validation remain pending. WSL does
not expose a physical DRM GPU. Injected positives do not certify
hardware or compatibility across all distributions.
