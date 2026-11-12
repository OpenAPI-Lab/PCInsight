#include "../src/linux_network_app_history.hpp"
#include "pcinsight/linux_history_store.hpp"
#include <cassert>
#include <iostream>
#include <limits>
using namespace pcinsight;using namespace pcinsight::detail;using namespace std::chrono_literals;
struct Executable {
    std::string path="/private/app";std::uint64_t inode=19;bool denied=false;unsigned reads=0;bool change_during=false;
    NetworkHistoryExecutableApi api(){return {this,[](void* context,const auto&,const auto&)->Metric<LinuxExecutableInfo>{
        auto& e=*static_cast<Executable*>(context);++e.reads;
        if(e.denied)return {{},Support::permission_required,"fixture denied"};
        LinuxExecutableInfo exe;exe.path={e.path,Support::estimated,"verified fixture"};exe.stamp={1,e.inode+(e.change_during?e.reads:0),64,0100755,100,0,200,0};
        return {exe,Support::estimated,"fixture full identity verification"};}};}
};
SystemSnapshot frame(unsigned n,std::uint64_t bytes=0){
    SystemSnapshot s;s.captured_at=std::chrono::system_clock::time_point{}+1h+std::chrono::seconds(n);
    LinuxProcessNetworkSnapshot f;f.coverage=LinuxProcessNetworkCoverage::observed_tcp_udp_socket_namespace_buckets;
    f.observer_boot_token={1,2};f.selection_epoch=9;f.producer_initial_tgid=10;f.producer_start_boottime_ns=40;
    f.observer={10,1000,40,1,2,3,4,5,6,7};f.counter_sweep_started_ns=1000*n;f.counter_sweep_finished_ns=1000*n+100;
    LinuxNetworkObservedProcess a{200,1000,500,{20,1000,50,1,2,30,4,5,6,7},1,n};f.observed_processes={a};
    LinuxProcessNetworkSample r;r.process=a.process;r.kernel={a.initial_tgid,a.initial_real_uid,a.leader_start_boottime_ns,99};r.completed_updates=n;r.payload_bytes_since_start={std::array<std::uint64_t,4>{0,0,bytes,bytes},Support::available,"fixture"};f.processes={r};
    s.linux_process_network={f,Support::estimated,"fixture"};return s;
}
auto take(LinuxNetworkAppHistory& h,SystemSnapshot s){SampleOptions o;o.include_app_history=o.include_linux_process_network=true;h.prepare(true);h.finish(s,o);return s.linux_network_app_history;}
const LinuxAppHistoryEntry& row(const auto& m){if(!m.value)std::cerr<<m.note<<'\n';assert(m.value&&m.value->size()==1);return m.value->front();}
int main(){
    Executable e;LinuxNetworkAppHistory h(e.api());assert(take(h,frame(1,100)).value->empty());
    auto r=take(h,frame(2,200));assert(!row(r).network_bytes.value);
    r=take(h,frame(3,275));assert(row(r).network_bytes.value==150&&row(r).network_bytes.support==Support::estimated&&!row(r).observed_cpu_time.value&&!row(r).notifications.value);
    auto persisted=LinuxHistoryStore::merge({},*r.value);for(int i=0;i<50;++i)persisted=LinuxHistoryStore::merge(persisted,*r.value);assert(persisted[0].network_bytes.value==150);
    // Replay establishes no new delta, and cannot leave a continuity bridge.
    r=take(h,frame(3,275));assert(row(r).network_bytes.value==150);take(h,frame(4,300));r=take(h,frame(5,325));assert(row(r).network_bytes.value==200);r=take(h,frame(6,350));assert(row(r).network_bytes.value==250);
    e.inode++;r=take(h,frame(7,500));assert(row(r).network_bytes.value==250);r=take(h,frame(8,510));assert(row(r).network_bytes.value==270);
    e.denied=true;r=take(h,frame(9,900));assert(row(r).network_bytes.value==270);e.denied=false;take(h,frame(10,950));r=take(h,frame(11,960));assert(row(r).network_bytes.value==290);
    e.change_during=true;r=take(h,frame(12,1000));assert(row(r).network_bytes.value==290);e.change_during=false;take(h,frame(13,1100));r=take(h,frame(14,1110));assert(row(r).network_bytes.value==310);
    auto changed=frame(15,2000);changed.linux_process_network.value->selection_epoch++;
    r=take(h,changed);assert(row(r).network_bytes.value==310);changed=frame(16,2100);changed.linux_process_network.value->selection_epoch++;take(h,changed);
    changed=frame(17,2110);changed.linux_process_network.value->selection_epoch++;r=take(h,changed);assert(row(r).network_bytes.value==330);
    // Transactional rejection preserves already confirmed rows and drops baseline.
    for(int mode=0;mode<8;++mode){
        Executable x;LinuxNetworkAppHistory ledger(x.api());take(ledger,frame(1,0));take(ledger,frame(2,10));take(ledger,frame(3,20));auto bad=frame(4,30);auto& f=*bad.linux_process_network.value;
        if(mode==0)f.processes.push_back(f.processes[0]);
        if(mode==1)f.processes[0].payload_bytes_since_start.support=Support::estimated;
        if(mode==2)f.processes[0].payload_bytes_since_start.value->at(2)=1;
        if(mode==3)f.processes[0].kernel.initial_tgid++;
        if(mode==4)f.observed_processes[0].observed_buckets=0;
        if(mode==5)f.processes[0].payload_bytes_since_start.value=std::array<std::uint64_t,4>{0,0,UINT64_MAX,UINT64_MAX};
        if(mode==6)bad.captured_at=std::chrono::system_clock::time_point{}+1s;
        if(mode==7){f.processes.clear();f.observed_processes[0].observed_buckets=0;}
        assert(!take(ledger,bad).value);r=take(ledger,frame(5,50));assert(row(r).network_bytes.value==20);
    }
    // Idle actors: measured zero only after a verified interval, no fake cookie.
    {Executable x;LinuxNetworkAppHistory ledger(x.api());for(unsigned n=1;n<=3;++n){auto s=frame(n);s.linux_process_network.value->processes.clear();s.linux_process_network.value->observed_processes[0].observed_buckets=0;r=take(ledger,s);}assert(row(r).network_bytes.value==0);
        r=take(ledger,frame(4,100));assert(row(r).network_bytes.value==0);r=take(ledger,frame(5,110));assert(row(r).network_bytes.value==20);}
    // Independent processes with the same verified executable sum, never UID rows.
    {Executable x;LinuxNetworkAppHistory ledger(x.api());for(unsigned n=1;n<=3;++n){auto s=frame(n,n*10);auto& f=*s.linux_process_network.value;auto a=f.observed_processes.front();a.process.pid++;a.process.process_inode++;a.initial_tgid++;f.observed_processes.push_back(a);auto b=f.processes.front();b.process=a.process;b.kernel.initial_tgid=a.initial_tgid;f.processes.push_back(b);r=take(ledger,s);}assert(row(r).network_bytes.value==40);}
    // New cookie starts its own baseline: its initial100bytes are NOT history.
    {Executable x;LinuxNetworkAppHistory ledger(x.api());take(ledger,frame(1,0));take(ledger,frame(2,10));
        auto s=frame(3,20);auto extra=s.linux_process_network.value->processes.front();extra.kernel.socket_network_namespace_cookie=100;extra.payload_bytes_since_start.value=std::array<std::uint64_t,4>{0,0,100,100};s.linux_process_network.value->processes.push_back(extra);s.linux_process_network.value->observed_processes[0].observed_buckets=2;r=take(ledger,s);assert(row(r).network_bytes.value==20);
        s=frame(4,30);extra.completed_updates=4;extra.payload_bytes_since_start.value->at(2)=101;extra.payload_bytes_since_start.value->at(3)=101;s.linux_process_network.value->processes.push_back(extra);s.linux_process_network.value->observed_processes[0].observed_buckets=2;r=take(ledger,s);assert(row(r).network_bytes.value==42);}
    // Full target and observer identities, not PID/path alone, control continuity.
    for(int mode=0;mode<7;++mode){Executable x;LinuxNetworkAppHistory ledger(x.api());take(ledger,frame(1,0));take(ledger,frame(2,10));take(ledger,frame(3,20));
        auto changed_frame=[&](unsigned n){auto s=frame(n,n*10);auto& f=*s.linux_process_network.value;
            if(mode==0)f.observer_boot_token[0]++;
            if(mode==1)f.observer.user_namespace_inode++;
            if(mode==2)f.producer_start_boottime_ns++;
            if(mode==3){f.observed_processes[0].process.start_ticks++;f.processes[0].process.start_ticks++;}
            if(mode==4){f.observed_processes[0].process.process_inode++;f.processes[0].process.process_inode++;}
            if(mode==5){f.observed_processes[0].initial_real_uid++;f.processes[0].kernel.initial_real_uid++;}
            if(mode==6){f.observed_processes[0].leader_start_boottime_ns++;f.processes[0].kernel.leader_start_boottime_ns++;}
            return s;};
        r=take(ledger,changed_frame(4));assert(row(r).network_bytes.value==20);take(ledger,changed_frame(5));r=take(ledger,changed_frame(6));assert(row(r).network_bytes.value==40);}
    // A total can reach uint64_max, but the next byte must reject transactionally.
    {Executable x;LinuxNetworkAppHistory ledger(x.api());take(ledger,frame(1));take(ledger,frame(2));auto s=frame(3);s.linux_process_network.value->processes[0].payload_bytes_since_start.value=std::array<std::uint64_t,4>{UINT64_MAX,0,0,0};r=take(ledger,s);assert(row(r).network_bytes.value==UINT64_MAX);
        s=frame(4);s.linux_process_network.value->processes[0].payload_bytes_since_start.value=std::array<std::uint64_t,4>{UINT64_MAX,1,0,0};assert(!take(ledger,s).value);s=frame(5);s.linux_process_network.value->coverage=LinuxProcessNetworkCoverage::selected_tcp_udp_socket_operations;assert(!take(ledger,s).value);}
    // Opt-out clears history; a capture gap merely drops the network baseline.
    {SampleOptions o;o.include_app_history=true;SystemSnapshot s;s.captured_at=std::chrono::system_clock::now();h.prepare(true);h.finish(s,o);assert(row(s.linux_network_app_history).network_bytes.value==330);
        o.include_app_history=false;h.prepare(false);h.finish(s,o);assert(!s.linux_network_app_history.value);assert(take(h,frame(1)).value->empty());}
    std::cout<<"network executable history fixtures passed\n";
}
