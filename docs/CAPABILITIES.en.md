# Capability matrix and roadmap

This roadmap contains historical milestones and proposed work. See the
[support matrix](SUPPORT_STATUS.en.md) for the current documented scope.

| Area | Linux | Windows | Strategy |
|---|---|---|---|
| CPU, uptime, processes, threads | Partial in 0.1 | Initial implementation; see [Windows backend](WINDOWS.en.md) | `/proc`, `/sys`; Windows GetSystemTimes, topology, and PSAPI |
| Dynamic RAM metrics | Partial in 0.1 | Usage, commit, and pools implemented; compression pending | `/proc/meminfo`; Windows GetPerformanceInfo |
| DDR, slots, speed, form factor | Built-in SMBIOS reader | Implemented subject to firmware | Binary DMI table in sysfs; GetSystemFirmwareTable on Windows |
| Physical disks and volumes | Dynamic in 0.2 | Inventory, PDH, volumes, and reported medium; WMI validation with permissions pending | sysfs, diskstats, and statvfs; Windows SetupAPI, Storage IOCTL, PDH, PSAPI, and WMI |
| Ethernet and addresses | Partial in 0.1 | IP Helper implemented; see [Windows network interfaces](WINDOWS_NETWORK.en.md) | sysfs, getifaddrs, and netlink; GetIfTable2 and unicast IP |
| Generic GPU | Partial DRM/hwmon in 0.2 | Pending | DRM/hwmon and optional driver backends; DXGI, PDH, and vendor APIs |
| Process CPU/RAM | Partial in 0.1 | Initial implementation; see [Windows processes](WINDOWS_PROCESSES.en.md) | Tick deltas and RSS; Toolhelp/Win32 |
| Process disk | Implemented in 0.2 | ETW decoder and private-thread test; general provider pending | `/proc/PID/io`; [ETW disk events](WINDOWS_DISK_EVENTS.en.md) rather than generic counters as proxies |
| Process network | Optional F03 source implemented: explicit selection or visible admission, standalone/Monitor/Sampler. Strict mode validated on a real view with 30/30 processes; not a universal total. | Privileged; no new Windows provider | LINUX_NETWORK_ACCEPTANCE.en.md; instrumented TCP/UDP payload, nominal windows, typed coverage and permissions, not on-wire or total asynchronous traffic. H04/J04 are separate. |
| Shared network producer | Public in observed mode: standalone, Monitor, Sampler, and selection owner. Tested with 65 real private processes in two batches. Structural ceiling of 1,024 actors. Updates preserve origin for retained selections; readmission starts fresh. | Not wired up; shared fixtures | [scaling limits](LINUX_NETWORK_SCALE.en.md); initially N + 6 + 8 × ceil(N/64) FDs. Three probes per batch initially, not a fixed count after churn; no global capture. |
| Network selection preparation | Read-only PID/start owner acquires, retains, and releases pidfds. Structural maximum of 1,024 with explicit rejection. No automatic enumeration or capture. | procfs/pidfd, system permissions | LinuxProcessNetworkSelectionOwner; [scaling limits](LINUX_NETWORK_SCALE.en.md); fixtures at 1,024/16,384 and 65 native private processes. No universal guarantee. |
| Inventory for network selection | Visible /proc, full non-atomic sweep, per-PID states and reasons without hiding unknowns. Strict bridge capped at 1,024, with no capture. | Read-only procfs/pidfd, current view and permissions | read_linux_network_process_inventory; [inventory](LINUX_NETWORK_INVENTORY.en.md) and [scaling limits](LINUX_NETWORK_SCALE.en.md); not a host-wide census. |
| Visible network admission | Standalone/Monitor/Sampler. Metadata and network values paired per snapshot and history entry, explicit refresh, and strict default. An unchanged set preserves rates. | Unavailable; lifecycle fixtures | [Monitor/Sampler admission](LINUX_NETWORK_ADMISSION_MONITOR.en.md); native tests used only two private processes and a restricted enumerator, not global capture. Expanded 0.x SystemSnapshot requires rebuilding. |
| Partial admission policy | Opt-in verified_only, with PID/state/reason exclusions alongside the sample. Pure planning, strict default, and rejection above 1,024 eligible processes. | No provider; policy and controller fixtures | [Admission policy](LINUX_NETWORK_ADMISSION_POLICY.en.md) and [scaling limits](LINUX_NETWORK_SCALE.en.md); verified procfs does not grant BPF permission. Runtime failure closes the whole capture. |
| TCP sockets/UID | Opt-in Sampler SOCK_DIAG implemented for ESTABLISHED and CLOSE_WAIT; no PID attribution or total network traffic | API absence documented | LINUX_TCP.md; no CLI, eBPF, or elevation; subject to kernel support and policy. |
| Services | systemd inventory, state, and confirmed control verified. Partial file-based SysV/OpenRC support. | SCM inventory and start/stop/restart; real isolated validation pending | systemd D-Bus and configuration readers; Service Control Manager |
| Automatic startup | XDG implemented | Pending | XDG autostart; Registry, Startup, and Task Scheduler |
| Users and sessions | utmpx implemented | Pending | utmpx and planned logind; WTS APIs |
| App history | Observed CPU in memory. LinuxHistoryStore persists rows, availability, and notes. F03 network integration into H04 pending. | Partial custom/OS record | [Linux history store](LINUX_HISTORY_STORE.en.md); cumulative merge without double counting, format separate from legacy and Windows, no automatic writes. |
| Actions | Processes, XDG, systemd, and sessions | Run new task, forced End task, EcoQoS, and SCM control; remaining actions and UAC pending | Separate implementations, authorization, and auditing |

## Recommended phases

### 0.2 — Complete Linux telemetry

- Deltas from `/proc/diskstats` and `/proc/PID/io`.
- Volumes, formatted capacity, system disk, and swap/page file.
- Netlink for network and route metadata.
- Extend the built-in SMBIOS reader for more inventory without running commands.
- Enumerate systemd services, logind sessions, and XDG/systemd startup entries.

### 0.3 — GPU plugins

- Common DRM backend.
- NVIDIA NVML, AMD ROCm/sysfs, and Intel Level Zero.
- Enumerate all available engines; the GUI decides which two `compute` engines to display.
- Query Vulkan, OpenGL, and OpenCL through optional loaders.

### 0.4 — Actions and security

- `terminate_process`: SIGTERM first; SIGKILL only when the caller requests it.
- Use D-Bus/polkit for services; never construct shell commands from user-supplied names.
- Enable or disable startup entries with recoverable transactions.
- Session disconnection through logind was proposed in this roadmap.
- Audit logging and structured errors.

Action auditing was implemented and verified with `AuditedActions`: requested
and completed events, preservation of checked identity, and a fail-closed sink
policy. See [AUDIT.md](AUDIT.en.md). Auditing does not replace authorization or a
privileged helper.

### 0.5 — Windows backend

- Processes, services, sessions, startup, and performance through native APIs.
- ETW for per-process disk and network metrics; PDH for performance counters.
- DXGI and GPU counters; WMI/CIM only for low-frequency inventory.
- Efficiency Mode through EcoQoS and priority APIs where the Windows version permits.

## API design remaining before 1.0 stabilization

- Advanced implementations and filters for `ServiceInfo`, `StartupEntry`,
  `UserSession`, and the `AppHistorySample` view. The old `AppHistoryEntry`
  is a legacy payload, not a measurement model with availability; see the
  [availability contract](METRIC_AVAILABILITY.en.md).
- An asynchronous observer with `std::stop_token`, configurable intervals,
  and backpressure.
- Field selection so a CPU view does not pay to enumerate everything.
- Stable IDs, monotonic timestamps, explicit SI/IEC units, and ABI versioning.
- Errors with `std::error_code` and queryable backend capabilities.

## Security

Read-only functionality should work without privileges where possible.
Actions should live in a minimal helper process authenticated by polkit/UAC,
with a closed list of operations. Run new task belongs in the GUI or an
explicit launch module; it must not accept an unvalidated shell string.

## Linux policy: no external applications

The core neither starts processes nor parses text produced by utilities.
The preferred source order is:

1. Kernel ABI: procfs, sysfs, netlink, sockets, and syscalls.
2. Firmware exposed by the kernel: SMBIOS/DMI, ACPI, and device tree.
3. Driver interfaces: DRM, hwmon, and PCI attributes.
4. An existing driver library, optionally loaded at runtime, only for metrics
   that the generic ABI cannot provide.
5. A privileged helper shipped with PCInsight for protected reads; never
   require the user to install a third-party utility.

There is no universal service interface across distributions. systemd,
OpenRC, runit, and s6 have different models, so this area needs native
adapters per init system. Likewise, firmware does not contain 3D, copy, or
compute utilization. Live values exist only when the GPU driver exposes
them. The API should report absence accurately rather than invent numbers.
