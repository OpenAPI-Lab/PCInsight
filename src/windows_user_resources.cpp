#include "windows_user_resources.hpp"
#include <charconv>
#include <cmath>
#include <limits>
#include <map>
#include <set>
#include <stdexcept>
#include <type_traits>
namespace pcinsight::detail {
namespace {
void text(const std::string& s,std::size_t& bytes){
    if(s.size()>32768 || s.find('\0')!=s.npos || s.size()>16*1024*1024-bytes)throw std::invalid_argument("WTS resource identity text exceeds bounds");
    bytes+=s.size();
}
std::uint32_t session_id(const std::string& id){
    if(!id.starts_with("wts:"))throw std::invalid_argument("WTS resource row has no WTS ID");
    std::uint32_t n=0;const auto begin=id.data()+4,end=id.data()+id.size();
    const auto parsed=std::from_chars(begin,end,n);
    if(parsed.ec!=std::errc{} || parsed.ptr!=end || n==0xffffffffU || id!="wts:"+std::to_string(n))throw std::invalid_argument("WTS resource row has invalid/reserved ID");
    return n;
}
template<class T> struct Sum {
    T total{};std::size_t known{},missing{};bool denied{},overflow{};
    void add(const Metric<T>& m){
        if(!m.value || (m.support!=Support::available && m.support!=Support::estimated)){++missing;denied|=m.support==Support::permission_required;return;}
        const auto v=*m.value;
        if constexpr(std::is_floating_point_v<T>){
            if(!std::isfinite(v) || v<0 || v>100){++missing;return;}
            if(v>100-total)overflow=true;
        }else if(v>std::numeric_limits<T>::max()-total)overflow=true;
        ++known;if(!overflow)total+=v;
    }
    Metric<T> result(std::size_t unassigned)const{
        if(overflow)return {{},Support::unsupported,"Visible session sum overflow/out of range; no clamp or wrap"};
        if(!known)return {{},denied?Support::permission_required:Support::unsupported,"No accessible values attributed to this session/account; NOT measured zero"};
        return {total,Support::estimated,"Visible process sum by token session ID + exact account; "+std::to_string(known)+" known, "+std::to_string(missing)+" missing/invalid; "+std::to_string(unassigned)+" processes unassigned globally; non-atomic, incomplete coverage, not unique physical RAM"};
    }
};
struct Resources{Sum<double> cpu;Sum<std::uint64_t> memory,disk,network;};
}
void fill_windows_user_resources(std::vector<UserSession>& rows,std::span<const ProcessInfo> processes){
    if(rows.size()>10000 || processes.size()>100000)throw std::length_error("WTS resources inventory too large");
    std::size_t bytes=0,unassigned=0;std::map<std::uint32_t,std::size_t> sessions;
    for(std::size_t k=0;k<rows.size();++k){
        text(rows[k].id,bytes);text(rows[k].username,bytes);
        if(rows[k].username.empty() || !sessions.emplace(session_id(rows[k].id),k).second)throw std::invalid_argument("WTS resources duplicate session/missing account");
    }
    std::vector<Resources> sums(rows.size());std::set<std::uint32_t> pids;
    for(const auto& p:processes){
        text(p.username,bytes);
        // PID 0 is the Windows idle pseudo-process, not session-owned work.
        if(!pids.insert(p.pid).second)throw std::invalid_argument("Duplicate process PID; no double counted session sum");
        if(!p.pid){++unassigned;continue;}
        if(!p.windows_session_id.value || p.windows_session_id.support!=Support::available || !p.windows_creation_time_100ns.value || !*p.windows_creation_time_100ns.value || p.windows_creation_time_100ns.support!=Support::available){++unassigned;continue;}
        const auto it=sessions.find(*p.windows_session_id.value);
        if(it==sessions.end() || p.username!=rows[it->second].username){++unassigned;continue;}
        auto& s=sums[it->second];s.cpu.add(p.cpu_percent);s.memory.add(p.memory_bytes);s.disk.add(p.disk_bytes_per_second);s.network.add(p.network_bytes_per_second);
    }
    auto next=rows; // Publish all or nothing; leave existing output untouched on exceptions.
    for(std::size_t k=0;k<next.size();++k){
        auto& row=next[k];const auto& s=sums[k];row.resource_scope="windows-visible-processes-by-wts-session-and-account";
        row.cpu_percent=s.cpu.result(unassigned);row.memory_bytes=s.memory.result(unassigned);row.disk_bytes_per_second=s.disk.result(unassigned);row.network_bytes_per_second=s.network.result(unassigned);
    }
    rows.swap(next);
}
}
