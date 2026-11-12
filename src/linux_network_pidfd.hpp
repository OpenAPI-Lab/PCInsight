#pragma once
#include "linux_network_session.hpp"
namespace pcinsight::detail {
// FEXIT security_ptrace_access_check(task*,unsigned)->int. ARRAY5 identity/quality;
// gated variant requires ARRAY6 with slot5=current caller pid_tgid (0 disables).
// Exact caller pid_tgid BEFORE target context, mode ATTACH_REALCREDS=0x12, return0 only.
Metric<std::vector<NetworkProbeInstruction>> make_private_pidfd_identity_probe(
    std::uint64_t caller_pid_tgid,int map_fd,const LinuxNetworkBtfLayout&,bool gated=false);
Metric<PrivateNetworkIdentity> validate_private_pidfd_identity_values(const std::array<std::uint64_t,5>&);
// Reusable probes/maps, never a cached identity. Session link is DISARMED outside
// the synchronous query (ARRAY slot5=exact caller pid_tgid, zero=disabled). Serialize calls
// in the same descriptor table. begin/finish are internal transaction primitives;
// the native wrapper below masks signals and performs the actual pidfd syscall.
class LinuxPidfdIdentityQuery {
public:
    LinuxPidfdIdentityQuery(NetworkSessionApi api,void* guard_context,std::uint64_t (*guard)(void*) noexcept):api_(api),guard_context_(guard_context),guard_(guard){}
    ~LinuxPidfdIdentityQuery(){stop();}
    LinuxPidfdIdentityQuery(const LinuxPidfdIdentityQuery&)=delete;
    LinuxPidfdIdentityQuery& operator=(const LinuxPidfdIdentityQuery&)=delete;
    Metric<bool> start(bool,const LinuxNetworkBtfLayout&);
    Metric<bool> begin();
    Metric<PrivateNetworkIdentity> finish();
    void cancel() noexcept;
    void stop() noexcept;
private:
    void close(int&) noexcept;
    bool disarm() noexcept;
    NetworkSessionApi api_;void* guard_context_;std::uint64_t (*guard_)(void*) noexcept;
    std::optional<LinuxNetworkBtfLayout> layout_;
    std::uint64_t creator_{},caller_{};
    bool armed_=false;
    int caller_map_=-1,result_map_=-1,caller_program_=-1,thread_program_=-1,identity_program_=-1,link_=-1;
};
// Explicit temporary hook, no task iterator/global enumeration. Duplicates the
// selected pidfd; does not duplicate a target file, deliver a signal or attach ptrace.
Metric<PrivateNetworkIdentity> read_private_pidfd_network_identity(bool authorized,int pidfd,const LinuxNetworkBtfLayout* session_layout=nullptr,LinuxPidfdIdentityQuery* reusable=nullptr);
}
