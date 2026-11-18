# Declared filesystem capacity independent of mounts: E07

The explicit read_linux_filesystem_capacity query is independent of
mountinfo. It reads the declared extent of an ext2/3/4 filesystem within
a selected source and region. It does not mount, format, repair, run
external tools, install dependencies, or elevate privileges. Monitor
does not enable it automatically.

## Usage and units

```cpp
#include <pcinsight/linux_filesystem_capacity.hpp>

pcinsight::LinuxFilesystemTarget target;
target.path = "/absolute/path/to/filesystem.img";
// offset_bytes is the START of the filesystem; default 0.
// If region_length_bytes is absent, use the rest of the source.
auto result = pcinsight::read_linux_filesystem_capacity(target);
if (result.value) {
    const auto bytes = result.value->declared_capacity_bytes;
    // Also display support/note: this is declared metadata, not free space.
}
// For a block device explicitly selected by the caller:
// target.path = "/dev/..."; target.allow_block_device = true;

// Alternative by DiskInfo.id: discover kernel partitions through sysfs.
// false (default) does NOT probe raw devices. true is explicit consent.
auto disk = pcinsight::read_linux_disk_filesystem_capacity("selected-disk-id", true);
// disk.value->regions holds availability per region.
// declared_total_bytes is present ONLY if every region is measurable
// and none overlap. It is not a sum of underlying physical disks.
```

These measurements are not interchangeable:

| Field | Meaning |
| --- | --- |
| source_size_bytes | Logical extent of the source file, or BLKGETSIZE64 of the selected device |
| declared_capacity_bytes | Blocks declared by the superblock multiplied by bytes per block; includes filesystem metadata |
| DiskInfo.formatted_capacity_bytes | Separate provider for visible capacity of mounted filesystems through statvfs, using its own aggregation policy |

A 32 MiB filesystem inside a 48 MiB source still declares 32 MiB.
Do not replace that result with source size, free space, physical or BIOS
capacity, or the sum of hidden partitions. Support::estimated is retained:
the declaration is known, but the provider does not certify that all
filesystem structures are consistent or mountable.

The earlier [mounted-capacity provider](LINUX_FORMATTED_CAPACITY.en.md)
does not change its semantics or start raw reads. The GUI can use this
separate query for the selected region and label its scope. Do not replace
one field with the other or add them as disjoint measures. There is no
new universal per-disk total.

## Formats and mapping policy

The ext family is supported with a primary superblock at offset 1024
from the region start, revisions 0 and 1, and blocks from 1 to 64 KiB.
The reader checks block, inode, and group geometry and bounds; 64-bit
counters only with the corresponding feature and consistent descriptor;
journal, checksum, and orphan-file dependencies; and product overflow.
Bigalloc, compression, journal-device as a source, snapshots or replicas,
and incompatible or read-only feature flags not recognized by this policy
yield absence. This does not claim support for every ext option or for
unmounted XFS, Btrfs, F2FS, FAT, NTFS, or other formats. Unknown compat
features are not used to interpret structures. This reader neither acts
as fsck nor writes the filesystem.

metadata_csum requires a recognized CRC32C type and a valid primary
checksum. If no checksum is configured, superblock_checksum_valid is
absent, not true. CRC is not a signature or authentication. The declared
UUID is optional with the dynamic revision and is not published as usable
if zero. Clones can share UUIDs, and sources are not deduplicated by UUID.
needs_journal_recovery and orphan_entries_may_be_present expose flags
only; they do not verify filesystem health. The reader does not read or
attempt recovery of the journal or orphan file.

Supported sources are a regular image file or a block device with separate
opt-in. A block device can be an unpartitioned disk, a partition, or a
logical device already exposed by the kernel. The caller selects the source
and region. Offsets are not inferred from names, and MBR/GPT or encrypted
or compressed containers are not explored automatically.

kernel_block_device identifies the selected logical object's major:minor.
When sysfs resolves it, disk_inventory_id associates that object or its
partition parent with a /sys/block inventory ID. It is estimated, not a
capacity allocation across physical disks. For DM/LVM/RAID/loop or complex
layers, only the accessible logical object's metadata is read. The provider
does not follow slaves or add underlying media. Without a verifiable direct
mapping, association is unknown even if the filesystem declaration is
available. A regular image is not associated through st_dev with the disk
holding the file; that is not the image's target volume.

The individual query does not sum sources. read_linux_disk_filesystem_capacity
selects a /sys/block object by ID and discovers its partitions using
dev/start/size. If it has no partitions, it probes the whole object.
It does not probe both to count the same storage twice. Each probe requires
the expected major:minor and consistent size, region, and association.
A second enumeration compares disk, partitions, diskseq, and sizes.
Changes invalidate the report so the caller can retry. Enumeration order
does not affect comparison. Limits are 128 regions and 4,096 entries,
without guessing names from prefixes. Supported direct kernel names and
their /dev nodes are required. Exotic names are not translated, and no
nodes are created.

declared_total_bytes is published only if **every** such region has a valid
declaration and none overlap. Duplicate aliases or devices are rejected;
layouts with overlapping extended partitions produce no total. One unknown
or inaccessible region invalidates the total while retaining per-region
availability. Mounts, images, UUID clones, and layers or slaves from other
objects are not mixed. Complete physical totals for complex layers remain
unknown. A present total describes **one selected logical object**, not
its physical media. This is E07's supported mapping and format policy, not
universal storage reconstruction or proof that a disk is unformatted.

## Access, errors, and consistency

The absolute path is limited to 32,768 B and must not contain NUL. It is
opened with O_RDONLY|O_CLOEXEC|O_NONBLOCK|O_NOFOLLOW. Only regular files
and block devices are accepted; a final symlink, FIFO, directory, or
character device is rejected. Block opt-in allows the read attempt but
does not grant privileges. OS denial returns permission_required. The
API does not use sudo, polkit, or UAC. It is neither a privileged helper
nor a sandbox for hostile paths. The host must choose trusted sources
and respect user permissions.

Offset and length must fit within the source and pread range. Two
1,024 B superblocks are read and must match. The fd and pathname are
revalidated for dev, inode, rdev, mode, nlink, size, mtime, and ctime,
along with source capacity and diskseq when exposed. The fd stays pinned
throughout the query and is closed through RAII. Explicit diskseq denial
is also preserved per field.

These are endpoint comparisons, not a transaction or durable identity.
Replacement before the first open, ABA changes, aliases, resizing, kernel
caches, and ancestor changes can escape detection or yield absence. A
mounted filesystem's declaration can differ from live state; caches are
not forcibly flushed. Do not use the result, UUID, or diskseq to authorize
formatting or modification. Inconsistency invalidates the result; the
provider does not reuse an old number or fall back to device size.
There is no guaranteed total I/O deadline or RSS bound.

## Evidence and compatibility

tests/linux_filesystem_capacity.cpp validates the portable parser, geometry,
units, CRC32C with a known vector, UUID and revision, 64-bit handling and
overflow, unknown features, orphan and journal dependencies, and bounds.
Release keeps assertions enabled. Deterministic fixtures for the per-disk
policy cover known multiple regions, denial or partial availability without
a total, wrong device, diskseq change, overlap, and duplicate devices.
These synthetic topologies are not physical validation of RAID, LVM, or
every partition type.

tests/linux_filesystem_capacity_native.cpp creates real, private 32 MiB
ext2/ext3/ext4 regular images as a normal user without mounting them.
Only the test harness uses mke2fs, and only if it is already installed,
to create fixtures rather than obtain data. The distributed library does
not depend on or run it. If it is absent, CMake skips that target without
claiming positive native evidence. The public API is tested with a 32 MiB
declaration inside a 48 MiB source, permissions, symlinks, FIFOs, offsets,
regions, and a corrupt checksum. A subregion fixture copies only metadata
from a real image; it is not presented as a second complete filesystem.
Formatting targets only exclusive files under mkdtemp, never real devices.

Manual --read-root-block-metadata mode, outside CTest, resolves the current
root device through sysfs/stat and checks opt-in and read-only access.
WSL exposed 8:48 mapped to sdd, with both declared and source size of
1,099,511,627,776 B and a valid checksum. No mount, filesystem,
configuration, or user file changed; only primary metadata was read.
The first conservative rejection at ro_compat=66667 included ORPHAN_PRESENT.
The specification was checked and support added with its dependencies,
without relaxing unknown-flag or checksum rejection. Diagnostic evidence
was kept in work/filesystem-capacity-native-block-diagnostic-linux.log.
Manual mode also checks the per-disk API only if root is an unpartitioned
object, repeating two reads of the same superblock for its single region.
If partitions are found, the test does not expand to other targets.

The additional API is in linux_filesystem_capacity.hpp. Existing layouts,
virtual ABI, SystemSnapshot, and history are unchanged. Windows tests the
portable parser and absence of the native Linux reader; its volume provider
is not replaced.

Verified on 2026-10-19: Linux Release passed 108/108 and Windows Release
passed 119/119. Native images and the real WSL device test exited 0. The
per-disk path from sysfs through the guarded region to the complete logical
sum also passed on unpartitioned sdd, without reading other targets.
Installed consumers were rebuilt and relinked and both exited 0. Final
builds had no warnings. Logs: work/filesystem-capacity-*. E07 moved from
P to C after verification of supported providers, formats, mappings,
and unknown handling, not merely by returning unsupported. Physical N02
and N03 remain open.

References: [kernel ext superblock and flags](https://www.kernel.org/doc/html/latest/filesystems/ext4/super.html)
and [ext checksums](https://www.kernel.org/doc/html/latest/filesystems/ext4/checksums.html).
