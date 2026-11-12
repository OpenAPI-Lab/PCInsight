#include "linux_cpu_frequency.hpp"
#include "linux_cpu_cache.hpp"
#include <algorithm>
#include <charconv>
#include <cmath>
#include <fstream>
#include <map>
namespace pcinsight::detail {
namespace {
Metric<double> absent(std::string note){return {{},Support::unsupported,std::move(note)};}
bool valid_ids(const std::vector<std::uint32_t>& ids){return !ids.empty()&&ids.size()<=4096&&std::is_sorted(ids.begin(),ids.end())&&std::adjacent_find(ids.begin(),ids.end())==ids.end();}
using Rows=std::map<std::uint32_t,const LinuxFrequencyRow*>;
std::optional<Rows> index_rows(const LinuxFrequencySnapshot& s){
    if(!valid_ids(s.online)||s.rows.size()!=s.online.size())return {};
    Rows out;
    for(const auto& row:s.rows){
        if(!std::binary_search(s.online.begin(),s.online.end(),row.cpu)||!out.emplace(row.cpu,&row).second)return {};
        for(const auto* f:{&row.hardware_khz,&row.scaling_khz,&row.base_khz})if(*f&&**f==0)return {};
    }
    return out;
}
std::optional<double> mean_ghz(const Rows& rows,bool hardware){
    long double total=0;
    for(const auto& [cpu,row]:rows){
        (void)cpu;const auto value=hardware?row->hardware_khz:row->scaling_khz;
        if(!value)return {};
        total+=static_cast<long double>(*value); // no overflowing uint64 sum
    }
    const auto mean=static_cast<double>(total/static_cast<long double>(rows.size())/1000000.0L);
    if(!std::isfinite(mean)||mean<=0)return {};
    return mean;
}
#ifdef __linux__
std::optional<std::string> read_frequency_text(const std::filesystem::path& path,std::size_t limit){
    std::ifstream in(path,std::ios::binary);if(!in)return {};
    std::string text(limit+1,'\0');in.read(text.data(),static_cast<std::streamsize>(text.size()));
    if(in.bad()||in.gcount()>static_cast<std::streamsize>(limit))return {};
    text.resize(static_cast<std::size_t>(in.gcount()));return text;
}
std::optional<std::uint64_t> read_khz(const std::filesystem::path& path){
    const auto text=read_frequency_text(path,64);return text?parse_linux_frequency_khz(*text):std::nullopt;
}
#endif
}
std::optional<std::uint64_t> parse_linux_frequency_khz(std::string_view text){
    if(text.empty()||text.size()>64)return {};
    const auto first=text.find_first_not_of(" \t\r\n"),last=text.find_last_not_of(" \t\r\n");
    if(first==text.npos)return {};
    text=text.substr(first,last-first+1);std::uint64_t khz{};
    const auto [end,ec]=std::from_chars(text.data(),text.data()+text.size(),khz);
    if(ec!=std::errc{}||end!=text.data()+text.size()||!khz)return {};
    return khz;
}
Metric<LinuxFrequencySnapshot> read_linux_cpu_frequency(const std::filesystem::path& root){
    auto fail=[]()->Metric<LinuxFrequencySnapshot>{return {{},Support::unsupported,"Online CPU inventory unreadable, invalid, changed or over bounds"};};
#ifdef __linux__
    const auto before=read_frequency_text(root/"online",65536);if(!before)return fail();
    const auto online=parse_linux_cache_cpulist(*before);if(!online)return fail();
    LinuxFrequencySnapshot s;s.online=*online;
    for(auto cpu:*online){
        const auto dir=root/("cpu"+std::to_string(cpu))/"cpufreq";
        s.rows.push_back({cpu,read_khz(dir/"cpuinfo_cur_freq"),read_khz(dir/"scaling_cur_freq"),read_khz(dir/"base_frequency")});
    }
    const auto after=read_frequency_text(root/"online",65536);if(!after||parse_linux_cache_cpulist(*after)!=online)return fail();
    return {std::move(s),Support::available,"Bounded per-online-CPU cpufreq reads; individual missing/invalid values retained as unknown"};
#else
    (void)root;return fail();
#endif
}
void apply_linux_cpu_frequency(CpuInfo& cpu,const Metric<LinuxFrequencySnapshot>& first,const Metric<LinuxFrequencySnapshot>& second){
    cpu.current_ghz=absent("No complete same-source frequency coverage for a stable online CPU set");
    cpu.base_ghz=absent("No common, stable base_frequency for all online CPUs; SMBIOS Current Speed/max frequency are not substitutes");
    if(!first.value||!second.value||first.value->online!=second.value->online)return;
    const auto a=index_rows(*first.value),b=index_rows(*second.value);if(!a||!b)return;
    // Frequencies change normally: don't require equal dynamic values between
    // reads. Publish the second read's complete, homogeneous-source sample.
    if(const auto mean=mean_ghz(*b,true))cpu.current_ghz={*mean,Support::estimated,"Arithmetic mean of cpuinfo_cur_freq over all online logical CPUs, sequential hardware-reported readings; not instantaneous/atomic or workload-weighted"};
    else if(const auto requested_mean=mean_ghz(*b,false))cpu.current_ghz={*requested_mean,Support::estimated,"Arithmetic mean of scaling_cur_freq over all online logical CPUs; driver-requested/reported frequency, not proof of actual hardware clock; no mixed-source/partial average"};
    std::optional<std::uint64_t> common;
    for(const auto& [id,row]:*b){
        const auto previous=a->at(id)->base_khz;
        if(!row->base_khz||previous!=row->base_khz||(common&&common!=row->base_khz))return;
        common=row->base_khz;
    }
    cpu.base_ghz={static_cast<double>(static_cast<long double>(*common)/1000000.0L),Support::estimated,"Common cpufreq base_frequency across all online CPUs, agreeing across two reads; configuration value, not current/max clock or direct BIOS verification"};
}
void fill_linux_cpu_frequency(CpuInfo& cpu,const std::filesystem::path& root){
    const auto first=read_linux_cpu_frequency(root),second=read_linux_cpu_frequency(root);
    apply_linux_cpu_frequency(cpu,first,second);
}
}
