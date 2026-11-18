# Linux memory composition and compression (C03)

`MemoryInfo::linux_details` adds Linux-specific data with per-field
availability. It is not Windows memory composition. On 2026-10-19 the user
replied “continue” to the specific proposal to accept the installed-minus-usable
difference as a labeled estimate, unknown without valid data. That acceptance
was recorded only for C03, not E06/L03 or other pending decisions.

## Composition for charts

`linux_details.composition` offers three estimated segments in bytes:

| Segment | Calculation |
|---|---|
| unavailable_estimate_bytes | MemTotal - MemAvailable |
| reclaimable_estimate_bytes | MemAvailable - MemFree |
| free_bytes | MemFree |

They are published together only when `MemTotal>0` and
`MemFree<=MemAvailable<=MemTotal`. Subtractions are checked first. The
three segments sum exactly to MemTotal for that sample. The first matches
the API's `in_use_bytes`: the complement of MemAvailable, not process
memory or an exclusive page class. The second is a difference between
estimates, not a guarantee of immediate reclaim. The kernel estimates
MemAvailable; the provider does not invent it when absent.

These are not Windows in-use/modified/standby/free states. A missing operand
or inconsistent relationship leaves composition absent with a note, not
a chart full of zeros. Valid independent readings remain available.
This is the kernel view, not a cgroup quota.

The report also exposes `free_bytes`, `buffers_bytes`, `page_cache_bytes`
(Cached), `anonymous_bytes` (AnonPages), `shared_bytes` (Shmem),
`slab_reclaimable_bytes` (SReclaimable), `slab_unreclaimable_bytes`
(SUnreclaim), `dirty_bytes`, and `writeback_bytes`. These categories
overlap: do **not** add them together or to the partition. Legacy
`cached_bytes` remains Cached + SReclaimable. `paged_pool_bytes` and
`nonpaged_pool_bytes` remain absent; Slab does not substitute for them.

The expanded parser adds MemFree, Buffers, AnonPages, Shmem, SUnreclaim,
Dirty, Writeback, and Zswapped. It retains the 1 MiB total, 10,000-line,
and 64 KiB per-line limits, kB = 1,024 B, and unsigned uint64 values.
It rejects NUL, duplicate selected fields, and overflow. A failure does
not publish a partial map. Applying another sample clears previous
details and reservation while preserving type, slots, and speed.

## Compression: separate sources

`zswap_pool_bytes` comes from Zswap; legacy `compressed_bytes` retains
that estimated proxy. `zswap_original_bytes` comes from Zswapped.
Absence does not show that compression is disabled; a reported zero is
retained. Do not add these values together or add them to RAM usage
again. `zram_devices` enumerates visible zramN devices under `/sys/block`:

| LinuxZramMemory field | mm_stat source (bytes) |
|---|---|
| original_data_bytes | orig_data_size |
| compressed_data_bytes | compr_data_size |
| allocated_memory_bytes | mem_used_total, including metadata and fragmentation |

A valid empty list means none were observed, not zero global compression.
A row can have absent or permission_required statistics rather than three
zeros. zram can hold swap or a filesystem; it does not measure an app's
RAM. It is not added to zswap as a universal total, and original minus
allocated is not guaranteed savings. They are different layers and reads
are not atomic. Identical pages, incompressible pages, and overhead matter.

Read-only collection pins the directory, checks dev before and after, and
verifies the same directory at the end. Identity is not persistent and
does not detect every reset or ABA change between endpoints. The reader
does not compare deltas. Limits are 4,096 block entries, 256 devices,
4,096 B per attribute, and 7–64 integers in one mm_stat row. Numeric
extensions at the end are allowed. It rejects units, signs, NUL, overflow,
multiple rows, and symlink, FIFO, or nonregular attributes; O_NONBLOCK
avoids blocking on a FIFO open. Inventory failure does not return a
partial list. Device failure retains an unknown row with a reason.
An inaccessible directory is not an empty one. Errno distinguishes
permission problems when there is evidence. Limits are not deadlines.
The provider does not elevate privileges or create devices.

It updates with each memory sample, even with include_disks=false.
It does not present cached data as current or run zramctl, modprobe,
dmidecode, or other applications.

## Installed RAM and reservation: accepted estimate

`firmware_installed_bytes` exposes existing SMBIOS data with provenance
and support, not a partial sum. It can be absent, inaccessible, or
incorrect. This is not universal BIOS access or live hotplug inventory.
`installed_minus_usable_bytes` subtracts only an available, positive
MemTotal that is no greater than valid installed RAM. A known zero
difference is estimated zero. Missing or contradictory sources do not
produce a subtraction, wraparound, or a fabricated total reservation.
Firmware permission denial is preserved.

For compatibility, legacy `hardware_reserved_bytes` mirrors the same
estimate and note. It does **not** measure hardware alone: the difference
also includes kernel effects, boot limits, or offline RAM. Prefer the
explicit new name in the GUI. Do not indiscriminately sum reserved ranges
from iomem, which can include ranges outside installed RAM. A dedicated
hardware-only provider is not claimed to be implemented.
The 2026-10-19 acceptance allows C03 to close with this limit, as already
allowed by its criterion. It does **not** implement a new hardware-only
reservation source or guarantee BIOS access. Physical firmware and
compression validation remain under N02. If a source is permission_required,
the estimate has no value and preserves that support and reason, even
if the source object contains a contradictory value.

## API, compatibility, and tests

```cpp
const auto& d = snapshot.memory.linux_details;
if (pcinsight::metric_is_known(d.composition)) {
    const auto& parts = *d.composition.value;
    // Draw with Linux labels and retain support/note.
}
if (pcinsight::metric_is_known(d.zram_devices)) {
    for (const auto& device : *d.zram_devices.value) {
        if (pcinsight::metric_is_known(device.memory)) {
            auto allocated = device.memory.value->allocated_memory_bytes;
            // allocated is not compressed_data_bytes.
        }
    }
}
const auto& difference = d.installed_minus_usable_bytes;
if (pcinsight::metric_is_known(difference) &&
    difference.support == pcinsight::Support::estimated) {
    // GUI label: "Installed RAM minus usable RAM (estimate)".
    // Never "hardware-only reservation". Keep note and byte units.
    auto bytes = *difference.value;
    (void)bytes;
} else {
    // Show unknown or permission required, not zero or the last known value.
}
```

The initial implementation (revision 9) expanded the public 0.x layout of
MemoryInfo and, by containment, SystemSnapshot. Rebuild the library and
**all** consumers with new headers. Windows does not publish
LinuxMemoryDetails; its native provider is unchanged. No stable ABI is
promised.

`tests/linux_memory_composition.cpp` covers the partition, boundaries,
missing values and zeros, overlapping fields, expanded parser, stale-value
clearing, consistent, inconsistent, and denied reservation proxies, and
the uint64 limit. It also covers mm_stat extensions, bounds, overflow,
text, and units. Linux adds a private sysfs tree with a valid device,
missing or malformed stats, oversized attributes, FIFO and symlink
attributes, non-zram names, a missing directory, and the 256-device
limit without partial results. It creates no devices and changes no real
swap. Integration checks the partition and Monitor report with disks
disabled. No RAM pressure is applied to force compression. Positive
fixtures and native availability are kept distinct.

Verification on 2026-10-16: Linux Release passed 80/80 and Windows Release
passed 100/100. Packages were reinstalled, and rebuilt external consumers
exited 0. Native WSL reported 727,801,856 B estimated unavailable,
423,112,704 B estimated reclaimable, and 15,406,100,480 B free, for
16,557,015,040 B total. No zram devices were observed. Zswap and
installed-minus-usable data were absent. This is not a positive physical
compression or BIOS result; positive cases come from fixtures. The
256-device and 4,096-entry limits and a valid sysfs-style directory
symlink were checked. Initial formatting and attribute warnings were
fixed; the final Linux build had no C++ or clock-skew warnings. This
does not close CI, hardware validation, or the prior N03 risk.

## Revision 39 (2026-10-19)

Acceptance of this scope applies only to C03 and does not accept other
project approximations. The existing criterion remains a dedicated
provider **or** acceptance of the limit. The dedicated provider is not
marked as implemented. Composition, MemAvailable, legacy Zswap, and
the difference remain estimates; Windows pools remain absent. Missing
zram does not certify zero compression, and unreadable SMBIOS does not
certify zero reservation.

Additional fix: apply_linux_memory_reservation preserves
permission_required and the note from the usable-RAM source as well as
firmware. New fixtures check missing data, denial, contradictory values,
unsupported data, previous-value clearing, an estimated zero difference
even when both sources are measured, an estimated usable-RAM source,
and an explicit note that the result is not hardware-only. The legacy
alias retains value, support, and note. The installed consumer verifies
those invariants.

No new layouts, virtual ABI, SystemSnapshot, history changes, dependencies,
privileged actions, zram or swap creation, or test memory pressure were
introduced. There were only native reads and private sysfs files from the
test harness; positive firmware and compression cases remain fixtures.
Revision logs: `work/memory-accepted-{build,focused,suite,install,consumer}-*.log`.
Final regression: Linux Release 108/108 and Windows Release 122/122.
Ten focused Linux repetitions passed, and both installed consumers were
rebuilt, relinked, and exited 0. Final builds had no warnings. Native WSL
reported positive composition, zero observed zram devices, and no Zswap
or firmware difference. This is absence, not zero global compression or
reservation. The cause of historical N03 CPU/app-history failures is not
claimed to have been fixed.

## Primary sources

[meminfo definitions](https://docs.kernel.org/filesystems/proc.html),
[zram statistics](https://docs.kernel.org/admin-guide/blockdev/zram.html),
and [zram sysfs ABI](https://github.com/torvalds/linux/blob/master/Documentation/ABI/testing/sysfs-block-zram).
The proposed partition and limits are library decisions, not kernel-defined
composition names or guarantees.
