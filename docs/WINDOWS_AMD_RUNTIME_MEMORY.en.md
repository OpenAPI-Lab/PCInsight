# Experimental AMD runtime memory query (Windows)

Revision 56. On 2026-10-21, the user explicitly chose to add an experimental
opt-in query separate from global memory, after discussing the potential
internal GPU state and resource use and the runtime-dependent scope.

## Explicit API, not enabled by Monitor

```cpp
#include <pcinsight/windows_amd_runtime_memory.hpp>
auto view = pcinsight::query_experimental_windows_amd_runtime_memory();
for (const auto& gpu : view.gpus) {
    // reported_free_bytes and reported_largest_block_bytes are estimated.
    // Do not assign them to GpuInfo.memory_available_bytes or call them globally free memory.
}
```

Only this explicit call runs the provider. There is no default option,
automatic registration, worker, timer, fallback in Monitor/Sampler, or
history storage. The `GpuInfo` and `SampleOptions` layouts are unchanged.
The query returns a new, separate WindowsAmdRuntimeMemorySnapshot type with
WindowsAmdRuntimeMemory rows: a session-local DXGI adapter_id, two
Metric<uint64_t> values in bytes, and an optional CL_DRIVER_VERSION. This
is not the driver marketing version or DXGI UMD version. The GUI decides
whether to offer and poll this experimental capability separately.

An estimated snapshot means at least one row contains estimates, not that
all rows were read. Inspect each Metric. No data yields unsupported; loader
access denial yields permission_required. Complete DXGI inventory with no
AMD GPU yields available and an empty list. Outside Windows the result is
unsupported with an empty list, not a successful empty hardware inventory.
DXGI can throw system_error as in Monitor. Missing data does not become
zero; zero written by a valid query is a value.

## New format evidence and semantic limits

The registered AMD specification identifies a free-memory query in KBytes
but does not fully describe its result. Research in this revision clarifies
what had been an unknown format: the current Khronos bindings use
vector<size_type>, and the published AMD CLR code describes two size_t
values: the reported total and the largest block. Installed AMD drivers
are not assumed to run that exact code.

This 64-bit Windows decoder requests both size_t values directly (16 B),
checks success and exact size, initializes both with sentinels, and
requires block <= total. It does not treat a zero-size query as the
definitive layout: the AMD code examined can return one element if the
buffer is smaller than the pair. There is no scalar fallback. Conversion
from KiB multiplies by 1,024 with an overflow check. Full integers, even
above 2^53, are preserved without double conversion or clamping to DXGI
capacities.

The published PAL backend first prepares heap resources and computes
process and cache usage; it can incorporate Windows budget information
and apply reservations and adjustments. Knowing the format does NOT
make the result globally free physical memory. Only the two figures
reported by the runtime are exposed, always as estimated with experimental
notes. They do not certify that an allocation of the reported block size
will succeed.

Primary sources reviewed:
[registered extension](https://registry.khronos.org/OpenCL/extensions/amd/cl_amd_device_attribute_query.txt),
[type in the Khronos bindings](https://github.com/KhronosGroup/OpenCL-CLHPP/blob/main/include/CL/opencl.hpp),
[two elements and AMD buffer behavior](https://github.com/ROCm/clr/blob/develop/opencl/amdocl/cl_device.cpp),
[PAL backend calculations and resources](https://github.com/ROCm/clr/blob/develop/rocclr/device/pal/paldevice.cpp).

## Matching, resources, and scope safeguards

The query uses the complete DXGI inventory and the shared [D3D11/OpenCL
matching helper](WINDOWS_OPENCL_INTEROP.en.md). It requires enumerated
root devices, an observed one-to-one association, and topology compatible
with the entire DXGI list, not matching by model or order. This does NOT
prove physical LUID identity or coverage of all nodes in a linked adapter.
AMD vendor ID 0x1002 must be available in DXGI and match CL_DEVICE_VENDOR_ID.
The cl_amd_device_attribute_query extension must appear as an exact token.
CL_DRIVER_VERSION is bounded, nonempty, and revalidated after the read.
Vendor and extension are checked again, and the association map is
rebuilt before publication. A change, missing data, or ambiguity yields
unknown. There is no atomic transaction against hotplug or ABA changes
and no guarantee about drivers that misreport identity.

OpenCL.dll is requested with System32 search and entry points are resolved
dynamically. No SDK, OpenCL link dependency, CLI, or installation. Owned
module and DXGI references are released through RAII; OpenCL IDs are
borrowed. PCInsight does not call clCreateContext, create buffers, compile
kernels, or change a host context. **The runtime can do so internally or
prepare heaps**, retain state, threads, or resources, and affect usage.
Zero impact or restoration of runtime state is not promised. Configuration,
environment, clocks, and privileges are unchanged. There is no timeout for
a blocked driver. The caller controls query frequency and serialization.

## Verification and remaining scope

Windows MinGW 131/131 and MSVC 131/131, Linux 111/111 Release. Twenty
repetitions each of the experimental AMD and prior interop providers per
Windows toolchain. Three installed external consumers rebuilt, relinked,
and run with exit code 0. The Linux consumer checks unsupported; the
Windows consumer does NOT invoke the experimental query by default.
The new header is installed without exported SDK dependencies.

Positive native results on AMD RX 7900 XT with both toolchains,
CL_DRIVER_VERSION 3679.0 (PAL,LC). An initial MinGW sample reported
21435580416 B and a largest block of 21413273600 B. These are dynamic
runtime values, not expected constants or certified global memory. The
reported figure exceeds even the DXGI dedicated capacity observed on
this host. It is not clamped, and the providers are not claimed to
measure the same thing. After the call, Monitor still leaves AMD
memory_available_bytes unknown.

Injected tests cover two GPUs with different values, zero, large integers,
overflow, sentinel/missing/partial writes, scalar returns or wrong sizes,
block > total, failed queries, extension suffixes, incorrect or changed
vendors, missing or changed driver version, changed topology or association,
duplicates, and missing APIs. Isolated failure does not erase another
valid estimate. Tests check release of all handles and factories actually
returned, including early returns. The first test assumed four adapters
were always closed, but an early rejection opens fewer. The test was
corrected to compare opens with closes without removing the check.
Initial logs are retained in `work/amd-runtime-*`; final logs use final-*,
repetitions repeat-*, installation install-*, and consumers consumer-*.
MSVC D9025 from /UNDEBUG is expected.

Extracting the association helper preserves tests for the previous
OpenCL version provider and its staging before publication. It does not
add this memory query to that automatic provider. This is an additive
0.2 API without changes to existing layouts, vtables, or formats. No
physical coverage of other AMD devices, UMA, linked adapters, or other
drivers; no positive NVIDIA result, Intel free memory, or universal
test matrix. **M04 remains open** for global memory and pending coverage;
this estimate does not replace that requirement.
