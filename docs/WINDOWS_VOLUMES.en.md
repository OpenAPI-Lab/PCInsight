# Windows: volumes, system disk, and page files

Implemented on 2026-10-13 using native volume APIs and PSAPI. The provider
is enabled with include_disks and supplies fields in the disk inventory.
It does not change partitions, mounts, files, or paging configuration.

## Formatted capacity

FindFirstVolume/FindNextVolume enumerates GUID names. Each volume is opened
without requesting write or file-read access and queried with
IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS. The extents identify its disks. Each
volume name is counted only once, regardless of drive letters or mounts.

For an accessible NTFS volume, FSCTL_GET_NTFS_VOLUME_DATA supplies
TotalClusters and BytesPerCluster. Their product measures filesystem cluster
capacity independently of quotas; it is not the disk's raw size or all
bytes allocated to the partition. Multiplication and addition are checked.

If that query is unavailable, GetDiskFreeSpaceEx supplies capacity visible
to the user. Quotas can limit it; if it contributes to the sum,
formatted_capacity_bytes is marked **estimated**. If all contributing
volumes use NTFS data, the field is marked available under that explicit
definition. A disk with no observed volumes and a complete inventory
yields zero.

A volume spanning multiple disks leaves formatted capacity unavailable
for the affected disks: the logical size is not divided arbitrarily among
devices. An inaccessible volume, invalid buffer, enumeration failure,
overflow, or sum exceeding physical capacity prevents a partial sum from
being presented as the total. Virtualization, thin provisioning, and
snapshots can require more metadata; exact agreement with physical
allocation capacity is not promised.

## Flags

system_disk identifies disks in the extents of the volume that contains
the directory returned by GetWindowsDirectory. **It does not classify the
firmware's EFI/boot partition**. It reports false only when the volume
query and mapping succeeded.

contains_page_file uses EnumPageFilesW for installed page files and resolves
their volumes with GetVolumePathName/GetVolumeNameForVolumeMountPoint. The
DOS-device prefix is normalized when PSAPI supplies it. A confirmed page
file yields true. False requires complete enumeration and mapping; access
failures or unresolved paths leave the result unknown. Page file presence
is not inferred from its default name or from the registry.

Flags are queried independently of capacity. A filesystem failure does
not erase an already confirmed system or page file mapping. Values reflect
observed readings; volumes, paging, or hotplug state can change between
queries, which do not form an atomic transaction.

## Limits and tests

Up to 4,096 volumes, 2,048 extents per query in a 64 KiB buffer, 1,024 page
files, and paths up to 32,768 characters. Sizes, offsets, nonnegative
extents, and duplicates are checked; RAID/MPIO allocation is not guessed.
Search and volume handles are closed on all paths. Callback exceptions
are caught inside the callback and do not cross the Win32 boundary.
Without a unique device number, the disk's metrics stay unavailable.

Fixtures cover summing and the estimated fallback, NTFS independently
of GetDiskFreeSpaceEx permissions, zero without volumes, multi-disk
volumes, permissions and errors, mid-enumeration failure, malformed
buffers, duplicate names, page file absence, and handle cleanup/caching.
Live read on the local machine: capacity on 5/5 disks, one Windows volume
disk, and one disk with a page file. Windows Release: 19/19; Ubuntu WSL:
6/6. No volumes or page files were modified.

2026-10-19 update: positive WMI results on 5/5 disks, without UAC or
permission changes. M03 completes the supported providers with evidence in
the [network identity guide](WINDOWS_NETWORK_IDENTITY.en.md). The provider
does not guess how to allocate capacity across layers. These results do not
verify other topologies, filesystems, quotas, permissions, or physical hotplug.

References: [volume extents](https://learn.microsoft.com/en-us/windows/win32/api/winioctl/ni-winioctl-ioctl_volume_get_volume_disk_extents),
[NTFS data](https://learn.microsoft.com/en-us/windows/win32/api/winioctl/ns-winioctl-ntfs_volume_data_buffer),
[capacity and quotas](https://learn.microsoft.com/en-us/windows/desktop/api/fileapi/nf-fileapi-getdiskfreespaceexw),
[installed page files](https://learn.microsoft.com/en-us/windows/win32/api/psapi/nf-psapi-enumpagefilesw).
