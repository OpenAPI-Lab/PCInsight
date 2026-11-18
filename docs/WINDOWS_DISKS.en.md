# Windows: disk device inventory

Implemented on 2026-10-13. SetupAPI enumerates present
GUID_DEVINTERFACE_DISK interfaces. Their paths are opened through
CreateFileW with zero requested access and read/write sharing. Only
property and geometry queries are performed. Disks are not mounted,
formatted, partitioned, or written to.

`SampleOptions::include_disks=true` enables inventory; false skips all
these queries. The provider currently enumerates on every sample to
reflect the devices present. This is not an atomic snapshot during hotplug.

## Available data

- id: the device interface path returned by Windows, without interpreting
  it or replacing it with a potentially reused PhysicalDrive index.
- model: VendorId/ProductId from STORAGE_DEVICE_DESCRIPTOR, when present
  as valid ASCII strings. The manufacturer or signature is not verified.
- bus_type: SATA, SAS, SCSI, ATA, NVMe, USB, RAID, Virtual, File-backed
  virtual, or Storage Spaces, according to the descriptor. Unrecognized
  values are left empty.
- capacity_bytes: OS-reported device size from
  IOCTL_DISK_GET_DRIVE_GEOMETRY_EX. It is not formatted volume capacity or
  the raw physical capacity of the chips in an SSD.
- media_type: `seek-penalty` or `no-seek-penalty` when the driver exposes
  IncursSeekPenalty. These are literal labels for that property, not
  guaranteed HDD/SSD classifications. An NVMe bus or TRIM support alone
  is not used to infer physical media.

Monitor adds the reported HDD/SSD/SCM classification through the [media
provider](WINDOWS_DISK_MEDIA.en.md). When available, media_type reflects
that type; reported_media_type adds availability and diagnostics for
permission issues or missing data.

Enumeration represents OS disk endpoints; RAID, virtualization, or drivers
can hide member disks. An inaccessible device retains its ID with missing
metrics. Capacity reports permission_required if opening or querying it
fails with access denied. Empty model, bus, or media fields do not
distinguish missing configuration from a read failure.

## Integrity and resources

A global enumeration or detail query failure, or an invalid record,
throws system_error without publishing a partial inventory. Limits:
4,096 interfaces, details up to 64 KiB, and descriptors up to 1 MiB.
Returned counts and sizes, string termination, and offsets are checked;
reads do not go beyond the supplied buffers. Disk handles and the SetupAPI
device information set are closed on all paths. No external utilities are
run and no additional privileges are requested.

The geometry query uses a 4,096-byte buffer. If a driver requires a larger
buffer or rejects the query, capacity stays unavailable. Failed individual
IOCTLs leave their fields unavailable without removing the device.
Synchronous driver calls have no provider-imposed timeout guarantee.

## Verification and remaining work

Injected fixtures cover properties and capacity, invalid size or path,
out-of-range descriptor offsets, denied opens, denied or negative geometry,
later enumeration failures, and resource cleanup. Local native read:
5 devices, three SATA and two NVMe, with capacities of approximately
500 GB, 500 GB, 4 TB, 1 TB, and 1 TB. No disk was altered.
Windows Release: 19/19; Ubuntu WSL: 6/6. Dynamic performance is implemented
by a separate [PDH provider](WINDOWS_DISK_PERFORMANCE.en.md).

Volume mapping, filesystem capacity, and system disk/page file flags are
implemented in the [volume guide](WINDOWS_VOLUMES.en.md), with explicit
support and availability. WMI classification is implemented. On 2026-10-19
it returned positive results on 5/5 disks (2 HDDs and 3 SSDs, SATA/NVMe)
without UAC or permission changes. The earlier access denial is historical
evidence, not the current state. See the [media guide](WINDOWS_DISK_MEDIA.en.md) and the
[network identity guide](WINDOWS_NETWORK_IDENTITY.en.md) for M03 completion
and limits or unverified topologies. IOCTL_DISK_PERFORMANCE is not used:
the dynamic provider queries PDH and documents its limits. M03 completes
the supported providers; it does not guarantee all physical topologies.

References: [SetupDiGetDeviceInterfaceDetailW](https://learn.microsoft.com/en-us/windows/win32/api/setupapi/nf-setupapi-setupdigetdeviceinterfacedetailw),
[storage descriptor](https://learn.microsoft.com/en-us/windows/win32/api/winioctl/ns-winioctl-storage_device_descriptor),
[property IOCTL](https://learn.microsoft.com/en-us/windows/win32/api/winioctl/ni-winioctl-ioctl_storage_query_property),
[geometry](https://learn.microsoft.com/en-us/windows/win32/api/winioctl/ni-winioctl-ioctl_disk_get_drive_geometry_ex).
