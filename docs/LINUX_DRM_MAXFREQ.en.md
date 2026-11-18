# DRM: cycles normalized by maximum frequency

The reader, sampler, aggregator, and opt-in Monitor pipeline accept
drm-cycles-N / drm-maxfreq-N pairs when drm-total-cycles-N is absent.
They do not run utilities, open GPU devices, or change driver settings.
Process/PID/start_ticks, descriptor, client, and PCI checks remain.

## API and semantics

LinuxDrmEngineUsage.counter_kind is cycles_max_frequency. busy_cycles
holds the counter and max_frequency_hz holds the frequency normalized
to Hz, both available. total_cycles is absent. The percentage is
estimated and calculated as follows:

    100 * delta(busy_cycles) / elapsed_seconds / max_frequency_hz / capacity

This is not active time. A workload active for the entire interval at
half the maximum frequency can yield 50%, not 100%. The result does
not measure useful performance or global GPU load either. Do not convert
these cycles to nanoseconds as though they were active time.
busy_nanoseconds can be used as a rate basis only when counter_kind is
nanoseconds. Always check the discriminator.

If one engine reports both ns and a cycles/maxfreq pair, the
frequency-normalized pair is selected and the raw ns counter is kept
as metadata. If it reports ns and cycles/total, the ns basis remains
preferred. The three bases are never mixed to calculate a percentage
or sum.

The aggregate uses summed_client_frequency_percent, never
summed_client_active_percent or summed_client_cycle_percent. It exposes
counter_kind and max_frequency_hz. The observed client sum remains
estimated, can exceed 100, and does not replace GpuInfo.utilization_percent.
Clients in the same class with different frequencies, capacities, or
bases are not summed; incompatible fields remain absent.

## Format and continuity

The parser accepts explicit Hz, KHz, and MHz units with a space or tab
between integer and unit, matching the specification's capitalization.
It does not guess units: a missing suffix is conservatively rejected
even though the general format allows omission. Fractions, signs, zero
frequency, unknown units, trailing junk, and uint64 scaling overflow are
rejected. The 64 KiB total, 4,096 B per line, and 128-engine limits still
apply. Orphaned or duplicate maxfreq, or maxfreq with total-cycles for
the same engine, rejects the record. These checks apply even if an
alternative ns counter exists. A malformed maxfreq is not ignored to
produce an apparently valid rate.

The first reading or a change in identity, capacity, basis, or maximum
frequency establishes a new baseline. A nonpositive CPU interval yields
no percentage. Equal timestamps retain the previous busy anchor, while
a backward CPU clock resets the baseline. A decreasing busy counter
retains the high-water mark and waits for the driver to catch up. Zero
delta over a positive interval is a measured zero. A result above 100
or nonfinite remains absent, without clipping. Integer deltas are
computed before conversion; cycles and frequency are not multiplied in
uint64.

Copies of a client must agree on normalized frequency and counter value
when compared. Duplicate copies are not counted twice. Consumer-supplied
percentages are ignored and recalculated from raw counters. The expanded
0.x API requires rebuilding the library and consumers together.

## Coverage

The format does not guarantee that a driver reports these fields or
enables profiling. The provider does not write sysfs to enable it.
The reader's PCI restriction remains: this does not add support for
platform GPUs without drm-pdev/PCI, such as many Panfrost devices.
BIOS is not used to invent a missing counter. This does not certify
physical Linux hardware, proprietary NVIDIA support, or global coverage.

Tests covered the initially failing parser regression, units,
conversion and overflow, 50/0/100% results, capacity, subsecond intervals,
equal and backward clocks, high-water handling, frequency changes, the
uint64 limit, duplicates and conflicts, mixed frequencies and bases,
tampered input, and Monitor correlation by PCI. Final Release passed
49/49 on Linux and 62/62 on Windows. Libraries and headers were installed,
and rebuilt external consumers ran on both systems. Linux verification
ran in WSL, not through the Windows backend.

Source: [DRM cycles and maximum-frequency specification](https://www.kernel.org/doc/html/latest/gpu/drm-usage-stats.html).
Profiling and platform limitation: [official Panfrost documentation](https://www.kernel.org/doc/html/latest/gpu/panfrost.html).
