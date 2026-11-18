# Native Windows GPU inventory

Revision 54 update: [Windows NVML memory](WINDOWS_NVML_MEMORY.en.md) adds an
independent source in Monitor for unallocated framebuffer bytes on an
unambiguously identified NVIDIA GPU. It does not change the static inventory
or infer free memory from DXGI/PDH. Support depends on driver DLLs, identity
matching, and KMT. No positive result on physical NVIDIA hardware yet, no M04
closure, and no equivalent global AMD/Intel support.

Revision 52 update: [opt-in WGL](WINDOWS_CURRENT_OPENGL.en.md) fills
opengl_version for the caller's current desktop context, only when the DXGI
adapter LUID is unique. It does not create a context or establish the maximum
version or per-physical-node version. The OpenGL items listed as pending in
the historical section below predate this limited capability. It does not
close M04.

With include_gpus=true, Monitor.sample enumerates non-software DXGI 1.1
adapters without WMI, external commands, additional installed components,
D3D device creation, or a graphics workload. With include_gpus=false, it
does not open a factory. It links dxgi, a Windows system library, and
exports that dependency to CMake package consumers. It does not change the
COM apartment, affinity, driver, or configuration.

## Data supplied by this component

- ID: dxgi-luid:high:low, with two 32-bit hexadecimal halves. Identifies
  the adapter for this host and session, NOT a physical PCI ID or an
  identifier that persists across boots.
- Model: Description converted from UTF-16 to UTF-8 with size limits and
  Unicode validation.
- Vendor and hardware_vendor_id/hardware_device_id: PCI/ACPI IDs from
  GetDesc1. AMD, Intel, and NVIDIA names are shown for known IDs. Other
  IDs are exposed as IDs without guessing a brand. VendorId 0 leaves the
  name empty; zero IDs are preserved as reported and can mean unspecified
  on older hardware levels.
- dedicated_memory_bytes: dedicated video memory capacity reported by DXGI.
- shared_memory_bytes: the maximum shareable system memory this adapter
  can use, NOT the amount currently used, reserved, or free.
- dedicated_system_memory_bytes: system memory dedicated at boot, separate
  from dedicated video memory. These amounts are not added together.
- driver_version: CheckInterfaceSupport with IDXGIDevice reports a 64-bit
  UMD version formatted as four 16-bit components. It is NOT the AMD/NVIDIA
  marketing version, a date, the DirectX version, or a feature level.
  Access denied yields permission_required; failed or unsupported queries
  yield no value. Errors do not yield a zero version.

Each capacity is the available reported value, including zero for a UMA
adapter. It does not imply advertised or BIOS capacity, or entirely free
VRAM: the driver may report less due to its own reservations. Do not sum
shared_memory_bytes across GPUs as though each amount were separate physical
RAM. Driver date and physical location remain unknown in this component;
neither is inferred from the LUID or device ID. Capacities are not converted
into dynamic usage or percentages.

This component leaves utilization, engines, temperature, memory_used, and
memory_available without values, with explicit reasons. It does not query
Vulkan, OpenCL, or OpenGL. Engines, global memory accounting, location,
driver date, vendor-specific temperature, and graphics-runtime capabilities
were still pending at this stage.

Update: [GPU memory](WINDOWS_GPU_MEMORY.en.md) adds dynamic dedicated/shared usage per adapter
in Monitor, without converting capacities or treating the caller's usage as
global. The internal read_windows_gpus function remains inventory-only.

## Enumeration, errors, and resources

Capability update: the [Direct3D 12 guide](WINDOWS_GPU_D3D12.en.md) adds the
validated feature level using the same adapter without creating a device or
workload. It is not the installed DirectX version. The [GPU device
guide](WINDOWS_GPU_DEVICES.en.md) adds driver date and PCI location to Monitor;
[GPU engines](WINDOWS_GPU_ENGINES.en.md) adds engines. The pending items above describe the
scope of the first implementation, not the combined state.

A new factory is created on each sample because a factory's static inventory
is no longer a valid view after hotplug. Adapters are not cached and enumeration
indices are not used as IDs. SOFTWARE flags exclude WARP/Basic Render. Not
all remaining adapters are necessarily physical PCI cards; there may be virtual
or remote adapters. A successful empty inventory means DXGI enumerated no
non-software adapters, not that the machine has no graphics hardware.

S_OK and a valid handle are required. Only DXGI_ERROR_NOT_FOUND ends the
list. Failed factory creation, EnumAdapters, or GetDesc1, duplicate LUIDs,
invalid Unicode, or more than 256 adapters throw system_error instead of
returning an empty or partial inventory as complete. Monitor.sample propagates
the error; consumers must handle it. Sampler retains its error-reporting
contract; there is no need to invent zeros. An optional driver_version
failure does not remove an already identified adapter.

The factory and adapters are released through RAII on success, at the end
of enumeration, on errors, and on allocation exceptions. The internal
injectable function table tests each path without large COM interface mocks.
There are no public callbacks or transfers of COM reference ownership to the
consumer. There is no guarantee of global consistency during hotplug within
a sample or deadline for a slow API or driver. Fields were added to GpuInfo
in 0.x: rebuild the library and consumers, and do not mix ABIs. Linux keeps
its previous providers; the new IDs and dedicated system memory metric from
this component remain unsupported there until a source is implemented.

## Verification

2026-10-14: Linux 35/35 and Windows 46/46 Release. Packages and external
consumers were rebuilt and run on both systems with the dxgi dependency
and new fields. Injected tests cover discrete and UMA adapters, capacities
without fabricated used/free values, software filtering, unknown PCI/ACPI
vendor, signed high LUID half, known/denied/unsupported driver version,
successful empty enumeration, failed factory/null/enumeration/GetDesc1 paths,
duplicates, truncated descriptions or invalid Unicode, limits, and cleanup
of all resources. include_gpus off/on/off does not retain hidden data.

Physical x64 machine: one AMD Radeon RX 7900 XT. DXGI reported 21413273600
bytes of dedicated video memory, 16972089344 bytes of shared capacity, and
UMD 32.0.31041.1004. Direct enumeration and enumeration through Monitor were
validated without creating a graphics device or workload. This does NOT
verify other drivers, physical hotplug, RDP/headless systems, or multi-GPU
machines. This inventory component alone does not complete Windows GPU support.

Sources: [EnumAdapters1 and lifetime/hotplug](https://learn.microsoft.com/en-us/windows/win32/api/dxgi/nf-dxgi-idxgifactory1-enumadapters1),
[GetDesc1 capacities and LUID](https://learn.microsoft.com/en-us/windows/win32/api/dxgi/ns-dxgi-dxgi_adapter_desc1),
[UMD version through IDXGIDevice](https://learn.microsoft.com/en-us/windows/win32/api/dxgi/nf-dxgi-idxgiadapter-checkinterfacesupport).
