#include "linux_network_shared.hpp"
#include <atomic>
#include <algorithm>
#include <cerrno>
#include <set>
#include <tuple>
#include <type_traits>
#include <utility>
namespace pcinsight::detail {
namespace {
template<class T> Metric<T> absent(const char* note,Support support=Support::unsupported){return {{},support,note};}
Support why(int error){return error==EPERM||error==EACCES?Support::permission_required:Support::unsupported;}
template<class T> bool exact(const Metric<T>& m){return m.value&&m.support==Support::available;}
bool same(const PrivateNetworkIdentity& a,const PrivateNetworkIdentity& b){return a.kernel_tgid==b.kernel_tgid&&a.kernel_real_uid==b.kernel_real_uid&&a.leader_start_boottime_ns==b.leader_start_boottime_ns;}
}
void LinuxSharedNetworkSession::close(int& fd) noexcept {if(fd>=0){const int n=fd;fd=-1;api_.close(api_.context,n);}}
void LinuxSharedNetworkSession::stop() noexcept {for(auto it=links_.rbegin();it!=links_.rend();++it)close(*it);for(auto it=programs_.rbegin();it!=programs_.rend();++it)close(*it);close(table_);close(counters_);actors_.clear();active_.clear();previous_.reset();creator_=0;}
Metric<bool> LinuxSharedNetworkSession::start(bool authorized,std::span<const PrivateNetworkIdentity> actors){
    if(!authorized)return absent<bool>("Shared capture requires explicit selected-set authorization",Support::permission_required);
    if(counters_>=0)return absent<bool>("Shared capture already active; stop before replacing set");
    if(actors.empty()||actors.size()>private_network_shared_max_actors)return absent<bool>("Raw shared batch requires1..64 actors; no truncation");
    std::set<std::uint32_t> ids;for(const auto& a:actors)if(!a.kernel_tgid||a.kernel_tgid>2147483647U||!a.leader_start_boottime_ns||!ids.insert(a.kernel_tgid).second)return absent<bool>("Invalid/duplicate selected kernel actor");
    if(!api_||!api_.update||!api_.actor_map||!clock_.local_process||!clock_.now)return absent<bool>("Shared capture APIs unavailable");
    try{
        creator_=clock_.local_process(clock_.context);if(!creator_)return absent<bool>("Shared caller guard unavailable");
        const auto layout=api_.layout(api_.context);if(!exact(layout)){stop();return {{},layout.support,layout.note};}
        if(!layout.value->inet_send_function_id||!layout.value->inet6_send_function_id||!layout.value->receive_function_id){stop();return absent<bool>("Shared traffic hooks unavailable");}
        actors_.assign(actors.begin(),actors.end());
        active_.assign(actors.size(),true);
        auto map=api_.map(api_.context,9+static_cast<unsigned>(actors.size())*private_network_shared_stride);counters_=map.fd;
        if(counters_<0){stop();return absent<bool>("Cannot create shared counter ARRAY",why(map.error));}
        map=api_.actor_map(api_.context,static_cast<unsigned>(actors.size()));table_=map.fd;
        if(table_<0){stop();return absent<bool>("Cannot create selected-actor HASH",why(map.error));}
        for(unsigned i=0;i<actors_.size();++i){const auto& a=actors_[i];const auto base=9+i*private_network_shared_stride;
            for(const auto& [key,value]:{std::pair{base,std::uint64_t{a.kernel_real_uid}},std::pair{base+1,a.leader_start_boottime_ns}}){const auto e=api_.update(api_.context,counters_,key,value);if(e){stop();return absent<bool>("Cannot initialize shared actor metadata",why(e));}}
            const auto e=api_.update(api_.context,table_,a.kernel_tgid,i+1);if(e){stop();return absent<bool>("Cannot initialize selected actor index",why(e));}
        }
        const std::array<std::uint32_t,3> hooks{layout.value->inet_send_function_id,layout.value->receive_function_id,layout.value->inet6_send_function_id};
        for(unsigned i=0;i<3;++i){const auto code=make_private_shared_network_probe(table_,counters_,static_cast<unsigned>(actors_.size()),i==1,*layout.value);
            if(!exact(code)){stop();return {{},code.support,code.note};}
            const auto loaded=api_.load(api_.context,*code.value,hooks[i],false);programs_[i]=loaded.fd;
            if(programs_[i]<0){stop();return absent<bool>("Cannot load shared traffic program",why(loaded.error));}}
        for(unsigned i=0;i<3;++i){const auto attached=api_.attach(api_.context,programs_[i]);links_[i]=attached.fd;
            if(links_[i]<0){stop();return absent<bool>("Cannot attach shared traffic link",why(attached.error));}}
        if(clock_.local_process(clock_.context)!=creator_){stop();return absent<bool>("Caller changed while starting shared capture");}
        const auto enabled=api_.update(api_.context,counters_,0,1);if(enabled){stop();return absent<bool>("Cannot enable complete shared capture",why(enabled));}
        return {true,Support::available,"Three traffic links; explicit fixed kernel token set, not PID attestation or global coverage"};
    }catch(...){stop();throw;}
}
Metric<bool> LinuxSharedNetworkSession::retain(bool authorized,std::span<const PrivateNetworkIdentity> retained){
    if(!authorized)return absent<bool>("Explicit retained-set authorization required",Support::permission_required);
    if(counters_<0)return absent<bool>("Shared capture stopped");
    if(clock_.local_process(clock_.context)!=creator_){stop();return absent<bool>("Forked shared collector cannot change selection");}
    // Allocate/validate the entire proposal before the first mutation. Stable
    // slots are NEVER cleared/reassigned: an already-running BPF invocation may
    // still hold the deleted HASH value and write its retired record later.
    std::vector<bool> next(actors_.size(),false);
    for(const auto& a:retained){auto at=std::find_if(actors_.begin(),actors_.end(),[&](const auto& b){return same(a,b);});
        if(at==actors_.end())return absent<bool>("Retention cannot add/change an actor");
        const auto i=static_cast<std::size_t>(at-actors_.begin());
        if(!active_[i]||next[i])return absent<bool>("Retention cannot re-admit/duplicate an actor");
        next[i]=true;
    }
    if(retained.empty()){stop();return {true,Support::available,"Empty retained set stopped all capture"};}
    if(next==active_)return {true,Support::available,"Unchanged retained set; counters preserved in original order"};
    if(!api_.erase)return absent<bool>("Shared actor HASH deletion unavailable");
    try{
        for(unsigned i=0;i<actors_.size();++i)if(active_[i]&&!next[i]){
            const auto error=api_.erase(api_.context,table_,actors_[i].kernel_tgid);
            if(error){stop();return absent<bool>("Shared actor removal failed; entire capture stopped",why(error));}
        }
        if(clock_.local_process(clock_.context)!=creator_){stop();return absent<bool>("Caller changed during shared removal");}
        active_=std::move(next);
        return {true,Support::available,"Selected HASH entries removed; retained origin unchanged, retired slots never reused"};
    }catch(...){stop();throw;}
}
Metric<PrivateSharedNetworkRead> LinuxSharedNetworkSession::read(){
    if(counters_<0)return absent<PrivateSharedNetworkRead>("Shared capture stopped");
    if(clock_.local_process(clock_.context)!=creator_){stop();return absent<PrivateSharedNetworkRead>("Forked shared collector rejected; inherited references closed");}
    const auto begin=clock_.now(clock_.context);if(!exact(begin))return absent<PrivateSharedNetworkRead>("Shared read clock unavailable",begin.support);
    if(!*begin.value||(previous_&&*begin.value<previous_->read_finished_ns))return absent<PrivateSharedNetworkRead>("Shared read clock zero/regressed");
    for(unsigned attempt=0;attempt<8;++attempt){
        PrivateSharedNetworkRead out;out.read_started_ns=*begin.value;
        auto get=[&](int fd,std::uint32_t key,std::uint64_t& value)->Metric<bool>{const auto m=api_.lookup(api_.context,fd,key);if(!exact(m))return {{},m.support,m.note};value=*m.value;std::atomic_thread_fence(std::memory_order_seq_cst);return {true,Support::available,{}};};
        std::uint64_t active=0,sequence=0;
        auto ok=get(counters_,7,active);if(!exact(ok))return {{},ok.support,ok.note};ok=get(counters_,8,sequence);if(!exact(ok))return {{},ok.support,ok.note};
        bool valid=true;std::uint64_t updates=0;
        for(unsigned i=0;i<actors_.size();++i){const auto& a=actors_[i];const auto base=9+i*private_network_shared_stride;std::uint64_t value=0;
            if(active_[i]){ok=get(table_,a.kernel_tgid,value);if(!exact(ok))return {{},ok.support,ok.note};if(value!=i+1)return absent<PrivateSharedNetworkRead>("Shared selected actor index changed");}
            for(const auto& [fd,key,expected]:{std::tuple{counters_,base,std::uint64_t{a.kernel_real_uid}},std::tuple{counters_,base+1,a.leader_start_boottime_ns}}){
                ok=get(fd,key,value);if(!exact(ok))return {{},ok.support,ok.note};if(value!=expected)return absent<PrivateSharedNetworkRead>("Shared selected actor table/metadata changed");}
            PrivateNetworkDiscoveryRead raw;
            for(unsigned key=0;key<private_network_discovery_entries;++key){ok=get(counters_,base+2+key,raw.values[key]);if(!exact(ok))return {{},ok.support,ok.note};}
            // The outer global writer/sequence bracket covers the entire sweep,
            // including this actor's complete ARRAY105; no per-actor partial read.
            raw.active_before=raw.active_after=raw.values[7];raw.sequence_before=raw.sequence_after=raw.values[8];
            auto checked=validate_private_network_discovery_read(raw);if(active_[i]&&!exact(checked))valid=false;
            if(UINT64_MAX-updates<raw.values[8])valid=false;else updates+=raw.values[8];
            if(active_[i])out.actors.push_back({a,checked.value?std::move(*checked.value):std::vector<PrivateDiscoveredNetworkBucket>{},raw.values[8],PrivateNetworkSessionState::active});
        }
        std::array<std::uint64_t,9> global{};for(unsigned key=0;key<9;++key){ok=get(counters_,key,global[key]);if(!exact(ok))return {{},ok.support,ok.note};}
        std::uint64_t seq_after=0,active_after=0;ok=get(counters_,8,seq_after);if(!exact(ok))return {{},ok.support,ok.note};ok=get(counters_,7,active_after);if(!exact(ok))return {{},ok.support,ok.note};
        if(active||active_after||global[7]||sequence!=seq_after||sequence!=global[8])continue;
        if(global[0]!=1||global[1]||global[2]||global[3]||global[4]||global[5]||global[6]||global[8]!=updates||!valid)return absent<PrivateSharedNetworkRead>("Shared counter quality/table/registry loss; no partial frame");
        const auto end=clock_.now(clock_.context);if(!exact(end)||*end.value<*begin.value||*end.value-*begin.value>LinuxPrivateNetworkSampler::max_read_span_ns)return absent<PrivateSharedNetworkRead>("Shared read exceeds1s or clock invalid");
        if(clock_.local_process(clock_.context)!=creator_){stop();return absent<PrivateSharedNetworkRead>("Caller changed during shared read");}
        out.completed_updates=seq_after;out.read_finished_ns=*end.value;
        if(previous_){if(seq_after<previous_->completed_updates)return absent<PrivateSharedNetworkRead>("Shared sequence regressed");
            for(const auto& a:out.actors){const auto prior=std::find_if(previous_->actors.begin(),previous_->actors.end(),[&](const auto& b){return same(a.identity,b.identity);});
                if(prior==previous_->actors.end())return absent<PrivateSharedNetworkRead>("Shared retained actor absent from checkpoint");
                const auto& b=*prior;
                if(a.completed_updates<b.completed_updates||a.buckets.size()<b.buckets.size())return absent<PrivateSharedNetworkRead>("Shared actor registry/sequence regressed");
                for(unsigned j=0;j<b.buckets.size();++j){if(a.buckets[j].socket_namespace_cookie!=b.buckets[j].socket_namespace_cookie||a.buckets[j].excluded_positive_receives<b.buckets[j].excluded_positive_receives)return absent<PrivateSharedNetworkRead>("Shared immutable cookie/exclusions regressed");
                    for(unsigned k=0;k<4;++k)if(a.buckets[j].bytes[k]<b.buckets[j].bytes[k])return absent<PrivateSharedNetworkRead>("Shared actor bytes regressed");}}}
        // Stage allocations BEFORE replacing the immutable-regression guard.
        // A failed vector/string allocation must not partially replace it.
        Metric<PrivateSharedNetworkRead> result{std::move(out),Support::available,"Coherent selected raw TCP/UDP buckets; no rates/app totals/general discovery"};
        auto checkpoint=*result.value;static_assert(std::is_nothrow_move_constructible_v<PrivateSharedNetworkRead>);
        previous_.emplace(std::move(checkpoint));return result;
    }
    return absent<PrivateSharedNetworkRead>("Shared writer overlap/sequence changed; bounded retries exhausted");
}
}
