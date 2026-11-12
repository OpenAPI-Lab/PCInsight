#include "linux_network_shared_set.hpp"
#include <algorithm>
#include <set>
namespace pcinsight::detail {
namespace {
template<class T> Metric<T> absent(const char* note,Support s=Support::unsupported){return {{},s,note};}
template<class T> bool exact(const Metric<T>& m){return m.value&&m.support==Support::available;}
bool same(const PrivateNetworkIdentity& a,const PrivateNetworkIdentity& b){return a.kernel_tgid==b.kernel_tgid&&a.kernel_real_uid==b.kernel_real_uid&&a.leader_start_boottime_ns==b.leader_start_boottime_ns;}
bool contains(std::span<const PrivateNetworkIdentity> list,const PrivateNetworkIdentity& a){return std::any_of(list.begin(),list.end(),[&](const auto& b){return same(a,b);});}
bool valid(std::span<const PrivateNetworkIdentity> list){if(list.empty()||list.size()>LinuxProcessNetworkSampler::max_processes)return false;std::set<std::uint32_t> seen;
    for(const auto& a:list)if(!a.kernel_tgid||a.kernel_tgid>2147483647U||!a.leader_start_boottime_ns||!seen.insert(a.kernel_tgid).second)return false;
    return true;
}
}
Metric<bool> LinuxSharedNetworkSet::start(bool authorized,std::span<const PrivateNetworkIdentity> actors){
    if(!authorized)return absent<bool>("Explicit admission authorization required",Support::permission_required);
    if(!groups_.empty())return absent<bool>("Shared set already active");
    if(!valid(actors)||!clock_.local_process)return absent<bool>("Shared set requires unique valid tokens within public capacity/caller guard");
    try{
        creator_=clock_.local_process(clock_.context);if(!creator_){stop();return absent<bool>("Shared set caller absent");}
        actors_.assign(actors.begin(),actors.end());groups_.reserve(LinuxProcessNetworkSampler::max_processes);
        for(std::size_t first=0;first<actors.size();first+=private_network_shared_max_actors){
            const auto chunk=actors.subspan(first,std::min<std::size_t>(private_network_shared_max_actors,actors.size()-first));
            Group g{std::make_unique<LinuxSharedNetworkSession>(api_,clock_),{chunk.begin(),chunk.end()}};
            auto r=g.capture->start(true,g.actors);if(!exact(r)){stop();return r;}groups_.push_back(std::move(g));
        }
        if(clock_.local_process(clock_.context)!=creator_){stop();return absent<bool>("Caller changed while admitting shared batches");}
        return {true,Support::available,"Complete explicit set admitted in disjoint batches of at most64"};
    }catch(...){stop();throw;}
}
Metric<bool> LinuxSharedNetworkSet::update(bool authorized,std::span<const PrivateNetworkIdentity> actors){
    if(!authorized)return absent<bool>("Explicit authorization required for every shared set update",Support::permission_required);
    if(groups_.empty())return absent<bool>("Shared set stopped");
    if(!valid(actors))return absent<bool>("Update requires unique valid tokens within public capacity; stop for empty");
    if(clock_.local_process(clock_.context)!=creator_){stop();return absent<bool>("Forked shared set update rejected");}
    try{
        std::vector<PrivateNetworkIdentity> next(actors.begin(),actors.end()),added;
        for(const auto& a:next)if(!contains(actors_,a))added.push_back(a);
        // Remove old routing BEFORE any replacement starts. Never reuse the old
        // maps, even for the same TGID with a different start/UID or re-admission.
        for(auto& g:groups_){std::vector<PrivateNetworkIdentity> keep;
            for(const auto& a:g.actors)if(contains(next,a))keep.push_back(a);
            auto r=g.capture->retain(true,keep);if(!exact(r)){stop();return r;}g.actors=std::move(keep);
        }
        std::erase_if(groups_,[](const auto& g){return g.actors.empty();});
        for(std::size_t first=0;first<added.size();first+=private_network_shared_max_actors){
            const auto chunk=std::span(added).subspan(first,std::min<std::size_t>(private_network_shared_max_actors,added.size()-first));
            Group g{std::make_unique<LinuxSharedNetworkSession>(api_,clock_),{chunk.begin(),chunk.end()}};
            auto r=g.capture->start(true,g.actors);if(!exact(r)){stop();return r;}groups_.push_back(std::move(g));
        }
        if(groups_.size()>next.size()||clock_.local_process(clock_.context)!=creator_){stop();return absent<bool>("Shared admission batch bound/caller invalid");}
        actors_=std::move(next);return {true,Support::available,"Retained producers uninterrupted; additions use fresh disjoint batch"};
    }catch(...){stop();throw;}
}
Metric<PrivateSharedNetworkRead> LinuxSharedNetworkSet::read(){
    if(groups_.empty())return absent<PrivateSharedNetworkRead>("Shared set stopped");
    if(clock_.local_process(clock_.context)!=creator_){stop();return absent<PrivateSharedNetworkRead>("Forked shared set read rejected");}
    PrivateSharedNetworkRead out;std::vector<PrivateDiscoveredNetworkSessionRead> found;found.reserve(actors_.size());
    for(auto& g:groups_){auto r=g.capture->read();if(!exact(r))return {{},r.support,r.note};
        if(!out.read_started_ns)out.read_started_ns=r.value->read_started_ns;
        else if(r.value->read_started_ns<out.read_finished_ns)return absent<PrivateSharedNetworkRead>("Shared batch clock overlap/regression");
        out.read_finished_ns=r.value->read_finished_ns;
        if(!out.read_started_ns||out.read_finished_ns<out.read_started_ns||out.read_finished_ns-out.read_started_ns>LinuxPrivateNetworkSampler::max_read_span_ns)
            return absent<PrivateSharedNetworkRead>("Entire shared batch sweep exceeds1s/invalid clock");
        if(UINT64_MAX-out.completed_updates<r.value->completed_updates)return absent<PrivateSharedNetworkRead>("Shared batch aggregate sequence overflow");
        out.completed_updates+=r.value->completed_updates;
        for(auto& a:r.value->actors)found.push_back(std::move(a));
    }
    if(found.size()!=actors_.size())return absent<PrivateSharedNetworkRead>("Shared batch actor shape changed");
    out.actors.reserve(actors_.size());
    for(const auto& a:actors_){auto at=std::find_if(found.begin(),found.end(),[&](const auto& b){return same(a,b.identity);});
        if(at==found.end())return absent<PrivateSharedNetworkRead>("Shared batch actor association missing");
        out.actors.push_back(std::move(*at));}
    if(clock_.local_process(clock_.context)!=creator_){stop();return absent<PrivateSharedNetworkRead>("Caller changed during shared batch sweep");}
    return {std::move(out),Support::available,"Per-batch coherent counters within bounded common sweep; not cross-batch atomic"};
}
}
