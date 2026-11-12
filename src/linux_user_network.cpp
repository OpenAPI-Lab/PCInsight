#include "pcinsight/linux_user_network.hpp"
#include "linux_network_self.hpp"
#include "linux_network_accounting.hpp"
#include <algorithm>
#include <map>
#include <set>
namespace pcinsight {
namespace {
template<class T> Metric<T> absent(std::string note,Support support=Support::unsupported){return {{},support==Support::permission_required?support:Support::unsupported,std::move(note)};}
bool identity(const LinuxNetworkProcIdentity& p){return detail::valid_network_self({p.pid,p.real_uid,p.start_ticks,p.procfs_device,p.procfs_inode,p.process_inode,p.pid_namespace_device,p.pid_namespace_inode,p.user_namespace_device,p.user_namespace_inode});}
bool empty(const Metric<std::uint64_t>& m){return !m.value&&(m.support==Support::unsupported||m.support==Support::permission_required);}
bool bracket(std::uint64_t start,std::uint64_t end){return start&&end>=start&&end-start<=LinuxProcessNetworkSampler::max_counter_sweep_ns;}
struct Sum {
    std::uint64_t value{},known{},missing{},denied{};bool overflow{};
    void add(const Metric<std::uint64_t>& m){
        if(!m.value){++missing;if(m.support==Support::permission_required)++denied;return;}
        ++known;
        if(*m.value>UINT64_MAX-value)overflow=true;
        else if(!overflow)value+=*m.value;
    }
    Metric<std::uint64_t> result()const{
        if(overflow)return absent<std::uint64_t>("Observer-UID partial bucket-rate sum overflow; no wrap or saturation");
        if(!known)return absent<std::uint64_t>("No measured bucket rates; idle metadata/unknown is not measured zero",denied?Support::permission_required:Support::unsupported);
        return {value,Support::estimated,"Sum of known rounded-down selected TCP/UDP operation payload rates; partial observer-UID scope, not wire/global/session totals; inspect coverage"};
    }
};
struct User {LinuxUserNetworkSample row;std::set<std::uint32_t> initial;std::array<Sum,4> lanes;Sum send,receive,total;};
}
Metric<LinuxUserNetworkReport> summarize_linux_user_network(const Metric<LinuxProcessNetworkSnapshot>& input){
    if(!metric_is_known(input))return absent<LinuxUserNetworkReport>(input.note.empty()?"F03 network frame absent":input.note,input.support);
    const auto& f=*input.value;
    auto reject=[](){return absent<LinuxUserNetworkReport>("Invalid observed F03 frame/identity/bucket/rate/window; no partial user report published");};
    if(f.coverage!=LinuxProcessNetworkCoverage::observed_tcp_udp_socket_namespace_buckets||!identity(f.observer)||
       f.observer_boot_token==std::array<std::uint64_t,2>{}||!f.selection_epoch||!f.producer_initial_tgid||f.producer_initial_tgid>2147483647U||!f.producer_start_boottime_ns||
       !bracket(f.counter_sweep_started_ns,f.counter_sweep_finished_ns)||f.observed_processes.empty()||f.observed_processes.size()>LinuxProcessNetworkSampler::max_processes||f.processes.size()>LinuxProcessNetworkSampler::max_observed_buckets)return reject();
    if(f.previous_counter_sweep){const auto& p=*f.previous_counter_sweep;if(!bracket(p[0],p[1])||p[1]>f.counter_sweep_started_ns||p[1]>=f.counter_sweep_finished_ns)return reject();}
    std::map<std::uint32_t,const LinuxNetworkObservedProcess*> actors;
    std::set<std::uint32_t> kernels;
    std::map<std::uint32_t,User> users;
    for(const auto& a:f.observed_processes){
        if(!identity(a.process)||!a.initial_tgid||a.initial_tgid>2147483647U||!a.leader_start_boottime_ns||
           !actors.emplace(a.process.pid,&a).second||!kernels.insert(a.initial_tgid).second||
           a.observed_buckets>LinuxProcessNetworkSampler::max_observed_namespaces_per_process||a.completed_updates<a.observed_buckets||(!a.observed_buckets&&a.completed_updates))return reject();
        auto& u=users[a.process.real_uid];u.row.real_uid=a.process.real_uid;++u.row.selected_processes;
        if(!a.observed_buckets)++u.row.idle_processes;
        u.initial.insert(a.initial_real_uid);
    }
    std::set<std::pair<std::uint32_t,std::uint64_t>> buckets;std::map<std::uint32_t,std::uint64_t> counts;
    for(const auto& row:f.processes){
        const auto at=actors.find(row.process.pid);if(at==actors.end())return reject();const auto& a=*at->second;
        if(row.process!=a.process||row.kernel.initial_tgid!=a.initial_tgid||row.kernel.initial_real_uid!=a.initial_real_uid||row.kernel.leader_start_boottime_ns!=a.leader_start_boottime_ns||
           !row.kernel.socket_network_namespace_cookie||!buckets.emplace(row.process.pid,row.kernel.socket_network_namespace_cookie).second||row.completed_updates!=a.completed_updates||
           !row.payload_bytes_since_start.value||row.payload_bytes_since_start.support!=Support::available)return reject();
        ++counts[a.process.pid];auto& u=users[a.process.real_uid];++u.row.observed_buckets;
        if(row.nominal_interval_ns.value){
            if(row.nominal_interval_ns.support!=Support::available||!f.previous_counter_sweep||!*row.nominal_interval_ns.value||
               *row.nominal_interval_ns.value!=f.counter_sweep_finished_ns-(*f.previous_counter_sweep)[1]||*row.nominal_interval_ns.value>detail::LinuxNetworkAccounting::max_interval_ns)return reject();
        }else if(!empty(row.nominal_interval_ns))return reject();
        for(unsigned lane=0;lane<4;++lane){const auto& rate=row.payload_bytes_per_second[lane];
            if(rate.value){if(rate.support!=Support::estimated||!row.nominal_interval_ns.value)return reject();}
            else if(!empty(rate))return reject();
            u.lanes[lane].add(rate);(lane%2?u.receive:u.send).add(rate);u.total.add(rate);
        }
    }
    for(const auto& [pid,a]:actors)if(counts[pid]!=a->observed_buckets)return reject();
    LinuxUserNetworkReport out;out.observer=f.observer;out.observer_boot_token=f.observer_boot_token;out.selection_epoch=f.selection_epoch;
    out.producer_initial_tgid=f.producer_initial_tgid;out.producer_start_boottime_ns=f.producer_start_boottime_ns;
    out.counter_sweep_started_ns=f.counter_sweep_started_ns;out.counter_sweep_finished_ns=f.counter_sweep_finished_ns;out.previous_counter_sweep=f.previous_counter_sweep;
    for(auto& [uid,u]:users){
        u.row.initial_real_uids.assign(u.initial.begin(),u.initial.end());
        for(unsigned lane=0;lane<4;++lane){const auto& s=u.lanes[lane];u.row.known[lane]=s.known;u.row.missing[lane]=s.missing;u.row.permission_denied[lane]=s.denied;u.row.overflow[lane]=s.overflow;u.row.partial_payload_bytes_per_second[lane]=s.result();}
        u.row.partial_send_bytes_per_second=u.send.result();u.row.partial_receive_bytes_per_second=u.receive.result();u.row.partial_total_bytes_per_second=u.total.result();
        u.row.send_overflow=u.send.overflow;u.row.receive_overflow=u.receive.overflow;u.row.total_overflow=u.total.overflow;
        out.users.push_back(std::move(u.row));
    }
    return {std::move(out),Support::estimated,"Unique observer-domain real UID groups of selected instrumented TCP/UDP operations; independent of login session inventory. Unknown/idle actors not invented as zero; no automatic session/account join. See paired F03/admission metadata for selection coverage"};
}
}
