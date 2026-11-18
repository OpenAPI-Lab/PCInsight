# Windows GPU: driver date and PCI location

Verified on 2026-10-14 (America/La_Paz). After DXGI inventory, `Monitor`
fills the existing `GpuInfo.driver_date` and `physical_location` fields
without external tools, WMI, BIOS reads, or additional installation.

## Identity and data

SetupAPI enumerates present `GUID_DISPLAY_DEVICE_ARRIVAL` interfaces.
`D3DKMTOpenAdapterFromDeviceName` maps each path to the exact DXGI LUID;
the model, vendor/device ID, and enumeration order are not compared.
Properties are read from the `SP_DEVINFO_DATA` returned for that same
interface. The path is opened again to check its LUID before accepting
the data. KMT handles, the SetupAPI set, and Gdi32 are released through
RAII. Gdi32 is loaded only from System32. The implementation uses a
documented public ABI subset with offset checks; no DDK installation is needed.

- `driver_date`: FILETIME DriverDate property converted to UTC
  `YYYY-MM-DD`. It is the installed driver's INF DriverVer date, not
  the installation, download, signing, build, or firmware date. A valid
  zero FILETIME does not indicate absence.
- `physical_location`: only for PCI BusTypeGuid. BusNumber and Address
  must be nonnegative INT32 or UINT32, with exact sizes. Address stores the
  device in the high word and the function in the low word; validation
  requires device <=31 and function <=7. Format:
  `PCI bus 3, device 0, function 0`. No PCI segment is invented, and this
  identifier is not treated as a persistent path: the bus can change.

Each sample enumerates again; no permanent cache hides hotplug. Invalid
dates, other buses, and missing properties leave the value absent.
A property error affects only its corresponding field. Access denials
are marked `permission_required`. No automatic elevation or helper launches.

## Failures and limits

Up to 256 interfaces and paths up to 64 KiB are accepted; structure
alignment, sizes, and NUL termination are checked. Incomplete enumeration
or a path that cannot be mapped invalidates all metadata from that query.
The provider does not publish a subset that could hide duplicate identities.
It does not remove the DXGI inventory or invalidate other telemetry providers.
Two distinct PnP nodes with the same LUID are ambiguous and their fields
stay unknown. Repeated sightings of the same node are deduplicated. Sampling
across APIs is not an atomic transaction when hardware or drivers change.

## Verification

The `windows-gpu-devices` test covers differing or missing identity, LUIDs
shared by nodes, set/property/KMT permissions, errors at the end of enumeration,
unterminated paths, incorrect sizes or types, out-of-range dates, non-PCI bus,
out-of-range device numbers, negative values, valid zeros, LUID changes,
the interface limit, absent APIs, and resource cleanup even on failure.
It includes a native query and a run through Monitor. On an AMD Radeon RX
7900 XT, the results were `2026-08-17` and `PCI bus 3, device 0, function 0`.
These are observations from one machine, not constants or guarantees for all
drivers.

Release regression: Windows 49/49 and Linux 35/35. External consumers of the
installed packages were rebuilt and run on both systems. Physical hotplug,
real linked adapters, and Intel/NVIDIA hardware were not validated here.

## Primary sources

- [Display interface and KMT matching](https://learn.microsoft.com/en-us/windows-hardware/drivers/install/guid-display-device-arrival)
- [OpenAdapterFromDeviceName ABI](https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/d3dkmthk/ns-d3dkmthk-_d3dkmt_openadapterfromdevicename)
- [KMT close](https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/d3dkmthk/nf-d3dkmthk-d3dkmtcloseadapter)
- [DriverDate](https://learn.microsoft.com/en-us/windows-hardware/drivers/install/devpkey-device-driverdate)
- [Bus/device/function encoding](https://learn.microsoft.com/en-us/windows-hardware/drivers/kernel/obtaining-device-configuration-information-at-irql---dispatch-level)
