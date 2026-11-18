# G06: Linux GPU telemetry audit by provider

The DRM/sysfs reader replaces the prior inventory's unchecked
std::stod/stoull conversions. Empty, unexpectedly negative, truncated,
suffixed, or overflowing numbers leave the field absent. They do not
abort the rest of the snapshot or turn into zero. Native reads are
bounded to 4 KiB, do not follow attribute symlinks, and retry EINTR
at most three times. The reader does not open DRM nodes, change
policies, run commands, install packages, or require root.

## Fields and sources

| Field | DRM/sysfs | NVML |
| --- | --- | --- |
| utilization_percent | Integer gpu_busy_percent from 0 to 100; reported overall utilization | Active-kernel window reported by NVML |
| dedicated_memory_bytes | mem_info_vram_total | Reported total framebuffer |
| memory_used_bytes / dedicated_memory_used_bytes | mem_info_vram_used; explicit alias between the fields | Used framebuffer v1, including driver reservation |
| memory_available_bytes | Total minus used, **estimated**, non-atomic headroom | Free memory reported in the same valid NVML tuple |
| shared_memory_bytes / shared_memory_used_bytes | Absent; GTT counters are not renamed | Absent; these queries do not provide Windows shared pools |
| temperature_celsius | One identifiable edge/core/GPU candidate, not the first arbitrary sensor | NVML GPU temperature query |
| driver_version | version attribute of the actual bound module | NVML system driver version |
| physical_location | Device PCI location, with directory and binding checks | Provider PCI identity; association only when unique and available |

Overall utilization is not presented as a specific 3D, copy, or compute
engine. DRM fdinfo engines are a different provider and scope; they are
not summed here. A correctly read zero is known (0% busy, 0 bytes used,
or 0 °C); absence is not zero. A known total without usage retains the
total and leaves headroom unknown. If used exceeds total, usage and
headroom are rejected but the read capacity is retained. Sampling is
sequential, not a transaction with respect to driver changes.

Sources: [AMDGPU memory and utilization interfaces](https://docs.kernel.org/gpu/amdgpu/driver-misc.html),
[AMDGPU hwmon](https://kernel.org/doc/html/latest/gpu/amdgpu/thermal.html),
and [NVML queries](https://docs.nvidia.com/deploy/nvml-api/latest/api/group__nvmlDeviceQueries.html).

## Linux details without fabricated equivalents

`GpuInfo.linux_sysfs_details` exposes Metrics for the bound driver name,
module name and version, VRAM total/used/headroom, GTT total/used/headroom,
and a sensor list with ID, label, and Celsius value. Driver and module
names are not versions. The provider does not look up a driver-named
directory in /sys/module assuming the names match; it follows the
driver/module binding. A built-in module or one without a version
attribute yields absence, not the name or uname output as fallback.

GTT is Linux manager accounting for its capacity, usage, and headroom.
It is not free RAM, physically dedicated memory, or Windows shared
reservation. The GUI can display it as GTT with its name and note.
GTT is not mixed with VRAM or used to fabricate shared_memory_bytes.
The details retain the DRM observation even when NVML separately
supplies the general fields.

All headroom values derived by subtraction are estimated. They do not
guarantee that an allocation of that size will succeed. Per-process
budgets, Vulkan heap sizes, and OpenCL allocation limits are not used
as global free memory. The Windows API's LOCAL/NON_LOCAL budgets stay
separate, without invented Linux values.

Sensors retain their label (edge, junction, mem, and others) and
identity, up to 128 channels and 32 per hwmon. Duplicate and invalid
IDs are rejected. The generic field requires one channel labeled
edge, GPU, gpu, or core, or unlabeled temp1 from the amdgpu driver
under its contract. Junction and mem are not treated as edge, and
two candidates are not resolved arbitrarily. An unknown label does
not prevent a reading from appearing in the list. Signed
millidegrees are converted to Celsius. The sanity range is −273.15
to 1,000 °C; invalid readings are not clamped.

## Identity and merging

Inventory accepts only card followed by a decimal index, not
connectors or arbitrary card names. The card and device directories
are pinned by fd. The reader checks their identities by dev/inode,
path, vendor/device, driver/module binding, and module/version
before and after. It does not reuse a global cache from another
card. Observed changes discard the row; other faulty attributes
fail individually. These checks do not block hotplug or guarantee
an atomic snapshot. Inventory is bounded to 1,024 entries and
reports partial coverage if rows are rejected or limits exceeded.

The [NVML provider](GPU_NVML.en.md) is associated with DRM only when both have an available,
normalized BDF unique on each side. An estimated location, name,
or index is never used. Its total/used/free tuple replaces DRM only
when complete, available, and consistent without overflow. Capacity
from one provider is not mixed with usage or free memory from the
other. Errors or missing symbols retain the DRM fallback.
memory_used and dedicated_memory_used update together. Shared memory
is not fabricated. Dates follow the separate G08 policy.

`SystemSnapshot.linux_gpu_inventory_source` distinguishes opt-out,
inaccessible sources, completed empty enumeration, and partial
DRM/NVML coverage. One complete source without the other is
estimated; both complete is available. This does not establish that
all GPUs were discovered on the host, in containers, or without
these drivers.

## Scope of completion

This audit covers implemented DRM/sysfs and NVML fields and providers,
including actual absences and shared/available memory semantics.
It is not universal Intel/AMD/NVIDIA telemetry, per-process memory,
MIG/vGPU, Windows pools on Linux, or every alternative driver source.
OpenGL, Vulkan, OpenCL, PCI names, and driver date are documented
separately. Physical hardware, driver, and distro validation remain
under G07/N02; Windows remains under M04. Missing sources are not
turned into fabricated estimates. The new 0.x GpuInfo/SystemSnapshot
layout requires rebuilding **all** consumers.

## Closure verification, revision 21 (2026-10-17)

Linux Release passed 89/89 and Windows Release passed 107/107.
Packages were reinstalled, and rebuilt, relinked external consumers
exited 0. Portable tests cover parsing, zero, overflow, suffixes,
signs, permissions, negative temperatures, sensor selection and
ambiguity, separate GTT, a module without version, and NVML merge
consistency. The native reader was tested on a private sysfs-like tree:
a differently named module, invalid or truncated attributes and
symlinks, changes to identity/vendor/version or directory during
reads, and empty or absent inventory. A BDF-shaped directory without
a verified PCI subsystem does **not** get physical_location.

The first test failed on both OSes because of a new, incorrect
expectation. −1 is invalid for bytes or a percentage, but valid in
millidegrees Celsius. The assertion was fixed to require −0.001 °C
without changing the temperature parser to hide the issue. Initial
logs were preserved in work/gpu-sysfs-first-test-{linux,windows}.log.
The final build had no warnings after correcting the initial Windows
unused warning. Native WSL returned zero GPU rows, a completed empty
DRM enumeration, and unavailable NVML. Its DRM/sysfs source was
estimated; this does not establish absence of physical GPU hardware.
G07 and N02 remain open.

Logs: work/gpu-sysfs-build-{linux,windows}.log,
work/gpu-sysfs-suite-{linux,windows}.log, and work/gpu-sysfs-native-linux.log.
Passing suites do not establish fixes for the prior N03 CPU or history
risks. No GPU configuration or control actions were performed.
