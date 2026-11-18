# Windows backend: verified scope

Updated 2026-10-21 (revision 51 MSVC validation). C++20, with no subprocesses
or additional runtime installations. Links the Windows system libraries
`psapi`, `advapi32`, `powrprof`, `pdh`, and `version`, among others.
Tested with MinGW-w64 x64 and MSVC 19.51 x64: 127/127 Release on both,
with separate installed consumers exiting 0; Linux 111/111.
See [MSVC validation](MSVC_VALIDATION.en.md) for fixes, logs, and limits.
ARM64/x86, multi-group machines, and other Windows versions still need
validation. MSVC claims and counts in historical documents describe their
original test dates; the MSVC report contains the newer evidence.

## Available

- Opt-in HKCU/HKLM [Run registry inventory](WINDOWS_STARTUP_REGISTRY.en.md)
  with explicit WOW64 views and raw commands, without executing them.
  Effective status is unknown and impact is not_measured; inventory is not
  exhaustive. Monitor adds [user and system Startup folders](WINDOWS_STARTUP_FOLDERS.en.md)
  without resolving shortcuts. An opt-in reads bounded saved [.lnk metadata](WINDOWS_SHORTCUT.en.md)
  without shell resolution and saved [.url targets](WINDOWS_INTERNET_SHORTCUT.en.md)
  without opening them. The separate [CompanyName label](WINDOWS_STARTUP_PUBLISHER.en.md)
  opt-in is estimated, NOT signature verification.

- [Local WTS sessions](WINDOWS_USERS.en.md) with include_user_sessions=true:
  account, domain, and connection state. [Visible CPU/RAM totals](WINDOWS_USER_RESOURCES.en.md)
  are aggregated by session and account as estimated, even when process rows
  are hidden. Uses system wtsapi32.

- Read-only [Win32 service inventory](WINDOWS_SERVICES.en.md) through SCM
  with `include_services=true`: name, state, valid PID, description, and
  load-order group.

- [Individual processes](WINDOWS_PROCESSES.en.md): PID/PPID, name, threads,
  user, path, architecture, VERSIONINFO description, resident memory, and
  CPU between samples when accessible. Status is active/exited/unknown;
  suspension is not detected at this stage.

- CPU name and manufacturer from the hardware registry (empty when not exposed).
- Physical cores, sockets, logical processors, and L1/L2/L3 totals from
  `GetLogicalProcessorInformationEx` and `GetActiveProcessorCount`.
- Utilization measures scheduler busy time between two samples. The first
  sample has no percentage, and an interval with no counter progress does
  not yield an artificial zero. This differs from the frequency-weighted
  utility shown by some versions of Task Manager.
- On multi-group systems, PDH uses `Processor Information(_Total)` through
  `PdhAddEnglishCounterW`, independently of Windows language. Each call
  result and counter status is validated. Failures, NaN, and values outside
  0–100 are not converted to zero. The query is discarded and recovery
  requires two new samples. Changes in CPU or group count also reset the
  baseline. The calling process's affinity is unchanged.
- Uptime, firmware virtualization indicator, and global process, thread,
  and handle counts. The virtualization indicator does not establish that
  a hypervisor is active.
- OS-visible RAM, available and in use, percentage, commit/limit, and paged
  and nonpaged pools. Pages are converted to bytes; in-use RAM is total minus
  available. [Compressed content](WINDOWS_COMPRESSION.en.md) has an
  experimental optional provider disabled by default. MemoryInfo.reported_total_bytes
  adds explicit availability and checked PSAPI multiplication for the total;
  legacy total_bytes remains. Rebuild 0.x consumers. See [Linux memory](LINUX_MEMORY.en.md)
  for the shared field contract; this work does not revalidate every other
  PSAPI product.
- RAM type (DDR/LPDDR), form factor, configured speed in MT/s, and occupied
  and firmware-represented slots via `GetSystemFirmwareTable(RSMB)`. No WMI,
  external commands, or temporary files. Inventory is read once per
  `Monitor`; recreate it after hardware changes. See [SMBIOS](SMBIOS.en.md)
  for uncertainties and parser validation.

## Explicitly labeled estimates

- `current_ghz`: average of Windows `CurrentMhz` values. It is a reported
  frequency accounting for throttling, not an instantaneous turbo measurement.
- `base_ghz`: SMBIOS Current Speed at boot, as an estimate only when
  available. Max Speed is not substituted, and exact nominal base frequency
  is not claimed.
- `cached_bytes`: PSAPI `SystemCache` (standby plus system working set),
  not an exact reproduction of Task Manager's Cached field.
- `hardware_reserved_bytes`: firmware-reported installed RAM minus
  OS-visible RAM. This can include boot limits as well as hardware reservations.

## Remaining work and availability

Exact nominal base frequency and a stable equivalent to Task Manager's
compressed memory remain pending. An internal interface already provides an
experimental opt-in compressed-content reading marked estimated. Estimated
dynamic frequency remains limited to single-group systems. Multi-group
utilization has a PDH provider but still needs validation on real multi-group
hardware. If the provider is disabled or permissions are missing, the result
is explicitly absent; one group's partial percentage is not reported as total.

[Networking](WINDOWS_NETWORK.en.md) uses IP Helper for interfaces, addresses,
and send/receive rates. Disks have [inventory, model, bus, and capacity](WINDOWS_DISKS.en.md),
[PDH performance](WINDOWS_DISK_PERFORMANCE.en.md), and
[volume, system-disk, and page-file mapping](WINDOWS_VOLUMES.en.md).
The [local WMI media provider](WINDOWS_DISK_MEDIA.en.md) reports HDD/SSD/SCM,
requires provider permissions, and can be disabled with include_disk_media=false.
It does not require installing external utilities.

[DXGI GPU inventory](WINDOWS_GPU_INVENTORY.en.md) covers non-software adapters,
models/IDs, dedicated/shared/dedicated-system capacities, and UMD version
when exposed. [Dynamic dedicated/shared memory](WINDOWS_GPU_MEMORY.en.md) is
implemented per adapter via PDH, along with [engines and an estimated
maximum observed utilization](WINDOWS_GPU_ENGINES.en.md). Temperature and
other GPU fields were pending at this stage; later evidence is listed below.

[Observed Windows CPU history](WINDOWS_APP_HISTORY.en.md) by executable path
is implemented through opt-in include_app_history and the windows_app_history
report with per-field availability. [Separate explicit persistence](WINDOWS_HISTORY_STORE.en.md)
preserves availability: Windows 78/78, Linux 58/58, and installed consumers
exited 0. Network and notifications remain pending, and activity before
capture cannot be reconstructed. A native test verified a positive CPU
reading from its own workload.

Startup inventory covers Run registrations and Startup folders. Saved
[.lnk](WINDOWS_SHORTCUT.en.md) and [.url](WINDOWS_INTERNET_SHORTCUT.en.md)
metadata, [self-declared CompanyName](WINDOWS_STARTUP_PUBLISHER.en.md), and
[local Authenticode signatures](WINDOWS_STARTUP_SIGNATURE.en.md) are opt-in.
[Run Properties](WINDOWS_STARTUP_PROPERTIES.en.md) uses a dedicated read-only
query without expansion or execution. [Startup source-file Properties](WINDOWS_STARTUP_FILE_PROPERTIES.en.md)
adds size, attributes, dates, and saved .lnk/.url metadata from the same handle.

[PCInsight's reversible HKCU Run control](WINDOWS_RUN_CONTROL.en.md) is separate
from Windows approval. The model was tested; successful native validation
remains pending because the transaction resource manager returned 6801.
No destructive or nontransactional fallback. After this partial component,
Windows passed 73/73 and Linux 54/54, with installed consumers exiting 0.
That is not successful native Run control validation.

PCInsight's Run and backup states now have a native, non-mutating reader
without KTM, based on two observations and marked estimated. Transactional
error 6801 was reproduced when reading real HKCU, not assumed to be
fixture-only. Ordinary reads work and do not establish that removal or
restoration is available.

The [HKCU Run/backup listing](WINDOWS_RUN_LISTING.en.md) needs neither KTM
nor caller-retained IDs. It reports case-insensitive name conflicts and
estimated, non-atomic results. A private native test and real HKCU read
were performed: Windows 74/74, Linux 54/54, and installed consumers exited 0.
Native write validation is still pending.

The [Run/backup report](WINDOWS_RUN_MONITOR.en.md) is integrated into
Monitor/Sampler as an opt-in disabled by default. Windows 75/75, Linux 55/55,
and installed consumers exited 0. Native Monitor observed 16 rows;
latest/history and the unsupported Linux result were verified, with no
mutation or generic StartupStatus change. Reporting does not enable KTM
actions.

[Explicit Run control auditing](WINDOWS_RUN_AUDIT.en.md) is integrated
into SystemActions/AuditedActions, separately from generic startup actions.
Windows 76/76, Linux 56/56, and installed consumers exited 0. Verified
privacy, request/result records, sink policies, and invalid dispatch.
Native error 6801 was reconfirmed; auditing does not make writes available.
Signature checks do not establish current revocation, cover catalogs,
or guarantee safe execution. Windows 72/72 and Linux 54/54; installed
consumers exited 0.

Effective shortcut resolution, tasks/packages, enabled/disabled approval,
impact, and actions remain pending. Run registration is not equivalent
to enabled autorun.

User inventory includes WTS and visible CPU/RAM totals by session and
account. Per-process disk/network and session actions remain pending.
Do not use a WTS inventory ID as a persistent control token.

Service start/stop/restart is implemented using caller permissions and
injected tests, without native mutation tests. [Run new task](WINDOWS_LAUNCH.en.md)
is implemented through CreateProcessW without a shell. Forced end task
is implemented with injected tests; see [termination](WINDOWS_TERMINATE.en.md)
for safety and PID identity limits. M07 verifies supported services,
actions, and [service details](WINDOWS_SERVICE_VIEW.en.md). A production
UAC helper remains under L05; full process and session resources remain
under M02/M06. [EcoQoS](WINDOWS_EFFICIENCY.en.md) can be queried and requested
through native APIs, including an identity-checked action. The combined
priority behavior of Task Manager's efficiency mode remains pending.
See [Windows services](WINDOWS_SERVICES.en.md) for service control.

API failures leave optional metrics without a value. `total_bytes` is
still a nonoptional public integer and becomes zero if reading fails.
Also check `available_bytes.support` and its value before displaying RAM.

## Verification

The `windows-integration` test keeps assertions active in Release. It
checks two samples, percentage ranges, monitor independence, RAM total/
available/in-use consistency, topology, commit, pools, and counters.
The full ordinary suite ran: Windows 19/19 and Ubuntu WSL 6/6. The
experimental compression ON variant previously passed 7/7. This does not
replace CI or testing on other machines. `windows-pdh` injects access
denials when opening, adding, sampling, and formatting; it checks invalid
data, recovery, topology changes, and resource cleanup. It also runs
native PDH on the local single-group machine. Real hotplug and other API
errors still need validation. Synthetic SMBIOS tests cover truncation,
extended fields, unknown values, mixed devices, and the RawSMBIOSData
wrapper. Local hardware read: DDR5 DIMM, 5,200 MT/s, two of four slots.

## API references

Revision 54: [free NVIDIA framebuffer memory](WINDOWS_NVML_MEMORY.en.md)
uses optional System32 driver reads and matches CUDA LUID/UUID to NVML
root UUID with KMT count 1. No SDK/CLI, subtraction of PDH from DXGI,
or allocation guarantee. Windows 129/129 on both toolchains, Linux 111/111.
Positive physical NVIDIA results and AMD/Intel free memory remain pending;
M04 is open.

[GPU memory budgets](WINDOWS_GPU_PROCESS_BUDGET.en.md) cover the calling
process and node 0 with independent LOCAL/NON_LOCAL groups. Windows
55/55 and Linux 39/39. Rebuild consumers for new 0.x fields. These budgets
do not replace global free memory or usage and do not change reservations.

[Main GPU sensor](WINDOWS_GPU_TEMPERATURE.en.md) through KMT, exact LUID,
and physical count 1. Physical AMD reading: 43°C. Windows 54/54 and Linux
39/39. No SDK, CLI, or BIOS reading. Linked adapters and 32-bit callers
remain unverified.

[Alternative D3D11 OpenCL matching](WINDOWS_OPENCL_INTEROP.en.md) verified
on physical AMD hardware. Always estimated, not physical LUID identity.
RX 7900 XT: OpenCL 2.0 AMD-APP (3679.0). Windows 53/53 and Linux 39/39.
No D3D device, OpenCL queue/context, or additional SDK.

[OpenCL through a valid DXGI LUID](WINDOWS_GPU_OPENCL.en.md): the local
AMD driver did not advertise cl_khr_device_uuid, so this path had no
value. Estimated D3D11 matching now supplements it, without supplying
a global version or exact identity.

[Vulkan per DXGI GPU](WINDOWS_GPU_VULKAN.en.md) matches a valid deviceLUID,
not a name, and reads the physical apiVersion. Windows 51/51, Linux 35/35,
and a build without optional Vulkan headers were verified.

[D3D12 capability](WINDOWS_GPU_D3D12.en.md) uses the exact DXGI adapter
and populates `direct3d12_feature_level` without creating a device. It
is not the installed DirectX version. Windows 50/50, Linux 35/35.

[GPU driver INF date and PCI location](WINDOWS_GPU_DEVICES.en.md) are
matched by KMT LUID to the same PnP interface. Verified with physical
AMD hardware and failure tests. No BIOS read or name matching. Regression:
Windows 49/49, Linux 35/35.

- [GetSystemTimes and per-group limits](https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-getsystemtimes)
- [Processor topology](https://learn.microsoft.com/en-us/windows/win32/api/sysinfoapi/nf-sysinfoapi-getlogicalprocessorinformationex)
- [PSAPI semantics](https://learn.microsoft.com/en-us/windows/win32/api/psapi/ns-psapi-performance_information)
- [Reported frequency](https://learn.microsoft.com/en-us/windows/win32/power/processor-power-information-str)
- [Counters and processor groups](https://techcommunity.microsoft.com/blog/itopstalkblog/tough-questions-answered-how-to-use-more-than-64-logical-processors-on-a-single-/338402)
- [Validating PDH results](https://learn.microsoft.com/en-us/windows/win32/api/pdh/nf-pdh-pdhgetformattedcountervalue)
