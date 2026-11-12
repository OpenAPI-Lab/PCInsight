#include "linux_process_cpu.hpp"
#include <cmath>
#include <limits>
#include <stdexcept>
namespace pcinsight::detail {
namespace {
Metric<double> missing(std::string why){return {{},Support::unsupported,std::move(why)};}
bool usable(const LinuxProcessCpuObservation& row){
    if(!row.counters.value||(row.counters.support!=Support::available&&row.counters.support!=Support::estimated))return false;
    const auto& stat=*row.counters.value;
    return row.pid&&stat.pid==row.pid&&stat.kernel_ticks<=std::numeric_limits<std::uint64_t>::max()-stat.user_ticks&&stat.ticks==stat.user_ticks+stat.kernel_ticks;
}
}
std::map<std::uint32_t,Metric<double>> LinuxProcessCpuTracker::sample(const std::optional<LinuxCpuCounters>& cpu,long hz,std::span<const LinuxProcessCpuObservation> rows){
    try{
        if(rows.size()>100000)throw std::invalid_argument("Too many process CPU observations");
        const bool current_valid=cpu&&hz>0&&valid_linux_cpu_counters(*cpu);
        const bool continuous=current_valid&&hz==previous_hz_&&linux_cpu_interval(previous_cpu_,cpu).has_value();
        std::map<std::uint32_t,LinuxProcessCpuObservation> next;
        std::map<std::uint32_t,Metric<double>> out;
        for(const auto& row:rows){
            if(!row.pid||out.contains(row.pid))throw std::invalid_argument("Invalid/duplicate process CPU PID");
            auto result=missing("Requires consecutive readable samples, same PID/start, CPU context and valid clock-tick rate");
            if(!usable(row)){
                if(row.counters.support==Support::permission_required)result={{},Support::permission_required,row.counters.note};
                else result=missing("Process CPU counters absent, untrusted, mismatched PID or overflowing/inconsistent sum");
            }else if(current_valid){
                const auto old=previous_.find(row.pid);
                if(continuous&&old!=previous_.end()){
                    const auto& a=*old->second.counters.value;const auto& b=*row.counters.value;
                    const long double seconds=std::chrono::duration<long double>(row.at.time_since_epoch()).count()-std::chrono::duration<long double>(old->second.at.time_since_epoch()).count();
                    if(b.start_ticks==a.start_ticks&&b.user_ticks>=a.user_ticks&&b.kernel_ticks>=a.kernel_ticks&&row.at>old->second.at&&std::isfinite(seconds)&&seconds>0){
                        const long double delta=static_cast<long double>(b.user_ticks-a.user_ticks)+static_cast<long double>(b.kernel_ticks-a.kernel_ticks);
                        const long double percentage=(delta/static_cast<long double>(hz)/seconds/static_cast<long double>(cpu->cpu_ids.size()))*100.0L;
                        if(std::isfinite(percentage)&&percentage>=0&&percentage<=100.0L)result={static_cast<double>(percentage),Support::estimated,"100 * process (utime+stime) delta / CLK_TCK / own monotonic elapsed seconds / visible proc-stat CPU count; host-capacity normalized, not cgroup quota, affinity or frequency-weighted utility; sequential quantized reads"};
                        else result=missing("Process CPU ratio outside 0..100 or not finite; no clamp");
                    }else result=missing("Process identity changed, individual counter decreased or sample clock did not advance");
                }
                next.emplace(row.pid,row);
            }
            out.emplace(row.pid,std::move(result));
        }
        previous_=std::move(next);previous_cpu_=current_valid?cpu:std::nullopt;previous_hz_=current_valid?hz:0;
        return out;
    }catch(...){reset();throw;}
}
}
