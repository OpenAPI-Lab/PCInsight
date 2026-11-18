# Free NVIDIA framebuffer memory on Windows

Revision 55 update: the [System32/legacy loader](WINDOWS_NVML_LOADER.en.md)
adds the fixed NVSMI location through Windows ProgramFilesX64 without using
PATH or environment variables. Only absence permits fallback, never access
denial or an invalid binary. Windows Release 130/130 on both toolchains and
Linux 111/111; still no positive physical NVIDIA result. The memory and
identity contract below is unchanged.

Revision 54, M04. `Monitor.sample(include_gpus=true)` can fill the existing
`GpuInfo.memory_available_bytes` field with **unallocated bytes reported by
NVML v1** for an unambiguously identified NVIDIA GPU. It does not subtract
PDH usage from DXGI capacity. This is not the caller's budget, DXGI
headroom, shared capacity, BAR1 memory, or a guarantee that a new allocation
of that size will succeed.

## Source and scope

`nvmlDeviceGetMemoryInfo` uses the v1 layout: three 64-bit integers (total,
free, used). Free is preserved as a 64-bit integer, even above 2^53.
Unavailable sentinels, success without output, free or used greater than
total, and triples that violate free = total - used are rejected. A correctly
reported zero is known, even when total/free/used are all zero. Failures are
not converted to zero.

The result is `Support::available` as a reading reported by that API, not
allocator certification. Under WDDM, Windows manages much of the memory:
NVML's scope is not a promise of allocation or equivalent to PDH usage.
`dedicated_memory_bytes`, `dedicated_memory_used_bytes`, the
`memory_used_bytes` alias, shared memory, engines, temperatures, budgets,
and driver version are unchanged. Do not add or compare readings from
these providers as though they were one accounting snapshot.

Every sample queries again. This provider runs AFTER the PDH provider
resets free memory. NVIDIA rows are reset to unknown before reading;
later errors do not retain previous values. Rows for other vendors or
without a known vendor ID are untouched. Monitor creates a new DXGI
inventory on each sample. `include_gpus=false` skips this provider.

## Matching, not by model or order

Only rows with hardware_vendor_id available and equal to 0x10de are
candidates. The chain is DXGI LUID -> CUDA LUID/UUID -> NVML root GPU UUID:

1. CUDA enumerates up to 256 devices. `cuDeviceGetLuid` and
   `cuDeviceGetUuid_v2` are read twice with different seeds to detect
   missing or partial output and changes. A one-bit node mask is required.
2. The destination LUID, source LUID, and CUDA UUID must each be unique.
   Empty, malformed, or duplicate IDs are not assigned, even with different
   hex capitalization or a duplicate row for a different vendor. The
   first or maximum result is not selected.
3. KMT opens the exact LUID and checks PHYSICALADAPTERCOUNT=1 before and
   after the read. Linked physical adapters or unknown topology are not
   published, even if CUDA's visibility filters expose only one node.
4. The 16-byte UUID is formatted as GPU-8-4-4-4-12 for
   `nvmlDeviceGetHandleByUUID`. An exact round trip with `nvmlDeviceGetUUID`
   is required. MIG prefixes, suffixes, and truncated values are rejected.
   Aliasing the same NVML handle across different UUIDs is detected.
5. After reading memory, the CUDA LUID, mask, and UUID, the NVML UUID, and
   the KMT physical count are revalidated. The CUDA count is rechecked
   at the end. Only then are staged results published. Identity or
   inventory changes can invalidate the whole query; an isolated memory
   failure does not erase another GPU's valid result.

CUDA device integers are local enumeration handles, never public IDs.
Zero LUIDs or UUIDs are not rejected for being zero if both reads write
them and the mapping is valid. A MIG UUID does not represent memory for
the whole adapter. Hidden or incompatible CUDA, TCC without a usable
LUID, or old APIs can leave the field unknown. Repeated reads are not
transactional against hotplug or ABA changes and do not guarantee a topology
the driver reports incorrectly.

## Dependencies, process state, and resources

No CUDA/NVML headers, import libraries, CLI, toolkit, installer, download,
service, or helper is required. Internal C ABI: CUDA WINAPI, NVML cdecl,
16-byte UUID, and 24-byte v1 memory structure. The native wrapper is limited
to 64-bit Windows callers.

nvcuda.dll and gdi32.dll use LOAD_LIBRARY_SEARCH_SYSTEM32. NVML prioritizes
that search and, since revision 55, accepts the fixed legacy location
ProgramFilesX64/NVIDIA Corporation/NVSMI only on absence, with a validated
path and DLL_LOAD_DIR|SYSTEM32 flags. All 13 entry points are checked before
initialization. The working directory, PATH, and environment are not
searched. See the [loader](WINDOWS_NVML_LOADER.en.md) for trust limits and
diagnostics: it does not certify signatures, ACLs, or preloaded modules.
Nothing is installed and PATH is not changed. NVIDIA hardware does not
guarantee NVML support or the required symbols; driver compatibility
cannot be inferred from the DLL location.

The provider calls `cuInit(0)` and pairs every successful `nvmlInit_v2`
with an `nvmlShutdown` attempt, including on errors and exceptions. There
is no cuShutdown API; the provider does not try to destroy global CUDA
state the host may use. Initialization can load driver components,
prepare hardware, and create internal state or threads. Zero impact or
restoration of driver internals is not promised. PCInsight does not create
CUDA contexts, retain the primary context, allocate buffers, launch kernels,
or change the current context, affinity, environment, clocks, or configuration.
It creates no workers of its own. Owned module references and KMT handles
are released through RAII; NVML query handles are borrowed. Failed shutdown
is neither retried nor escalated. The process is not guaranteed to end up
globally uninitialized.

NVML_NO_PERMISSION and KMT STATUS_ACCESS_DENIED map to permission_required.
A numerically equal CUDA code is NOT treated as an NVML code. Other
failures, incomplete ABI, and ambiguous topology yield unsupported with a
reason. Calls to Monitor must still be serialized. There is no timeout
that can interrupt a blocked driver.

## Revision 54 verification and physical limits

Release: Windows MinGW 129/129, MSVC 129/129, and Linux 111/111. Twenty
repetitions of the new test per Windows compiler. Three installed external
consumers rebuilt, relinked, and run without NVIDIA headers or new package
link dependencies. Source: `src/windows_nvml_memory.cpp`; tests:
`tests/windows_nvml_memory.cpp`. Initial and prototype logs are retained in
`work/nvml-free-*`; final logs are `nvml-free-final-*` and repetitions
are `nvml-free-release-repeat-*`.

Injected API tests cover matching two GPUs and negative LUID high halves,
dynamic values, recovery, zero, values above 2^53, missing or partial
writes, counts and limits, changed identities/masks/UUIDs, root/MIG/alias
cases, linked or changed topology, permissions, missing symbols, independent
failures, KMT cleanup, and init/shutdown balance. Existing Monitor, budget,
and consumer tests no longer incorrectly require every Windows system
to lack free memory. They accept a new value only for vendor 0x10de with
NVML provenance and available support. Direct PDH/DXGI checks still require
that those sources do not invent free memory.

Current physical host: AMD RX 7900 XT. Checks confirm that no NVIDIA
reading is assigned to it, that the optional loader/invalid-ID probe stays
unknown with DLLs absent from System32, and that Monitor off/on/off does
not fabricate data. **No positive physical NVIDIA result or validation
of WDDM/TCC, multi-GPU, MIG, linked adapters, or hotplug.** Injected tests
do not certify those cases. This block does not close M04. AMD/Intel global
free memory and remaining physical coverage are still pending. Linux
retains its existing NVML provider and semantics. No new API, layout,
vtable, or formats; optional additional behavior under the existing
include_gpus option.

Primary sources: [NVML queries and WDDM scope](https://docs.nvidia.com/deploy/nvml-api/latest/api/group__nvmlDeviceQueries.html),
[v1 layout and units](https://docs.nvidia.com/deploy/nvml-api/latest/api/structnvmlMemory__t.html),
[CUDA identities](https://docs.nvidia.com/cuda/cuda-driver-api/cuda_driver_api/group__CUDA__DEVICE.html),
[NVIDIA UUID matching example](https://github.com/NVIDIA/nvbandwidth/blob/main/inline_common.h),
[Windows NVML locations](https://docs.nvidia.com/deploy/nvml-api/nvml-api-reference.html),
[KMT physical count](https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/d3dkmthk/ne-d3dkmthk-_kmtqueryadapterinfotype).
