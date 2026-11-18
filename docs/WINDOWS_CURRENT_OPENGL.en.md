# Windows OpenGL: matching the current context to a DXGI adapter

Partial M04, revision 52, 2026-10-21. The new WGL provider is integrated
into Monitor through the existing include_current_opengl_context option
(false by default). The library does not create a window or context, draw
or run a workload, or change the context, profile, pixel format, or settings.
It runs no programs and installs no SDKs or drivers.

```cpp
pcinsight::SampleOptions options;
options.include_gpus = true;
options.include_current_opengl_context = true;
// Call on the thread that ALREADY has the host's current desktop WGL context.
auto snapshot = monitor.sample(options);
for (const auto& gpu : snapshot.gpus) {
    if (pcinsight::metric_is_known(gpu.opengl_version)) {
        // CONTEXT version associated with this adapter, not a hardware maximum.
    }
}
```

Without a current context, a valid identity contract, or a unique match, the
field is absent/unsupported with a reason. It is not zero or a global loader
version. Disabling the option skips WGL, as does include_gpus=false. Sampler
runs on its own thread and does not inherit the GUI thread's current context.
Do not change this behavior by creating a context automatically in the collector.

## Attribution and scope

A stable current context and DC and a valid, bounded desktop GL_VERSION
are required. For GL >=3, extensions are enumerated through
glGetStringi/GL_NUM_EXTENSIONS without using legacy GL_EXTENSIONS in a core
profile. Older contexts use the legacy list. An exact base and Win32
extension pair is required for either EXT_memory_object or EXT_semaphore;
prefixes, suffixes, duplicates, and malformed metadata do not establish support.

GetUnsignedBytevEXT(DEVICE_LUID_EXT) returns the eight bytes the specification
associates with IDXGIAdapter1. The LUID must match EXACTLY one DXGI row, not
the name, vendor, device ID, or order. The LUID, version, and context/DC
checks are repeated before publishing; detected changes discard the result.
The two LUID output buffers start with different bytes to detect missing or
partial writes. A consistently reported zero LUID can match a row; zero is
not treated as failure, and no validity indicator is invented.
[EXT Win32 identity contract](https://raw.githubusercontent.com/KhronosGroup/OpenGL-Registry/main/extensions/EXT/EXT_external_objects_win32.txt),
[base EXT function contract](https://registry.khronos.org/OpenGL/extensions/EXT/EXT_external_objects.txt).

Windows GpuInfo describes a DXGI ADAPTER. The provider publishes GL_VERSION
from the context associated with that adapter, not its maximum supported
version or core/compatibility capability, DirectX level, the capability of
an internal physical GPU, or proof of hardware acceleration. The node mask
is neither queried nor inferred. This context is not assigned to other
adapters, and a duplicate LUID in the inventory leaves every affected row
without a value. The report preserves the full returned string, not a
marketing version.

## Loading, limits, and side effects

opengl32.dll is loaded ONLY from System32 with an RAII reference, and the
basic entry points are resolved. EXT functions are resolved for the current
context without caching between contexts. NULL and the 1/2/3/-1 sentinel
values are rejected. A missing module or function leaves the value unknown;
an observed Win32 access denial on load retains permission_required.
GL failures without a permission code are not reclassified.
[WGL and the calling thread](https://learn.microsoft.com/en-us/windows/win32/api/wingdi/nf-wingdi-wglgetcurrentcontext),
[context-specific function lookup](https://learn.microsoft.com/en-us/windows/win32/api/wingdi/nf-wingdi-wglgetprocaddress).

Limits: 1,024 B for printable ASCII GL_VERSION, 65,536 B for the legacy
list, 4,096 extensions, 256 B per indexed name, and 128 KiB of retained
indexed names. The numeric version must have two or three components,
major >0, and components <=65535; a printable suffix is allowed. OpenGL ES
is not reported as desktop GL. Out-of-limit or unterminated data is rejected.
Driver pointers must satisfy the API contract: there is no isolation from a
faulty driver or invalid pointers.

The library does NOT call glGetError or consume previous errors. As with any
GL query, a driver that violates its contract can generate new errors.
There is no promise to restore error state, and the library does not try to
do so by consuming the host's errors. The final queries were verified here
without new errors. There is also no timeout for a blocked driver. Repeated
checks are not transactional and do not protect against ABA changes or
hotplug after the call returns.

No changes to SampleOptions/GpuInfo layouts, signatures, vtables, history
formats, or version 0.2. The option previously applied only to Linux. The
Windows opt-in now runs these queries on the calling thread. Rebuild and
relink the library and consumers together. Linux keeps EGL/GLX unchanged.
The package does not require consumers to link opengl32 because it loads it
dynamically. Only the test executable links OpenGL/GDI/User to create its
private context.

## Validation and native diagnostics

The test creates ONLY its own private hidden window and context, restores
the previous pair, and removes its context, DC, window, and class. It does
not call SwapBuffers or draw. CTest limits the process to 40 seconds.
Coverage includes no context, memory/semaphore extension pairs, legacy and
indexed extensions, invalid or ES versions, limits, duplicate names,
missing functions and sentinels, a known zero LUID, missing/partial/changing/
unrelated LUID output, changing DC/context/version, and duplicate DXGI rows.
No stale versions are kept and unrelated metrics are unchanged. Monitor
on/off and include_gpus=false were verified; an installed consumer with no
context receives no fabricated version and opens no context.

Bounded RX 7900 XT diagnostic: the private context reported
`4.6.0 Compatibility Profile Context 26.8.1.260810` and a LUID matching DXGI.
An additional node query made ONLY IN THE TEST returned GL_INVALID_ENUM
(1280) without writing its output. The first prototype, which required a
node, returned unknown. The contract was revisited: a LUID is enough for
the ADAPTER-level context version; attribution to a physical node is not
needed. The unnecessary query was removed from the provider without
converting -1/0 into a valid node or claiming physical capability. The
earlier diagnostic is preserved in work/wgl-identity-diagnostic.log and
wgl-final-native-mingw.log.

The final provider gave a positive native result in MinGW and MSVC: the
same LUID/version only on the correct row, both through a direct call and
through Monitor, with the context/DC unchanged. Tests seeded an existing
error and verified it was not consumed, then ran without a prior error to
check that no new error was masked by the old one. Renderer names are not
accepted as proof of identity. This is an observation of the current driver,
not a multi-vendor, multi-GPU, RDP, or headless test matrix or a maximum
version per adapter.

Final Release suites: Windows 128/128 MinGW and 128/128 MSVC, Linux 111/111.
The WGL test was repeated 20 times per Windows toolchain. Independent
installed consumers from all three toolchains were rebuilt, relinked, and
run with exit code 0. Final logs: work/wgl-adapter-{build,native,suite}-{mingw,msvc}.log,
wgl-final-{build,suite}-linux.log, wgl-install-*, and wgl-consumer-*.
Final delivery evidence, including a known zero LUID, negative HighPart,
absent/partial outputs, and sentinel-safe diagnostics, is in
wgl-release-{build,suite,repeat,install,consumer}-{mingw,msvc}.log and
wgl-release-{build,suite,install,consumer}-linux.log. Twenty final repetitions
per Windows toolchain. Earlier prototype repetitions are retained, but not
counted as coverage of every final case.
MSVC retains the intentional D9025 from test /UNDEBUG; warnings are not
disabled. Initial and intermediate logs do not replace evidence for the
final scope. M04 remains open for its other criteria and coverage, with
no new closure or fractional credit; M05 remains deferred by user decision.
