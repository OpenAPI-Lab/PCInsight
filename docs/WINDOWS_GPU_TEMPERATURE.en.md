# Windows GPU temperature through KMT

`Monitor.sample(include_gpus=true)` queries the adapter's main sensor
through D3DKMTQueryAdapterInfo and ADAPTERPERFDATA. `temperature_celsius`
holds the reported reading in Celsius (the native integer in tenths divided
by 10.0), not hotspot, VRAM, or CPU temperature or a value inferred from load
or fan speed. Each sample queries again; errors do not retain an old reading.

## Identity and ABI

The provider opens the exact DXGI inventory LUID with
D3DKMTOpenAdapterFromLuid. It does not use name, vendor, or order. It
checks that the input LUID is unchanged and requires exactly one physical
adapter through PHYSICALADAPTERCOUNT before and after the reading. If
multiple physical adapters are linked, it does not arbitrarily use index 0
as the overall temperature; the field stays absent. Invalid or duplicate
DXGI identifiers, including IDs that differ only in case, are not matched.

The implementation uses a documented ABI subset with size, offset, and
alignment assertions: a 32-bit KMT handle, 12-byte open-LUID structure,
64-byte perfdata, and 24-byte query on x64. Numeric types 30 (physical count)
and 62 (perfdata) were checked against Microsoft's published header, not
inferred by counting lines on the summary page. No new SDK, DDK, or library
is required. Gdi32 is loaded from System32, and its three functions are
resolved dynamically. This native provider currently supports only 64-bit
callers. For 32-bit callers it reports no value rather than assuming a
different ABI is compatible.

Only physical index 0 is queried after confirming a count of 1. Sizes,
pointers, and input fields must not change. Temperature is initialized
with an out-of-range sentinel to detect success without output. The accepted
range is 0..255°C. A correctly returned zero is valid, not assumed absent.
The metric describes the driver's report; it does not certify sensor
accuracy or calibration or replace an external physical check.

## Failures and resources

Missing API or support, query failure, invalid range or format, a count of
zero or more than one, and a changed count all yield no value with a reason.
STATUS_ACCESS_DENIED maps to permission_required. No elevation or helper is
launched. Failure for one GPU does not remove other GPUs or their inventory,
memory, or engines. There are at most 256 entries per call. KMT handles and
the DLL are released through RAII even on errors or exceptions. The DLL is
not retained. There is no worker, permanent cache, or graphics workload.
Hotplug is not handled as an atomic transaction, and no deadline is imposed
on a blocked driver call. No vendor thermal APIs, external apps, or BIOS
reads are used.

## Verification

Windows Release 54/54 and Linux 39/39; installed packages and consumers
were exercised. Injected API tests cover distinct GPUs and a signed LUID
high half, 48.1/51.2°C conversion, zero and the 255°C limit, denied
open/query calls, null handles or altered identity, zero and multiple
counts, changed sizes/pointers/fields, success without output, invalid
ranges, failed count revalidation, dynamic updates, errors without stale
values, invalid and duplicate IDs, missing APIs, the inventory limit,
and cleanup of all returned handles.
Monitor off/on/off and a native call on the AMD RX 7900 XT returned
**43°C** in one sample. This is a dynamic reading, not a constant.
Intel/NVIDIA hardware, real LDA, physical hotplug, 32-bit callers, and older
drivers or drivers without perfdata support were not validated. Linux
retains its earlier hwmon/NVML providers.

Sources: [ADAPTER_PERFDATA units and scope](https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/d3dkmthk/ns-d3dkmthk-_d3dkmt_adapter_perfdata),
[open by LUID](https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/d3dkmthk/ns-d3dkmthk-_d3dkmt_openadapterfromluid),
[query and buffer](https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/d3dkmthk/ns-d3dkmthk-_d3dkmt_queryadapterinfo),
[Microsoft header with values and layouts](https://github.com/microsoft/win32metadata/blob/main/generation/WinSDK/RecompiledIdlHeaders/shared/d3dkmthk.h).
