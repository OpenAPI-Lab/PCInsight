# Windows OpenCL by DXGI LUID

The provider reads CL_DEVICE_VERSION per GPU using cl_khr_device_uuid.
It reuses the inventory and validation core from the [Linux OpenCL
provider](LINUX_GPU_OPENCL.en.md), but Windows identity uses a LUID, not
PCI. `Monitor` runs it only with include_gpus=true.

OpenCL.dll is loaded from System32, three entry points are resolved, and
all platforms and root GPUs are enumerated. No context, queue, sub-device,
or kernel is created. No mandatory OpenCL SDK, headers, link dependency,
or external tool. The ABI table declares CL_API_CALL (__stdcall on Windows),
fixed-width integers, and borrowed opaque handles. x64 was tested; native
32-bit Windows execution is not certified. RAII releases the DLL, and no
pointers are retained after unloading. Configuration is unchanged.

## Identity

CL_DEVICE_LUID_VALID_KHR (a 32-bit cl_bool) is queried only if the device
advertises the exact cl_khr_device_uuid token. It must be CL_TRUE; if
CL_FALSE, the LUID and mask are undefined and are not queried. Other
values are invalid. Exactly eight LUID bytes and four bytes for
CL_DEVICE_NODE_MASK_KHR are read. The mask must have exactly one bit.
The LUID is converted to the same dxgi-luid:high:low ID as the DXGI
inventory. The sign bit of the high half and zeros are preserved.

There is no match by model, vendor, order, or incomplete PCI information.
A LUID repeated across devices, ICDs, or destination entries is ambiguous
and receives no version, even when versions agree. Unrelated GPUs are not
assigned. The value is the device's CL_DEVICE_VERSION string, not the
platform version, OpenCL C version, or certification of all optional
OpenCL 3.0 features.

## Failures and limits

The core retains Linux limits: 128 platforms, 256 GPUs per platform,
4,096 in total, three retries for a growing count, 64 KiB for extensions,
and 1 KiB for the version. Owner/type, exact sizes, NUL termination, tokens,
and version format are checked. Invalid inventory or identity discards
the entire OpenCL sample. A version failure affects only the GPU already
identified. Other GPU providers and DXGI inventory remain available.
Missing data does not mean the hardware lacks OpenCL. Win32 access denial
when loading the DLL maps to permission_required; no such mapping is
assumed for all OpenCL errors.

Without the extension or a valid LUID, this path leaves the value absent.
Update: the [OpenCL interoperability guide](WINDOWS_OPENCL_INTEROP.en.md) adds one-to-one D3D11 matching as
estimated, verified on physical AMD hardware. It is not equivalent to LUID
identity and does not replace an existing value. No software is installed
and no global version is copied as a fallback. There is no atomic snapshot
across APIs, deadline for a blocked driver, or guarantee of zero load cost.

## Verification

Windows Release 52/52 and Linux 39/39; installed packages and consumers
were exercised. Injected API tests cover two platforms with different
versions, a signed LUID high half, duplicate source/destination IDs,
missing APIs, permissions and query failures yielding no value, counts,
growth, handles/owner/type, text and NUL termination, similar tokens,
false or invalid LUID validity, sizes, zero or multibit masks, and
isolated version errors. Monitor off/on/off and native query were tested.
The shared core retains Linux PCI and loader fixture tests.

On the RX 7900 XT in Windows, the native OpenCL device **did not advertise
cl_khr_device_uuid**, so no version was obtained through this path. This
was a read-only native diagnostic recorded by the test, not an inference
from the model. The positive path is not certified on physical hardware,
other vendors, multi-GPU systems, or RDP. Fixtures test the positive path
but do not substitute for that validation.

Source: [CL_DEVICE_VERSION, valid LUID, and DXGI matching](https://registry.khronos.org/OpenCL/specs/unified/refpages/man/html/clGetDeviceInfo.html).
