#include "windows_disk_performance.hpp"
#ifdef _WIN32
#include <pdhmsg.h>
#include <cmath>
#include <cstring>
#include <limits>
#include <optional>
#include <set>
namespace pcinsight::detail {
namespace {
Metric<double> missing(const char* operation,PDH_STATUS error) {
    return {std::nullopt,error==ERROR_ACCESS_DENIED||static_cast<DWORD>(error)==PDH_ACCESS_DENIED ?
        Support::permission_required : Support::unsupported,
        std::string(operation)+"; PDH status="+std::to_string(static_cast<unsigned long>(error))};
}
void clear_metrics(DiskInfo& disk,const Metric<double>& error) {
    disk.active_percent=disk.average_response_ms=error;
    disk.read_bytes_per_second=disk.write_bytes_per_second=disk.transfer_bytes_per_second=
        {std::nullopt,error.support,error.note};
}
std::optional<std::uint32_t> disk_number(const std::wstring& name) {
    std::uint64_t value=0; std::size_t pos=0;
    while (pos<name.size() && name[pos]>=L'0' && name[pos]<=L'9') {
        value=value*10+unsigned(name[pos]-L'0');
        if(value>=0xffffffffULL) return std::nullopt;
        ++pos;
    }
    if(!pos || (pos<name.size() && name[pos]!=L' ')) return std::nullopt;
    return static_cast<std::uint32_t>(value);
}
using Values=std::map<std::uint32_t,Metric<double>>;
struct ArrayResult { Values values; Metric<double> error; };
ArrayResult values(DiskPdhApi& api,PDH_HCOUNTER counter) {
    DWORD bytes=0,count=0;
    auto rc=api.format(counter,PDH_FMT_DOUBLE|PDH_FMT_NOCAP100,&bytes,&count,nullptr);
    if(static_cast<DWORD>(rc)!=PDH_MORE_DATA) return {{},missing("disk counter size",rc)};
    if(bytes<sizeof(PDH_FMT_COUNTERVALUE_ITEM_W)||bytes>4*1024*1024)
        return {{},missing("invalid disk counter buffer",PDH_INVALID_DATA)};
    std::vector<unsigned char> buffer(bytes);
    rc=api.format(counter,PDH_FMT_DOUBLE|PDH_FMT_NOCAP100,&bytes,&count,
                  reinterpret_cast<PPDH_FMT_COUNTERVALUE_ITEM_W>(buffer.data()));
    if(rc!=ERROR_SUCCESS) return {{},missing("disk counter array",rc)};
    if(bytes>buffer.size() || count>10000 || count>bytes/sizeof(PDH_FMT_COUNTERVALUE_ITEM_W))
        return {{},missing("invalid disk counter count",PDH_INVALID_DATA)};
    Values out; std::set<std::uint32_t> seen;
    const auto base=reinterpret_cast<std::uintptr_t>(buffer.data());
    for(DWORD i=0;i<count;++i) {
        PDH_FMT_COUNTERVALUE_ITEM_W item{};
        std::memcpy(&item,buffer.data()+i*sizeof item,sizeof item);
        const auto pointer=reinterpret_cast<std::uintptr_t>(item.szName);
        if(pointer<base+count*sizeof item || pointer>=base+bytes || (pointer-base)%alignof(wchar_t))
            return {{},missing("invalid disk instance pointer",PDH_INVALID_DATA)};
        const auto capacity=(bytes-(pointer-base))/sizeof(wchar_t);
        const auto end=std::wmemchr(item.szName,L'\0',capacity);
        if(!end) return {{},missing("unterminated disk instance",PDH_INVALID_DATA)};
        auto number=disk_number(std::wstring(item.szName,end));
        if(!number) continue; // Includes _Total; never attribute it to an individual disk.
        if(!seen.insert(*number).second) {
            out[*number]=missing("ambiguous disk instance",PDH_INVALID_DATA); continue;
        }
        if(item.FmtValue.CStatus!=PDH_CSTATUS_VALID_DATA && item.FmtValue.CStatus!=PDH_CSTATUS_NEW_DATA)
            out[*number]=missing("disk counter data",static_cast<PDH_STATUS>(item.FmtValue.CStatus));
        else if(!std::isfinite(item.FmtValue.doubleValue)||item.FmtValue.doubleValue<0)
            out[*number]=missing("invalid disk counter value",PDH_INVALID_DATA);
        else out[*number]={item.FmtValue.doubleValue,Support::available,"PDH PhysicalDisk counter"};
    }
    return {std::move(out),missing("disk instance not present",PDH_CSTATUS_NO_INSTANCE)};
}
Metric<std::uint64_t> byte_rate(const Metric<double>& metric) {
    if(!metric.value) return {std::nullopt,metric.support,metric.note};
    if(static_cast<long double>(*metric.value)>=std::numeric_limits<std::uint64_t>::max())
        return {std::nullopt,Support::unsupported,"Disk rate exceeds uint64 range"};
    return {static_cast<std::uint64_t>(*metric.value),Support::available,"PDH bytes/s rounded down"};
}
}
void WindowsDiskPerformance::reset() {
    if(query_) api_.close(query_);
    query_=nullptr; counters_={}; primed_=false; identities_.clear();
}
void WindowsDiskPerformance::fill(std::vector<DiskInfo>& disks,const std::map<std::uint32_t,std::string>& mapping) {
    const auto initial=missing("Requires two disk counter samples and a unique device number",PDH_INVALID_DATA);
    for(auto& disk:disks) clear_metrics(disk,initial);
    if(mapping.empty()) { reset(); return; }
    if(mapping!=identities_) { reset(); identities_=mapping; }
    if(!query_) {
        auto rc=api_.open(nullptr,0,&query_);
        if(rc!=ERROR_SUCCESS) { reset(); for(auto& d:disks) clear_metrics(d,missing("PdhOpenQuery disks",rc)); return; }
        constexpr const wchar_t* paths[]{L"\\PhysicalDisk(*)\\Disk Read Bytes/sec",L"\\PhysicalDisk(*)\\Disk Write Bytes/sec",
            L"\\PhysicalDisk(*)\\% Idle Time",L"\\PhysicalDisk(*)\\Avg. Disk sec/Transfer"};
        for(unsigned i=0;i<4;++i) {
            rc=api_.add(query_,paths[i],0,&counters_[i]);
            if(rc!=ERROR_SUCCESS) { reset(); for(auto& d:disks) clear_metrics(d,missing("PdhAddEnglishCounter disks",rc)); return; }
        }
    }
    auto rc=api_.collect(query_);
    if(rc!=ERROR_SUCCESS) { reset(); for(auto& d:disks) clear_metrics(d,missing("PdhCollect disks",rc)); return; }
    if(!primed_) { primed_=true; return; }
    std::array<ArrayResult,4> results;
    for(unsigned i=0;i<4;++i) results[i]=values(api_,counters_[i]);
    std::map<std::string,std::uint32_t> reverse;
    for(const auto& [number,path]:mapping) reverse.emplace(path,number);
    for(auto& disk:disks) {
        auto found=reverse.find(disk.id); if(found==reverse.end()) continue;
        std::array<Metric<double>,4> m;
        for(unsigned i=0;i<4;++i) {
            const auto it=results[i].values.find(found->second);
            m[i]=it==results[i].values.end()?results[i].error:it->second;
        }
        disk.read_bytes_per_second=byte_rate(m[0]); disk.write_bytes_per_second=byte_rate(m[1]);
        if(disk.read_bytes_per_second.value && disk.write_bytes_per_second.value &&
            *disk.write_bytes_per_second.value<=std::numeric_limits<std::uint64_t>::max()-*disk.read_bytes_per_second.value)
            disk.transfer_bytes_per_second={*disk.read_bytes_per_second.value+*disk.write_bytes_per_second.value,
                Support::available,"Sum of reported read/write byte rates"};
        else if(!disk.read_bytes_per_second.value) disk.transfer_bytes_per_second=
            {std::nullopt,disk.read_bytes_per_second.support,disk.read_bytes_per_second.note};
        else if(!disk.write_bytes_per_second.value) disk.transfer_bytes_per_second=
            {std::nullopt,disk.write_bytes_per_second.support,disk.write_bytes_per_second.note};
        else disk.transfer_bytes_per_second={std::nullopt,Support::unsupported,"Total disk byte rate overflow"};
        if(m[2].value && *m[2].value<=100)
            disk.active_percent={100-*m[2].value,Support::available,"100 minus PDH % Idle Time"};
        else disk.active_percent=m[2].value?missing("Idle percentage outside 0..100",PDH_INVALID_DATA):m[2];
        if(m[3].value && *m[3].value<=std::numeric_limits<double>::max()/1000)
            disk.average_response_ms={*m[3].value*1000,Support::available,"PDH Avg. Disk sec/Transfer converted to ms"};
        else disk.average_response_ms=m[3].value?missing("Disk latency overflow",PDH_INVALID_DATA):m[3];
    }
}
}
#endif
