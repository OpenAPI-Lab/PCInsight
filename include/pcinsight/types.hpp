#pragma once
#include "metric.hpp"
#include "linux_process_network.hpp"
#include "linux_network_admission.hpp"
#include "linux_user_network.hpp"

#include <chrono>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace pcinsight {

// Native engine capability class, NOT proof of the workload actually executing.
enum class LinuxDrmEngineClass { graphics_compute, copy, compute, video_codec, video_decode, video_encode, video_processing, jpeg };
enum class LinuxDrmCounterKind { nanoseconds, cycles, cycles_max_frequency };
struct LinuxDrmEngineUsage {
    std::string name; // Native driver engine/class name, not a generic 3D/copy mapping.
    std::uint64_t busy_nanoseconds{}, capacity{1}; // busy_nanoseconds valid only for nanoseconds kind.
    Metric<double> utilization_percent;
    Metric<LinuxDrmEngineClass> engine_class{};
    LinuxDrmCounterKind counter_kind{LinuxDrmCounterKind::nanoseconds};
    Metric<std::uint64_t> busy_cycles{}, total_cycles{};
    Metric<std::uint64_t> max_frequency_hz{};
};
struct LinuxDrmClientUsage {
    std::uint32_t pid{}, descriptor{};
    std::uint64_t process_start_ticks{}, client_id{};
    std::string driver, pci_address;
    std::chrono::steady_clock::time_point captured_at;
    std::vector<LinuxDrmEngineUsage> engines;
    Metric<std::chrono::nanoseconds> sample_interval;
};
struct LinuxDrmEngineAggregate {
    std::string name;
    Metric<std::uint64_t> capacity;
    std::uint64_t observed_clients{}, measured_clients{};
    // Sum of selected clients' rates, NOT GPU utilization; may exceed 100.
    Metric<double> summed_client_active_percent;
    Metric<std::chrono::nanoseconds> shortest_interval, longest_interval;
    Metric<LinuxDrmEngineClass> engine_class{};
    Metric<LinuxDrmCounterKind> counter_kind{};
    Metric<double> summed_client_cycle_percent; // GPU-cycle ratios; never mixed with active-time rates above.
    Metric<std::uint64_t> max_frequency_hz{};
    Metric<double> summed_client_frequency_percent; // Busy cycles / elapsed CPU time / maximum frequency / capacity.
};
struct LinuxDrmGpuAggregate {
    std::string pci_address, driver;
    std::uint64_t input_records{}, unique_clients{};
    std::chrono::steady_clock::time_point earliest_observation, latest_observation;
    std::vector<LinuxDrmEngineAggregate> engines;
    Metric<std::string> inventory_gpu_id; // Monitor only: unique available PCI match, never index/model.
};
struct LinuxDrmDiscoveryOptions {
    std::uint32_t max_processes{4096}, max_descriptors{65536}, max_observations{512};
    std::chrono::milliseconds max_duration{250}; // Cooperative budget, NOT syscall timeout.
    bool operator==(const LinuxDrmDiscoveryOptions&) const = default;
};
struct LinuxDrmDiscoverySummary {
    std::uint64_t processes_seen{}, descriptors_seen{}, drm_candidates{};
    std::uint64_t directory_failures{}, unclassified_descriptors{}, unreadable_drm_clients{}, permission_denials{};
    bool limit_reached{}, time_limit_reached{};
    std::chrono::steady_clock::time_point started_at, finished_at;
    std::string scope;
};
struct LinuxDrmDiscovery : LinuxDrmDiscoverySummary {
    std::vector<Metric<LinuxDrmClientUsage>> observations; // Successful raw reads; duplicates retained.
};
struct LinuxDrmMonitorReport {
    Metric<LinuxDrmDiscoverySummary> discovery; // No raw PID/fd observations retained in history.
    Metric<std::vector<LinuxDrmGpuAggregate>> aggregates;
};

struct CpuInfo {
    std::string vendor, model;
    Metric<double> utilization_percent, current_ghz, base_ghz;
    Metric<unsigned> sockets, physical_cores, logical_processors;
    Metric<bool> virtualization_enabled;
    // Linux: distinct sysfs cache instances reachable by online CPUs, sharing
    // deduplicated and data/instruction summed; estimated, not hidden/offline hardware.
    Metric<std::uint64_t> l1_bytes, l2_bytes, l3_bytes;
    Metric<std::uint64_t> processes, threads, handles;
    Metric<std::chrono::seconds> uptime;
    // Linux-only: compatible KVM system API queried on a validated device fd.
    // Not firmware enablement, hardware support, VM creation or guest activity.
    Metric<unsigned> linux_kvm_api_version;
    // Availability/provenance-aware CPU text; legacy vendor/model mirror these
    // when present. Reported identifiers are not physical hardware attestation.
    Metric<std::string> reported_vendor,reported_model;
};

// Linux estimate partition: unavailable + reclaimable estimate + free = MemTotal.
// Not Windows in-use/modified/standby/free and not a sum of raw meminfo categories.
struct LinuxMemoryComposition {
    std::uint64_t unavailable_estimate_bytes{}, reclaimable_estimate_bytes{}, free_bytes{};
};
struct LinuxZramMemory {
    std::uint64_t original_data_bytes{}, compressed_data_bytes{}, allocated_memory_bytes{};
};
struct LinuxZramDevice {
    std::string id; // Kernel zramN name, not persistent identity across hotplug/boots.
    Metric<LinuxZramMemory> memory;
};
struct LinuxMemoryDetails {
    Metric<LinuxMemoryComposition> composition;
    // Overlapping kernel categories: NEVER add these to the composition partition.
    Metric<std::uint64_t> free_bytes, buffers_bytes, page_cache_bytes, anonymous_bytes,
        shared_bytes, slab_reclaimable_bytes, slab_unreclaimable_bytes,
        dirty_bytes, writeback_bytes;
    Metric<std::uint64_t> zswap_pool_bytes, zswap_original_bytes;
    Metric<std::vector<LinuxZramDevice>> zram_devices;
    // Linux difference is ALWAYS estimated, even when both inputs are measured.
    // Includes kernel/boot limits/offline RAM, not exclusively hardware reserve.
    Metric<std::uint64_t> firmware_installed_bytes, installed_minus_usable_bytes;
};

struct MemoryInfo {
    std::uint64_t total_bytes{};
    Metric<double> utilization_percent;
    Metric<std::uint64_t> in_use_bytes, compressed_bytes, available_bytes;
    Metric<std::uint64_t> committed_bytes, commit_limit_bytes, cached_bytes;
    Metric<std::uint64_t> paged_pool_bytes, nonpaged_pool_bytes;
    // Linux hardware_reserved_bytes is a legacy estimated alias of
    // linux_details.installed_minus_usable_bytes. Prefer that explicit name.
    Metric<std::uint64_t> speed_mts, slots_used, slots_total, hardware_reserved_bytes;
    Metric<std::string> type, form_factor;
    // Availability-aware OS-visible total. Prefer over legacy total_bytes;
    // a zero legacy fallback does not establish a measured total.
    Metric<std::uint64_t> reported_total_bytes;
    LinuxMemoryDetails linux_details;
};

struct LinuxNetworkDetails {
    std::uint32_t interface_index{}, link_layer_type{}; // Kernel ARPHRD, not bus/media.
    Metric<std::string> vendor, model, connection_type, bus, driver;
    Metric<std::uint32_t> hardware_vendor_id, hardware_device_id; // Bus-specific PCI or USB IDs.
    Metric<bool> virtual_device, wireless;
};
struct NetworkAdapter {
    std::string id, name, model, connection_type;
    std::vector<std::string> ipv4, ipv6;
    Metric<std::uint64_t> send_bytes_per_second, receive_bytes_per_second;
    std::uint64_t total_sent_bytes{}, total_received_bytes{};
    Metric<LinuxNetworkDetails> linux_details;
};

struct LinuxDiskDetails {
    Metric<std::string> kernel_subsystem;
    // Device-side protocol and reported target/controller transport are separate:
    // a SATA device may be reached through a bridge or a SAS target port.
    Metric<std::string> device_protocol, transport;
    Metric<std::string> media_type;
    Metric<std::uint32_t> nominal_rotation_rpm;
};

struct DiskInfo {
    std::string id, model, bus_type, media_type;
    Metric<std::string> reported_media_type; // Provider-reported medium, not queue/rotational inference.
    Metric<double> active_percent;
    Metric<std::uint64_t> transfer_bytes_per_second, read_bytes_per_second, write_bytes_per_second;
    Metric<double> average_response_ms;
    Metric<std::uint64_t> capacity_bytes, formatted_capacity_bytes;
    Metric<bool> system_disk, contains_page_file;
    // Linux queue/rotational hint, NOT a physical HDD/SSD classification.
    // Absent on backends that do not query this property.
    Metric<bool> rotational;
    Metric<LinuxDiskDetails> linux_details;
};

struct GpuEngine {
    std::string name; Metric<double> utilization_percent;
    std::string id{}, type{}; // Windows physical-node/engine identity and reported type
};
struct GpuProcessMemoryBudget {
    Metric<std::uint32_t> process_id, node_index;
    Metric<std::uint64_t> budget_bytes, current_usage_bytes;
    Metric<std::uint64_t> available_for_reservation_bytes, current_reservation_bytes;
    Metric<std::uint64_t> budget_headroom_bytes; // max(budget-usage,0), NOT GPU-global free memory
};
struct LinuxOpenGlDetails {
    std::string context_api, provider;
    Metric<std::string> context_version;
    // Renderer-reported profile capabilities, NOT a context creation test or
    // a driver-independent/physical-hardware maximum. Unknown on EGL-only path.
    Metric<std::string> reported_max_core_profile_version, reported_max_compatibility_profile_version;
    Metric<bool> core_profile_supported, compatibility_profile_supported;
};
struct LinuxGpuTemperatureSensor {
    std::string id;
    Metric<std::string> label;
    Metric<double> celsius;
};
struct LinuxGpuSysfsDetails {
    Metric<std::string> kernel_driver_name, module_name, module_version;
    Metric<std::uint64_t> vram_total_bytes, vram_used_bytes, vram_headroom_bytes;
    // GTT manager counters, NOT Windows shared GPU capacity/usage or RAM free.
    Metric<std::uint64_t> gtt_total_bytes, gtt_used_bytes, gtt_headroom_bytes;
    std::vector<LinuxGpuTemperatureSensor> temperature_sensors;
};
struct GpuInfo {
    std::string id, vendor, model;
    std::vector<GpuEngine> engines;
    Metric<double> utilization_percent, temperature_celsius;
    Metric<std::uint64_t> memory_used_bytes, memory_available_bytes;
    Metric<std::uint64_t> dedicated_memory_bytes, shared_memory_bytes;
    Metric<std::string> driver_version, driver_date, vulkan_version, opencl_version, opengl_version;
    Metric<std::string> physical_location;
    // Windows DXGI static inventory; not live GPU-global usage or a PCI location.
    Metric<std::uint64_t> dedicated_system_memory_bytes;
    Metric<std::uint32_t> hardware_vendor_id, hardware_device_id; // DXGI or Linux DRM PCI IDs
    Metric<std::uint64_t> dedicated_memory_used_bytes, shared_memory_used_bytes;
    // Windows: highest validated known D3D12 candidate, not installed DirectX version.
    Metric<std::string> direct3d12_feature_level;
    // Windows caller process, physical node 0; LOCAL/NON_LOCAL are not dedicated/shared aliases.
    GpuProcessMemoryBudget local_process_memory, non_local_process_memory;
    // Optional local PCI database labels, not driver-reported retail/board identity.
    Metric<std::string> pci_vendor_name, pci_device_name;
    Metric<LinuxOpenGlDetails> linux_opengl_details;
    Metric<LinuxGpuSysfsDetails> linux_sysfs_details;
};

// Linux per-field availability. Sequential observations, not an atomic row.
// Desktop descriptions are untrusted metadata associations, not authentication.
struct LinuxProcessDetails {
    Metric<std::string> name,status,username,architecture,description,executable;
    Metric<std::string> description_source;
    Metric<std::uint32_t> real_uid;
    // UID observed stable around BOTH I/O counter reads for this rate interval.
    // Estimated endpoint attribution, not proof of continuous credentials or causality.
    Metric<std::uint32_t> disk_rate_real_uid;
};
// Windows process-wide I/O accounting, including files/network/devices.
// NEVER use these counters/rates as disk-only or network-only attribution.
struct WindowsProcessIoCounters {
    std::uint64_t read_operations{}, write_operations{}, other_operations{};
    std::uint64_t read_transfer_bytes{}, write_transfer_bytes{}, other_transfer_bytes{};
};
struct WindowsProcessIoDetails {
    WindowsProcessIoCounters counters; // Native cumulative uint64 counters, exact representation.
    Metric<double> read_bytes_per_second, write_bytes_per_second, other_bytes_per_second;
    Metric<double> interval_seconds; // Monotonic interval shared by the three estimated rates.
};
struct ProcessInfo {
    std::uint32_t pid{}, parent_pid{};
    std::string name, status, username, architecture, description, executable;
    Metric<double> cpu_percent;
    Metric<std::uint64_t> memory_bytes, disk_bytes_per_second, network_bytes_per_second;
    Metric<bool> efficiency_mode_available;
    Metric<bool> efficiency_mode_requested; // Windows explicit EcoQoS, not effective hardware QoS.
    std::uint64_t thread_count{};
    // Native FILETIME creation timestamp (100ns since 1601 UTC), not Unix time.
    // Windows only; pass unchanged with pid to terminate_process_checked.
    Metric<std::uint64_t> windows_creation_time_100ns;
    // Linux /proc/PID/stat field 22, clock ticks since boot, NOT wall-clock time.
    // Pass unchanged with pid to terminate_process_checked on the same host/boot.
    Metric<std::uint64_t> linux_start_ticks;
    // Windows primary-token Terminal Services session ID; not a stable action token.
    Metric<std::uint32_t> windows_session_id;
    Metric<std::uint64_t> windows_cpu_time_100ns; // Native cumulative kernel+user CPU, not elapsed wall time.
    LinuxProcessDetails linux_details; // Other backends leave all fields unsupported.
    Metric<WindowsProcessIoDetails> windows_io; // Other backends leave unsupported.
};

struct LinuxServiceDetails {
    std::string provider, group_kind; // systemd-system-manager / systemd-slice, or a partial file scan.
    Metric<std::string> name, description, status;
    Metric<std::string> load_state, active_state, sub_state, unit_file_state, slice;
    // Native MainPID=0 means no main PID reported, not a usable process identity.
    Metric<std::uint32_t> main_pid;
};

struct ServiceInfo {
    std::string id, name, description, status, group;
    std::optional<std::uint32_t> pid;
    // systemd metadata. Empty on other providers or when not known.
    std::string load_state, active_state, sub_state, unit_file_state;
    Metric<LinuxServiceDetails> linux_details;
};

enum class StartupStatus { enabled, disabled, unknown };
enum class StartupImpact { none, low, medium, high, not_measured };
struct WindowsShortcutInfo {
    // Stored metadata only, NOT a resolved/verified launch target or argument vector.
    Metric<std::string> stored_local_target, arguments, working_directory;
    Metric<std::string> stored_url; // .url metadata only; arbitrary schemes are NOT authorized for navigation.
};
enum class WindowsSignatureStatus { trusted_local_policy, rejected, no_embedded_signature, unavailable };
struct WindowsStartupSignature {
    WindowsSignatureStatus status{WindowsSignatureStatus::unavailable};
    Metric<std::int32_t> native_status; // WinVerifyTrust LONG, success is exactly zero.
    Metric<std::string> signer_name; // Leaf certificate display name only after trust success.
    std::string source_executable, policy;
};
struct StartupEntry {
    std::string id, name, publisher, command, source;
    StartupStatus status{StartupStatus::unknown};
    StartupImpact impact{StartupImpact::not_measured};
    // Linux caller-context standard XDG conditions, NOT proof of launch/readiness.
    Metric<bool> xdg_standard_eligible;
    // Linux: evaluates the AutostartCondition predicate only, not manager support
    // or launch readiness. No key -> true; unsupported/malformed -> no value.
    Metric<bool> autostart_condition_satisfied;
    // Opt-in overlay of caller-associated observations, NOT automatic boot impact.
    // impact above is only the shorthand; GUI must show confidence/scope below.
    Metric<StartupImpact> observed_startup_impact;
    Metric<std::chrono::nanoseconds> observed_startup_cpu_time;
    Metric<std::uint64_t> observed_startup_disk_bytes;
    Metric<std::chrono::steady_clock::duration> observed_startup_interval;
    std::string impact_scope;
    Metric<WindowsShortcutInfo> windows_shortcut;
    // Opt-in self-reported VERSIONINFO (Windows) or AppStream developer (Linux),
    // never a verified signer. Source executable is absent for AppStream.
    Metric<std::string> publisher_label, publisher_source_executable;
    Metric<WindowsStartupSignature> windows_signature;
};

// PCInsight-owned registration state, NOT Windows StartupApproved or launch eligibility.
enum class WindowsRunRegistrationState { registered, disabled_by_pcinsight, conflict, absent };
struct WindowsRunRegistration {
    std::string entry_id, command;
    WindowsRunRegistrationState state{WindowsRunRegistrationState::absent};
    std::uint32_t native_type{}; // REG_SZ=1 or REG_EXPAND_SZ=2; never expand.
    std::vector<std::uint8_t> native_data; // Exact bytes including any terminator, for checked action.
};

enum class WindowsStartupFileState { absent, registered, retained_by_pcinsight, conflict };
struct WindowsStartupFileIdentity {
    std::uint64_t volume{}, file_index{}, creation_time_100ns{}, last_write_time_100ns{}, size_bytes{};
    bool operator==(const WindowsStartupFileIdentity&) const = default;
};
struct WindowsStartupFileRegistration {
    std::string entry_id;
    WindowsStartupFileState state{WindowsStartupFileState::absent};
    WindowsStartupFileIdentity directory_identity;
    // Present only for registered/retained states, not absent/conflict.
    std::optional<WindowsStartupFileIdentity> file_identity;
};
struct WindowsStartupFileBackup {
    std::string name; // Exact UTF-8 filename, not an executable/product description.
    WindowsStartupFileRegistration registration;
};

struct LinuxSessionDetails {
    std::string provider; // "login1" or "utmp"; not an authentication authority.
    Metric<std::string> state; // Native provider state, not a Windows WTS mapping.
    Metric<bool> active, remote; // login1 foreground flag and reported remote origin.
    Metric<std::uint32_t> unix_uid; // login1 ListSessions UID; not a global identity across namespaces.
};

struct LinuxUserIoCoverage {
    std::uint64_t known{}, missing{}, permission_denied{}, identity_unverified{};
    bool overflow{};
};
struct LinuxUserIoDetails {
    std::string scope; // visible processes by reported real UID, NOT by session.
    Metric<std::uint32_t> real_uid;
    std::uint64_t input_processes{}, processes_without_uid{};
    Metric<std::uint64_t> matching_processes;
    Metric<LinuxUserIoCoverage> disk_coverage;
    Metric<std::uint64_t> disk_bytes_per_second, network_bytes_per_second;
};

struct UserSession {
    std::string id, username, display_name, status;
    Metric<double> cpu_percent;
    Metric<std::uint64_t> memory_bytes, disk_bytes_per_second, network_bytes_per_second;
    // Linux CPU/RAM repeat by username; I/O uses linux_io_details' numeric scope.
    // Don't sum Linux rows as if independent ownership. Windows scope identifies WTS session/account sums.
    std::string resource_scope;
    // Availability of detach preserving applications, NOT logout/termination.
    Metric<bool> disconnect_available;
    // Explicit destructive logout, separate from detach. Linux login1 provider.
    std::string logout_token;
    Metric<bool> logout_available; // token can be queried; authorization not guaranteed
    Metric<LinuxSessionDetails> linux_details;
    Metric<LinuxUserIoDetails> linux_io_details;
};

// Legacy v1/v2 payload ONLY: counters have no recorded availability. Native
// collectors leave SystemSnapshot::app_history empty; use app_history.hpp view.
struct AppHistoryEntry {
    std::string id, name;
    std::chrono::nanoseconds cpu_time{};
    std::uint64_t network_bytes{};
    std::uint64_t notifications{};
    std::chrono::system_clock::time_point period_start, period_end;
};
// Same type/layout/symbol identity for source and on-disk compatibility.
using LegacyAppHistoryEntry = AppHistoryEntry;
struct WindowsAppHistoryEntry {
    std::string id, name; // Exact observed executable path; not commercial/package identity.
    Metric<std::chrono::nanoseconds> observed_cpu_time;
    Metric<std::uint64_t> network_bytes, notifications;
    std::chrono::system_clock::time_point period_start, period_end;
};
struct LinuxAppHistoryEntry {
    std::string id,name; // Exact observed exe path; kernel label, not package identity.
    Metric<std::chrono::nanoseconds> observed_cpu_time;
    Metric<std::uint64_t> network_bytes,notifications;
    std::chrono::system_clock::time_point period_start,period_end;
};

struct SystemSnapshot {
    std::chrono::system_clock::time_point captured_at;
    CpuInfo cpu;
    MemoryInfo memory;
    std::vector<DiskInfo> disks;
    std::vector<NetworkAdapter> networks;
    std::vector<GpuInfo> gpus;
    std::vector<ProcessInfo> processes;
    std::vector<ServiceInfo> services;
    std::vector<StartupEntry> startup_entries;
    std::vector<UserSession> user_sessions;
    std::vector<LegacyAppHistoryEntry> app_history; // Legacy payload, NOT measured telemetry; native backends keep empty.
    LinuxDrmMonitorReport linux_drm;
    Metric<std::vector<WindowsRunRegistration>> windows_run_registrations; // Separate opt-in metadata, not generic startup status.
    Metric<std::vector<WindowsAppHistoryEntry>> windows_app_history; // include_app_history; no legacy zero-valued unknowns.
    Metric<std::vector<LinuxAppHistoryEntry>> linux_app_history; // Observed CPU only; explicit availability, legacy app_history stays empty.
    Metric<std::vector<LinuxAppHistoryEntry>> linux_network_app_history; // Independent observed payload period; CPU/notifications unknown. Never merge blindly with CPU periods.
    Metric<LinuxUserNetworkReport> linux_user_network; // Unique observer-domain UID rows; NOT repeated or joined by login session/name.
    // Known means the selected provider completed its enumeration (which may be empty),
    // NOT that every session/user on the host is represented. Linux only.
    Metric<std::string> linux_session_inventory_source;
    Metric<std::string> linux_service_inventory_source; // Selected manager enumeration or explicitly partial file scan; Linux only.
    Metric<std::string> linux_gpu_inventory_source; // Selected DRM/NVML coverage; empty success != inaccessible/disabled.
    Metric<std::vector<WindowsStartupFileBackup>> windows_startup_file_backups; // Separate opt-in, NOT Windows approval or generic startup status.
    Metric<LinuxProcessNetworkSnapshot> linux_process_network; // Separate selected payload report, NEVER legacy/global totals.
    Metric<LinuxNetworkAdmissionChange> linux_network_admission; // Last explicit visible refresh, not a census at sample time; absent in manual-selection mode.
};

enum class LinuxServiceProvider { legacy_default,systemd_system,systemd_current_user,sysv,runit,s6,openrc_configured,openrc_marked };
struct LinuxServiceSelection {
    LinuxServiceProvider provider{LinuxServiceProvider::legacy_default};
    std::string root; // runit/s6 scan root, or OpenRC runtime root.
    std::string init_directory,runlevel_directory; // SysV/OpenRC configured roots.
    std::uint32_t max_entries{1024}; // Filesystem providers only.
    std::uint64_t sysv_prefix_budget{8*1024*1024}; // SysV descriptive read only; 0 disables.
};
struct SampleOptions {
    bool include_processes{true};
    bool include_disks{true};
    bool include_networks{true};
    bool include_gpus{true};
    bool include_services{false};
    bool include_startup_entries{false};
    bool include_user_sessions{false};
    bool include_app_history{false};
    bool include_disk_media{true}; // Windows storage-provider metadata; false avoids COM/WMI.
    bool prefer_logind_sessions{true}; // Linux; false explicitly chooses utmp reports.
    bool include_current_opengl_context{false}; // Caller-thread Linux EGL/GLX or Windows WGL query; no context creation. GLX can report profile limits.
    bool include_linux_drm_clients{false}; // Explicit potentially costly scan; also requires include_gpus.
    LinuxDrmDiscoveryOptions linux_drm_limits{};
    bool include_windows_startup_shortcut_metadata{false}; // Also requires include_startup_entries; bounded read-only .lnk/.url parsing.
    bool include_windows_startup_publisher{false}; // Separate opt-in: reads explicitly identified executable version resources, not signatures.
    bool include_windows_startup_signature{false}; // Separate expensive opt-in: Authenticode local/cache-only policy, no revocation checking.
    bool include_windows_managed_run{false}; // Independent read-only HKCU Run/PCInsight-backup listing; no KTM or mutation.
    // Linux: bounded XDG application metadata lookup; false avoids the scan.
    bool include_linux_process_descriptions{true};
    bool include_linux_startup_publisher{false}; // Also requires include_startup_entries; optional system libxml2, bounded AppStream metadata scan.
    bool include_windows_startup_file_backups{false}; // Independent read-only discovery; no restore, registry, or generic Startup inventory.
    bool include_linux_process_network{false}; // Read explicit capture only; flag never starts/authorizes BPF. False stops capture.
    bool include_linux_user_network{false}; // Unique observer-UID report without requiring login/session inventory; still requires authorized F03 capture + network flag.
    // Linux only, also requires include_services. Explicit providers NEVER fallback.
    // Appended field: recompile all 0.x clients; default preserves legacy behavior.
    LinuxServiceSelection linux_service_selection{};
};

} // namespace pcinsight
