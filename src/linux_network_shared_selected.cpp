#include "linux_network_shared_selected.hpp"
#include <set>
#include <cerrno>
#include <algorithm>
namespace pcinsight::detail {
namespace {
template<class T> Metric<T> absent(std::string note,Support support=Support::unsupported){return {{},support,std::move(note)};}
template<class T> bool exact(const Metric<T>& m){return m.value&&m.support==Support::available;}
bool same(const PrivateNetworkIdentity& a,const PrivateNetworkIdentity& b){return a.kernel_tgid==b.kernel_tgid&&a.kernel_real_uid==b.kernel_real_uid&&a.leader_start_boottime_ns==b.leader_start_boottime_ns;}
}
void LinuxSharedSelectedNetworkSampler::discard_window() noexcept {reducer_.reset();previous_.clear();bracket_.reset();}
void LinuxSharedSelectedNetworkSampler::stop() noexcept {
    capture_.stop();query_.stop();
    for(const auto& h:held_)if(h.fd>=0)target_.close(target_.context,h.fd);
    held_.clear();layout_.reset();scope_.reset();creator_=0;discard_window();
}
Metric<PrivatePidfdNetworkJoin> LinuxSharedSelectedNetworkSampler::join(const Held& h){
    if(!layout_)return absent<PrivatePidfdNetworkJoin>("Shared selected layout absent");
    auto j=target_.join_query?target_.join_query(target_.context,h.fd,h.pid,h.start,*layout_,query_):target_.join_layout?target_.join_layout(target_.context,h.fd,h.pid,h.start,*layout_):target_.join(target_.context,h.fd,h.pid,h.start);
    if(!exact(j)){j.note="Selected PID="+std::to_string(h.pid)+", caller="+std::to_string(creator_)+": "+j.note;return j;}
    const auto& c=j.value->local;
    if(c.observer.local_pid!=creator_||!validate_private_pidfd_network_join(h.pid,h.start,c,j.value->kernel,j.value->kernel,c).value||
        (h.identity.kernel_tgid&&!same(h.identity,j.value->kernel)))return absent<PrivatePidfdNetworkJoin>("Shared selected pidfd/kernel/procfs identity absent or changed");
    return j;
}
Metric<std::uint64_t> LinuxSharedSelectedNetworkSampler::start(bool authorized,std::span<const PrivateNetworkSelection> selection){
    if(!authorized)return absent<std::uint64_t>("Explicit shared selected authorization required",Support::permission_required);
    if(scope_||!held_.empty())return absent<std::uint64_t>("Shared selected session already started; stop before restart");
    if(selection.empty()||selection.size()>max_selected_network_actors)return absent<std::uint64_t>("Shared selected requires nonempty selection within process capacity");
    std::set<std::uint32_t> locals;
    for(const auto& s:selection)if(s.pidfd<0||!s.pid||s.pid>2147483647U||!s.start_ticks||s.socket_cookie||!locals.insert(s.pid).second)return absent<std::uint64_t>("Unique valid PID/start/pidfd and ZERO cookies required for observed mode");
    if(!api_||!clock_.local_process||!target_.hold||!target_.close||!target_.join)return absent<std::uint64_t>("Shared selected APIs incomplete");
    try{
        creator_=clock_.local_process(clock_.context);if(!creator_){stop();return absent<std::uint64_t>("Shared selected caller guard absent");}
        auto layout=api_.layout(api_.context);if(!exact(layout)){stop();return {{},layout.support,layout.note};}layout_=std::move(*layout.value);
        const auto producer=read_private_network_caller_identity(api_,*layout_);if(!exact(producer)){stop();return {{},producer.support,producer.note};}
        if(target_.join_query){const auto ready=query_.start(true,*layout_);if(!exact(ready)){stop();return {{},ready.support,ready.note};}}
        std::vector<PrivateNetworkProcContext> contexts;std::vector<PrivateNetworkIdentity> tokens;std::set<std::uint32_t> kernels;
        held_.reserve(selection.size());contexts.reserve(selection.size());tokens.reserve(selection.size());
        for(const auto& s:selection){const auto retained=target_.hold(target_.context,s.pidfd);
            if(retained.fd<0){stop();return absent<std::uint64_t>("Cannot retain shared selected pidfd",retained.error==EACCES||retained.error==EPERM?Support::permission_required:Support::unsupported);}
            held_.push_back({retained.fd,s.pid,s.start_ticks,{}});auto joined=join(held_.back());
            if(!exact(joined)){stop();return {{},joined.support,joined.note};}
            if(!kernels.insert(joined.value->kernel.kernel_tgid).second||(!contexts.empty()&&(joined.value->local.boot!=contexts.front().boot||joined.value->local.observer!=contexts.front().observer))){stop();return absent<std::uint64_t>("Duplicate kernel actor or inconsistent shared observer/boot");}
            held_.back().identity=joined.value->kernel;tokens.push_back(joined.value->kernel);contexts.push_back(joined.value->local);
        }
        const auto started=capture_.start(true,tokens);if(!exact(started)){stop();return {{},started.support,started.note};}
        for(unsigned i=0;i<held_.size();++i){const auto joined=join(held_[i]);if(!exact(joined)){stop();return {{},joined.support,joined.note};}
            if(joined.value->local!=contexts[i]){stop();return absent<std::uint64_t>("Shared selected association changed while starting capture");}}
        if(clock_.local_process(clock_.context)!=creator_){stop();return absent<std::uint64_t>("Caller changed while starting shared selected window");}
        const auto epoch=next_private_network_epoch();if(!epoch){stop();return absent<std::uint64_t>("Shared selection epoch exhausted");}
        NetworkAccountingScope scope;scope.boot=contexts.front().boot;scope.producer_epoch=epoch;scope.pid_domain=scope.user_domain=NetworkIdentityDomain::kernel_initial;
        scope.coverage=NetworkAccountingCoverage::selected_observed_actor_set;scope.producer_tgid=producer.value->kernel_tgid;scope.producer_start_boottime_ns=producer.value->leader_start_boottime_ns;
        scope_=scope;discard_window();return {epoch,Support::available,"Authenticated pidfd selection over shared admission batches; no automatic discovery"};
    }catch(...){stop();throw;}
}
Metric<std::uint64_t> LinuxSharedSelectedNetworkSampler::update_selection(bool authorized,std::span<const PrivateNetworkSelection> selection){
    if(!authorized)return absent<std::uint64_t>("Explicit authorization required for every shared selection update",Support::permission_required);
    if(!scope_||held_.empty())return absent<std::uint64_t>("Shared selection update requires active collector");
    if(selection.empty()||selection.size()>max_selected_network_actors)return absent<std::uint64_t>("Update requires nonempty selection within process capacity; stop for empty");
    std::set<std::uint32_t> locals;
    for(const auto& s:selection)if(s.pidfd<0||!s.pid||s.pid>2147483647U||!s.start_ticks||s.socket_cookie||!locals.insert(s.pid).second)
        return absent<std::uint64_t>("Update requires unique valid PID/start/pidfd and ZERO cookies");
    auto reject=[&](std::string note,Support support=Support::unsupported){stop();return absent<std::uint64_t>(std::move(note),support);};
    if(clock_.local_process(clock_.context)!=creator_)return reject("Forked shared selection update rejected");
    // Only newly duplicated descriptors are owned by this temporary guard;
    // retained descriptors remain owned by held_ until the commit below.
    struct NewDescriptors {SelectedNetworkSourceApi api;std::vector<int> fds;
        ~NewDescriptors(){for(int fd:fds)api.close(api.context,fd);}} fresh{target_,{}};
    try{
        fresh.fds.reserve(selection.size());std::vector<Held> next;next.reserve(selection.size());
        std::vector<PrivateNetworkIdentity> tokens;tokens.reserve(selection.size());
        std::vector<PrivateNetworkProcContext> contexts;contexts.reserve(selection.size());std::set<std::uint32_t> kernels;
        for(const auto& s:selection){auto at=std::find_if(held_.begin(),held_.end(),[&](const auto& h){return h.pid==s.pid&&h.start==s.start_ticks;});
            Held h;
            if(at!=held_.end())h=*at;
            else{const auto fd=target_.hold(target_.context,s.pidfd);if(fd.fd<0)return reject("Cannot retain new shared selected pidfd",fd.error==EPERM||fd.error==EACCES?Support::permission_required:Support::unsupported);
                fresh.fds.push_back(fd.fd);h={fd.fd,s.pid,s.start_ticks,{}};}
            const auto j=join(h);if(!exact(j))return reject(j.note,j.support);
            if(j.value->local.boot!=scope_->boot||(!contexts.empty()&&j.value->local.observer!=contexts.front().observer)||!kernels.insert(j.value->kernel.kernel_tgid).second)
                return reject("Shared update duplicate kernel actor or inconsistent boot/observer");
            h.identity=j.value->kernel;next.push_back(h);tokens.push_back(h.identity);contexts.push_back(j.value->local);
        }
        const auto changed=capture_.update(true,tokens);if(!exact(changed))return reject(changed.note,changed.support);
        for(unsigned i=0;i<next.size();++i){const auto j=join(next[i]);if(!exact(j))return reject(j.note,j.support);
            if(j.value->local!=contexts[i])return reject("Shared association changed during admission update");}
        if(clock_.local_process(clock_.context)!=creator_)return reject("Caller changed during shared selection update");
        const auto epoch=next_private_network_epoch();if(!epoch)return reject("Shared selection epoch exhausted");
        // Stage the return allocation before transferring ownership/committing.
        Metric<std::uint64_t> result{epoch,Support::available,"Shared selection updated; retained origin preserved, new/re-admitted actors fresh, ALL rate baselines reset"};
        for(const auto& old:held_)if(std::none_of(next.begin(),next.end(),[&](const auto& h){return h.fd==old.fd;}))target_.close(target_.context,old.fd);
        held_=std::move(next);fresh.fds.clear();scope_->producer_epoch=epoch;discard_window();return result;
    }catch(...){stop();throw;}
}
Metric<PrivateCommonNetworkWindow> LinuxSharedSelectedNetworkSampler::sample_common(){
    auto reject=[&](std::string note,Support support=Support::unsupported){discard_window();return absent<PrivateCommonNetworkWindow>(std::move(note),support);};
    if(!scope_)return reject("Shared selected window stopped");
    if(clock_.local_process(clock_.context)!=creator_){stop();return reject("Forked shared selected sampler rejected; inherited references closed");}
    try{
        std::vector<PrivateNetworkProcContext> current;current.reserve(held_.size());
        for(const auto& h:held_){const auto j=join(h);if(!exact(j))return reject(j.note,j.support);
            if(j.value->local.boot!=scope_->boot||(!current.empty()&&j.value->local.observer!=current.front().observer))return reject("Shared selected boot/observer changed");
            current.push_back(j.value->local);}
        if(!previous_.empty()&&previous_!=current)return reject("Shared selected context changed between windows; all baselines discarded");
        auto raw=capture_.read();if(!exact(raw))return reject(raw.note,raw.support);
        const auto& r=*raw.value;
        if(r.actors.size()!=held_.size()||!r.read_started_ns||r.read_finished_ns<r.read_started_ns||r.read_finished_ns-r.read_started_ns>LinuxPrivateNetworkSampler::max_read_span_ns||
            (bracket_&&r.read_started_ns<(*bracket_)[1]))return reject("Shared selected sweep shape/clock bracket invalid");
        NetworkAccountingInput input{*scope_,r.read_finished_ns,0,true,{}};std::vector<PrivateCommonNetworkWindow::ObservedActor> observed;observed.reserve(held_.size());
        for(unsigned i=0;i<held_.size();++i){const auto& a=r.actors[i];if(a.state!=PrivateNetworkSessionState::active||!same(a.identity,held_[i].identity))return reject("Shared raw actor identity/state changed");
            for(const auto& b:a.buckets)input.rows.push_back({{a.identity.kernel_tgid,a.identity.kernel_real_uid,a.identity.leader_start_boottime_ns,b.socket_namespace_cookie},b.bytes,true,false});
            observed.push_back({a.identity,current[i],a.buckets,a.completed_updates});}
        for(unsigned i=0;i<held_.size();++i){const auto j=join(held_[i]);if(!exact(j))return reject(j.note,j.support);if(j.value->local!=current[i])return reject("Shared selected association changed after sweep");}
        if(clock_.local_process(clock_.context)!=creator_)return reject("Shared selected caller changed after sweep");
        auto accounting=reducer_.sample({std::move(input),Support::available,"Shared coherent counters with authenticated joins on both sides"});if(!exact(accounting))return reject(accounting.note,accounting.support);
        PrivateCommonNetworkWindow out{std::move(*accounting.value),{},r.read_started_ns,r.read_finished_ns,bracket_,std::move(observed)};
        bracket_=std::array<std::uint64_t,2>{r.read_started_ns,r.read_finished_ns};previous_=std::move(current);
        return {std::move(out),Support::available,"Shared admission-batch nominal window; estimated rates/partial initial-UID sums, not atomic cross-batch or process/app/user totals"};
    }catch(...){discard_window();throw;}
}
}
