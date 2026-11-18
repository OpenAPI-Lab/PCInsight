# OpenCL on Linux: per-GPU version

The first platform's CL_PLATFORM_VERSION is no longer copied to every
card. `GpuInfo.opencl_version` now retains the CL_DEVICE_VERSION string
of the GPU matched by a full, unique PCI address. It describes the
OpenCL device, not the platform, loader, OpenCL C, or support for all
optional OpenCL 3.0 features.

## Identity and API

The provider runs after DRM and NVML. It accepts only an available
physical_location, with a full BDF and a four- or eight-digit
hexadecimal domain. Matching uses domain/bus/device/function, not
name, vendor, index, or first GPU. It queries all platforms and their
root GPU devices and checks CL_DEVICE_PLATFORM and CL_DEVICE_TYPE.
It requires the exact cl_khr_pci_bus_info token per device before
reading its PCI structure. Bus must be at most 255, device at most 31,
function at most 7, and the full domain must fit in 32 bits. Partial
vendor extensions are not used to invent missing domains.

An address repeated in DRM/NVML or among different devices or ICDs
is ambiguous and remains absent, even when versions agree. Without an
extension, valid identity, or valid version there is no global fallback.
Missing data does not establish lack of hardware support. A version
error affects only the identified device. Inventory or identity
failures discard the whole query rather than publishing a subset that
could hide a duplicate.

## Limits and resources

Limits are 128 platforms, 256 devices per platform, and 4,096 devices
total. Enumerations require exact counts and allow three attempts if
the list grows between calls. Null or duplicate handles and conflicting
ownership are invalid. CL_DEVICE_NOT_FOUND and CL_PLATFORM_NOT_FOUND_KHR
leave the corresponding list empty. Extensions are capped at 64 KiB
and versions at 1 KiB, with sizes and NUL checked and printable ASCII
required. Versions must use OpenCL major.minor followed by a space
and suffix. OpenCL C or malformed values are not accepted as GPU versions.

The provider locally dlopens libOpenCL.so.1 and resolves three functions
with dlsym. It does not link to OpenCL, require headers or an SDK,
install software, or start subprocesses. The public Linux ABI subset
uses fixed-width integers, borrowed opaque IDs, and a 16-byte PCI
structure. It creates no contexts, queues, subdevices, or kernels, and
does not retain or release borrowed root IDs. The library reference is
the only acquired resource and RAII releases it even on failure.
There is no permanent cache or worker. The runtime can initialize drivers;
there is no deadline for a blocked driver or atomic snapshot against
hotplug. OpenCL errors alone do not establish permission denial.

## Verification

Linux Release passed 39/39 and Windows Release passed 51/51; packages
were installed and installed consumers ran. Two platforms and two PCI
domains reported different versions: OpenCL 1.2 fixture-A and OpenCL
3.0 fixture-B. The shared fixture does not export clGetPlatformInfo, so
the dlopen/dlsym test can obtain only per-device CL_DEVICE_VERSION.
API tests cover errors and empty lists, limits, persistent growth,
null or duplicate handles and owners, CPU type, invalid text sizes
or types, missing NUL, tokens with prefixes or suffixes, invalid or
ambiguous PCI, isolated version failure, and unavailable API. Both
OpenCL tests also pass in a Linux configuration without optional
Vulkan headers.

A native WSL query found no GPU PCI match. **Physical Linux DRM/OpenCL
validation remains pending**, as for Vulkan. Synthetic tests do not
replace it. Drivers without the standard PCI extension and devices
on other buses remain unavailable.

Sources: [per-device PCI extension](https://registry.khronos.org/OpenCL/specs/unified/refpages/man/html/cl_khr_pci_bus_info.html),
[PCI structure](https://registry.khronos.org/OpenCL/specs/unified/refpages/man/html/cl_device_pci_bus_info_khr.html),
[device enumeration](https://registry.khronos.org/OpenCL/specs/unified/refpages/man/html/clGetDeviceIDs.html),
and [CL_DEVICE_VERSION](https://registry.khronos.org/OpenCL/specs/unified/refpages/man/html/clGetDeviceInfo.html).
