# Windows Vulkan by adapter

## Revision 53 update: header path and MSVC

Windows detection now accepts `PCINSIGHT_VULKAN_INCLUDE_DIR`, as Linux does,
and also searches `VULKAN_SDK/Include` when that environment is configured.
Previously it only checked the compiler's implicit include path: MinGW
found Vulkan, but the MSVC build verified in revision 52 left it
`not built`. The path is used only to build the library and its internal
test. It is not exported to consumers, the loader is not linked, and
no SDK is downloaded.

Example using an existing copy of the headers. Pass the directory that
contains `vulkan/vulkan_core.h`, not the file itself. If the build cached
a negative result, clear just that CMake check so it runs again:

```powershell
cmake -S outputs/pcinsight -B work/build-msvc-release `
  -U PCINSIGHT_HAS_VULKAN_HEADERS `
  -DPCINSIGHT_VULKAN_INCLUDE_DIR:PATH=C:/ruta/Vulkan-Headers/include
```

`-DPCINSIGHT_HAS_VULKAN_HEADERS=OFF` remains an explicit opt-out, even
when the path exists. Without headers, the rest of the library still
builds. No version is fabricated and nothing is installed automatically.
Changing the path does not automatically invalidate the cached CMake
check. Use `-U` as shown above or a fresh build; do not force `ON`
without checking.

Destination ambiguity is also handled: two GPU rows with the same DXGI
ID do not receive the same Vulkan reading. Both become unknown and any
previous value is cleared; another row with a unique ID keeps its reading.
A row with an empty ID receives no result. This complements the existing
check for duplicate LUIDs across Vulkan devices without making hotplug
transactional.

Local Release verification for this component: Windows MinGW 128/128
and MSVC 128/128, Linux 111/111. Twenty repetitions of `windows-gpu-vulkan`
per Windows compiler included the new injected cases. MSVC and MinGW
queried the RX 7900 XT natively and obtained Vulkan 1.4.349 by LUID; this
was not merely a stub build. A MinGW build with the provider explicitly
OFF was also checked (1/1 test). Three installed external consumers were
rebuilt and run; the exported CMake target contained no Vulkan path.
Logs: `work/vulkan-portable-*`. MSVC retains the intentional D9025 from
test /UNDEBUG; warnings are not disabled. No new API, layout, vtable, or
formats. M04 remains open for global available memory and real adapter
and vendor coverage, neither of which this work certifies.

Limited free-memory research: the [AMD extension registered with
Khronos](https://registry.khronos.org/OpenCL/extensions/amd/cl_amd_device_attribute_query.txt)
describes `CL_DEVICE_GLOBAL_FREE_MEMORY_AMD` in KBytes, but does not detail
the result's type or composition there. This revision does not guess a
decoder or subtract PDH usage from DXGI capacity. Resolving that approach
is not a prerequisite for this Vulkan component.

## Provider contract

`Monitor.sample(include_gpus=true)` fills `GpuInfo.vulkan_version` with
`VkPhysicalDeviceProperties.apiVersion` from the corresponding physical
device. This is not the loader version or the version enabled by an
application. The major.minor.patch is retained for Vulkan variant 0,
major version 1. The highest version in the system is not assigned to
every card.

## Identity and query

`vulkan-1.dll` is loaded only from System32, and the five required core
entry points are resolved before creating an instance. The existing loader
must offer Vulkan 1.1. The instance requests 1.1, with no explicit layers
or extensions, surfaces, logical devices, queues, or graphics workload.
System-configured implicit layers remain under loader control. Nothing
is installed and no external program runs.

The `VkPhysicalDeviceProperties2 -> VkPhysicalDeviceIDProperties` chain
returns deviceLUID. It is compared to the DXGI ID only when deviceLUIDValid
is exactly VK_TRUE and deviceNodeMask has one bit. Matching does not use
the name, vendor, device ID, order, or an invented UUID. CPU devices are
skipped. A LUID shared by multiple devices, such as linked nodes or
alternative ICDs, is ambiguous and gets no value. The first or highest
result is not chosen.

Enumeration is limited to 256 devices and three attempts for VK_INCOMPLETE.
Errors, a truncated list, counts outside the buffer, null or duplicate
handles, and malformed identity metadata invalidate the whole query.
Results are published only after the list is validated; partial output
does not hide duplicates. A missing ID or invalid version leaves that
field absent. This does not remove DXGI inventory or invalidate other GPU
providers.

A missing loader or entry point, version 1.0, or a Vulkan error yields no
value, not version zero or proof the hardware lacks Vulkan. Win32 access
denied when loading the DLL is marked permission_required. Vulkan errors
do not reliably map to Win32 permissions and are not reclassified.
The instance and module are released through RAII. There is no permanent
cache or worker. Hardware or driver changes between APIs are not handled
as an atomic transaction, and this implementation imposes no timeout on
a blocked driver.

## Building without a mandatory dependency

CMake checks that the available Vulkan headers include the required
structures, functions, and macros. It does not download packages or
require an SDK. If headers are absent, the rest of PCInsight builds and
the field stays absent with a `not built` note.
`-DPCINSIGHT_HAS_VULKAN_HEADERS=OFF` explicitly disables this provider at
build time. Installed public headers and libraries do not depend on Vulkan
headers or require linking vulkan-1. Consumers do not need to install a
loader if they only want other metrics.

## Verification

Windows Release 51/51 and Linux 35/35; installed packages and external
consumers were exercised on both. The `windows-gpu-vulkan` test covers
distinct GPUs with distinct versions, a signed LUID high half, loader 1.0
or error, failed or null creation, cleanup, missing APIs, counts beyond
the limit or buffer, null or duplicate handles, persistent or recovered
INCOMPLETE, absent/invalid/ambiguous LUIDs, invalid masks, version zero
or another variant, CPU devices, and an empty list. Native query and
Monitor off/on/off were tested. A separate build with
PCINSIGHT_HAS_VULKAN_HEADERS=OFF and its test were also run.

Native RX 7900 XT: Vulkan `1.4.349`. This is an observation from the
installed driver, not a constant. No physical validation of multi-GPU
systems, other vendors, old loaders, or hotplug. At this stage, Windows
OpenCL/OpenGL and temperature were still pending.

Linux update: the [Linux Vulkan guide](LINUX_GPU_VULKAN.en.md) describes
removal of the global loader fallback and addition of unique PCI matching,
verified with a synthetic API and loader. Physical DRM validation in
Linux is still needed; the Windows test does not substitute for it.

Sources: [deviceLUID and DXGI matching](https://docs.vulkan.org/refpages/latest/refpages/source/VkPhysicalDeviceIDProperties.html),
[device versus instance version](https://docs.vulkan.org/spec/latest/chapters/extensions.html),
[Windows loader core exports](https://github.com/KhronosGroup/Vulkan-Loader/blob/main/loader/vulkan-1.def).
