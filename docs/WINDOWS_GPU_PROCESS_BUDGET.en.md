# GPU memory budget for the calling process

Revision 54 update: [Windows NVML](WINDOWS_NVML_MEMORY.en.md) adds an
INDEPENDENT source in Monitor for unallocated framebuffer memory on an
unambiguously identified NVIDIA GPU. The historical references below to
unknown free memory still apply to DXGI budgets, not to that new provider.
Budget, CurrentUsage, and AvailableForReservation are not used to fill free
memory.

`GpuInfo.local_process_memory` and `non_local_process_memory` contain
GpuProcessMemoryBudget values. These are DXGI budgets for the process
calling PCInsight and physical node 0, separate from adapter-wide capacity
and usage. **They do not fill memory_available_bytes or represent global
free memory.**

## Fields and contract

- process_id: the actual caller PID in that sample, not an arbitrary PID
  selected in the GUI or an aggregate of all processes.
- node_index: requested index 0, not the number of nodes or an aggregate
  of linked GPUs.
- budget_bytes: the target memory budget Windows assigns to the process.
- current_usage_bytes: usage for that process, node, and segment group.
- available_for_reservation_bytes: the amount the process can request as
  a reservation, not physically free memory or a guarantee that a block
  of that size can be allocated.
- current_reservation_bytes: current reservation, an indication of the
  working set.
- budget_headroom_bytes: max(budget_bytes-current_usage_bytes,0), using
  saturating 64-bit integer arithmetic. Not allocatable or free memory:
  usage can exceed the budget.

All sizes are in bytes. LOCAL and NON_LOCAL are DXGI memory segment
groups. They are not renamed dedicated/shared because that equivalence
depends on the adapter and UMA. Do not sum process budgets as physical
capacity. Reported zeros are valid. Full UINT64 values are preserved
without converting to double. Headroom is defined exactly by the formula,
not an estimate of physical availability; its note preserves that scope.

## Query

The same IDXGIAdapter1 used for inventory is queried for IDXGIAdapter3,
without enumerating again or matching by name or index. QueryVideoMemoryInfo
queries node 0 for each group independently. A LOCAL failure does not erase
NON_LOCAL, or vice versa. A missing interface, permission issue, or query
failure yields no value, not zero. The PID and requested index are retained
even when the query fails: they are context, not measured data. Results
are reset on each call so errors do not leave stale values.

RAII releases the extra COM reference, even if a failure returns a pointer.
No D3D device, workload, reservation changes, callbacks, worker, affinity
changes, SDK installation, or external tool. DXGI was already a backend
dependency. SetVideoMemoryReservation is not called. The query can touch
internal driver state; there is no imposed deadline for a blocked API.
The two queries and other providers do not form an atomic transaction.

## Remaining scope

Only the calling process and node 0 are covered, not arbitrary PID budgets
or every node. Linux leaves these fields absent and retains its earlier
providers. At this stage Windows leaves memory_available_bytes absent:
it does not subtract PDH usage from DXGI capacity or present this budget
as free memory. The KMT segment statistics examined are documented as
reserved for system use; they were not adopted as a stable free-memory
source. The AMD OpenCL query evaluated at this stage was not implemented:
its public layout and scope contract needed more validation. No types or
second output were guessed.

Public fields added in 0.x require rebuilding the library and consumers
together. This component is complete; global free memory is separate
and is not marked complete by this work.

## Verification

Windows Release 55/55 and Linux 39/39; installation and external consumers
were exercised. Injected tests cover exact adapter identity, independent
groups, fields and PID/node, genuine zero, usage above budget without
underflow, maximum UINT64, permissions, absent/null/failed interface, a
single group failure, stale-value reset, incomplete function table, and
COM cleanup even on exceptions. The test preserves a fixture's global
metric unchanged. Injected inventory assigns different budgets to two GPUs.
Monitor off/on and native query: RX 7900 XT, PID 22616 in that run, node 0,
local budget 20607967232 bytes and usage 0. These values apply to that
process at that time, not free capacity or constants. Intel, NVIDIA, LDA,
and physical hotplug are not certified.

Sources: [process and node scope](https://learn.microsoft.com/en-us/windows/win32/api/dxgi1_4/nf-dxgi1_4-idxgiadapter3-queryvideomemoryinfo),
[meaning of the four fields](https://learn.microsoft.com/en-us/windows/win32/api/dxgi1_4/ns-dxgi1_4-dxgi_query_video_memory_info),
[reserved KMT statistics](https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/d3dkmthk/ns-d3dkmthk-d3dkmt_querystatistics_segment_information).
