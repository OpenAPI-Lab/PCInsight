# Linux disk media, protocol, and transport identification

E06 provides explicit identification through sysfs without running
utilities, opening /dev devices, issuing identification commands, writing
attributes, or requiring new packages. It uses data already exposed by
the kernel. It does not certify the physical medium hidden behind a
bridge, VM, or RAID. On 2026-10-19 the user replied “continue” to the
specific proposal to accept this scope for E06: report declared values and
keep the medium unknown when hidden, without automatically treating NVMe
as SSD. This acceptance applies only to E06, not L03 or other open decisions.

## Public contract

DiskInfo.linux_details contains Metric<LinuxDiskDetails>. Each field has
its own availability and note. The structure's presence confirms a
verified read of the device context, not that all fields exist.

| Field | Meaning |
|---|---|
| kernel_subsystem | Name reported by device/subsystem, not an inferred transport |
| device_protocol | SCSI/NVMe from the subsystem, or ATA/SATA from IDENTIFY in SAT VPD page 89 |
| transport | SCSI target-port protocol or the NVMe controller's transport attribute |
| media_type | Classification reported by identification; estimated, not a queue hint |
| nominal_rotation_rpm | Reported nominal RPM, never live measured speed |

reported_media_type mirrors linux_details.media_type. The legacy strings
media_type (rotational/non-rotational/unknown) and bus_type (subsystem)
retain their former contracts and are not silently replaced by another
concept. The GUI should consult the Metrics and their notes. Do not
concatenate all labels as a single physical connection: SATA and SAS
can describe different layers of the same path. Windows leaves
linux_details absent.

The initial implementation (revision 14) expanded DiskInfo/SystemSnapshot
in the 0.x API. Rebuild the library and **all** consumers, including installed
consumers. The history format is unchanged.

## Sources and validation

SCSI: read binary device/vpd_pg89, vpd_pgb1, and vpd_pg83. The kernel
publishes these pages from its VPD cache. The library neither requests
a rescan nor issues SCSI commands.
[Kernel VPD publication](https://github.com/torvalds/linux/blob/v6.12/drivers/scsi/scsi_sysfs.c).

All pages require the correct page code, a direct-access or zoned block
peripheral type, a connected qualifier, and a declared length equal to
the full file. Truncated data is not accepted as a valid prefix.

- SAT page 89: 572 B, IDENTIFY DEVICE command 0xec, 512 B payload starting
  at offset 60, an ATA device, and ATA version 7 or later with bit 15 clear.
  The checksum is checked when signature 0xa5 is present. Word 76 must
  contain positive SATA speed bits and a nonreserved value to report SATA.
  Otherwise ATA is retained; PATA is not inferred. Word 217 equal to 1
  yields estimated SSD under ATA classification; 0x0401–0xfffe yields
  HDD and nominal RPM. Values 0, 0xffff, and 2–0x0400 do not report media.
  [SAT page 89 payload and libata B1 characteristics](https://github.com/torvalds/linux/blob/v6.12/drivers/ata/libata-scsi.c),
  [kernel ATA fields and helpers](https://github.com/torvalds/linux/blob/v6.12/include/linux/ata.h).
- B1: at least 64 B. Nominal big-endian rotation rate in bytes 4 and 5.
  A value of 1 is reported as non-rotating, not necessarily SSD. A rate
  of 0x0401–0xfffe yields estimated HDD and nominal RPM. Conflicting valid
  ATA and B1 rates invalidate both media and RPM rather than arbitrarily
  choosing a page.
- VPD 83: all descriptors are traversed and lengths and reserved fields
  checked. Only PIV=1 and target-port association with supported binary
  EUI, NAA, or relative-port descriptors are used. Supported protocols
  must agree: Fibre Channel, Parallel SCSI, IEEE1394, SRP, iSCSI, SAS,
  and ATA. Missing, conflicting, malformed, or unsupported evidence
  leaves transport absent. A LUN identifier is not interpreted as a port.
  [Protocol identifiers](https://github.com/torvalds/linux/blob/v6.12/include/scsi/scsi_proto.h),
  [PIV and port association in the kernel target](https://github.com/torvalds/linux/blob/v6.12/drivers/target/target_core_spc.c).

NVMe: the subsystem establishes the protocol, not storage technology.
The provider accepts transport=pcie/tcp/rdma/fc/loop from the same
controller directory. A TCP controller is not called PCIe, and NVMe
does not automatically mean SSD. A multipath namespace that does not
directly expose this controller may lack these data.
[NVMe transport ABI](https://www.kernel.org/doc/Documentation/ABI/stable/sysfs-nvme).

## Reading and identity

Directories are pinned by descriptors. The block and device inode/dev
pairs are compared with their paths at the end. Required major:minor
and optional diskseq are validated and compared before and after.
Subsystem, pages, and transport are reread. An observed change
invalidates all of linux_details without retaining the old medium.
Parsing does not rely on sda/nvme*/ata* names or model/vendor strings.

Attributes are opened read-only with CLOEXEC/NONBLOCK/NOFOLLOW,
checked with fstat as regular files, and read with EINTR handling and
enforced limits. Page 89 is limited to 572 B; B1/83 to 65,539 B;
dev/diskseq/transport to 128 B; and links to 4,095 B. A FIFO, directory,
or attribute symlink is not read as a regular file. Observed EACCES or
EPERM is reported as permission_required, not inferred from absence.

This is not an atomic snapshot. Intermediate A-to-B-to-A changes,
attribute replacement with the same contents, hidden backing media,
and identity reuse without diskseq are not universally detectable.
Read limits are not driver deadlines. The provider does not search
for ancestors by name or traverse layers to invent the medium behind
dm, md, loop, virtio, or USB. Firmware and bridges can misreport or emulate data.

## Verification and scope

tests/linux_disk_identity.cpp exercises portable parsers with synthetic
pages, checksum, limits and ranges, conflicts, separate transports, and
stale-value clearing. On Linux, the same native reader processes a private
synthetic sysfs tree with a FIFO, an oversized file, and deterministic
changes to major:minor, diskseq, VPD, subsystem, device, and the block
directory during reads. These positive cases are not physical hardware
or pages from real disks. Monitor integration reads real /sys/block,
without writes, and checks common-field consistency.

E06 does not establish universally exact physical classification.
NVMe media, non-rotating SCSI without further technology identification,
and backing media under virtual layers can remain unknown. Returning
unsupported does not close that part. Additional physical hardware
validation remains under N02. E01–E05 and E07 are unchanged.

On 2026-10-16, final Release suites passed on Linux (84/84) and Windows
(104/104). Packages were reinstalled and both rebuilt external consumers
exited 0 with the new layout. Ubuntu WSL showed four rows with pinned
contexts and zero media reports, without inventing hardware types behind
virtual disks. Positive SATA/SSD/HDD/SAS/NVMe cases were fixtures, not
physical validation of those technologies.

The initial Linux suite passed 83/84: linux-app-history failed because
its report was absent, not because of disk identification. Its native
test uses include_disks=false. The cause was not established. Logs were
preserved in work/disk-identity-failed-linux-tests.log and ...-list.log.
The note was printed before the original assertion; three reruns and
the 84/84 suite then passed without changing history logic or assertions.
The failure is not attributed to CPU or claimed fixed by E06; it remains
under N03. Initial links also used an object compiled before the internal
test-hook signature changed. A subsequent full rebuild had no errors
or warnings. The host clock was not changed in response to transient
MSFS clock-skew warnings.

Status E meant waiting for an additional provider or acceptance of the
limit. The 2026-10-19 response allows closure of E06 through the existing
acceptance criterion, not through a universally exact provider. No new
ID or partial credit is added to the count.

## Revision 40 (2026-10-19)

The acceptance retains reported/estimated and unknown labels. It does
not authorize fabricating physical types, guessing them from model names,
or traversing virtual layers as if they were exclusive physical devices.
Physical validation of other technologies remains under N02; Windows
positive results are not evidence for the Linux provider.

Diagnostic audit and fix: without a valid report, if an ATA or B1 page
is denied, both medium and RPM preserve permission_required and an
explicit reason. An independent valid page can still contribute its
report when the other is denied. Non-rotating does not become zero RPM.
Additional tests check that VPD or transport from another layer does not
classify virtio, USB, block, or PCI as a physical disk; NVMe/TCP with
unrelated ATA VPD does not acquire an SSD/HDD label; replacing an SSD
sample with NVMe without evidence clears the old report; and permission
denial or absence never retains an old classification. The
reported_media_type alias preserves the internal field's value, support,
and note, also verified by the installed consumer.

For the GUI, check metric_is_known(disk.linux_details), then each field's
Metric. Display “unknown medium” if media_type has no valid value and
retain support/note. Do not fall back from disk.media_type to an SSD
classification. An available structure with all physical fields absent
does not certify that the disk has no medium or that its performance
or capacity is zero.

No layouts, API, virtual ABI, SystemSnapshot, history, dependencies, /dev
access, disk/configuration changes, or elevation were introduced.
Positive ATA/SATA/SSD/HDD/SAS/NVMe cases still use fixtures with the native
reader over private sysfs, not certified physical hardware. Historical
CPU/app-history failures remain under N03 and are not resolved simply
by repeating a passing suite. Evidence: work/disk-reported-*.log.
Regression: Linux Release 108/108 and Windows Release 122/122. Ten
focused Linux reruns passed; both installed consumers were rebuilt,
relinked, and exited 0; builds had no warnings. Native WSL had four
pinned contexts and zero media reports, with unknown retained.
