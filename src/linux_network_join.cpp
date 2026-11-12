#include "linux_network_join.hpp"
#include "linux_network_sampler.hpp"
#ifdef __linux__
#include <cerrno>
#include <fcntl.h>
#include <poll.h>
#include <unistd.h>
#endif
namespace pcinsight::detail {
namespace {
template<class T> Metric<T> absent(std::string note,Support support=Support::unsupported){return {{},support,std::move(note)};}
bool valid_kernel(const PrivateNetworkIdentity& a){return a.kernel_tgid&&a.kernel_tgid<=2147483647U&&a.leader_start_boottime_ns;}
bool same_kernel(const PrivateNetworkIdentity& a,const PrivateNetworkIdentity& b){return a.kernel_tgid==b.kernel_tgid&&a.kernel_real_uid==b.kernel_real_uid&&a.leader_start_boottime_ns==b.leader_start_boottime_ns;}
}
Metric<PrivatePidfdNetworkJoin> validate_private_pidfd_network_join(std::uint32_t pid,std::uint64_t start,
    const PrivateNetworkProcContext& before,const PrivateNetworkIdentity& first,const PrivateNetworkIdentity& last,const PrivateNetworkProcContext& after){
    if(!pid||pid>2147483647U||!start||!valid_kernel(first)||!same_kernel(first,last)||before!=after||
        (!before.boot[0]&&!before.boot[1])||!valid_network_self(before.observer)||!valid_network_self(before.process)||
        before.process.local_pid!=pid||before.process.start_ticks!=start||
        before.observer.procfs_device!=before.process.procfs_device||before.observer.procfs_inode!=before.process.procfs_inode)
        return absent<PrivatePidfdNetworkJoin>("pidfd/procfs identity or observer context absent/changed");
    // procfs UID uses the observer's user namespace, not process.user_namespace.
    // Equal integers across local/kernel domains are neither required nor proof.
    return {PrivatePidfdNetworkJoin{first,before},Support::available,
        "Bracketed selected pidfd/procfs association; distinct identity domains, not atomic/future liveness"};
}
Metric<PrivatePidfdNetworkJoin> read_private_pidfd_network_join(bool authorized,int pidfd,std::uint32_t pid,std::uint64_t start,const LinuxNetworkBtfLayout* session_layout,LinuxPidfdIdentityQuery* reusable){
    if(!authorized)return absent<PrivatePidfdNetworkJoin>("Selected pidfd join requires explicit authorization",Support::permission_required);
    if(pidfd<0||!pid||pid>2147483647U||!start)return absent<PrivatePidfdNetworkJoin>("Invalid selected pidfd join request");
#ifdef __linux__
    struct Fd {int n;~Fd(){if(n>=0)::close(n);}} held{::fcntl(pidfd,F_DUPFD_CLOEXEC,0)};
    if(held.n<0)return absent<PrivatePidfdNetworkJoin>("Cannot duplicate selected pidfd",errno==EPERM||errno==EACCES?Support::permission_required:Support::unsupported);
    auto alive=[&]{pollfd p{held.n,POLLIN,0};return ::poll(&p,1,0)==0&&p.revents==0;};
    auto context=[&]()->Metric<PrivateNetworkProcContext>{
        auto api=native_network_sampler_api();auto boot=api.boot(api.context);if(!boot.value)return absent<PrivateNetworkProcContext>(boot.note,boot.support);
        auto observer=read_linux_network_self();if(!observer.value)return absent<PrivateNetworkProcContext>(observer.note,observer.support);
        auto process=read_linux_network_pidfd_process(held.n,pid,start);if(!process.value)return absent<PrivateNetworkProcContext>(process.note,process.support);
        return {PrivateNetworkProcContext{*boot.value,*observer.value,*process.value},Support::available,{}};
    };
    if(!alive())return absent<PrivatePidfdNetworkJoin>("Selected pidfd not live");
    auto before=context();if(!before.value)return absent<PrivatePidfdNetworkJoin>(before.note,before.support);
    // A same-thread-group pidfd lookup may bypass security_ptrace_access_check,
    // so the external-target hook need not emit a result for SELF. Bind the
    // already verified self pidfd/procfs context to TWO fresh current-caller
    // kernel observations instead. Never use this route for another process,
    // infer kernel IDs from local PID/UID, or reuse a cached identity.
    const bool self=private_network_join_is_self(*before.value);
    auto query=[&]()->Metric<PrivateNetworkIdentity>{
        if(!self)return read_private_pidfd_network_identity(true,held.n,session_layout,reusable);
        const auto layout=session_layout?Metric<LinuxNetworkBtfLayout>{*session_layout,Support::available,{}}:read_linux_network_btf();
        if(!layout.value||layout.support!=Support::available)return {{},layout.support,layout.note};
        return read_private_network_caller_identity(native_network_session_api(),*layout.value);
    };
    auto first=query();if(!first.value)return absent<PrivatePidfdNetworkJoin>(first.note,first.support);
    auto last=query();if(!last.value)return absent<PrivatePidfdNetworkJoin>(last.note,last.support);
    auto after=context();if(!after.value)return absent<PrivatePidfdNetworkJoin>(after.note,after.support);
    if(!alive())return absent<PrivatePidfdNetworkJoin>("Selected pidfd exited during join");
    return validate_private_pidfd_network_join(pid,start,*before.value,*first.value,*last.value,*after.value);
#else
    (void)session_layout;
    (void)reusable;
    return absent<PrivatePidfdNetworkJoin>("Selected pidfd join requires Linux");
#endif
}
}
