# N03: Linux performance contract for C++ consumers

Revision 160, 2026-11-02. Traces the requested **Performance** panel. This is
not certification of the whole project or closure of N03/G07/N02/B08. All
members below are in `include/pcinsight/types.hpp` unless an additional API
is named. Table prefixes refer to objects in the `SystemSnapshot` returned
by `Monitor::sample` or retained by `Sampler`. The
[public example](../examples/linux_performance.cpp) compiles these accesses
against the SDK without internal headers. It contains no actions or BPF capture.

Mapping to existing tracking IDs, not new subtasks: window and sampling
A01/A02/A03; CPU B01–B08/N02; RAM C01/C02/C03/N02; disks E01–E07; network
F01/F02; GPU G01–G08, with physical validation under G07. R02–R12 in
`SEGUIMIENTO.txt` are the user requirements covered by this matrix.
Delivery and documentation belong to N03. Listing a row does not close
the work on its provider.

## Rules for displaying data without inventing values

- A `Metric<T>` is known only when `metric_is_known` is true. Read support
  and note: available and estimated are distinct. Display known zero and
  false, but show missing, permission_required, and unsupported as unknown,
  never with `value_or(0)`. Do not plot nonfinite floating-point values.
- For structures inside a Metric, check the parent first, then each child.
  The example propagates parent absence even when it contains an object.
- Rates require two continuous observations from the **same** Monitor.
  Warmup, reset, identity change, or lost counters produce gaps, not zeros.
  Do not create a new Monitor every frame to calculate percentages or rates.
- Native units: B and B/s, GHz, MT/s, ms, seconds, Celsius, and a 0–100
  percentage when specified by the provider. Convert using KiB = B/1024,
  MiB = B/1024², GiB = B/1024³, decimal MB = B/1e6, and Mbps = (B/s) × 8/1e6.
  Label the selected unit and do not use these interchangeably.
- Reported static data can change after hotplug, configuration, or driver
  changes. Do not promise an atomic CPU/RAM/GPU/disk/network snapshot.
  Preserve identity and data quality; do not match adapters by position or
  retail name.

## CPU — `snapshot.cpu` prefix

| Requested data | Member / unit | Linux contract and test |
| --- | --- | --- |
| Vendor and model | reported_vendor, reported_model / text | Reported identifiers, not attestation. Prefer these over legacy vendor/model strings without availability. [Identity](LINUX_CPU_IDENTITY.en.md); [test](../tests/linux_cpu_identity.cpp). |
| Dynamic utilization | utilization_percent / % | Validated proc/stat delta; first interval is unknown. Not a cgroup quota. [Contract](LINUX_CPU_USAGE.en.md); [test](../tests/linux_cpu_usage.cpp). |
| Current speed | current_ghz / GHz | Average across online logical CPUs from a complete cpuinfo_cur_freq or scaling_cur_freq source. The latter can represent a requested P-state. Estimated, not an instantaneous application clock. [Frequency](LINUX_CPU_FREQUENCY.en.md); [test](../tests/linux_cpu_frequency.cpp). |
| Base speed | base_ghz / GHz | base_frequency must agree across CPUs and readings. A configuration value, not turbo speed, SMBIOS Current Speed, or direct BIOS verification. Absent if the source is missing or inconsistent. Same frequency contract and test. |
| Sockets, cores, logical processors | sockets, physical_cores, logical_processors / counts | Visible online topology; do not invent offline or hidden cores. Logical processors are hardware contexts, not the count of process threads. [Topology](LINUX_CPU_TOPOLOGY.en.md); [test](../tests/linux_cpu_topology.cpp). |
| Processes and threads | processes, threads / counts | Visible procfs processes and sum of visible Threads. Namespace, hidepid, permissions, and non-atomic reads limit the scope. [Counts](LINUX_COUNTS.en.md); [test](../tests/linux_counts.cpp). |
| Handles | handles / estimated count | Descriptors from visible processes, not Windows handles or unique files. Shared descriptor tables can be counted twice. Same counts contract and test. |
| Uptime | uptime / seconds | /proc/uptime as whole seconds, including suspend time and namespace offset. Not the GUI process lifetime or certification of the physical host. [Contract](LINUX_UPTIME.en.md); [test](../tests/linux_uptime.cpp). |
| Firmware virtualization | virtualization_enabled / bool | **No Linux firmware provider: unsupported.** B08 remains open. Do not infer true or false from /dev/kvm, vmx/svm, or running in a VM. [Contract](LINUX_VIRTUALIZATION.en.md); [test](../tests/linux_virtualization.cpp). |
| Separate KVM information | linux_kvm_api_version / version | Compatible query on a validated descriptor; permissions may be required. Does not replace the firmware flag or create a VM. Same virtualization contract and test. |
| L1/L2/L3 cache | l1_bytes, l2_bytes, l3_bytes / B | sysfs instances reachable by online CPUs, deduplicated by sharing. L1 combines instruction and data caches. Estimated; excludes hidden or offline caches. [Cache](LINUX_CPU_CACHE.en.md); [test](../tests/linux_cpu_cache.cpp). |

## RAM — `snapshot.memory` prefix

| Requested data | Member / unit | Linux contract and test |
| --- | --- | --- |
| RAM amount | reported_total_bytes / B | Usable MemTotal visible to the kernel, not physically installed capacity. Legacy total_bytes=0 does not establish a measurement. [Memory](LINUX_MEMORY.en.md); [test](../tests/linux_memory.cpp). |
| DDR type and form factor | type, form_factor / text | Firmware/SMBIOS when exposed and readable. Do not guess from frequency or CPU. [SMBIOS](SMBIOS.en.md); [test](../tests/smbios.cpp). |
| Speed, used slots, and total slots | speed_mts / MT/s; slots_used, slots_total / counts | SMBIOS inventory within its limits. Do not infer DIMMs that are not reported. Same SMBIOS evidence. |
| Usage and in use | utilization_percent / %; in_use_bytes / B | Estimated fraction (MemTotal − MemAvailable) / MemTotal, and difference in B. Not Windows Working Set or the sum of process RSS. [Memory](LINUX_MEMORY.en.md); [test](../tests/linux_memory.cpp). |
| Available | available_bytes / B | Kernel estimate MemAvailable, not MemFree or a cgroup quota. Same memory evidence. |
| Compressed | compressed_bytes / B | Estimated alias for the Zswap pool, not all Linux compression or original Zswapped/zram data. Do not add it to in_use again. Same memory evidence. |
| Compression details | linux_details.zswap_pool_bytes, zswap_original_bytes; zram_devices[].memory.{original_data_bytes,compressed_data_bytes,allocated_memory_bytes} / B | Separate sources. Do not add them as disjoint partitions. Absence does not mean compression is disabled. [Composition](LINUX_MEMORY_COMPOSITION.en.md); [test](../tests/linux_memory_composition.cpp). |
| Composition | linux_details.composition.{unavailable_estimate_bytes,reclaimable_estimate_bytes,free_bytes} / B | Validated estimated partition that sums to MemTotal. Not Windows modified/standby/in-use/free states. Same composition evidence. |
| Committed and limit | committed_bytes, commit_limit_bytes / B | Committed_AS and CommitLimit, not resident RAM. Overcommit can exceed the limit. Same memory evidence. |
| Cached | cached_bytes / B | Estimated Cached + SReclaimable. Excludes Buffers; do not reuse the Windows meaning. Same memory evidence. |
| Paged/nonpaged pools | paged_pool_bytes, nonpaged_pool_bytes / B | **Unsupported.** No Windows equivalent is implemented; Slab is not renamed to fill the fields. Same memory evidence. |
| Additional Linux categories | linux_details.free_bytes, buffers_bytes, page_cache_bytes, anonymous_bytes, shared_bytes, slab_reclaimable_bytes, slab_unreclaimable_bytes, dirty_bytes, writeback_bytes / B | Overlapping counters. Do not add them together or to composition. Same composition evidence. |
| Hardware reserved | hardware_reserved_bytes / B | Legacy estimated alias for installed_minus_usable_bytes, not an exclusively hardware reservation. Prefer the explicit name. Same composition evidence. |
| Installed RAM and difference | linux_details.firmware_installed_bytes, installed_minus_usable_bytes / B | Firmware-reported installed RAM minus usable RAM, only with consistent inputs. Includes kernel, boot, and offline limits; unknown without a source. Same SMBIOS/composition evidence. |

## Disk — each `snapshot.disks[i]`

| Requested data | Member / unit | Linux contract and test |
| --- | --- | --- |
| Identifier/model | id, model / legacy text | Labels for the observed device, not universal physical identity. Empty strings are not known data. [Identity](LINUX_DISK_IDENTITY.en.md); [test](../tests/linux_disk_identity.cpp). |
| Active time | active_percent / % | Estimated io_ticks delta over elapsed time, capped at 100. Does not establish hardware saturation. [Rates](LINUX_DISK_RATES.en.md); [test](../tests/linux_disk_rates.cpp). |
| Transfer/read/write speed | transfer_bytes_per_second, read_bytes_per_second, write_bytes_per_second / B/s | Deltas of 512-byte sectors, monotonic clock, and checked identity and continuity. Convert to KiB/s in the GUI. Same rate evidence. |
| Average response | average_response_ms / ms | Estimated average for completed reads/writes; no operations means no average. Not end-to-end latency and does not include discard/flush. Same rate evidence. |
| Capacity | capacity_bytes / B | Capacity of the visible block device, not necessarily the manufacturer's raw physical capacity. [Properties](LINUX_DISK_PROPERTIES.en.md); [test](../tests/linux_disk_properties.cpp). |
| Automatic formatted capacity | formatted_capacity_bytes / B | Estimated from mounted, attributable, deduplicated filesystems. **Does not guarantee total formatted capacity**, especially unmounted capacity. [Contract](LINUX_FORMATTED_CAPACITY.en.md); [test](../tests/linux_formatted_capacity.cpp). |
| Explicit unmounted formatted capacity | read_linux_filesystem_capacity / read_linux_disk_filesystem_capacity in linux_filesystem_capacity.hpp / B | Primary ext2/3/4 superblock for selected regions; block devices are opt-in. Does not cover all formats, raw physical capacity, or a universal sum of layers. [Contract](LINUX_FILESYSTEM_CAPACITY.en.md); [test](../tests/linux_filesystem_capacity.cpp). Not run by the example. |
| System disk | system_disk / bool | Based on visible root association and known topology. Can be unknown in unresolved stacks or namespaces. [Contract](LINUX_SYSTEM_DISK.en.md); [test](../tests/linux_system_disk.cpp). |
| Page file | contains_page_file / bool | Attributable active swap, not the existence of every inactive swap file. Absence or inaccessibility is not false. [Contract](LINUX_SWAP_DISKS.en.md); [test](../tests/linux_swap_disks.cpp). |
| HDD/SSD/SATA/SAS/NVMe | reported_media_type and linux_details.{device_protocol,transport,media_type} / text | Media, protocol, and transport are separate, using SAT/SCSI/NVMe sources when available. Do not infer SATA from scsi or SSD from a name. [Identity](LINUX_DISK_IDENTITY.en.md); [test](../tests/linux_disk_identity.cpp). |
| Rotation | rotational / bool | Kernel queue hint, not HDD/SSD or physical medium certification. Same properties evidence. |

## Ethernet/network — each `snapshot.networks[i]`

| Requested data | Member / unit | Linux contract and test |
| --- | --- | --- |
| Adapter name | name, id / text | Observed interface name and identity, not manufacturer. [Identity](LINUX_NETWORK_IDENTITY.en.md); [test](../tests/linux_network_identity.cpp). |
| Vendor/model | linux_details.{vendor,model} / text | PCI/USB and optional local labels, reported or estimated. Do not use modalias or driver as the model. Check the parent first. Same identity evidence. |
| Connection type | linux_details.connection_type / text | ARPHRD, wireless, or virtual evidence, not a name heuristic or proof of Internet access, physical link, or speed. Same identity evidence. |
| Send/receive/throughput | send_bytes_per_second, receive_bytes_per_second / B/s | Estimated interface rates without a fixed advertised ceiling. Not bits/s or traffic attributed causally to applications. [Rate contract](LINUX_NETWORK_RATES.en.md), [test](../tests/linux_network_rates.cpp). Show both; sum them only as a labeled aggregate, not as bandwidth. |
| IPv4 and IPv6 | ipv4, ipv6 / address lists | Do not assume a single address. See [Linux indexed reads and the explicit query with availability](LINUX_NETWORK_IDENTITY.en.md). `query_linux_network_addresses` distinguishes a complete empty dump from absence. The example shows it as a separate query. Legacy Monitor lists have no individual Metric; an empty list does not prove a disabled protocol or a complete query. The native test covers loopback; the synthetic test covers a labeled address. |
| Additional totals | total_sent_bytes, total_received_bytes / legacy B | Do not use these as rates or evidence of availability. They do not replace send/receive Metrics. [Test](../tests/linux_network_rates.cpp). |

## GPU — each `snapshot.gpus[i]` and separate providers

| Requested data | Member / unit | Linux contract and test |
| --- | --- | --- |
| Vendor/model/identity | vendor, model, id; pci_vendor_name, pci_device_name / labels | Provider and optional PCI dictionary. Do not invent a retail name from an ID. Empty legacy strings are unknown. [Names](LINUX_GPU_NAMES.en.md); [test](../tests/linux_gpu_names.cpp). |
| Global utilization | utilization_percent / % | Consistent gpu_busy_percent or NVML source, not utilization for a specific 3D, copy, or compute engine. [Telemetry](LINUX_GPU_TELEMETRY.en.md); [test](../tests/linux_gpu_sysfs.cpp). |
| 3D/copy/compute and all other engines | engines[].name/utilization_percent and linux_drm.aggregates[].engines[] / % | The library does not cap compute engines at two. Do not fabricate engine values from overall utilization. DRM fdinfo covers observed clients; summed_client_active_percent can exceed 100 and **is not global GPU utilization**. [DRM](LINUX_DRM_MONITOR.en.md), [classes](LINUX_DRM_CLASSES.en.md); [test](../tests/linux_drm_monitor.cpp). DRM discovery is opt-in and disabled in the example. |
| GPU memory used | memory_used_bytes, dedicated_memory_used_bytes / B | VRAM/sysfs or NVML framebuffer, depending on the source. These are documented aliases; do not add them. Same telemetry evidence. |
| Dedicated capacity | dedicated_memory_bytes / B | Provider-reported total, not a per-process budget. Same telemetry evidence. |
| Available GPU memory | memory_available_bytes / B | NVML free memory or estimated total minus used. It does not guarantee an allocation of that size. Vulkan heaps and OpenCL max allocation do not substitute for global free memory. Same telemetry evidence. |
| Shared capacity/usage | shared_memory_bytes, shared_memory_used_bytes / B | **These Linux providers supply no equivalent**, so the result is unknown. Do not substitute GTT or total RAM. Same telemetry evidence. |
| Additional GTT metrics | linux_sysfs_details.{gtt_total_bytes,gtt_used_bytes,gtt_headroom_bytes} / B | GTT manager accounting, distinct from Windows shared memory, VRAM, or free RAM. Check parent and child. Same telemetry evidence. |
| Temperature | temperature_celsius / Celsius | Identifiable GPU sensor or NVML. Ambiguous sensors retain their details; the collector does not choose the first one. linux_sysfs_details.temperature_sensors lists them all. Same telemetry evidence. |
| Driver version | driver_version / text | Bound module or NVML version, not the driver or kernel name. Same telemetry evidence. |
| Driver date | driver_date / date | **No reliable source recorded: unknown.** Not mtime, a DRM placeholder, or a date-like version. [Contract](LINUX_GPU_DRIVER_DATE.en.md); [test](../tests/linux_gpu_driver_date.cpp). |
| Vulkan | vulkan_version / text | Reported API capability attributed according to the provider. Not DirectX or evidence of graphics application behavior. [Contract](LINUX_GPU_VULKAN.en.md); [test](../tests/linux_gpu_vulkan.cpp). |
| OpenCL | opencl_version / text | Compatible runtime and available attribution. Does not guarantee executable kernels or global memory. [Contract](LINUX_GPU_OPENCL.en.md); [test](../tests/linux_gpu_opencl.cpp). |
| OpenGL | opengl_version, linux_opengl_details / text/structure | Caller's current context; requires include_current_opengl_context=true. It does not create a context, match by model, or claim a physical maximum. See the [OpenGL context contract](LINUX_CURRENT_OPENGL.en.md) and the [EGL](../tests/linux_current_opengl.cpp) and [GLX](../tests/linux_current_glx.cpp) tests. Disabled in the example. |
| Physical location | physical_location / PCI BDF | Validated native bus, device, and function, not a list index or serial number. Same telemetry evidence. |
| Inventory/quality | snapshot.linux_gpu_inventory_source / scope | Successful empty inventory is distinct from a disabled or inaccessible path. It does not certify every adapter in every namespace or driver. [Test](../tests/linux_gpu_sysfs.cpp). |

## Dynamic 0–60 second window

Use `Sampler(interval, 0, options)`: automatic capacity is
`floor(60s / interval) + 1`. `timed_history()` returns steady-clock
envelopes for samples that began within [now − 60s, now], even when stopped.
Do not persist those points as wall-clock dates, interpolate missing points,
or assume exactly 61 values. `latest()` can be older than the window; show its
age rather than plotting it as just collected. Starting clears history but
does not guarantee all baselines are reset.
[Public contract](../include/pcinsight/sampler.hpp), [test](../tests/sampler_window.cpp).
The GUI plots and scales percentages and B/s; the library does not draw charts.

## Usage and delivery limits

`pcinsight-linux-performance --self-test` verifies rendering without the
OS: unknown versus zero or false, estimated status, permission denied even
with a value, NaN, control character escaping, absent parents, and more than
two engines. `--sample` takes two read-only Linux samples using the same
Monitor. With no arguments it only prints help and does not collect data.
Terminal output includes field, unit, support, value or unknown, and note.
It is neither JSON nor a serializer for untrusted UTF-8. Disk, network,
and GPU indices are local to that output, **not stable GUI identities**.
The example illustrates the main fields and Linux compression. The matrix
also covers optional APIs it does not run: block regions, DRM clients, and
the OpenGL context query.

The [test consumer](../tests/performance_consumer/CMakeLists.txt) builds the
same example using only the installed package. Its CMake configuration
validates the SDK; consumers do not need to configure their GUI with CMake
before designing it. There is no dependency installation, service control,
privilege elevation, eBPF capture, or BIOS change. The SDK does not promise
data the kernel, firmware, or driver does not expose. Cross-platform
physical validation G07/N02 and firmware virtualization B08 remain open.
This work covers Performance only. It does not close traceability for
processes, history, startup, users, services, or actions, or final N03 acceptance.

## Revision 160 evidence

Linux Release passed 190/190. Both example tests were repeated ten times
(20 runs), and ten pure-contract MSVC runs passed. The external consumer of
the installed SDK was built and relinked; self-test and `--sample` exited 0.
The final build had no code warnings, and the matrix's local links were
checked. Logs: `work/performance-contract-*`. WSL observations included known
CPU and memory utilization and unavailable current and base frequencies with
explicit notes. No maximums or zeros were substituted. This is not evidence
for another distro, a physical GPU, or remote CI. The production library and
its layouts are unchanged; only the example, tests, and consumer documentation
were changed. At this checkpoint N03 remained E; C60/E10/P3/B2, with 3X
excluded, gave 60/75 (80%) complete and 15/75 (20%) open.
