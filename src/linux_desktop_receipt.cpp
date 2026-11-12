#include "linux_desktop_receipt.hpp"
#ifdef __linux__
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <unistd.h>
#endif
namespace pcinsight::detail {
namespace {
template<class T> Metric<T> absent(std::string n,Support s=Support::unsupported){return {{},s,std::move(n)};}
bool same(const PrivatePidfdNetworkJoin& a,const PrivatePidfdNetworkJoin& b){return a.local==b.local&&a.kernel.kernel_tgid==b.kernel.kernel_tgid&&a.kernel.kernel_real_uid==b.kernel.kernel_real_uid&&a.kernel.leader_start_boottime_ns==b.kernel.leader_start_boottime_ns;}
bool absolute(const std::string& p){return !p.empty()&&p.front()=='/'&&p.find('\0')==p.npos;}
bool valid_route(const LinuxDesktopRoute& r){
    const auto& q=r.request;
    if(!absolute(r.entry_id)||r.entry_id.size()>32768||!r.entry_id.ends_with(".desktop")||!absolute(q.executable)||q.executable.find('=')!=q.executable.npos||q.elevated||(!q.working_directory.empty()&&!absolute(q.working_directory))||q.arguments.size()>4096)return false;
    std::size_t bytes=q.executable.size()+q.working_directory.size();if(bytes>1048576)return false;
    for(const auto& a:q.arguments){if(a.find('\0')!=a.npos||a.size()>1048576-bytes)return false;bytes+=a.size();}return true;
}
bool support_valid(Support s){return s==Support::available||s==Support::estimated||s==Support::unsupported||s==Support::permission_required;}
}
Metric<LinuxDesktopLaunchReceipt> project_linux_manual_receipt(const LinuxDesktopRoute& route,const PrivatePidfdNetworkJoin& selected,SelectedExecFile pin,std::uint32_t pid,ActionError error,const Metric<SelectedExecRead>& read){
    if(!valid_route(route)||(!selected.local.boot[0]&&!selected.local.boot[1])||!valid_network_self(selected.local.observer)||!valid_network_self(selected.local.process)||!selected.kernel.kernel_tgid||selected.kernel.kernel_tgid>2147483647U||!selected.kernel.leader_start_boottime_ns||pin.device_major>4095||pin.device_minor>1048575||static_cast<unsigned>(error)>static_cast<unsigned>(ActionError::stale_target)|| (pid&&pid!=selected.local.process.local_pid)||(error==ActionError::none&&!pid))return absent<LinuxDesktopLaunchReceipt>("Invalid manual receipt route/key/pin/outcome");
    if(!read.value)return absent<LinuxDesktopLaunchReceipt>(read.note,read.support==Support::permission_required?read.support:Support::unsupported);
    if((read.support!=Support::available&&read.support!=Support::estimated)||!same(selected,read.value->selected))return absent<LinuxDesktopLaunchReceipt>("Receipt source support or authenticated identity changed");
    const auto& e=read.value->evidence;
    if(e.quality_flags&~64ULL||!support_valid(e.first_exec.support)||!support_valid(e.pinned_file_matches.support)||(e.publishing&&(e.first_exec.value||e.pinned_file_matches.value))||(!e.first_exec.value&&e.first_exec.support==Support::available)||(!e.pinned_file_matches.value&&e.pinned_file_matches.support==Support::available)||(e.first_exec.value&&e.first_exec.support!=Support::available&&e.first_exec.support!=Support::estimated)||(e.pinned_file_matches.value&&e.pinned_file_matches.support!=Support::available&&e.pinned_file_matches.support!=Support::estimated))return absent<LinuxDesktopLaunchReceipt>("Invalid receipt exec quality/publication");
    Metric<bool> link{{},Support::unsupported,"No verified first exec match; manual action/exit/EOF is not positive evidence"};
    if(e.first_exec.value){const auto& x=*e.first_exec.value;
        if(!x.observed_monotonic_ns||x.device_major>4095||x.device_minor>1048575||x.kernel_real_uid!=selected.kernel.kernel_real_uid||!pid||(error!=ActionError::none&&error!=ActionError::system_error))return absent<LinuxDesktopLaunchReceipt>("Contradictory first exec metadata or cancelled launch");
        const bool matches=SelectedExecFile{x.inode,x.device_major,x.device_minor}==pin;
        if(e.pinned_file_matches.value&&*e.pinned_file_matches.value!=matches)return absent<LinuxDesktopLaunchReceipt>("First exec/pin comparison contradicts receipt");
        if(e.first_exec.support==Support::available&&e.pinned_file_matches.support==Support::available&&e.pinned_file_matches.value&&read.support==Support::available&&!e.quality_flags&&!e.publishing)
            link={matches,Support::available,"This manual desktop snapshot's controlled child first exec vs pinned object; not automatic startup/app/content/current-image proof"};
    }else if(e.pinned_file_matches.value)return absent<LinuxDesktopLaunchReceipt>("File match without first exec evidence");
    const auto support=read.support==Support::estimated||e.quality_flags||e.publishing?Support::estimated:Support::available;
    return {LinuxDesktopLaunchReceipt{LinuxDesktopLaunchOrigin::manual_request,route,selected,pin,pid,error,e,std::move(link),read.value->pidfd_exited},support,"Historical controlled MANUAL launch receipt, no automatic origin"};
}
void LinuxDesktopLaunchSession::stop() noexcept {capture_.stop();route_.reset();selected_.reset();pinned_={};}
ActionResult LinuxDesktopLaunchSession::start(const std::string& id,bool launch_consent,bool capture_consent){
    if(!launch_consent||!capture_consent)return {ActionError::permission_denied,"Separate explicit launch and selected-capture consent required"};
    if(attempted_)return {ActionError::stale_target,"One-shot launch session already attempted; create another session"};
    attempted_=true;
    try{
        auto result=launch_linux_desktop_entry(id,true,[&](const LinuxDesktopRoute& route,const LinuxGatedLaunchTarget& target){
            // All allocating preparation happens while the child is still gated.
            route_=route;
            const auto joined=capture_.start(true,target);if(!joined.value)return ActionResult{joined.support==Support::permission_required?ActionError::permission_denied:ActionError::not_supported,joined.note};
#ifdef __linux__
            struct stat st{};if(::fstat(target.borrowed_executable_fd,&st)!=0)return ActionResult{ActionError::stale_target,"Cannot inspect pinned receipt executable"};
            pinned_={static_cast<std::uint64_t>(st.st_ino),major(st.st_dev),minor(st.st_dev)};
#endif
            selected_=*joined.value;return ActionResult{};
        });
        // No allocations or hidden sampling after release: PID ownership is never lost.
        error_=result.error;caller_pid_=result.process_id;if(!selected_)stop();return result;
    }catch(...){stop();throw;}
}
Metric<LinuxDesktopLaunchReceipt> LinuxDesktopLaunchSession::sample(){
    if(!route_||!selected_)return absent<LinuxDesktopLaunchReceipt>("Manual launch session has no active authenticated capture");
    try{const auto read=capture_.sample();const auto receipt=project_linux_manual_receipt(*route_,*selected_,pinned_,caller_pid_,error_,read);if(!receipt.value)stop();return receipt;}catch(...){stop();throw;}
}
}
