# Windows: dynamic disk performance

Implemented on 2026-10-13 using PDH and the English PhysicalDisk counters.
Each Monitor owns its query. The device number is obtained with
IOCTL_STORAGE_GET_DEVICE_NUMBER using the disk interface handle. The numeric
prefix of the PhysicalDisk instance is matched to that inventory path.
`_Total` is not used as if it were an individual disk.

## Metrics

- read_bytes_per_second: Disk Read Bytes/sec.
- write_bytes_per_second: Disk Write Bytes/sec.
- transfer_bytes_per_second: sum of the published read and write rates,
  with an overflow check. Integer bytes/s values are rounded down.
- active_percent: 100 minus % Idle Time; values outside 0..100 are unavailable.
- average_response_ms: Avg. Disk sec/Transfer multiplied by 1,000.

The figures cover the interval between samples, not an instantaneous peak.
Latency is the average reported by the transfer counter, not the latency of
each operation or a promise that it matches another GUI exactly. A valid
zero is preserved and is not treated as lack of support.

The first sample only establishes the baseline. Two successful collections
and valid PDH data are required. Any change to a number/path mapping, an
empty inventory, disabling disk collection, or a global collection failure
resets the query and requires a new baseline. An enumeration error also
invalidates the dynamic provider.

## Mapping, errors, and limits

An unavailable number or one shared by multiple endpoints is not matched to
any of them. Duplicate instances for the same number are considered ambiguous.
Enumeration and PDH do not form an atomic transaction: hotplug can change
the mappings between calls. Invalidating observed changes reduces mismatches
but does not eliminate every race or guarantee permanent physical identity
in RAID, MPIO, or virtualization setups where drivers do not report it correctly.

Sizes, counts, pointers, and NUL termination of names in PDH buffers (up to
4 MiB or 10,000 entries), CStatus, and finite nonnegative values are checked.
Format errors can affect a counter or an instance; the affected data is left
unavailable, not reported as zero. If opening the query, adding counters,
or collecting data fails, the query is closed and a new one is attempted
on a later sample. Access denied is marked permission_required. If any of
the four counters is missing when creating the query, this provider publishes
no dynamic metrics for that sample.

The provider does not use IOCTL_DISK_PERFORMANCE or run diskperf or another
utility to enable counters. It queries the available PDH infrastructure;
OS configuration or the system provider can prevent data collection. Opening
the query can make the first sample more expensive. The provider imposes no
deadline of its own on synchronous system-provider calls.

## Verification

Fixtures cover baselines and deltas, metrics and unit conversion, zero,
NaN and negative values, overflow, invalid percentages and CStatus,
malformed buffers and names, duplicates, permissions, errors, cleanup
and recovery, and replacement of a number/path mapping. Live integration
queried rates on 5/5 disks without generating test I/O or changing disks.
It also checks disabling and re-enabling collection. Windows Release:
19/19; Ubuntu WSL: 6/6. Additional hotplug, MPIO, Windows-version, and
compiler tests remain pending.

The [volumes provider](WINDOWS_VOLUMES.en.md) covers volumes, filesystem
capacity, system disks, and page files. The [reported media
provider](WINDOWS_DISK_MEDIA.en.md) returned positive results on 5/5 disks
in the 2026-10-19 update. M03 closes the supported providers with evidence
in the [network identity guide](WINDOWS_NETWORK_IDENTITY.en.md): read/write
rates and latency on 5/5 disks, and valid active_percent on 1/5 in the detailed
sample. A later diagnostic showed active_percent on 5/5 with no provider
changes; both samples were retained. %IdleTime availability varies.
Out-of-range values remain unavailable; they are neither clamped nor
replaced with a fabricated zero. Other physical topologies are not claimed
to have been tested.

References: [PDH counter arrays](https://learn.microsoft.com/en-us/windows/win32/api/pdh/nf-pdh-pdhgetformattedcounterarrayw),
[device number](https://learn.microsoft.com/en-us/windows/win32/api/winioctl/ns-winioctl-storage_device_number),
[disk performance and latency](https://learn.microsoft.com/en-us/windows-server/storage/storage-spaces/performance-history-for-drives).
