# Direct3D 12 feature level by GPU

`GpuInfo.direct3d12_feature_level` exposes the highest validated level
among the known candidates `12_2`, `12_1`, `12_0`, `11_1`, and `11_0`.
It is not the installed DirectX version, DLL version, driver version,
Ultimate certification, or a list of all optional capabilities. It is not
inferred from the model, Windows version, or presence of a file.

## Provider

The DXGI inventory passes the same `IDXGIAdapter1` to `D3D12CreateDevice`
with `ppDevice=nullptr`: this validates support without obtaining a device
or creating queues, windows, commands, or a graphics workload. There is
no default adapter or WARP fallback. The function is resolved from
`d3d12.dll` loaded from System32, without external tools, installation,
or a new link dependency. The DLL is released through RAII and no pointers
are retained on exit. The debug layer is not enabled and consumer settings
are not changed. The runtime can initialize its driver internally; there
is no promise of zero overhead or a driver deadline.

Up to five candidates are checked in descending order:

- `S_FALSE` is the documented success result for a query with null output.
  The level is `available` if every higher candidate returned
  `DXGI_ERROR_UNSUPPORTED`. The scope ends at 12_2 and excludes future levels.
- `E_INVALIDARG` can mean an older runtime does not recognize the level.
  If a lower level validates, it is returned as `estimated`: a confirmed
  lower bound, not the hardware's absolute maximum.
- Access denied: no value, `permission_required`. Device removal, other
  errors, or an unexpected `S_OK`: no value, `unsupported`. These errors
  are not hidden by a lower result. A missing DLL or entry point also
  yields no version.

Each sample queries the adapters currently present. Optional failures do
not remove the rest of the inventory. `include_gpus=false` does not start
this provider. The field stays absent on Linux and does not replace the
Vulkan or OpenCL providers. Public fields in 0.x require rebuilding the
library and consumers together.

## Verification

2026-10-14 local time, America/La_Paz: Release test `windows-gpu-d3d12`
covers each level, lack of support, access denied, device removal, generic
errors, invalid success, a runtime that rejects 12_2 and validates 12_1,
null API or adapter, and the exact adapter pointer and null output on
every call. Injected inventory tests two GPUs with different results and
excludes software adapters. Native queries and Monitor off/on/off were
tested. The RX 7900 XT validated `12_2`.

Regression: Windows 50/50, Linux 35/35. Packages were installed and
external consumers rebuilt and run on both. No physical validation of an
old runtime, Intel/NVIDIA hardware, RDP, or multi-GPU systems. Windows
Vulkan/OpenCL/OpenGL support was still pending; this component did not
complete the entire GPU subtask.

Source: [D3D12CreateDevice: explicit adapter, query without a device, and S_FALSE](https://learn.microsoft.com/en-us/windows/win32/api/d3d12/nf-d3d12-d3d12createdevice).
