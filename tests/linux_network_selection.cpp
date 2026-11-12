#include "../src/linux_network_selection.hpp"
#include "../src/linux_process_details.hpp"
#include <cassert>
#include <algorithm>
#include <cerrno>
#include <stdexcept>
#include <type_traits>
#include <string_view>
#ifdef __linux__
#include <fcntl.h>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>
#include <filesystem>
#include <iostream>
#endif
using namespace pcinsight;using namespace pcinsight::detail;
namespace {
const std::array<LinuxNetworkProcessCandidate,2> ab{{{8,200},{9,201}}};
struct Fake {
    std::array<int,LinuxProcessNetworkSelectionOwner::max_processes+16> live{};int next=0,opens=0,inspects=0,contexts=0,calls=0;
    int deny_open=-1,deny_inspect=-1,drift_at=-1,context_drift=-1;
    bool throw_open=false,throw_inspect=false,throw_context=false;Support support=Support::available,process_support=Support::available;
    std::uint64_t caller=7;
    NetworkSelectionContext c{{1,2},{7,42,100,1,2,3,4,5,6,7}};
    static Fake& f(void* p){return *static_cast<Fake*>(p);}
    NetworkSelectionSourceApi api(){return {this,
        [](void* p) noexcept->std::uint64_t{return f(p).caller;},
        [](void* p)->Metric<NetworkSelectionContext>{auto& s=f(p);++s.calls;++s.contexts;if(s.throw_context){s.throw_context=false;throw std::runtime_error("context");}
            auto c=s.c;if(s.contexts==s.context_drift)++c.observer.user_namespace_inode;return {c,s.support,{}};},
        [](void* p,std::uint32_t pid)->NetworkSessionFd{auto& s=f(p);++s.calls;++s.opens;if(s.throw_open){s.throw_open=false;throw std::runtime_error("open");}
            if(static_cast<int>(pid)==s.deny_open)return {-1,EACCES};assert(static_cast<std::size_t>(s.next)<s.live.size());s.live[s.next]=static_cast<int>(pid);return {s.next++,0};},
        [](void* p,int fd,std::uint32_t pid,std::uint64_t start)->Metric<LinuxNetworkSelfIdentity>{auto& s=f(p);++s.calls;++s.inspects;assert(fd>=0&&fd<s.next&&s.live[fd]==static_cast<int>(pid));
            if(s.throw_inspect){s.throw_inspect=false;throw std::runtime_error("inspect");}if(static_cast<int>(pid)==s.deny_inspect)return {{},Support::permission_required,"denied"};
            LinuxNetworkSelfIdentity process{pid,43,start,1,2,100+pid,4,9,6,10};if(s.inspects==s.drift_at)++process.local_real_uid;
            return {process,s.process_support,{}};},
        [](void* p,int fd) noexcept {auto& s=f(p);assert(fd>=0&&fd<s.next&&s.live[fd]);s.live[fd]=0;}};}
    void clean(){for(auto fd:live)assert(fd==0);}
};
void fixture(){
    static_assert(!std::is_copy_constructible_v<LinuxProcessNetworkSelectionOwner>);
    static_assert(std::is_nothrow_move_constructible_v<LinuxProcessNetworkSelectionOwner>);
    {Fake f;LinuxNetworkSelectionOwner o(f.api());assert(!o.selections().value);assert(o.refresh(false,ab).support==Support::permission_required&&f.calls==0);
        auto bad=ab;bad[1].pid=8;assert(!o.refresh(true,bad).value);bad=ab;bad[0].process_start_ticks=0;assert(!o.refresh(true,bad).value);
        bad=ab;bad[0].pid=0;assert(!o.refresh(true,bad).value);bad=ab;bad[0].pid=2147483648U;assert(!o.refresh(true,bad).value);
        std::vector<LinuxNetworkProcessCandidate> many(LinuxProcessNetworkSelectionOwner::max_processes+1,ab[0]);assert(!o.refresh(true,many).value&&f.calls==0);f.clean();}
    {Fake f;LinuxNetworkSelectionOwner o(f.api());auto initial=o.refresh(true,ab);assert(initial.value&&initial.value->changed&&initial.value->added==2);
        auto rows=o.selections();assert(rows.value&&rows.value->size()==2&&rows.value->at(0).pidfd==0&&rows.value->at(0).socket_network_namespace_cookie==0);
        auto reverse=ab;std::swap(reverse[0],reverse[1]);auto same=o.refresh(true,reverse);assert(same.value&&!same.value->changed&&same.value->generation==initial.value->generation&&f.opens==2);
        assert(o.selections().value->at(0).pid==8);assert(o.refresh(false,{}).support==Support::permission_required&&o.selections().value->size()==2);
        std::vector<LinuxNetworkProcessCandidate> many(LinuxProcessNetworkSelectionOwner::max_processes+1,ab[0]);assert(!o.refresh(true,many).value&&o.selections().value->size()==2);
        auto removed=o.refresh(true,std::span(ab).last(1));assert(removed.value&&removed.value->changed&&removed.value->retired==1&&removed.value->retained==1&&!f.live[0]&&f.live[1]);
        auto added=o.refresh(true,ab);assert(added.value&&added.value->added==1&&added.value->retained==1&&f.opens==3);
        auto reused=ab;++reused[0].process_start_ticks;auto changed=o.refresh(true,reused);assert(changed.value&&changed.value->added==1&&changed.value->retired==1&&f.opens==4);
        auto empty=o.refresh(true,{});assert(empty.value&&empty.value->changed&&empty.value->retired==2&&o.selections().value->empty());f.clean();
        assert(!o.refresh(true,{}).value->changed);o.clear();assert(!o.selections().value);}
    {Fake f;LinuxNetworkSelectionOwner o(f.api());std::vector<LinuxNetworkProcessCandidate> full;
        for(std::uint32_t pid=10;pid<10+LinuxProcessNetworkSelectionOwner::max_processes;++pid)full.push_back({pid,100+pid});
        assert(o.refresh(true,full).value&&f.opens==LinuxProcessNetworkSelectionOwner::max_processes&&o.selections().value->size()==full.size());
        std::reverse(full.begin(),full.end());assert(!o.refresh(true,full).value->changed&&f.opens==LinuxProcessNetworkSelectionOwner::max_processes);o.clear();f.clean();}
    for(unsigned fault=0;fault<13;++fault){Fake f;{LinuxNetworkSelectionOwner o(f.api());assert(o.refresh(true,std::span(ab).first(1)).value);
        if(fault==0)f.deny_open=9;if(fault==1)f.deny_inspect=9;if(fault==2)f.deny_inspect=8;
        if(fault==3)f.drift_at=f.inspects+3; // Recheck FIRST actor after the new actor was acquired.
        if(fault==4)f.context_drift=f.contexts+2;if(fault==5)f.context_drift=f.contexts+3;
        if(fault==6)f.support=Support::estimated;if(fault==7)f.throw_open=true;if(fault==8)f.throw_inspect=true;if(fault==9)f.throw_context=true;
        if(fault==10)++f.c.boot[0];if(fault==11)f.process_support=Support::estimated;if(fault==12)++f.c.observer.user_namespace_inode;
        bool threw=false;try{assert(!o.refresh(true,ab).value);}catch(const std::runtime_error&){threw=true;}
        assert(threw==(fault==7||fault==8||fault==9));assert(!o.selections().value);f.clean();
    }f.clean();}
    {Fake f;LinuxNetworkSelectionOwner o(f.api());assert(o.refresh(true,ab).value);f.caller=10;const auto calls=f.calls;assert(!o.selections().value&&f.calls==calls);f.clean();}
    {Fake f;LinuxNetworkSelectionOwner o(f.api());assert(o.refresh(true,ab).value);f.caller=10;const auto calls=f.calls;assert(!o.refresh(true,ab).value&&f.calls==calls);f.clean();}
    {Fake f;LinuxNetworkSelectionOwner o(f.api());f.caller=0;assert(!o.refresh(true,{}).value);f.clean();}
    {LinuxNetworkSelectionOwner o(NetworkSelectionSourceApi{});assert(!o.refresh(true,{}).value);}
    {LinuxProcessNetworkSelectionOwner owner;assert(!owner.selections().value&&owner.refresh(false,ab).support==Support::permission_required);
        auto moved=std::move(owner);assert(!owner.selections().value);owner.clear();moved.clear();}
}
#ifdef __linux__
std::size_t descriptors(){std::size_t n=0;for(const auto& item:std::filesystem::directory_iterator("/proc/self/fd")){(void)item;++n;}return n;}
int native(){
    ::alarm(20);const auto baseline=descriptors();
    struct Child {pid_t pid=-1;int read=-1,write=-1;~Child(){if(pid>0){::kill(pid,SIGKILL);int status;while(::waitpid(pid,&status,0)<0&&errno==EINTR){}}
        if(read>=0)::close(read);if(write>=0)::close(write);}} child[2];
    {
        for(auto& c:child){int pipe[2];assert(::pipe2(pipe,O_CLOEXEC)==0);c.read=pipe[0];c.write=pipe[1];}
        for(unsigned i=0;i<2;++i){const auto pid=::fork();assert(pid>=0);if(!pid){::alarm(15);for(unsigned j=0;j<2;++j){::close(child[j].write);if(i!=j)::close(child[j].read);}
                char command=0;const bool done=::read(child[i].read,&command,1)==1&&command=='X';::_exit(done?0:1);}child[i].pid=pid;}
        std::array<LinuxNetworkProcessCandidate,2> candidates;
        for(unsigned i=0;i<2;++i){::close(child[i].read);child[i].read=-1;const auto identity=read_linux_process_identity("/proc/"+std::to_string(child[i].pid));assert(identity.value);
            candidates[i]={static_cast<std::uint32_t>(child[i].pid),identity.value->start_ticks};}
        const auto no_owned=descriptors();LinuxProcessNetworkSelectionOwner owner;auto prepared=owner.refresh(true,candidates);assert(prepared.value&&prepared.value->added==2&&descriptors()==no_owned+2);
        auto rows=owner.selections();assert(rows.value&&rows.value->size()==2&&rows.value->at(0).socket_network_namespace_cookie==0);
        auto moved=std::move(owner);assert(!owner.selections().value);auto reverse=candidates;std::swap(reverse[0],reverse[1]);auto same=moved.refresh(true,reverse);
        assert(same.value&&!same.value->changed&&same.value->generation==prepared.value->generation&&descriptors()==no_owned+2);
        const auto copy=::fork();assert(copy>=0);if(!copy){const bool rejected=!moved.selections().value;moved.clear();::_exit(rejected?0:1);}int copied=0;
        assert(::waitpid(copy,&copied,0)==copy&&WIFEXITED(copied)&&WEXITSTATUS(copied)==0&&moved.selections().value);
        auto remove=moved.refresh(true,std::span(candidates).last(1));assert(remove.value&&remove.value->retired==1&&descriptors()==no_owned+1);
        assert(moved.refresh(true,candidates).value&&descriptors()==no_owned+2);
        auto stale=candidates;++stale[0].process_start_ticks;assert(!moved.refresh(true,stale).value&&!moved.selections().value&&descriptors()==no_owned);
        assert(moved.refresh(true,candidates).value);
        char command='X';assert(::write(child[1].write,&command,1)==1);int status=0;assert(::waitpid(child[1].pid,&status,0)>0&&WIFEXITED(status)&&WEXITSTATUS(status)==0);child[1].pid=-1;
        assert(!moved.refresh(true,candidates).value&&!moved.selections().value&&descriptors()==no_owned);
        assert(moved.refresh(true,std::span(candidates).first(1)).value);auto empty=moved.refresh(true,{});assert(empty.value&&empty.value->retired==1&&moved.selections().value->empty()&&descriptors()==no_owned);
        moved.clear();assert(::write(child[0].write,&command,1)==1);assert(::waitpid(child[0].pid,&status,0)>0&&WIFEXITED(status)&&WEXITSTATUS(status)==0);child[0].pid=-1;
        for(auto& c:child){::close(c.write);c.write=-1;}
    }
    assert(descriptors()==baseline);::alarm(0);std::cout<<"PASS read-only selection owner: two private children, pidfds acquired/retained/retired; reorder unchanged generation; live move/fork guard; stale start and exited actor fail closed; empty clears; no FD leak; no BPF/capture/signal delivery to children\n";return 0;
}
#endif
}
int main(int argc,char** argv){if(argc==2&&std::string_view(argv[1])=="--private-children"){
#ifdef __linux__
    return native();
#else
    return 77;
#endif
}assert(argc==1);fixture();}
