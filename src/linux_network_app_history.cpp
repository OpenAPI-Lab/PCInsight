#include "linux_network_app_history.hpp"
#include "linux_network_selection.hpp"
#include <algorithm>
#include <limits>
#include <set>
#include <stdexcept>
namespace pcinsight::detail {
namespace {
bool usable(const Metric<LinuxExecutableInfo>& m){
    if(!m.value||(m.support!=Support::available&&m.support!=Support::estimated))return false;
    const auto& p=m.value->path;
    return p.value&&(p.support==Support::available||p.support==Support::estimated)&&!p.value->empty()&&p.value->size()<=65536&&p.value->front()=='/'&&p.value->find('\0')==p.value->npos;
}
bool same_executable(const LinuxExecutableInfo& a,const LinuxExecutableInfo& b){return a.path.value==b.path.value&&a.stamp==b.stamp;}
bool same_actor(const LinuxNetworkObservedProcess& a,const LinuxNetworkObservedProcess& b){
    return a.process==b.process&&a.initial_tgid==b.initial_tgid&&a.initial_real_uid==b.initial_real_uid&&a.leader_start_boottime_ns==b.leader_start_boottime_ns;
}
bool same_capture(const LinuxProcessNetworkSnapshot& a,const LinuxProcessNetworkSnapshot& b){
    return a.selection_epoch==b.selection_epoch&&a.observer_boot_token==b.observer_boot_token&&a.observer==b.observer&&a.producer_initial_tgid==b.producer_initial_tgid&&a.producer_start_boottime_ns==b.producer_start_boottime_ns;
}
LinuxNetworkProcIdentity public_identity(const LinuxNetworkSelfIdentity& s){
    return {s.local_pid,s.local_real_uid,s.start_ticks,s.procfs_device,s.procfs_inode,s.process_inode,s.pid_namespace_device,s.pid_namespace_inode,s.user_namespace_device,s.user_namespace_inode};
}
template<class T> bool exact(const Metric<T>& m){return m.value&&m.support==Support::available;}
}
NetworkHistoryExecutableApi native_network_history_executable_api(){
    return {nullptr,[](void*,const LinuxProcessNetworkSnapshot& frame,const LinuxNetworkObservedProcess& actor)->Metric<LinuxExecutableInfo>{
#ifdef __linux__
        const auto api=native_network_selection_source_api();
        const auto before=api.observe(api.context);
        auto context_matches=[&](const auto& c){return exact(c)&&c.value->boot==frame.observer_boot_token&&public_identity(c.value->observer)==frame.observer;};
        if(!context_matches(before))return {{},Support::unsupported,"Network history observer/boot changed before executable read"};
        const auto opened=api.open(api.context,actor.process.pid);
        if(opened.fd<0)return {{},Support::permission_required,"Network history cannot pin selected process"};
        struct Close {NetworkSelectionSourceApi api;int fd;~Close(){api.close(api.context,fd);}} close{api,opened.fd};
        auto inspect=[&]{return api.inspect(api.context,opened.fd,actor.process.pid,actor.process.start_ticks);};
        auto matches=[&](const auto& id){return exact(id)&&public_identity(*id.value)==actor.process;};
        if(!matches(inspect()))return {{},Support::unsupported,"Network history selected identity changed before executable read"};
        const auto path=std::filesystem::path("/proc")/std::to_string(actor.process.pid);
        auto stat=read_linux_process_identity(path);
        if(!exact(stat)||stat.value->pid!=actor.process.pid||stat.value->start_ticks!=actor.process.start_ticks)return {{},Support::unsupported,"Network history process start unavailable"};
        auto exe=read_linux_process_executable(path,*stat.value);
        if(!matches(inspect())||!context_matches(api.observe(api.context)))return {{},Support::unsupported,"Network history identity changed during executable read"};
        return exe;
#else
        (void)frame;(void)actor;return {{},Support::unsupported,"Linux-only network executable history"};
#endif
    }};
}
void LinuxNetworkAppHistory::prepare(bool enabled){
    before_.clear();
    if(!enabled){reset();return;}
    if(frame_&&api_.read)for(const auto& actor:frame_->observed_processes)
        before_.emplace(actor.process.pid,api_.read(api_.context,*frame_,actor));
}
void LinuxNetworkAppHistory::finish(SystemSnapshot& snapshot,const SampleOptions& options){
    auto& report=snapshot.linux_network_app_history;
    if(!options.include_app_history){reset();report={{},Support::unsupported,"Linux network app history disabled"};return;}
    auto output=[&](std::string note){std::vector<LinuxAppHistoryEntry> out;for(const auto& [id,r]:rows_)out.push_back(r);report={std::move(out),Support::estimated,std::move(note)};};
    try{
        if(!options.include_linux_process_network||!snapshot.linux_process_network.value){
            break_continuity();
            if(rows_.empty())report={{},Support::unsupported,"No observed network app history; explicit Linux capture and history options required"};
            else output("No active network frame; confirmed totals retained, gap not reconstructed");
            return;
        }
        const auto& f=*snapshot.linux_process_network.value;
        const auto now=snapshot.captured_at;
        if(f.coverage!=LinuxProcessNetworkCoverage::observed_tcp_udp_socket_namespace_buckets||!f.selection_epoch||
           f.observer_boot_token==std::array<std::uint64_t,2>{}||!f.producer_initial_tgid||!f.producer_start_boottime_ns||
           !f.counter_sweep_started_ns||f.counter_sweep_finished_ns<f.counter_sweep_started_ns||
           f.counter_sweep_finished_ns-f.counter_sweep_started_ns>LinuxProcessNetworkSampler::max_counter_sweep_ns||
           f.observed_processes.size()>LinuxProcessNetworkSampler::max_processes||f.processes.size()>LinuxProcessNetworkSampler::max_observed_buckets||
           now.time_since_epoch().count()<=0||(initialized_&&now<last_))throw std::invalid_argument("Network history frame/domain/time bounds invalid (observed mode required)");
        const bool continuous=frame_&&same_capture(*frame_,f)&&f.counter_sweep_started_ns>frame_->counter_sweep_finished_ns;
        auto next=rows_;std::map<std::uint32_t,Point> points;std::set<std::uint32_t> seen;std::set<std::uint32_t> kernel_seen;
        for(const auto& actor:f.observed_processes){
            if(!actor.process.pid||!actor.initial_tgid||!actor.leader_start_boottime_ns||!actor.process.process_inode||
               !seen.insert(actor.process.pid).second||!kernel_seen.insert(actor.initial_tgid).second||actor.observed_buckets>LinuxProcessNetworkSampler::max_observed_namespaces_per_process)
                throw std::invalid_argument("Network history invalid/duplicate actor");
            const auto post=api_.read?api_.read(api_.context,f,actor):Metric<LinuxExecutableInfo>{};
            const auto pre=before_.find(actor.process.pid);
            if(!continuous||pre==before_.end()||!usable(pre->second)||!usable(post)||!same_executable(*pre->second.value,*post.value))continue;
            const auto old_actor=std::find_if(frame_->observed_processes.begin(),frame_->observed_processes.end(),[&](const auto& a){return same_actor(a,actor);});
            if(old_actor==frame_->observed_processes.end())continue;
            points.emplace(actor.process.pid,Point{actor,*post.value,{}});
        }
        // Validate all rows, including un-attributable actors. Never silently
        // accept malformed/duplicate buckets just because exe access was denied.
        std::set<std::pair<std::uint32_t,std::uint64_t>> buckets;
        std::map<std::uint32_t,std::size_t> counts;
        for(const auto& row:f.processes){
            const auto actor=std::find_if(f.observed_processes.begin(),f.observed_processes.end(),[&](const auto& a){return a.process==row.process&&a.initial_tgid==row.kernel.initial_tgid&&a.initial_real_uid==row.kernel.initial_real_uid&&a.leader_start_boottime_ns==row.kernel.leader_start_boottime_ns;});
            if(actor==f.observed_processes.end()||!row.kernel.socket_network_namespace_cookie||!exact(row.payload_bytes_since_start)||
               row.completed_updates!=actor->completed_updates||!buckets.emplace(row.process.pid,row.kernel.socket_network_namespace_cookie).second)
                throw std::invalid_argument("Network history invalid counter bucket/association");
            ++counts[row.process.pid];
            if(auto p=points.find(row.process.pid);p!=points.end())p->second.buckets.emplace(row.kernel.socket_network_namespace_cookie,*row.payload_bytes_since_start.value);
        }
        for(const auto& actor:f.observed_processes)if(counts[actor.process.pid]!=actor.observed_buckets)throw std::invalid_argument("Network history incomplete bucket set");
        for(const auto& [pid,p]:points){
            const auto& path=*p.executable.path.value;
            auto r=next.find(path);
            if(r==next.end()){
                if(next.size()>=4096)throw std::invalid_argument("Network history exceeds4096 paths; reset history");
                LinuxAppHistoryEntry row;row.id=row.name=path;row.period_start=initialized_?started_:now;row.period_end=now;
                row.observed_cpu_time.note="CPU is collected separately in linux_app_history; do not join independent periods";
                row.network_bytes.note="Baseline only; no verified consecutive executable interval yet";
                row.notifications.note="Notifications not collected by network history";
                r=next.emplace(path,std::move(row)).first;
            }
            const auto old=previous_.find(pid);
            if(old==previous_.end()||!same_actor(old->second.actor,p.actor)||!same_executable(old->second.executable,p.executable))continue;
            if(p.actor.completed_updates<old->second.actor.completed_updates)throw std::invalid_argument("Network history actor sequence decreased");
            std::uint64_t delta=0;
            for(const auto& [cookie,counters]:p.buckets){
                const auto previous=old->second.buckets.find(cookie);
                if(previous==old->second.buckets.end())continue; // No retroactive first-bucket bytes.
                for(unsigned lane=0;lane<4;++lane){
                    if(counters[lane]<previous->second[lane])throw std::invalid_argument("Network history payload counter decreased");
                    const auto amount=counters[lane]-previous->second[lane];
                    if(amount>std::numeric_limits<std::uint64_t>::max()-delta)throw std::overflow_error("Network history interval byte sum overflow");
                    delta+=amount;
                }
            }
            for(const auto& [cookie,counters]:old->second.buckets)if(!p.buckets.contains(cookie))throw std::invalid_argument("Network history bucket disappeared without epoch change");
            const auto total=r->second.network_bytes.value.value_or(0);
            if(delta>std::numeric_limits<std::uint64_t>::max()-total)throw std::overflow_error("Network history accumulated bytes overflow");
            r->second.network_bytes={total+delta,Support::estimated,"Sum of verified consecutive TCP/UDP socket-operation payload deltas (send+receive); observed subset, not wire bytes or all app traffic. New buckets/gaps excluded"};
            r->second.period_end=now;
        }
        std::size_t retained=0;for(const auto& [id,r]:next){const auto cost=id.size()+r.id.size()+r.name.size();if(cost>4*1024*1024-retained)throw std::invalid_argument("Network history retained paths exceed4MiB");retained+=cost;}
        rows_=std::move(next);previous_=std::move(points);frame_=f;before_.clear();
        if(!initialized_)started_=now;
        last_=now;initialized_=true;
        output("Observed network history by exact executable path across selected users; independent CPU/notification periods, no automatic persistence. Endpoint checks cannot detect exec-and-return ABA between reads");
    }catch(const std::invalid_argument& e){break_continuity();report={{},Support::unsupported,e.what()};}
    catch(const std::overflow_error& e){break_continuity();report={{},Support::unsupported,e.what()};}
    catch(...){break_continuity();throw;}
}
}
