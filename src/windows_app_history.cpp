#include "windows_app_history.hpp"
#include <limits>
#include <set>
#include <stdexcept>
namespace pcinsight::detail {
Metric<std::vector<WindowsAppHistoryEntry>> WindowsAppHistory::sample(const std::vector<ProcessInfo>& processes,std::chrono::system_clock::time_point now){
    try{
        if(processes.size()>10000 || now.time_since_epoch().count()<=0 || (last_.time_since_epoch().count() && now<last_))throw std::invalid_argument("App history sample exceeds bounds or wall clock moved backwards");
        auto next=rows_;std::map<std::uint32_t,Previous> current;std::set<std::uint32_t> seen;std::size_t input=0;
        const auto start=started_.time_since_epoch().count()?started_:now;
        for(const auto& p:processes){
            if(!seen.insert(p.pid).second)throw std::invalid_argument("Duplicate PID in app history sample");
            const auto cost=p.name.size()+p.executable.size();
            if(p.name.size()>16384 || p.executable.size()>32768 || cost>16*1024*1024-input)throw std::invalid_argument("App history process text exceeds bounds");
            input+=cost;
            if(!p.pid || p.executable.empty() || p.executable.find('\0')!=p.executable.npos || !p.windows_creation_time_100ns.value || p.windows_creation_time_100ns.support!=Support::available || !*p.windows_creation_time_100ns.value || !p.windows_cpu_time_100ns.value || p.windows_cpu_time_100ns.support!=Support::available)continue;
            Previous value{*p.windows_creation_time_100ns.value,*p.windows_cpu_time_100ns.value,p.executable};
            auto found=next.find(p.executable);
            if(found==next.end()){
                if(next.size()>=4096)throw std::invalid_argument("App history exceeds 4096 executable paths; reset capture");
                WindowsAppHistoryEntry row;row.id=p.executable;row.name=p.name;row.period_start=start;
                row.observed_cpu_time={std::chrono::nanoseconds(0),Support::estimated,"Sum of consecutive visible-process CPU deltas only; baseline zero is not zero application activity"};
                row.network_bytes.note="Per-application network accounting unavailable";row.notifications.note="Per-application notification accounting unavailable";
                found=next.emplace(p.executable,std::move(row)).first;
            }
            const auto before=previous_.find(p.pid);
            if(before!=previous_.end() && before->second.created==value.created && before->second.path==value.path && value.ticks>=before->second.ticks){
                const auto delta=value.ticks-before->second.ticks;
                const auto max=std::numeric_limits<std::int64_t>::max();const auto accumulated=found->second.observed_cpu_time.value->count();
                if(delta>static_cast<std::uint64_t>((max-accumulated)/100))throw std::invalid_argument("App history cumulative CPU duration overflow");
                *found->second.observed_cpu_time.value+=std::chrono::nanoseconds(static_cast<std::int64_t>(delta*100));
            }
            current.emplace(p.pid,std::move(value));
        }
        std::vector<WindowsAppHistoryEntry> output;std::size_t text=0;
        for(auto& [path,row]:next){
            const auto cost=path.size()+row.id.size()+row.name.size();if(cost>4*1024*1024-text)throw std::invalid_argument("App history retained identity text exceeds 4 MiB");text+=cost;
            row.period_end=now;output.push_back(row);
        }
        rows_=std::move(next);previous_=std::move(current);started_=start;last_=now;
        return {std::move(output),Support::estimated,"Observed CPU deltas grouped by exact executable path across visible users/processes; missed lifetimes/gaps/denials are not reconstructed. Network and notifications unavailable"};
    }catch(const std::invalid_argument& e){previous_.clear();return {{},Support::unsupported,e.what()};}
    catch(...){previous_.clear();throw;}
}
}
