# PCInsight support status

**Language:** [Español](SUPPORT_STATUS.md) · English

PCInsight 0.2.0 is a development SDK available for testing and integration.
It does not implement every Task Manager feature or guarantee every metric
on every distribution. This page distinguishes **implemented features**,
**validation evidence**, and **missing features**. Progress on an application
that uses the SDK does not indicate the status of this API.

## What you can use on Linux

| Area | Available implementation | Limitations to surface in your application |
| --- | --- | --- |
| CPU | Identity, utilization between samples, frequencies, topology, caches, counts, and uptime where a source is available | Base and current frequency are different. KVM does not prove the firmware setting. Topology or frequency may be estimated or unknown. |
| RAM | Visible totals, used and available memory, commit, caches, kernel details, and SMBIOS inventory | DDR type, slots, and speed depend on firmware and access. zram/zswap are not equivalent to Windows-wide compression. Installed minus visible RAM is not an exact measure of hardware reservation. |
| Disk | Inventory, provider-reported rates, activity and latency, capacity, mounts, and root/swap associations | Mounted capacity does not establish total formatted capacity. Do not infer SSD status from the name or arbitrarily distribute usage across RAID/LVM layers. |
| Network | Adapters, rates, addresses by index, and an explicit IP query with availability status | Commercial make and model may be unavailable. The IP query and snapshot are not atomic; an index can be reused. An empty legacy list does not prove that a protocol is disabled. |
| GPU | Inventory and telemetry exposed through DRM/sysfs/hwmon and available API providers | Full global telemetry is not guaranteed for AMD, Intel, or NVIDIA. Engines, temperature, memory, or versions may be absent. GTT is not automatically equivalent to Windows shared GPU memory. |
| Processes and details | PID, identity, accessible user/state/architecture/executable fields, and CPU, RAM, and I/O where supported | Snapshots are not atomic. Processes may disappear or become inaccessible. I/O and RSS are not disk space or private RAM. Per-process network data requires explicit capture and permissions. |
| App history | Observation and storage according to the CPU, network, or notification source | It does not reconstruct earlier history. Sources have different periods and coverage; do not combine them as a universal record. Installing the SDK does not start observers. |
| Startup | XDG inventory, eligibility, properties, personal-entry changes, and observed impact measurements or overlays | Coverage is not universal across desktops or startup mechanisms. A name or PID does not prove why an app started. Publisher is declared metadata, not a verified signature. |
| Users | Available sessions and visible resource totals with a declared scope | There is no universal per-session attribution or sum of unique physical memory. Disconnect and logout are distinct. |
| Services | systemd and separate runit, s6, OpenRC, and SysV adapters | Inventory files and markers do not establish live status. There is no universal init detection. Selecting a provider does not authorize control. |

For fields, units, functions, and tests, see the contracts for
[performance](LINUX_PERFORMANCE_REQUIREMENTS.md),
[processes](LINUX_PROCESS_REQUIREMENTS.md),
[app history](LINUX_APP_HISTORY_REQUIREMENTS.md),
[startup](LINUX_STARTUP_REQUIREMENTS.md), [users](LINUX_USERS_REQUIREMENTS.md),
and [services](LINUX_SERVICES_REQUIREMENTS.md) (Spanish).

## Missing features, beyond hardware validation

| Feature | What is missing or not offered |
| --- | --- |
| BIOS/UEFI virtualization setting | No implemented and validated OEM provider reports the actual setting. CPU flags, SMBIOS, and KVM access are not substitutes. |
| SysV start, stop, and restart | A read-only catalog is available. Script-based control is not implemented, and there is no fallback that runs the script. |
| Complete automatic startup attribution | Observation does not establish that the OS launched every application or capture every login launch. PCInsight remains an observer rather than replacing the desktop launcher. |
| Operational privileged helper | Internal planning and authorization components have private tests, but there is no integrated deployment. The library neither installs a service nor elevates privileges automatically. |
| Safe generic Linux efficiency mode | The generic action is disabled. The advanced unchecked path requires explicit risk acceptance and must not be used as an automatic fallback. |

Windows paged/nonpaged pools and DirectX are not presented as Linux metrics.
PCInsight does not invent a driver date or GPU reading that the source does
not expose. A field in a C++ type does not establish platform equivalence.

## Actions and permissions

Reading data does not perform actions. Ending a process, modifying autostart,
logging out, and controlling services require a separate request and the
appropriate permissions. Use verified identities where available; a visible
PID or name is not authorization. Do not automatically substitute another
operation when a request is denied.

OpenRC control is opt-in. It uses the OpenRC runtime and scripts only for
explicitly requested control operations, not for collection. It is disabled
by default. This does not imply equivalent SysV control. Running an action
with the caller's current permissions is not the same as providing a helper
that can elevate privileges.

## Tested and untested

- Local baseline: Ubuntu WSL x86-64, GCC 13.3 and Clang 18.1.3 with libstdc++. Revision 230 passed 201 GCC and 198 Clang tests, with the optional recorder enabled only for GCC, and 13 delivery tests per toolchain. Later revisions added targeted changes and tests; the same full-suite results do not automatically apply to them.
- Recent IP-query and permissions work has synthetic tests, native loopback tests, external SDK consumer tests, and packaging checks. Simulated cases do not validate physical hardware.
- Physical coverage is incomplete for CPU/RAM/firmware, disks and storage topologies, adapters, and AMD/Intel/NVIDIA GPUs. Real-world acceptance of alternative service supervisors is also incomplete. [How to help](LINUX_HARDWARE_EVIDENCE.en.md).
- GCC and Clang workflows are included. Local verification does not establish that CI passed on GitHub; check the repository's Actions results.
- The 1.0 ABI and final audit are not complete. C++20 and an exact package version do not guarantee compatibility across all compiler, runtime, and revision combinations. Rebuild consumers when changing SDK revisions.
- Historical intermittent CPU and history failures did not reproduce in recent reruns. Passing tests do not establish that the underlying cause was fixed.

The verifiable scope can be published without waiting for physical hardware.
Keep it clear which features were tested, simulated, or not yet validated.
The source tree contains partial Windows work, but Windows is outside the
current focus. There is no claim of parity with Linux or Windows Task Manager.
