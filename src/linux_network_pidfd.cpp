#include "linux_network_pidfd.hpp"
#include "linux_network_session.hpp"
#include <bit>
#include <cerrno>
#include <utility>
#ifdef __linux__
#include <fcntl.h>
#include <poll.h>
#include <pthread.h>
#include <signal.h>
#include <sys/syscall.h>
#include <unistd.h>
#endif
namespace pcinsight::detail {
Metric<std::vector<NetworkProbeInstruction>> make_private_pidfd_identity_probe(std::uint64_t caller,int map,const LinuxNetworkBtfLayout& l,bool gated){
    if(std::endian::native!=std::endian::little||!(caller>>32)||static_cast<std::uint32_t>(caller)==0||
        (caller>>32)>2147483647||static_cast<std::uint32_t>(caller)>2147483647||map<0||!l.pidfd_identity.value||l.pidfd_identity.support!=Support::available)
        return {{},Support::unsupported,"Exact caller/map/pidfd BTF required"};
    const auto& p=*l.pidfd_identity.value;
    if(!p.function_id||p.task_tgid_offset>1048576||p.task_cred_offset>1048576||p.cred_uid_offset>1048576||l.task_group_leader_offset>1048576||l.task_start_boottime_offset>1048576)
        return {{},Support::unsupported,"Invalid pidfd identity offsets/hook"};
    std::vector<NetworkProbeInstruction> code;std::vector<std::size_t> exits,errors;
    auto add=[&](unsigned op,unsigned d,unsigned s,int off,int imm){code.push_back({static_cast<std::uint8_t>(op),static_cast<std::uint8_t>(d|(s<<4)),static_cast<std::int16_t>(off),imm});};
    auto constant=[&](unsigned reg,std::uint64_t value){add(0x18,reg,0,0,static_cast<std::int32_t>(value));add(0,0,0,0,static_cast<std::int32_t>(value>>32));};
    auto jump=[&](auto& list,unsigned op,unsigned d,unsigned s,int imm){list.push_back(code.size());add(op,d,s,0,imm);};
    auto read=[&](unsigned size,std::uint32_t offset){add(0x7a,10,0,-16,0);add(0xbf,3,8,0,0);add(0x07,3,0,0,static_cast<int>(offset));
        add(0xbf,1,10,0,0);add(0x07,1,0,0,-16);add(0xb7,2,0,0,size);add(0x85,0,0,0,113);jump(errors,0x55,0,0,0);};
    auto lookup=[&](int key){add(0x62,10,0,-4,key);add(0xbf,2,10,0,0);add(0x07,2,0,0,-4);add(0x18,1,1,0,map);add(0,0,0,0,0);add(0x85,0,0,0,1);jump(exits,0x15,0,0,0);};
    add(0xbf,6,1,0,0);add(0x85,0,0,0,14);
    if(gated){add(0xbf,7,0,0,0);lookup(5);add(0x79,9,0,0,0);jump(exits,0x5d,7,9,0);}
    else{constant(9,caller);jump(exits,0x5d,0,9,0);} // BEFORE any selected context
    add(0x79,0,6,8,0);add(0xbc,0,0,0,0);jump(exits,0x55,0,0,0x12); // PTRACE_MODE_ATTACH_REALCREDS
    add(0x79,0,6,16,0);add(0xbc,0,0,0,0);jump(exits,0x55,0,0,0); // permission check succeeded
    // Read pointer-valued argument through probe_read_kernel so task equality
    // is a scalar check, not verifier-forbidden ptr arithmetic.
    add(0xbf,8,6,0,0);read(8,0);add(0x79,8,10,-16,0);jump(errors,0x15,8,0,0);add(0xbf,7,8,0,0);
    read(8,l.task_group_leader_offset);add(0x79,8,10,-16,0);jump(errors,0x15,8,0,0);jump(errors,0x5d,8,7,0); // leader only
    read(4,p.task_tgid_offset);add(0x61,9,10,-16,0);add(0x7b,10,9,-24,0);
    read(8,l.task_start_boottime_offset);add(0x79,9,10,-16,0);add(0x7b,10,9,-32,0);
    read(8,p.task_cred_offset);add(0x79,8,10,-16,0);jump(errors,0x15,8,0,0);
    read(4,p.cred_uid_offset);add(0x61,9,10,-16,0);add(0x7b,10,9,-40,0);
    for(const auto& pair:{std::pair{0,-24},std::pair{1,-40},std::pair{2,-32}}){lookup(pair.first);add(0x79,9,10,pair.second,0);add(0x7b,0,9,0,0);}
    lookup(3);add(0xb7,9,0,0,1);add(0xdb,0,9,0,0);const auto success_exit=code.size();add(0x05,0,0,0,0);
    const auto failure=code.size();lookup(4);add(0xb7,9,0,0,1);add(0xdb,0,9,0,0);
    const auto finish=code.size();add(0xb7,0,0,0,0);add(0x95,0,0,0,0);
    code[success_exit].offset=static_cast<std::int16_t>(finish-success_exit-1);
    for(auto at:exits)code[at].offset=static_cast<std::int16_t>(finish-at-1);
    for(auto at:errors)code[at].offset=static_cast<std::int16_t>(failure-at-1);
    return {std::move(code),Support::available,"Single caller-thread pidfd invalid-FD lookup identity probe; no enumeration"};
}
namespace {
template<class T> Metric<T> fail(int e,const char* note){return {{},e==EPERM||e==EACCES?Support::permission_required:Support::unsupported,std::string(note)+" errno="+std::to_string(e)};}
}
Metric<PrivateNetworkIdentity> validate_private_pidfd_identity_values(const std::array<std::uint64_t,5>& value){
    if(!value[0]||value[0]>2147483647||value[1]>UINT32_MAX||!value[2]||value[3]!=1||value[4])return fail<PrivateNetworkIdentity>(EINVAL,"Absent/ambiguous/failed pidfd identity result");
    return {PrivateNetworkIdentity{static_cast<std::uint32_t>(value[0]),static_cast<std::uint32_t>(value[1]),value[2]},Support::available,
        "Kernel identity observed through selected pidfd invalid-file lookup; not procfs/app join or stable credentials guarantee"};
}
void LinuxPidfdIdentityQuery::close(int& fd) noexcept {if(fd>=0){const int n=fd;fd=-1;api_.close(api_.context,n);}}
bool LinuxPidfdIdentityQuery::disarm() noexcept {
    if(!armed_)return true;
    armed_=false;
    // A forked copy must NEVER write the parent's shared map.
    if(guard_(guard_context_)!=creator_){close(link_);return false;}
    try{if(!api_.update(api_.context,result_map_,5,0))return true;}catch(...){}
    close(link_);close(identity_program_);return false; // Fail closed on inability to disable.
}
void LinuxPidfdIdentityQuery::cancel() noexcept {(void)disarm();}
void LinuxPidfdIdentityQuery::stop() noexcept {cancel();close(link_);close(identity_program_);close(thread_program_);close(caller_program_);close(result_map_);close(caller_map_);layout_.reset();creator_=caller_=0;armed_=false;}
Metric<bool> LinuxPidfdIdentityQuery::start(bool authorized,const LinuxNetworkBtfLayout& layout){
    if(!authorized)return fail<bool>(EPERM,"Reusable pidfd query requires explicit authorization");
    if(layout_)return fail<bool>(EBUSY,"Reusable pidfd query already started");
    if(!api_||!api_.update||!guard_)return fail<bool>(ENOSYS,"Reusable pidfd query API incomplete");
    const auto checked=make_private_pidfd_identity_probe((1ULL<<32)|1,0,layout);
    if(!checked.value)return {{},checked.support,checked.note};
    try{
        creator_=guard_(guard_context_);if(!creator_)return fail<bool>(EINVAL,"Reusable query caller guard absent");
        auto map=api_.map(api_.context,3);caller_map_=map.fd;if(caller_map_<0){stop();return fail<bool>(map.error,"Reusable caller map");}
        map=api_.map(api_.context,6);result_map_=map.fd;if(result_map_<0){stop();return fail<bool>(map.error,"Reusable result map");}
        const auto code=make_private_network_identity_probe(caller_map_,layout);if(!code.value){stop();return {{},code.support,code.note};}
        auto loaded=api_.load(api_.context,*code.value,0,true);caller_program_=loaded.fd;
        if(caller_program_<0){stop();return fail<bool>(loaded.error,"Reusable caller program");}
        const std::array<NetworkProbeInstruction,2> thread_code{{{0x85,0,0,14},{0x95,0,0,0}}};
        loaded=api_.load(api_.context,thread_code,0,true);thread_program_=loaded.fd;
        if(thread_program_<0){stop();return fail<bool>(loaded.error,"Reusable thread program");}
        layout_=layout;return {true,Support::available,"Reusable loaded pidfd query; no link attached"};
    }catch(...){stop();throw;}
}
Metric<bool> LinuxPidfdIdentityQuery::begin(){
    if(!layout_)return fail<bool>(ENODEV,"Reusable pidfd query stopped");
    if(guard_(guard_context_)!=creator_){stop();return fail<bool>(ECHILD,"Forked reusable pidfd query rejected");}
    if(armed_)return fail<bool>(EBUSY,"Reusable pidfd query transaction already active");
    try{
        // The gate is disabled while clearing either map. Failed resets never
        // permit old successful values to masquerade as a fresh observation.
        for(std::uint32_t k=0;k<3;++k){const auto e=api_.update(api_.context,caller_map_,k,0);if(e)return fail<bool>(e,"Reset reusable caller map");}
        auto ran=api_.run(api_.context,caller_program_);if(ran.error||ran.returned)return fail<bool>(ran.error?ran.error:EINVAL,"Reusable caller TEST_RUN");
        std::array<std::uint64_t,3> c{};
        for(std::uint32_t k=0;k<3;++k){const auto v=api_.lookup(api_.context,caller_map_,k);if(!v.value||v.support!=Support::available)return {{},v.support,v.note};c[k]=*v.value;}
        if(!c[0]||c[0]>2147483647U||c[1]>UINT32_MAX||!c[2])return fail<bool>(EINVAL,"Reusable current caller identity invalid");
        ran=api_.run(api_.context,thread_program_);if(ran.error||!ran.returned||ran.returned>2147483647U)return fail<bool>(ran.error?ran.error:EINVAL,"Reusable current caller thread invalid");
        const auto caller=(c[0]<<32)|ran.returned;
        if(identity_program_<0){
            // Reset a potentially stale gate after any failed disarm BEFORE
            // attaching a replacement link. Never transiently arm old state.
            const auto disabled=api_.update(api_.context,result_map_,5,0);if(disabled)return fail<bool>(disabled,"Reset reusable query gate");
            const auto code=make_private_pidfd_identity_probe(caller,result_map_,*layout_,true);if(!code.value)return {{},code.support,code.note};
            const auto loaded=api_.load(api_.context,*code.value,layout_->pidfd_identity.value->function_id,false);identity_program_=loaded.fd;
            if(identity_program_<0)return fail<bool>(loaded.error,"Reusable pidfd program load");
        }
        for(std::uint32_t k=0;k<5;++k){const auto e=api_.update(api_.context,result_map_,k,0);if(e)return fail<bool>(e,"Reset reusable result map");}
        if(link_<0){const auto attached=api_.attach(api_.context,identity_program_);link_=attached.fd;
            if(link_<0)return fail<bool>(attached.error,"Reusable pidfd link attach");}
        caller_=caller;armed_=true;
        const auto enabled=api_.update(api_.context,result_map_,5,caller_);if(enabled){cancel();return fail<bool>(enabled,"Arm reusable query for current caller thread");}
        return {true,Support::available,"Current caller thread armed for synchronous pidfd query only"};
    }catch(...){cancel();throw;}
}
Metric<PrivateNetworkIdentity> LinuxPidfdIdentityQuery::finish(){
    if(!armed_)return fail<PrivateNetworkIdentity>(ENODEV,"Reusable query transaction absent");
    if(guard_(guard_context_)!=creator_){stop();return fail<PrivateNetworkIdentity>(ECHILD,"Forked reusable query result rejected");}
    if(!disarm())return fail<PrivateNetworkIdentity>(EIO,"Reusable query gate could not be disabled; link retired");
    std::array<std::uint64_t,5> values{};
    for(std::uint32_t k=0;k<5;++k){const auto v=api_.lookup(api_.context,result_map_,k);if(!v.value||v.support!=Support::available)return {{},v.support,v.note};values[k]=*v.value;}
    return validate_private_pidfd_identity_values(values);
}
Metric<PrivateNetworkIdentity> read_private_pidfd_network_identity(bool authorized,int pidfd,const LinuxNetworkBtfLayout* session_layout,LinuxPidfdIdentityQuery* reusable){
    if(!authorized)return fail<PrivateNetworkIdentity>(EPERM,"Explicit selected pidfd authorization required; no calls made");
    if(pidfd<0)return fail<PrivateNetworkIdentity>(EINVAL,"Invalid pidfd");
#if defined(__linux__) && defined(SYS_pidfd_getfd)
    struct Fd{int n;~Fd(){if(n>=0)::close(n);}} selected{::fcntl(pidfd,F_DUPFD_CLOEXEC,0)};
    if(selected.n<0)return fail<PrivateNetworkIdentity>(errno,"Duplicate selected pidfd");
    auto alive=[&](){pollfd p{selected.n,POLLIN,0};return ::poll(&p,1,0)==0&&p.revents==0;};
    if(!alive())return fail<PrivateNetworkIdentity>(ESRCH,"Selected handle ready/invalid/exited");
    const auto layout=session_layout?Metric<LinuxNetworkBtfLayout>{*session_layout,Support::available,"Validated layout owned by live capture session"}:read_linux_network_btf();
    if(!layout.value||layout.support!=Support::available)return {{},layout.support,layout.note};
    if(!layout.value->pidfd_identity.value||layout.value->pidfd_identity.support!=Support::available)return {{},layout.value->pidfd_identity.support,layout.value->pidfd_identity.note};
    const auto api=native_network_session_api();
    struct Mask {sigset_t previous{};bool active=false;~Mask(){if(active)::pthread_sigmask(SIG_SETMASK,&previous,nullptr);}} mask;
    sigset_t blocked;::sigfillset(&blocked);const auto masked=::pthread_sigmask(SIG_BLOCK,&blocked,&mask.previous);
    if(masked)return fail<PrivateNetworkIdentity>(masked,"Block caller signals during identity query");
    mask.active=true;
    if(reusable){
        const auto begun=reusable->begin();if(!begun.value)return {{},begun.support,begun.note};
        struct Cancel {LinuxPidfdIdentityQuery* query;~Cancel(){query->cancel();}} cancel{reusable};
        const auto rc=::syscall(SYS_pidfd_getfd,selected.n,-1,0);const auto error=errno;
        if(rc>=0){::close(static_cast<int>(rc));return fail<PrivateNetworkIdentity>(EINVAL,"Unexpected target file duplicated");}
        if(error!=EBADF)return fail<PrivateNetworkIdentity>(error,"Reusable invalid-FD pidfd query not authorized/available");
        const auto result=reusable->finish();
        if(!alive())return fail<PrivateNetworkIdentity>(ESRCH,"Selected pidfd exited during reusable query");
        return result;
    }
    const auto caller=read_private_network_caller_identity(api,*layout.value);if(!caller.value)return caller;
    struct ApiFd {NetworkSessionApi api;int n=-1;~ApiFd(){if(n>=0)api.close(api.context,n);}};
    const std::array<NetworkProbeInstruction,2> thread_code{{{0x85,0,0,14},{0x95,0,0,0}}};
    const auto thread_load=api.load(api.context,thread_code,0,true);if(thread_load.fd<0)return fail<PrivateNetworkIdentity>(thread_load.error,"Load caller thread test");
    ApiFd thread{api,thread_load.fd};const auto tid=api.run(api.context,thread.n);
    if(tid.error||!tid.returned||tid.returned>2147483647U)return fail<PrivateNetworkIdentity>(tid.error?tid.error:EINVAL,"Caller initial TID discovery");
    const auto created=api.map(api.context,5);if(created.fd<0)return fail<PrivateNetworkIdentity>(created.error,"Create pidfd identity map");ApiFd map{api,created.fd};
    const auto code=make_private_pidfd_identity_probe((std::uint64_t{caller.value->kernel_tgid}<<32)|tid.returned,map.n,*layout.value);
    if(!code.value)return {{},code.support,code.note};
    const auto loaded=api.load(api.context,*code.value,layout.value->pidfd_identity.value->function_id,false);
    if(loaded.fd<0)return fail<PrivateNetworkIdentity>(loaded.error,"Load pidfd identity fexit");
    ApiFd program{api,loaded.fd};
    const auto attached=api.attach(api.context,program.n);if(attached.fd<0)return fail<PrivateNetworkIdentity>(attached.error,"Attach pidfd identity fexit");
    ApiFd link{api,attached.fd};
    const auto rc=::syscall(SYS_pidfd_getfd,selected.n,-1,0);const auto error=errno;
    api.close(api.context,link.n);link.n=-1; // detach BEFORE lookup; trigger is synchronous
    if(rc>=0){::close(static_cast<int>(rc));return fail<PrivateNetworkIdentity>(EINVAL,"Unexpected target file duplicated");}
    if(error!=EBADF)return fail<PrivateNetworkIdentity>(error,"Invalid-FD pidfd query not authorized/available");
    if(!alive())return fail<PrivateNetworkIdentity>(ESRCH,"Selected pidfd exited during query");
    std::array<std::uint64_t,5> value{};for(std::uint32_t i=0;i<5;++i){const auto v=api.lookup(api.context,map.n,i);if(!v.value||v.support!=Support::available)return {{},v.support,v.note};value[i]=*v.value;}
    return validate_private_pidfd_identity_values(value);
#else
    (void)session_layout;
    (void)reusable;
    return fail<PrivateNetworkIdentity>(ENOSYS,"Pidfd identity query requires Linux pidfd_getfd");
#endif
}
}
