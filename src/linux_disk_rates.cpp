#include "linux_disk_rates.hpp"
#include <algorithm>
#include <charconv>
#include <cmath>
#include <stdexcept>
namespace pcinsight::detail {
namespace {
bool space(char c){return c==' '||c=='\t'||c=='\r'||c=='\n';}
template<class T> bool number(std::string_view s,T& value){
    if(s.empty())return false;
    const auto [end,error]=std::from_chars(s.data(),s.data()+s.size(),value);
    return error==std::errc{} && end==s.data()+s.size();
}
Metric<std::uint64_t> byte_rate(long double sectors,long double seconds){
    const auto value=sectors*512.0L/seconds;
    if(!std::isfinite(value)||value>=std::ldexp(1.0L,64))return {{},Support::unsupported,"Disk byte rate exceeds uint64 range"};
    return {static_cast<std::uint64_t>(value),Support::estimated,"Block-sector delta (512 bytes) / monotonic seconds, rounded down"};
}
}
std::optional<LinuxDiskStat> parse_linux_disk_stat(std::string_view text){
    if(text.size()>16384)return {};
    LinuxDiskStat result{};unsigned count=0;
    while(!text.empty()){
        while(!text.empty()&&space(text.front()))text.remove_prefix(1);
        if(text.empty())break;
        const auto end=text.find_first_of(" \t\r\n");
        const auto token=text.substr(0,end);std::uint64_t value{};
        if(count>=64||!number(token,value))return {};
        if(count<11)result[count]=value;
        ++count;
        if(end==text.npos)break;
        text.remove_prefix(end);
    }
    if(count<11)return {};
    return result;
}
bool valid_linux_disk_device(std::string_view text){
    const auto colon=text.find(':');
    if(colon==text.npos)return false;
    std::uint32_t major{},minor{};
    return number(text.substr(0,colon),major)&&number(text.substr(colon+1),minor);
}
std::map<std::string,LinuxDiskRates> LinuxDiskRateTracker::sample(std::span<const LinuxDiskCounters> rows,std::chrono::steady_clock::time_point now){
    try{
        if(rows.size()>100000)throw std::invalid_argument("Too many disk rows");
        std::map<std::string,Previous> next;std::map<std::string,LinuxDiskRates> output;
        for(const auto& row:rows){
            if(row.name.empty()||row.name.size()>4096||row.name.find('\0')!=row.name.npos||!valid_linux_disk_device(row.device)||next.contains(row.name))throw std::invalid_argument("Invalid/duplicate disk identity");
            LinuxDiskRates rates;
            rates.read.note=rates.write.note=rates.transfer.note=rates.active.note=rates.response.note=
                "Requires consecutive samples with same block identity and nondecreasing cumulative counters";
            const auto found=previous_.find(row.name);
            if(found!=previous_.end()){
                const auto& old=found->second;
                const auto seconds=std::chrono::duration<long double>(now.time_since_epoch()).count()-std::chrono::duration<long double>(old.at.time_since_epoch()).count();
                bool monotonic=true;
                for(unsigned i=0;i<11;++i)if(i!=8&&row.stat[i]<old.counters.stat[i])monotonic=false;
                if(now>old.at&&seconds>0&&std::isfinite(seconds)&&row.device==old.counters.device&&row.disk_sequence==old.counters.disk_sequence&&monotonic){
                    const auto& a=row.stat;const auto& b=old.counters.stat;
                    const auto read=static_cast<long double>(a[2]-b[2]),write=static_cast<long double>(a[6]-b[6]);
                    rates.read=byte_rate(read,seconds);rates.write=byte_rate(write,seconds);rates.transfer=byte_rate(read+write,seconds);
                    rates.active={static_cast<double>(std::min(100.0L,static_cast<long double>(a[9]-b[9])/(seconds*10.0L))),Support::estimated,"io_ticks delta / elapsed time, capped at 100%; not hardware saturation"};
                    const auto operations=static_cast<long double>(a[0]-b[0])+static_cast<long double>(a[4]-b[4]);
                    if(operations>0)rates.response={static_cast<double>((static_cast<long double>(a[3]-b[3])+static_cast<long double>(a[7]-b[7]))/operations),Support::estimated,"Read+write request milliseconds / completed reads+writes; excludes discard/flush"};
                    else rates.response.note="No completed reads/writes in interval; average response is undefined";
                }
            }
            output.emplace(row.name,std::move(rates));next.emplace(row.name,Previous{row,now});
        }
        previous_=std::move(next);return output;
    }catch(...){reset();throw;}
}
}
