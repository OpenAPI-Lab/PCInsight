#include "linux_network_accounting.hpp"
#include <limits>
#include <set>

namespace pcinsight::detail {
namespace {
template<class T> Metric<T> absent(std::string note){return {std::nullopt,Support::unsupported,std::move(note)};}
bool valid_scope(const NetworkAccountingScope& s){
    auto domain=[](NetworkIdentityDomain d,std::uint64_t inode){
        return (d==NetworkIdentityDomain::namespace_inode&&inode)||(d==NetworkIdentityDomain::kernel_initial&&!inode);
    };
    if(!(s.boot[0]||s.boot[1])||!s.producer_epoch||!domain(s.pid_domain,s.pid_namespace)||!domain(s.user_domain,s.user_namespace))return false;
    if(s.coverage==NetworkAccountingCoverage::declared_buckets)return !s.producer_tgid&&!s.producer_start_boottime_ns;
    return (s.coverage==NetworkAccountingCoverage::self_single_socket_namespace||s.coverage==NetworkAccountingCoverage::selected_single_socket_namespace||s.coverage==NetworkAccountingCoverage::selected_actor_set||s.coverage==NetworkAccountingCoverage::selected_namespace_bucket_set||s.coverage==NetworkAccountingCoverage::selected_observed_namespace_set||s.coverage==NetworkAccountingCoverage::selected_observed_actor_set)&&
        s.pid_domain==NetworkIdentityDomain::kernel_initial&&s.user_domain==NetworkIdentityDomain::kernel_initial&&
        s.producer_tgid&&s.producer_tgid<=2147483647U&&s.producer_start_boottime_ns;
}
bool usable(const NetworkAccountingRow& row){return row.coherent&&!row.saturated;}
Metric<std::uint64_t> rate(std::uint64_t delta,std::uint64_t ns){
    // Compute floor(delta * 1e9 / ns) exactly without a 128-bit extension or
    // lossy floating-point conversion (including UINT64_MAX boundary values).
    constexpr std::uint64_t billion=1'000'000'000;
    constexpr auto limit=std::numeric_limits<std::uint64_t>::max();
    const auto whole=delta/ns;
    if(whole>limit/billion)return absent<std::uint64_t>("rate overflow");
    const auto remainder=delta%ns; // ns <= 60e9, remainder*billion may overflow.
    // Binary long division of remainder*billion. Since remainder<ns, each
    // step's remainder<ns and twice remainder fits uint64 for our ns limit.
    std::uint64_t fraction=0, residue=0;
    for(int bit=29;bit>=0;--bit){
        fraction*=2;
        residue*=2;
        if((billion>>bit)&1)residue+=remainder;
        fraction+=residue/ns;
        residue%=ns;
    }
    const auto base=whole*billion;
    if(fraction>limit-base)return absent<std::uint64_t>("rate overflow");
    return {base+fraction,Support::estimated,"instrumented socket payload bytes/s; not wire throughput"};
}
}
Metric<NetworkAccountingOutput> LinuxNetworkAccounting::sample(const Metric<NetworkAccountingInput>& input){
    auto reject=[&](std::string note){reset();return absent<NetworkAccountingOutput>(std::move(note));};
    if(!metric_is_known(input)){
        reset();
        return {std::nullopt,input.support==Support::permission_required?Support::permission_required:Support::unsupported,
            input.note.empty()?"producer snapshot absent":input.note};
    }
    // An estimated input is not the producer's guarantee of coherent counters.
    if(input.support!=Support::available)return reject("producer snapshot is not verified coherent");
    const auto& now=*input.value;
    if(!valid_scope(now.scope)||!now.observed_at_ns||!now.enumeration_complete||now.rows.size()>max_rows)
        return reject("invalid scope/time, partial enumeration or accounting row limit");
    if(now.scope.coverage==NetworkAccountingCoverage::self_single_socket_namespace&&
        (now.rows.size()!=1||now.rows[0].actor.tgid!=now.scope.producer_tgid||
         now.rows[0].actor.leader_start_boottime_ns!=now.scope.producer_start_boottime_ns))
        return reject("SELF scope must enumerate exactly its creator bucket");
    if(now.scope.coverage==NetworkAccountingCoverage::selected_single_socket_namespace&&
        (now.rows.size()!=1||now.rows[0].actor.tgid>2147483647U||!now.rows[0].actor.leader_start_boottime_ns))
        return reject("Selected scope requires exactly one valid kernel actor bucket, not the system");
    if(now.scope.coverage==NetworkAccountingCoverage::selected_actor_set||now.scope.coverage==NetworkAccountingCoverage::selected_namespace_bucket_set){
        if(now.rows.empty()||now.rows.size()>max_selected_network_actors)return reject("Selected actor set exceeds capacity or is empty");
        std::map<std::pair<std::uint32_t,std::uint64_t>,std::uint32_t> processes;
        for(const auto& row:now.rows)if(!row.actor.tgid||row.actor.tgid>2147483647U||!row.actor.leader_start_boottime_ns||
            (!processes.emplace(std::pair{row.actor.tgid,row.actor.leader_start_boottime_ns},row.actor.real_uid).second&&
             (now.scope.coverage!=NetworkAccountingCoverage::selected_namespace_bucket_set||processes.at({row.actor.tgid,row.actor.leader_start_boottime_ns})!=row.actor.real_uid)))
            return reject("Invalid/duplicate kernel process or inconsistent UID in selected bucket set");
    }
    std::map<NetworkActor,const NetworkAccountingRow*> current,old;
    if(now.scope.coverage==NetworkAccountingCoverage::selected_observed_actor_set){
        std::map<std::pair<std::uint32_t,std::uint64_t>,std::pair<std::uint32_t,std::size_t>> processes;
        for(const auto& row:now.rows){const auto& a=row.actor;
            if(!a.tgid||a.tgid>2147483647U||!a.leader_start_boottime_ns)return reject("Invalid observed kernel actor");
            auto [at,inserted]=processes.emplace(std::pair{a.tgid,a.leader_start_boottime_ns},std::pair{a.real_uid,std::size_t{0}});
            if((!inserted&&at->second.first!=a.real_uid)||++at->second.second>max_network_namespaces_per_actor||processes.size()>max_selected_network_actors)
                return reject("Observed scope exceeds actor/namespace capacity or has inconsistent UID");
        }
    }
    if(now.scope.coverage==NetworkAccountingCoverage::selected_observed_namespace_set){
        if(now.rows.size()>max_network_namespaces_per_actor)return reject("Observed registry exceeds16 buckets");
        for(const auto& row:now.rows){
            const auto& a=row.actor;
            if(a.tgid>2147483647U||!a.leader_start_boottime_ns||
                a.tgid!=now.rows.front().actor.tgid||a.real_uid!=now.rows.front().actor.real_uid||
                a.leader_start_boottime_ns!=now.rows.front().actor.leader_start_boottime_ns)
                return reject("Observed scope requires one selected actor, possibly no activity yet");
        }
    }
    for(const auto& row:now.rows){
        if(!row.actor.tgid||!row.actor.socket_network_namespace_cookie||!current.emplace(row.actor,&row).second)
            return reject("invalid or duplicate actor bucket");
    }
    const bool same=previous_&&previous_->scope==now.scope;
    if(same&&(now.observed_at_ns<=previous_->observed_at_ns||now.lost_updates<previous_->lost_updates))
        return reject("non-monotonic time/loss counter; producer must change epoch on reset");
    const auto elapsed=same?now.observed_at_ns-previous_->observed_at_ns:0;
    const bool window=same&&elapsed<=max_interval_ns&&now.lost_updates==previous_->lost_updates;
    if(window)for(const auto& row:previous_->rows)old.emplace(row.actor,&row);
    NetworkAccountingOutput out{now.scope,now.observed_at_ns,now.lost_updates,{},{}};
    std::map<std::uint32_t,NetworkUidRates> users;
    for(const auto& [key,row]:current){
        NetworkActorRates result;
        result.actor=key;
        result.bytes=usable(*row)?Metric<NetworkCounters>{row->bytes,
            now.lost_updates?Support::estimated:Support::available,
            "instrumented payload since producer start; losses imply lower bound"}:
            absent<NetworkCounters>("incoherent or saturated counter row");
        for(auto& metric:result.bytes_per_second)metric=absent<std::uint64_t>("no matching clean baseline/window");
        result.interval_ns=absent<std::uint64_t>("no matching clean baseline/window");
        const auto found=old.find(key);
        if(usable(*row)&&found!=old.end()&&usable(*found->second)){
            const auto& before=found->second->bytes;
            bool monotonic=true;
            for(std::size_t i=0;i<4;++i)if(row->bytes[i]<before[i])monotonic=false;
            // A reset in any lane invalidates the whole bucket for this interval.
            if(monotonic){
                result.interval_ns={elapsed,Support::available,"producer observation interval, nanoseconds"};
                for(std::size_t i=0;i<4;++i)result.bytes_per_second[i]=rate(row->bytes[i]-before[i],elapsed);
            }
        }
        auto& user=users[key.real_uid];
        user.real_uid=key.real_uid;
        ++user.observed_buckets;
        for(std::size_t i=0;i<4;++i){
            if(!metric_is_known(result.bytes_per_second[i])){++user.missing[i];continue;}
            ++user.known[i];
            if(user.overflow[i])continue;
            auto& sum=user.bytes_per_second[i];
            const auto amount=*result.bytes_per_second[i].value;
            const auto prior=sum.value.value_or(0); // Accumulator only, NOT a missing observation.
            if(amount>std::numeric_limits<std::uint64_t>::max()-prior){
                user.overflow[i]=true;
                sum=absent<std::uint64_t>("UID rate sum overflow");
            }else sum={prior+amount,Support::estimated,"partial instrumented actor-bucket sum; see coverage"};
        }
        out.actors.push_back(std::move(result));
    }
    for(auto& [uid,user]:users){
        (void)uid;
        for(std::size_t i=0;i<4;++i)if(!user.known[i])
            user.bytes_per_second[i]=absent<std::uint64_t>("no measured actor buckets for this UID");
        out.users.push_back(std::move(user));
    }
    previous_=now; // Includes unusable rows: they cannot seed the next interval.
    return {std::move(out),Support::available,"internal reduction only; no live provider or global traffic claim"};
}
}
