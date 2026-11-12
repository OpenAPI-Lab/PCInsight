#include "linux_memory.hpp"
#include <array>
#include <charconv>
#include <fstream>
#include <limits>
namespace pcinsight::detail {
namespace {
bool tracked(std::string_view key){
    for(auto name:{"MemTotal:","MemAvailable:","Cached:","SReclaimable:","Committed_AS:","CommitLimit:","Zswap:",
        "MemFree:","Buffers:","AnonPages:","Shmem:","SUnreclaim:","Dirty:","Writeback:","Zswapped:"})if(key==name)return true;
    return false;
}
std::vector<std::string_view> fields(std::string_view text){
    std::vector<std::string_view> out;
    while(!text.empty()){
        const auto first=text.find_first_not_of(" \t\r");if(first==text.npos)break;text.remove_prefix(first);
        const auto end=text.find_first_of(" \t\r");out.push_back(text.substr(0,end));
        if(out.size()>3||end==text.npos)break;
        text.remove_prefix(end);
    }
    return out;
}
Metric<std::uint64_t> missing(std::string note){return {{},Support::unsupported,std::move(note)};}
}
Metric<LinuxMeminfo> parse_linux_meminfo(std::string_view text){
    auto fail=[](std::string note)->Metric<LinuxMeminfo>{return {{},Support::unsupported,std::move(note)};};
    if(text.empty()||text.size()>1024*1024)return fail("Empty/oversized meminfo inventory");
    LinuxMeminfo out;unsigned rows=0;
    while(!text.empty()){
        const auto newline=text.find('\n');const auto line=text.substr(0,newline);
        if(++rows>10000||line.size()>65536||line.find('\0')!=line.npos)return fail("Invalid meminfo text/bounds");
        const auto values=fields(line);
        if(!values.empty()&&tracked(values[0])){
            std::uint64_t value{};
            if(values.size()!=3||values[2]!="kB")return fail("Invalid units/fields in selected meminfo row");
            const auto [end,error]=std::from_chars(values[1].data(),values[1].data()+values[1].size(),value);
            if(error!=std::errc{}||end!=values[1].data()+values[1].size()||value>std::numeric_limits<std::uint64_t>::max()/1024)return fail("Invalid/overflowing meminfo byte count");
            const auto key=std::string(values[0].substr(0,values[0].size()-1));
            if(!out.emplace(key,value*1024).second)return fail("Duplicate selected meminfo field");
        }
        if(newline==text.npos)break;
        text.remove_prefix(newline+1);
    }
    return {std::move(out),Support::available,"Selected kernel meminfo fields converted from kB (1024 bytes)"};
}
void apply_linux_meminfo(MemoryInfo& memory,const Metric<LinuxMeminfo>& input){
    memory.linux_details={}; // No stale per-field values, zram inventory or firmware difference.
    memory.hardware_reserved_bytes=missing("Requires firmware and usable RAM; not a direct hardware-reservation measurement");
    const auto get=[&](const char* key)->Metric<std::uint64_t>{
        if(!input.value||(input.support!=Support::available&&input.support!=Support::estimated))return {{},input.support==Support::permission_required?Support::permission_required:Support::unsupported,input.note.empty()?"Meminfo source unavailable":input.note};
        const auto found=input.value->find(key);
        if(found==input.value->end())return missing(std::string("Meminfo field unavailable: ")+key);
        return {found->second,input.support,std::string("Kernel meminfo ")+key+" in bytes"};
    };
    memory.reported_total_bytes=get("MemTotal");
    if(memory.reported_total_bytes.value&&!*memory.reported_total_bytes.value)memory.reported_total_bytes=missing("MemTotal is zero; no usable RAM total established");
    memory.total_bytes=memory.reported_total_bytes.value.value_or(0); // legacy placeholder, not a measured zero
    memory.available_bytes=get("MemAvailable");
    if(memory.available_bytes.value){memory.available_bytes.support=Support::estimated;memory.available_bytes.note="Kernel MemAvailable estimate; not MemFree or cgroup remaining quota";}
    memory.in_use_bytes=missing("Requires valid MemTotal and MemAvailable");
    memory.utilization_percent={{},Support::unsupported,"Requires valid positive MemTotal and MemAvailable"};
    if(memory.reported_total_bytes.value&&memory.available_bytes.value){
        const auto total=*memory.reported_total_bytes.value,available=*memory.available_bytes.value;
        if(available>total){memory.available_bytes=missing("MemAvailable exceeds MemTotal; inconsistent sample");memory.in_use_bytes=missing("MemAvailable exceeds MemTotal; cannot subtract");memory.utilization_percent.note=memory.in_use_bytes.note;}
        else{
            const auto used=total-available;
            memory.in_use_bytes={used,Support::estimated,"MemTotal minus kernel MemAvailable estimate; not sum of process RSS"};
            memory.utilization_percent={static_cast<double>(100.0L*used/total),Support::estimated,"100*(MemTotal-MemAvailable)/MemTotal; estimate, not Windows composition"};
        }
    }
    const auto cached=get("Cached"),reclaimable=get("SReclaimable");
    memory.cached_bytes=missing("Requires both Cached and SReclaimable; no missing-field zero fallback");
    if(cached.value&&reclaimable.value){
        if(*reclaimable.value>std::numeric_limits<std::uint64_t>::max()-*cached.value)memory.cached_bytes=missing("Cached + SReclaimable exceeds uint64 range");
        else{
            const auto value=*cached.value+*reclaimable.value;
            if(memory.reported_total_bytes.value&&value>*memory.reported_total_bytes.value)memory.cached_bytes=missing("Cached + SReclaimable exceeds MemTotal; inconsistent sample");
            else memory.cached_bytes={value,Support::estimated,"Cached + SReclaimable only; excludes Buffers and is not Windows Cached"};
        }
    }
    memory.committed_bytes=get("Committed_AS");memory.commit_limit_bytes=get("CommitLimit");
    memory.compressed_bytes=get("Zswap");
    if(memory.compressed_bytes.value){memory.compressed_bytes.support=Support::estimated;memory.compressed_bytes.note="Zswap backend compressed-size pool only; not all compressed RAM, zram, or Windows compressed memory";}
    memory.paged_pool_bytes=missing("Windows paged pool has no equivalent provided by this Linux source");
    memory.nonpaged_pool_bytes=missing("Windows nonpaged pool has no equivalent provided by this Linux source");
    auto& details=memory.linux_details;
    details.free_bytes=get("MemFree");details.buffers_bytes=get("Buffers");
    details.page_cache_bytes=get("Cached");details.anonymous_bytes=get("AnonPages");
    details.shared_bytes=get("Shmem");details.slab_reclaimable_bytes=get("SReclaimable");
    details.slab_unreclaimable_bytes=get("SUnreclaim");details.dirty_bytes=get("Dirty");details.writeback_bytes=get("Writeback");
    details.zswap_pool_bytes=get("Zswap");details.zswap_original_bytes=get("Zswapped");
    details.zram_devices.note="Linux zram inventory not queried by meminfo parser";
    details.firmware_installed_bytes.note="Firmware installed RAM not supplied";
    details.installed_minus_usable_bytes.note="Requires firmware installed RAM and valid kernel MemTotal";
    details.composition.note="Requires MemTotal > 0 and MemFree <= MemAvailable <= MemTotal; no missing-field zero fallback";
    if(memory.reported_total_bytes.value&&memory.available_bytes.value&&details.free_bytes.value){
        const auto total=*memory.reported_total_bytes.value,available=*memory.available_bytes.value,free=*details.free_bytes.value;
        if(free<=available&&available<=total)
            details.composition={LinuxMemoryComposition{total-available,available-free,free},Support::estimated,
                "Linux estimate partition: MemTotal-MemAvailable, MemAvailable-MemFree, MemFree; not Windows composition. Raw categories overlap"};
        else details.composition.note="MemFree/MemAvailable/MemTotal inconsistent; no composition published";
    }
}
void apply_linux_memory_reservation(MemoryInfo& memory,const Metric<std::uint64_t>& installed){
    auto& details=memory.linux_details;
    const auto valid=[](const auto& m){return m.value&&(m.support==Support::available||m.support==Support::estimated);};
    details.firmware_installed_bytes=installed;
    if(!valid(installed)){
        details.firmware_installed_bytes.value.reset();
        if(installed.support!=Support::permission_required)details.firmware_installed_bytes.support=Support::unsupported;
    }
    details.installed_minus_usable_bytes=missing("Requires valid firmware installed RAM >= usable kernel MemTotal; not direct hardware-reservation measurement");
    if(installed.support==Support::permission_required){details.installed_minus_usable_bytes.support=Support::permission_required;details.installed_minus_usable_bytes.note=installed.note;}
    else if(memory.reported_total_bytes.support==Support::permission_required){
        details.installed_minus_usable_bytes.support=Support::permission_required;
        details.installed_minus_usable_bytes.note=memory.reported_total_bytes.note.empty()?
            "Usable kernel MemTotal requires permission; no installed-minus-usable estimate":memory.reported_total_bytes.note;
    }
    if(valid(details.firmware_installed_bytes)&&valid(memory.reported_total_bytes)&&*memory.reported_total_bytes.value>0){
        if(*installed.value>=*memory.reported_total_bytes.value)
            details.installed_minus_usable_bytes={*installed.value-*memory.reported_total_bytes.value,Support::estimated,
                "Firmware installed RAM minus kernel MemTotal: includes kernel/boot limits/offline RAM and reservations; NOT hardware-only reserved bytes"};
        else details.installed_minus_usable_bytes.note="Firmware installed RAM below kernel MemTotal; inconsistent sources, no subtraction";
    }
    memory.hardware_reserved_bytes=details.installed_minus_usable_bytes; // Legacy estimated proxy only.
}
Metric<LinuxMeminfo> read_linux_meminfo(){
#ifdef __linux__
    std::ifstream input("/proc/meminfo",std::ios::binary);if(!input)return {{},Support::unsupported,"Meminfo source unavailable"};
    std::string text;std::array<char,4096> chunk{};
    while(input){input.read(chunk.data(),chunk.size());const auto count=static_cast<std::size_t>(input.gcount());if(count>1024*1024-text.size())return {{},Support::unsupported,"Meminfo exceeds 1 MiB"};text.append(chunk.data(),count);}
    if(!input.eof())return {{},Support::unsupported,"Meminfo read failed"};
    return parse_linux_meminfo(text);
#else
    return {{},Support::unsupported,"Linux meminfo source unsupported on this platform"};
#endif
}
}
