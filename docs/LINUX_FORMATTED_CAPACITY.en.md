# Capacity of mounted filesystems on Linux

This work corrects the DiskInfo.formatted_capacity_bytes provider. The field
remains for compatibility, but on Linux its value is **an estimate of the
sum of mounted, visible, attributable filesystems**, not a universal read
of all formatted capacity. The caller should display that scope. The
provider does not inspect unmounted filesystems or read raw devices.

2026-10-19 update: the separate explicit E07 API reads primary metadata
from selected ext2/3/4 regions, mounted or not. See
[filesystem capacity](LINUX_FILESYSTEM_CAPACITY.en.md). It does not change
the mounted-filesystem field or provider described here, or add layers or
aliases into a universal physical total. Declared capacity including
metadata and visible statvfs capacity are not interchangeable.

Previously, a disk without an associated volume published an available
zero, indistinguishable from a measured zero. Products were added without
overflow checks, and an alias could be discarded before its statvfs result
was known. Now:

- With no attributable measurement, formatted_capacity_bytes is absent
  with a note, not zero.
- Both f_blocks*f_frsize and the per-disk sum are checked against uint64.
  A zero f_frsize is invalid. A valid zero product retains its optional zero.
- Filesystems are grouped by their device major:minor, without adding bind
  mounts or aliases of that device again. Decimal major/minor values are normalized.
- If a path fails, another alias is tried. Two aliases that return different
  capacities or incompatible filesystem types invalidate that device.
- If an already associated filesystem device cannot be measured, the sum
  of the disk's other accessible devices is not published as a total.

A valid result has Support::estimated and a scope note. Native EACCES or
EPERM is preserved as permission_required. Other read failures, conflicts,
range errors, or unsupported topology yield unsupported. A mountinfo
parse error does not publish a partial vector or map.

## Source, association, and native reads

The list comes from /proc/self/mountinfo in the calling process's view.
Mandatory fields, the separator, and trailing fields are validated.
Unknown optional fields are not interpreted. Absolute paths decode
040/011/012/134 in one pass. A decoded backslash does not start another
substitution. NUL, unknown escapes, and invalid structure are rejected.
[Official mountinfo format](https://man7.org/linux/man-pages/man5/proc_pid_mountinfo.5.html).

Association follows /sys/dev/block/major:minor. A partition is associated
with its parent through the presence of partition, and that parent must
match an entry in /sys/block. This does not depend on a literal block path
component, which is not present in every sysfs layout. The provider does
not distribute device-mapper or RAID capacity across physical disks or
sum underlying layers. Unattributable devices are omitted from the map;
this does not establish that the physical disk has no other filesystem
or is unformatted.

Each query opens the path with O_PATH|O_CLOEXEC, without reading file
contents or creating anything. It checks st_dev with fstat against
mountinfo and computes capacity with fstatvfs on the same descriptor.
A visible overmount on a different device is rejected. RAII closes
the descriptor on errors as well. The provider does not run a path-based
statvfs after validating a different object.
[open/O_PATH reference](https://man7.org/linux/man-pages/man2/open.2.html),
[fstatvfs and capacity units](https://man7.org/linux/man-pages/man3/statvfs.3.html).

Initially supported types are ext2/ext3/ext4, vfat/msdos/exfat, ntfs/ntfs3,
iso9660/udf, and squashfs. This is a conservative provider policy, not a
claim that other filesystems cannot report statvfs. XFS, Btrfs, F2FS, EROFS,
bcachefs, and unknown types require their own topology coverage or
provider. When such a filesystem is associated with a disk, its aggregate
is absent. tmpfs, overlay, and other mounts without a block-device
association do not become capacity on the root disk. None of these paths
requires installing applications.

## Limits and compatibility

Limits: 16 MiB of mountinfo, 100,000 rows, 65,536 B per row, 1,024 fields
per row, and 16,384 B per encoded path. The native reader is bounded
before accumulating more than 16 MiB. Disk identities are limited to
4,096 B and exclude slash, backslash, and NUL. This is not a total RAM
limit: copies, maps, and rows are retained. Synchronous queries have
no overall deadline. O_PATH does not guarantee that a filesystem
responds within a fixed time.

No new public API or persistent format is introduced. Windows keeps its
volume provider. Linux callers must check value, Support, and note:
absence no longer means 0 B. Matching a device does not establish durable
identity or an atomic snapshot of mountinfo, sysfs, and all capacities.
Resizes, device reuse, or concurrent changes may require another sample.

The estimate is total visible filesystem capacity, not free space or
raw or BIOS capacity. Quotas and mount views can affect reported values.
Hidden or unmounted volumes and the capacity of all physical media are
not reconstructed. The sum can differ from partition sizes. system_disk
and page file retain legacy code in this work; their semantics were
tracked separately. The later [system disk change](LINUX_SYSTEM_DISK.en.md)
corrects direct association of /. Page file and additional topologies
retain separate work items.

## Verification

On 2026-10-15, Linux Release passed 62/62 and Windows Release passed 82/82.
Packages were reinstalled, and rebuilt external consumers exited 0. The
new linux-formatted-capacity test keeps assertions enabled. It covers
structure, escapes and normalization, alias deduplication, retry after
denial, conflicting capacities and types, rejection of partial sums,
measured zero versus absence, product and sum overflow, zero fragment
size, incomplete providers, unsupported topologies, text and row limits,
and invalid identities. Invalid input fails before filesystem queries.

Native Ubuntu WSL: an O_PATH fstatvfs reference read for / was checked
against statvfs; a wrong device and a relative path were deliberately
rejected. Monitor published one estimated disk row and 27 absent rows
with notes, not 27 zeros. There were no mounts, unmounts, raw reads,
disk workloads, or elevation. Windows runs portable fixtures and its
backend regressions, not native mountinfo collection. Denials, conflicts,
and multiple topologies are fixtures, not changes to real mounts.
Other kernels and C libraries and additional physical NVMe/RAID
association need more coverage.
