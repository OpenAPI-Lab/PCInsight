#include "linux_desktop_resources.hpp"
#include "startup_birth_archive.hpp"
#ifdef __linux__
#include <sys/syscall.h>
#include <unistd.h>
#endif
namespace pcinsight::detail {
namespace {
template<class T> Metric<T> absent(std::string note,Support support=Support::unsupported){return {{},support,std::move(note)};}
bool same_file(const StartupBirthObservation::Executable& a,const StartupBirthObservation::Executable& b){return a.inode==b.inode&&a.device_major==b.device_major&&a.device_minor==b.device_minor&&a.kernel_real_uid==b.kernel_real_uid;}
}
Metric<LinuxManualLaunchResources> measure_linux_manual_launch_resources(const Metric<LinuxDesktopLaunchReceipt>& receipt,const StartupBirthObserverReport& births,std::chrono::nanoseconds window,const StartupImpactPolicy& policy){
    if(!receipt.value||receipt.support!=Support::available)return absent<LinuxManualLaunchResources>("Exact confirmed manual receipt required");
    const auto& r=*receipt.value;
    const Metric<SelectedExecRead> source{SelectedExecRead{r.selected,r.evidence,r.pidfd_exited},Support::available,"receipt consistency recheck"};
    const auto checked=project_linux_manual_receipt(r.route,r.selected,r.pinned_file,r.caller_owned_pid,r.launch_error,source);
    if(r.origin!=LinuxDesktopLaunchOrigin::manual_request||!checked.value||checked.value->entry_first_exec_matches.value!=true||checked.value->entry_first_exec_matches.support!=Support::available||r.entry_first_exec_matches.value!=true||r.entry_first_exec_matches.support!=Support::available)return absent<LinuxManualLaunchResources>("Receipt is not a confirmed controlled manual entry match");
    const auto invalid=validate_startup_birth_archive(StartupBirthArchive{births,window,policy});
    if(!invalid.empty())return absent<LinuxManualLaunchResources>(invalid);
    if(births.state!=StartupBirthObserverState::closed||births.error!=ActionError::none||births.quality_flags||births.publishing||!births.first_exec_enabled||!births.final_resources_enabled||!births.pid_mapping_enabled||births.boot!=r.selected.local.boot||births.pid_namespace_device!=r.selected.local.observer.pid_namespace_device||births.pid_namespace_inode!=r.selected.local.observer.pid_namespace_inode)return absent<LinuxManualLaunchResources>("Birth source quality/boot/observer namespace/mode not confirmed");
    const auto& kernel=r.selected.kernel;const StartupBirthProcessKey key{births.capture_id,births.boot,kernel.kernel_tgid,kernel.kernel_real_uid,kernel.leader_start_boottime_ns};
    const StartupBirthObservation* row=nullptr;
    for(const auto& candidate:births.observations)if(candidate.key==key){if(row)return absent<LinuxManualLaunchResources>("Ambiguous retained group");row=&candidate;}
    if(!row)return absent<LinuxManualLaunchResources>("Exact boot/kernel TGID/UID/start-boottime group absent; never join by PID alone");
    const auto& local=r.selected.local.process;
    if(!row->local_pid.value||row->local_pid.support!=Support::available||row->local_pid.value->tgid!=local.local_pid||row->local_pid.value->namespace_device!=local.pid_namespace_device||row->local_pid.value->namespace_inode!=local.pid_namespace_inode||!row->first_executable.value||row->first_executable.support!=Support::available||!same_file(*row->first_executable.value,*r.evidence.first_exec.value))return absent<LinuxManualLaunchResources>("Namespace PID or independent first-exec file metadata differs");
    const auto time=r.evidence.first_exec.value->observed_monotonic_ns;
    if(time<row->birth_monotonic_ns||(row->final_monotonic_ns&&time>*row->final_monotonic_ns))return absent<LinuxManualLaunchResources>("Receipt first exec lies outside retained lifetime");
    if(row->startup_entry_id.value&&*row->startup_entry_id.value!=r.route.entry_id)return absent<LinuxManualLaunchResources>("Conflicting declared entry binding; source not overwritten");
    // Stage only the chosen row's declaration in a COPY, not in observer/recorder.
    auto staged=births;for(auto& item:staged.observations)if(item.key==key)item.startup_entry_id={r.route.entry_id,Support::estimated,"Controlled manual receipt route, not automatic origin/app identity"};
    const auto measured=measure_startup_birth_impact(staged,key,window,policy);if(!measured)return absent<LinuxManualLaunchResources>(measured.message);
    return {LinuxManualLaunchResources{LinuxDesktopLaunchOrigin::manual_request,*measured.value,{true,Support::available,"Same authenticated group and independently observed first-exec file; MANUAL route only"}},Support::estimated,"Single manual launch group resource lower bounds; includes pre-exec overhead/waited-child IO, no tree sums/automatic startup proof"};
}
void LinuxDesktopResourceSession::stop() noexcept {births_.close();launch_.stop();active_=false;}
ActionResult LinuxDesktopResourceSession::start(const std::string& id,bool launch_consent,bool capture_consent,const std::string& capture_id){
    if(!launch_consent||!capture_consent)return {ActionError::permission_denied,"Explicit launch and selected-creator/exec capture consents required"};
    if(attempted_)return {ActionError::stale_target,"Resource session is one-shot"};
    attempted_=true;
#ifdef __linux__
    try{
        const auto self=read_linux_network_self();if(!self.value||self.support!=Support::available)return {ActionError::not_supported,self.note};
        struct Fd{int value;~Fd(){if(value>=0)::close(value);}} fd{static_cast<int>(::syscall(SYS_pidfd_open,::getpid(),0))};
        if(fd.value<0)return {ActionError::not_supported,"Cannot pin resource-session creator"};
        StartupBirthObserverOptions options{capture_id,1,true,true};
        const auto armed=births_.start(true,fd.value,self.value->local_pid,self.value->start_ticks,options);if(!armed)return armed;
        active_=true;
        return launch_.start(id,true,true); // PID ownership returned unchanged, no post-release allocation.
    }catch(...){stop();throw;}
#else
    (void)id;(void)capture_id;return {ActionError::not_supported,"Controlled resources require Linux"};
#endif
}
Metric<LinuxDesktopResourceReport> LinuxDesktopResourceSession::finish(std::chrono::nanoseconds window,const StartupImpactPolicy& policy){
    if(!active_)return absent<LinuxDesktopResourceReport>("Resource session inactive/finished");
    try{
        (void)births_.poll();births_.close();auto births=births_.report();
        auto receipt=launch_.sample();launch_.stop();active_=false;
        auto resources=measure_linux_manual_launch_resources(receipt,births,window,policy);
        return {LinuxDesktopResourceReport{std::move(receipt),std::move(births),std::move(resources)},Support::available,"Explicit historical finalization, not automatic-startup or complete coverage"};
    }catch(...){stop();throw;}
}
}
