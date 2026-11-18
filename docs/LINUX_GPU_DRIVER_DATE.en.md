# Linux GPU driver date: sources and explicit absence

G08 uses the existing `GpuInfo.driver_date` field (Metric<string>).
For the current Linux providers, value remains absent and support is
unsupported, with a note beginning `Linux driver date unavailable:`.
This is not a zero date, known empty string, pending query, or
permission denial. No actual driver release or installation date was obtained.

This work completes the requested G08 source and availability policy,
whose criterion allows explicit unavailability with a reason. It does
not close the rest of GPU telemetry or implement a source that returns
a date. Existing Windows behavior is unchanged: it queries a device
date from its own provider, not from this policy.

## Sources reviewed

| Provider / scenario | What it reports | Date policy |
| --- | --- | --- |
| DRM/sysfs, including AMD, Intel, nouveau, and other drivers | Identity, module, and versions depending on the driver; DRM_VERSION retains a historical date field | No validated release or installation date source. The obsolete or constant DRM field is not used. |
| NVIDIA NVML | The system query in use obtains the driver version | It does not obtain a release or installation date. Do not parse a version as a date. |
| DRM and NVML merged by PCI | Available metrics from both are combined | Use a combined-source note without dropping the diagnostic during merge. |
| Vulkan/OpenCL/EGL/GLX | API versions, context, or renderer/driver information according to the interface | Versions and informational text are not converted into a kernel driver date. |
| Linux provider not yet registered | No date-source contract | Explicit absence. Do not guess provenance from model, name, or row position. |

Current DRM retains a placeholder in its date field for compatibility.
Historical documentation already warned that the static date was poorly
maintained. Even a YYYYMMDD string from an older kernel does not establish
the installed module's date or the package's release date. The provider
does not add a device open or ioctl just to query that obsolete field.
Sources: [current DRM drm_version](https://github.com/torvalds/linux/blob/master/drivers/gpu/drm/drm_ioctl.c)
and [historical DRM documentation](https://www.kernel.org/doc/html/v4.18/gpu/drm-internals.html).

The NVML query returns a version identifier, not a date. A date-like
string is retained as a version without extracting a calendar value.
[NVML system queries](https://docs.nvidia.com/deploy/nvml-api/latest/api/group__nvmlSystemQueries.html).
CL_DRIVER_VERSION is a provider-specific version. Vulkan driverInfo is
driver information without a release or installation date contract.
[OpenCL clGetDeviceInfo](https://registry.khronos.org/OpenCL/specs/unified/refpages/man/html/clGetDeviceInfo.html),
[Vulkan driver properties](https://docs.vulkan.org/refpages/latest/refpages/source/VkPhysicalDeviceDriverProperties.html).

## Values that are not substitutes

driver_date is not inferred from BIOS/SMBIOS/VBIOS, file
mtime/ctime/birthtime, package installation time, module build text,
srcversion, kernel version, graphics API version, documentation date,
or a downloaded release catalog. These describe different objects or
events, and distro updates or backports can retain versions. The
library does not run modinfo, glxinfo, nvidia-smi, or package managers.
It adds no network use, helper, date cache, SDK, or runtime dependency.

The policy does not rule out a verifiable date from a future driver or
specific API. A source that returns a date must define what it means,
link it to the device and loaded driver, distinguish support from
errors, and be tested. Calendar-shaped numbers are not enough.

## Integration and tests

DRM rows initialize the diagnostic before providers are merged. NVML
sets its note when creating each row. A successful PCI merge uses the
combined-source note. NVML rows kept separate due to missing or duplicate
identity retain their source. NVML initialization failure does not turn
lack of date support into permission_required; that error stays on the
metrics actually queried, such as version or utilization. Windows date
data is unaffected.

`tests/linux_gpu_driver_date.cpp` covers DRM, NVML, combined, and unknown
policies; absence versus a known value; normal versions and five
formats including date-like text; exact merges; separate and duplicate
rows; denied initialization; and GPU opt-out. Logic tests run on Linux
and Windows, but only Linux queries native Monitor. The installed Linux
consumer checks absence, support, and note for all visible rows without
requiring a physical GPU. No public API, ABI, layout, or history format
changed here.

Closure verification on 2026-10-17, revision 20: Linux Release passed
88/88 and Windows Release passed 106/106. Packages were reinstalled
and both external consumers rebuilt, relinked, and exited 0. WSL
reported zero GPU rows in the native test. Missing devices are not
positive physical driver or date coverage. Policy and merge tests use
fixtures without installing or changing drivers. Builds had no warnings.
Workspace-root logs: work/gpu-driver-date-build-{linux,windows}.log,
work/gpu-driver-date-suite-{linux,windows}.log, and
work/gpu-driver-date-native-linux.log. G06/G07/N02/M04 remain separate;
this passing suite does not resolve historical N03 CPU or history risks.
