# Linux disk capacity and the rotation hint

This work corrects capacity_bytes and avoids fabricated HDD, SSD, or NVMe
SSD classifications. Monitor's include_disks uses native reads only, with
no subprocesses, installed utilities, elevation, or sysfs writes. It does
not change formatted volume calculations, system_disk, or page file
reporting; those fields were not revalidated here.
A later [formatted capacity change](LINUX_FORMATTED_CAPACITY.en.md) corrects the
sum of mounted filesystems and marks it estimated, without claiming coverage
of unmounted filesystems.

## Capacity

capacity_bytes is the size of the block device visible to the OS: size in
512-byte sectors converted to bytes. It is not the raw manufacturing
capacity, free space, or the sum of filesystems. Virtual devices are included
without claiming that their underlying physical medium was identified.

The old reader used stoull without checking complete consumption and
multiplied by 512 without checking for overflow. The reader now requires a
complete unsigned decimal uint64, with optional trailing whitespace and
at most 128 B. Empty input, NUL, a sign, trailing text, overflow, or excess
size yields an absent/unsupported Metric with a note. It checks
sectors<=UINT64_MAX/512 before multiplying. A valid size of zero is reported
as zero, not absence. Available means the visible device published the
size, not that the exact physical capacity was established. An invalid
capacity conversion for one row does not abort inventory of other disks.

## Rotation and physical media

The new DiskInfo.rotational, a Metric<bool> at the end of the struct, is true
for queue/rotational=1 and false for 0, with Support::available for a read
of the kernel hint. It accepts only literals 0 and 1, optional trailing
whitespace, and up to 128 B. Missing or invalid input yields a Metric
without a value and with a note. This is a configurable queue attribute,
not proof of physical technology.
[Official queue/rotational ABI](https://www.kernel.org/doc/Documentation/ABI/stable/sysfs-block).

| Source | Linux media_type | rotational |
|---|---|---|
| queue/rotational=0 | non-rotational | false, available |
| queue/rotational=1 | rotational | true, available |
| missing or invalid | unknown | no value, unsupported |

E06 update: [disk identification](LINUX_DISK_IDENTITY.en.md) adds an
independent SAT/SCSI/NVMe provider. reported_media_type can reflect that
provider, not this hint. The hint is not converted into HDD, SSD, or SCM:
RAM, loop, virtual and stacked devices, and read failures are not labeled
SSD. The GUI can show the hint and its note, not “confirmed SSD.”

bus_type comes from the native device/subsystem name. nvme is displayed
as NVMe, other identifiers are retained, and missing or invalid input
yields unknown. The bus is no longer inferred from a disk name prefix.
Accepted identifiers contain ASCII letters, digits, underscores, and
hyphens, up to 128 B. scsi does not establish SATA or SAS transport.
Reported identification is queried separately in linux_details; its
sources and limits are documented in [disk identification](LINUX_DISK_IDENTITY.en.md).

## Compatibility and limits

The 0.x API expanded DiskInfo. Rebuild the library and consumers. The
new field is at the end; existing members were not reordered. Windows
retains its medium, bus, and capacity providers and leaves rotational
absent rather than inferring it from HDD/SSD. The installed consumer
checks the new Linux labels.

Callers that compared media_type with HDD, SSD, or NVMe SSD must update
their GUI. A previously inferred medium may now be unknown. This corrects
semantics; it does not remove a previously confirmed physical fact. Check
has_value to distinguish false from absence. An empty read is not used to
infer specific permissions.

The generic reader loads text before validating it. The 128 B limit is an
acceptance limit, not a read budget for arbitrary files. Model, size, hint,
and subsystem are neither an atomic snapshot nor persistent identity.
Resample after changes or hotplug. This does not promise universal
SATA/SAS/SSD identification or direct BIOS access.

## Verification

On 2026-10-15, Linux Release passed 61/61 and Windows Release passed 81/81.
Installation and rebuilt external consumers exited 0. Assertions remain
enabled in Release. The new portable test covers zero, normal, maximum
allowed and first overflowing capacities; signs, NUL, trailing text, and
oversized input; rotation 0, 1, missing, or invalid; reused rows without
stale values; valid and invalid subsystems; an nvme-fake name that does
not imply a bus; and contradictory hints without fabricated media.

Native Ubuntu WSL: 28 stable rows were checked against sysfs reads before
and after Monitor. Capacity, rotation hint, and label matched, with no
writes. The installed Linux consumer checks the new field's labels and
availability. Windows runs portable fixtures and its own storage regressions;
this does not validate native sysfs capture on Windows. Physical media on
other machines, hotplug, and actual sysfs permission denials were not
validated. Empty and invalid inputs are covered by fixtures, without
inventing permission_required.
