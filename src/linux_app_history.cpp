#include "linux_app_history.hpp"
#include <limits>
#include <set>
#include <stdexcept>
namespace pcinsight::detail {
namespace {
bool usable(const LinuxHistoryObservation& o){
    const auto supported=[](auto support){return support==Support::available||support==Support::estimated;};
    if(!o.pid||!o.counters.value||!supported(o.counters.support)||!o.executable.value||!supported(o.executable.support))return false;
    const auto& s=*o.counters.value;const auto& path=o.executable.value->path;
    return s.pid==o.pid&&s.kernel_ticks<=std::numeric_limits<std::uint64_t>::max()-s.user_ticks&&s.ticks==s.user_ticks+s.kernel_ticks&&path.value&&supported(path.support)&&!path.value->empty()&&path.value->front()=='/'&&path.value->find('\0')==path.value->npos;
}
// Exact floor((ticks * 1e9 + carry)/hz), with retained fractional remainder.
// No uint128 dependency and no multiplication of a large fractional numerator.
std::pair<std::uint64_t,std::uint64_t> nanoseconds(std::uint64_t ticks,std::uint64_t hz,std::uint64_t carry){
    constexpr std::uint64_t billion=1000000000;
    const auto max=static_cast<std::uint64_t>(std::numeric_limits<std::chrono::nanoseconds::rep>::max());
    const auto whole=ticks/hz;if(whole>max/billion)throw std::overflow_error("Linux app CPU duration exceeds nanoseconds range");
    auto remainder=ticks%hz;std::uint64_t fraction=0;
    for(unsigned digit=0;digit<9;++digit){
        std::uint64_t rest=0,quotient=0;
        for(unsigned n=0;n<10;++n){if(rest>=hz-remainder){rest-=hz-remainder;++quotient;}else rest+=remainder;}
        fraction=fraction*10+quotient;remainder=rest;
    }
    // hz <= LONG_MAX <= uint64_max/2 on supported platforms.
    if(remainder>=hz-carry){remainder-=hz-carry;++fraction;}else remainder+=carry;
    const auto base=whole*billion;if(fraction>max-base)throw std::overflow_error("Linux app CPU duration exceeds nanoseconds range");
    return {base+fraction,remainder};
}
}
Metric<std::vector<LinuxAppHistoryEntry>> LinuxAppHistory::sample(const std::optional<LinuxCpuCounters>& context,long hz,std::span<const LinuxHistoryObservation> observations,std::chrono::system_clock::time_point now){
    try{
        if(observations.size()>100000)throw std::invalid_argument("Linux history exceeds 100000 observations");
        if(now.time_since_epoch().count()<=0)throw std::invalid_argument("Linux history requires a positive wall-clock timestamp");
        if(initialized_&&now<last_)throw std::invalid_argument("Linux history wall clock moved backward");
        if(!context||!valid_linux_cpu_counters(*context)||hz<=0)throw std::invalid_argument("Linux history needs valid CPU/boot context and CLK_TCK");
        if(initialized_&&(hz_!=hz||boot_!=context->boot_time))throw std::invalid_argument("Linux history units/boot changed: reset capture explicitly");
        const auto start=initialized_?started_:now;
        auto next=rows_;std::map<std::uint32_t,Previous> current;std::set<std::uint32_t> seen;
        std::size_t input=0;
        for(const auto& o:observations){
            if(!o.pid||!seen.insert(o.pid).second)throw std::invalid_argument("Invalid/duplicate Linux history PID");
            const auto path=o.executable.value?o.executable.value->path.value:std::nullopt;
            if(o.name.size()>16384||(path&&path->size()>65536))throw std::invalid_argument("Linux history identity string exceeds bounds");
            const auto cost=o.name.size()+(path?path->size():0);
            if(cost>16*1024*1024-input)throw std::invalid_argument("Linux history identity payload exceeds bounds");
            input+=cost;
            if(!usable(o))continue;
            const auto& id=*path;
            auto found=next.find(id);
            if(found==next.end()){
                if(next.size()>=4096)throw std::invalid_argument("Linux history exceeds 4096 paths: reset capture");
                LinuxAppHistoryEntry row;row.id=id;row.name=o.name.empty()?id:o.name;row.period_start=start;row.period_end=now;
                row.observed_cpu_time.note="Baseline only; no observed CPU interval yet";
                row.network_bytes.note="Per-app Linux network history not collected";row.notifications.note="Linux app notifications not collected";
                found=next.emplace(id,Accumulator{std::move(row),0}).first;
            }
            const auto old=previous_.find(o.pid);
            if(old!=previous_.end()&&o.at>old->second.at&&old->second.path==id&&old->second.executable==o.executable.value->stamp){
                const auto delta=linux_process_tick_delta(old->second.counters,*o.counters.value);
                if(delta){
                    const auto [amount,remainder]=nanoseconds(*delta,static_cast<std::uint64_t>(hz),found->second.remainder);
                    const auto accumulated=found->second.row.observed_cpu_time.value.value_or(std::chrono::nanoseconds{}).count();
                    if(amount>static_cast<std::uint64_t>(std::numeric_limits<std::chrono::nanoseconds::rep>::max()-accumulated))throw std::overflow_error("Linux history accumulated CPU duration overflow");
                    found->second.row.observed_cpu_time={std::chrono::nanoseconds(accumulated+static_cast<std::chrono::nanoseconds::rep>(amount)),Support::estimated,"Sum of consecutive visible-process utime/stime deltas grouped by exact executable path; observed subset only, gaps/missed lifetimes not reconstructed"};
                    found->second.remainder=remainder;found->second.row.period_end=now;
                }
            }
            current.emplace(o.pid,Previous{*o.counters.value,o.executable.value->stamp,id,o.at});
        }
        std::vector<LinuxAppHistoryEntry> output;std::size_t retained=0;
        for(const auto& [id,acc]:next){const auto cost=id.size()+acc.row.id.size()+acc.row.name.size();if(cost>4*1024*1024-retained)throw std::invalid_argument("Linux retained history payload exceeds 4MiB");retained+=cost;output.push_back(acc.row);}
        rows_=std::move(next);previous_=std::move(current);started_=start;last_=now;boot_=context->boot_time;hz_=hz;initialized_=true;
        return {std::move(output),Support::estimated,"Linux CPU observed only while history enabled, grouped by exact visible executable paths across users; missing intervals/metadata may undercount. Network/notifications unknown; no automatic persistence"};
    }catch(const std::invalid_argument& e){break_continuity();return {{},Support::unsupported,e.what()};}
    catch(const std::overflow_error& e){break_continuity();return {{},Support::unsupported,e.what()};}
    catch(...){break_continuity();throw;}
}
}
