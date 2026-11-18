# Vulkan on Linux: per-GPU version

The earlier assignment of `vkEnumerateInstanceVersion` to every
`GpuInfo` was corrected: it describes the loader, not a graphics card.
`vulkan_version` now contains the physical device's `apiVersion` only
after a unique PCI match is verified in both inventories. There is no
fallback based on name, vendor, order, or loader version.

## Query

The provider runs after DRM and NVML are combined. It uses an available
physical location: a full BDF with a four- or eight-digit hexadecimal
domain, normalized with the NVML PCI parser. Estimated, invalid, or
duplicate locations do not get a version. The domain matters: cards
with identical bus/device/function in different domains remain distinct.

The provider opens libvulkan.so.1 with local dlopen and resolves all entry
points before creating a 1.1 instance. There are no external commands,
downloads, installations, surfaces, logical devices, queues, or workloads.
The driver can initialize, so the query is not free. No layers or
extensions are explicitly enabled; the loader controls implicit ones.
VK_EXT_pci_bus_info is checked per device before querying
VkPhysicalDevicePCIBusInfoPropertiesEXT. Bus must be at most 255, device
at most 31, function at most 7, and the domain must fit in 32 bits.

Limits are 256 devices, 4,096 extensions per device, and three attempts on
VK_INCOMPLETE. Errors, incorrect buffers or counts, null or duplicate
handles, and extensions without NUL invalidate the Vulkan sample.
Results are published only after the list is validated; other metrics
are not removed. A PCI address repeated across ICDs or devices is
ambiguous even if versions agree. CPU devices are skipped. Variant 0,
major 1 versions are displayed as major.minor.patch, without capping
the physical version at the version requested by the instance.

Missing extensions, loader, API, identity, or a valid version leave the
value absent with a reason, not zero or proof of no hardware support.
Vulkan errors do not always establish permission denial. The instance
and library use RAII, with no permanent cache or worker. There is no
atomic transaction against hotplug or deadline for a blocked driver.

## Optional headers

CMake looks for PCINSIGHT_VULKAN_INCLUDE_DIR and checks structs and
macros. Without compatible headers it builds the rest and leaves a
`not built` note on the field. PCINSIGHT_HAS_LINUX_VULKAN_HEADERS=OFF is
also supported. The library does not link to libvulkan or expose Vulkan
headers in its public API; installed consumers do not need those
dependencies. Headers already present in the local SDK were copied to
work for this build. No software was downloaded or installed. The old
global value does not reappear when headers are absent.

This is an intentional semantic change: a previously global field can
now be absent if identity is not verified. The [Linux OpenCL update](LINUX_GPU_OPENCL.en.md)
also removes the first-platform fallback and binds versions by PCI.
Both implementations were tested; physical Linux validation remains open.

## Verification

Linux Release passed 37/37 and Windows Release passed 51/51; packages
were installed and external consumers ran. Injected tests cover two
PCI domains with versions 1.3.123 and 1.4.123 versus loader 1.4.500;
four- and eight-digit BDF domains; missing, estimated, and invalid
identity; duplicates in both inventories; missing or truncated
extensions; errors and bounds; persistent and recovered INCOMPLETE;
CPU devices; invalid versions; and clearing stale values. A
libvulkan.so.1 fixture tests dlopen/dlsym and actual provider matching
against two synthetic devices. The separate build without optional
headers and its test also passed.

The native loader ran in WSL, where /sys/class/drm did not expose
cards and no PCI match was obtained. **Physical Linux validation
with a DRM GPU remains pending.** Neither the fixture nor AMD testing
on Windows substitutes for it. Other buses and drivers without
VK_EXT_pci_bus_info remain unavailable.

Sources: [per-device extension](https://docs.vulkan.org/refpages/latest/refpages/source/VK_EXT_pci_bus_info.html)
and [PCI properties](https://docs.vulkan.org/refpages/latest/refpages/source/VkPhysicalDevicePCIBusInfoPropertiesEXT.html).
