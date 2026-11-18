# Per-client DRM engines: explicit reads and sampling

The public API is in pcinsight/linux_drm.hpp. The consumer supplies the
target process's PID and descriptor number. The library does not open
that GPU device or create a client. It neither changes Monitor nor
automatically scans all processes.

```cpp
#include <pcinsight/linux_drm.hpp>
pcinsight::LinuxDrmClientSampler client;
auto first = client.sample(pid, fd); // Counters, no percentages yet.
// Later, from the consumer's timer:
auto later = client.sample(pid, fd);
if (later.value) {
    for (const auto& e : later.value->engines) {
        // e.name is native; check counter_kind before reading the counter.
        // Display e.utilization_percent only when it has a value.
    }
}
```

read_linux_drm_client_usage returns the cumulative snapshot without
keeping state. The sampler retains one identity, starts no threads,
and does not sleep. reset discards the baseline. It is not thread-safe.
sample_interval uses steady_clock and is not an atomic hardware
interval. A read error clears the baseline, so another sample is required.

## Identity and format

The reader uses only read-only procfs and sysfs. It requires a live
process with stable PID/start_ticks, a character-device descriptor in
the DRM subsystem, a valid sysfs PCI identity, and matching drm-pdev.
It rereads fdinfo to verify client, driver, and PCI, then rechecks device
and process. Detected changes, permission denial, and unverified
identity do not produce data. This does not guarantee atomicity
against races. Size limits do not impose a deadline on a blocked
kernel or driver.

The bounded parser accepts drm-driver, drm-client-id, drm-pdev, and
drm-engine-* times in ns. It uses reported capacity or one if absent.
It preserves native names and every observed engine without imposing a
two-compute-engine limit. It does not turn gfx/render into exclusive 3D.
It rejects relevant duplicates, other units, signs, uint64 overflow,
zero or orphaned capacity, records over 64 KiB, lines over 4,096 B,
names over 128 B, and more than 128 engines. The
[cycle provider](LINUX_DRM_CYCLES.en.md) adds GPU-clock cycle pairs with
a separate typed basis; the [maximum-frequency provider](LINUX_DRM_MAXFREQ.en.md)
adds cycles normalized by maximum frequency.

The percentage is the active-time delta divided by interval and
capacity, multiplied by 100. It is marked estimated: activity for
this client, not total GPU load. A first read, new identity, capacity,
or engine, clock without progress, or a delta above interval times
capacity leaves the percentage absent without clipping to 100.
A decreasing counter retains the previous internal high-water mark
and waits for the driver to catch up. The snapshot still shows the
raw counter actually read.

## Coverage and remaining work

This is an optional driver interface, not a guarantee for every kernel
or distribution. Without the interface, permission, or PCI evidence,
there is no value. Zero is published only for a measured zero delta.
Windows returns unsupported and retains its PDH provider.

Shared descriptors can represent the **same** client. Aggregation must
deduplicate by PCI/client-id, not sum rows by PID/fd. A sampler keeps
its baseline when switching to a duplicated descriptor of the same
process and client. A different process resets the baseline even if
it shares that client. [Explicit aggregation](LINUX_DRM_AGGREGATE.en.md)
adds deduplication and client rate sums for an explicit batch. This
reader does not automatically discover descriptors, report complete
GPU percentages, or promise Task Manager equivalence. It does not
create a 60-second window; consumers can retain explicit samples.

[DRM discovery](LINUX_DRM_DISCOVERY.en.md) adds a bounded automatic
sweep as an explicit call. Other drivers, classes, and global coverage
remain open. [DRM engine classes](LINUX_DRM_CLASSES.en.md) adds
AMDGPU/i915/Xe taxonomy, and [Monitor integration](LINUX_DRM_MONITOR.en.md)
adds opt-in collection and unique matching to GPU inventory. Proprietary
NVIDIA support and physical AMD/Intel/NVIDIA validation remain pending.
The available WSL environment does not expose physical DRM. Synthetic
data do not certify hardware. The reader uses existing descriptors;
it does not open a GPU to manufacture a positive case.

## Verification

Parser and state tests cover multiple classes and compute engines,
capacity, delta and zero, high-water handling and recovery, overflow,
changes in client/PID/start/driver/PCI/engine, duplicated descriptors,
errors, and reset. The injected reader API tests process, device, and
client revalidation and permissions. A native Linux query rejects its
own pipe as non-DRM. Linux Release passed 43/43 and Windows Release
passed 56/56. Installed package consumers build and run the public API.
Positive physical DRM/sysfs validation remains pending.

Source: [DRM per-client statistics interface](https://www.kernel.org/doc/html/latest/gpu/drm-usage-stats.html).
