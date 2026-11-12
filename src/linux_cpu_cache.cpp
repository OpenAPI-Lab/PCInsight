#include "linux_cpu_cache.hpp"
#include <algorithm>
#include <charconv>
#include <fstream>
#include <limits>
#include <map>
#include <set>
namespace pcinsight::detail {
namespace {
constexpr std::size_t max_cpus=4096, max_entries=65536, max_shared_ids=1048576;
std::string_view trim_cache(std::string_view s){
    const auto first=s.find_first_not_of(" \t\r\n");if(first==s.npos)return {};
    const auto last=s.find_last_not_of(" \t\r\n");return s.substr(first,last-first+1);
}
template<class T> bool number(std::string_view s,T& n){
    if(s.empty())return false;
    auto [end,ec]=std::from_chars(s.data(),s.data()+s.size(),n);
    return ec==std::errc{}&&end==s.data()+s.size();
}
bool canonical(const std::vector<std::uint32_t>& ids){
    return !ids.empty()&&ids.size()<=max_cpus&&std::is_sorted(ids.begin(),ids.end())&&std::adjacent_find(ids.begin(),ids.end())==ids.end();
}
bool cache_type(const std::string& s){return s=="Data"||s=="Instruction"||s=="Unified";}
Metric<std::uint64_t> missing_cache(std::string s){return {{},Support::unsupported,std::move(s)};}
#ifdef __linux__
std::optional<std::string> read_cache_file(const std::filesystem::path& path){
    std::ifstream in(path,std::ios::binary);if(!in)return {};
    std::string text(65537,'\0');in.read(text.data(),static_cast<std::streamsize>(text.size()));
    if(in.bad()||in.gcount()>65536)return {};
    text.resize(static_cast<std::size_t>(in.gcount()));return text;
}
#endif
}
std::optional<std::vector<std::uint32_t>> parse_linux_cache_cpulist(std::string_view text){
    if(text.size()>65536)return {};
    text=trim_cache(text);if(text.empty())return {};
    std::vector<std::uint32_t> ids;
    while(!text.empty()){
        const auto comma=text.find(',');const auto token=text.substr(0,comma);const auto dash=token.find('-');
        std::uint32_t first{},last{};
        if(!number(token.substr(0,dash),first))return {};
        last=first;
        if(dash!=token.npos&&!number(token.substr(dash+1),last))return {};
        if(last<first||static_cast<std::uint64_t>(last)-first+1>max_cpus-ids.size())return {};
        // Accept any ordering but reject duplicate/overlapping ranges below.
        for(std::uint64_t i=first;i<=last;++i)ids.push_back(static_cast<std::uint32_t>(i));
        if(comma==text.npos)break;
        text.remove_prefix(comma+1);if(text.empty())return {};
    }
    std::sort(ids.begin(),ids.end());if(!canonical(ids))return {};return ids;
}
std::optional<std::uint64_t> parse_linux_cache_size(std::string_view text){
    if(text.size()>64)return {};
    text=trim_cache(text);if(text.size()<2||text.back()!='K')return {};
    text.remove_suffix(1);std::uint64_t kib{};
    if(!number(text,kib)||!kib||kib>std::numeric_limits<std::uint64_t>::max()/1024)return {};
    return kib*1024;
}
std::array<Metric<std::uint64_t>,3> linux_cache_totals(const LinuxCacheSnapshot& s){
    std::array<Metric<std::uint64_t>,3> out;
    for(auto& metric:out)metric=missing_cache("Incomplete or inconsistent sysfs cache level; no total inferred");
    if(!canonical(s.online)||s.entries.size()>max_entries)return out;
    // Invalid identities could conceal a level: do not salvage a partial inventory.
    std::size_t shared_ids=0;
    for(const auto& e:s.entries){
        if(!e.level||e.level>64||!std::binary_search(s.online.begin(),s.online.end(),e.cpu)||e.shared.size()>max_shared_ids-shared_ids)return out;
        shared_ids+=e.shared.size();
    }
    for(std::uint32_t level=1;level<=3;++level){
        using Key=std::pair<std::string,std::vector<std::uint32_t>>;
        std::map<Key,std::uint64_t> instances;
        std::map<std::pair<std::uint32_t,std::string>,Key> coverage;
        std::map<std::uint32_t,std::set<std::string>> types;
        bool valid=true;
        for(const auto& e:s.entries){
            if(e.level!=level)continue;
            if(!cache_type(e.type)||!e.bytes||!canonical(e.shared)||!std::binary_search(e.shared.begin(),e.shared.end(),e.cpu)){valid=false;break;}
            Key key{e.type,e.shared};
            if(!coverage.emplace(std::make_pair(e.cpu,e.type),key).second){valid=false;break;}
            types[e.cpu].insert(e.type);
            const auto [it,inserted]=instances.emplace(key,e.bytes);
            if(!inserted&&it->second!=e.bytes){valid=false;break;}
        }
        if(!valid||instances.empty()||types.size()!=s.online.size())continue;
        const auto& signature=types.begin()->second;
        if(signature.contains("Unified")&&signature.size()!=1)continue;
        for(const auto& [cpu,seen]:types){(void)cpu;if(seen!=signature)valid=false;}
        for(const auto& [key,bytes]:instances){
            (void)bytes;
            for(const auto cpu:key.second){
                if(!std::binary_search(s.online.begin(),s.online.end(),cpu))continue;
                const auto it=coverage.find({cpu,key.first});
                if(it==coverage.end()||it->second!=key)valid=false;
            }
        }
        std::uint64_t total=0;
        for(const auto& [key,bytes]:instances){
            (void)key;
            if(bytes>std::numeric_limits<std::uint64_t>::max()-total){valid=false;break;}
            total+=bytes;
        }
        if(valid)out[level-1]={total,Support::estimated,"Distinct sysfs cache instances reachable by online CPUs; split data/instruction summed, sharing deduplicated; not offline/hidden hardware or an atomic topology snapshot"};
    }
    return out;
}
Metric<LinuxCacheSnapshot> read_linux_cache_snapshot(const std::filesystem::path& root){
    auto fail=[]()->Metric<LinuxCacheSnapshot>{return {{},Support::unsupported,"Cache sysfs unreadable, malformed, incomplete, changed or over bounds"};};
#ifdef __linux__
    const auto online_text=read_cache_file(root/"online");if(!online_text)return fail();
    const auto online=parse_linux_cache_cpulist(*online_text);if(!online)return fail();
    LinuxCacheSnapshot s;s.online=*online;std::size_t shared_ids=0;
    for(const auto cpu:s.online){
        std::error_code ec;const auto cache=root/("cpu"+std::to_string(cpu))/"cache";
        std::filesystem::directory_iterator it(cache,ec),end;if(ec)return fail();
        std::size_t count=0,visited=0;
        for(;it!=end;it.increment(ec)){
            if(ec)return fail();
            if(++visited>128)return fail();
            const auto name=it->path().filename().string();
            if(name.rfind("index",0)!=0)continue;
            unsigned index{};if(!number(std::string_view(name).substr(5),index)||++count>64||s.entries.size()>=max_entries)return fail();
            const auto level_text=read_cache_file(it->path()/"level"),type=read_cache_file(it->path()/"type"),size=read_cache_file(it->path()/"size"),shared=read_cache_file(it->path()/"shared_cpu_list");
            if(!level_text||!type||!size||!shared)return fail();
            LinuxCacheEntry e;e.cpu=cpu;
            const auto bytes=parse_linux_cache_size(*size);const auto ids=parse_linux_cache_cpulist(*shared);
            if(!number(trim_cache(*level_text),e.level)||!e.level||e.level>64||!bytes||!ids||ids->size()>max_shared_ids-shared_ids)return fail();
            shared_ids+=ids->size();
            e.type=trim_cache(*type);if(!cache_type(e.type))return fail();
            e.bytes=*bytes;e.shared=*ids;s.entries.push_back(std::move(e));
        }
        if(ec||!count)return fail();
    }
    const auto after=read_cache_file(root/"online");if(!after||parse_linux_cache_cpulist(*after)!=online)return fail();
    std::sort(s.entries.begin(),s.entries.end());
    return {std::move(s),Support::available,"Bounded sysfs cache inventory"};
#else
    (void)root;return fail();
#endif
}
void fill_linux_cpu_cache(CpuInfo& cpu,const std::filesystem::path& root){
    auto totals=std::array<Metric<std::uint64_t>,3>{};
    const auto first=read_linux_cache_snapshot(root),second=read_linux_cache_snapshot(root);
    if(first.value&&second.value&&first.value==second.value)totals=linux_cache_totals(*first.value);
    else for(auto& m:totals)m=missing_cache("Cache inventory unavailable or changed across two reads; no partial total");
    cpu.l1_bytes=std::move(totals[0]);cpu.l2_bytes=std::move(totals[1]);cpu.l3_bytes=std::move(totals[2]);
}
}
