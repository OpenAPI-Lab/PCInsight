# NVIDIA NVML provider on Linux

Revision 21: the G06 [GPU telemetry audit and fixes](LINUX_GPU_TELEMETRY.en.md)
require an available PCI location and a complete, consistent NVML
memory tuple for a merge. NVML capacity is not mixed with DRM used
or free memory. Dedicated used explicitly aliases reported used;
shared used remains absent and is not renamed GTT. The public inventory
diagnostic distinguishes partial, empty, and unavailable coverage.
Rebuild **all** consumers for the new 0.x GpuInfo/SystemSnapshot layout.
Verification for this revision: Linux 89/89, Windows 107/107, and rebuilt
installed consumers exited 0. The earlier numbers below are historical,
not additional physical validation.

## Scope of this work

An optional provider is integrated into `Monitor::sample` when
`include_gpus=true`. It reports NVIDIA inventory (UUID, model, and PCI),
driver version, overall utilization, total/used/free framebuffer
memory, and temperature. It does not run `nvidia-smi`, install packages,
change clocks or policies, or elevate privileges.

The library uses `dlopen("libnvidia-ml.so.1", RTLD_NOW|RTLD_LOCAL)` and
explicitly versioned NVML symbols. The driver must provide that library.
CUDA/NVML headers are not required to build PCInsight. Each Monitor
attempts loading on its first GPU sample. Create another Monitor to
retry after installing or changing the driver. The DRM reader continues
to work without NVML. Monitor retains the loaded library until destruction.

Each read balances init and shutdown, including when inventory fails or
an exception is thrown. Queries have no timeout of their own; a blocked
driver can delay sampling. NVML can share initialization references with
other libraries in the process.

## Semantics and limits

- Identity uses UUID, with validated PCI as fallback, never an ephemeral index.
- DRM association requires equivalent, unique PCI addresses in both
  inventories. Four- and eight-digit domains and letter case are normalized.
  Ambiguous matches stay separate rather than overwriting another GPU.
- NVML v1 memory: `used` includes driver reservation and is not app usage.
  The provider requires total=free+used without overflow or sentinel
  values. Total is reported framebuffer capacity, not an independent
  BIOS reading.
- Utilization is time with active kernels within the driver window,
  not a 3D or copy engine or a percentage of allocated bytes.
  No engines are fabricated.
- Per-field errors leave values absent; NO_PERMISSION yields
  `permission_required`. Missing optional symbols do not discard
  inventory. NVML failure does not erase valid DRM data.
- This provider offers no Windows-style shared reservation, driver date,
  or OpenGL context. It does not implement MIG enumeration or GPU
  process telemetry. Products or partitions without supported metrics
  return absence.
- Inventory is capped at 4,096 devices, and strings are bounded and must
  be NUL-terminated. Errors or missing identity can yield partial
  inventory with internal diagnostics. At the original checkpoint, the
  public API had no global GPU diagnostic when it could enumerate no
  devices; see the revision 21 update above.

## Verification and closure criterion

On 2026-10-14, implementation and automated tests for the scope above
were completed. Linux Release passed 9/9 and Windows Release passed 21/21.
Windows tests portable logic and does not load a Linux NVML driver.

Tests cover valid zero, limits, memory overflow and sentinels,
permissions, lost GPUs, missing optional symbols, two GPUs, duplicate
or ambiguous identities, PCI normalization, and balanced init/shutdown.
CTest also loads a synthetic `.so` with the actual ABI symbol names and
tests the full `Monitor` with two GPUs. `LD_LIBRARY_PATH` is changed only
for that test process. The synthetic library is neither installed nor
included in the deliverable.

**Physical validation is pending; the GPU section is not complete.**
The available system has an AMD Radeon RX 7900 XT. Ubuntu WSL exposes
`/dev/dxg`, but `libnvidia-ml.so.1` was not present in the inspected
driver paths and NVML could not be loaded. Native execution verified
NVML absence and fallback, not NVIDIA metrics. Hardware verification
requires Linux with an NVIDIA GPU and an available NVML driver. Run
`pcinsight-nvml-tests` without CTest's synthetic environment, check
real inventory and metrics, then compare two `Monitor` samples.
This evidence does not close work that requires physical validation.

Primary references: [NVML](https://docs.nvidia.com/deploy/nvml-api/latest/index.html),
[queries and semantics](https://docs.nvidia.com/deploy/nvml-api/latest/api/group__nvmlDeviceQueries.html),
and [NVIDIA's published ABI](https://github.com/NVIDIA/go-nvml/blob/main/pkg/nvml/nvml.h).
