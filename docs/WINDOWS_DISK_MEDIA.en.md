# Windows: reported HDD/SSD/SCM media type

Implemented on 2026-10-13 using native COM/WMI and the local storage provider
MSFT_PhysicalDisk. It does not run PowerShell, Get-PhysicalDisk, or external
programs. It links the SDK's ole32/oleaut32/wbemuuid libraries and uses OS
components at runtime.

## Source and identity

The query is fixed: SerialNumber, Size, BusType, and MediaType from
MSFT_PhysicalDisk in `ROOT\Microsoft\Windows\Storage`. MediaType=3/4/5 maps
to HDD/SSD/SCM; 0 and other values do not yield a classification. Windows
or its provider reports these attributes, some of which can be changed
administratively. They are not independent verification of physical hardware.

Matching requires a nonempty serial number, size and bus values matching
the native inventory's Storage IOCTL data, and exactly one match in each
source. Only surrounding whitespace is trimmed from the serial number;
the provider does not guess byte swaps or treat the WMI DeviceId as the
PhysicalDrive number. Differences, duplicates, incomplete identifiers,
and virtual or RAID devices without correlatable data remain unclassified.

DiskInfo.reported_media_type exposes the value, support status, and note.
When a reported type is available, media_type also receives HDD/SSD/SCM.
Otherwise, the older field may retain seek-penalty/no-seek-penalty as a
literal hint from the driver, not as a guaranteed classification. The
serial number is used internally for matching and caching; it is neither
exposed nor persisted as a new identifier in the public API.

## Permissions, COM, and cost

The provider does not elevate privileges. E_ACCESSDENIED/WBEM_E_ACCESS_DENIED
are reported as permission_required. On the development machine, the initial
live query returned **0x80041003, access denied**, so 0/5 disks had a WMI
media type. Inventory, capacity, volumes, and performance remained available.
This does not mean the disks have no media type.

COM is initialized on the calling thread, and only the provider's own
initialization is balanced. If the thread already belongs to an STA,
the provider uses that apartment without tearing it down. It does not
call CoInitializeSecurity to impose a global host policy; it calls only
CoSetProxyBlanket on local proxies. A host that needs global COM security
configuration must set it before its first COM operations; COM can
implicitly initialize its defaults when services are used.

The cache lasts 30 seconds or until observed identities change, disk or
metadata collection is disabled, or the sampling clock moves backward.
Failures are cached too, to avoid repeating RPC on every sample.
SampleOptions.include_disk_media=false skips COM/WMI and resets that cache
while preserving other disk data. This is a Windows metadata option; it
does not change the Linux disk provider.

Enumeration is limited to 4,096 rows and BSTR strings to 4,096 characters.
Integer conversion rejects negative values, truncation, and overflow;
incomplete responses are discarded. Next uses one-second waits and an
observed five-second budget for traversing rows. **This is not an overall
timeout**: ConnectServer can wait up to its native two-minute maximum,
and other COM calls or property reads can block longer. Collecting in a
worker, as Sampler does, avoids blocking the GUI. Proxies, BSTRs, and
VARIANTs are released through RAII. No storage-changing methods are called.

## Verification and status

Fixtures cover VARIANT/BSTR, known and unknown media, mismatched identity,
duplicates in either source, missing data, permission errors, cache refresh
and reset, and removal of an earlier classification. Live integration
checks both the permission failure and preservation of an STA initialized
by the consumer. The live read observed the permission failure described
above. Privileges and disk attributes were not changed. Windows Release:
19/19; Linux WSL: 6/6. Consumers must recompile for the new field and option;
there is no stable ABI in 0.x.

2026-10-19 update, revision 38: WMI returned positive results on 5/5 disks,
with 2 physical HDDs and 3 physical SSDs on SATA/NVMe, without UAC or
permission or configuration changes. The 0x80041003 failure above is
historical, not the current state; this revision did not identify or correct
its cause. WMI code was unchanged. M03 closes the supported providers, with
evidence in the [network identity guide](WINDOWS_NETWORK_IDENTITY.en.md).
Not all COM transports, extreme timeouts, or other physical topologies
and versions are claimed to have been tested.

References: [MSFT_PhysicalDisk](https://learn.microsoft.com/en-us/windows-hardware/drivers/storage/msft-physicaldisk),
[hardware identity](https://learn.microsoft.com/en-us/windows-hardware/drivers/storage/msft-storagefaultdomain),
[COM for WMI](https://learn.microsoft.com/en-us/windows/win32/wmisdk/initializing-com-for-a-wmi-application).
