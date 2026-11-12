#include "linux_startup_resources.hpp"
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
template<class T> Metric<T> absent(std::string n,Support s=Support::unsupported){return {{},s,std::move(n)};}
template<class T> bool exact(const Metric<T>& m){return m.value&&m.support==Support::available;}
bool valid(PrivateNetworkIdentity k){return k.kernel_tgid&&k.kernel_tgid<=2147483647U&&k.leader_start_boottime_ns;}
}
Metric<std::vector<NetworkProbeInstruction>> make_startup_resource_probe(int fd,const LinuxStartupResourceBtfLayout& l,PrivateNetworkIdentity expected){
    if(std::endian::native!=std::endian::little||fd<0||!valid(expected)||!l.function_id)return absent<std::vector<NetworkProbeInstruction>>("Invalid resource schema/identity/map");
    for(auto offset:{l.tgid,l.leader,l.start,l.cred,l.uid,l.signal,l.nr_threads,l.live,l.task_runtime,l.signal_runtime,l.task_read,l.task_write,l.signal_read,l.signal_write})
        if(offset>1048576)return absent<std::vector<NetworkProbeInstruction>>("Resource offset exceeds bound");
    std::vector<NetworkProbeInstruction> c;std::vector<std::size_t> exits,failures;
    auto op=[&](unsigned code,unsigned d,unsigned s,int offset,int imm){c.push_back({static_cast<std::uint8_t>(code),static_cast<std::uint8_t>(d|(s<<4)),static_cast<std::int16_t>(offset),imm});};
    auto jump=[&](auto& list,unsigned code,unsigned d,int imm){list.push_back(c.size());op(code,d,0,0,imm);};
    auto compare=[&](auto& list,unsigned code,unsigned d,unsigned s){list.push_back(c.size());op(code,d,s,0,0);};
    auto constant=[&](unsigned d,std::uint64_t v){op(0x18,d,0,0,static_cast<std::int32_t>(v));op(0,0,0,0,static_cast<std::int32_t>(v>>32));};
    auto read=[&](unsigned base,unsigned offset,unsigned bytes,auto& errors){
        op(0x7a,10,0,-16,0);op(0xbf,3,base,0,0);op(0x07,3,0,0,static_cast<int>(offset));
        op(0xbf,1,10,0,0);op(0x07,1,0,0,-16);op(0xb7,2,0,0,static_cast<int>(bytes));op(0x85,0,0,0,113);
        jump(errors,0x55,0,0);op(bytes==8?0x79:0x61,0,10,-16,0);
    };
    auto lookup=[&](unsigned key){op(0x62,10,0,-4,key);op(0xbf,2,10,0,0);op(0x07,2,0,0,-4);op(0x18,1,1,0,fd);op(0,0,0,0,0);op(0x85,0,0,0,1);};
    // This hook executes in the REAPER. Identity comes from its target argument,
    // never get_current_pid_tgid/get_current_uid_gid.
    op(0x79,6,1,0,0);read(6,l.tgid,4,exits);jump(exits,0x55,0,static_cast<int>(expected.kernel_tgid));
    read(6,l.leader,8,exits);compare(exits,0x5d,0,6); // Direct leader invocation only.
    read(6,l.start,8,exits);constant(9,expected.leader_start_boottime_ns);compare(exits,0x5d,0,9);
    read(6,l.cred,8,exits);jump(exits,0x15,0,0);op(0xbf,7,0,0,0);
    read(7,l.uid,4,exits);constant(9,expected.kernel_real_uid);compare(exits,0x5d,0,9);
    read(6,l.signal,8,failures);jump(failures,0x15,0,0);op(0xbf,7,0,0,0);
    read(7,l.live,4,failures);jump(exits,0x55,0,0);
    read(7,l.nr_threads,4,failures);jump(exits,0x55,0,1);
    // No live threads and only the leader remains unaccounted in signal.
    // Add each component ONCE, with overflow detection before publication.
    const std::array<std::array<unsigned,3>,3> fields{{
        {l.task_runtime,l.signal_runtime,24},{l.task_read,l.signal_read,32},{l.task_write,l.signal_write,40}}};
    for(const auto& f:fields){
        read(6,f[0],8,failures);op(0xbf,9,0,0,0);read(7,f[1],8,failures);op(0x0f,9,0,0,0);
        compare(failures,0xad,9,0);op(0x7b,10,9,-static_cast<int>(f[2]),0); // unsigned sum < operand
    }
    read(7,l.live,4,failures);jump(failures,0x55,0,0);
    read(7,l.nr_threads,4,failures);jump(failures,0x55,0,1);
    lookup(0);jump(exits,0x15,0,0);op(0xbf,8,0,0,0);op(0x79,9,8,0,0);jump(failures,0x55,9,0);
    const std::array<std::uint64_t,3> ids{expected.kernel_tgid,expected.kernel_real_uid,expected.leader_start_boottime_ns};
    for(unsigned n=0;n<3;++n){lookup(n+2);jump(failures,0x15,0,0);constant(9,ids[n]);op(0x7b,0,9,0,0);}
    op(0x85,0,0,0,5);op(0xbf,9,0,0,0);lookup(5);jump(failures,0x15,0,0);op(0x7b,0,9,0,0);
    for(unsigned n=0;n<3;++n){lookup(n+6);jump(failures,0x15,0,0);op(0x79,9,10,-static_cast<int>(24+8*n),0);op(0x7b,0,9,0,0);}
    op(0xb7,9,0,0,1);op(0xdb,8,9,0,0xe1);
    const auto success=c.size();op(0xb7,0,0,0,0);op(0x95,0,0,0,0);
    const auto failure=c.size();lookup(1);const auto missing=c.size();op(0x15,0,0,0,0);op(0xb7,9,0,0,1);op(0xdb,0,9,0,0x40);
    const auto end=c.size();op(0xb7,0,0,0,0);op(0x95,0,0,0,0);
    c[missing].offset=static_cast<std::int16_t>(end-missing-1);
    for(auto n:exits)c[n].offset=static_cast<std::int16_t>(success-n-1);
    for(auto n:failures)c[n].offset=static_cast<std::int16_t>(failure-n-1);
    return {std::move(c),Support::available,"Selected direct leader-release group accounting; internal release loops not covered"};
}
Metric<std::optional<StartupFinalResources>> decode_startup_resources(const std::array<std::uint64_t,9>& v,PrivateNetworkIdentity k){
    if(!valid(k)||v[0]>1||v[1])return absent<std::optional<StartupFinalResources>>("Resource state/read/overflow/duplicate failure");
    if(!v[0])return {std::optional<StartupFinalResources>{},Support::available,"No direct leader-release record; may await reaping or be outside hook coverage"};
    if(v[2]!=k.kernel_tgid||v[3]!=k.kernel_real_uid||v[4]!=k.leader_start_boottime_ns||!v[5])return absent<std::optional<StartupFinalResources>>("Resource identity/time mismatch");
    return {StartupFinalResources{k,v[5],v[6],v[7],v[8]},Support::available,"Scheduler-runtime ns (not procfs CPU); group IO includes waited children; zero is valid"};
}
void LinuxStartupResourceCapture::stop() noexcept {
#ifdef __linux__
    for(int* f:{&link_,&program_,&map_,&pidfd_})if(*f>=0){::close(*f);*f=-1;}
#endif
    selected_.reset();creator_=euid_=0;
}
Metric<PrivatePidfdNetworkJoin> LinuxStartupResourceCapture::start(bool authorized,int borrowed,std::uint32_t pid,std::uint64_t ticks){
    if(!authorized)return absent<PrivatePidfdNetworkJoin>("Explicit selected resource-capture authorization required",Support::permission_required);
    if(selected_)return absent<PrivatePidfdNetworkJoin>("Resource capture already active; stop first");
#ifdef __linux__
    auto fail=[&](std::string note,Support s=Support::unsupported){stop();return absent<PrivatePidfdNetworkJoin>(std::move(note),s);};
    try{
        creator_=static_cast<std::uint32_t>(::getpid());euid_=static_cast<std::uint32_t>(::geteuid());
        pidfd_=::fcntl(borrowed,F_DUPFD_CLOEXEC,3);if(pidfd_<0)return fail("Cannot own selected pidfd");
        const auto blob=read_linux_kernel_btf_blob();if(!exact(blob))return fail(blob.note,blob.support);
        const auto layout=resolve_linux_network_btf(*blob.value,sizeof(void*));const auto schema=resolve_linux_startup_resource_btf(*blob.value,sizeof(void*));
        if(!exact(layout))return fail(layout.note,layout.support);
        if(!exact(schema))return fail(schema.note,schema.support);
        const auto selected=read_private_pidfd_network_join(true,pidfd_,pid,ticks,&*layout.value);if(!exact(selected))return fail(selected.note,selected.support);
        const auto api=native_network_session_api();const auto map=api.map(api.context,9);if(map.fd<0)return fail("Resource map creation failed",map.error==EPERM||map.error==EACCES?Support::permission_required:Support::unsupported);map_=map.fd;
        const auto code=make_startup_resource_probe(map_,*schema.value,selected.value->kernel);if(!exact(code))return fail(code.note);
        auto pointer=[](const void* p){return static_cast<std::uint64_t>(reinterpret_cast<std::uintptr_t>(p));};
        const char license[]="GPL";union bpf_attr load{};load.prog_type=BPF_PROG_TYPE_TRACING;load.expected_attach_type=BPF_TRACE_FENTRY;load.attach_btf_id=schema.value->function_id;
        load.insn_cnt=static_cast<__u32>(code.value->size());load.insns=pointer(code.value->data());load.license=pointer(license);
        program_=static_cast<int>(::syscall(SYS_bpf,BPF_PROG_LOAD,&load,sizeof(load)));if(program_<0)return fail("Resource-entry program load failed errno="+std::to_string(errno),errno==EPERM||errno==EACCES?Support::permission_required:Support::unsupported);
        union bpf_attr attach{};attach.link_create.prog_fd=program_;attach.link_create.attach_type=BPF_TRACE_FENTRY;
        link_=static_cast<int>(::syscall(SYS_bpf,BPF_LINK_CREATE,&attach,sizeof(attach)));if(link_<0)return fail("Resource-entry link attach failed errno="+std::to_string(errno));
        const auto after=read_private_pidfd_network_join(true,pidfd_,pid,ticks,&*layout.value);
        if(!exact(after)||after.value->local!=selected.value->local||after.value->kernel.kernel_tgid!=selected.value->kernel.kernel_tgid||after.value->kernel.kernel_real_uid!=selected.value->kernel.kernel_real_uid||after.value->kernel.leader_start_boottime_ns!=selected.value->kernel.leader_start_boottime_ns||creator_!=static_cast<std::uint32_t>(::getpid())||euid_!=static_cast<std::uint32_t>(::geteuid()))return fail("Selected identity changed while attaching resource capture");
        selected_=*selected.value;return selected;
    }catch(...){stop();throw;}
#else
    (void)borrowed;(void)pid;(void)ticks;return absent<PrivatePidfdNetworkJoin>("Linux resource capture unavailable");
#endif
}
Metric<StartupResourceRead> LinuxStartupResourceCapture::sample(){
    if(!selected_)return absent<StartupResourceRead>("Resource capture inactive");
#ifdef __linux__
    auto fail=[&](std::string note){stop();return absent<StartupResourceRead>(std::move(note));};
    if(creator_!=static_cast<std::uint32_t>(::getpid())||euid_!=static_cast<std::uint32_t>(::geteuid()))return fail("Fork/caller/euid changed; inherited references closed without map writes");
    try{
        const auto source=native_network_selection_source_api();const auto context=source.observe(source.context);
        if(!exact(context)||context.value->boot!=selected_->local.boot||context.value->observer!=selected_->local.observer)return fail("Resource observer context/boot changed");
        pollfd p{pidfd_,POLLIN,0};const auto rc=::poll(&p,1,0);if(rc<0||p.revents&~(POLLIN|POLLHUP))return fail("Selected pidfd readiness unavailable");
        const bool exited=rc>0;
        const auto api=native_network_session_api();std::array<std::uint64_t,9> raw{};
        const auto before=api.lookup(api.context,map_,0);if(!exact(before))return fail("Resource state read failed");
        for(unsigned n=1;n<9;++n){const auto v=api.lookup(api.context,map_,n);if(!exact(v))return fail("Resource data read failed");raw[n]=*v.value;}
        const auto after=api.lookup(api.context,map_,0);if(!exact(after))return fail("Resource state recheck failed");
        std::atomic_thread_fence(std::memory_order_seq_cst);
        if(*before.value!=*after.value){return absent<StartupResourceRead>("Resource publication crossed this read; retry without baseline mutation");}
        raw[0]=*after.value;const auto decoded=decode_startup_resources(raw,selected_->kernel);if(!exact(decoded))return fail(decoded.note);
        const auto final=source.observe(source.context);
        if(!exact(final)||*final.value!=*context.value||creator_!=static_cast<std::uint32_t>(::getpid())||euid_!=static_cast<std::uint32_t>(::geteuid()))return fail("Resource observer changed during event read");
        return {StartupResourceRead{*selected_,*decoded.value,exited},Support::available,"Direct leader-release accounting only; missing resources after pidfd exit are unknown, not zero/failure. No procfs CPU-baseline mixing or exclusive IO claim"};
    }catch(...){stop();throw;}
#else
    return absent<StartupResourceRead>("Linux resource capture unavailable");
#endif
}
}

