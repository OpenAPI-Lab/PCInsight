# Linux memory: validated availability and arithmetic

This work covers dynamic reads of /proc/meminfo and the calculation of
used and available RAM, utilization, cache, commit, and zswap. It does not
run external utilities, elevate privileges, write, change swap, or create
memory pressure. Firmware/SMBIOS remains a separate provider; this does not
add universal BIOS reads.

C03 update: the new MemoryInfo.linux_details report expands the categories,
adds an estimated partition, separates original zswap size from pool size,
reports zram per device, and explicitly names installed_minus_usable_bytes.
See [Linux memory composition](LINUX_MEMORY_COMPOSITION.en.md). This does not
turn Slab into Windows pools or the installed-minus-usable difference into
an exclusively hardware reservation. At this checkpoint C03 remained open
until that limit was resolved or accepted. The new 0.x layout requires
rebuilding the library and **all** consumers.

## Total with explicit availability

The new MemoryInfo.reported_total_bytes is a Metric<uint64_t> at the end of
the struct. Prefer it to total_bytes to check availability. Linux obtains it
from MemTotal, the usable RAM reported by the kernel, not raw physically
installed RAM. Missing or zero MemTotal leaves the Metric absent. The legacy
total_bytes scalar retains zero as a placeholder when the total is absent.
**It is not a measured zero and must not be used to infer availability.**

Windows also fills reported_total_bytes when PSAPI GetPerformanceInfo
provides valid PhysicalTotal and PageSize, checking that their product fits
in uint64. This work does not revalidate every other PSAPI product. Custom
backends can leave the new Metric absent; its value is not inferred from
their legacy scalar. The expanded 0.x MemoryInfo layout requires rebuilding
the library and consumers.

## Parser and source

The native reader accumulates at most 1 MiB. Read errors do not produce a
partial map. The parser accepts up to 10,000 lines and 64 KiB per line, with
no NUL. It selects MemTotal, MemAvailable, Cached, SReclaimable,
Committed_AS, CommitLimit, and Zswap. C03 adds MemFree, Buffers, AnonPages,
Shmem, SUnreclaim, Dirty, Writeback, and Zswapped. Selected fields require a
name followed by a colon, a complete unsigned decimal uint64, exactly the
kB unit, and no extra tokens. kB is converted to 1,024 B only after a range
check. Duplicate or malformed selected fields reject the entire map; earlier
parsed values are not published. Unknown fields are ignored to allow
extensions and unitless counters such as HugePages_Total.

A missing selected field does not produce zero. Applying the map clears
earlier dynamic data even when reusing MemoryInfo, without clearing firmware
metadata. A stream failure is not classified as permission denial without
evidence. Explicitly injected denial is preserved in directly read metrics.
A valid sample is not cached and presented as fresh.

## Meaning of the values

- available_bytes: the kernel's MemAvailable estimate, Support::estimated.
  Not MemFree or the memory remaining under a cgroup quota.
- in_use_bytes: estimated MemTotal minus MemAvailable. If an operand is
  absent or available exceeds total, no subtraction is performed: usage
  and percentage are absent with a note. An inconsistent available value
  is not advertised as valid either. The percentage requires a positive
  total and is in 0–100. Available zero yields 100%; available equal to total
  yields 0%.
- cached_bytes: estimated Cached + SReclaimable. Both fields must exist.
  A sum outside uint64 or above a known MemTotal yields absence. It excludes
  Buffers and is not the Windows Cached field. Missing components are not
  added as zeros.
- committed_bytes and commit_limit_bytes: independent Committed_AS and
  CommitLimit readings, not resident RAM. Committed may exceed the limit.
  This is not a used-memory subtraction, and overcommit policy can allow
  the limit to be exceeded.
- compressed_bytes: estimated Zswap proxy, representing only memory
  consumed by its compressed backend. It does not measure Zswapped
  (original size), zram, or all compressed RAM on Linux or Windows. Do
  not add it to in_use again as extra physical memory. This API does not
  provide an exact composition identity.
- paged_pool/nonpaged_pool: absent. These are Windows concepts without an
  equivalent from this provider; Slab or SReclaimable are not renamed to
  simulate them.

MemAvailable and the composition of the fields follow kernel definitions.
MemTotal, Cached, and zswap are not equivalents of Windows indicators.
[Official meminfo definitions](https://docs.kernel.org/filesystems/proc.html).

A valid zero, for example Zswap, Cached, or Committed_AS, is preserved as
zero rather than confused with absence. Derived data carries scope and
estimation notes. An inconsistent relationship invalidates dependent fields
without clearing valid independent commit or firmware readings.

hardware_reserved_bytes computes firmware-installed RAM minus MemTotal
only when both are set and the firmware value is at least the visible total.
It does not use a placeholder total of zero to claim that all installed RAM
is reserved. A known zero difference can be published as estimated. The
difference can include boot limits, not just physical reservations.
Validating every firmware variant is a separate scope.

## Interpretation limits

No MemAvailable fallback is invented on kernels that do not publish it;
another provider would be required. Meminfo reflects the kernel view
available to the caller, not a cgroup quota or necessarily the memory that
can be allocated inside a container. These fields are not read in one
transaction with SMBIOS or process snapshots. Concurrent changes may
require another sample. Parser limits are not a total RAM budget or a read
deadline. This work does not close all memory composition, universal
compression, cgroups, or physical module inventory.

## Verification

On 2026-10-15, Linux Release passed 66/66 and Windows Release passed 86/86.
Packages were reinstalled, and rebuilt external consumers exited 0.
The new linux-memory test keeps assertions enabled and covers units,
fields, numbers, duplicates, the kB conversion limit, NUL and text/line
limits, each optional field omitted, availability versus zero, available
above total, 0/100% endpoints, cache sums out of range or above total,
valid committed above limit, and clearing previous values on errors.
Applying the dynamic provider preserves firmware metadata.

The recorded native Ubuntu WSL run had 16,557,035,520 B of visible RAM,
15,836,041,216 B available, and 4.35461% estimated utilization. It checked
used plus available equals total within one snapshot and that the
percentage is in range. It was read-only, with no memory pressure.
Native Windows checks that available reported_total_bytes equals the
legacy total. Installed consumers on both platforms check the public
field. No native PSAPI or procfs errors were injected, and RAM hotplug or
real compression was not forced. Edge and absence cases use portable
fixtures. Other kernels and C libraries, cgroup quotas, and further
composition or compression need additional coverage.
