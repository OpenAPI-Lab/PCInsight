# PCInsight

**Language:** [Español](README.md) · English

PCInsight is a standalone C++20 SDK for building system monitors and task
managers, with Linux as its primary platform. This repository contains the
library, examples, and tests. It does not include the GUI or the larger project
that uses the SDK.

**Status: 0.2.0 in development.** The Linux SDK is ready to try and integrate,
but it does not yet cover every feature or guarantee support on every
distribution. Available metrics depend on kernel, firmware, drivers, and
permissions. A missing value is not zero. The SDK does not yet promise a
stable 1.0 ABI. Windows support is partial and is not the focus of this release.
On other platforms the backend returns a snapshot marked as not implemented.
The documented validation is local; the presence of CI workflows is not
evidence that they have passed on GitHub.

## Get started

- [Use PCInsight from C++, without adding CMake to your application](docs/GETTING_STARTED_CPP.en.md).
- [See what works, what is partial, and what is not implemented](docs/SUPPORT_STATUS.en.md).
- [Verify Linux hardware and share your results](docs/LINUX_HARDWARE_EVIDENCE.en.md).
- [Contribute code, tests, documentation, or hardware reports](CONTRIBUTING.en.md).
- [Report a security issue privately](SECURITY.en.md).
- [Maintain and publish the SDK separately from the GUI](docs/INDEPENDENT_REPOSITORY.en.md).

## Build and try it on Linux

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
ctest --test-dir build --output-on-failure
./build/pcinsight-cli
```

CMake builds the SDK itself. Your application can link the resulting
`libpcinsight.a` directly. See the [complete C++ example](examples/getting_started.cpp)
for a runnable program.

```cpp
#include <pcinsight/monitor.hpp>
#include <chrono>
#include <thread>

pcinsight::Monitor monitor;
monitor.sample(); // Establish a baseline for rate calculations.
std::this_thread::sleep_for(std::chrono::seconds(1));
auto snapshot = monitor.sample();

if (pcinsight::metric_is_known(snapshot.cpu.utilization_percent)) {
    double cpu_percent = *snapshot.cpu.utilization_percent.value;
}
```

The API separates static inventory (models, vendors, capacity, caches, and
topology), dynamic snapshots (utilization, rates, and counters), recent
sampling history, and privileged actions. Accumulated app history is a
separate report with its own availability and coverage.

For a GUI, use `pcinsight::Sampler` to collect samples in the background.
The default interval is one second. History retains samples started in the
past 60 seconds; `timed_history()` exposes monotonic start and end timestamps.
The automatic capacity (`0`) allows up to 61 points at one second or 601
at 100 ms. Explicit capacity can shorten the window. Interval and metric
groups are configurable, and slow collection can affect cadence. Draw the
graph using timestamps, not a fixed count of 60 rows. Missing data is a gap,
not zero. CPU utilization and rates need deltas between samples; an isolated
reading cannot produce them. See the [Sampler contract](docs/SAMPLER_WINDOW.en.md).

`Metric<T>` exposes `value`, `support`, and `note`. Check
`metric_is_known(m)` to distinguish a measured zero from a missing value.
For app history, use `app_history_view(snapshot)` and `AppHistorySample` from
`pcinsight/app_history.hpp`. The old `AppHistoryEntry`, also called
`LegacyAppHistoryEntry`, lacks availability metadata; do not treat its
numbers as known measurements. See the [metric availability contract](docs/METRIC_AVAILABILITY.en.md).

## Linux coverage

PCInsight exposes CPU, memory, process, disk, network, GPU, session, startup,
service, and observed application history data where supported. Collection
uses system interfaces; it does not need to run tools such as `dmidecode`,
`lspci`, `systemctl`, or `nvidia-smi`. Some optional providers depend on
interfaces or runtime libraries already available on the host.

Read-only sampling is separate from actions such as ending a process,
changing startup entries, logging out, or controlling a service. Those actions
require explicit requests and appropriate permissions. Installing the SDK
does not start network or notification capture or reconstruct past activity.
See the [support matrix](docs/SUPPORT_STATUS.en.md) before wiring actions to a GUI.

Part of the validation used WSL or simulated data. Those tests do not establish
support on physical hardware. A report for a single device or metric is useful;
see the [hardware verification guide](docs/LINUX_HARDWARE_EVIDENCE.en.md).

- **CPU:** Model, vendor, utilization, current and base frequency when exposed,
  topology, caches, uptime, and visible process, thread, and descriptor counts.
  Logical processors are hardware threads, distinct from software threads.
  Base frequency comes from `base_frequency` or readable SMBIOS, not from
  current frequency.
- **Memory:** Total, used, available, commit, cache, zswap, and physical SMBIOS
  data. The SDK includes a binary SMBIOS reader for RAM and firmware frequency.
  SMBIOS is inventory, not live telemetry; it can be absent on ARM, VMs, or
  systems that do not expose it. `/sys/firmware/dmi/tables/DMI` may require root.
- **Processes:** PID/PPID, user, state, CPU, RSS, I/O, thread count, executable,
  and ELF architecture. Per-process traffic is not reliably available from
  `/proc`; it requires a privileged facility such as eBPF/cgroups or nftables.
- **Disks:** Type and bus, physical and mounted capacity, root disk, swap,
  activity, read and write rates, and average latency.
- **Network:** Interfaces, addresses, total bytes, and throughput per adapter.
- **GPU:** PCI/DRM inventory, location, driver, and utilization, VRAM, and
  temperature when the driver exposes them. Windows handles, paged/nonpaged
  pools, compressed-memory reporting, and DirectX are platform-specific;
  Linux exposes descriptors, kernel/zswap metrics, and Vulkan/OpenGL/OpenCL.
- **Users:** logind/utmp sessions and visible resource totals by user, with
  explicit scope limits.
- **Startup:** System and user XDG entries, with transactional writes to
  enable or disable supported entries.
- **Services:** systemd inventory and a partial file-based fallback.
  The [explicit SysV query](docs/LINUX_SYSV_SERVICES.en.md) exposes declared LSB
  descriptions in `ServiceInfo` without inferring PID, liveness, or enablement.
  systemd start/stop/restart uses D-Bus and loads `libsystemd` if the system
  provides it. The SysV query does not add script control.
- **Actions:** Launch without a shell, terminate with verified identity/pidfd,
  and explicitly log out through logind. The [Linux CPU/I/O preset](docs/LINUX_EFFICIENCY.en.md)
  is an advanced unchecked flow with explicit PID/TID risk acknowledgement
  and a receipt; the generic action is disabled with no automatic fallback.
  Disconnect fails closed and is not logout.
- **History:** CPU accumulates by executable after `Monitor` is created.
  [LinuxHistoryStore](docs/LINUX_HISTORY_STORE.en.md) persists Linux rows with
  availability and notes through explicit save and retention. `HistoryStore`
  remains legacy only. F03 network data does not populate the legacy H04
  report; see the [history reference](docs/HISTORY.en.md).

Distribution independence means relying on system interfaces rather than a
package manager or installed command-line tools. Some features still require
specific kernel drivers and mounted procfs/sysfs interfaces.

## Technical references

- Performance: [fields and units](docs/LINUX_PERFORMANCE_REQUIREMENTS.en.md) · [C++ example](examples/linux_performance.cpp)
- Processes and details: [fields, identity, and actions](docs/LINUX_PROCESS_REQUIREMENTS.en.md) · [C++ example](examples/linux_processes.cpp)
- App history: [CPU, network, and notifications](docs/LINUX_APP_HISTORY_REQUIREMENTS.en.md) · [C++ example](examples/linux_app_history.cpp)
- Startup: [fields and actions](docs/LINUX_STARTUP_REQUIREMENTS.en.md) · [C++ example](examples/linux_startup.cpp)
- Users and sessions: [scope and actions](docs/LINUX_USERS_REQUIREMENTS.en.md) · [C++ example](examples/linux_users.cpp)
- Services: [providers, fields, and actions](docs/LINUX_SERVICES_REQUIREMENTS.en.md) · [C++ example](examples/linux_services.cpp)
- SDK: [Linux delivery guide](docs/LINUX_02_HANDOFF.en.md) · [header/library compatibility check](docs/SDK_HEADER_COMPATIBILITY.en.md) · [version policy](docs/SDK_VERSION_POLICY.en.md)

The [Linux delivery audit](docs/LINUX_DELIVERY_AUDIT.en.md) maps requirements
R01–R20 to consumer guides, tests, and SDK limits. The optional header/library
link check does not guarantee a 1.0 ABI or require CMake in the GUI.
See also the [capability roadmap](docs/CAPABILITIES.en.md) and
[implementation status](docs/STATUS.en.md).

The services example does not control services. The users and startup
examples are read-only. The app-history example does not enable network or
notification capture or write history files. The processes example performs
no actions or capture; `--sample` inventories visible processes but displays
only SELF. The performance example uses only the public SDK, distinguishes
unknown from zero, and does not require a GUI build. Its `--self-test` does
not query the OS; `--sample` takes two Linux observations without actions or
eBPF capture. With no arguments it prints help.

### Linux startup, sessions, and other optional providers

- The [startup recorder](docs/STARTUP_RECORDER.en.md) provides
  `LinuxStartupRecorder` for a host that runs before the GUI. It requires
  consent, explicit timing and shutdown, and an explicit request to save
  history. The standalone executable is build-time opt-in, with no
  installation or activation rules. Markers do not prove autostart, and
  short-lived apps are not universally captured. I07 remains open.
  The [startup capture example](docs/STARTUP_CAPTURE.en.md) combines candidates,
  sampling, and history saves without installing or enabling autostart.
  It neither proves automatic launch nor replaces early host integration.
- [Linux disconnect capability](docs/LINUX_DISCONNECT.en.md) is a read-only
  provider query. utmp/login1 do not offer a detach operation that preserves
  applications. Show it as unavailable; logout is separate and can terminate
  processes. This does not rule out support on external remote servers.
- [Manage user accounts](docs/ACCOUNT_MANAGEMENT.en.md) documents explicit GUI
  integration. The SDK is not an account manager and does not open settings
  automatically.
- The [declared startup publisher](docs/LINUX_STARTUP_PUBLISHER.en.md) can be
  queried without external commands using opt-in AppStream metadata and the
  system XML library if present. It installs nothing and does not verify signers.
- [Linux notification capture](docs/LINUX_NOTIFICATIONS.en.md) requires explicit
  consent. It counts requests accepted by the service, not displayed
  notifications or past activity, and retains no titles or content.
- For selected ext2/3/4 regions, [declared capacity](docs/LINUX_FILESYSTEM_CAPACITY.en.md)
  is read-only, does not mount the region or run external commands, and
  requires opt-in for block devices. It is not usable or free space, nor a
  universal physical aggregate.

### Linux network accounting

- [Per-user network data](docs/LINUX_USER_NETWORK.en.md) is available in
  `snapshot.linux_user_network`: one row per UID in the observer's scope,
  with domain, coverage, and partial send, receive, and total rates. It does
  not duplicate sessions or match users by name. `include_linux_user_network`
  does not require session inventory. F03 capture still needs prior explicit
  authorization.
- [Observed network history by executable](docs/LINUX_NETWORK_APP_HISTORY.en.md)
  integrates F03 with `Monitor` and `Sampler` in
  `snapshot.linux_network_app_history`. It requires authorized observed
  capture and both history and network options. It reports estimated
  accumulated bytes with explicit identity, windows, and gaps. CPU and
  network have separate periods and files. `LinuxHistoryStore` persists
  availability without duplicating snapshots. The 0.x layout change
  requires rebuilding the library and all consumers.
- The [public selected-process network API](docs/LINUX_PROCESS_NETWORK.en.md)
  uses `<pcinsight/linux_process_network.hpp>` and
  `pcinsight::LinuxProcessNetworkSampler`. This opt-in API uses pidfds,
  identities, and declared or [opt-in observed cookies](docs/LINUX_PROCESS_NETWORK_OBSERVED.en.md),
  nominal windows, estimated rates, and partial UID totals. It does not
  start BPF automatically from `Monitor` or report all host, process, or
  user traffic.
- [Monitor/Sampler integration](docs/LINUX_PROCESS_NETWORK_MONITOR.en.md) requires
  authorized setup separately from `include_linux_process_network`.
  Disabling capture or stopping the worker closes it; restarting does not
  reauthorize it. Rebuild the library and consumers after 0.x layout changes.
- [Selection updates](docs/LINUX_PROCESS_NETWORK_UPDATE.en.md) add or remove
  processes with renewed consent, preserve retained captures, and reset all
  rates without automatic discovery. Updates can run with the
  [background sampler active](docs/LINUX_PROCESS_NETWORK_WORKER.en.md), serialized
  with collection and stop. History preserves epochs; do not combine their rates.
- [Declared namespace buckets](docs/LINUX_PROCESS_NETWORK_BUCKETS.en.md) allow
  multiple cookies per process without counting traffic twice. Select this mode
  [explicitly when starting Monitor/Sampler](docs/LINUX_PROCESS_NETWORK_BUCKET_MONITOR.en.md).
  Existing calls remain strict, and the declared mode does not discover namespaces.
- The [observed-cookie engine](docs/LINUX_NETWORK_DISCOVERY.en.md) counts buckets
  without manual cookies. [Selected windows](docs/LINUX_NETWORK_DISCOVERED_WINDOWS.en.md)
  verify pidfd/procfs identities and calculate per-cookie rates.
  [Multi-process collection](docs/LINUX_NETWORK_OBSERVED_MULTI.en.md) adds a
  common nominal window, empty records, and group reset. It is wired to the
  public provider and [observed Monitor/Sampler mode](docs/LINUX_PROCESS_NETWORK_OBSERVED_MONITOR.en.md),
  not general process discovery. The revision 83 layout change requires a
  full rebuild when upgrading from earlier headers.
- [PID/start-time selection management](docs/LINUX_NETWORK_SELECTION_OWNER.en.md)
  manages pidfds without manual opens, rejects excess capacity, and never
  enumerates processes or starts capture automatically. The structural limit
  is 1,024 monitored processes. The [gated reusable pidfd query](docs/LINUX_NETWORK_QUERY_SESSION.en.md)
  passed native test 17, including rate, UID, and cleanup checks.
  [Larger selections](docs/LINUX_NETWORK_SCALE.en.md) retain raw batches of up
  to 64 and up to 16 cookies per process without truncation or automatic session
  capture. [Earlier cost failures](docs/LINUX_NETWORK_CAPACITY64.en.md) are
  historical evidence, not current results.
- The [public shared backend](docs/LINUX_NETWORK_SHARED_PUBLIC.en.md) serves
  observed namespaces in `LinuxProcessNetworkSampler`, `Monitor`, and
  `Sampler`. Tests covered 65 private processes in two batches, owned pidfds,
  endpoint joins, four rates, and partial UID totals.
  [Authenticated updates](docs/LINUX_NETWORK_SHARED_UPDATES.en.md) preserve
  retained capture and use fresh capture for new or readmitted processes
  without counting gaps. Each successful update advances the epoch and
  resets rates. It uses bounded disjoint batches, not a fixed three probes
  after every update. There is no general capture or automatic admission;
  strict defaults and declared-cookie behavior are unchanged.
- The [visible process inventory](docs/LINUX_NETWORK_INVENTORY.en.md) preserves
  verified identities and per-row reasons. Its strict bridge rejects unknown
  identities or excess capacity without silently shortening the list or
  starting capture. [Standalone strict admission](docs/LINUX_NETWORK_ADMISSION.en.md)
  connects inventory to the shared provider through explicitly authorized
  refresh. An unchanged set preserves rates and epoch; changes preserve
  retained capture, and readmissions start fresh. It rejects unknowns and
  sets above 1,024 rather than capturing a hidden subset. Tests covered
  private processes, not the user's entire session.
- An [opt-in partial policy](docs/LINUX_NETWORK_ADMISSION_POLICY.en.md) can
  exclude unverified rows and report PID, state, and reason alongside the
  snapshot. Strict remains the default; more than 1,024 eligible processes
  rejects the whole set. The 0.x admission report layout changed.
  [Monitor/Sampler admission](docs/LINUX_NETWORK_ADMISSION_MONITOR.en.md)
  publishes network data, metadata, and exclusions together in
  `SystemSnapshot` and worker history. Refresh requires explicit
  authorization; flags alone do not rearm capture. Rebuild the library and
  all consumers for these layout changes.
- [F03 validation against a real process view](docs/LINUX_NETWORK_ACCEPTANCE.en.md)
  passed in WSL for 30 of 30 processes in strict mode and fixed collector
  SELF handling. This optional source has explicit limits and does not
  establish universal traffic coverage or completion of H04 history or J04
  session aggregation.

### Alternative Linux service providers and privileged operations

- [runit in ServiceInfo](docs/LINUX_RUNIT_SERVICES.en.md) exposes reported
  states, not proof of host liveness, a host PID, or control capability.
  [s6 in ServiceInfo](docs/LINUX_S6_SERVICES.en.md) exposes historical records
  and loggers without inferring a live supervisor or process.
  [OpenRC in ServiceInfo](docs/LINUX_OPENRC_SERVICES.en.md) exposes candidates and
  markers in separate views without matching identities by name or running
  scripts for collection.
- [Service selection in Monitor](docs/LINUX_SERVICE_SELECTION.en.md) lets
  consumers choose a provider and view explicitly, with no silent fallback.
  The default preserves the previous path. Rebuild clients after the
  `SampleOptions` expansion. See the [alternative init audit](docs/LINUX_K03_AUDIT.en.md)
  for unresolved K03 work.
- [Linux L05 preflight](docs/LINUX_PRIVILEGED_PREFLIGHT.en.md) is still in
  internal validation. There is no installed helper, operational
  authentication, or privilege elevation. The
  [internal authorization coordinator](docs/LINUX_PRIVILEGED_AUTHORIZATION.en.md)
  tests binding and rejection and has internal native adapters tested with
  fixtures. That coordinator does not itself execute operations.
- [Native identity checks](docs/LINUX_PRIVILEGED_IDENTITY.en.md) validate message
  and connection identity; secure deployment remains open.
  [Native CheckAuthorization](docs/LINUX_PRIVILEGED_NATIVE_AUTHORIZATION.en.md)
  is implemented and tested in isolation. The
  [fail-closed receiver](docs/LINUX_PRIVILEGED_RECEIVER.en.md) is tested with
  real libsystemd and a private broker, a simulated Authority, and no executor.
  Secure configuration and deployment and real polkit acceptance are still pending.
- The [protected internal policy](docs/LINUX_PRIVILEGED_POLICY.en.md) supports
  descriptor-anchored reads, reloads without fallback, and rejection when a
  policy generation is invalidated during a query. It still needs a secure
  bootstrap and does not install or run a helper.
  The [absolute path anchor and connector](docs/LINUX_PRIVILEGED_CONNECTION.en.md)
  verify paths and peer identity and revoke the connection on detected
  changes. Deployment hardening and real polkit and execution validation remain.
- The [internal execution flow](docs/LINUX_PRIVILEGED_EXECUTION.en.md) is off
  by default. In tests it binds fresh authorization to a restricted operation
  against a simulated manager. A queued job does not mean the service
  started, and an uncertain submission is not retried.
  [Asynchronous job tracking](docs/LINUX_PRIVILEGED_JOBS.en.md) distinguishes
  completion, unsuccessful completion, and unknown outcomes; a timeout is
  not treated as cancellation.
- The [per-request budget](docs/LINUX_PRIVILEGED_BUDGET.en.md) shares a time
  budget and RPC quota across phases; it is not a hard deadline for the
  process or transport. [Internal admission](docs/LINUX_PRIVILEGED_ADMISSION.en.md)
  limits bursts per receiver and sender and rejects local queue pressure;
  it does not limit bytes before receipt. The
  [transport boundary test](docs/LINUX_PRIVILEGED_TRANSPORT.en.md) verifies
  rejection before the handler on a limited private broker, not the host
  bus. Do not install its profile on a shared bus; deployment and
  architecture remain open.

### Partial Windows support

See the [Windows backend limits](docs/WINDOWS.en.md). Windows is secondary to
the Linux delivery and does not have feature parity.

- [General per-process I/O](docs/WINDOWS_PROCESS_IO.en.md) exposes 64-bit
  counters and rates separately from disk-specific and network-specific
  metrics. Rebuild the library and consumers for the new `ProcessInfo` field.
- The [experimental AMD memory query](docs/WINDOWS_AMD_RUNTIME_MEMORY.en.md)
  is an explicit call separate from `Monitor` and `Sampler`. It returns
  OpenCL runtime estimates, not global free memory, and can initialize
  internal GPU resources.
- [NVIDIA free framebuffer memory](docs/WINDOWS_NVML_MEMORY.en.md) optionally
  uses driver libraries and CUDA/NVML/KMT identities without requiring an
  SDK or external commands. It lacks a positive physical NVIDIA test,
  does not guarantee allocatable memory, and does not cover AMD or Intel.
- The [current-context OpenGL query](docs/WINDOWS_CURRENT_OPENGL.en.md) is
  opt-in per thread and uses the LUID to identify a DXGI adapter. It does
  not create a context or report a hardware maximum. The value stays
  unknown without verified identity.
- An [explicit action](docs/WINDOWS_STARTUP_FILE_CONTROL.en.md) withdraws or
  restores user Startup files using a checked rename, independently of KTM.
  It does not modify Windows approval state or connect to generic buttons
  automatically. [Backup reporting in Monitor/Sampler](docs/WINDOWS_STARTUP_FILE_MONITOR.en.md)
  is opt-in and independent of control and approval. Rebuild the library
  and consumers together for the `SystemSnapshot` and `SampleOptions`
  layout changes. [Common Startup scope](docs/WINDOWS_COMMON_STARTUP_FILE_CONTROL.en.md)
  is queried separately. Its actions can affect every user, never elevate
  permissions automatically, and are not equivalent to HKLM/StartupApproved.

## Project origins and contributions

The project maintainer designed PCInsight and defined its requirements. The
initial implementation was vibe-coded with Codex under the maintainer's
direction. Contributions are welcome, whether written by hand or with AI
assistance, including bug fixes, tests, documentation, and hardware validation.
See the [contribution guide](CONTRIBUTING.en.md).

## License

The [MIT license](LICENSE) applies to PCInsight, including its SDK,
documentation, and examples. It does not set the license for the separate GUI
or other projects that use the SDK. Keep the copyright and license notices
when redistributing the library or a substantial portion of it.
