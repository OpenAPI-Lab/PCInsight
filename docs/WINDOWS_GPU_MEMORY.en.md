# Dynamic Windows GPU memory

Revision 56: the [experimental AMD query](WINDOWS_AMD_RUNTIME_MEMORY.en.md),
explicitly chosen by the user, adds separate opt-in types and a call for
runtime estimates. It does not fill memory_available_bytes or run in
Monitor/Sampler. New primary evidence clarifies size_t[2], but does not
resolve the scope, budgets, caches, or internal runtime resources.

Revision 54 update: [Windows NVML](WINDOWS_NVML_MEMORY.en.md) can fill
memory_available_bytes AFTER this PDH stage, only for an unambiguously
identified NVIDIA root device matched by CUDA LUID/UUID with KMT count 1.
It does not subtract PDH usage from DXGI capacity or change the counters
or capacities below. NVML reports unallocated memory, not a guarantee
that it can be allocated or a budget for the caller. There is no positive
result on physical NVIDIA hardware yet; AMD/Intel and failure cases remain
unknown. References below to unknown free memory describe this PDH stage,
not the new independent source.

WindowsGpuMemory, integrated into Monitor through include_gpus, reads
the English local PDH counters GPU Adapter Memory(*) / Dedicated Usage
and Shared Usage. It runs no tools, creates no D3D device, runs no benchmark,
and installs no services. It uses only system PDH and the LUIDs from the
[DXGI inventory](WINDOWS_GPU_INVENTORY.en.md).

## Fields and scope

- dedicated_memory_used_bytes: dedicated usage, in bytes, reported by the
  adapter counter for an unambiguous physical instance under that LUID.
- shared_memory_used_bytes: shared system memory usage, tracked separately.
- Legacy memory_used_bytes: alias for the DEDICATED usage above, NOT the
  sum of dedicated and shared usage or usage by PCInsight's own process.
- dedicated_memory_bytes/shared_memory_bytes retain DXGI's static CAPACITIES;
  they do not become usage fields. dedicated_system_memory_bytes stays separate.
- memory_available_bytes remains unknown: subtracting PDH usage from DXGI
  capacity does not establish free or allocatable memory or a process budget.

The new counters are for the ADAPTER, not GPU Process Memory. Process values
are not added: shared allocations could be counted twice. Likewise,
IDXGIAdapter3::QueryVideoMemoryInfo is not treated as global memory; its
CurrentUsage and Budget apply to the calling process. A consumer that needs
per-process budgets would need a separate contract; that scope is not passed
off as a GPU total. Update: the [GPU process budget guide](WINDOWS_GPU_PROCESS_BUDGET.en.md) adds that separate
contract for the caller and node 0 (LOCAL/NON_LOCAL), without filling the
global memory_available_bytes field.

PDH values are requested as 64-bit integers with NOSCALE/NOCAP100: whole
bytes without automatic scaling or a double that loses precision above 2^53.
A valid counter zero is preserved. A negative, absent, or invalid item or
status does not produce zero. Usage can exceed a DXGI capacity with a
different scope or reservations; it is not clamped and no negative or
invented free value is calculated. Dedicated and shared queries are
separate, not an atomic snapshot with one another, inventory, or other adapters.

## Identities and lifecycle

Accepted names: luid_0xHHHHHHHH_0xLLLLLLLL_phys_N, with upper- or lower-case
hex and a decimal N representable in 32 bits. They are matched to
dxgi-luid:high:low, not by order or model. _Total and per-process counter
names are not assigned to a GPU. Two instances with the same LUID, whether
duplicates or different physical nodes, leave that resource unknown.
Linked adapters are not automatically summed. Coverage of nodes not
exposed by a driver is not promised. Changed LUID formats and ambiguous
suffixes are rejected rather than partially matched.

Each Monitor owns its two queries and counters. An initial preparation
collection and a second collection are required before publishing values,
even though memory is a gauge, not a rate. No delta is fabricated. Failed
open/add/collect/array operations discard only the affected query; recovery
requires fresh preparation and collection. Shared failure does not erase a
valid dedicated value, or vice versa. An invalid item or missing instance
leaves the GPU/resource unknown. A later valid sample can restore it without
retaining figures from an earlier capture.

A changed LUID set, an empty list, or include_gpus=false closes both queries.
Re-enabling requires preparation; no values retained from before disabling
appear. Duplicate or empty input identities are not applied. DXGI inventory
still fails explicitly on enumeration failure. PDH failure affects only
optional metrics and does not remove cards from the inventory. Permission
failures are permission_required; other failures are unsupported, with the
PDH status in the note.

Per-array limits are 4 MiB, 16,384 items, and 128 wchar per name. Size/count,
in-buffer pointers, alignment, termination, item status, and nonnegative byte
counts are checked. If the second call reports growth or fails, there is no
reallocation loop or partial map; a later sample retries with a new query.
Queries and counters close on reset or destruction. There are no hidden
threads or timers, and no guaranteed deadline for a slow PDH provider.
The caller serializes its Monitor. New GpuInfo 0.x fields require rebuilding
consumers. Linux retains its earlier sources and existing memory_used_bytes;
this component does not populate these specific fields there.

## Verification and remaining work

Windows 47/47 and Linux 35/35 Release; external consumers were installed,
rebuilt, and run on both systems. Tests with active checks cover signed/hex
LUID matching, rising and falling values, genuine zero, maximum int64 without
double conversion, permission and open/add/collect/format errors, nulls,
recovery, dedicated/shared independence, absent/duplicate/multinode/per-process
instances, invalid format, negative bytes, limits/pointers/counts/names, and
resource cleanup. Monitor reset was verified by toggling include_gpus.

Native x64 test on an AMD Radeon RX 7900 XT: an initial sample of 1625387008
dedicated and 177475584 shared bytes. These are observed amounts, not
expected values or permanent limits. Readings were also taken through
Monitor without generating a graphics workload. Other drivers, older WDDM,
linked and multi-GPU setups, and physical hotplug are not certified.
At this stage GPU engines/utilization, temperature, properly scoped free
memory, driver date/location, and runtimes remained pending. This component
did not complete all Windows GPU work.

Update: [Windows GPU engines](WINDOWS_GPU_ENGINES.en.md) adds engines and
the estimated observed maximum by LUID/node/index. The other pending items
listed above were unchanged by that update.

Sources: [PDH arrays and flags](https://learn.microsoft.com/en-us/windows/win32/api/pdh/nf-pdh-pdhgetformattedcounterarrayw),
[QueryVideoMemoryInfo and process scope](https://learn.microsoft.com/en-us/windows/win32/api/dxgi1_4/nf-dxgi1_4-idxgiadapter3-queryvideomemoryinfo),
[VidMm and double counting when summing processes](https://devblogs.microsoft.com/directx/gpus-in-the-task-manager/).
