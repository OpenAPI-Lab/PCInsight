#include "../src/linux_network_inventory.hpp"
#include <cassert>
#include <stdexcept>
#include <algorithm>
#include <string_view>
#include <cerrno>
#ifdef __linux__
#include <filesystem>
#include <signal.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <unistd.h>
#include <iostream>
#endif
using namespace pcinsight;using namespace pcinsight::detail;
namespace {
struct Fake {
    int contexts=0,inspects=0,clocks=0,enumerations=0,drift=-1;
    std::uint64_t time=100,step=10;int back=-1;
    Support context_support=Support::available,enum_support=Support::available,clock_support=Support::available,row_support=Support::available;
    bool throw_context=false,throw_enum=false,throw_read=false,missing_verified=false,foreign_procfs=false,error_value=false;
    LinuxNetworkInventoryState row_state=LinuxNetworkInventoryState::verified;
    std::uint32_t error_pid=0;
    NetworkSelectionContext context{{1,2},{7,42,100,1,2,3,4,5,6,7}};
    std::vector<std::uint32_t> pids{9,8};
    static Fake& f(void* p){return *static_cast<Fake*>(p);}
    NetworkInventoryApi api(){return {this,
        [](void* p)->Metric<NetworkSelectionContext>{auto& s=f(p);++s.contexts;if(s.throw_context)throw std::runtime_error("context");auto c=s.context;if(s.contexts==s.drift)++c.observer.user_namespace_inode;return {c,s.context_support,{}};},
        [](void* p)->Metric<std::uint64_t>{auto& s=f(p);++s.clocks;auto now=s.time;s.time+=s.step;if(s.clocks==s.back)now=1;return {now,s.clock_support,{}};},
        [](void* p,const NetworkSelectionContext&)->Metric<std::vector<std::uint32_t>>{auto& s=f(p);++s.enumerations;if(s.throw_enum)throw std::runtime_error("enum");return {s.pids,s.enum_support,"enum"};},
        [](void* p,std::uint32_t pid)->NetworkInventoryObservation{auto& s=f(p);++s.inspects;if(s.throw_read)throw std::runtime_error("inspect");
            Metric<LinuxNetworkSelfIdentity> identity{LinuxNetworkSelfIdentity{pid,0,200+pid,1,2,100+pid,4,9,6,10},s.row_support,"identity"};
            if(s.foreign_procfs)++identity.value->procfs_inode;
            if(s.missing_verified)identity.value.reset();
            const auto state=(!s.error_pid||s.error_pid==pid)?s.row_state:LinuxNetworkInventoryState::verified;
            if(state!=LinuxNetworkInventoryState::verified){if(!s.error_value)identity.value.reset();identity.support=state==LinuxNetworkInventoryState::permission_denied?Support::permission_required:Support::unsupported;identity.note="explicit row failure";}
            return {state,identity};}};}
};
void fixtures(){
    {Fake f;assert(collect_linux_network_inventory(false,f.api()).support==Support::permission_required&&f.contexts==0);assert(!collect_linux_network_inventory(true,{}).value);}
    {Fake f;auto r=collect_linux_network_inventory(true,f.api());assert(r.value&&r.support==Support::available&&r.value->enumeration_completed&&r.value->processes.size()==2);
        assert(r.value->processes[0].pid==8&&r.value->processes[1].pid==9&&r.value->observer.pid==7&&r.value->processes[0].observer_real_uid.value==0);
        auto candidates=linux_network_inventory_candidates(r);assert(candidates.value&&candidates.value->size()==2&&candidates.value->at(0).process_start_ticks==208);
        for(unsigned fault=0;fault<15;++fault){auto bad=r;
            if(fault==0)bad.support=Support::estimated;if(fault==1)bad.value->enumeration_completed=false;if(fault==2)bad.value->boot={};if(fault==3)bad.value->observer.procfs_inode=0;
            if(fault==4)bad.value->started_ns=0;if(fault==5)bad.value->finished_ns=1;if(fault==6)bad.value->finished_ns+=linux_network_inventory_max_span_ns;
            if(fault==7)bad.value->processes[0].candidate.support=Support::estimated;if(fault==8)bad.value->processes[0].candidate.value->pid=9;
            if(fault==9)bad.value->processes[0].candidate.value->process_start_ticks=0;if(fault==10)bad.value->processes[0].observer_real_uid.value.reset();
            if(fault==11)bad.value->processes[0].state=LinuxNetworkInventoryState::unavailable;if(fault==12)bad.value->processes[1].pid=8;
            if(fault==13)std::reverse(bad.value->processes.begin(),bad.value->processes.end());if(fault==14)bad.value.reset();
            assert(!linux_network_inventory_candidates(bad).value);}
    }
    {Fake f;f.pids.clear();auto r=collect_linux_network_inventory(true,f.api());assert(r.value&&r.value->processes.empty()&&linux_network_inventory_candidates(r).value->empty());}
    {Fake f;f.pids.clear();const auto cap=LinuxProcessNetworkSampler::max_processes;for(unsigned pid=1;pid<=cap+1;++pid)f.pids.push_back(pid);auto r=collect_linux_network_inventory(true,f.api());assert(r.value&&r.value->processes.size()==cap+1&&f.inspects==cap+1);
        assert(!linux_network_inventory_candidates(r).value);f.pids.resize(cap);f.time=100;auto valid=collect_linux_network_inventory(true,f.api());assert(linux_network_inventory_candidates(valid).value->size()==cap);}
    for(auto state:{LinuxNetworkInventoryState::permission_denied,LinuxNetworkInventoryState::disappeared,LinuxNetworkInventoryState::unavailable}){Fake f;f.row_state=state;
        auto r=collect_linux_network_inventory(true,f.api());assert(r.value&&r.value->processes.size()==2&&r.value->processes[0].state==state&&!r.value->processes[0].candidate.value&&!r.value->processes[0].observer_real_uid.value);
        const auto c=linux_network_inventory_candidates(r);assert(!c.value&&c.support==(state==LinuxNetworkInventoryState::permission_denied?Support::permission_required:Support::unsupported));}
    {Fake f;f.row_state=LinuxNetworkInventoryState::permission_denied;f.error_pid=9;auto mixed=collect_linux_network_inventory(true,f.api());
        assert(mixed.value&&mixed.value->processes.size()==2&&mixed.value->processes[0].candidate.value&&!mixed.value->processes[1].candidate.value);
        assert(linux_network_inventory_candidates(mixed).support==Support::permission_required);}
    for(unsigned fault=0;fault<21;++fault){Fake f;
        if(fault==0)f.context_support=Support::estimated;if(fault==1)f.context.boot={};if(fault==2)f.drift=2;if(fault==3)f.clock_support=Support::estimated;
        if(fault==4)f.time=0;if(fault==5)f.step=linux_network_inventory_max_span_ns+1;if(fault==6)f.back=3;
        if(fault==7)f.enum_support=Support::permission_required;if(fault==8)f.pids={8,8};if(fault==9)f.pids={0};if(fault==10)f.pids={2147483648U};
        if(fault==11)f.pids.resize(linux_network_inventory_max_processes+1,8);if(fault==12)f.foreign_procfs=true;if(fault==13)f.missing_verified=true;
        if(fault==14)f.row_support=Support::estimated;if(fault==15)f.throw_context=true;if(fault==16)f.throw_enum=true;if(fault==17)f.throw_read=true;
        if(fault==18)f.back=4;if(fault==19)f.context_support=Support::permission_required;if(fault==20)f.enum_support=Support::estimated;
        assert(!collect_linux_network_inventory(true,f.api()).value);if(fault==11)assert(f.inspects==0);}
    {Fake f;f.row_state=LinuxNetworkInventoryState::disappeared;f.error_value=true;assert(!collect_linux_network_inventory(true,f.api()).value);}
    {Fake f;f.row_state=static_cast<LinuxNetworkInventoryState>(99);assert(!collect_linux_network_inventory(true,f.api()).value);}
    assert(read_linux_network_process_inventory(false).support==Support::permission_required);
}
#ifdef __linux__
std::size_t descriptors(){std::size_t n=0;for(const auto& f:std::filesystem::directory_iterator("/proc/self/fd")){(void)f;++n;}return n;}
int native(){
    ::alarm(20);const auto baseline=descriptors();
    struct Child {pid_t pid=-1;int r=-1,w=-1;~Child(){if(pid>0){::kill(pid,SIGKILL);int s;while(::waitpid(pid,&s,0)<0&&errno==EINTR){}}if(r>=0)::close(r);if(w>=0)::close(w);}} children[2];
    for(auto& c:children){int p[2];assert(::pipe2(p,O_CLOEXEC)==0);c.r=p[0];c.w=p[1];}
    for(unsigned i=0;i<2;++i){auto pid=::fork();assert(pid>=0);if(!pid){::alarm(15);for(unsigned n=0;n<2;++n){::close(children[n].w);if(n!=i)::close(children[n].r);}
            char x=0;const bool ok=::read(children[i].r,&x,1)==1&&x=='X';::_exit(ok?0:1);}children[i].pid=pid;}
    for(auto& c:children){::close(c.r);c.r=-1;}const auto before=descriptors();
    auto report=read_linux_network_process_inventory(true);assert(report.value&&report.value->enumeration_completed&&report.value->observer.pid==static_cast<std::uint32_t>(::getpid())&&descriptors()==before);
    std::array<LinuxNetworkProcessCandidate,2> selected;
    for(unsigned i=0;i<2;++i){auto row=std::find_if(report.value->processes.begin(),report.value->processes.end(),[&](const auto& r){return r.pid==static_cast<std::uint32_t>(children[i].pid);});
        assert(row!=report.value->processes.end()&&row->state==LinuxNetworkInventoryState::verified&&row->candidate.value);selected[i]=*row->candidate.value;}
    LinuxProcessNetworkSelectionOwner owner;assert(owner.refresh(true,selected).value&&owner.selections().value->size()==2);owner.clear();assert(descriptors()==before);
    auto all=linux_network_inventory_candidates(report);if(all.value){assert(all.value->size()==report.value->processes.size()&&all.value->size()<=LinuxProcessNetworkSampler::max_processes);}
    else assert(!all.note.empty()); // Capacity/unknown identities cannot silently produce a subset.
    for(auto& c:children){char x='X';assert(::write(c.w,&x,1)==1);int s=0;assert(::waitpid(c.pid,&s,0)==c.pid&&WIFEXITED(s)&&WEXITSTATUS(s)==0);c.pid=-1;::close(c.w);c.w=-1;}
    assert(descriptors()==baseline);::alarm(0);
    std::cout<<"PASS readonly visible procfs inventory: complete directory walk, per-row quality, two private children verified and fed to pidfd owner; strict all-candidates bridge returned ALL or explicit absence, no silent subset; no FD leak, BPF, signal or traffic capture. Restricted/dynamic view, NOT all-host coverage\n";return 0;
}
#endif
}
int main(int argc,char** argv){if(argc==2&&std::string_view(argv[1])=="--native-readonly"){
#ifdef __linux__
    return native();
#else
    return 77;
#endif
}assert(argc==1);fixtures();}
