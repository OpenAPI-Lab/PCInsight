#include "linux_cpu_usage.hpp"
#include <algorithm>
#include <charconv>
#include <fstream>
#include <limits>
#include <set>
namespace pcinsight::detail {
namespace {
template<class T> bool number(std::string_view text,T& value){if(text.empty())return false;const auto [end,error]=std::from_chars(text.data(),text.data()+text.size(),value);return error==std::errc{}&&end==text.data()+text.size();}
std::optional<std::vector<std::string_view>> fields(std::string_view line){
    if(line.size()>65536)return {};
    std::vector<std::string_view> result;
    while(!line.empty()){
        const auto first=line.find_first_not_of(" \t\r");if(first==line.npos)break;line.remove_prefix(first);
        const auto end=line.find_first_of(" \t\r");result.push_back(line.substr(0,end));
        if(result.size()>11)return {};
        if(end==line.npos)break;
        line.remove_prefix(end);
    }
    return result;
}
bool valid(const LinuxCpuCounters& counters){
    if(counters.fields<4||counters.fields>10||counters.cpu_ids.empty()||counters.cpu_ids.size()>100000||!std::is_sorted(counters.cpu_ids.begin(),counters.cpu_ids.end())||std::adjacent_find(counters.cpu_ids.begin(),counters.cpu_ids.end())!=counters.cpu_ids.end())return false;
    for(unsigned i=counters.fields;i<8;++i)if(counters.ticks[i])return false;
    return true;
}
struct IntervalCheck {
    std::optional<LinuxCpuInterval> interval;
    std::string reason;
};
IntervalCheck check_interval(const std::optional<LinuxCpuCounters>& previous,const std::optional<LinuxCpuCounters>& current){
    if(!current)return {{},"CPU interval unavailable: current counters missing (read/parse failure; exact source error unknown)"};
    if(!previous)return {{},"CPU interval unavailable: previous counters missing (initial sample or baseline reset)"};
    if(!valid(*previous))return {{},"CPU interval unavailable: invalid previous counters"};
    if(!valid(*current))return {{},"CPU interval unavailable: invalid current counters"};
    if(previous->fields!=current->fields)return {{},"CPU interval unavailable: schema changed from "+std::to_string(previous->fields)+" to "+std::to_string(current->fields)};
    if(previous->cpu_ids!=current->cpu_ids)return {{},"CPU interval unavailable: visible CPU IDs changed"};
    if(previous->boot_time!=current->boot_time)return {{},"CPU interval unavailable: boot token changed (value or presence)"};
    constexpr std::array<std::string_view,8> states{"user","nice","system","idle","iowait","irq","softirq","steal"};
    LinuxCpuInterval delta;
    for(unsigned i=0;i<8;++i){
        if(current->ticks[i]<previous->ticks[i])return {{},"CPU interval unavailable: counter decreased: "+std::string(states[i])+" previous="+std::to_string(previous->ticks[i])+" current="+std::to_string(current->ticks[i])};
        const auto change=current->ticks[i]-previous->ticks[i];
        if(change>std::numeric_limits<std::uint64_t>::max()-delta.total)return {{},"CPU interval unavailable: delta sum overflow at "+std::string(states[i])};
        delta.total+=change;
        if(i==3||i==4)delta.idle+=change; // subset of already overflow-checked total
    }
    if(!delta.total)return {{},"CPU interval unavailable: counters did not advance"};
    return {delta,{}};
}
}
std::optional<LinuxCpuCounters> parse_linux_cpu_counters(std::string_view text){
    if(text.empty()||text.size()>16*1024*1024)return {};
    LinuxCpuCounters result;std::set<std::uint32_t> ids;bool aggregate=false,boot=false;
    while(!text.empty()){
        const auto newline=text.find('\n');const auto line=text.substr(0,newline);
        const auto first=line.find_first_not_of(" \t\r");
        if(first!=line.npos){
            const auto tail=line.substr(first);const auto end=tail.find_first_of(" \t\r");const auto label=tail.substr(0,end);
            if(!aggregate&&label!="cpu")return {};
            if(label=="cpu"||label.starts_with("cpu")){
                const auto tokens=fields(line);if(!tokens||tokens->size()<5)return {};
                const unsigned count=static_cast<unsigned>(tokens->size()-1);
                std::array<std::uint64_t,8> ticks{};
                for(unsigned i=0;i<count;++i){std::uint64_t value{};if(!number((*tokens)[i+1],value))return {};if(i<8)ticks[i]=value;}
                if(label=="cpu"){
                    if(aggregate)return {};
                    aggregate=true;result.ticks=ticks;result.fields=count;
                }else{
                    std::uint32_t id{};
                    if(count!=result.fields||!number(label.substr(3),id)||ids.size()>=100000||!ids.insert(id).second)return {};
                }
            }else if(label=="btime"){
                const auto tokens=fields(line);std::uint64_t value{};
                if(boot||!tokens||tokens->size()!=2||!number((*tokens)[1],value))return {};
                boot=true;result.boot_time=value;
            }
        }
        if(newline==text.npos)break;
        text.remove_prefix(newline+1);
    }
    result.cpu_ids.assign(ids.begin(),ids.end());
    if(!aggregate||!valid(result))return {};
    return result;
}
bool valid_linux_cpu_counters(const LinuxCpuCounters& counters){return valid(counters);}
std::optional<LinuxCpuInterval> linux_cpu_interval(const std::optional<LinuxCpuCounters>& previous,const std::optional<LinuxCpuCounters>& current){
    return check_interval(previous,current).interval;
}
Metric<double> linux_cpu_usage(const std::optional<LinuxCpuCounters>& previous,const std::optional<LinuxCpuCounters>& current){
    const auto checked=check_interval(previous,current);
    if(!checked.interval)return {{},Support::unsupported,checked.reason};
    const auto& delta=checked.interval;
    return {static_cast<double>(100.0L*static_cast<long double>(delta->total-delta->idle)/delta->total),Support::estimated,"Scheduler counter ratio across visible CPUs; idle+iowait excluded, steal included, guest not double-counted; not frequency-weighted utility or cgroup quota usage"};
}
std::optional<LinuxCpuCounters> read_linux_cpu_counters(){
#ifdef __linux__
    std::ifstream input("/proc/stat",std::ios::binary);if(!input)return {};
    std::string text;std::array<char,4096> chunk{};
    while(input){input.read(chunk.data(),chunk.size());const auto count=static_cast<std::size_t>(input.gcount());if(count>16*1024*1024-text.size())return {};text.append(chunk.data(),count);}
    if(!input.eof())return {};
    return parse_linux_cpu_counters(text);
#else
    return {};
#endif
}
}
