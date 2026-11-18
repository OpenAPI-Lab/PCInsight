# Active swap by device on Linux

This is the DiskInfo.contains_page_file provider in Monitor/include_disks.
On Linux the legacy name means **active swap areas directly associated
with the device**, including supported partitions and files. It does not
look for inactive files or establish the configuration of the next boot.
It is not a Windows pagefile or per-app swap byte accounting. There is
no new public API or layout.

2026-10-19 update: E05 closed after a positive result with a real active
swap file on ext4 in WSL. The documented scope remains direct partitions
and ext2/3/4 files, with estimated association and no universal coverage
of topologies or filesystems. The 2026-10-16 evidence below is historical,
not the current status.

Previously, stat was queried only for block-device entries. Files were
ignored, and unidentified paths or mappings yielded false for all disks.
The provider now preserves availability and distinguishes yes, no, and unknown.

2026-10-16 update (E05 E-to-E, not closed): checked file size and
revalidated metadata and pathname after resolving the disk. The native
regular-file path was tested against synthetic inventory. The environment
then had only partition swap on /dev/sdc, so end-to-end testing with a real
active swap file was pending. No swap was enabled or disabled and no
narrower scope was presumed accepted.

## Collection and state

The source is /proc/swaps, parsed fully before querying paths. The
Filename/Type/Size/Used/Priority header and five fields per entry are
required. Supported types are file and partition. Size and usage are
unsigned uint64, usage cannot exceed size, and priority is int32.
Absolute paths decode 040/011/012/134 once. NUL, unknown escapes or types,
and duplicate paths are rejected. Parse errors do not return partial results.
[proc_swaps manual](https://man7.org/linux/man-pages/man5/proc_swaps.5.html),
[Linux v6.12 reference source, swap_show](https://github.com/torvalds/linux/blob/v6.12/mm/swapfile.c).

After resolving paths, the list is read again. Path, type, size, and
priority must match regardless of order. Used may vary without changing
configuration. If the second read fails, is invalid, or differs, the
report remains absent so the consumer can resample; no partial flags
are published. Two matching observations are not a transaction and do
not prove continuity between them. Intermediate swapoff/swapon can be
invisible.

With a stable report:

- A uniquely identified disk among known targets receives estimated true.
- Other disks receive estimated false only when every entry was resolved,
  all target disks appear unambiguously in inventory, and layering policy
  allows it. A valid list with no active areas allows false for all valid IDs.
- If an entry fails because of permissions, path, topology, or association,
  known positives are kept and other results are absent, not false.
  Explicit denial yields permission_required for unknowns; other failures
  yield unsupported.
- Missing or duplicate target disks prevent global negatives. A duplicate
  or invalid inventory ID is not published as positive either.

In native collection, a device with virtual in its sysfs path, visible
slave devices, or a failed layering query does not allow other disks to be
ruled out. It retains the direct positive and leaves the rest unknown.
The provider does not distribute swap across physical RAID/LVM/device-mapper/loop
disks or promise to discover media hidden by drivers. Flags remain estimated,
with notes explaining their scope.

## Native association and identity limits

Each pathname is opened with O_PATH|O_CLOEXEC and checked with fstat on
the same descriptor. file requires a regular file and uses st_dev;
partition requires a block device and uses st_rdev. A raw device is never
opened to read its contents. A type that disagrees with the listing is
rejected. RAII closes the descriptor, including on errors.

For files, size_kib*1024 must fit in uint64 and cannot exceed a nonnegative
st_size. This is a minimum consistency check, not exact size including
the header, proof of allocated extents or absence of holes, verification
of the swap signature, or activation eligibility. Direct queries also
reject Used>Size or an absent internal resolver. After mapping, fstat on
the fd and fstatat on the pathname are repeated. dev, inode, mode, rdev,
size, nlink, mtime, and ctime must agree, including nanosecond fractions.
Atime is not compared. Disappearance, replacement, truncation, or metadata
changes during the query yield absence, not invented positives or
negatives. A resolver result with invalid disk identity is not published.

Initial file support is limited to ext2/ext3/ext4 families identified
with fstatfs on the same descriptor. Other filesystems, including XFS,
Btrfs, and F2FS, remain unknown until a topology-specific provider exists.
Partitions do not need a mounted filesystem. The provider reuses
resolve_linux_block_device: /sys/dev/block/major:minor, the partition
parent, and matching /sys/block, without assuming an sd/nvme name or a
literal block path component.

Importantly, /proc/swaps publishes names, not a verifiable inode token
for the kernel's active swap object. The fd pins **the path that was
opened**, not proof that it is the inode the kernel uses for swap.
Renames, path replacement, namespaces or chroots, and device reuse can
cause absence or an incorrect estimated association. Two identical lists
do not eliminate that race. Do not use this report to authorize
destructive actions or as security identity. Show estimated, retain
notes, and resample when necessary.

The 2026-10-16 revalidation detects changes between its endpoints. It
does not prove that a replacement before the first open matches the
original active inode or detect every ABA change. The fd is not
permission for swapoff or another action. The added internal resolver
callback can force races in tests; production still uses
resolve_linux_block_device.

The production provider installs no dependencies and does not call
swapon/swapoff, helpers, shells, mounting operations, or elevation. It
does not read the contents of swap files or other reference files.
There are no new automatic writes or path persistence. The authorized
manual test below is separate: it initializes and activates only its
own private file.

## Limits and compatibility

Inventory reads are capped at 16 MiB, 100,000 entries, 64 KiB per header
or row, and 16 KiB per encoded path. Disk IDs are at most 4,096 B, with no
NUL, slashes, dot, or dot-dot. All input is validated before reference
queries. Flags use a count index rather than a quadratic loop over disks.
These limits are not a total RAM budget or a syscall deadline. Copies
and key sets are retained.

Linux consumers must check Metric.value.has_value(). Present false is
not unknown. Some previously available false results are now absent or
estimated. This corrects semantics; it does not remove a reliable
measurement. Windows keeps its page-file provider, and the history
format is unchanged. Layering policy is conservative. There is no
universal promise of physical location or validation of all Linux
filesystems, drivers, and environments.

## Verification

On 2026-10-15, Linux Release passed 64/64 and Windows Release passed 84/84.
Packages were reinstalled, and rebuilt external consumers exited 0.
The new linux-swap-disks test keeps assertions enabled and covers headers,
escapes, types, numbers and ranges, duplicate paths, input/row/path limits,
matching and changed configuration, variation in Used and order,
failed second reads, incomplete APIs, and inaccessible or invalid mappings.
Fixtures also cover yes/no/absent, retained positives, missing or duplicate
inventory targets, known empty lists, and layers without inferred negatives.

Native Ubuntu WSL: existing active inventory yielded one true and 27 false
estimated values, with zero unknowns on that run. Monitor and opt-out were
verified. Relative paths, a character device, and unsupported proc
filesystems were rejected. An ordinary /etc/passwd reference was opened
**for metadata only** and associated with sdd through st_dev. Its contents
were not read and it was not claimed to be active swap. No swap file was
created or activated. Thus this run did not establish native end-to-end
operation with a new active swap file. Fixtures and a regular-file reference
probe covered the file path. Other filesystems and RAID, renamed swaps,
native permission denials, and other kernels and C libraries require more
coverage. Windows runs portable fixtures and its own regressions, not
native Linux collection.

On 2026-10-16, Linux Release passed 82/82 and Windows Release passed
102/102. Packages were reinstalled, and relinked external consumers
exited 0. No new public layout or API was introduced, and the final Linux
build had no warnings. The new tests/linux_swap_path.cpp uses a private
ordinary 2 MiB file on ext4 /tmp, a reference mapping 8:48 to sdd, and the
sample_linux_swap_disks flow with its native reader and flags over
synthetic inventory. It checks pathname replacement, truncation, unlink,
chmod, and writes during resolution; smaller size, overflow, Used>Size,
wrong type, FIFO, absent resolver, and invalid ID. Incomplete results
clear an earlier positive and leave unknown, not false. On other
filesystems or without direct mapping, the positive is explicitly
skipped and absence checked. This WSL run did execute the positive
reference case.

This was **not** an active swap-file test. It did not run swapon, swapoff,
mkswap, mounting operations, or read swap contents. Only ordinary
private fixtures were written and removed by the test. Real WSL inventory
was /dev/sdc partition, 4,194,304 KiB, used 0, priority -2. At that checkpoint,
E05 remained E until a real active file and relevant supported topologies
could be tested. The prior N03 CPU risk is not claimed resolved. This
suite passed but did not determine the earlier cause.

### Authorized native positive result, 2026-10-19

New tests/linux_swap_active_native.cpp builds the Linux executable
pcinsight-linux-swap-active-native. It is **not** registered with CTest.
Without the exact --authorized-private-swap argument it returns 77 before
creating files. It requires separate human authorization and root. It is
not a public library action.

The bounded test requires MemAvailable of at least 512 MiB, at least
256 MiB free storage, and direct mapping for ext2/3/4 /tmp. It creates
a mode-0700 mkdtemp directory and an exclusive mode-0600/O_NOFOLLOW file,
allocates and writes 64 MiB, and calls fsync. It initializes the v1
header following the [Linux v6.6 swap_header union](https://github.com/torvalds/linux/blob/v6.6/include/linux/swap.h),
with native page size and the SWAPSPACE2 signature, without mkswap or
new dependencies. Native swapon and swapoff calls target only that
path. The test does not force memory pressure or touch existing swap,
fstab, mounts, or permanent configuration.

On WSL kernel 6.6.114.1, ext4 /dev/sdd, the active file reported
65,532 KiB (4,096 B header), used 0, and disk sdd. Real /proc/swaps fed
read_linux_swap_path_disk, the native report, and
Monitor::sample/include_disks: contains_page_file=true with
Support::estimated. No fixture replaced inventory. Successful swapoff,
absence from the list, unlink, and rmdir were checked. The existing
/dev/sdc 4,194,304 KiB, priority -2 configuration was preserved; Used is
volatile. The manual test exited 0, and the guard without the flag
exited 77. Log: work/swap-active-native-linux.log. File contents were
not read while active.

Normal and exception cleanup attempts deactivation before removal and
checks absence from inventory and dev/inode/type identity before unlink.
If deactivation or identity cannot be confirmed, it keeps the file and
prints its path. SIGKILL, host failure, or external termination cannot
guarantee RAII. Do not run the test unattended or without explicit authorization.

E05 moved from E to C on this positive result and prior partition,
parsing, race, and conservative topology coverage. It does not validate
XFS/Btrfs/F2FS, physical RAID/LVM distribution, universal kernel-inode
identity, or every kernel. Estimated is not promoted to exact. The
production provider, API, layout, and history are unchanged.
Linux Release regression passed 102/102 and Windows Release passed
116/116. Packages were reinstalled and both external consumers were
compiled and exited 0. Builds had no warnings. Logs:
work/swap-active-{build,suite,install,consumer}-{linux,windows}.log.
