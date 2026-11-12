#include <pcinsight/monitor.hpp>
#include <cstdio>
#include <pcinsight/windows_amd_runtime_memory.hpp>
#include <pcinsight/app_history.hpp>
#include <pcinsight/sampler.hpp>
#include <pcinsight/actions.hpp>
#include <pcinsight/account_management.hpp>
#include <pcinsight/linux_efficiency.hpp>
#include <pcinsight/linux_session_control.hpp>
#include <pcinsight/linux_services.hpp>
#include <pcinsight/linux_tcp.hpp>
#include <pcinsight/linux_process_network.hpp>
#include <pcinsight/linux_process_network_selection.hpp>
#include <pcinsight/linux_network_inventory.hpp>
#include <pcinsight/linux_network_admission.hpp>
#include <pcinsight/linux_user_network.hpp>
#include <pcinsight/startup_recorder.hpp>
#include <pcinsight/startup_birth_observer.hpp>
#include <pcinsight/startup_birth_impact.hpp>
#include <pcinsight/startup_birth_overlay.hpp>
#include <pcinsight/startup_executable_matches.hpp>
static_assert(pcinsight::LinuxProcessNetworkSampler::max_processes==1024);
static_assert(pcinsight::LinuxProcessNetworkSampler::max_observed_buckets==16384);
#include <pcinsight/audit.hpp>
#include <pcinsight/startup.hpp>
#include <pcinsight/linux_startup_publisher.hpp>
#include <pcinsight/linux_notifications.hpp>
#include <pcinsight/linux_filesystem_capacity.hpp>
#include <pcinsight/startup_impact.hpp>
#include <pcinsight/startup_observer.hpp>
#include <pcinsight/startup_candidates.hpp>
#include <pcinsight/startup_overlay.hpp>
#include <pcinsight/startup_store.hpp>
#include <pcinsight/linux_drm.hpp>
#include <pcinsight/windows_history_store.hpp>
#include <pcinsight/linux_history_store.hpp>
#include <pcinsight/windows_services.hpp>
#include <pcinsight/windows_network_identity.hpp>
#include <pcinsight/windows_startup_file_control.hpp>
int main() {
    {pcinsight::SystemSnapshot s;if(pcinsight::apply_startup_birth_impact(s,{},{}).error!=pcinsight::ActionError::permission_denied)return 96;}
    pcinsight::LinuxStartupBirthObserver birth_observer;
    if(pcinsight::discover_startup_executable_matches({},{}).error!=pcinsight::ActionError::invalid_argument)return 98;
    if(pcinsight::measure_startup_birth_impact({},{}).error!=pcinsight::ActionError::invalid_argument)return 99;
    {pcinsight::LinuxStartupRecorder recorder;if(recorder.watch_births(false,-1,0,0,{}).error!=pcinsight::ActionError::permission_denied)return 97;}
    if(birth_observer.start(false,-1,0,0,{}).error!=pcinsight::ActionError::permission_denied)return 99;
    birth_observer.close();
    pcinsight::LinuxStartupRecorder idle_recorder;
    const pcinsight::StartupRecorderOptions recorder_defaults;
    if(recorder_defaults.enable_privileged_exit_disk_recovery||recorder_defaults.max_exit_disk_captures!=8||idle_recorder.report().exit_disk_attached||idle_recorder.report().exit_disk_released_without_final)return 1;
    pcinsight::StartupImpactArchive legacy_startup_capture;
    if(legacy_startup_capture.diagnostics)return 1;
    pcinsight::StartupCaptureDiagnostics diagnostic_shape;
    if(diagnostic_shape.elapsed.count()!=0||diagnostic_shape.exit_disk_enabled)return 1;
    if(idle_recorder.start({}).error!=pcinsight::ActionError::permission_denied||idle_recorder.report().state!=pcinsight::StartupRecorderState::idle||idle_recorder.save({}))return 1;
    if(pcinsight::summarize_linux_user_network({}).value)return 1;
    if(!pcinsight::LinuxHistoryStore::merge({},{}).empty()||pcinsight::LinuxHistoryStore::load({}))return 1;
    std::fputs("consumer: begin selected network checks\n",stderr);
    pcinsight::LinuxVisibleProcessNetworkSampler visible_network;
    if(visible_network.refresh_visible(false).support!=pcinsight::Support::permission_required||visible_network.sample().value)return 1;
    visible_network.stop();
    if(visible_network.refresh_visible(false,pcinsight::LinuxNetworkAdmissionPolicy::verified_only).support!=pcinsight::Support::permission_required||visible_network.sample_with_admission().value)return 1;
    const auto absent_admission_plan=pcinsight::plan_linux_network_admission({{},pcinsight::Support::unsupported,"consumer absent inventory"},pcinsight::LinuxNetworkAdmissionPolicy::verified_only);
    if(absent_admission_plan.value)return 1;
    pcinsight::Monitor visible_monitor;
    if(visible_monitor.refresh_linux_visible_process_network(false).support!=pcinsight::Support::permission_required)return 1;
    pcinsight::Sampler visible_worker;
    if(visible_worker.start_linux_visible_process_network(false).support!=pcinsight::Support::permission_required||visible_worker.refresh_linux_visible_process_network(false).support!=pcinsight::Support::permission_required)return 1;
    pcinsight::LinuxProcessNetworkSampler process_network;
    pcinsight::LinuxProcessNetworkSampler process_buckets(pcinsight::LinuxProcessNetworkSelectionMode::declared_namespace_buckets);
    if(process_buckets.start(false,{}).support!=pcinsight::Support::permission_required||process_buckets.sample().value)return 1;
    process_buckets.stop();
    pcinsight::LinuxProcessNetworkSampler observed_network(pcinsight::LinuxProcessNetworkSelectionMode::observed_namespace_buckets);
    static_assert(pcinsight::LinuxProcessNetworkSampler::max_observed_buckets==16384);
    pcinsight::LinuxProcessNetworkSnapshot observed_shape;
    observed_shape.coverage=pcinsight::LinuxProcessNetworkCoverage::observed_tcp_udp_socket_namespace_buckets;
    observed_shape.observed_processes.push_back({});
    if(observed_shape.observed_processes[0].observed_buckets!=0||observed_network.start(false,{}).support!=pcinsight::Support::permission_required||observed_network.sample().value)return 1;
    observed_network.stop();
    pcinsight::LinuxProcessNetworkSelectionOwner selection_owner;
    if(selection_owner.refresh(false,{}).support!=pcinsight::Support::permission_required||selection_owner.selections().value)return 1;
    auto moved_selection_owner=std::move(selection_owner);selection_owner.clear();moved_selection_owner.clear();
    if(pcinsight::read_linux_network_process_inventory(false).support!=pcinsight::Support::permission_required)return 1;
    if(pcinsight::linux_network_inventory_candidates({}).value)return 1;
    const auto capture_denied=process_network.start(false,{});
    if(process_network.update_selection(false,{}).support!=pcinsight::Support::permission_required||process_network.update_selection(true,{}).value)return 1;
    if(capture_denied.value||capture_denied.support!=pcinsight::Support::permission_required||process_network.sample().value)return 1;
    process_network.stop();
    std::fputs("consumer: standalone selected network passed\n",stderr);
    pcinsight::SampleOptions selected_network_options;selected_network_options.include_linux_process_network=true;
    pcinsight::Monitor selected_network_monitor;
    if(selected_network_monitor.start_linux_process_network(false,{}).value)return 1;
    if(selected_network_monitor.start_linux_process_network(false,{},pcinsight::LinuxProcessNetworkSelectionMode::declared_namespace_buckets).support!=pcinsight::Support::permission_required)return 1;
    if(selected_network_monitor.start_linux_process_network(false,{},pcinsight::LinuxProcessNetworkSelectionMode::observed_namespace_buckets).support!=pcinsight::Support::permission_required)return 1;
    if(selected_network_monitor.update_linux_process_network_selection(false,{}).support!=pcinsight::Support::permission_required||selected_network_monitor.update_linux_process_network_selection(true,{}).value)return 1;
    if(selected_network_monitor.sample(selected_network_options).linux_process_network.value)return 1;
    selected_network_monitor.stop_linux_process_network();
    std::fputs("consumer: Monitor selected network passed\n",stderr);
    pcinsight::Sampler selected_network_sampler(std::chrono::milliseconds(100),2,selected_network_options);
    if(selected_network_sampler.start_linux_process_network(false,{}).value)return 1;
    if(selected_network_sampler.start_linux_process_network(false,{},pcinsight::LinuxProcessNetworkSelectionMode::declared_namespace_buckets).support!=pcinsight::Support::permission_required)return 1;
    if(selected_network_sampler.start_linux_process_network(false,{},pcinsight::LinuxProcessNetworkSelectionMode::observed_namespace_buckets).support!=pcinsight::Support::permission_required)return 1;
    if(selected_network_sampler.update_linux_process_network_selection(false,{}).support!=pcinsight::Support::permission_required||selected_network_sampler.update_linux_process_network_selection(true,{}).value)return 1;
    selected_network_sampler.stop();
    std::fputs("consumer: Sampler selected network passed; begin legacy checks\n",stderr);
#ifndef _WIN32
    const auto experimental_amd=pcinsight::query_experimental_windows_amd_runtime_memory();
    if(experimental_amd.support!=pcinsight::Support::unsupported || !experimental_amd.gpus.empty())return 1;
#endif
    for (const auto provider : {pcinsight::LinuxDisconnectProvider::utmp, pcinsight::LinuxDisconnectProvider::login1}) {
        const auto capability = pcinsight::query_linux_disconnect_capability(provider);
#ifdef __linux__
        if (capability.value != false || capability.support != pcinsight::Support::available || capability.note.empty()) return 1;
#else
        if (capability.value || capability.support != pcinsight::Support::unsupported) return 1;
#endif
    }
    if (pcinsight::query_linux_disconnect_capability(static_cast<pcinsight::LinuxDisconnectProvider>(99)).value) return 1;
    for(const auto view:{pcinsight::WindowsMachineRunView::registry32,pcinsight::WindowsMachineRunView::registry64}){
        const auto machine_run=pcinsight::list_windows_machine_run_registrations(view);
#ifndef _WIN32
        if(machine_run.value)return 1;
#else
        if(machine_run.value)for(const auto& row:*machine_run.value){if(!row.entry_id.starts_with(view==pcinsight::WindowsMachineRunView::registry32?"winrun:hklm:32:":"winrun:hklm:64:"))return 1;}
#endif
    }
    std::fputs("consumer: legacy Run listings passed\n",stderr);
    const auto common_backups=pcinsight::list_windows_common_startup_file_backups(); // Independent, read-only, never restore automatically.
#ifndef _WIN32
    if(common_backups.value)return 1;
#else
    if(common_backups.value)for(const auto& item:*common_backups.value){if(!item.registration.entry_id.starts_with("winfolder:common:"))return 1;}
#endif
    const auto startup_backups=pcinsight::list_windows_startup_file_backups(); // Read-only; never restore automatically.
#ifndef _WIN32
    if(startup_backups.value)return 1;
#else
    if(startup_backups.value)for(const auto& item:*startup_backups.value){const auto& row=item.registration;
        if(item.name.empty())return 1;
        if(!row.entry_id.starts_with("winfolder:user:") || (row.state!=pcinsight::WindowsStartupFileState::retained_by_pcinsight && row.state!=pcinsight::WindowsStartupFileState::conflict))return 1;
    }
#endif
    if(pcinsight::read_windows_startup_file_registration("invalid").value)return 1;
    if(pcinsight::set_windows_startup_file_registered({},false).error==pcinsight::ActionError::none)return 1;
    if(pcinsight::query_windows_network_identity("invalid").value)return 1;
    const auto invalid_service=pcinsight::open_windows_service("");
    if(invalid_service.view || invalid_service.result.error==pcinsight::ActionError::none)return 1;
    if(pcinsight::read_linux_filesystem_capacity({}).value)return 1; // Link installed API, no raw-device access.
    if(pcinsight::read_linux_disk_filesystem_capacity("invalid/identifier",true).value)return 1;
    pcinsight::LinuxNotificationObserver notifications;
    if(notifications.report().value || notifications.start().error != pcinsight::ActionError::invalid_argument)return 1;
    notifications.stop(); // Installed API linked without monitoring the user's bus.
    std::fputs("consumer: legacy startup/service probes passed\n",stderr);
    const auto declared_publishers = pcinsight::read_linux_startup_publishers();
    std::vector<pcinsight::StartupEntry> no_startup_rows;
    pcinsight::apply_linux_startup_publishers(no_startup_rows, declared_publishers);
#ifndef __linux__
    if(declared_publishers.value)return 1;
#endif
    pcinsight::AccountManagementIntegration accounts;
    if(accounts.capability().value||accounts.request_open().host_invoked)return 1;
    if(pcinsight::open_linux_service({pcinsight::LinuxServiceScope::current_user,"../invalid.service"}).value)return 1;
    if(pcinsight::query_linux_service_inventory(static_cast<pcinsight::LinuxServiceScope>(99)).value)return 1;
    if(pcinsight::control_linux_service({pcinsight::LinuxServiceScope::current_user,"../invalid.service"},pcinsight::ServiceAction::start).error!=pcinsight::ActionError::invalid_argument)return 1;
    // Compile/link the installed API without authorizing any priority change.
    if (pcinsight::apply_linux_efficiency_unchecked(0, 0).result.error == pcinsight::ActionError::none) return 1;
    if (pcinsight::restore_linux_efficiency_unchecked({}).result.error == pcinsight::ActionError::none) return 1;
    pcinsight::Monitor monitor;
    pcinsight::SampleOptions backup_options;
    backup_options.include_processes=backup_options.include_disks=backup_options.include_networks=backup_options.include_gpus=false;
    if(backup_options.include_windows_startup_file_backups)return 1;
    const auto backup_disabled=monitor.sample(backup_options);
    if(backup_disabled.windows_startup_file_backups.value || backup_disabled.windows_startup_file_backups.note.find("disabled")==std::string::npos)return 1;
    backup_options.include_windows_startup_file_backups=true;
    const auto backup_enabled=monitor.sample(backup_options);
    if(!backup_enabled.startup_entries.empty() || backup_enabled.windows_run_registrations.value || backup_enabled.windows_startup_file_backups.note.empty())return 1;
#ifndef _WIN32
    if(backup_enabled.windows_startup_file_backups.value)return 1;
#endif
    pcinsight::LinuxTcpSampler tcp_sampler;
    pcinsight::LinuxTcpSocketHolderSampler held_tcp;
    if(held_tcp.sample({}).value||pcinsight::read_linux_tcp_socket_holder({}).value)return 1;
    held_tcp.reset();
    tcp_sampler.reset();
#ifndef __linux__
    if(tcp_sampler.sample().value)return 1;
#else
    const auto tcp=tcp_sampler.sample();
    if(pcinsight::metric_is_known(tcp)){
        if(!tcp.value->network_namespace_inode||!tcp.value->user_namespace_inode)return 1;
        for(const auto& socket:tcp.value->sockets)
            if(socket.acknowledged_bytes_per_second.value||socket.received_bytes_per_second.value)return 1;
        for(const auto& user:tcp.value->users)
            if(user.acknowledged_coverage.known+user.acknowledged_coverage.missing!=user.observed_sockets)return 1;
    }else if(tcp.value||tcp.note.empty())return 1;
#endif
    pcinsight::LinuxUserIoDetails user_io;
    if(user_io.real_uid.value||user_io.matching_processes.value||user_io.disk_coverage.value||user_io.disk_bytes_per_second.value||user_io.network_bytes_per_second.value)return 1;
    pcinsight::LinuxProcessDetails io_process;
    if(io_process.disk_rate_real_uid.value)return 1;
    pcinsight::LinuxDrmClientSampler drm_client;
    if (drm_client.sample(0, 0).value || pcinsight::read_linux_drm_client_usage(0, 0).value) return 1;
    drm_client.reset();
    pcinsight::LinuxDrmGpuSampler drm_selected;
    const auto empty_drm = drm_selected.sample({});
    if (!empty_drm.value || !empty_drm.value->empty()) return 1;
    drm_selected.reset();
    if (pcinsight::classify_linux_drm_engine("i915", "render").value != pcinsight::LinuxDrmEngineClass::graphics_compute) return 1;
    if (pcinsight::classify_linux_drm_engine("unknown", "compute").value) return 1;
    pcinsight::LinuxDrmEngineUsage raw_drm;
    pcinsight::LinuxDrmEngineAggregate summed_drm;
    if (raw_drm.counter_kind != pcinsight::LinuxDrmCounterKind::nanoseconds || raw_drm.busy_cycles.value ||
        raw_drm.total_cycles.value || raw_drm.max_frequency_hz.value || summed_drm.max_frequency_hz.value ||
        summed_drm.summed_client_cycle_percent.value || summed_drm.summed_client_frequency_percent.value) return 1;
    pcinsight::LinuxDrmDiscoveryOptions drm_limits;
    drm_limits.max_processes = 0; // Invalid limits must not start a scan.
    if (pcinsight::discover_linux_drm_clients(drm_limits).value) return 1;
    pcinsight::SampleOptions defaults;
    if(!defaults.include_linux_process_descriptions) return 1;
    if (defaults.include_current_opengl_context || defaults.include_linux_drm_clients ||
        defaults.include_windows_startup_shortcut_metadata || defaults.include_windows_startup_publisher ||
        defaults.include_windows_startup_signature || defaults.include_windows_managed_run) return 1;
    const auto snapshot = monitor.sample();
    if(snapshot.linux_user_network.value||pcinsight::SampleOptions{}.include_linux_user_network)return 1;
    for(const auto& process:snapshot.processes){
#ifdef _WIN32
        if(process.windows_io.value && (!pcinsight::metric_is_known(process.windows_creation_time_100ns) || !pcinsight::metric_is_known(process.windows_io)))return 1;
        if(process.windows_io.value && (process.windows_io.value->read_bytes_per_second.value || process.windows_io.value->write_bytes_per_second.value || process.windows_io.value->other_bytes_per_second.value))return 1; // first capture, no baseline
#else
        if(process.windows_io.value || process.windows_io.support!=pcinsight::Support::unsupported)return 1;
#endif
    }
#ifdef _WIN32
    pcinsight::SampleOptions wgl_options;
    wgl_options.include_processes=wgl_options.include_disks=wgl_options.include_networks=false;
    wgl_options.include_current_opengl_context=true;
    // This standalone consumer never creates a WGL context; opt-in must not
    // invent a global driver version or create a context on its behalf.
    for(const auto& gpu:monitor.sample(wgl_options).gpus)if(gpu.opengl_version.value)return 1;
#endif
#ifdef __linux__
    if(snapshot.linux_gpu_inventory_source.note.empty())return 1;
    for(const auto& gpu:snapshot.gpus)if(gpu.linux_sysfs_details.value){
        const auto& details=*gpu.linux_sysfs_details.value;
        if(details.vram_headroom_bytes.value && details.vram_headroom_bytes.support!=pcinsight::Support::estimated)return 1;
        if(gpu.shared_memory_used_bytes.value)return 1; // never GTT relabeled as Windows shared
    }
#else
    if(snapshot.linux_gpu_inventory_source.value)return 1;
#endif
#ifdef __linux__
    for(const auto& gpu:snapshot.gpus)
        if(gpu.driver_date.value || gpu.driver_date.support!=pcinsight::Support::unsupported ||
            !gpu.driver_date.note.starts_with("Linux driver date unavailable:"))return 1;
#endif
    for(const auto& gpu:snapshot.gpus)if(gpu.linux_opengl_details.value)return 1; // opt-out, not a measured context
    pcinsight::LinuxOpenGlDetails gl_details;
    if(gl_details.context_version.value || gl_details.core_profile_supported.value || gl_details.reported_max_core_profile_version.value)return 1;
    if(pcinsight::app_history_view(snapshot).value) return 1; // disabled, not a measured empty capture
    pcinsight::Metric<unsigned> known_zero{0,pcinsight::Support::available,"fixture"};
    if(!pcinsight::metric_is_known(known_zero)) return 1;
    pcinsight::LegacyAppHistoryEntry legacy;legacy.id="fixture";
    const auto imported=pcinsight::import_legacy_history(std::span(&legacy,1));
    if(!imported.value||imported.value->front().network_bytes.value||imported.value->front().cpu_time.value) return 1;
    if(snapshot.cpu.vendor!=snapshot.cpu.reported_vendor.value.value_or("") || snapshot.cpu.model!=snapshot.cpu.reported_model.value.value_or("")) return 1;
    if(!snapshot.cpu.uptime.value || snapshot.cpu.uptime.value->count()<0) return 1;
    if(!snapshot.memory.reported_total_bytes.value || *snapshot.memory.reported_total_bytes.value!=snapshot.memory.total_bytes) return 1;
    if(snapshot.windows_run_registrations.value) return 1;
    if(snapshot.windows_app_history.value) return 1;
    if(snapshot.linux_app_history.value) return 1;
    if(snapshot.linux_session_inventory_source.value) return 1; // disabled by default, not a measured empty source
    if(snapshot.linux_service_inventory_source.value) return 1;
    if (snapshot.linux_drm.discovery.value || snapshot.linux_drm.aggregates.value) return 1;
    for (const auto& gpu : snapshot.gpus)
        if (gpu.direct3d12_feature_level.value && gpu.direct3d12_feature_level.value->empty()) return 1;
    for (const auto& gpu : snapshot.gpus) {
        const auto& b = gpu.local_process_memory;
        if (b.budget_bytes.value && (!b.process_id.value || !b.node_index.value ||
            !b.current_usage_bytes.value || !b.budget_headroom_bytes.value)) return 1;
    }
    pcinsight::Sampler sampler;
    pcinsight::SamplerSample timed_sample;
    if(timed_sample.started_at!=timed_sample.finished_at || sampler.latest_timed() ||
        !sampler.timed_history().empty() || !sampler.history().empty()) return 1;
    // Invalid identity cannot cause a mutation on any backend.
    auto actions = pcinsight::make_system_actions();
    // Invalid launch exercises installed headers and must never create a child.
    const auto invalid_launch = actions->launch_process({});
    if (invalid_launch || invalid_launch.process_id != 0) return 1;
    if (actions->terminate_process_checked(0, 0, true)) return 1;
#ifdef __linux__
    pcinsight::SampleOptions service_options;service_options.include_services=true;
    service_options.include_processes=service_options.include_disks=service_options.include_networks=service_options.include_gpus=false;
    const auto service_snapshot=monitor.sample(service_options);
    if(!pcinsight::metric_is_known(service_snapshot.linux_service_inventory_source))return 1;
    for(const auto& service:service_snapshot.services){
        if(!pcinsight::metric_is_known(service.linux_details))return 1;
        const auto& d=*service.linux_details.value;
        if(service.group!=d.slice.value.value_or(""))return 1;
        if(service.description!=d.description.value.value_or(""))return 1;
        if(service.pid&&(!pcinsight::metric_is_known(d.main_pid)||service.pid!=d.main_pid.value||!*service.pid))return 1;
        if(d.provider=="systemd-system-manager"&&d.group_kind!="systemd-slice")return 1;
    }
    pcinsight::SampleOptions session_options;session_options.include_user_sessions=true;
    session_options.include_processes=session_options.include_disks=session_options.include_networks=session_options.include_gpus=false;
    const auto session_snapshot=monitor.sample(session_options);
    if(!session_snapshot.processes.empty())return 1;
    if(!session_snapshot.linux_session_inventory_source.value&&!session_snapshot.user_sessions.empty())return 1;
    for(const auto& user:session_snapshot.user_sessions){
        if(!pcinsight::metric_is_known(user.linux_details))return 1;
        const auto& d=*user.linux_details.value;
        if(d.provider!=session_snapshot.linux_session_inventory_source.value)return 1;
        if(d.provider=="login1"&&user.status!=d.state.value.value_or("unknown"))return 1;
        if(d.provider=="utmp"&&(d.active.value||d.remote.value||d.unix_uid.value))return 1;
    }
    for(const auto& n:snapshot.networks){
        if(!n.linux_details.value || !n.linux_details.value->interface_index ||
            n.model!=n.linux_details.value->model.value.value_or("") ||
            n.connection_type!=n.linux_details.value->connection_type.value.value_or("")) return 1;
    }
    if(!snapshot.memory.linux_details.composition.value || !snapshot.memory.linux_details.zram_devices.value) return 1;
    {
        const auto& c=*snapshot.memory.linux_details.composition.value;
        if(c.unavailable_estimate_bytes+c.reclaimable_estimate_bytes+c.free_bytes!=snapshot.memory.reported_total_bytes.value ||
            c.unavailable_estimate_bytes!=snapshot.memory.in_use_bytes.value) return 1;
        if(snapshot.memory.linux_details.installed_minus_usable_bytes.value!=snapshot.memory.hardware_reserved_bytes.value) return 1;
        const auto& difference=snapshot.memory.linux_details.installed_minus_usable_bytes;
        if(difference.support!=snapshot.memory.hardware_reserved_bytes.support ||
            difference.note!=snapshot.memory.hardware_reserved_bytes.note) return 1;
        if(difference.value && (difference.support!=pcinsight::Support::estimated ||
            difference.note.find("NOT hardware-only")==std::string::npos)) return 1;
    }
    auto linux_history_options=defaults;linux_history_options.include_app_history=true;linux_history_options.include_processes=false;linux_history_options.include_linux_process_descriptions=false;
    const auto linux_history=monitor.sample(linux_history_options);
    if(linux_history.linux_network_app_history.value||pcinsight::app_history_view(linux_history.linux_network_app_history).value)return 1; // No explicit BPF capture, not a measured zero.
    if(!pcinsight::app_history_view(linux_history).value) return 1;
    if(!linux_history.processes.empty()||!linux_history.app_history.empty()||!linux_history.linux_app_history.value||linux_history.linux_app_history.support!=pcinsight::Support::estimated) return 1;
    for(const auto& entry:*linux_history.linux_app_history.value) {
        if(entry.id.empty()||entry.id.front()!='/'||entry.network_bytes.value||entry.notifications.value) return 1;
        if(entry.observed_cpu_time.value && (entry.observed_cpu_time.value->count()<0||entry.observed_cpu_time.support!=pcinsight::Support::estimated)) return 1;
    }
    if(snapshot.cpu.virtualization_enabled.value) return 1;
    for(const auto& process:snapshot.processes)
        if(process.efficiency_mode_available.value!=false ||
            process.efficiency_mode_available.support!=pcinsight::Support::available) return 1;
    if(snapshot.cpu.linux_kvm_api_version.value && (*snapshot.cpu.linux_kvm_api_version.value!=12 || snapshot.cpu.linux_kvm_api_version.support!=pcinsight::Support::available)) return 1;
    for(const auto& disk:snapshot.disks) {
        if(pcinsight::metric_is_known(disk.linux_details)) {
            const auto& report=disk.linux_details.value->media_type;
            if(disk.reported_media_type.value!=report.value || disk.reported_media_type.support!=report.support ||
                disk.reported_media_type.note!=report.note) return 1;
        }
        if(disk.reported_media_type.value && (!disk.linux_details.value ||
            disk.reported_media_type.value!=disk.linux_details.value->media_type.value ||
            disk.reported_media_type.support!=pcinsight::Support::estimated)) return 1;
        if(disk.rotational.value) {
            if(disk.media_type!=(*disk.rotational.value?"rotational":"non-rotational")) return 1;
        } else if(disk.media_type!="unknown") return 1;
    }
    if (pcinsight::list_windows_run_registrations().value) return 1;
    for (const auto& gpu : snapshot.gpus)
        if (gpu.pci_device_name.value && (gpu.pci_device_name.support != pcinsight::Support::estimated ||
            !gpu.hardware_vendor_id.value || !gpu.hardware_device_id.value)) return 1;
    for (const auto& process : snapshot.processes) {
        if (!process.linux_start_ticks.value || process.windows_creation_time_100ns.value) return 1;
        const auto& d=process.linux_details;
        if(process.name!=d.name.value.value_or("") || process.status!=d.status.value.value_or("") ||
            process.username!=d.username.value.value_or("") || process.executable!=d.executable.value.value_or("") ||
            process.architecture!=d.architecture.value.value_or("") || process.description!=d.description.value.value_or("")) return 1;
        if(d.description.value && (!d.description_source.value || d.description.support!=pcinsight::Support::estimated)) return 1;
        if(d.disk_rate_real_uid.value)return 1; // first Monitor sample has no I/O interval.
    }
    auto io_options=defaults;
    io_options.include_processes=false;io_options.include_user_sessions=true;
    io_options.include_disks=io_options.include_networks=io_options.include_gpus=false;
    io_options.include_linux_process_descriptions=false;
    const auto io_snapshot=monitor.sample(io_options);
    if(!io_snapshot.processes.empty())return 1;
    for(const auto& user:io_snapshot.user_sessions){
        if(!user.linux_io_details.value)return 1;
        const auto& io=*user.linux_io_details.value;
        if(io.scope!="linux-visible-processes-by-reported-real-uid"||io.network_bytes_per_second.value||user.network_bytes_per_second.value)return 1;
        if(user.disk_bytes_per_second.value!=io.disk_bytes_per_second.value)return 1;
        if(pcinsight::metric_is_known(io.real_uid)){
            if(!io.matching_processes.value||!io.disk_coverage.value)return 1;
            if(io.disk_coverage.value->known+io.disk_coverage.value->missing!=*io.matching_processes.value)return 1;
        }else if(io.matching_processes.value||io.disk_coverage.value||io.disk_bytes_per_second.value)return 1;
    }
#else
    for(const auto& n:snapshot.networks) if(n.linux_details.value) return 1;
    if(snapshot.memory.linux_details.composition.value || snapshot.memory.linux_details.zram_devices.value ||
        snapshot.memory.linux_details.zswap_pool_bytes.value || snapshot.memory.linux_details.firmware_installed_bytes.value) return 1;
    for(const auto& process:snapshot.processes) {
        const auto& d=process.linux_details;
        if(d.name.value||d.status.value||d.username.value||d.architecture.value||d.description.value||d.executable.value||d.description_source.value||d.real_uid.value||d.disk_rate_real_uid.value) return 1;
    }
    if(snapshot.cpu.linux_kvm_api_version.value) return 1;
    auto history_options=defaults;history_options.include_app_history=true;history_options.include_processes=false;
    const auto history=monitor.sample(history_options);
    if(!pcinsight::app_history_view(history).value) return 1;
    if(history.linux_app_history.value) return 1;
    if(!history.processes.empty() || !history.app_history.empty()) return 1;
    if(history.windows_app_history.value) {
        if(history.windows_app_history.support!=pcinsight::Support::estimated) return 1;
        for(const auto& app:*history.windows_app_history.value)
            if(!app.observed_cpu_time.value || app.network_bytes.value || app.notifications.value) return 1;
    }
    auto user_options=defaults;
    user_options.include_user_sessions=true;
    user_options.include_processes=false;
    const auto users=monitor.sample(user_options);
    if (!users.processes.empty()) return 1;
    for (const auto& user:users.user_sessions)
        if (user.id.empty() || user.username.empty() || user.resource_scope!="windows-visible-processes-by-wts-session-and-account" ||
            user.disk_bytes_per_second.value || user.network_bytes_per_second.value || user.linux_io_details.value || !user.logout_token.empty()) return 1;
    auto startup_options=defaults;
    startup_options.include_windows_managed_run=true;
    startup_options.include_startup_entries=true;
    startup_options.include_windows_startup_shortcut_metadata=true;
    startup_options.include_windows_startup_publisher=true;
    startup_options.include_windows_startup_signature=true;
    startup_options.include_processes=false;
    const auto startup=monitor.sample(startup_options);
    const auto& managed_run=startup.windows_run_registrations;
    if(managed_run.value && managed_run.support!=pcinsight::Support::estimated) return 1;
    if(managed_run.value) for(const auto& entry:*managed_run.value)
        if(!entry.entry_id.starts_with("winrun:hkcu:shared:")) return 1;
    for(const auto& entry:startup.startup_entries) {
        if(!entry.id.starts_with("winrun:"))continue;
        if(entry.id.starts_with("winrun:hkcu:shared:")) {
            const auto registration=pcinsight::read_windows_run_registration(entry.id);
            if(registration.value && (registration.support!=pcinsight::Support::estimated ||
                registration.value->entry_id!=entry.id)) return 1;
        }
        const auto properties=pcinsight::get_startup_properties(entry.id);
        if(properties && (properties.value->entry.id!=entry.id || properties.value->source_file_size_bytes.value ||
            properties.value->entry.command.empty() || properties.value->fields.size()!=6)) return 1;
        if(!properties && properties.error!=pcinsight::ActionError::not_found &&
            properties.error!=pcinsight::ActionError::stale_target && properties.error!=pcinsight::ActionError::permission_denied) return 1;
        break;
    }
    for(const auto& entry:startup.startup_entries) {
        if(!entry.id.starts_with("winfolder:"))continue;
        const auto properties=pcinsight::get_startup_properties(entry.id);
        if(properties && (properties.value->entry.id!=entry.id || !properties.value->source_file_size_bytes.value ||
            !properties.value->entry.command.empty() || properties.value->fields.size()!=6)) return 1;
        break;
    }
    for(const auto& entry:startup.startup_entries)
        if((!entry.id.starts_with("winrun:") && !entry.id.starts_with("winfolder:")) || entry.status!=pcinsight::StartupStatus::unknown ||
            entry.impact!=pcinsight::StartupImpact::not_measured) return 1;
    for(const auto& entry:startup.startup_entries)
        if(entry.windows_shortcut.value && entry.windows_shortcut.value->stored_url.value &&
            (!entry.id.starts_with("winfolder:") || !entry.command.empty() ||
             entry.windows_shortcut.value->stored_local_target.value)) return 1;
    for(const auto& entry:startup.startup_entries)
        if(!entry.publisher.empty() && (entry.publisher_label.value!=entry.publisher ||
            entry.publisher_label.support!=pcinsight::Support::estimated || !entry.publisher_source_executable.value)) return 1;
    for (const auto& process : snapshot.processes)
        if (process.linux_start_ticks.value) return 1;
    for(const auto& entry:startup.startup_entries) {
        if(!entry.windows_signature.value) continue;
        const auto& signature=*entry.windows_signature.value;
        const bool accepted=signature.status==pcinsight::WindowsSignatureStatus::trusted_local_policy;
        if(!signature.native_status.value || accepted!=(*signature.native_status.value==0) ||
            (signature.signer_name.value && !accepted) || signature.source_executable.empty()) return 1;
    }
    for (const auto& gpu : snapshot.gpus)
        if (!gpu.dedicated_memory_bytes.value || !gpu.shared_memory_bytes.value ||
            !gpu.dedicated_system_memory_bytes.value || !gpu.hardware_vendor_id.value || !gpu.hardware_device_id.value) return 1;
    for (const auto& gpu : snapshot.gpus) {
        if (gpu.memory_used_bytes.value != gpu.dedicated_memory_used_bytes.value) return 1;
        if (gpu.memory_available_bytes.value && (gpu.hardware_vendor_id.support != pcinsight::Support::available || gpu.hardware_vendor_id.value != 0x10deU || gpu.memory_available_bytes.support != pcinsight::Support::available || !gpu.memory_available_bytes.note.starts_with("NVML v1 unallocated framebuffer bytes"))) return 1;
    }
    for (const auto& gpu : snapshot.gpus)
        for (const auto& engine : gpu.engines)
            if (engine.id.empty()) return 1;
#endif
    if (actions->set_efficiency_mode_checked(0, 0, true)) return 1;
    if (actions->set_efficiency_mode(0, true)) return 1;
    if (actions->disconnect_user("pcinsight-invalid-session")) return 1;
    if (actions->terminate_user_session("pcinsight-invalid-session")) return 1;
    if (actions->set_startup_enabled_for_current_user("", false)) return 1;
    if (actions->reset_startup_for_current_user("")) return 1;
    if (pcinsight::get_startup_properties("")) return 1;
    if (pcinsight::read_windows_run_registration("").value) return 1;
    if (pcinsight::set_windows_run_registration_enabled({},false)) return 1;
    if (pcinsight::read_startup_process_sample(0,0)) return 1;
    if (pcinsight::read_startup_scheduler_process_sample(0,0)) return 1;
    if (pcinsight::read_startup_child_exit_sample(0,0)) return 1;
    if (pcinsight::StartupImpactStore::load({})) return 1;
    if (pcinsight::StartupImpactStore::save({},{})) return 1;
    if (pcinsight::WindowsHistoryStore::save({},{})) return 1;
    if (pcinsight::WindowsHistoryStore::load({})) return 1;
    if (!pcinsight::WindowsHistoryStore::merge({},{}).empty()) return 1;
    pcinsight::StartupImpactObserver startup_observer;
    startup_observer.close();
    if (startup_observer.finish_process({})) return 1;
    if (startup_observer.finish_partial_process({})) return 1;
    if (startup_observer.poll().attempted != 0) return 1;
    if (startup_observer.poll(std::chrono::steady_clock::now()).attempted != 0) return 1;
    if (!pcinsight::discover_startup_process_candidates({}).candidates.empty()) return 1;
    auto startup_view=snapshot;
    if (!pcinsight::apply_startup_impact_observations(startup_view,{} ,"consumer-test-context")) return 1;
    unsigned audit_events = 0;
    pcinsight::AuditedActions audited(std::move(actions), [&](const pcinsight::AuditEvent&) { ++audit_events; });
    if (audited.terminate_process_checked(0, 0, true)) return 1;
    if (audit_events != 2 || audited.diagnostics().failed_request_records != 0) return 1;
    if (audited.set_windows_run_registration_enabled({},false)) return 1;
    if (audit_events != 4 || audited.diagnostics().failed_request_records != 0) return 1;
    if (audited.set_windows_startup_file_registered({},false)) return 1;
    if (audit_events != 6 || audited.diagnostics().failed_request_records != 0) return 1;
    return snapshot.captured_at.time_since_epoch().count() > 0 &&
        sampler.last_error().empty() ? 0 : 1;
}
