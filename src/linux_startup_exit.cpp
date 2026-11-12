#include "linux_startup_exit.hpp"
#include "linux_network_selection.hpp"
#include <atomic>
#include <bit>
#ifdef __linux__
#include <cerrno>
#include <fcntl.h>
#include <linux/bpf.h>
#include <poll.h>
#include <sys/syscall.h>
#include <unistd.h>
#endif
namespace pcinsight::detail {
namespace {
template<class T> Metric<T> absent(std::string note,Support support=Support::unsupported){return {{},support,std::move(note)};}
template<class T> bool exact(const Metric<T>& m){return m.value&&m.support==Support::available;}
bool valid(PrivateNetworkIdentity k){return k.kernel_tgid&&k.kernel_tgid<=2147483647U&&k.leader_start_boottime_ns;}
}
Metric<std::vector<NetworkProbeInstruction>> make_startup_selected_exit_probe(int map_fd,const LinuxNetworkBtfLayout& l,PrivateNetworkIdentity expected){
    if(std::endian::native!=std::endian::little||map_fd<0||!valid(expected)||l.task_group_leader_offset>1048576||l.task_start_boottime_offset>1048576)return absent<std::vector<NetworkProbeInstruction>>("Invalid selected exit schema/identity/map");
    std::vector<NetworkProbeInstruction> c;std::vector<std::size_t> exits,failures;
    auto op=[&](unsigned code,unsigned d,unsigned s,int offset,int imm){c.push_back({static_cast<std::uint8_t>(code),static_cast<std::uint8_t>(d|(s<<4)),static_cast<std::int16_t>(offset),imm});};
    auto jump=[&](auto& list,unsigned code,unsigned d,int imm){list.push_back(c.size());op(code,d,0,0,imm);};
    auto constant=[&](unsigned d,std::uint64_t v){op(0x18,d,0,0,static_cast<std::int32_t>(v));op(0,0,0,0,static_cast<std::int32_t>(v>>32));};
    auto lookup=[&](unsigned key){op(0x62,10,0,-4,key);op(0xbf,2,10,0,0);op(0x07,2,0,0,-4);op(0x18,1,1,0,map_fd);op(0,0,0,0,0);op(0x85,0,0,0,1);};
    op(0xbf,6,1,0,0);op(0x85,0,0,0,14);op(0x77,0,0,0,32);
    jump(exits,0x55,0,static_cast<int>(expected.kernel_tgid)); // BEFORE context/map access.
    op(0x79,9,6,8,0);op(0x67,9,0,0,32);op(0xc7,9,0,0,32);
    jump(exits,0x15,9,0);jump(failures,0x55,9,1); // Last live thread only, not leader-only exit.
    op(0x85,0,0,0,35);op(0xbf,8,0,0,0);op(0x79,9,6,0,0);
    failures.push_back(c.size());op(0x5d,8,9,0,0); // taskstats argument must be current.
    for(const auto offset:{l.task_group_leader_offset,l.task_start_boottime_offset}){
        op(0x7a,10,0,-16,0);op(0xbf,3,8,0,0);op(0x07,3,0,0,static_cast<int>(offset));
        op(0xbf,1,10,0,0);op(0x07,1,0,0,-16);op(0xb7,2,0,0,8);op(0x85,0,0,0,113);jump(failures,0x55,0,0);
        op(0x79,8,10,-16,0);jump(failures,0x15,8,0);
    }
    constant(9,expected.leader_start_boottime_ns);failures.push_back(c.size());op(0x5d,8,9,0,0);
    op(0x85,0,0,0,15);op(0xbc,0,0,0,0);constant(9,expected.kernel_real_uid);failures.push_back(c.size());op(0x5d,0,9,0,0);
    lookup(0);jump(exits,0x15,0,0);op(0xbf,7,0,0,0);op(0x79,9,7,0,0);jump(failures,0x55,9,0);
    // One writer: only group_dead=1. Publish state LAST via atomic exchange.
    const std::array<std::uint64_t,4> values{1,expected.kernel_tgid,expected.kernel_real_uid,expected.leader_start_boottime_ns};
    for(unsigned index=0;index<4;++index){lookup(index+2);jump(failures,0x15,0,0);constant(9,values[index]);op(0x7b,0,9,0,0);}
    op(0x85,0,0,0,5);op(0xbf,9,0,0,0);lookup(6);jump(failures,0x15,0,0);op(0x7b,0,9,0,0);
    op(0xb7,9,0,0,1);op(0xdb,7,9,0,0xe1); // BPF_XCHG|BPF_FETCH, full publication barrier.
    const auto success=c.size();op(0xb7,0,0,0,0);op(0x95,0,0,0,0);
    const auto failure=c.size();lookup(1);const auto missing=c.size();op(0x15,0,0,0,0);op(0xb7,9,0,0,1);op(0xdb,0,9,0,0x40); // sticky OR flag, cannot wrap.
    const auto end=c.size();op(0xb7,0,0,0,0);op(0x95,0,0,0,0);
    c[missing].offset=static_cast<std::int16_t>(end-missing-1);
    for(const auto index:exits)c[index].offset=static_cast<std::int16_t>(success-index-1);
    for(const auto index:failures)c[index].offset=static_cast<std::int16_t>(failure-index-1);
    return {std::move(c),Support::available,"Selected TGID/UID/start filtered last-thread exit-entry event, no counters/content/global subscription"};
}
Metric<std::optional<StartupSelectedExitEvent>> decode_startup_selected_exit(const std::array<std::uint64_t,7>& v,PrivateNetworkIdentity expected){
    if(!valid(expected)||v[0]>1||v[1])return absent<std::optional<StartupSelectedExitEvent>>("Exit event identity/read/duplicate failure");
    if(!v[0])return {std::optional<StartupSelectedExitEvent>{},Support::available,"No selected group-exit event published yet; not proof process is alive"};
    if(v[2]!=1||v[3]!=expected.kernel_tgid||v[4]!=expected.kernel_real_uid||v[5]!=expected.leader_start_boottime_ns||!v[6])return absent<std::optional<StartupSelectedExitEvent>>("Invalid selected group-exit event");
    return {StartupSelectedExitEvent{expected,v[6]},Support::available,"Observed taskstats_exit(group_dead=1) entry; no CPU/disk/exit-code/app-origin claim"};
}
void LinuxStartupSelectedExitCapture::stop() noexcept {
#ifdef __linux__
    for(int* f:{&link_,&program_,&map_,&pidfd_})if(*f>=0){::close(*f);*f=-1;}
#endif
    selected_.reset();creator_=euid_=0;
}
Metric<PrivatePidfdNetworkJoin> LinuxStartupSelectedExitCapture::start(bool authorized,int borrowed,std::uint32_t pid,std::uint64_t ticks){
    if(!authorized)return absent<PrivatePidfdNetworkJoin>("Explicit selected exit-capture authorization required",Support::permission_required);
    if(selected_)return absent<PrivatePidfdNetworkJoin>("Exit capture already active; stop first");
#ifdef __linux__
    auto fail=[&](std::string note,Support s=Support::unsupported){stop();return absent<PrivatePidfdNetworkJoin>(std::move(note),s);};
    try{
        creator_=static_cast<std::uint32_t>(::getpid());euid_=static_cast<std::uint32_t>(::geteuid());
        pidfd_=::fcntl(borrowed,F_DUPFD_CLOEXEC,3);if(pidfd_<0)return fail("Cannot own selected pidfd");
        const auto blob=read_linux_kernel_btf_blob();if(!exact(blob))return fail(blob.note,blob.support);
        const auto layout=resolve_linux_network_btf(*blob.value,sizeof(void*));const auto hook=resolve_linux_taskstats_exit_hook(*blob.value);
        if(!exact(layout))return fail(layout.note,layout.support);
        if(!exact(hook))return fail(hook.note,hook.support);
        const auto selected=read_private_pidfd_network_join(true,pidfd_,pid,ticks,&*layout.value);if(!exact(selected))return fail(selected.note,selected.support);
        const auto api=native_network_session_api();const auto map=api.map(api.context,7);if(map.fd<0)return fail("Exit map creation failed",map.error==EPERM||map.error==EACCES?Support::permission_required:Support::unsupported);map_=map.fd;
        const auto code=make_startup_selected_exit_probe(map_,*layout.value,selected.value->kernel);if(!exact(code))return fail(code.note);
        auto pointer=[](const void* p){return static_cast<std::uint64_t>(reinterpret_cast<std::uintptr_t>(p));};
        const char license[]="GPL";union bpf_attr load{};load.prog_type=BPF_PROG_TYPE_TRACING;load.expected_attach_type=BPF_TRACE_FENTRY;load.attach_btf_id=*hook.value;
        load.insn_cnt=static_cast<__u32>(code.value->size());load.insns=pointer(code.value->data());load.license=pointer(license);
        program_=static_cast<int>(::syscall(SYS_bpf,BPF_PROG_LOAD,&load,sizeof(load)));if(program_<0)return fail("Exit-entry program load failed errno="+std::to_string(errno),errno==EPERM||errno==EACCES?Support::permission_required:Support::unsupported);
        union bpf_attr attach{};attach.link_create.prog_fd=program_;attach.link_create.attach_type=BPF_TRACE_FENTRY;
        link_=static_cast<int>(::syscall(SYS_bpf,BPF_LINK_CREATE,&attach,sizeof(attach)));if(link_<0)return fail("Exit-entry link attach failed errno="+std::to_string(errno));
        const auto after=read_private_pidfd_network_join(true,pidfd_,pid,ticks,&*layout.value);
        if(!exact(after)||after.value->local!=selected.value->local||after.value->kernel.kernel_tgid!=selected.value->kernel.kernel_tgid||after.value->kernel.kernel_real_uid!=selected.value->kernel.kernel_real_uid||after.value->kernel.leader_start_boottime_ns!=selected.value->kernel.leader_start_boottime_ns||creator_!=static_cast<std::uint32_t>(::getpid())||euid_!=static_cast<std::uint32_t>(::geteuid()))return fail("Selected identity changed while attaching exit capture");
        selected_=*selected.value;return selected;
    }catch(...){stop();throw;}
#else
    (void)borrowed;(void)pid;(void)ticks;return absent<PrivatePidfdNetworkJoin>("Linux exit capture unavailable");
#endif
}
Metric<StartupSelectedExitRead> LinuxStartupSelectedExitCapture::sample(){
    if(!selected_)return absent<StartupSelectedExitRead>("Exit capture inactive");
#ifdef __linux__
    auto fail=[&](std::string note){stop();return absent<StartupSelectedExitRead>(std::move(note));};
    if(creator_!=static_cast<std::uint32_t>(::getpid())||euid_!=static_cast<std::uint32_t>(::geteuid()))return fail("Fork/caller/euid changed; inherited references closed without map writes");
    try{
        const auto source=native_network_selection_source_api();const auto context=source.observe(source.context);
        if(!exact(context)||context.value->boot!=selected_->local.boot||context.value->observer!=selected_->local.observer)return fail("Exit observer context/boot changed");
        pollfd p{pidfd_,POLLIN,0};const auto rc=::poll(&p,1,0);if(rc<0||p.revents&~(POLLIN|POLLHUP))return fail("Selected pidfd readiness unavailable");
        const bool exited=rc>0;
        const auto api=native_network_session_api();std::array<std::uint64_t,7> raw{};
        const auto before=api.lookup(api.context,map_,0);if(!exact(before))return fail("Exit state read failed");
        for(unsigned n=1;n<7;++n){const auto v=api.lookup(api.context,map_,n);if(!exact(v))return fail("Exit data read failed");raw[n]=*v.value;}
        const auto after=api.lookup(api.context,map_,0);if(!exact(after))return fail("Exit state recheck failed");
        std::atomic_thread_fence(std::memory_order_seq_cst);
        if(*before.value!=*after.value){return absent<StartupSelectedExitRead>("Exit publication crossed this read; retry without baseline mutation");}
        raw[0]=*after.value;const auto decoded=decode_startup_selected_exit(raw,selected_->kernel);if(!exact(decoded))return fail(decoded.note);
        if(exited&&!decoded.value->has_value())return fail("Pidfd exited without a captured event; no invented event/counters");
        const auto final=source.observe(source.context);
        if(!exact(final)||*final.value!=*context.value||creator_!=static_cast<std::uint32_t>(::getpid())||euid_!=static_cast<std::uint32_t>(::geteuid()))return fail("Exit observer changed during event read");
        return {StartupSelectedExitRead{*selected_,*decoded.value,exited},Support::available,"Selected lifecycle event retained after reaping; event entry and pidfd completion are separate, no final resource totals"};
    }catch(...){stop();throw;}
#else
    return absent<StartupSelectedExitRead>("Linux exit capture unavailable");
#endif
}
}
