#include "linux_network_selected_sampler.hpp"
#include <cerrno>
#ifdef __linux__
#include <fcntl.h>
#include <unistd.h>
#endif
namespace pcinsight::detail {
namespace {
template<class T> Metric<T> absent(std::string note,Support support=Support::unsupported){return {{},support,std::move(note)};}
template<class T> bool exact(const Metric<T>& m){return m.value&&m.support==Support::available;}
bool same(const PrivateNetworkIdentity& a,const PrivateNetworkIdentity& b){return a.kernel_tgid==b.kernel_tgid&&a.kernel_real_uid==b.kernel_real_uid&&a.leader_start_boottime_ns==b.leader_start_boottime_ns;}
bool valid(const PrivatePidfdNetworkJoin& j,std::uint32_t pid,std::uint64_t start,std::uint64_t creator){
    return j.local.observer.local_pid==creator&&validate_private_pidfd_network_join(pid,start,j.local,j.kernel,j.kernel,j.local).value.has_value();
}
}
void LinuxSelectedNetworkSampler::reset_window() noexcept {reducer_.reset();last_context_.reset();last_end_=0;}
void LinuxSelectedNetworkSampler::stop() noexcept {
    query_.stop();
    capture_.stop();if(pidfd_>=0){target_.close(target_.context,pidfd_);pidfd_=-1;}
    reset_window();scope_.reset();verified_layout_.reset();identity_={};pid_=0;start_=cookie_=creator_=0;filter_other_namespaces_=false;discover_=false;last_discovered_.reset();
}
Metric<PrivatePidfdNetworkJoin> LinuxSelectedNetworkSampler::start(bool authorized,int fd,std::uint32_t pid,std::uint64_t start,std::uint64_t cookie,bool filter_other_namespaces){
    return start_impl(authorized,fd,pid,start,cookie,filter_other_namespaces,false);
}
Metric<PrivatePidfdNetworkJoin> LinuxSelectedNetworkSampler::start_discovering(bool authorized,int fd,std::uint32_t pid,std::uint64_t start){
    return start_impl(authorized,fd,pid,start,0,false,true);
}
Metric<PrivatePidfdNetworkJoin> LinuxSelectedNetworkSampler::start_impl(bool authorized,int fd,std::uint32_t pid,std::uint64_t start,std::uint64_t cookie,bool filter_other_namespaces,bool discover){
    if(!authorized)return absent<PrivatePidfdNetworkJoin>("Selected window requires explicit authorization",Support::permission_required);
    if(scope_||pidfd_>=0)return absent<PrivatePidfdNetworkJoin>("Selected sampler already started; stop before restart");
    if(fd<0||!pid||pid>2147483647U||!start||(!discover&&!cookie)||!capture_api_||!clock_.now||!clock_.local_process||!target_.hold||!target_.close||!target_.join)
        return absent<PrivatePidfdNetworkJoin>("Selected PID/start/cookie and complete APIs required");
    try{
        creator_=clock_.local_process(clock_.context);if(!creator_){stop();return absent<PrivatePidfdNetworkJoin>("Caller process guard unavailable");}
        const auto held=target_.hold(target_.context,fd);pidfd_=held.fd;
        if(pidfd_<0){stop();return absent<PrivatePidfdNetworkJoin>("Cannot retain selected pidfd",held.error==EPERM||held.error==EACCES?Support::permission_required:Support::unsupported);}
        const auto layout=capture_api_.layout(capture_api_.context);
        if(!exact(layout)){stop();return absent<PrivatePidfdNetworkJoin>(layout.note,layout.support);}
        if(target_.join_query){const auto ready=query_.start(true,*layout.value);if(!exact(ready)){stop();return absent<PrivatePidfdNetworkJoin>(ready.note,ready.support);}}
        const auto joined=target_.join_query?target_.join_query(target_.context,pidfd_,pid,start,*layout.value,query_):target_.join_layout?target_.join_layout(target_.context,pidfd_,pid,start,*layout.value):target_.join(target_.context,pidfd_,pid,start);
        if(!exact(joined)){stop();return absent<PrivatePidfdNetworkJoin>(joined.note,joined.support);}
        if(!valid(*joined.value,pid,start,creator_)){stop();return absent<PrivatePidfdNetworkJoin>("Selected initial association invalid");}
        const auto producer=read_private_network_caller_identity(capture_api_,*layout.value);
        if(!exact(producer)){stop();return absent<PrivatePidfdNetworkJoin>(producer.note,producer.support);}
        const auto started=discover?capture_.start_discovering_selected(true,joined.value->kernel):capture_.start_selected(true,joined.value->kernel,cookie,filter_other_namespaces);
        if(!exact(started)){stop();return absent<PrivatePidfdNetworkJoin>(started.note,started.support);}
        const auto serial=next_private_network_epoch();if(!serial){stop();return absent<PrivatePidfdNetworkJoin>("Producer epoch exhausted");}
        NetworkAccountingScope scope;scope.boot=joined.value->local.boot;scope.producer_epoch=serial;
        scope.pid_domain=scope.user_domain=NetworkIdentityDomain::kernel_initial;
        scope.coverage=discover?NetworkAccountingCoverage::selected_observed_namespace_set:NetworkAccountingCoverage::selected_single_socket_namespace;
        scope.producer_tgid=producer.value->kernel_tgid;scope.producer_start_boottime_ns=producer.value->leader_start_boottime_ns;
        scope_=scope;verified_layout_=*layout.value;identity_=joined.value->kernel;pid_=pid;start_=start;cookie_=cookie;filter_other_namespaces_=filter_other_namespaces;discover_=discover;reset_window();return joined;
    }catch(...){stop();throw;}
}
Metric<PrivatePidfdNetworkJoin> LinuxSelectedNetworkSampler::join_current(){
    if(!scope_)return absent<PrivatePidfdNetworkJoin>("Selected sampler stopped");
    if(clock_.local_process(clock_.context)!=creator_){stop();return absent<PrivatePidfdNetworkJoin>("Forked selected collector rejected");}
    if(!verified_layout_)return absent<PrivatePidfdNetworkJoin>("Selected session layout absent");
    auto result=target_.join_query?target_.join_query(target_.context,pidfd_,pid_,start_,*verified_layout_,query_):target_.join_layout?target_.join_layout(target_.context,pidfd_,pid_,start_,*verified_layout_):target_.join(target_.context,pidfd_,pid_,start_);
    if(!exact(result))return result;
    if(!valid(*result.value,pid_,start_,creator_)||!same(result.value->kernel,identity_)||result.value->local.boot!=scope_->boot)
        return absent<PrivatePidfdNetworkJoin>("Selected identity/boot context changed");
    return result;
}
Metric<PrivateNetworkSessionRead> LinuxSelectedNetworkSampler::read_current(){
    if(discover_)return absent<PrivateNetworkSessionRead>("Observed-cookie mode requires discovered read");
    if(!scope_)return absent<PrivateNetworkSessionRead>("Selected sampler stopped");
    if(clock_.local_process(clock_.context)!=creator_){stop();return absent<PrivateNetworkSessionRead>("Forked selected collector rejected");}
    auto result=capture_.read();if(!exact(result))return result;
    const auto& r=*result.value;
    if(r.state!=PrivateNetworkSessionState::active||r.target_kind!=PrivateNetworkTargetKind::selected_actor||!same(r.identity,identity_)||r.expected_socket_network_namespace_cookie!=cookie_||r.filters_other_socket_namespaces!=filter_other_namespaces_)
        return absent<PrivateNetworkSessionRead>("Selected capture identity/cookie/state changed");
    return result;
}
Metric<PrivateSelectedNetworkWindow> LinuxSelectedNetworkSampler::sample(){
    if(discover_)return absent<PrivateSelectedNetworkWindow>("Observed-cookie mode requires sample_discovered");
    auto reject=[&](std::string note,Support support=Support::unsupported){reset_window();return absent<PrivateSelectedNetworkWindow>(std::move(note),support);};
    if(!scope_)return reject("Selected sampler stopped");
    if(clock_.local_process(clock_.context)!=creator_){stop();return reject("Forked sampler is not creator; inherited capture and pidfd closed");}
    try{
        const auto before=join_current();
        if(!exact(before))return reject(before.note,before.support);
        if(!valid(*before.value,pid_,start_,creator_)||!same(before.value->kernel,identity_)||before.value->local.boot!=scope_->boot)
            return reject("Selected identity/boot context changed before read");
        if(!last_context_||*last_context_!=before.value->local)reset_window();
        const auto begin=clock_.now(clock_.context);if(!exact(begin))return reject(begin.note,begin.support);
        const auto read=read_current();if(!exact(read))return reject(read.note,read.support);
        const auto end=clock_.now(clock_.context);if(!exact(end))return reject(end.note,end.support);
        if(!*begin.value||*end.value<*begin.value||*begin.value<last_end_||*end.value-*begin.value>LinuxPrivateNetworkSampler::max_read_span_ns)
            return reject("Selected read clock bracket invalid/overlapping/exceeds1s");
        const auto& r=*read.value;
        if(r.state!=PrivateNetworkSessionState::active||r.target_kind!=PrivateNetworkTargetKind::selected_actor||!same(r.identity,identity_)||r.expected_socket_network_namespace_cookie!=cookie_)
            return reject("Selected capture identity/cookie/state changed");
        const auto after=join_current();
        if(!exact(after))return reject(after.note,after.support);
        if(!valid(*after.value,pid_,start_,creator_)||!same(after.value->kernel,identity_)||after.value->local!=before.value->local||clock_.local_process(clock_.context)!=creator_)
            return reject("Selected association changed after read");
        NetworkAccountingInput input{*scope_,*end.value,0,true,{
            {{identity_.kernel_tgid,identity_.kernel_real_uid,identity_.leader_start_boottime_ns,cookie_},r.counters.bytes,true,false}}};
        auto accounting=reducer_.sample({std::move(input),Support::available,"One selected actor/cookie bucket only"});
        if(!exact(accounting))return reject(accounting.note,accounting.support);
        last_end_=*end.value;last_context_=after.value->local;
        return {PrivateSelectedNetworkWindow{{std::move(*accounting.value),r.counters,*begin.value,*end.value},after.value->local},Support::available,
            "Selected pidfd/procfs-associated window; read-end rates estimated, UID sum partial; no global/app totals"};
    }catch(...){reset_window();throw;}
}
Metric<PrivateSelectedDiscoveredNetworkWindow> LinuxSelectedNetworkSampler::sample_discovered(){
    auto reject=[&](std::string note,Support support=Support::unsupported){reset_window();return absent<PrivateSelectedDiscoveredNetworkWindow>(std::move(note),support);};
    if(!discover_)return absent<PrivateSelectedDiscoveredNetworkWindow>("Observed-cookie mode is not active");
    if(!scope_)return reject("Selected sampler stopped");
    if(clock_.local_process(clock_.context)!=creator_){stop();return reject("Forked sampler rejected; inherited capture and pidfd closed");}
    try{
        const auto before=join_current();if(!exact(before))return reject(before.note,before.support);
        if(!last_context_||*last_context_!=before.value->local)reset_window();
        const auto begin=clock_.now(clock_.context);if(!exact(begin))return reject(begin.note,begin.support);
        const auto read=read_discovered_current();if(!exact(read))return reject(read.note,read.support);
        const auto end=clock_.now(clock_.context);if(!exact(end))return reject(end.note,end.support);
        if(!*begin.value||*end.value<*begin.value||*begin.value<last_end_||*end.value-*begin.value>LinuxPrivateNetworkSampler::max_read_span_ns)
            return reject("Observed read clock bracket invalid/overlapping/exceeds1s");
        const auto& r=*read.value;
        if(r.state!=PrivateNetworkSessionState::active||!same(r.identity,identity_))return reject("Observed capture identity/state changed");
        const auto after=join_current();if(!exact(after))return reject(after.note,after.support);
        if(after.value->local!=before.value->local||clock_.local_process(clock_.context)!=creator_)
            return reject("Selected association changed during observed read");
        NetworkAccountingInput input{*scope_,*end.value,0,true,{}};
        for(const auto& b:r.buckets)input.rows.push_back({{identity_.kernel_tgid,identity_.kernel_real_uid,identity_.leader_start_boottime_ns,b.socket_namespace_cookie},b.bytes,true,false});
        auto accounting=reducer_.sample({std::move(input),Support::available,"Complete bounded observed registry for ONE selected actor"});
        if(!exact(accounting))return reject(accounting.note,accounting.support);
        last_discovered_=r;last_end_=*end.value;last_context_=after.value->local;
        return {PrivateSelectedDiscoveredNetworkWindow{std::move(*accounting.value),r.buckets,r.completed_updates,*begin.value,*end.value,after.value->local},Support::available,
            "Selected pidfd/procfs-associated observed buckets; new bucket first rate absent, read-end rates estimated; UID sum partial, not app/global totals"};
    }catch(...){reset_window();throw;}
}
Metric<PrivateDiscoveredNetworkSessionRead> LinuxSelectedNetworkSampler::read_discovered_current(){
    using Read=PrivateDiscoveredNetworkSessionRead;
    if(!scope_||!discover_)return absent<Read>("Observed-cookie mode is not active");
    if(clock_.local_process(clock_.context)!=creator_){stop();return absent<Read>("Forked observed reader rejected");}
    auto result=capture_.read_discovered();if(!exact(result))return result;
    const auto& r=*result.value;
    if(r.state!=PrivateNetworkSessionState::active||!same(r.identity,identity_))return absent<Read>("Observed capture identity/state changed");
    if(last_discovered_){
        const auto& prior=*last_discovered_;
        if(r.completed_updates<prior.completed_updates||r.buckets.size()<prior.buckets.size())return absent<Read>("Observed registry/sequence regressed");
        for(std::size_t i=0;i<prior.buckets.size();++i){
            const auto& a=prior.buckets[i];const auto& b=r.buckets[i];
            if(a.socket_namespace_cookie!=b.socket_namespace_cookie||b.excluded_positive_receives<a.excluded_positive_receives)
                return absent<Read>("Observed immutable slot/exclusion counter regressed");
            for(unsigned lane=0;lane<4;++lane)if(b.bytes[lane]<a.bytes[lane])return absent<Read>("Observed payload counter regressed");
        }
    }
    return result;
}
SelectedNetworkSourceApi native_selected_network_source_api(){
#ifdef __linux__
    return {nullptr,
        [](void*,int fd)->NetworkSessionFd{const int n=::fcntl(fd,F_DUPFD_CLOEXEC,0);return {n,n<0?errno:0};},
        [](void*,int fd) noexcept {::close(fd);},
        [](void*,int fd,std::uint32_t pid,std::uint64_t start){return read_private_pidfd_network_join(true,fd,pid,start);},
        [](void*,int fd,std::uint32_t pid,std::uint64_t start,const LinuxNetworkBtfLayout& layout){return read_private_pidfd_network_join(true,fd,pid,start,&layout);},
        [](void*,int fd,std::uint32_t pid,std::uint64_t start,const LinuxNetworkBtfLayout& layout,LinuxPidfdIdentityQuery& query){return read_private_pidfd_network_join(true,fd,pid,start,&layout,&query);}};
#else
    return {};
#endif
}
}
