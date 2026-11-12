#include "linux_cpu_topology.hpp"
#include "linux_cpu_cache.hpp"
#include <algorithm>
#include <charconv>
#include <fstream>
#include <map>
#include <set>
namespace pcinsight::detail {
namespace {
constexpr std::size_t max_cpus=4096,max_members=1048576;
using Ids=std::vector<std::uint32_t>;
Metric<unsigned> unknown(std::string note){return {{},Support::unsupported,std::move(note)};}
LinuxTopologyCounts missing(){return {unknown("Package topology incomplete/unknown/inconsistent; no socket count inferred"),unknown("Core topology incomplete/unknown/inconsistent; no logical-to-physical fallback"),unknown("Online CPU inventory unavailable, invalid or changed")};}
bool canonical(const Ids& ids){return !ids.empty()&&ids.size()<=max_cpus&&std::is_sorted(ids.begin(),ids.end())&&std::adjacent_find(ids.begin(),ids.end())==ids.end();}
bool member(const Ids& ids,std::uint32_t cpu){return std::binary_search(ids.begin(),ids.end(),cpu);}
// Validate that the lists form a partition of the observed online set. Keep
// offline members in each identity, but don't demand their sysfs rows.
bool partition(const Ids& online,const std::map<std::uint32_t,Ids>& lists){
    if(lists.size()!=online.size())return false;
    std::map<Ids,std::size_t> groups;
    std::map<std::uint32_t,std::size_t> membership;
    for(const auto& [cpu,ids]:lists){
        if(!canonical(ids)||!member(ids,cpu))return false;
        const auto [group,inserted]=groups.emplace(ids,groups.size());(void)inserted;
        membership.emplace(cpu,group->second);
    }
    std::map<std::uint32_t,std::size_t> owners;
    for(const auto& [ids,group]:groups){
        for(auto peer:ids){
            const auto [owner,inserted]=owners.emplace(peer,group);
            if(!inserted&&owner->second!=group)return false; // offline members cannot belong to two distinct groups either
            if(!member(online,peer))continue;
            const auto other=membership.find(peer);if(other==membership.end()||other->second!=group)return false;
        }
    }
    return true;
}
#ifdef __linux__
std::optional<std::string> read_topology_file(const std::filesystem::path& path){
    std::ifstream in(path,std::ios::binary);if(!in)return {};
    std::string text(65537,'\0');in.read(text.data(),static_cast<std::streamsize>(text.size()));
    if(in.bad()||in.gcount()>65536)return {};
    text.resize(static_cast<std::size_t>(in.gcount()));return text;
}
std::optional<Ids> read_list_alias(const std::filesystem::path& dir,const char* preferred,const char* legacy){
    const auto text=read_topology_file(dir/preferred);
    if(text)return parse_linux_cache_cpulist(*text); // Present but malformed is not an excuse to hide it with fallback.
    std::error_code ec;
    if(std::filesystem::exists(dir/preferred,ec)||ec)return {};
    const auto old=read_topology_file(dir/legacy);return old?parse_linux_cache_cpulist(*old):std::nullopt;
}
#endif
}
std::optional<std::int32_t> parse_linux_package_id(std::string_view text){
    if(text.empty()||text.size()>64)return {};
    const auto first=text.find_first_not_of(" \t\r\n"),last=text.find_last_not_of(" \t\r\n");
    if(first==text.npos)return {};
    text=text.substr(first,last-first+1);std::int32_t id{};
    const auto [end,ec]=std::from_chars(text.data(),text.data()+text.size(),id);
    if(ec!=std::errc{}||end!=text.data()+text.size()||id<0||text.front()=='-')return {};
    return id;
}
LinuxTopologyCounts linux_topology_counts(const LinuxTopologySnapshot& snapshot){
    auto out=missing();
    if(!canonical(snapshot.online))return out;
    out.logical={static_cast<unsigned>(snapshot.online.size()),Support::estimated,"Kernel sysfs online logical CPUs; not installed hardware, process affinity or cgroup quota"};
    if(snapshot.rows.size()!=snapshot.online.size())return out;
    std::map<std::uint32_t,Ids> packages,cores;
    std::map<std::uint32_t,std::int32_t> ids;
    std::set<std::uint32_t> seen;
    std::size_t members=0;
    bool packages_complete=true,cores_complete=true;
    for(const auto& row:snapshot.rows){
        if(!member(snapshot.online,row.cpu)||!seen.insert(row.cpu).second)return out;
        for(const auto* list:{&row.package_cpus,&row.core_cpus})if(*list){
            if((*list)->size()>max_members-members)return out;
            members+=(*list)->size();
        }
        if(row.package_id&&*row.package_id>=0&&row.package_cpus){packages.emplace(row.cpu,*row.package_cpus);ids.emplace(row.cpu,*row.package_id);}
        else packages_complete=false;
        if(row.core_cpus)cores.emplace(row.cpu,*row.core_cpus);
        else cores_complete=false;
    }
    if(!packages_complete||!partition(snapshot.online,packages))return out;
    std::map<std::int32_t,Ids> by_package_id;
    std::map<Ids,std::int32_t> by_package_group;
    for(const auto& [cpu,list]:packages){
        const auto id=ids.at(cpu);
        const auto [id_it,new_id]=by_package_id.emplace(id,list);
        const auto [group_it,new_group]=by_package_group.emplace(list,id);
        if((!new_id&&id_it->second!=list)||(!new_group&&group_it->second!=id))return out;
    }
    out.sockets={static_cast<unsigned>(by_package_group.size()),Support::estimated,"Distinct kernel package groups represented by online CPUs with nonnegative physical_package_id; not motherboard socket inventory"};
    if(!cores_complete||!partition(snapshot.online,cores))return out;
    std::set<Ids> unique_cores;
    for(const auto& [cpu,core]:cores){
        const auto& package=packages.at(cpu);
        if(!std::includes(package.begin(),package.end(),core.begin(),core.end()))return out;
        unique_cores.insert(core);
    }
    out.cores={static_cast<unsigned>(unique_cores.size()),Support::estimated,"Distinct kernel core sibling groups represented by online CPUs inside verified package groups; not proof of physical/hidden hardware"};
    return out;
}
Metric<LinuxTopologySnapshot> read_linux_cpu_topology(const std::filesystem::path& root){
    auto fail=[]()->Metric<LinuxTopologySnapshot>{return {{},Support::unsupported,"Online CPU inventory unreadable, malformed, changed or over bounds"};};
#ifdef __linux__
    const auto before=read_topology_file(root/"online");if(!before)return fail();
    const auto online=parse_linux_cache_cpulist(*before);if(!online)return fail();
    LinuxTopologySnapshot out;out.online=*online;std::size_t members=0;
    for(auto cpu:*online){
        LinuxTopologyRow row;row.cpu=cpu;
        const auto dir=root/("cpu"+std::to_string(cpu))/"topology";
        const auto id=read_topology_file(dir/"physical_package_id");if(id)row.package_id=parse_linux_package_id(*id);
        row.package_cpus=read_list_alias(dir,"package_cpus_list","core_siblings_list");
        row.core_cpus=read_list_alias(dir,"core_cpus_list","thread_siblings_list");
        for(const auto* list:{&row.package_cpus,&row.core_cpus})if(*list){
            if((*list)->size()>max_members-members)return fail();
            members+=(*list)->size();
        }
        out.rows.push_back(std::move(row));
    }
    const auto after=read_topology_file(root/"online");if(!after||parse_linux_cache_cpulist(*after)!=online)return fail();
    return {std::move(out),Support::available,"Bounded online CPU topology inventory; missing per-CPU topology preserved as unknown"};
#else
    (void)root;return fail();
#endif
}
void apply_linux_cpu_topology(CpuInfo& cpu,const Metric<LinuxTopologySnapshot>& first,const Metric<LinuxTopologySnapshot>& second){
    auto counts=missing();
    if(first.value&&second.value&&first.value==second.value)counts=linux_topology_counts(*first.value);
    cpu.sockets=std::move(counts.sockets);cpu.physical_cores=std::move(counts.cores);cpu.logical_processors=std::move(counts.logical);
}
void fill_linux_cpu_topology(CpuInfo& cpu,const std::filesystem::path& root){
    const auto first=read_linux_cpu_topology(root),second=read_linux_cpu_topology(root);
    apply_linux_cpu_topology(cpu,first,second);
}
}
