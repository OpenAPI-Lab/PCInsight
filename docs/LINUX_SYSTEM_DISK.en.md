# Linux system_disk indicator with explicit unknown state

This work corrects DiskInfo.system_disk in Monitor/include_disks. On
Linux it means the block device directly associated with the filesystem
mounted at / in the caller's view. It does not mean the EFI partition,
bootloader location, /boot, firmware boot disk, or all physical media in
a storage stack. The public API and layout are unchanged. It does not
mount, unmount, or write to devices.

Previously, if root identification failed, root_disk stayed empty and
all disks received available false. This confused “not identified” with
“not the system disk.” The Metric now preserves uncertainty:

- With a known association and exactly one matching inventory row, that
  row is true and the others false, with Support::estimated and a scope note.
- If / cannot be identified or the topology is unsupported, all values
  are absent with notes, not fabricated false values.
- If the associated root disk is missing or duplicated in inventory,
  rows remain absent so the consumer can resample. No global “no” is published.
- An explicit denial from the root provider remains permission_required.
  Other identification failures are unsupported. Missing permissions do
  not trigger elevation.

Check system_disk.value.has_value() before interpreting the stored bool.
A present false means “not the directly associated root device.” Do not
display absence as false.

## Association and checks

The provider reuses the bounded /proc/self/mountinfo reader, escape parser,
and block resolver from [formatted capacity](LINUX_FORMATTED_CAPACITY.en.md).
Mounted capacity uses the same helpers; this work does not change its
capacity semantics.

Root is opened with O_PATH|O_CLOEXEC. fstat on the same descriptor obtains
st_dev, and RAII closes it. No root contents are read or external commands
run. mountinfo rows whose decoded mountpoint is / are checked against
that device. Other root rows on a different device are ignored because
they do not match the current reference. Conflicting types among
matching candidates fail closed. No match indicates a change or an
inconsistent view, not false.
[mountinfo/st_dev reference](https://man7.org/linux/man-pages/man5/proc_pid_mountinfo.5.html),
[open/O_PATH](https://man7.org/linux/man-pages/man2/open.2.html).

The same conservative filesystem policy as mounted capacity applies:
ext2/3/4, vfat/msdos/exfat, ntfs/ntfs3, iso9660/udf, and squashfs. Other
types, including XFS, Btrfs, F2FS, overlay, and tmpfs, need their own
provider or coverage. This does not claim the kernel lacks information
about them. The provider does not publish a single-device association
it cannot justify under its contract.

The device is followed from /sys/dev/block/major:minor. A partition is
associated with its parent through partition, and the result must match
/sys/block without relying on a literal block path component. Device-mapper
can be identified as the direct logical device without assigning its
result to slave devices. An empty returned name, a name containing slash,
backslash, or NUL, dot or dot-dot, or a name longer than 4,096 B is invalid.
Numeric root identity requires complete uint32 major and minor values.

## Limits

The provider inherits mountinfo limits: 16 MiB total, 100,000 rows,
65,536 B per row, 1,024 fields, and 16,384 B per encoded path. It validates
all text before querying root or the resolver. Parse failure does not
publish a partial association. There is no root cache between samples
or new background thread.

This is not an atomic snapshot of mountinfo, the descriptor for /, sysfs,
and disk inventory. Names and numbers can be reused; rapid changes may
require another poll. O_PATH pins the root reference for fstat, not its
physical identity throughout collection. Chroots, containers, and
namespaces can have a different root than the host. This does not
identify the host's system disk. There is no global deadline or universal
hotplug guarantee. Bootloader/EFI, multi-device/RAID, and topologies
outside the list remain open. contains_page_file retains independent
scope. The later [active swap provider](LINUX_SWAP_DISKS.en.md) adds
explicit availability and its own topology limits.

## Verification

On 2026-10-15, Linux Release passed 63/63 and Windows Release passed 83/83.
Installation and rebuilt external consumers exited 0. The new
linux-system-disk test keeps assertions enabled and covers root
association, hidden candidates, missing /, device mismatch, unsupported
topologies and types, denial, invalid identity or API, text and row
limits, yes/no/absent, and missing or duplicate root inventory. Root
reads do not occur if parsing fails. No fallback publishes a global false.

Native Ubuntu WSL reported root sdd, one true and 27 false values marked
estimated, and zero unknowns. Reference root st_dev was checked against
stat. Monitor integration and opt-out were verified, with reads only.
Windows runs portable fixtures, not native Linux root collection. Real
overmounts, denied root access, chroot, and physical NVMe/RAID were not
tested. Injected inputs cover topology failures and ambiguity.
