#include "linux_process_network.hpp"
#include "linux_network_shared_selected.hpp"
#include <exception>
#include <map>
#include <set>
namespace pcinsight::detail {
namespace {
template<class T> Metric<T> absent(std::string note,Support support=Support::unsupported){return {{},support,std::move(note)};}
LinuxNetworkProcIdentity local(const LinuxNetworkSelfIdentity& s){return {s.local_pid,s.local_real_uid,s.start_ticks,s.procfs_device,s.procfs_inode,s.process_inode,
    s.pid_namespace_device,s.pid_namespace_inode,s.user_namespace_device,s.user_namespace_inode};}
bool empty_metric(const Metric<std::uint64_t>& m){return !m.value&&(m.support==Support::unsupported||m.support==Support::permission_required);}
}
Metric<LinuxProcessNetworkSnapshot> publish_linux_process_network(const PrivateCommonNetworkWindow& frame){
    static_assert(LinuxProcessNetworkSampler::max_processes==LinuxMultiNetworkSampler::max_actors);
    static_assert(LinuxProcessNetworkSampler::max_counter_sweep_ns==LinuxPrivateNetworkSampler::max_read_span_ns);
    static_assert(LinuxProcessNetworkSampler::max_observed_namespaces_per_process==private_network_discovery_capacity);
    static_assert(LinuxProcessNetworkSampler::max_observed_buckets<=LinuxNetworkAccounting::max_rows);
    auto reject=[](){return absent<LinuxProcessNetworkSnapshot>("Invalid/inconsistent selected network frame; nothing published");};
    const auto& a=frame.accounting;const auto& s=a.scope;
    const bool buckets=s.coverage==NetworkAccountingCoverage::selected_namespace_bucket_set;
    const bool observed=s.coverage==NetworkAccountingCoverage::selected_observed_actor_set;
    if((s.coverage!=NetworkAccountingCoverage::selected_actor_set&&!buckets&&!observed)||s.pid_domain!=NetworkIdentityDomain::kernel_initial||s.user_domain!=NetworkIdentityDomain::kernel_initial||
        s.pid_namespace||s.user_namespace||!(s.boot[0]||s.boot[1])||!s.producer_epoch||!s.producer_tgid||s.producer_tgid>2147483647U||!s.producer_start_boottime_ns||a.lost_updates||
        (observed?(frame.observed.empty()||frame.observed.size()>LinuxProcessNetworkSampler::max_processes||!frame.associations.empty()||a.actors.size()>LinuxProcessNetworkSampler::max_observed_buckets):
            (!frame.observed.empty()||a.actors.empty()||a.actors.size()>LinuxProcessNetworkSampler::max_processes||a.actors.size()!=frame.associations.size()))||
        !frame.read_started_ns||frame.read_finished_ns<frame.read_started_ns||frame.read_finished_ns-frame.read_started_ns>LinuxProcessNetworkSampler::max_counter_sweep_ns||
        a.observed_at_ns!=frame.read_finished_ns)return reject();
    if(frame.previous_read_bracket){const auto& p=*frame.previous_read_bracket;
        if(!p[0]||p[1]<p[0]||p[1]-p[0]>LinuxProcessNetworkSampler::max_counter_sweep_ns||p[1]>frame.read_started_ns||p[1]>=frame.read_finished_ns)return reject();}
    std::vector<PrivateCommonNetworkActor> discovered_associations;
    if(observed){
        std::set<std::uint32_t> pids;std::set<std::pair<std::uint32_t,std::uint64_t>> kernels;
        const auto& observer=frame.observed.front().local.observer;
        for(const auto& actor:frame.observed){const auto& c=actor.local;const auto& k=actor.identity;
            if(!pids.insert(c.process.local_pid).second||!kernels.emplace(k.kernel_tgid,k.leader_start_boottime_ns).second||
                c.boot!=s.boot||c.observer!=observer||!validate_private_pidfd_network_join(c.process.local_pid,c.process.start_ticks,c,k,k,c).value||
                actor.buckets.size()>LinuxProcessNetworkSampler::max_observed_namespaces_per_process||actor.completed_updates<actor.buckets.size()||
                (actor.buckets.empty()&&actor.completed_updates))return reject();
            std::set<std::uint64_t> cookies;
            for(const auto& b:actor.buckets){
                if(!b.socket_namespace_cookie||!cookies.insert(b.socket_namespace_cookie).second||
                    (!b.excluded_positive_receives&&b.bytes==NetworkCounters{}))return reject();
                discovered_associations.push_back({{k.kernel_tgid,k.kernel_real_uid,k.leader_start_boottime_ns,b.socket_namespace_cookie},c,
                    {b.bytes,b.excluded_positive_receives,actor.completed_updates}});
            }
        }
        if(discovered_associations.size()!=a.actors.size())return reject();
    }
    const auto& source=observed?discovered_associations:frame.associations;
    std::map<NetworkActor,const PrivateCommonNetworkActor*> associations;
    std::map<std::uint32_t,const PrivateCommonNetworkActor*> local_pids;
    std::map<std::pair<std::uint32_t,std::uint64_t>,const PrivateCommonNetworkActor*> kernels;
    const auto& observer=observed?frame.observed.front().local.observer:source.front().local.observer;
    for(const auto& row:source){const auto& key=row.actor;const auto& c=row.local;
        PrivateNetworkIdentity k{key.tgid,key.real_uid,key.leader_start_boottime_ns};
        auto compatible=[&](const PrivateCommonNetworkActor* p){return (buckets||observed)&&p->local==c&&p->actor.tgid==key.tgid&&p->actor.real_uid==key.real_uid&&p->actor.leader_start_boottime_ns==key.leader_start_boottime_ns;};
        const auto [local_at,local_new]=local_pids.emplace(c.process.local_pid,&row);
        const auto [kernel_at,kernel_new]=kernels.emplace(std::pair{key.tgid,key.leader_start_boottime_ns},&row);
        if(!key.socket_network_namespace_cookie||!associations.emplace(key,&row).second||(!kernel_new&&!compatible(kernel_at->second))||
            (!local_new&&!compatible(local_at->second))||c.boot!=s.boot||c.observer!=observer||
            !validate_private_pidfd_network_join(c.process.local_pid,c.process.start_ticks,c,k,k,c).value)return reject();
    }
    LinuxProcessNetworkSnapshot out;out.coverage=observed?LinuxProcessNetworkCoverage::observed_tcp_udp_socket_namespace_buckets:buckets?LinuxProcessNetworkCoverage::declared_tcp_udp_socket_namespace_buckets:LinuxProcessNetworkCoverage::selected_tcp_udp_socket_operations;
    out.observer_boot_token=s.boot;out.selection_epoch=s.producer_epoch;out.producer_initial_tgid=s.producer_tgid;
    out.producer_start_boottime_ns=s.producer_start_boottime_ns;out.observer=local(observer);
    out.counter_sweep_started_ns=frame.read_started_ns;out.counter_sweep_finished_ns=frame.read_finished_ns;out.previous_counter_sweep=frame.previous_read_bracket;
    for(const auto& actor:frame.observed)out.observed_processes.push_back({actor.identity.kernel_tgid,actor.identity.kernel_real_uid,actor.identity.leader_start_boottime_ns,
        local(actor.local.process),static_cast<std::uint64_t>(actor.buckets.size()),actor.completed_updates});
    std::map<std::uint32_t,LinuxProcessNetworkUidSample> expected;std::set<NetworkActor> seen;
    for(const auto& row:a.actors){const auto found=associations.find(row.actor);if(found==associations.end()||!seen.insert(row.actor).second)return reject();
        const auto& association=*found->second;
        if(!row.bytes.value||row.bytes.support!=Support::available||*row.bytes.value!=association.quality.bytes)return reject();
        if(row.interval_ns.value){if(row.interval_ns.support!=Support::available||!frame.previous_read_bracket||
                *row.interval_ns.value!=frame.read_finished_ns-(*frame.previous_read_bracket)[1]||!*row.interval_ns.value||*row.interval_ns.value>LinuxNetworkAccounting::max_interval_ns)return reject();}
        else if(!empty_metric(row.interval_ns))return reject();
        auto& user=expected[row.actor.real_uid];user.initial_real_uid=row.actor.real_uid;++user.observed_buckets;
        for(unsigned lane=0;lane<4;++lane){const auto& rate=row.bytes_per_second[lane];
            if(!rate.value){if(!empty_metric(rate))return reject();++user.missing[lane];continue;}
            if(!row.interval_ns.value||rate.support!=Support::estimated)return reject();
            ++user.known[lane];if(user.overflow[lane])continue;
            const auto prior=user.partial_payload_bytes_per_second[lane].value.value_or(0); // Accumulator, never a missing sample.
            if(*rate.value>UINT64_MAX-prior){user.overflow[lane]=true;user.partial_payload_bytes_per_second[lane].value.reset();}
            else user.partial_payload_bytes_per_second[lane].value=prior+*rate.value;
        }
        out.processes.push_back({{row.actor.tgid,row.actor.real_uid,row.actor.leader_start_boottime_ns,row.actor.socket_network_namespace_cookie},local(association.local.process),
            row.bytes,row.bytes_per_second,row.interval_ns,association.quality.excluded_positive_receives,association.quality.completed_updates});
    }
    if(expected.size()!=a.users.size())return reject();
    std::set<std::uint32_t> users;
    for(const auto& user:a.users){const auto found=expected.find(user.real_uid);if(found==expected.end()||!users.insert(user.real_uid).second)return reject();const auto& e=found->second;
        if(user.observed_buckets!=e.observed_buckets||user.known!=e.known||user.missing!=e.missing||user.overflow!=e.overflow)return reject();
        for(unsigned lane=0;lane<4;++lane){const auto& rate=user.bytes_per_second[lane];
            if(!e.known[lane]||e.overflow[lane]){if(!empty_metric(rate))return reject();}
            else if(rate.support!=Support::estimated||rate.value!=e.partial_payload_bytes_per_second[lane].value)return reject();}
        out.users.push_back({user.real_uid,user.observed_buckets,user.known,user.missing,user.overflow,user.bytes_per_second});
    }
    return {std::move(out),Support::estimated,"Selected instrumented TCP/UDP payload only; nominal common window with bounded skew; partial initial-UID sums, not global/process/user totals"};
}
}
namespace pcinsight {
struct LinuxProcessNetworkSampler::Impl {
    LinuxProcessNetworkSelectionMode mode;
    detail::LinuxMultiNetworkSampler sampler;
    detail::LinuxSharedSelectedNetworkSampler shared;
    explicit Impl(LinuxProcessNetworkSelectionMode m=LinuxProcessNetworkSelectionMode::strict_single_namespace):mode(m),
        sampler(detail::native_network_session_api(),detail::native_network_sampler_api(),detail::native_selected_network_source_api(),m==LinuxProcessNetworkSelectionMode::declared_namespace_buckets,m==LinuxProcessNetworkSelectionMode::observed_namespace_buckets){}
    bool valid_mode()const{return mode==LinuxProcessNetworkSelectionMode::strict_single_namespace||mode==LinuxProcessNetworkSelectionMode::declared_namespace_buckets||mode==LinuxProcessNetworkSelectionMode::observed_namespace_buckets;}
    bool observed()const{return mode==LinuxProcessNetworkSelectionMode::observed_namespace_buckets;}
    void stop()noexcept{shared.stop();sampler.stop();}
};
LinuxProcessNetworkSampler::LinuxProcessNetworkSampler():impl_(std::make_unique<Impl>()){}
LinuxProcessNetworkSampler::LinuxProcessNetworkSampler(LinuxProcessNetworkSelectionMode mode):impl_(std::make_unique<Impl>(mode)){}
LinuxProcessNetworkSampler::~LinuxProcessNetworkSampler()=default;
LinuxProcessNetworkSampler::LinuxProcessNetworkSampler(LinuxProcessNetworkSampler&&) noexcept=default;
LinuxProcessNetworkSampler& LinuxProcessNetworkSampler::operator=(LinuxProcessNetworkSampler&&) noexcept=default;
void LinuxProcessNetworkSampler::stop() noexcept {if(impl_)impl_->stop();}
Metric<std::uint64_t> LinuxProcessNetworkSampler::start(bool authorized,std::span<const LinuxProcessNetworkSelection> selection){
    if(!authorized)return {{},Support::permission_required,"Explicit selected BPF capture authorization required; no new capture started"};
#ifdef __linux__
    if(selection.empty()||selection.size()>max_processes)return {{},Support::unsupported,"Select nonempty set within max_processes with pidfds/start ticks; cookie policy depends on explicit mode"};
    try{if(!impl_)impl_=std::make_unique<Impl>();if(!impl_->valid_mode())return {{},Support::unsupported,"Unknown selected network mode"};
        std::vector<detail::PrivateNetworkSelection> rows;rows.reserve(selection.size());
        for(const auto& s:selection)rows.push_back({s.pidfd,s.pid,s.process_start_ticks,s.socket_network_namespace_cookie});
        return impl_->observed()?impl_->shared.start(true,rows):impl_->sampler.start(true,rows);
    }catch(const std::exception& e){stop();return {{},Support::unsupported,std::string("Selected network start failed: ")+e.what()};}
#else
    (void)selection;return {{},Support::unsupported,"Selected BPF process network sampler requires Linux"};
#endif
}
Metric<LinuxProcessNetworkSnapshot> LinuxProcessNetworkSampler::sample(){
    if(!impl_)return {{},Support::unsupported,"Selected network sampler stopped/moved-from"};
    try{const auto frame=impl_->observed()?impl_->shared.sample_common():impl_->sampler.sample_common();if(!frame.value||frame.support!=Support::available)return {{},frame.support,frame.note};
        auto result=detail::publish_linux_process_network(*frame.value);if(!result.value)impl_->stop();return result;
    }catch(const std::exception& e){stop();return {{},Support::unsupported,std::string("Selected network sample failed; capture stopped: ")+e.what()};}
}
Metric<std::uint64_t> LinuxProcessNetworkSampler::update_selection(bool authorized,std::span<const LinuxProcessNetworkSelection> selection){
    if(!authorized)return {{},Support::permission_required,"Explicit authorization required for each selected network update"};
    if(!impl_)return {{},Support::unsupported,"Selection update requires an active sampler; moved-from/stopped cannot rearm"};
#ifdef __linux__
    if(selection.empty()||selection.size()>max_processes)return {{},Support::unsupported,"Replacement must be nonempty within max_processes; stop for empty"};
    try{std::vector<detail::PrivateNetworkSelection> rows;rows.reserve(selection.size());
        for(const auto& s:selection)rows.push_back({s.pidfd,s.pid,s.process_start_ticks,s.socket_network_namespace_cookie});
        return impl_->observed()?impl_->shared.update_selection(true,rows):impl_->sampler.update_selection(true,rows);
    }catch(const std::exception& e){stop();return {{},Support::unsupported,std::string("Selected network update failed; all capture stopped: ")+e.what()};}
#else
    (void)selection;return {{},Support::unsupported,"Selected network updates require Linux"};
#endif
}
}
