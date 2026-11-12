#include "pcinsight/monitor.hpp"
#include "backend.hpp"
#include "linux_drm_monitor.hpp"
#include "pcinsight/startup.hpp"
#include "windows_startup_file_monitor.hpp"
#include "linux_process_network_monitor.hpp"
#include "linux_network_admission_monitor.hpp"
#include "linux_network_app_history.hpp"

namespace pcinsight::detail {
void fill_windows_startup_file_backups(SystemSnapshot& snapshot,const SampleOptions& options,const WindowsStartupFileBackupProvider& provider){
    if(!options.include_windows_startup_file_backups){
        snapshot.windows_startup_file_backups={{},Support::unsupported,"Startup file backup listing disabled; enable include_windows_startup_file_backups explicitly"};
    }else if(!provider){
        snapshot.windows_startup_file_backups={{},Support::unsupported,"Startup file backup provider unavailable"};
    }else snapshot.windows_startup_file_backups=provider();
}
}

namespace pcinsight {
struct Monitor::Impl { std::unique_ptr<detail::Backend> backend{detail::make_backend()}; detail::LinuxDrmMonitor drm; detail::LinuxProcessNetworkMonitor process_network;
    std::unique_ptr<detail::LinuxVisibleNetworkMonitor> visible{std::make_unique<detail::LinuxVisibleNetworkMonitor>()};
    detail::LinuxNetworkAppHistory network_history; };
Monitor::Monitor() : impl_(std::make_unique<Impl>()) {}
Monitor::~Monitor() = default;
Monitor::Monitor(Monitor&&) noexcept = default;
Monitor& Monitor::operator=(Monitor&&) noexcept = default;
SystemSnapshot Monitor::sample(const SampleOptions& options) {
    if(!impl_)impl_=std::make_unique<Impl>();
    if(!options.include_linux_process_network)stop_linux_process_network();
    try {
        auto snapshot=impl_->backend->sample(options);
        impl_->drm.fill(snapshot,options);
#ifndef _WIN32
        snapshot.windows_app_history.note="Windows-only CPU history report; Linux history is in linux_app_history";
#else
        snapshot.linux_app_history.note="Linux-only observed CPU history report";
#endif
        if(options.include_windows_managed_run)
            snapshot.windows_run_registrations=list_windows_run_registrations();
        else
            snapshot.windows_run_registrations={{},Support::unsupported,"PCInsight Run/backup listing disabled; enable include_windows_managed_run explicitly"};
        detail::fill_windows_startup_file_backups(snapshot,options);
        impl_->network_history.prepare(options.include_app_history);
        if(impl_->visible->configured())impl_->visible->fill(snapshot,options);
        else{impl_->process_network.fill(snapshot,options);snapshot.linux_network_admission={{},Support::unsupported,"No visible admission route; manual selection/flag alone is not inventory coverage"};}
        impl_->network_history.finish(snapshot,options);
        if((options.include_user_sessions||options.include_linux_user_network)&&options.include_linux_process_network)
            snapshot.linux_user_network=summarize_linux_user_network(snapshot.linux_process_network);
        else snapshot.linux_user_network={{},Support::unsupported,"Linux user network disabled; enable include_linux_user_network (or user-session report) and authorized process-network report explicitly"};
        return snapshot;
    }catch(...){impl_->drm.reset();stop_linux_process_network();throw;}
}
Metric<std::uint64_t> Monitor::start_linux_process_network(bool authorized,std::span<const LinuxProcessNetworkSelection> selection){
    return start_linux_process_network(authorized,selection,LinuxProcessNetworkSelectionMode::strict_single_namespace);
}
Metric<std::uint64_t> Monitor::start_linux_process_network(bool authorized,std::span<const LinuxProcessNetworkSelection> selection,LinuxProcessNetworkSelectionMode mode){
    if(!impl_)impl_=std::make_unique<Impl>();
    if(impl_->visible->configured())return {{},authorized?Support::unsupported:Support::permission_required,"Stop visible admission before starting a manual selection; fresh consent required"};
    return impl_->process_network.start(authorized,selection,mode);
}
void Monitor::stop_linux_process_network() noexcept {if(impl_){impl_->process_network.stop();impl_->visible->stop();impl_->network_history.break_continuity();}}
Metric<std::uint64_t> Monitor::update_linux_process_network_selection(bool authorized,std::span<const LinuxProcessNetworkSelection> selection){
    if(!authorized)return {{},Support::permission_required,"Explicit authorization required for each Monitor selection update"};
    if(!impl_)return {{},Support::unsupported,"Moved-from Monitor cannot update capture; explicit start required"};
    if(impl_->visible->configured())return {{},Support::unsupported,"Manual selection update cannot replace visible admission; stop first"};
    return impl_->process_network.update_selection(true,selection);
}
Metric<LinuxNetworkAdmissionChange> Monitor::refresh_linux_visible_process_network(bool authorized,LinuxNetworkAdmissionPolicy policy){
    if(!authorized)return {{},Support::permission_required,"Explicit authorization required for EACH Monitor inventory admission refresh"};
    if(!impl_)impl_=std::make_unique<Impl>();
    if(impl_->process_network.active())return {{},Support::unsupported,"Stop manual selection before visible admission setup"};
    return impl_->visible->refresh(true,policy);
}
}
namespace pcinsight::detail {
Monitor MonitorAdmissionAccess::make(VisibleNetworkMonitorApi api){Monitor result;result.impl_->visible=std::make_unique<LinuxVisibleNetworkMonitor>(api);return result;}
}
