#include "linux_selected_exec.hpp"
#include "linux_startup_birth.hpp"
#include <atomic>
#ifdef __linux__
#include <cerrno>
#include <fcntl.h>
#include <linux/bpf.h>
#include <poll.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <sys/syscall.h>
#include <unistd.h>
#endif
namespace pcinsight::detail {
namespace {
template<class T> Metric<T> absent(std::string note,Support s=Support::unsupported){return {{},s,std::move(note)};}
template<class T> bool exact(const Metric<T>& m){return m.value&&m.support==Support::available;}
bool key_valid(PrivateNetworkIdentity k){return k.kernel_tgid&&k.kernel_tgid<=2147483647U&&k.leader_start_boottime_ns;}
#ifdef __linux__
bool same(PrivateNetworkIdentity a,PrivateNetworkIdentity b){return a.kernel_tgid==b.kernel_tgid&&a.kernel_real_uid==b.kernel_real_uid&&a.leader_start_boottime_ns==b.leader_start_boottime_ns;}
#endif
}
Metric<SelectedExecEvidence> decode_selected_exec(const SelectedExecRaw& r,PrivateNetworkIdentity k,SelectedExecFile file,std::uint64_t flags){
    if(!key_valid(k)||file.device_major>4095||file.device_minor>1048575||flags&~64ULL||r[0]!=1||r[1]!=k.kernel_tgid||r[2]!=k.kernel_real_uid||r[3]!=k.leader_start_boottime_ns||r[15]>3)
        return absent<SelectedExecEvidence>("Invalid selected-exec arm/key/flags/state");
    for(unsigned n=4;n<15;++n)if(r[n])return absent<SelectedExecEvidence>("Selected-exec unused fields are not birth times/counters");
    SelectedExecEvidence out;out.quality_flags=flags;
    out.first_exec={{},Support::unsupported,"No first successful exec evidence; pending/uncovered is not false/zero"};
    out.pinned_file_matches={{},Support::unsupported,"No successful exec tuple to compare with pinned object"};
    if(r[15]==2||(r[15]==3&&!(flags&64))){out.publishing=true;return {out,Support::estimated,"First exec publication/failure flag in flight; resample"};}
    if(r[15]==0){for(unsigned n=16;n<20;++n)if(r[n])return absent<SelectedExecEvidence>("Unpublished selected exec has data");}
    if(r[15]==1){
        if(!r[16]||r[18]>UINT32_MAX||r[19]>UINT32_MAX||r[19]!=k.kernel_real_uid)return absent<SelectedExecEvidence>("Selected exec time/device/real UID changed or invalid");
        StartupBirthObservation::Executable e{r[16],r[17],static_cast<std::uint32_t>(r[18]>>20),static_cast<std::uint32_t>(r[18]&1048575U),static_cast<std::uint32_t>(r[19])};
        out.first_exec={e,Support::available,"First successful kernel exec of authenticated selected group; historical filesystem metadata, not current image"};
        out.pinned_file_matches={SelectedExecFile{e.inode,e.device_major,e.device_minor}==file,Support::available,"Observed first-exec tuple vs still-pinned launch object; NOT content/hash/app/automatic-startup proof"};
    }else if(r[15]==3)out.first_exec.note="First exec event read failed; later exec cannot replace it";
    const auto support=flags?Support::estimated:Support::available;
    return {std::move(out),support,"Selected first-exec evidence only, never birth/CPU/app/cause inference"};
}
void LinuxSelectedExecCapture::stop() noexcept {
#ifdef __linux__
    for(auto p:{&link_,&program_,&allowlist_,&records_,&control_,&pidfd_,&image_})if(*p>=0){::close(*p);*p=-1;}
#endif
    selected_.reset();retained_.reset();owner_=euid_=0;expected_={};
}
Metric<PrivatePidfdNetworkJoin> LinuxSelectedExecCapture::start(bool authorized,const LinuxGatedLaunchTarget& target){
    if(!authorized)return absent<PrivatePidfdNetworkJoin>("Explicit selected-exec authorization required",Support::permission_required);
    if(selected_||target.child.local_pid==target.owner.observer.local_pid)return absent<PrivatePidfdNetworkJoin>("Selected-exec capture active or target is not a distinct gated child");
#ifdef __linux__
    auto fail=[&](std::string note,Support s=Support::unsupported){stop();return absent<PrivatePidfdNetworkJoin>(std::move(note),s);};
    try{
        owner_=static_cast<std::uint32_t>(::getpid());euid_=static_cast<std::uint32_t>(::geteuid());
        pidfd_=::fcntl(target.borrowed_pidfd,F_DUPFD_CLOEXEC,3);image_=::fcntl(target.borrowed_executable_fd,F_DUPFD_CLOEXEC,3);
        if(pidfd_<0||image_<0)return fail("Cannot duplicate gated pidfd/executable pin");
        struct stat st{};if(::fstat(image_,&st)!=0||!S_ISREG(st.st_mode)||static_cast<std::uint64_t>(st.st_dev)!=target.executable.device||static_cast<std::uint64_t>(st.st_ino)!=target.executable.inode||major(st.st_dev)>4095||minor(st.st_dev)>1048575)return fail("Gated executable pin changed/invalid");
        expected_={static_cast<std::uint64_t>(st.st_ino),major(st.st_dev),minor(st.st_dev)};
        const auto context=native_network_selection_source_api();const auto before=context.observe(context.context);
        if(!exact(before)||*before.value!=target.owner)return fail("Gated selected-exec caller context differs");
        const auto blob=read_linux_kernel_btf_blob();if(!exact(blob))return fail(blob.note,blob.support);
        const auto network=resolve_linux_network_btf(*blob.value,sizeof(void*));const auto image_layout=resolve_linux_birth_exec_btf(*blob.value,sizeof(void*));
        if(!exact(network))return fail(network.note,network.support);
        if(!exact(image_layout))return fail(image_layout.note,image_layout.support);
        const auto selected=read_private_pidfd_network_join(true,pidfd_,target.child.local_pid,target.child.start_ticks,&*network.value);
        if(!exact(selected))return fail("Gated selected-exec pidfd authentication failed: "+selected.note,selected.support);
        if(selected.value->local.process!=target.child||selected.value->local.observer!=target.owner.observer||selected.value->local.boot!=target.owner.boot)return fail("Gated selected-exec authenticated context differs");
        auto pointer=[](const void* p){return static_cast<std::uint64_t>(reinterpret_cast<std::uintptr_t>(p));};
        auto map=[&](unsigned type,unsigned count,unsigned size){union bpf_attr a{};a.map_type=type;a.key_size=4;a.value_size=size;a.max_entries=count;return static_cast<int>(::syscall(SYS_bpf,BPF_MAP_CREATE,&a,sizeof a));};
        auto update=[&](int fd,std::uint32_t key,const void* value){union bpf_attr a{};a.map_fd=fd;a.key=pointer(&key);a.value=pointer(value);return ::syscall(SYS_bpf,BPF_MAP_UPDATE_ELEM,&a,sizeof a)==0;};
        control_=map(BPF_MAP_TYPE_ARRAY,2,8);records_=map(BPF_MAP_TYPE_ARRAY,1,sizeof(SelectedExecRaw));allowlist_=map(BPF_MAP_TYPE_HASH,1,8);
        if(control_<0||records_<0||allowlist_<0)return fail("Selected-exec maps unavailable",errno==EACCES||errno==EPERM?Support::permission_required:Support::unsupported);
        SelectedExecRaw raw{};raw[0]=1;raw[1]=selected.value->kernel.kernel_tgid;raw[2]=selected.value->kernel.kernel_real_uid;raw[3]=selected.value->kernel.leader_start_boottime_ns;
        if(!update(records_,0,raw.data()))return fail("Cannot seed selected-exec ARM identity");
        // Reuse the existing exec-only ABI/generator, not a birth decoder/sensor.
        // Words4..14 remain UNUSED ZERO. No birth time or CPU baseline invented.
        LinuxStartupResourceBtfLayout layout{};layout.leader=network.value->task_group_leader_offset;layout.start=network.value->task_start_boottime_offset;
        const auto code=make_startup_birth_exec_probe(control_,records_,allowlist_,layout,*image_layout.value,1);if(!exact(code))return fail(code.note);
        const char license[]="GPL";union bpf_attr load{};load.prog_type=BPF_PROG_TYPE_RAW_TRACEPOINT;load.insn_cnt=static_cast<__u32>(code.value->size());load.insns=pointer(code.value->data());load.license=pointer(license);
        program_=static_cast<int>(::syscall(SYS_bpf,BPF_PROG_LOAD,&load,sizeof load));if(program_<0)return fail("Selected-exec program load errno="+std::to_string(errno),errno==EACCES||errno==EPERM?Support::permission_required:Support::unsupported);
        const char hook[]="sched_process_exec";union bpf_attr attach{};attach.raw_tracepoint.name=pointer(hook);attach.raw_tracepoint.prog_fd=program_;
        link_=static_cast<int>(::syscall(SYS_bpf,BPF_RAW_TRACEPOINT_OPEN,&attach,sizeof attach));if(link_<0)return fail("Selected-exec attach errno="+std::to_string(errno));
        const auto after=read_private_pidfd_network_join(true,pidfd_,target.child.local_pid,target.child.start_ticks,&*network.value);
        const auto owner=context.observe(context.context);
        if(!exact(after)||after.value->local!=selected.value->local||!same(after.value->kernel,selected.value->kernel)||!exact(owner)||*owner.value!=target.owner||owner_!=static_cast<std::uint32_t>(::getpid())||euid_!=static_cast<std::uint32_t>(::geteuid()))return fail("Selected-exec target/caller changed while attaching");
        // Enrollment LAST; arming callback must keep child behind the exec gate.
        const std::uint64_t slot=0;if(!update(allowlist_,selected.value->kernel.kernel_tgid,&slot))return fail("Selected-exec enrollment failed");
        selected_=*selected.value;return selected;
    }catch(...){stop();throw;}
#else
    (void)target;return absent<PrivatePidfdNetworkJoin>("Selected exec capture requires Linux");
#endif
}
Metric<SelectedExecRead> LinuxSelectedExecCapture::sample(){
    if(!selected_)return absent<SelectedExecRead>("Selected-exec capture inactive");
#ifdef __linux__
    auto fail=[&](std::string n){stop();return absent<SelectedExecRead>(std::move(n));};
    if(owner_!=static_cast<std::uint32_t>(::getpid())||euid_!=static_cast<std::uint32_t>(::geteuid()))return fail("Selected-exec caller/fork/euid changed; inherited references closed");
    try{
        const auto api=native_network_selection_source_api();const auto before=api.observe(api.context);
        if(!exact(before)||before.value->boot!=selected_->local.boot||before.value->observer!=selected_->local.observer)return fail("Selected-exec owner context changed");
        auto get=[&](int fd,std::uint32_t key,void* value){union bpf_attr a{};a.map_fd=fd;a.key=static_cast<__u64>(reinterpret_cast<std::uintptr_t>(&key));a.value=static_cast<__u64>(reinterpret_cast<std::uintptr_t>(value));return ::syscall(SYS_bpf,BPF_MAP_LOOKUP_ELEM,&a,sizeof a)==0;};
        std::uint64_t flags{},last_flags{};SelectedExecRaw a{},b{};
        if(!get(control_,1,&flags)||!get(records_,0,a.data()))return fail("Selected-exec record read failed");
        std::atomic_thread_fence(std::memory_order_seq_cst);
        if(!get(records_,0,b.data())||!get(control_,1,&last_flags))return fail("Selected-exec record recheck failed");
        flags|=last_flags;
        if(a[15]!=b[15]||b[15]==2){b[15]=2;} // Decoder returns only publishing, never partial event fields.
        const auto decoded=decode_selected_exec(b,selected_->kernel,expected_,flags);if(!decoded.value)return fail(decoded.note);
        if(retained_&&(!decoded.value->first_exec.value||*decoded.value->first_exec.value!=*retained_))return fail("Selected first exec rewrote/disappeared");
        const auto after=api.observe(api.context);if(!exact(after)||*after.value!=*before.value||owner_!=static_cast<std::uint32_t>(::getpid())||euid_!=static_cast<std::uint32_t>(::geteuid()))return fail("Selected-exec context changed during sample");
        pollfd p{pidfd_,POLLIN,0};const auto rc=::poll(&p,1,0);if(rc<0||p.revents&~(POLLIN|POLLHUP))return fail("Selected-exec pidfd exit guard failed");
        if(decoded.value->first_exec.value)retained_=*decoded.value->first_exec.value;
        return {SelectedExecRead{*selected_,*decoded.value,rc>0},decoded.support,"Selected first successful exec, no current-image/app/startup-cause inference"};
    }catch(...){stop();throw;}
#else
    return absent<SelectedExecRead>("Selected exec capture requires Linux");
#endif
}
}
