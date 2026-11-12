#include <pcinsight/linux_process_network_selection.hpp>
#include "../src/linux_process_details.hpp"
#include "../src/linux_network_shared.hpp"
#include "../src/linux_network_join.hpp"
#include "../src/linux_network_shared_selected.hpp"
#include "../src/linux_process_network.hpp"
#include "../src/linux_network_admission.hpp"
#include <algorithm>
#include <array>
#include <iostream>
#include <stdexcept>
#include <string_view>
#include <cerrno>
#include <chrono>
#include <utility>
#include <thread>
#include <csignal>
#include <dirent.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <unistd.h>
using namespace pcinsight;
namespace {
void require(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
struct Fd {int n=-1;~Fd(){if(n>=0)::close(n);}};
void close_fd(int& n){if(n>=0){::close(n);n=-1;}}
void transfer(int fd,void* data,std::size_t size,bool write){auto p=static_cast<char*>(data);while(size){const auto n=write?::write(fd,p,size) : ::read(fd,p,size);
    if(n<0&&errno==EINTR)continue;require(n>0,"private child pipe failed");p+=n;size-=static_cast<std::size_t>(n);}}
std::size_t descriptors(){DIR* raw=::opendir("/proc/self/fd");require(raw,"FD census failed");std::size_t n=0;while(auto e=::readdir(raw))if(e->d_name[0]!='.')++n;::closedir(raw);return n;}
struct Udp {
    Fd fd; sockaddr_in address{};
    Udp(){fd.n=::socket(AF_INET,SOCK_DGRAM|SOCK_CLOEXEC,0);require(fd.n>=0,"UDP socket");address.sin_family=AF_INET;address.sin_addr.s_addr=htonl(INADDR_LOOPBACK);
        require(::bind(fd.n,reinterpret_cast<sockaddr*>(&address),sizeof(address))==0,"private UDP bind");socklen_t size=sizeof(address);require(::getsockname(fd.n,reinterpret_cast<sockaddr*>(&address),&size)==0,"private UDP address");}
    std::uint64_t cookie(){
#ifdef SO_NETNS_COOKIE
        std::uint64_t cookie=0;socklen_t size=sizeof(cookie);require(::getsockopt(fd.n,SOL_SOCKET,SO_NETNS_COOKIE,&cookie,&size)==0&&size==sizeof(cookie)&&cookie,"independent SO_NETNS_COOKIE oracle");return cookie;
#else
        throw std::runtime_error("SO_NETNS_COOKIE absent from headers");
#endif
    }
    void traffic(std::size_t bytes){std::array<char,256> payload{},received{};require(bytes<=payload.size(),"private payload bound");
        require(::sendto(fd.n,payload.data(),bytes,0,reinterpret_cast<sockaddr*>(&address),sizeof(address))==static_cast<ssize_t>(bytes),"private UDP send");
        require(::recv(fd.n,received.data(),received.size(),0)==static_cast<ssize_t>(bytes),"private UDP receive");}
};
void private_tcp(int family,std::size_t bytes){
    Fd listener,client,peer;listener.n=::socket(family,SOCK_STREAM|SOCK_CLOEXEC,0);client.n=::socket(family,SOCK_STREAM|SOCK_CLOEXEC,0);require(listener.n>=0&&client.n>=0,"private TCP sockets");
    sockaddr_storage storage{};socklen_t length=0;
    if(family==AF_INET){auto& a=*reinterpret_cast<sockaddr_in*>(&storage);a.sin_family=AF_INET;a.sin_addr.s_addr=htonl(INADDR_LOOPBACK);length=sizeof(a);}
    else{auto& a=*reinterpret_cast<sockaddr_in6*>(&storage);a.sin6_family=AF_INET6;a.sin6_addr=in6addr_loopback;length=sizeof(a);}
    auto address=reinterpret_cast<sockaddr*>(&storage);require(::bind(listener.n,address,length)==0&&::listen(listener.n,1)==0,"private TCP listener");
    require(::getsockname(listener.n,address,&length)==0&&::connect(client.n,address,length)==0,"private TCP connect");peer.n=::accept4(listener.n,nullptr,nullptr,SOCK_CLOEXEC);require(peer.n>=0,"private TCP accept");
    std::array<char,256> data{};require(bytes<=data.size(),"private TCP payload bound");
    for(const auto& pair:{std::pair{client.n,peer.n},std::pair{peer.n,client.n}}){
        require(::send(pair.first,data.data(),bytes,MSG_NOSIGNAL)==static_cast<ssize_t>(bytes),"private TCP send");
        require(::recv(pair.second,data.data(),bytes,MSG_WAITALL)==static_cast<ssize_t>(bytes),"private TCP recv");}
}
void private_udp6(std::size_t bytes){Fd fd;fd.n=::socket(AF_INET6,SOCK_DGRAM|SOCK_CLOEXEC,0);require(fd.n>=0,"private UDP6 socket");sockaddr_in6 a{};a.sin6_family=AF_INET6;a.sin6_addr=in6addr_loopback;socklen_t n=sizeof(a);
    require(::bind(fd.n,reinterpret_cast<sockaddr*>(&a),n)==0&&::getsockname(fd.n,reinterpret_cast<sockaddr*>(&a),&n)==0,"private UDP6 address");std::array<char,256> data{};require(bytes<=data.size(),"private UDP6 bound");
    require(::sendto(fd.n,data.data(),bytes,0,reinterpret_cast<sockaddr*>(&a),n)==static_cast<ssize_t>(bytes),"private UDP6 send");require(::recv(fd.n,data.data(),data.size(),0)==static_cast<ssize_t>(bytes),"private UDP6 receive");}
struct Child {
    pid_t pid=-1;int control[2]{-1,-1},report[2]{-1,-1};
    ~Child(){if(pid>0){::kill(pid,SIGKILL);int s=0;while(::waitpid(pid,&s,0)<0&&errno==EINTR){}}for(int fd:{control[0],control[1],report[0],report[1]})if(fd>=0)::close(fd);}
};
template<std::size_t count,class Go,class Phase> void shared_test(const std::vector<LinuxProcessNetworkSelection>& rows,const std::array<std::uint64_t,count>& cookies,LinuxProcessNetworkSelectionOwner& owner,std::size_t no_capture,Go go,Phase phase){
    using namespace pcinsight::detail;
    const auto api=native_network_session_api();const auto clock=native_network_sampler_api();const auto l=api.layout(api.context);require(l.value.has_value(),l.note.c_str());
    LinuxPidfdIdentityQuery query(api,clock.context,clock.local_process);const auto ready=query.start(true,*l.value);require(ready.value.has_value(),ready.note.c_str());
    std::vector<PrivateNetworkIdentity> tokens;
    for(const auto& r:rows){const auto j=read_private_pidfd_network_join(true,r.pidfd,r.pid,r.process_start_ticks,&*l.value,&query);require(j.value.has_value(),j.note.c_str());tokens.push_back(j.value->kernel);}
    query.stop();LinuxSharedNetworkSession capture;auto started=capture.start(true,tokens);require(started.value.has_value(),started.note.c_str());owner.clear();
    require(descriptors()==no_capture+8,"shared capture must own exactly2maps/3programs/3links, independent of17actors");phase("shared-started");
    auto take=[&]{auto r=capture.read();require(r.value.has_value()&&r.support==Support::available,r.note.c_str());require(r.value->actors.size()==count,"shared actor cardinality");return *r.value;};
    auto verify=[&](const PrivateSharedNetworkRead& r,unsigned rounds,unsigned tcp=0){require(r.completed_updates==(2*rounds+8*tcp)*count,"shared global operation count");
        for(std::size_t i=0;i<count;++i){const auto& a=r.actors[i];require(a.identity.kernel_tgid==tokens[i].kernel_tgid&&a.identity.kernel_real_uid==tokens[i].kernel_real_uid&&a.identity.leader_start_boottime_ns==tokens[i].leader_start_boottime_ns,"shared actor identity");
            if(!rounds&&!tcp){require(a.buckets.empty()&&a.completed_updates==0,"shared invented empty bucket");continue;}
            require(a.completed_updates==2*rounds+8*tcp&&a.buckets.size()==1&&a.buckets[0].socket_namespace_cookie==cookies[i],"shared cookie/sequence oracle");
            require(a.buckets[0].excluded_positive_receives==0&&a.buckets[0].bytes==std::array<std::uint64_t,4>{4*tcp*(64+i),4*tcp*(64+i),rounds*(64+i),rounds*(64+i)},"shared per-actor payload oracle");}};
    verify(take(),0);go();verify(take(),1);go();verify(take(),2);go('V');verify(take(),3);go('C');verify(take(),3,1);phase("shared-measured");
    {Udp parent;parent.traffic(100);private_udp6(100);private_tcp(AF_INET,100);private_tcp(AF_INET6,100);}verify(take(),3,1);
    const auto copied=::fork();require(copied>=0,"shared fork guard");if(!copied){const bool rejected=!capture.read().value;capture.stop();::_exit(rejected?0:1);}
    int status=0;require(::waitpid(copied,&status,0)==copied&&WIFEXITED(status)&&WEXITSTATUS(status)==0,"shared child collector not rejected");verify(take(),3,1);
    const auto retained=capture.retain(true,std::span(tokens).subspan(1));require(retained.value.has_value(),retained.note.c_str());
    go();auto subset=capture.read();require(subset.value&&subset.value->actors.size()==count-1&&subset.value->completed_updates==14*count+2*(count-1),"shared removal global sequence/cardinality");
    for(std::size_t i=1;i<count;++i){const auto& a=subset.value->actors[i-1];require(a.identity.kernel_tgid==tokens[i].kernel_tgid&&a.completed_updates==16&&a.buckets.size()==1&&
        a.buckets[0].bytes==std::array<std::uint64_t,4>{4*(64+i),4*(64+i),4*(64+i),4*(64+i)},"shared retained counters lost origin");}
    const auto before=subset.value->completed_updates;require(!capture.retain(true,tokens).value,"shared retired slot re-admitted");
    require(capture.retain(true,std::span(tokens).last(1)).value.has_value(),"shared second removal");go();subset=capture.read();
    require(subset.value&&subset.value->actors.size()==1&&subset.value->completed_updates==before+2&&subset.value->actors[0].identity.kernel_tgid==tokens.back().kernel_tgid&&
        subset.value->actors[0].buckets[0].bytes==std::array<std::uint64_t,4>{4*80,4*80,5*80,5*80},"shared retired traffic counted or retained origin reset");
    require(descriptors()==no_capture+8,"shared removal reloaded/detached producer");phase("shared-retained");
    require(capture.retain(true,{}).value.has_value(),"shared empty retention stop");require(descriptors()==no_capture,"shared capture FD leak");
    started=capture.start(true,tokens);require(started.value.has_value(),started.note.c_str());verify(take(),0);go();verify(take(),1);capture.stop();require(descriptors()==no_capture,"shared restart FD leak");phase("shared-stopped");
}
template<std::size_t count,class Go,class Phase> void shared_window_test(const std::vector<LinuxProcessNetworkSelection>& initial,const std::array<LinuxNetworkProcessCandidate,count>& candidates,
    const std::array<std::uint64_t,count>& cookies,std::array<Child,count>& children,LinuxProcessNetworkSelectionOwner& owner,std::size_t no_capture,Go go,Phase phase){
    using namespace pcinsight::detail;
    auto selections=[](const std::vector<LinuxProcessNetworkSelection>& source){std::vector<PrivateNetworkSelection> out;for(const auto& s:source)out.push_back({s.pidfd,s.pid,s.process_start_ticks,0});return out;};
    LinuxSharedSelectedNetworkSampler sampler;auto epoch=sampler.start(true,selections(initial));require(epoch.value.has_value(),epoch.note.c_str());owner.clear();std::size_t active=count;
    require(descriptors()==no_capture+active+14,"shared window must own Npidfds+8captureFD+6queryFD");phase("shared-window-started");
    auto take=[&]{auto frame=sampler.sample_common();require(frame.value.has_value()&&frame.support==Support::available,frame.note.c_str());
        auto projected=publish_linux_process_network(*frame.value);require(projected.value.has_value()&&projected.support==Support::estimated,projected.note.c_str());
        require(projected.value->observed_processes.size()==active&&projected.value->selection_epoch==epoch.value,"shared window metadata/epoch");return std::pair{std::move(*frame.value),std::move(*projected.value)};};
    auto row=[&](const LinuxProcessNetworkSnapshot& s,std::size_t i)->const LinuxProcessNetworkSample&{const auto p=std::find_if(s.processes.begin(),s.processes.end(),[&](const auto& p){return p.process.pid==candidates[i].pid;});
        require(p!=s.processes.end()&&p->process.start_ticks==candidates[i].process_start_ticks&&p->kernel.socket_network_namespace_cookie==cookies[i],"shared window process/cookie association");return *p;};
    auto bytes=[&](const LinuxProcessNetworkSnapshot& s,unsigned udp,unsigned tcp){require(s.processes.size()==active,"shared window bucket cardinality");for(std::size_t i=0;i<active;++i)
        require(row(s,i).payload_bytes_since_start.value==std::array<std::uint64_t,4>{4*tcp*(64+i),4*tcp*(64+i),udp*(64+i),udp*(64+i)},"shared window four-lane payload oracle");};
    auto no_rates=[&](const LinuxProcessNetworkSnapshot& s){for(const auto& p:s.processes){require(!p.nominal_interval_ns.value,"shared window invented first interval");for(const auto& r:p.payload_bytes_per_second)require(!r.value,"shared window invented first rate");}};
    auto rates=[&](const LinuxProcessNetworkSnapshot& previous,const LinuxProcessNetworkSnapshot& current){const auto ns=current.counter_sweep_finished_ns-previous.counter_sweep_finished_ns;require(ns>0&&current.users.size()==1,"shared window nominal interval/UID shape");std::array<std::uint64_t,4> sum{};
        for(std::size_t i=0;i<active;++i){const auto& before=row(previous,i);const auto& after=row(current,i);require(after.nominal_interval_ns.value==ns,"shared window common interval");
            for(unsigned lane=0;lane<4;++lane){const auto delta=after.payload_bytes_since_start.value->at(lane)-before.payload_bytes_since_start.value->at(lane);const auto expected=delta*1'000'000'000/ns;
                require(after.payload_bytes_per_second[lane].value==expected&&after.payload_bytes_per_second[lane].support==Support::estimated,"shared window actor rate oracle");sum[lane]+=expected;}}
        for(unsigned lane=0;lane<4;++lane)require(current.users[0].known[lane]==active&&current.users[0].missing[lane]==0&&!current.users[0].overflow[lane]&&current.users[0].partial_payload_bytes_per_second[lane].value==sum[lane],"shared window partial UID oracle");};
    auto empty=take();require(empty.second.processes.empty()&&empty.second.users.empty(),"shared window empty registry invented activity");go();auto first=take();bytes(first.second,1,0);no_rates(first.second);
    go();go('V');go('C');auto measured=take();bytes(measured.second,3,1);rates(first.second,measured.second);phase("shared-window-measured");
    {Udp parent;parent.traffic(100);private_udp6(100);private_tcp(AF_INET,100);private_tcp(AF_INET6,100);}auto idle=take();bytes(idle.second,3,1);rates(measured.second,idle.second);
    for(const auto& p:idle.second.processes)for(const auto& r:p.payload_bytes_per_second)require(r.value==0,"shared window parent traffic attributed");
    go();std::optional<LinuxProcessNetworkSnapshot> threaded;std::exception_ptr error;
    std::thread worker([&]{try{threaded=take().second;}catch(...){error=std::current_exception();}});worker.join();if(error)std::rethrow_exception(error);require(threaded.has_value(),"shared worker result missing");bytes(*threaded,4,1);rates(idle.second,*threaded);
    auto main_again=take();bytes(main_again.second,4,1);rates(*threaded,main_again.second);
    const auto copy=::fork();require(copy>=0,"shared window fork");if(!copy){const bool rejected=!sampler.sample_common().value;sampler.stop();::_exit(rejected?0:1);}
    int status=0;require(::waitpid(copy,&status,0)==copy&&WIFEXITED(status)&&WEXITSTATUS(status)==0,"shared window fork collector not rejected");auto after_fork=take();bytes(after_fork.second,4,1);rates(main_again.second,after_fork.second);
    auto& last=children.back();char exit='X';transfer(last.control[1],&exit,1,true);const auto reaped=::waitpid(last.pid,&status,0);require(reaped==last.pid&&WIFEXITED(status)&&WEXITSTATUS(status)==0,"shared private exit");last.pid=-1;
    require(!sampler.sample_common().value&&!sampler.sample_common().value,"shared exited actor published idle/partial frame");sampler.stop();require(descriptors()==no_capture,"shared window stop FD leak");
    active=count-1;const auto ready=owner.refresh(true,std::span(candidates).first(active));require(ready.value.has_value(),ready.note.c_str());const auto fresh=owner.selections();require(fresh.value.has_value(),fresh.note.c_str());
    const auto old=epoch.value;epoch=sampler.start(true,selections(*fresh.value));require(epoch.value&&epoch.value!=old,epoch.note.c_str());owner.clear();require(descriptors()==no_capture+active+14,"shared window restart reference count");
    empty=take();require(empty.second.processes.empty()&&empty.second.users.empty(),"shared restart invented old buckets");go();auto warm=take();bytes(warm.second,1,0);no_rates(warm.second);
    sampler.discard_window();auto discarded=take();bytes(discarded.second,1,0);no_rates(discarded.second);go();auto resumed=take();bytes(resumed.second,2,0);rates(discarded.second,resumed.second);
    auto update=[&](std::size_t n,bool reverse=false){auto ready=owner.refresh(true,std::span(candidates).first(n));require(ready.value.has_value(),ready.note.c_str());
        auto proposed=owner.selections();require(proposed.value.has_value(),proposed.note.c_str());auto chosen=selections(*proposed.value);if(reverse)std::reverse(chosen.begin(),chosen.end());
        const auto old_epoch=epoch.value;epoch=sampler.update_selection(true,chosen);require(epoch.value&&epoch.value!=old_epoch,epoch.note.c_str());owner.clear();active=n;};
    update(count-2);require(descriptors()==no_capture+active+14,"shared retained batch reloaded during removal");
    go();auto reduced=take();bytes(reduced.second,3,0);no_rates(reduced.second); // removed child STILL sends
    update(count-1);require(descriptors()==no_capture+active+22,"shared new admission must add exactly one8FD batch");
    auto readmitted=take();require(readmitted.second.processes.size()==active-1,"shared readmission included old/gap bucket");no_rates(readmitted.second);
    go();auto mixed=take();require(mixed.second.processes.size()==active,"shared readmitted first bucket absent");
    for(std::size_t i=0;i<active;++i){const auto& p=row(mixed.second,i);const auto rounds=i+1==active?1u:4u;
        require(p.payload_bytes_since_start.value==std::array<std::uint64_t,4>{0,0,rounds*(64+i),rounds*(64+i)},"shared retained origin/readmission gap oracle");
        if(i+1==active)for(const auto& rate:p.payload_bytes_per_second)require(!rate.value,"shared re-admitted first rate invented");}
    go();go('V');go('C');auto updated=take();rates(mixed.second,updated.second);
    for(std::size_t i=0;i<active;++i){const auto rounds=i+1==active?3u:6u;require(row(updated.second,i).payload_bytes_since_start.value==std::array<std::uint64_t,4>{4*(64+i),4*(64+i),rounds*(64+i),rounds*(64+i)},"shared updated TCP/UDP oracle");}
    update(count-1,true);require(descriptors()==no_capture+active+22,"shared reorder rebuilt producer");auto reordered=take();no_rates(reordered.second);
    for(std::size_t i=0;i<active;++i)require(row(reordered.second,i).payload_bytes_since_start.value==row(updated.second,i).payload_bytes_since_start.value,"shared reorder reset counters");
    phase("shared-selection-updated");
    sampler.stop();require(descriptors()==no_capture,"shared window restart FD leak");phase("shared-window-stopped");
}
template<std::size_t count> int run(unsigned mode){
    const auto began=std::chrono::steady_clock::now();auto phase=[&](const char* label){std::cerr<<"capacity"<<count<<" phase="<<label<<" elapsed_ms="<<std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now()-began).count()<<'\n';};
    ::signal(SIGPIPE,SIG_IGN);::signal(SIGCHLD,SIG_DFL);::alarm(60);const auto baseline=descriptors();
    {
        std::array<Child,count> children;
        for(auto& c:children)require(::pipe2(c.control,O_CLOEXEC)==0&&::pipe2(c.report,O_CLOEXEC)==0,"capacity private pipes");
        for(std::size_t i=0;i<count;++i){const auto pid=::fork();require(pid>=0,"capacity private fork");if(!pid){::alarm(55);
                for(std::size_t j=0;j<count;++j){::close(children[j].control[1]);::close(children[j].report[0]);if(i!=j){::close(children[j].control[0]);::close(children[j].report[1]);}}
                try{Udp socket;auto cookie=socket.cookie();transfer(children[i].report[1],&cookie,sizeof(cookie),true);
                    for(;;){char command=0;transfer(children[i].control[0],&command,1,false);if(command=='X')::_exit(0);require(command=='G'||command=='C'||command=='V',"capacity child command");
                        if(command=='G')socket.traffic(64+i);else if(command=='V')private_udp6(64+i);else{private_tcp(AF_INET,64+i);private_tcp(AF_INET6,64+i);}char done='D';transfer(children[i].report[1],&done,1,true);}
                }catch(...){::_exit(1);}
            }children[i].pid=pid;
        }
        std::array<LinuxNetworkProcessCandidate,count> candidates;std::array<std::uint64_t,count> cookies{};
        for(std::size_t i=0;i<count;++i){auto& c=children[i];close_fd(c.control[0]);close_fd(c.report[1]);transfer(c.report[0],&cookies[i],sizeof(cookies[i]),false);
            const auto identity=detail::read_linux_process_identity("/proc/"+std::to_string(c.pid));require(identity.value.has_value(),"capacity child start identity");candidates[i]={static_cast<std::uint32_t>(c.pid),identity.value->start_ticks};}
        const auto no_capture=descriptors();LinuxProcessNetworkSelectionOwner owner;const auto ready=owner.refresh(true,candidates);require(ready.value.has_value(),ready.note.c_str());
        const auto rows=owner.selections();require(rows.value&&rows.value->size()==count,"capacity prepared rows");
        phase("prepared");
        auto go=[&](char command='G'){for(auto& c:children)if(c.pid>0)transfer(c.control[1],&command,1,true);for(auto& c:children)if(c.pid>0){char done=0;transfer(c.report[0],&done,1,false);require(done=='D',"capacity private workload");}};
        if(mode==2)shared_window_test(*rows.value,candidates,cookies,children,owner,no_capture,go,phase);
        else if(mode==1)shared_test(*rows.value,cookies,owner,no_capture,go,phase);
        else{
        LinuxProcessNetworkSampler sampler(LinuxProcessNetworkSelectionMode::observed_namespace_buckets);const auto started=sampler.start(true,*rows.value);require(started.value.has_value(),started.note.c_str());owner.clear();
        phase("started");
        require(descriptors()==no_capture+count+6+8*((count+63)/64),"PUBLIC observed FD count must match Npidfds+6query+8 per batch of at most64");
        auto take=[&]{auto r=sampler.sample();require(metric_is_known(r)&&r.support==Support::estimated,r.note.c_str());
            require(r.value->coverage==LinuxProcessNetworkCoverage::observed_tcp_udp_socket_namespace_buckets&&r.value->observed_processes.size()==count&&r.value->selection_epoch==started.value,"capacity shape/coverage/epoch");return *r.value;};
        auto row=[&](const LinuxProcessNetworkSnapshot& w,std::size_t i)->const LinuxProcessNetworkSample&{const auto at=std::find_if(w.processes.begin(),w.processes.end(),[&](const auto& p){return p.process.pid==candidates[i].pid;});
            require(at!=w.processes.end()&&at->process.start_ticks==candidates[i].process_start_ticks&&at->kernel.socket_network_namespace_cookie==cookies[i],"capacity process/cookie key");return *at;};
        auto empty=take();require(empty.processes.empty()&&empty.users.empty(),"capacity initial invented buckets");
        phase("empty");
        go();auto first=take();require(first.processes.size()==count,"capacity first bucket cardinality");
        phase("first");
        for(std::size_t i=0;i<count;++i){const auto& p=row(first,i);require(!p.nominal_interval_ns.value&&!p.payload_bytes_per_second[2].value,"capacity first invented rate");
            require(p.payload_bytes_since_start.value==std::array<std::uint64_t,4>{0,0,64+i,64+i},"capacity first payload oracle");}
        go();auto measured=take();const auto ns=measured.counter_sweep_finished_ns-first.counter_sweep_finished_ns;require(ns>0,"capacity nominal interval");std::array<std::uint64_t,4> sum{};
        phase("measured");
        require(measured.processes.size()==count&&measured.users.size()==1,"capacity same-UID grouping");
        for(std::size_t i=0;i<count;++i){const auto& p=row(measured,i);require(p.nominal_interval_ns.value==ns&&p.excluded_positive_receives==0,"capacity interval/exclusions");
            for(unsigned lane=0;lane<4;++lane){const auto delta=lane<2?0:64+i;const auto rate=delta*1'000'000'000/ns;
                require(p.payload_bytes_since_start.value->at(lane)==2*delta&&p.payload_bytes_per_second[lane].value==rate&&p.payload_bytes_per_second[lane].support==Support::estimated,"capacity four-lane rate oracle");sum[lane]+=rate;}}
        for(unsigned lane=0;lane<4;++lane)require(measured.users[0].known[lane]==count&&measured.users[0].missing[lane]==0&&!measured.users[0].overflow[lane]&&measured.users[0].partial_payload_bytes_per_second[lane].value==sum[lane],"capacity partial UID oracle");
        {Udp parent;parent.traffic(100);}auto idle=take();for(const auto& p:idle.processes)for(const auto& rate:p.payload_bytes_per_second)require(rate.value==0,"capacity parent traffic counted");
        sampler.stop();require(descriptors()==no_capture,"capacity capture/owned pidfd FD leak");
        phase("stopped");
        if constexpr(count>64){
            // Native inventory/owner/BPF, but enumeration is restricted to our
            // private children. This MUST NOT enumerate/capture the host view.
            using namespace pcinsight::detail;std::vector<std::uint32_t> pids;for(const auto& c:candidates)pids.push_back(c.pid);
            auto inventory=native_network_inventory_api();inventory.context=&pids;
            inventory.enumerate=[](void* p,const NetworkSelectionContext&)->Metric<std::vector<std::uint32_t>>{
                return {*static_cast<std::vector<std::uint32_t>*>(p),Support::available,"TEST private children only, not full /proc"};};
            LinuxNetworkAdmission visible(inventory,native_network_selection_source_api(),network_admission_capture_api(sampler));
            auto change=visible.refresh(true);require(change.value&&change.value->verified_processes==count&&change.value->complete_encountered_set&&change.value->excluded.empty(),change.note.c_str());
            require(descriptors()==no_capture+2*count+6+8*((count+63)/64),"large admission owner/capture FD census");phase("admitted");
            auto qualified=[&]{auto r=visible.sample_with_admission();require(r.value&&r.support==Support::estimated,r.note.c_str());
                require(r.value->admission.selection_epoch==r.value->network.selection_epoch&&r.value->admission.selection_epoch==change.value->selection_epoch&&r.value->network.observed_processes.size()==count,"large admission paired frame");return std::move(*r.value);};
            require(qualified().network.processes.empty(),"large admission invented initial buckets");go();auto a=qualified();
            auto same=visible.refresh(true);require(same.value&&!same.value->selection_changed&&same.value->selection_epoch==change.value->selection_epoch,"large unchanged admission changed epoch");
            go();auto b=qualified();const auto dt=b.network.counter_sweep_finished_ns-a.network.counter_sweep_finished_ns;require(dt>0,"large admission rate interval");
            for(std::size_t i=0;i<count;++i){const auto& p=row(b.network,i);require(p.payload_bytes_since_start.value==std::array<std::uint64_t,4>{0,0,2*(64+i),2*(64+i)}&&p.nominal_interval_ns.value==dt&&p.payload_bytes_per_second[2].value==(64+i)*1'000'000'000/dt,"large admission payload/rate oracle");}
            visible.stop();require(descriptors()==no_capture,"large admission FD leak");phase("admission-stopped");
        }
        }
        for(auto& c:children)if(c.pid>0){char command='X';transfer(c.control[1],&command,1,true);int status=0;const auto pid=::waitpid(c.pid,&status,0);if(pid==c.pid)c.pid=-1;
            require(pid>0&&WIFEXITED(status)&&WEXITSTATUS(status)==0,"capacity private child exit");}
    }
    require(descriptors()==baseline,"capacity harness FD leak");::alarm(0);
    if(mode==2){std::cout<<"PASS shared authenticated windows17: owned pidfds/owner cleared;2joins per actor/window(each native join2kernel queries);initial3shared traffic links+1gated identity link; initial N+14owned FDs; empty actors/new bucket no rates; TCP/UDP IPv4/IPv6 four-lane payload/common interval/partial UID integer oracles via existing projection; main-worker-main; parent excluded/fork rejected-parent intact; exited actor twice rejects ALL; stop/restart16 new epoch/empty/new counters/discard baseline/recovery; update16->15->16 retains origin/removed child sends excluded/re-admission fresh counters excluding gap/new epoch-reset rates/reorder preserves counters; second batch exactly8FDs, per-batch coherent common sweep NOT cross-batch atomic; no FD leak/all children normal exit. INTERNAL authenticated update, projection check NOT public backend activation; NO automatic admission. ONE real shared netns/SO oracle initialization;64x16 fixtures,not real64 certification\n";return 0;}
    if(mode==1){std::cout<<"PASS shared capacity17:3 traffic links/3programs/2maps for17 private actors, kernel tokens point-joined before start; UDP64..80 IPv4 twice/IPv6 once and TCP IPv4/IPv6 4x64..80 per direction, four payload lanes/cookie/sequence oracles, empty registry and whole-read coherence; parent TCP/UDP excluded, collector fork rejected/parent unchanged; removal17->16->1 retains origin/FDs, retired private actors keep sending but are excluded, re-admission refused, empty set stops; restart empty/counters fresh, no FD leak/all children normal exit. INTERNAL raw producer only: NO ongoing pidfd join/rates/public backend/general admission. ONE real shared netns/SO oracle initialization;64x16 fixtures, not real64 certification\n";return 0;}
    std::cout<<"PASS capacity"<<count<<" PUBLIC shared: private selected processes; readonly owner -> observed PUBLIC capture; census N+6+8*ceil(N/64)FDs; empty metadata/first bucket no rates; per-actor UDP payload64+i twice, four-lane/common interval and partial UID oracles; parent excluded; owner cleared/capture owned references; no FD leak/all children normal exit. For count>64 also native admission over PRIVATE injected enumeration,2N+6+8*ceil(N/64)FDs, paired epoch/counters/rates and unchanged refresh preserves baseline. ONE real shared netns/SO oracle initialization; larger capacity/buckets are fixtures, not native certification or general/global capture\n";return 0;
}
}
int main(int argc,char** argv){const bool shared=argc==2&&std::string_view(argv[1])=="--authorized-private-shared-capacity17";
    if(argc==2&&std::string_view(argv[1])=="--authorized-private-capacity65")try{return run<65>(0);}catch(const std::exception& e){std::cerr<<"FAIL capacity65: "<<e.what()<<'\n';return 1;}
    const bool windows=argc==2&&std::string_view(argv[1])=="--authorized-private-shared-windows17";
    if(!shared&&!windows&&(argc!=2||std::string_view(argv[1])!="--authorized-private-capacity17")){std::cerr<<"NO capture: requires explicit authorization and private capacity17 flag\n";return 77;}
    try{return run<17>(windows?2:shared?1:0);}catch(const std::exception& e){std::cerr<<"FAIL capacity17: "<<e.what()<<'\n';return 1;}}
