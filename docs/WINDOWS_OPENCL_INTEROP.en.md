# Windows OpenCL: alternative matching through D3D11

For GPUs without a valid OpenCL LUID, `Monitor` attempts a match through
`cl_khr_d3d11_sharing`. The field is still `GpuInfo.opencl_version`,
but its support is **estimated**: it identifies the version of the OpenCL
device matched by interoperability, not physical identity as established
by the UUID/LUID path. No existing value is replaced. LUID remains preferred.

## Why estimated

`clGetDeviceIDsFromD3D11KHR` with CL_D3D11_DXGI_ADAPTER_KHR accepts an
IDXGIAdapter and returns interoperable OpenCL devices. CL_ALL_DEVICES_FOR_D3D11_KHR
can include non-preferred devices with lower performance. Picking the first
result or treating interoperability as proof of the same physical chip
is not sufficient. Only an observed one-to-one match is accepted: exactly
one device for that adapter, and that device is not associated with any other
DXGI adapter. Even with both checks, support remains estimated, with no claim
of a physical LUID match.

## Query and checks

All platforms and root GPUs are enumerated. Each device's type, owner, and
exact cl_khr_d3d11_sharing token are checked. The extension function is obtained
per platform through clGetExtensionFunctionAddressForPlatform.
CL_ALL_DEVICES_FOR_D3D11_KHR is queried for the explicit adapter, not the
default, without creating a D3D11 device, OpenCL context or queue, surfaces,
buffers, or kernels. No new DLL, link dependency, SDK, installer, or
subprocess: it reuses the already loaded System32 OpenCL.dll and existing
DXGI APIs.

A fresh DXGI factory is used and COM adapters are managed through RAII.
LUID and flags are revalidated after the queries, and the hardware topology
must match the input inventory. Software adapters are excluded. Null,
duplicate, inconsistent, or failed inventories invalidate the fallback
without clearing previous values. A missing extension function on a
platform that advertises support also invalidates the fallback: another
association could otherwise go unnoticed. Each result must be a root GPU
on the same platform that advertised the extension. Unknown or duplicate
devices and CPU devices are not accepted. Multiple GPUs or ICDs for one
adapter are ambiguous.

Limits: 128 platforms, 256 GPUs per platform, 4,096 total root devices,
256 DXGI adapters, and 256 results per interop query. Growing counts are
retried up to three times; errors and out-of-limit buffers are not published
as complete lists. CL_DEVICE_NOT_FOUND means an empty list, not a version.
After validation, CL_DEVICE_VERSION is queried using the limits and format
shared with Linux. An isolated version failure does not clear another
adapter's version. No permanent cache, worker, atomic hotplug snapshot, or
timeout for a blocked driver.

## Verification

Windows Release 53/53 and Linux 39/39; installed packages and consumers
were exercised. Injected tests with two platforms and adapters cover a
unique match, one-to-many and many-to-one matches, unknown and duplicate
devices, limits and growing counts, missing extension/token/entry point,
enumeration failure after valid data, changed LUID, conflicting software
flags or topology, failed or null factory, null or duplicate adapters,
isolated version failures, preservation of the LUID result, and COM cleanup
even when errors return handles.

**Positive native result on AMD RX 7900 XT:** OpenCL 2.0 AMD-APP (3679.0),
marked estimated because it was matched by D3D11 interoperability. The
same driver does not advertise cl_khr_device_uuid, so that path still
produces no value. The Monitor test accepts only available with a LUID note
or estimated with an interop note; it does not treat them as equivalent.
Multi-GPU hardware, Intel, NVIDIA, RDP, physical hotplug, and a positive
UUID/LUID result still need validation. This does not imply universal driver
compatibility.

Source: [matching API, DXGI type, and scope of the ALL set](https://registry.khronos.org/OpenCL/specs/unified/refpages/man/html/clGetDeviceIDsFromD3D11KHR.html).
