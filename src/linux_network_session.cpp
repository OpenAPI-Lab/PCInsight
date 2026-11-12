#include "linux_network_session.hpp"
#include <atomic>
#include <cerrno>
#include <cstring>
#include <limits>
#ifdef __linux__
#include <linux/bpf.h>
#include <sys/syscall.h>
#include <unistd.h>
#endif
namespace pcinsight::detail {
namespace {
Support support(int error){return error==EACCES||error==EPERM?Support::permission_required:Support::unsupported;}
template<class T> Metric<T> failed(int error,const char* operation){return {{},support(error),std::string(operation)+" errno="+std::to_string(error?error:EIO)};}
template<class T> bool exact(const Metric<T>& value){return value.value.has_value()&&value.support==Support::available;}
struct OwnedTemporary {
    const NetworkSessionApi& api;int fd=-1;
    ~OwnedTemporary(){if(fd>=0)api.close(api.context,fd);}
};
}
void LinuxPrivateNetworkSession::close(int& fd) noexcept {if(fd>=0){const auto value=fd;fd=-1;api_.close(api_.context,value);}}
void LinuxPrivateNetworkSession::detach() noexcept {
    for(auto it=links_.rbegin();it!=links_.rend();++it)close(*it);
    for(auto it=programs_.rbegin();it!=programs_.rend();++it)close(*it);
    if(map_>=0)state_=PrivateNetworkSessionState::detached;
}
void LinuxPrivateNetworkSession::stop() noexcept {detach();close(map_);identity_={};layout_.reset();target_kind_=PrivateNetworkTargetKind::self;expected_netns_cookie_=0;filter_other_namespaces_=false;discover_=false;state_=PrivateNetworkSessionState::stopped;}
Metric<PrivateNetworkIdentity> LinuxPrivateNetworkSession::read_caller_identity(){
    if(state_!=PrivateNetworkSessionState::active||!layout_||target_kind_!=PrivateNetworkTargetKind::self)return failed<PrivateNetworkIdentity>(ENODEV,"Active SELF capture required");
    return read_private_network_caller_identity(api_,*layout_);
}
Metric<PrivateNetworkIdentity> read_private_network_caller_identity(const NetworkSessionApi& api,const LinuxNetworkBtfLayout& layout){
    if(!api)return failed<PrivateNetworkIdentity>(ENOSYS,"caller identity API unavailable");
    const auto created=api.map(api.context,3);if(created.fd<0)return failed<PrivateNetworkIdentity>(created.error,"identity map");
    OwnedTemporary map{api,created.fd};
    const auto instructions=make_private_network_identity_probe(map.fd,layout);
    if(!metric_is_known(instructions))return {{},instructions.support,instructions.note};
    const auto loaded=api.load(api.context,*instructions.value,0,true);
    if(loaded.fd<0)return failed<PrivateNetworkIdentity>(loaded.error,"unattached identity program");
    OwnedTemporary program{api,loaded.fd};const auto ran=api.run(api.context,program.fd);
    if(ran.error || ran.returned)return failed<PrivateNetworkIdentity>(ran.error?ran.error:EIO,"local identity TEST_RUN");
    auto pid=api.lookup(api.context,map.fd,0),uid=api.lookup(api.context,map.fd,1),start=api.lookup(api.context,map.fd,2);
    for(const auto* field:{&pid,&uid,&start})if(!exact(*field))return {{},field->support,"Exact identity required: "+field->note};
    if(!*pid.value || *pid.value>2147483647 || *uid.value>UINT32_MAX || !*start.value)return failed<PrivateNetworkIdentity>(EINVAL,"identity output");
    return {PrivateNetworkIdentity{static_cast<std::uint32_t>(*pid.value),static_cast<std::uint32_t>(*uid.value),*start.value},Support::available,"Kernel self TGID/initial-domain UID/leader start_boottime; not procfs namespace identity"};
}
Metric<PrivateNetworkIdentity> LinuxPrivateNetworkSession::start(bool authorized,std::uint64_t netns_cookie){
    return start_impl(authorized,netns_cookie,nullptr);
}
Metric<PrivateNetworkIdentity> LinuxPrivateNetworkSession::start_selected(bool authorized,PrivateNetworkIdentity selected,std::uint64_t cookie,bool filter_other_namespaces){
    if(!authorized)return failed<PrivateNetworkIdentity>(EPERM,"explicit selected actor capture authorization required; no calls made");
    if(!selected.kernel_tgid||selected.kernel_tgid>2147483647U||!selected.leader_start_boottime_ns||!cookie)
        return failed<PrivateNetworkIdentity>(EINVAL,"positive kernel TGID/start and socket namespace cookie required; no calls made");
    return start_impl(true,cookie,&selected,filter_other_namespaces);
}
Metric<PrivateNetworkIdentity> LinuxPrivateNetworkSession::start_discovering_selected(bool authorized,PrivateNetworkIdentity selected){
    if(!authorized)return failed<PrivateNetworkIdentity>(EPERM,"explicit observed-cookie capture authorization required; no calls made");
    if(!selected.kernel_tgid||selected.kernel_tgid>2147483647U||!selected.leader_start_boottime_ns)
        return failed<PrivateNetworkIdentity>(EINVAL,"positive selected kernel TGID/start required");
    return start_impl(true,0,&selected,false,true);
}
Metric<PrivateNetworkIdentity> LinuxPrivateNetworkSession::start_impl(bool authorized,std::uint64_t netns_cookie,const PrivateNetworkIdentity* selected,bool filter_other_namespaces,bool discover){
    if(!authorized)return failed<PrivateNetworkIdentity>(EPERM,"explicit capture authorization required; no calls made");
    if(state_!=PrivateNetworkSessionState::stopped)return failed<PrivateNetworkIdentity>(EBUSY,"session already owns a map; stop before restart");
    if(!api_)return failed<PrivateNetworkIdentity>(ENOSYS,"native capture API unavailable");
    try {
        const auto layout=api_.layout(api_.context);
        if(!exact(layout))return {{},layout.support,"Validated exact BTF required: "+layout.note};
        if(!layout.value->inet_send_function_id || !layout.value->inet6_send_function_id || !layout.value->receive_function_id)
            return failed<PrivateNetworkIdentity>(ENOSYS,"classified fexit hooks absent");
        if((netns_cookie||discover)&&!exact(layout.value->socket_namespace))
            return {{},layout.value->socket_namespace.support,"Exact socket namespace BTF required: "+layout.value->socket_namespace.note};
        const auto identity=selected?Metric<PrivateNetworkIdentity>{*selected,Support::available,"Caller-declared kernel-domain selected actor; not procfs attestation"}:
            read_private_network_caller_identity(api_,*layout.value);
        if(!metric_is_known(identity))return identity;
        identity_=*identity.value;layout_=*layout.value;expected_netns_cookie_=netns_cookie;filter_other_namespaces_=filter_other_namespaces;discover_=discover;
        target_kind_=selected?PrivateNetworkTargetKind::selected_actor:PrivateNetworkTargetKind::self;
        const auto created=api_.map(api_.context,discover_?private_network_discovery_entries:9);
        if(created.fd<0){stop();return failed<PrivateNetworkIdentity>(created.error,"counter map");}
        map_=created.fd;
        const std::array<std::uint32_t,3> hooks{layout.value->inet_send_function_id,layout.value->receive_function_id,layout.value->inet6_send_function_id};
        for(unsigned i=0;i<3;++i){
            const auto code=discover_?make_private_discovering_network_probe(identity_.kernel_tgid,map_,i==1,*layout.value,identity_):
                make_private_classified_network_probe(identity_.kernel_tgid,map_,i==1,*layout.value,&identity_,expected_netns_cookie_,filter_other_namespaces_);
            if(!metric_is_known(code)){stop();return {{},code.support,code.note};}
            const auto loaded=api_.load(api_.context,*code.value,hooks[i],false);
            if(loaded.fd<0){stop();return failed<PrivateNetworkIdentity>(loaded.error,"classified program load");}
            programs_[i]=loaded.fd;
        }
        for(unsigned i=0;i<3;++i){const auto attached=api_.attach(api_.context,programs_[i]);
            if(attached.fd<0){stop();return failed<PrivateNetworkIdentity>(attached.error,"classified link attach");}
            links_[i]=attached.fd;
        }
        state_=PrivateNetworkSessionState::active;return identity;
    }catch(...){stop();throw;}
}
Metric<PrivateNetworkSessionRead> LinuxPrivateNetworkSession::read(){
    if(discover_)return failed<PrivateNetworkSessionRead>(EINVAL,"observed-cookie capture requires read_discovered; not aggregate counters");
    if(map_<0)return failed<PrivateNetworkSessionRead>(ENODEV,"capture map absent");
    Metric<PrivateNetworkQualityCounters> checked;
    for(unsigned attempt=0;attempt<8;++attempt){
        PrivateNetworkQualityRead r;
        auto get=[&](std::uint32_t key,std::uint64_t& out)->Metric<bool>{
            const auto value=api_.lookup(api_.context,map_,key);
            if(!exact(value))return {{},value.support,"Exact counter required: "+value.note};
            out=*value.value;std::atomic_thread_fence(std::memory_order_seq_cst);return {true,Support::available,{}};
        };
        auto fetch=[&]()->Metric<bool>{
            auto ok=get(7,r.active_before);if(!metric_is_known(ok))return ok;
            ok=get(8,r.sequence_before);if(!metric_is_known(ok))return ok;
            for(std::uint32_t i=0;i<7;++i){ok=get(i,r.values[i]);if(!metric_is_known(ok))return ok;}
            ok=get(8,r.sequence_after);if(!metric_is_known(ok))return ok;
            return get(7,r.active_after);
        };
        const auto fetched=fetch();if(!metric_is_known(fetched))return {{},fetched.support,fetched.note};
        checked=validate_private_network_quality_read(r);
        if(metric_is_known(checked))return {PrivateNetworkSessionRead{identity_,*checked.value,state_,expected_netns_cookie_?std::optional<std::uint64_t>{expected_netns_cookie_}:std::nullopt,target_kind_,filter_other_namespaces_},Support::available,
            target_kind_==PrivateNetworkTargetKind::selected_actor?"Coherent selected actor instrumented payload; caller-declared identity, partial paths, no procfs/app/UID join":
            expected_netns_cookie_?"Coherent SELF payload constrained to declared socket netns cookie; partial paths, no procfs/app/UID join":"Coherent instrumented SELF payload counters only; coverage partial, no PID/UID/netns join"};
        if(r.active_before==0 && r.active_after==0 && r.sequence_before==r.sequence_after)break;
    }
    return {{},checked.support,checked.note};
}
Metric<PrivateDiscoveredNetworkSessionRead> LinuxPrivateNetworkSession::read_discovered(){
    if(!discover_||map_<0)return failed<PrivateDiscoveredNetworkSessionRead>(ENODEV,"observed-cookie capture absent or different mode");
    Metric<std::vector<PrivateDiscoveredNetworkBucket>> checked;
    for(unsigned attempt=0;attempt<8;++attempt){PrivateNetworkDiscoveryRead raw;
        auto get=[&](std::uint32_t key,std::uint64_t& value)->Metric<bool>{const auto result=api_.lookup(api_.context,map_,key);
            if(!exact(result))return {{},result.support,"Exact discovery counter required: "+result.note};
            value=*result.value;std::atomic_thread_fence(std::memory_order_seq_cst);return {true,Support::available,{}};};
        auto fetch=[&]()->Metric<bool>{auto ok=get(7,raw.active_before);if(!exact(ok))return ok;
            ok=get(8,raw.sequence_before);if(!exact(ok))return ok;
            for(unsigned key=0;key<private_network_discovery_entries;++key){ok=get(key,raw.values[key]);if(!exact(ok))return ok;}
            ok=get(8,raw.sequence_after);if(!exact(ok))return ok;return get(7,raw.active_after);};
        const auto fetched=fetch();if(!exact(fetched))return {{},fetched.support,fetched.note};
        checked=validate_private_network_discovery_read(raw);
        if(exact(checked))return {PrivateDiscoveredNetworkSessionRead{identity_,std::move(*checked.value),raw.sequence_after,state_},Support::available,
            "Selected actor observed socket-cookie buckets, capacity16; no authenticated procfs join/rates/inventory or global totals"};
        if(!raw.active_before&&!raw.active_after&&raw.sequence_before==raw.sequence_after)break;
    }
    return {{},checked.support,checked.note};
}
#ifdef __linux__
namespace {
std::uint64_t pointer(const void* p){return reinterpret_cast<std::uintptr_t>(p);}
int bpf(enum bpf_cmd cmd,union bpf_attr& a){return static_cast<int>(::syscall(SYS_bpf,cmd,&a,sizeof(a)));}
NetworkSessionFd result(int fd){return {fd,fd<0?errno:0};}
Metric<LinuxNetworkBtfLayout> layout(void*){return read_linux_network_btf();}
NetworkSessionFd map(void*,unsigned entries){union bpf_attr a{};a.map_type=BPF_MAP_TYPE_ARRAY;a.key_size=4;a.value_size=8;a.max_entries=entries;return result(bpf(BPF_MAP_CREATE,a));}
NetworkSessionFd actor_map(void*,unsigned entries){union bpf_attr a{};a.map_type=BPF_MAP_TYPE_HASH;a.key_size=4;a.value_size=8;a.max_entries=entries;return result(bpf(BPF_MAP_CREATE,a));}
NetworkSessionFd load(void*,std::span<const NetworkProbeInstruction> code,std::uint32_t hook,bool identity){
    if(code.empty() || code.size()>4096)return {-1,EINVAL};
    static_assert(sizeof(bpf_insn)==sizeof(NetworkProbeInstruction));const char license[]="GPL";union bpf_attr a{};
    a.prog_type=identity?BPF_PROG_TYPE_RAW_TRACEPOINT:BPF_PROG_TYPE_TRACING;
    if(!identity){a.expected_attach_type=BPF_TRACE_FEXIT;a.attach_btf_id=hook;}
    a.insn_cnt=static_cast<__u32>(code.size());a.insns=pointer(code.data());a.license=pointer(license);
    return result(bpf(BPF_PROG_LOAD,a));
}
NetworkSessionRun run(void*,int fd){union bpf_attr a{};a.test.prog_fd=fd;const auto rc=bpf(BPF_PROG_TEST_RUN,a);return {rc<0?errno:0,a.test.retval};}
Metric<std::uint64_t> lookup(void*,int fd,std::uint32_t key){std::uint64_t value=0;union bpf_attr a{};a.map_fd=fd;a.key=pointer(&key);a.value=pointer(&value);
    if(bpf(BPF_MAP_LOOKUP_ELEM,a)<0)return failed<std::uint64_t>(errno,"BPF map lookup");
    return {value,Support::available,{}};}
NetworkSessionFd attach(void*,int fd){union bpf_attr a{};a.link_create.prog_fd=fd;a.link_create.attach_type=BPF_TRACE_FEXIT;return result(bpf(BPF_LINK_CREATE,a));}
void close_fd(void*,int fd) noexcept {::close(fd);}
int update(void*,int fd,std::uint32_t key,std::uint64_t value){union bpf_attr a{};a.map_fd=fd;a.key=reinterpret_cast<std::uint64_t>(&key);a.value=reinterpret_cast<std::uint64_t>(&value);a.flags=BPF_ANY;return bpf(BPF_MAP_UPDATE_ELEM,a)<0?errno:0;}
int erase(void*,int fd,std::uint32_t key){union bpf_attr a{};a.map_fd=fd;a.key=pointer(&key);return bpf(BPF_MAP_DELETE_ELEM,a)<0?errno:0;}
}
#endif
NetworkSessionApi native_network_session_api(){
#ifdef __linux__
    return {nullptr,layout,map,load,run,lookup,attach,close_fd,update,actor_map,erase};
#else
    return {};
#endif
}
}
