#include "linux_network_multi.hpp"
#include <set>
#include <map>
namespace pcinsight::detail {
namespace {
template<class T> Metric<T> absent(std::string note,Support support=Support::unsupported){return {{},support,std::move(note)};}
template<class T> bool exact(const Metric<T>& m){return m.value&&m.support==Support::available;}
bool valid_selection(std::span<const PrivateNetworkSelection> selection,bool buckets,bool observed){
    if(buckets&&observed)return false;
    if(selection.empty()||selection.size()>LinuxMultiNetworkSampler::max_actors)return false;
    std::map<std::uint32_t,std::uint64_t> local;
    std::set<std::pair<std::uint32_t,std::uint64_t>> cookies;
    for(const auto& s:selection){
        if(s.pidfd<0||!s.pid||s.pid>2147483647U||!s.start_ticks||(observed?s.socket_cookie!=0:!s.socket_cookie)||!cookies.emplace(s.pid,s.socket_cookie).second)return false;
        if(!local.emplace(s.pid,s.start_ticks).second&&(!buckets||local.at(s.pid)!=s.start_ticks))return false;
    }
    return true;
}
bool add_association(std::vector<PrivatePidfdNetworkJoin>& seen,std::set<NetworkActor>& keys,
    const PrivatePidfdNetworkJoin& j,std::uint64_t cookie,bool buckets){
    const auto& k=j.kernel;
    if(!keys.insert({k.kernel_tgid,k.kernel_real_uid,k.leader_start_boottime_ns,cookie}).second)return false;
    for(const auto& prior:seen)if(prior.local.process.local_pid==j.local.process.local_pid||
        (prior.kernel.kernel_tgid==k.kernel_tgid&&prior.kernel.leader_start_boottime_ns==k.leader_start_boottime_ns)){
        if(!buckets||prior.local!=j.local||prior.kernel.kernel_tgid!=k.kernel_tgid||prior.kernel.kernel_real_uid!=k.kernel_real_uid||prior.kernel.leader_start_boottime_ns!=k.leader_start_boottime_ns)return false;
    }
    seen.push_back(j);return true;
}
}
void LinuxMultiNetworkSampler::discard_windows() noexcept {
    for(auto& actor:actors_)if(actor)actor->discard_window();
    previous_.clear();
    common_reducer_.reset();common_bracket_.reset();
}
void LinuxMultiNetworkSampler::stop() noexcept {discard_windows();actors_.clear();epoch_=creator_=0;boot_={};mode_=Mode::none;}
Metric<std::uint64_t> LinuxMultiNetworkSampler::start(bool authorized,std::span<const PrivateNetworkSelection> selection){
    if(!authorized)return absent<std::uint64_t>("Explicit multi-actor authorization required",Support::permission_required);
    if(epoch_||!actors_.empty())return absent<std::uint64_t>("Multi sampler already started; stop to replace selection");
    if(!valid_selection(selection,namespace_buckets_,observed_cookies_)||!clock_.local_process)return absent<std::uint64_t>("Selection requires valid unique rows within capacity/caller guard; observed mode requires zero cookies and no declared bucket mode");
    try{
        creator_=clock_.local_process(clock_.context);if(!creator_){stop();return absent<std::uint64_t>("Caller guard absent");}
        std::set<NetworkActor> kernels;std::vector<PrivatePidfdNetworkJoin> seen;std::optional<LinuxNetworkSelfIdentity> observer;
        actors_.reserve(selection.size());
        for(const auto& s:selection){
            auto actor=std::make_unique<LinuxSelectedNetworkSampler>(capture_,clock_,target_);
            const auto started=observed_cookies_?actor->start_discovering(true,s.pidfd,s.pid,s.start_ticks):actor->start(true,s.pidfd,s.pid,s.start_ticks,s.socket_cookie,namespace_buckets_);
            if(!exact(started)){stop();return absent<std::uint64_t>(started.note,started.support);}
            const auto& j=*started.value;
            if(!add_association(seen,kernels,j,s.socket_cookie,namespace_buckets_)||
                (observer&&(j.local.observer!=*observer||j.local.boot!=boot_))||clock_.local_process(clock_.context)!=creator_){
                stop();return absent<std::uint64_t>("Duplicate kernel process or inconsistent observer/boot during selection");
            }
            observer=j.local.observer;boot_=j.local.boot;actors_.push_back(std::move(actor));
        }
        epoch_=next_private_network_epoch();if(!epoch_){stop();return absent<std::uint64_t>("Selection epoch exhausted");}
        return {epoch_,Support::available,observed_cookies_?"Selected actors with observed cookie registries; common windows only, not general process discovery":namespace_buckets_?"Explicit declared namespace buckets; no discovery or process totals":"Fixed explicit selection only; per-actor independent windows"};
    }catch(...){stop();throw;}
}
Metric<std::uint64_t> LinuxMultiNetworkSampler::update_selection(bool authorized,std::span<const PrivateNetworkSelection> selection){
    if(!authorized)return absent<std::uint64_t>("Explicit authorization required for each selection update",Support::permission_required);
    if(!epoch_||actors_.empty())return absent<std::uint64_t>("Selection update requires an active collector; use start first");
    if(!valid_selection(selection,namespace_buckets_,observed_cookies_))return absent<std::uint64_t>("Replacement requires valid unique rows within capacity; observed mode requires zero cookies; stop for empty");
    auto reject=[&](std::string note,Support support=Support::unsupported){stop();return absent<std::uint64_t>(std::move(note),support);};
    if(clock_.local_process(clock_.context)!=creator_)return reject("Forked collector cannot update selection; inherited references closed");
    try{
        std::vector<std::unique_ptr<LinuxSelectedNetworkSampler>> next(selection.size());
        for(std::size_t i=0;i<selection.size();++i){const auto& row=selection[i];
            for(auto& actor:actors_)if(actor&&actor->pid_==row.pid&&actor->start_==row.start_ticks&&actor->cookie_==row.socket_cookie){
                next[i]=std::move(actor);break;
            }
        }
        // Retired captures must close BEFORE replacements, including a changed cookie
        // for the same process. Runtime failure is fail-closed, not an old-selection rollback.
        actors_.clear();actors_=std::move(next);discard_windows();mode_=Mode::none;
        for(std::size_t i=0;i<selection.size();++i)if(!actors_[i]){const auto& row=selection[i];
            auto actor=std::make_unique<LinuxSelectedNetworkSampler>(capture_,clock_,target_);
            const auto started=observed_cookies_?actor->start_discovering(true,row.pidfd,row.pid,row.start_ticks):actor->start(true,row.pidfd,row.pid,row.start_ticks,row.socket_cookie,namespace_buckets_);
            if(!exact(started))return reject(started.note,started.support);
            actors_[i]=std::move(actor);
        }
        std::set<NetworkActor> kernels;std::vector<PrivatePidfdNetworkJoin> seen;std::optional<LinuxNetworkSelfIdentity> observer;
        for(auto& actor:actors_){
            const auto joined=actor->join_current();if(!exact(joined))return reject(joined.note,joined.support);
            const auto& j=*joined.value;
            if(j.local.boot!=boot_||(observer&&j.local.observer!=*observer)||
                !add_association(seen,kernels,j,actor->cookie_,namespace_buckets_))
                return reject("Replacement has duplicate kernel actors or inconsistent observer/boot");
            observer=j.local.observer;
        }
        if(clock_.local_process(clock_.context)!=creator_)return reject("Caller changed during selection update");
        const auto epoch=next_private_network_epoch();if(!epoch)return reject("Selection epoch exhausted");
        epoch_=epoch;
        return {epoch_,Support::available,"Updated explicit selected actors; all baselines reset, retained counters keep their original capture origin"};
    }catch(...){stop();throw;}
}
Metric<PrivateMultiNetworkWindow> LinuxMultiNetworkSampler::sample(){
    if(observed_cookies_)return absent<PrivateMultiNetworkWindow>("Observed-cookie collection requires sample_common");
    auto reject=[&](std::string note,Support support=Support::unsupported){discard_windows();return absent<PrivateMultiNetworkWindow>(std::move(note),support);};
    if(!epoch_)return reject("Multi sampler stopped");
    if(clock_.local_process(clock_.context)!=creator_){stop();return reject("Forked multi collector rejected; inherited references closed");}
    if(mode_!=Mode::independent){discard_windows();mode_=Mode::independent;}
    try{
        PrivateMultiNetworkWindow out;out.selection_epoch=epoch_;out.actors.reserve(actors_.size());
        std::vector<PrivateNetworkProcContext> current;current.reserve(actors_.size());
        std::set<NetworkActor> kernels;std::vector<PrivatePidfdNetworkJoin> seen;
        for(auto& actor:actors_){
            auto result=actor->sample();if(!exact(result))return reject(result.note,result.support);
            const auto& row=*result.value;
            if(row.window.accounting.actors.size()!=1||row.local.boot!=boot_||row.local.observer.local_pid!=creator_||
                (!current.empty()&&row.local.observer!=current.front().observer))
                return reject("Multi selection context/shape absent/changed");
            const auto& key=row.window.accounting.actors[0].actor;
            if(!add_association(seen,kernels,{{key.tgid,key.real_uid,key.leader_start_boottime_ns},row.local},key.socket_network_namespace_cookie,namespace_buckets_))return reject("Duplicate/inconsistent kernel actor bucket in multi window");
            current.push_back(row.local);out.actors.push_back(std::move(*result.value));
        }
        if(clock_.local_process(clock_.context)!=creator_||(!previous_.empty()&&previous_!=current))
            return reject("Multi selection context changed; all baselines discarded");
        previous_=std::move(current);
        return {std::move(out),Support::available,
            "Explicit bucket collection; independent intervals, no simultaneous snapshot or cross-actor UID sum"};
    }catch(...){discard_windows();throw;}
}
Metric<PrivateCommonNetworkWindow> LinuxMultiNetworkSampler::sample_common(){
    auto reject=[&](std::string note,Support support=Support::unsupported){discard_windows();return absent<PrivateCommonNetworkWindow>(std::move(note),support);};
    if(!epoch_)return reject("Multi sampler stopped");
    if(clock_.local_process(clock_.context)!=creator_){stop();return reject("Forked common collector rejected; inherited references closed");}
    if(mode_!=Mode::common){discard_windows();mode_=Mode::common;}
    try{
        std::vector<PrivateNetworkProcContext> current;current.reserve(actors_.size());
        std::set<NetworkActor> keys;std::vector<PrivatePidfdNetworkJoin> seen;
        for(auto& actor:actors_){
            const auto joined=actor->join_current();if(!exact(joined))return reject(joined.note,joined.support);
            if(!add_association(seen,keys,*joined.value,actor->cookie_,namespace_buckets_))return reject("Common actor/bucket associations inconsistent");
            const auto& c=joined.value->local;
            if(c.boot!=boot_||(!current.empty()&&c.observer!=current.front().observer))return reject("Common observer/boot context changed");
            current.push_back(c);
        }
        if(!previous_.empty()&&previous_!=current)return reject("Common selection context changed; all baselines discarded");
        const auto begin=clock_.now(clock_.context);if(!exact(begin))return reject(begin.note,begin.support);
        if(!*begin.value||(common_bracket_&&*begin.value<(*common_bracket_)[1]))return reject("Common sweep starts at zero or overlaps previous sweep");
        auto scope=*actors_.front()->scope_;scope.producer_epoch=epoch_;scope.coverage=observed_cookies_?NetworkAccountingCoverage::selected_observed_actor_set:namespace_buckets_?NetworkAccountingCoverage::selected_namespace_bucket_set:NetworkAccountingCoverage::selected_actor_set;
        NetworkAccountingInput input{scope,0,0,true,{}};input.rows.reserve(actors_.size());
        std::vector<PrivateCommonNetworkActor> associations;associations.reserve(actors_.size());
        std::vector<PrivateDiscoveredNetworkSessionRead> discovered;discovered.reserve(actors_.size());
        std::vector<PrivateCommonNetworkWindow::ObservedActor> observed;observed.reserve(actors_.size());
        for(std::size_t i=0;i<actors_.size();++i){auto& actor=*actors_[i];const auto& s=*actor.scope_;
            if(s.boot!=scope.boot||s.producer_tgid!=scope.producer_tgid||s.producer_start_boottime_ns!=scope.producer_start_boottime_ns||
                s.pid_domain!=scope.pid_domain||s.user_domain!=scope.user_domain)return reject("Common producer scopes differ");
            if(observed_cookies_){
                const auto read=actor.read_discovered_current();if(!exact(read))return reject(read.note,read.support);
                const auto& r=*read.value;
                for(const auto& b:r.buckets){const NetworkActor key{r.identity.kernel_tgid,r.identity.kernel_real_uid,r.identity.leader_start_boottime_ns,b.socket_namespace_cookie};
                    input.rows.push_back({key,b.bytes,true,false});
                }
                observed.push_back({r.identity,current[i],r.buckets,r.completed_updates});discovered.push_back(r);
            }else{
                const auto read=actor.read_current();if(!exact(read))return reject(read.note,read.support);
                const auto& r=*read.value;const NetworkActor key{r.identity.kernel_tgid,r.identity.kernel_real_uid,r.identity.leader_start_boottime_ns,actor.cookie_};
                input.rows.push_back({key,r.counters.bytes,true,false});associations.push_back({key,current[i],r.counters});
            }
        }
        const auto end=clock_.now(clock_.context);if(!exact(end))return reject(end.note,end.support);
        if(*end.value<*begin.value||*end.value-*begin.value>LinuxPrivateNetworkSampler::max_read_span_ns)return reject("Common counter sweep invalid/exceeds1s");
        for(std::size_t i=0;i<actors_.size();++i){const auto joined=actors_[i]->join_current();if(!exact(joined))return reject(joined.note,joined.support);
            if(joined.value->local!=current[i])return reject("Common association changed after sweep");}
        if(clock_.local_process(clock_.context)!=creator_)return reject("Common caller changed after sweep");
        input.observed_at_ns=*end.value;
        auto accounting=common_reducer_.sample({std::move(input),Support::available,"Bounded selected actor counter sweep; nominal common end time"});
        if(!exact(accounting))return reject(accounting.note,accounting.support);
        PrivateCommonNetworkWindow out{std::move(*accounting.value),std::move(associations),*begin.value,*end.value,common_bracket_,std::move(observed)};
        // No per-actor registry commit until EVERY read, join and reduction succeeded.
        for(std::size_t i=0;i<discovered.size();++i)actors_[i]->last_discovered_=std::move(discovered[i]);
        common_bracket_=std::array<std::uint64_t,2>{*begin.value,*end.value};previous_=std::move(current);
        return {std::move(out),Support::available,
            "Common nominal-end selected window; estimated rates/partial UID sums, bounded read skew, NOT atomic/global totals"};
    }catch(...){discard_windows();throw;}
}
}
