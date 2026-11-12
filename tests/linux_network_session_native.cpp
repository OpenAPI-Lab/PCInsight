#include "../src/linux_network_session.hpp"
#include "../src/linux_network_sampler.hpp"
#include "../src/linux_network_pidfd.hpp"
#include "../src/linux_network_join.hpp"
#include "../src/linux_network_selected_sampler.hpp"
#include "../src/linux_network_multi.hpp"
#include <pcinsight/linux_process_network.hpp>
#include <pcinsight/linux_process_network_selection.hpp>
#include <pcinsight/monitor.hpp>
#include <pcinsight/sampler.hpp>
#include "../src/linux_process_details.hpp"
#include <array>
#include <algorithm>
#include <cerrno>
#include <fcntl.h>
#include <cstring>
#include <dirent.h>
#include <iostream>
#include <stdexcept>
#include <string_view>
#include <thread>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/syscall.h>
#include <poll.h>
#include <signal.h>
#include <unistd.h>
using namespace pcinsight;using namespace pcinsight::detail;
namespace {
void require(bool ok,const char* note){if(!ok)throw std::runtime_error(note);}
struct Fd {int fd;explicit Fd(int v):fd(v){}void close(){if(fd>=0){::close(fd);fd=-1;}}~Fd(){close();}Fd(const Fd&)=delete;};
unsigned descriptors(){auto* dir=::opendir("/proc/self/fd");require(dir,"fd inventory");unsigned n=0;
    while(auto* e=::readdir(dir))if(e->d_name[0]!='.')++n;::closedir(dir);return n;}
void configure(int fd){require(fd>=0,"socket");timeval timeout{2,0};
    require(::setsockopt(fd,SOL_SOCKET,SO_RCVTIMEO,&timeout,sizeof(timeout))==0,"receive timeout");
    require(::setsockopt(fd,SOL_SOCKET,SO_SNDTIMEO,&timeout,sizeof(timeout))==0,"send timeout");}
struct Endpoint {sockaddr_storage address{};socklen_t size{};};
Endpoint bind_loopback(int fd,int family){Endpoint e;
    if(family==AF_INET){sockaddr_in a{};a.sin_family=AF_INET;a.sin_addr.s_addr=htonl(INADDR_LOOPBACK);std::memcpy(&e.address,&a,sizeof(a));e.size=sizeof(a);}
    else{sockaddr_in6 a{};a.sin6_family=AF_INET6;a.sin6_addr=in6addr_loopback;std::memcpy(&e.address,&a,sizeof(a));e.size=sizeof(a);}
    require(::bind(fd,reinterpret_cast<sockaddr*>(&e.address),e.size)==0,"bind");
    require(::getsockname(fd,reinterpret_cast<sockaddr*>(&e.address),&e.size)==0,"getsockname");return e;}
void traffic(int family,bool message){
    std::array<char,1024> sent{},got{};sent.fill('s');
    Fd listener(::socket(family,SOCK_STREAM|SOCK_CLOEXEC,0));configure(listener.fd);auto e=bind_loopback(listener.fd,family);
    require(::listen(listener.fd,1)==0,"listen");Fd client(::socket(family,SOCK_STREAM|SOCK_CLOEXEC,0));configure(client.fd);
    require(::connect(client.fd,reinterpret_cast<sockaddr*>(&e.address),e.size)==0,"connect");
    Fd server(::accept4(listener.fd,nullptr,nullptr,SOCK_CLOEXEC));configure(server.fd);
    iovec data{sent.data(),sent.size()};msghdr msg{};msg.msg_iov=&data;msg.msg_iovlen=1;
    require((message?::sendmsg(client.fd,&msg,MSG_NOSIGNAL) : ::send(client.fd,sent.data(),sent.size(),MSG_NOSIGNAL))==1024,"TCP send");
    require(::recv(server.fd,got.data(),1,MSG_PEEK)==1,"TCP peek");
    std::size_t total=0;while(total<got.size()){auto n=::recv(server.fd,got.data()+total,got.size()-total,0);require(n>0,"TCP recv");total+=static_cast<std::size_t>(n);}
    require(sent==got,"TCP payload");
    Fd receiver(::socket(family,SOCK_DGRAM|SOCK_CLOEXEC,0));configure(receiver.fd);e=bind_loopback(receiver.fd,family);
    Fd sender(::socket(family,SOCK_DGRAM|SOCK_CLOEXEC,0));configure(sender.fd);
    data.iov_len=64;msg.msg_name=&e.address;msg.msg_namelen=e.size;
    require((message?::sendmsg(sender.fd,&msg,MSG_NOSIGNAL) : ::sendto(sender.fd,sent.data(),64,MSG_NOSIGNAL,reinterpret_cast<sockaddr*>(&e.address),e.size))==64,"UDP send");
    require(::recv(receiver.fd,got.data(),1,MSG_PEEK)==1,"UDP peek");require(::recv(receiver.fd,got.data(),got.size(),0)==64,"UDP recv");
}
PrivateNetworkQualityCounters read(LinuxPrivateNetworkSession& s){auto r=s.read();require(metric_is_known(r),r.note.c_str());return r.value->counters;}
bool same(const PrivateNetworkQualityCounters& a,const PrivateNetworkQualityCounters& b){return a.bytes==b.bytes&&a.excluded_positive_receives==b.excluded_positive_receives&&a.completed_updates==b.completed_updates;}
std::uint64_t socket_namespace_cookie(){
#ifdef SO_NETNS_COOKIE
    Fd socket(::socket(AF_INET,SOCK_DGRAM|SOCK_CLOEXEC,0));configure(socket.fd);
    std::uint64_t cookie=0;socklen_t size=sizeof(cookie);
    require(::getsockopt(socket.fd,SOL_SOCKET,SO_NETNS_COOKIE,&cookie,&size)==0&&size==sizeof(cookie)&&cookie,"SO_NETNS_COOKIE");return cookie;
#else
    throw std::runtime_error("SO_NETNS_COOKIE absent from build headers");
#endif
}
PrivateNetworkWindow window(LinuxPrivateNetworkSampler& sampler,bool joined){
    if(joined){auto r=sampler.sample_joined_self();require(r.value.has_value(),r.note.c_str());
        require(r.value->local.local_pid==static_cast<std::uint32_t>(::getpid())&&r.value->local.local_real_uid==static_cast<std::uint32_t>(::getuid())&&r.value->local.start_ticks,"joined local identity");
        return std::move(r.value->window);}
    auto r=sampler.sample();require(r.value.has_value(),r.note.c_str());return std::move(*r.value);
}
int test_window(bool joined){
    const auto baseline=descriptors();const auto cookie=socket_namespace_cookie();LinuxPrivateNetworkSampler sampler;
    const auto identity=sampler.start(true,cookie);if(!identity.value){std::cerr<<"UNAVAILABLE: "<<identity.note<<'\n';return 77;}
    auto first=window(sampler,joined);require(first.accounting.actors.size()==1&&first.accounting.users.size()==1,"SELF coverage");
    require(!first.accounting.actors[0].interval_ns.value,"first window fabricated rate");
    const auto scope=first.accounting.scope;
    require(scope.pid_domain==NetworkIdentityDomain::kernel_initial&&scope.user_domain==NetworkIdentityDomain::kernel_initial&&
        scope.coverage==NetworkAccountingCoverage::self_single_socket_namespace&&!scope.pid_namespace&&!scope.user_namespace,"kernel initial domains");
    for(int family:{AF_INET,AF_INET6})for(bool message:{false,true})traffic(family,message);
    auto next=window(sampler,joined);const auto& actor=next.accounting.actors[0];const auto& user=next.accounting.users[0];
    require(actor.actor.tgid==identity.value->kernel_tgid&&actor.actor.real_uid==identity.value->kernel_real_uid&&
        actor.actor.leader_start_boottime_ns==identity.value->leader_start_boottime_ns&&actor.actor.socket_network_namespace_cookie==cookie,"actor key");
    require(actor.interval_ns.value.has_value(),"missing measured interval");const auto ns=*actor.interval_ns.value;
    require(ns==next.read_finished_ns-first.read_finished_ns,"interval uses wrong clock");
    const std::array<std::uint64_t,4> bytes{4096,4096,256,256};
    require(actor.bytes.value==bytes&&next.quality.excluded_positive_receives==8,"window bytes/peeks");
    for(unsigned i=0;i<4;++i){const auto expected=bytes[i]*1'000'000'000/ns;
        require(actor.bytes_per_second[i].value==expected&&actor.bytes_per_second[i].support==Support::estimated,"rate oracle");
        require(user.bytes_per_second[i].value==expected&&user.known[i]==1&&user.missing[i]==0,"partial UID sum");}
    auto idle=window(sampler,joined);for(const auto& rate:idle.accounting.actors[0].bytes_per_second)require(rate.value==0,"idle fabricated absence");
    const auto child=::fork();require(child>=0,"fork window");
    if(child==0){const bool rejected=joined?!sampler.sample_joined_self().value:!sampler.sample().value;sampler.stop();::_exit(rejected?0:1);}
    int status=0;require(::waitpid(child,&status,0)==child&&WIFEXITED(status)&&WEXITSTATUS(status)==0,"fork SELF not rejected");
    idle=window(sampler,joined);require(idle.accounting.actors[0].bytes.value==bytes,"child changed parent capture");
    sampler.stop();require(descriptors()==baseline,"window FD leak");
    require(sampler.start(true,cookie).value.has_value(),"window restart");auto restarted=window(sampler,joined);
    require(restarted.accounting.scope.producer_epoch!=scope.producer_epoch&&!restarted.accounting.actors[0].interval_ns.value,"restart reused baseline/epoch");
    sampler.stop();require(descriptors()==baseline,"window restart FD leak");::alarm(0);
    std::cout<<"PASS SELF windows: TCP4096/4096 UDP256/256 peek8; four rate/UID lanes match oracle; interval_ns="<<ns
        <<"; idle0; fork rejected; restart epoch/baseline clean; no FD leak\n";
    if(joined)std::cout<<"PASS direct SELF join local_pid="<<::getpid()<<" kernel_tgid="<<identity.value->kernel_tgid<<" local_uid="<<::getuid()
        <<" kernel_uid="<<identity.value->kernel_real_uid<<"; repeated procfs/caller/leader and fresh BPF identity checks\n";
    return 0;
}
}
namespace {
struct PrivateChildToken {PrivateNetworkIdentity kernel;LinuxNetworkSelfIdentity local;std::uint64_t socket_cookie{};};
void transfer(int fd,void* data,std::size_t size,bool writing){
    auto* bytes=static_cast<char*>(data);unsigned interrupts=0;
    while(size){const auto n=writing?::write(fd,bytes,size) : ::read(fd,bytes,size);
        if(n<0&&errno==EINTR&&++interrupts<=4)continue;
        require(n>0,"private child pipe transfer");bytes+=n;size-=static_cast<std::size_t>(n);}
}
int test_private_child(bool discover_by_pidfd,bool window_mode){
    ::signal(SIGPIPE,SIG_IGN);::signal(SIGCHLD,SIG_DFL); // Test-process-only signal policy.
    const auto baseline=descriptors();
    {
        int control_raw[2]{-1,-1},report_raw[2]{-1,-1};require(::pipe2(control_raw,O_CLOEXEC)==0,"control pipe");
        Fd control_read(control_raw[0]),control_write(control_raw[1]);require(::pipe2(report_raw,O_CLOEXEC)==0,"report pipe");
        Fd report_read(report_raw[0]),report_write(report_raw[1]);
        const auto pid=::fork();require(pid>=0,"fork private target");
        if(pid==0){
            ::alarm(window_mode?35:15);::close(control_write.fd);::close(report_read.fd);
            try{
                const auto layout=read_linux_network_btf();require(layout.value.has_value(),"child BTF");
                PrivateNetworkIdentity kernel{};
                if(!discover_by_pidfd){const auto identity=read_private_network_caller_identity(native_network_session_api(),*layout.value);require(identity.value.has_value(),"child kernel token");kernel=*identity.value;}
                const auto local=read_linux_network_self();require(local.value.has_value(),"child SELF tokens");
                PrivateChildToken token{kernel,*local.value,socket_namespace_cookie()};transfer(report_write.fd,&token,sizeof(token),true);
                for(;;){char command=0;transfer(control_read.fd,&command,1,false);if(command=='X')::_exit(0);require(command=='G',"private child command");
                    for(int family:{AF_INET,AF_INET6})for(bool message:{false,true})traffic(family,message);
                    char done='D';transfer(report_write.fd,&done,1,true);}
            }catch(...){::_exit(1);}
        }
        struct ChildGuard {pid_t pid;~ChildGuard(){if(pid>0){::kill(pid,SIGKILL);int status=0;while(::waitpid(pid,&status,0)<0&&errno==EINTR){}}}} child{pid};
        control_read.close();report_write.close();
        Fd pidfd(static_cast<int>(::syscall(SYS_pidfd_open,pid,0)));require(pidfd.fd>=0,"private target pidfd");
        auto alive=[&]{pollfd fd{pidfd.fd,POLLIN,0};require(::poll(&fd,1,0)==0,"private child exited or pidfd error");};
        PrivateChildToken token{};transfer(report_read.fd,&token,sizeof(token),false);
        require(token.local.local_pid==static_cast<std::uint32_t>(pid)&&valid_network_self(token.local),"child local token not owned fork target");
        auto query=[&](int handle){const auto count=descriptors();sigset_t before{},after{};require(::pthread_sigmask(SIG_SETMASK,nullptr,&before)==0,"get mask before");
            auto result=read_private_pidfd_network_identity(true,handle);require(::pthread_sigmask(SIG_SETMASK,nullptr,&after)==0,"get mask after");
            for(int signal=1;signal<NSIG;++signal)require(::sigismember(&before,signal)==::sigismember(&after,signal),"query signal mask not restored");
            require(descriptors()==count,"query FD leak");return result;};
        if(discover_by_pidfd){
            require(token.kernel.kernel_tgid==0&&token.kernel.leader_start_boottime_ns==0,"child unexpectedly reported kernel identity");
            auto identity=query(pidfd.fd);if(!identity.value){std::cerr<<"UNAVAILABLE pidfd identity: "<<identity.note<<'\n';return 77;}
            const auto checked=query(pidfd.fd);require(checked.value&&checked.value->kernel_tgid==identity.value->kernel_tgid&&
                checked.value->kernel_real_uid==identity.value->kernel_real_uid&&checked.value->leader_start_boottime_ns==identity.value->leader_start_boottime_ns,"pidfd identity changed during revalidation");
            require(!query(control_write.fd).value,"non-pidfd accepted");token.kernel=*identity.value;
            const auto parent_read=read_linux_process_identity("/proc/"+std::to_string(pid));
            require(parent_read.value.has_value(),"parent selected proc identity");
            const auto start=parent_read.value->start_ticks;
            const auto before_join=descriptors();
            auto joined=read_private_pidfd_network_join(true,pidfd.fd,static_cast<std::uint32_t>(pid),start);
            require(joined.value.has_value(),joined.note.c_str());
            require(joined.value->kernel.kernel_tgid==token.kernel.kernel_tgid&&joined.value->kernel.kernel_real_uid==token.kernel.kernel_real_uid&&
                joined.value->kernel.leader_start_boottime_ns==token.kernel.leader_start_boottime_ns&&joined.value->local.process==token.local,"pidfd/procfs child association mismatch");
            auto again=read_private_pidfd_network_join(true,pidfd.fd,static_cast<std::uint32_t>(pid),start);
            require(again.value&&again.value->local==joined.value->local,"repeated selected join changed");
            require(!read_private_pidfd_network_join(true,pidfd.fd,static_cast<std::uint32_t>(pid),start+1).value,"stale selected start accepted");
            require(!read_private_pidfd_network_join(true,pidfd.fd,static_cast<std::uint32_t>(::getpid()),start).value,"wrong local PID accepted");
            require(!read_private_pidfd_network_join(true,control_write.fd,static_cast<std::uint32_t>(pid),start).value,"pipe joined as pidfd");
            require(descriptors()==before_join,"join FD leak");
            std::cout<<"PASS selected pidfd/procfs join: parent-read start, repeated bracket, separate local/kernel IDs; stale start/wrong PID/pipe rejected; no FD leak\n";
        }
        alive();LinuxPrivateNetworkSession selected;auto started=selected.start_selected(true,token.kernel,token.socket_cookie);
        if(!started.value){std::cerr<<"UNAVAILABLE selected target: "<<started.note<<'\n';return 77;}
        auto initial=selected.read();require(initial.value&&initial.value->target_kind==PrivateNetworkTargetKind::selected_actor,"selected actor metadata");
        require((initial.value->counters.bytes==std::array<std::uint64_t,4>{}),"selected initial counters");
        auto go=[&]{alive();char command='G';transfer(control_write.fd,&command,1,true);char done=0;transfer(report_read.fd,&done,1,false);require(done=='D',"child completion");alive();};
        go();auto measured=selected.read();require(measured.value.has_value(),measured.note.c_str());
        require((measured.value->counters.bytes==std::array<std::uint64_t,4>{4096,4096,256,256}),"external child four lanes");
        require(measured.value->counters.excluded_positive_receives==8,"child peeks");
        for(int family:{AF_INET,AF_INET6})for(bool message:{false,true})traffic(family,message);
        auto unchanged=selected.read();require(unchanged.value&&same(measured.value->counters,unchanged.value->counters),"collector parent counted as target");
        selected.detach();go();unchanged=selected.read();require(unchanged.value&&same(measured.value->counters,unchanged.value->counters),"new child traffic after detach counted");selected.stop();
        for(bool wrong_uid:{false,true}){auto invalid=token.kernel;if(wrong_uid)invalid.kernel_real_uid^=1;else ++invalid.leader_start_boottime_ns;
            require(selected.start_selected(true,invalid,token.socket_cookie).value.has_value(),"selected negative setup");go();
            require(!selected.read().value&&!selected.read().value,"selected identity mismatch not sticky");selected.stop();}
        LinuxSelectedNetworkSampler selected_window;
        if(window_mode){
            const auto before_window=descriptors();
            Fd caller_copy(::fcntl(pidfd.fd,F_DUPFD_CLOEXEC,0));require(caller_copy.fd>=0,"window caller copy");
            auto identity=selected_window.start(true,caller_copy.fd,token.local.local_pid,token.local.start_ticks,token.socket_cookie);
            require(identity.value.has_value(),identity.note.c_str());caller_copy.close(); // Sampler owns a different duplicate.
            auto first=selected_window.sample();require(first.value.has_value(),first.note.c_str());
            require(!first.value->window.accounting.actors[0].interval_ns.value,"selected first window fabricated interval");
            const auto scope=first.value->window.accounting.scope;
            require(scope.coverage==NetworkAccountingCoverage::selected_single_socket_namespace&&scope.producer_tgid!=token.kernel.kernel_tgid,"selected coverage/producer confused with target");
            go();auto measured_window=selected_window.sample();require(measured_window.value.has_value(),measured_window.note.c_str());
            const auto& w=measured_window.value->window;const auto& actor=w.accounting.actors[0];
            const auto ns=w.read_finished_ns-first.value->window.read_finished_ns;
            const std::array<std::uint64_t,4> bytes{4096,4096,256,256};
            require(ns&&actor.interval_ns.value==ns&&actor.bytes.value==bytes,"selected window measured interval/bytes");
            require(measured_window.value->local.process==token.local&&actor.actor.tgid==token.kernel.kernel_tgid&&
                actor.actor.real_uid==token.kernel.kernel_real_uid&&w.quality.excluded_positive_receives==8,"selected joined identity/quality");
            for(unsigned lane=0;lane<4;++lane){const auto rate=bytes[lane]*1'000'000'000/ns;
                require(actor.bytes_per_second[lane].value==rate&&actor.bytes_per_second[lane].support==Support::estimated,"selected rate oracle");
                require(w.accounting.users[0].bytes_per_second[lane].value==rate&&w.accounting.users[0].known[lane]==1&&w.accounting.users[0].missing[lane]==0,"selected partial UID oracle");}
            for(int family:{AF_INET,AF_INET6})for(bool message:{false,true})traffic(family,message);
            auto idle=selected_window.sample();require(idle.value.has_value(),idle.note.c_str());
            for(const auto& rate:idle.value->window.accounting.actors[0].bytes_per_second)require(rate.value==0,"selected idle/parent traffic not zero");
            const auto copy=::fork();require(copy>=0,"selected fork guard setup");
            if(copy==0){const bool rejected=!selected_window.sample().value;selected_window.stop();::_exit(rejected?0:1);}
            int copied_status=0;require(::waitpid(copy,&copied_status,0)==copy&&WIFEXITED(copied_status)&&WEXITSTATUS(copied_status)==0,"selected forked collector not rejected");
            idle=selected_window.sample();require(idle.value&&idle.value->window.accounting.actors[0].bytes.value==bytes,"fork changed selected parent capture");
            selected_window.stop();require(descriptors()==before_window,"selected window FD leak");
            require(selected_window.start(true,pidfd.fd,token.local.local_pid,token.local.start_ticks,token.socket_cookie).value.has_value(),"selected window restart");
            auto restarted=selected_window.sample();require(restarted.value&&!restarted.value->window.accounting.actors[0].interval_ns.value&&
                restarted.value->window.accounting.scope.producer_epoch!=scope.producer_epoch,"selected restart reused baseline/epoch");
            std::cout<<"PASS selected associated windows: four payload/UID rate lanes match integer oracle; interval_ns="<<ns
                <<"; caller FD closed safely; parent excluded/idle0; fork rejected; restart epoch/baseline clean; no FD leak\n";
        }
        require(selected.start_selected(true,token.kernel,token.socket_cookie).value.has_value(),"selected restart");
        char command='X';transfer(control_write.fd,&command,1,true);int status=0;const auto reaped=::waitpid(pid,&status,0);
        if(reaped==pid)child.pid=-1;
        require(reaped==pid&&WIFEXITED(status)&&WEXITSTATUS(status)==0,"private child exit");
        pollfd exited{pidfd.fd,POLLIN,0};require(::poll(&exited,1,0)==1&&(exited.revents&POLLIN),"pidfd exit not observable");
        if(discover_by_pidfd)require(!query(pidfd.fd).value,"exited pidfd identity accepted");
        if(discover_by_pidfd)require(!read_private_pidfd_network_join(true,pidfd.fd,token.local.local_pid,token.local.start_ticks).value,"exited pidfd join accepted");
        if(window_mode){require(!selected_window.sample().value&&!selected_window.sample().value,"exited selected window accepted");selected_window.stop();}
        selected.stop();
        std::cout<<"PASS selected private child local_pid="<<token.local.local_pid<<" kernel_tgid="<<token.kernel.kernel_tgid
            <<" uid="<<token.kernel.kernel_real_uid<<"; TCP4096/4096 UDP256/256 peek8; parent excluded; detach frozen; wrong start/UID sticky; pidfd exit; resources closed\n";
        if(discover_by_pidfd)std::cout<<"PASS pidfd identity: child reported no kernel token; repeated discovery, invalid handle/exit rejected; signal mask and FD count restored\n";
    }
    require(descriptors()==baseline,"private child FD leak");::alarm(0);return 0;
}
}
namespace {
struct MonitorNetworkAdapter {
    Monitor monitor;SampleOptions options;
    LinuxProcessNetworkSelectionMode mode;
    explicit MonitorNetworkAdapter(LinuxProcessNetworkSelectionMode m=LinuxProcessNetworkSelectionMode::strict_single_namespace):mode(m){options.include_processes=options.include_disks=options.include_networks=options.include_gpus=false;options.include_linux_process_network=true;}
    auto start(bool authorized,std::span<const LinuxProcessNetworkSelection> selection){return mode==LinuxProcessNetworkSelectionMode::strict_single_namespace?monitor.start_linux_process_network(authorized,selection):monitor.start_linux_process_network(authorized,selection,mode);}
    auto update_selection(bool authorized,std::span<const LinuxProcessNetworkSelection> selection){return monitor.update_linux_process_network_selection(authorized,selection);}
    auto sample(){return monitor.sample(options).linux_process_network;}
    void stop(){monitor.stop_linux_process_network();}
    void set_included(bool included){options.include_linux_process_network=included;}
};
int test_private_multi(bool common_mode,bool public_mode,bool monitor_mode,bool update_mode,bool worker_mode,bool bucket_mode,bool bucket_worker,bool discovery_mode,bool discovered_window,bool observed_multi,bool public_observed,bool observed_monitor,bool observed_worker,bool owned_selection){
    ::signal(SIGPIPE,SIG_IGN);::signal(SIGCHLD,SIG_DFL);
    const auto baseline=descriptors();
    {
        struct Child {
            pid_t pid=-1;int control[2]{-1,-1},report[2]{-1,-1},pidfd=-1;
            ~Child(){if(pid>0){::kill(pid,SIGKILL);int status=0;while(::waitpid(pid,&status,0)<0&&errno==EINTR){}}
                for(int fd:{control[0],control[1],report[0],report[1],pidfd})if(fd>=0)::close(fd);}
        } children[2];
        auto close_fd=[](int& fd){if(fd>=0){::close(fd);fd=-1;}};
        for(auto& c:children){require(::pipe2(c.control,O_CLOEXEC)==0,"multi control pipe");require(::pipe2(c.report,O_CLOEXEC)==0,"multi report pipe");}
        for(unsigned i=0;i<2;++i){const auto pid=::fork();require(pid>=0,"multi fork");
            if(pid==0){::alarm(50);
                for(unsigned n=0;n<2;++n){::close(children[n].control[1]);::close(children[n].report[0]);
                    if(n!=i){::close(children[n].control[0]);::close(children[n].report[1]);}}
                try{auto cookie=socket_namespace_cookie();transfer(children[i].report[1],&cookie,sizeof(cookie),true);
                    for(;;){char command=0;transfer(children[i].control[0],&command,1,false);if(command=='X')::_exit(0);require(command=='G'||command=='T',"multi command");
                        auto workload=[&]{for(unsigned repeat=0;repeat<=i;++repeat)for(int family:{AF_INET,AF_INET6})for(bool msg:{false,true})traffic(family,msg);};
                        if(command=='T'){std::exception_ptr first,second;
                            std::thread a([&]{try{workload();}catch(...){first=std::current_exception();}}),b([&]{try{workload();}catch(...){second=std::current_exception();}});
                            a.join();b.join();if(first)std::rethrow_exception(first);if(second)std::rethrow_exception(second);
                        }else workload();
                        char done='D';transfer(children[i].report[1],&done,1,true);}
                }catch(...){::_exit(1);}
            }children[i].pid=pid;
        }
        std::array<PrivateNetworkSelection,2> selection{};
        for(unsigned i=0;i<2;++i){auto& c=children[i];close_fd(c.control[0]);close_fd(c.report[1]);
            c.pidfd=static_cast<int>(::syscall(SYS_pidfd_open,c.pid,0));require(c.pidfd>=0,"multi pidfd");
            std::uint64_t cookie=0;transfer(c.report[0],&cookie,sizeof(cookie),false);
            const auto identity=read_linux_process_identity("/proc/"+std::to_string(c.pid));require(identity.value.has_value(),"multi parent proc stat");
            selection[i]={c.pidfd,static_cast<std::uint32_t>(c.pid),identity.value->start_ticks,cookie};
        }
        if(observed_worker){
            SampleOptions options;options.include_processes=options.include_disks=options.include_networks=options.include_gpus=false;options.include_linux_process_network=true;
            Sampler background(std::chrono::milliseconds(200),128,options);std::array<LinuxProcessNetworkSelection,2> requested;
            for(unsigned i=0;i<2;++i)requested[i]={selection[i].pidfd,selection[i].pid,selection[i].start_ticks,0};
            const auto expected_after_stop=descriptors()-2;
            auto epoch=background.start_linux_process_network(true,requested,LinuxProcessNetworkSelectionMode::observed_namespace_buckets);require(epoch.value.has_value(),epoch.note.c_str());
            require(descriptors()==expected_after_stop+18,"observed Sampler must own2pidfds+6queryFDs+8sharedFDs plus2inputFDs");
            for(auto& c:children)close_fd(c.pidfd);background.start();
            auto wait_frame=[&](auto accept){const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(12);
                while(std::chrono::steady_clock::now()<deadline){for(const auto& item:background.timed_history()){const auto& r=item.snapshot.linux_process_network;
                        if(metric_is_known(r)&&r.value->selection_epoch==epoch.value&&accept(*r.value))return *r.value;}
                    require(background.running(),background.last_error().c_str());std::this_thread::sleep_for(std::chrono::milliseconds(20));}
                throw std::runtime_error("observed worker frame timeout");};
            auto first=[&](std::size_t count){auto w=wait_frame([](const auto&){return true;});require(w.coverage==LinuxProcessNetworkCoverage::observed_tcp_udp_socket_namespace_buckets&&w.observed_processes.size()==count&&!w.previous_counter_sweep,"observed worker epoch shape/first bracket");
                for(const auto& row:w.processes)require(!row.nominal_interval_ns.value,"observed worker crossed epoch baseline");return w;};
            auto find=[&](const LinuxProcessNetworkSnapshot& w,unsigned i)->const LinuxProcessNetworkSample*{const auto p=std::find_if(w.processes.begin(),w.processes.end(),[&](const auto& p){return p.process.pid==requested[i].pid;});return p==w.processes.end()?nullptr:&*p;};
            const std::array<std::uint64_t,4> unit{4096,4096,256,256};
            auto go=[&](unsigned i){char command='G';transfer(children[i].control[1],&command,1,true);char done=0;transfer(children[i].report[0],&done,1,false);require(done=='D',"observed worker private workload");};
            auto totals=[&](unsigned a,unsigned b){return wait_frame([&](const auto& w){for(unsigned i=0;i<2;++i){const auto p=find(w,i);if(!p||!p->payload_bytes_since_start.value)return false;
                        for(unsigned lane=0;lane<4;++lane)if(p->payload_bytes_since_start.value->at(lane)!=unit[lane]*(i?b:a))return false;}return true;});};
            auto oracles=[&]{std::optional<LinuxProcessNetworkSnapshot> prior;unsigned intervals=0;
                for(const auto& timed:background.timed_history()){const auto& report=timed.snapshot.linux_process_network;if(!metric_is_known(report)||report.value->selection_epoch!=epoch.value)continue;const auto& w=*report.value;
                    if(prior&&w.previous_counter_sweep){const auto ns=w.counter_sweep_finished_ns-prior->counter_sweep_finished_ns;require(ns>0,"observed worker nominal interval");
                        std::array<std::uint64_t,4> sum{},known{},missing{};
                        for(const auto& row:w.processes){const auto old=std::find_if(prior->processes.begin(),prior->processes.end(),[&](const auto& p){return p.kernel==row.kernel&&p.process==row.process;});
                            for(unsigned lane=0;lane<4;++lane){if(old==prior->processes.end()){require(!row.payload_bytes_per_second[lane].value&&!row.nominal_interval_ns.value,"observed worker new bucket invented baseline");++missing[lane];}
                                else{const auto rate=(row.payload_bytes_since_start.value->at(lane)-old->payload_bytes_since_start.value->at(lane))*1'000'000'000/ns;
                                    require(row.payload_bytes_per_second[lane].value==rate&&row.payload_bytes_per_second[lane].support==Support::estimated&&row.nominal_interval_ns.value==ns,"observed worker four-lane rate oracle");sum[lane]+=rate;++known[lane];}}}
                        if(!w.processes.empty()){require(w.users.size()==1,"observed worker UID count");for(unsigned lane=0;lane<4;++lane){require(w.users[0].known[lane]==known[lane]&&w.users[0].missing[lane]==missing[lane],"observed worker UID availability");
                                if(known[lane])require(w.users[0].partial_payload_bytes_per_second[lane].value==sum[lane],"observed worker partial UID oracle");else require(!w.users[0].partial_payload_bytes_per_second[lane].value,"observed worker missing UID invented zero");}}
                        ++intervals;
                    }prior=w;
                }require(intervals>0,"observed worker no measured interval");};
            auto empty=first(2);require(empty.processes.empty()&&empty.users.empty()&&empty.observed_processes[0].observed_buckets==0,"observed worker fabricated initial traffic");
            go(0);go(1);auto measured=totals(1,2);oracles();
            require(!background.start_linux_process_network(true,requested,LinuxProcessNetworkSelectionMode::strict_single_namespace).value,"observed worker allowed running setup/mode switch");
            require(background.update_linux_process_network_selection(false,requested).support==Support::permission_required,"observed worker update consent bypass");
            const auto old=epoch.value;epoch=background.update_linux_process_network_selection(true,std::span(requested).last(1));require(epoch.value&&epoch.value!=old&&background.running(),"observed worker removal epoch");
            auto warm=first(1);require(warm.processes.size()==1&&find(warm,1)->payload_bytes_since_start.value==find(measured,1)->payload_bytes_since_start.value,"observed worker retained origin changed");go(0);
            children[0].pidfd=static_cast<int>(::syscall(SYS_pidfd_open,children[0].pid,0));require(children[0].pidfd>=0,"observed worker addition pidfd");requested[0].pidfd=children[0].pidfd;
            Metric<std::uint64_t> added;std::jthread updater([&]{added=background.update_linux_process_network_selection(true,requested);});(void)background.latest();(void)background.timed_history();updater.join();close_fd(children[0].pidfd);
            require(added.value&&background.running(),added.note.c_str());epoch=added;warm=first(2);require(!find(warm,0)&&find(warm,1),"observed worker readd replayed gap/removed retained bucket");
            go(0);go(1);(void)totals(1,4);oracles();
            require(!background.update_linux_process_network_selection(true,{}).value&&background.running(),"observed worker failed update stopped worker/accepted empty");
            const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(12);bool absent=false;
            while(std::chrono::steady_clock::now()<deadline){const auto last=background.latest();if(last&&!last->linux_process_network.value){absent=true;break;}require(background.running(),"observed worker stopped on capture absence");std::this_thread::sleep_for(std::chrono::milliseconds(20));}
            require(absent&&!background.update_linux_process_network_selection(true,requested).value,"observed worker failure silently rearmed");background.stop();
            require(background.last_error().empty()&&descriptors()==expected_after_stop,"observed worker capture/FD error");
            children[0].pidfd=static_cast<int>(::syscall(SYS_pidfd_open,children[0].pid,0));require(children[0].pidfd>=0,"observed worker default setup pidfd");requested[0].pidfd=children[0].pidfd;
            require(!background.start_linux_process_network(true,std::span(requested).first(1)).value,"observed worker default retained observed mode");close_fd(children[0].pidfd);
            background.start();const auto restart_deadline=std::chrono::steady_clock::now()+std::chrono::seconds(12);
            while(!background.latest()&&background.running()&&std::chrono::steady_clock::now()<restart_deadline)std::this_thread::sleep_for(std::chrono::milliseconds(20));
            const auto restarted=background.latest();background.stop();require(restarted&&!restarted->linux_process_network.value&&descriptors()==expected_after_stop,"observed worker restart reauthorized/leaked");
            for(auto& child:children){char command='X';transfer(child.control[1],&command,1,true);int status=0;const auto reaped=::waitpid(child.pid,&status,0);if(reaped==child.pid)child.pid=-1;require(reaped>0&&WIFEXITED(status)&&WEXITSTATUS(status)==0,"observed worker child exit");}
            std::cout<<"PASS observed Sampler worker: two private selected actors/cookie0, empty metadata then discovered traffic; four actor/partial UID rate oracles and new-bucket baseline absence; active selection AB->B->AB, concurrent updater/history, retained origin/new actor excludes gap/epoch baseline reset; denied update unchanged; failed update closes capture while worker continues; default setup strict after stop/restart never rearms; no FD leak. ONE real netns shared, SO oracle initialization; no general/global capture\n";
        }else if(public_observed){
            std::array<LinuxProcessNetworkSelection,2> requested;
            for(unsigned i=0;i<2;++i)requested[i]={selection[i].pidfd,selection[i].pid,selection[i].start_ticks,0};
            LinuxProcessNetworkSelectionOwner owner;
            if(owned_selection){std::array<LinuxNetworkProcessCandidate,2> candidates;
                for(unsigned i=0;i<2;++i)candidates[i]={selection[i].pid,selection[i].start_ticks};
                const auto prepared=owner.refresh(true,candidates);require(prepared.value.has_value(),prepared.note.c_str());
                const auto rows=owner.selections();require(rows.value&&rows.value->size()==2,"prepared selection rows absent");
                for(const auto& row:*rows.value){const auto at=std::find_if(requested.begin(),requested.end(),[&](const auto& r){return r.pid==row.pid;});require(at!=requested.end(),"prepared selection unexpected actor");*at=row;}
                for(auto& child:children)close_fd(child.pidfd); // Only owner handles remain before downstream setup.
            }
            auto exercise=[&](auto& sampler){
            auto invalid=requested;invalid[0].socket_network_namespace_cookie=selection[0].socket_cookie;
            require(!sampler.start(true,invalid).value,"public observed silently ignored declared cookie");
            LinuxProcessNetworkSampler strict;require(!strict.start(true,requested).value,"default strict accepted discovery selection");
            const auto expected_after_stop=descriptors()-2;auto epoch=sampler.start(true,requested);require(epoch.value.has_value(),epoch.note.c_str());
            require(descriptors()==expected_after_stop+18,"PUBLIC/Monitor observed backend must use one shared batch, not fanout");
            for(auto& child:children)close_fd(child.pidfd);
            owner.clear(); // Downstream owns independent pidfd references after successful setup.
            auto active=std::move(sampler);require(!sampler.sample().value,"moved-from observed sampler active");
            auto take=[&](std::size_t selected,std::size_t buckets){auto r=active.sample();require(metric_is_known(r)&&r.support==Support::estimated,r.note.c_str());
                require(r.value->coverage==LinuxProcessNetworkCoverage::observed_tcp_udp_socket_namespace_buckets&&r.value->selection_epoch==epoch.value&&r.value->observed_processes.size()==selected&&r.value->processes.size()==buckets,"public observed shape/coverage/epoch");
                return *r.value;};
            auto metadata=[&](const LinuxProcessNetworkSnapshot& w,unsigned i)->const LinuxNetworkObservedProcess&{
                const auto p=std::find_if(w.observed_processes.begin(),w.observed_processes.end(),[&](const auto& row){return row.process.pid==selection[i].pid;});
                require(p!=w.observed_processes.end()&&p->process.start_ticks==selection[i].start_ticks,"public observed selected association");return *p;};
            auto row=[&](const LinuxProcessNetworkSnapshot& w,unsigned i)->const LinuxProcessNetworkSample&{const auto& m=metadata(w,i);
                const auto p=std::find_if(w.processes.begin(),w.processes.end(),[&](const auto& row){return row.process.pid==selection[i].pid;});
                require(p!=w.processes.end()&&p->kernel.initial_tgid==m.initial_tgid&&p->kernel.initial_real_uid==m.initial_real_uid&&p->kernel.leader_start_boottime_ns==m.leader_start_boottime_ns&&
                    p->kernel.socket_network_namespace_cookie==selection[i].socket_cookie&&p->completed_updates==m.completed_updates&&m.observed_buckets==1,"public observed kernel/cookie/sequence oracle");return *p;};
            auto go=[&](unsigned i,char command='G'){transfer(children[i].control[1],&command,1,true);char done=0;transfer(children[i].report[0],&done,1,false);require(done=='D',"public observed workload");};
            auto empty=take(2,0);require(empty.users.empty()&&!empty.previous_counter_sweep&&metadata(empty,0).observed_buckets==0&&metadata(empty,1).completed_updates==0,"public observed fabricated empty traffic");
            go(0);auto first=take(2,1);require(!row(first,0).nominal_interval_ns.value&&metadata(first,1).observed_buckets==0,"public observed first bucket rate");
            go(1);auto before=take(2,2);require(row(before,0).payload_bytes_per_second[0].value==0&&!row(before,1).nominal_interval_ns.value,"public observed late bucket rate");
            go(0,'T');go(1,'T');auto measured=take(2,2);const auto ns=measured.counter_sweep_finished_ns-before.counter_sweep_finished_ns;require(ns>0,"public observed interval");
            const std::array<std::uint64_t,4> unit{4096,4096,256,256};std::array<std::uint64_t,4> sum{};
            for(unsigned i=0;i<2;++i){const auto& p=row(measured,i);require(p.nominal_interval_ns.value==ns&&p.excluded_positive_receives==24*(i+1),"public observed common interval/exclusions");
                for(unsigned lane=0;lane<4;++lane){const auto rate=unit[lane]*2*(i+1)*1'000'000'000/ns;
                    require(p.payload_bytes_since_start.value->at(lane)==unit[lane]*3*(i+1)&&p.payload_bytes_per_second[lane].value==rate&&p.payload_bytes_per_second[lane].support==Support::estimated,"public observed four-lane rate oracle");sum[lane]+=rate;}}
            require(measured.users.size()==1,"public observed same initial UID");for(unsigned lane=0;lane<4;++lane)
                require(measured.users[0].known[lane]==2&&measured.users[0].missing[lane]==0&&measured.users[0].partial_payload_bytes_per_second[lane].value==sum[lane],"public observed partial UID oracle");
            require(active.update_selection(false,requested).support==Support::permission_required,"public observed update consent bypass");
            for(int family:{AF_INET,AF_INET6})for(bool msg:{false,true})traffic(family,msg);auto idle=take(2,2);
            for(const auto& p:idle.processes)for(const auto& rate:p.payload_bytes_per_second)require(rate.value==0,"public observed parent counted");
            const auto copy=::fork();require(copy>=0,"public observed fork setup");if(copy==0){const bool rejected=!active.sample().value;active.stop();::_exit(rejected?0:1);}
            int copied=0;require(::waitpid(copy,&copied,0)==copy&&WIFEXITED(copied)&&WEXITSTATUS(copied)==0,"public observed fork guard");
            const auto old=epoch.value;epoch=active.update_selection(true,std::span(requested).last(1));require(epoch.value&&epoch.value!=old,"public observed remove epoch");
            auto warm=take(1,1);require(!warm.previous_counter_sweep&&!row(warm,1).nominal_interval_ns.value,"public observed remove baseline");go(0);
            require(descriptors()==expected_after_stop+15,"PUBLIC/Monitor removal must preserve one8FD batch+6queryFDs+1pidfd");
            children[0].pidfd=static_cast<int>(::syscall(SYS_pidfd_open,children[0].pid,0));require(children[0].pidfd>=0,"public observed readd pidfd");requested[0].pidfd=children[0].pidfd;
            epoch=active.update_selection(true,requested);require(epoch.value.has_value(),epoch.note.c_str());close_fd(children[0].pidfd);warm=take(2,1);
            require(descriptors()==expected_after_stop+24,"PUBLIC/Monitor re-admission must add exactly one8FD batch+1pidfd");
            require(!warm.previous_counter_sweep&&metadata(warm,0).observed_buckets==0&&row(warm,1).payload_bytes_since_start.value==row(measured,1).payload_bytes_since_start.value,"public observed readd registry/retained origin");
            go(0);warm=take(2,2);for(unsigned lane=0;lane<4;++lane)require(row(warm,0).payload_bytes_since_start.value->at(lane)==unit[lane]&&!row(warm,0).payload_bytes_per_second[lane].value,"public observed readd gap/baseline");
            auto finish=[&](unsigned i){char command='X';transfer(children[i].control[1],&command,1,true);int status=0;const auto reaped=::waitpid(children[i].pid,&status,0);if(reaped==children[i].pid)children[i].pid=-1;
                require(reaped>0&&WIFEXITED(status)&&WEXITSTATUS(status)==0,"public observed child exit");};
            finish(1);require(!active.sample().value&&!active.sample().value,"public observed published partial group after exit");
            epoch=active.update_selection(true,std::span(requested).first(1));require(epoch.value.has_value(),epoch.note.c_str());warm=take(1,1);require(!warm.previous_counter_sweep&&!row(warm,0).nominal_interval_ns.value,"public observed exit removal baseline");
            active.stop();require(descriptors()==expected_after_stop,"public observed stop FD leak");
            if constexpr(requires{active.set_included(false);}){
                require(!active.monitor.start_linux_process_network(true,std::span(requested).first(1)).value,"Monitor default retained observed mode");
            }
            children[0].pidfd=static_cast<int>(::syscall(SYS_pidfd_open,children[0].pid,0));require(children[0].pidfd>=0,"public observed restart pidfd");requested[0].pidfd=children[0].pidfd;
            invalid[0]=requested[0];invalid[0].socket_network_namespace_cookie=selection[0].socket_cookie;
            require(!active.start(true,std::span(invalid).first(1)).value,"public observed stop lost cookie policy");
            epoch=active.start(true,std::span(requested).first(1));require(epoch.value.has_value(),epoch.note.c_str());close_fd(children[0].pidfd);empty=take(1,0);
            require(!empty.previous_counter_sweep&&metadata(empty,0).completed_updates==0,"public observed restart retained registry");
            if constexpr(requires{active.set_included(false);}){
                active.set_included(false);require(!active.sample().value&&descriptors()==expected_after_stop,"Monitor observed opt-out retained capture");
                active.set_included(true);require(!active.sample().value,"Monitor flag rearmed observed capture");
                std::cout<<"PASS Monitor observed integration: metadata preserved through SystemSnapshot; default setup strict after stop; opt-out closes capture; flag alone cannot rearm\n";
            }
            active.stop();
            require(descriptors()==expected_after_stop,"public observed restart FD leak");finish(0);
            std::cout<<"PASS PUBLIC observed-cookie sampler: explicit mode/cookie0/default strict unchanged; two private actors, owned pidfds/closed inputs/live move; selected metadata preserves empty actors; new/late buckets no rate; four payload/rate and partial UID oracles A TCP12288/12288 UDP768/768 peek24, B double/two threads each; parent excluded/fork rejected; consent/update/remove/readd gap and epochs clean; exited actor rejects whole group twice/removal recovers; stop/restart retains observed policy and starts empty; no FD leak. ONE real netns shared; SO_NETNS_COOKIE independently initializes oracle, no general process discovery or multiple-real-netns claim\n";
            };
            if(observed_monitor){MonitorNetworkAdapter sampler(LinuxProcessNetworkSelectionMode::observed_namespace_buckets);exercise(sampler);}
            else{LinuxProcessNetworkSampler sampler(LinuxProcessNetworkSelectionMode::observed_namespace_buckets);exercise(sampler);}
            if(owned_selection)std::cout<<"PASS owned selection -> PUBLIC BPF: readonly PID/start policy acquired pidfds, original handles closed, downstream duplicated owned rows, owner cleared while capture remains valid; private actor oracles and teardown clean\n";
        }else if(observed_multi){
            auto requested=selection;for(auto& row:requested)row.socket_cookie=0;
            LinuxMultiNetworkSampler group(native_network_session_api(),native_network_sampler_api(),native_selected_network_source_api(),false,true);
            const auto expected_after_stop=descriptors()-2;auto epoch=group.start(true,requested);require(epoch.value.has_value(),epoch.note.c_str());
            for(auto& c:children)close_fd(c.pidfd);
            auto take=[&](std::size_t count){auto r=group.sample_common();require(metric_is_known(r),r.note.c_str());const auto& w=*r.value;
                require(w.observed.size()==count&&w.associations.empty()&&w.accounting.scope.coverage==NetworkAccountingCoverage::selected_observed_actor_set&&w.accounting.scope.producer_epoch==epoch.value,"observed multi scope/count/epoch");
                return *r.value;};
            auto observed=[&](const PrivateCommonNetworkWindow& w,unsigned i)->const PrivateCommonNetworkWindow::ObservedActor&{
                const auto p=std::find_if(w.observed.begin(),w.observed.end(),[&](const auto& a){return a.local.process.local_pid==selection[i].pid;});
                require(p!=w.observed.end(),"observed multi process association absent");return *p;};
            auto row=[&](const PrivateCommonNetworkWindow& w,unsigned i)->const NetworkActorRates&{const auto& a=observed(w,i);
                require(a.buckets.size()==1&&a.buckets[0].socket_namespace_cookie==selection[i].socket_cookie,"observed multi independent cookie oracle");
                const auto p=std::find_if(w.accounting.actors.begin(),w.accounting.actors.end(),[&](const auto& r){return r.actor.tgid==a.identity.kernel_tgid&&r.actor.socket_network_namespace_cookie==selection[i].socket_cookie;});
                require(p!=w.accounting.actors.end(),"observed multi accounting row absent");return *p;};
            auto go=[&](unsigned i,char command='G'){transfer(children[i].control[1],&command,1,true);char done=0;transfer(children[i].report[0],&done,1,false);require(done=='D',"observed multi private workload");};
            auto empty=take(2);require(empty.accounting.actors.empty()&&empty.accounting.users.empty()&&observed(empty,0).buckets.empty()&&observed(empty,1).buckets.empty(),"observed multi initial empty registries");
            go(0);auto first=take(2);require(first.accounting.actors.size()==1&&!row(first,0).interval_ns.value&&observed(first,1).buckets.empty(),"observed multi first cookie fabricated rate");
            go(1);auto before=take(2);require(before.accounting.actors.size()==2&&row(before,0).bytes_per_second[0].value==0&&!row(before,1).interval_ns.value,"observed multi late actor baseline");
            go(0,'T');go(1,'T');auto measured=take(2);const auto ns=measured.read_finished_ns-before.read_finished_ns;require(ns>0,"observed multi interval");
            const std::array<std::uint64_t,4> unit{4096,4096,256,256};
            std::array<std::uint64_t,4> sum{};
            for(unsigned i=0;i<2;++i){const auto& a=row(measured,i);require(a.interval_ns.value==ns,"observed multi common nominal interval");
                require(observed(measured,i).buckets[0].excluded_positive_receives==24*(i+1),"observed multi exclusion oracle");
                for(unsigned lane=0;lane<4;++lane){const auto rate=unit[lane]*2*(i+1)*1'000'000'000/ns;
                    require(a.bytes.value->at(lane)==unit[lane]*3*(i+1)&&a.bytes_per_second[lane].value==rate&&a.bytes_per_second[lane].support==Support::estimated,"observed multi four-lane actor oracle");sum[lane]+=rate;}}
            require(measured.accounting.users.size()==1,"observed multi shared initial UID");
            for(unsigned lane=0;lane<4;++lane)require(measured.accounting.users[0].known[lane]==2&&measured.accounting.users[0].missing[lane]==0&&measured.accounting.users[0].bytes_per_second[lane].value==sum[lane],"observed multi partial UID oracle");
            for(int family:{AF_INET,AF_INET6})for(bool msg:{false,true})traffic(family,msg);
            auto idle=take(2);for(const auto& a:idle.accounting.actors)for(const auto& rate:a.bytes_per_second)require(rate.value==0,"observed multi parent traffic counted");
            const auto copied=::fork();require(copied>=0,"observed multi fork setup");if(copied==0){const bool rejected=!group.sample_common().value;group.stop();::_exit(rejected?0:1);}
            int status=0;require(::waitpid(copied,&status,0)==copied&&WIFEXITED(status)&&WEXITSTATUS(status)==0,"observed multi fork guard");
            const auto prior_epoch=epoch.value;epoch=group.update_selection(true,std::span(requested).last(1));require(epoch.value&&epoch.value!=prior_epoch,"observed multi removal epoch");
            auto warm=take(1);require(!warm.previous_read_bracket&&!row(warm,1).interval_ns.value,"observed multi removal retained baseline");go(0); // Gap must not be replayed on re-addition.
            children[0].pidfd=static_cast<int>(::syscall(SYS_pidfd_open,children[0].pid,0));require(children[0].pidfd>=0,"observed multi readd pidfd");requested[0].pidfd=children[0].pidfd;
            epoch=group.update_selection(true,requested);require(epoch.value.has_value(),epoch.note.c_str());close_fd(children[0].pidfd);warm=take(2);
            require(!warm.previous_read_bracket&&observed(warm,0).buckets.empty()&&row(warm,1).bytes.value==row(measured,1).bytes.value&&!row(warm,1).interval_ns.value,"observed multi retained counters/new registry/gap");
            go(0);warm=take(2);for(unsigned lane=0;lane<4;++lane)require(row(warm,0).bytes.value->at(lane)==unit[lane]&&!row(warm,0).bytes_per_second[lane].value,"observed multi readded traffic/baseline");
            auto finish=[&](unsigned i){char command='X';transfer(children[i].control[1],&command,1,true);int result=0;const auto reaped=::waitpid(children[i].pid,&result,0);if(reaped==children[i].pid)children[i].pid=-1;
                require(reaped>0&&WIFEXITED(result)&&WEXITSTATUS(result)==0,"observed multi child exit");};
            finish(1);require(!group.sample_common().value&&!group.sample_common().value,"observed multi published partial group after exit");
            epoch=group.update_selection(true,std::span(requested).first(1));require(epoch.value.has_value(),epoch.note.c_str());warm=take(1);
            require(!warm.previous_read_bracket&&!row(warm,0).interval_ns.value,"observed multi exited removal bridged baseline");
            group.stop();require(descriptors()==expected_after_stop,"observed multi capture/owned pidfd leak");finish(0);
            std::cout<<"PASS observed-cookie common windows: two private selected actors, no cookie arguments; owned pidfds/caller handles closed; empty registries/late buckets have no fabricated rate; concurrent same-actor threads TCP12288/12288 UDP768/768 peek24 and double for second actor; four actor/partial UID rate oracles/common nominal end; parent excluded; fork rejected; removal/readd epochs reset all rates, retained origin/new actor excludes gap; exited actor rejects whole group twice, removal recovers without baseline bridge; no FD leak. ONE real netns shared by both actors; no public/global discovery or multiple-real-netns claim\n";
        }else if(discovered_window){
            const auto expected_after_stop=descriptors()-2;LinuxSelectedNetworkSampler sampler;
            auto start=[&]{const auto r=sampler.start_discovering(true,children[0].pidfd,selection[0].pid,selection[0].start_ticks);require(r.value.has_value(),r.note.c_str());};
            start();for(auto& c:children)close_fd(c.pidfd);
            auto take=[&]{auto r=sampler.sample_discovered();require(metric_is_known(r),r.note.c_str());
                require(r.value->local.process.local_pid==selection[0].pid&&r.value->local.process.start_ticks==selection[0].start_ticks,"observed selected association");return *r.value;};
            auto go=[&](unsigned i,char command='G'){transfer(children[i].control[1],&command,1,true);char done=0;transfer(children[i].report[0],&done,1,false);require(done=='D',"observed selected workload");};
            auto empty=take();require(empty.buckets.empty()&&empty.accounting.actors.empty()&&empty.accounting.users.empty(),"observed initial empty registry");
            const auto epoch=empty.accounting.scope.producer_epoch;
            go(0);auto first=take();require(first.buckets.size()==1&&first.buckets[0].socket_namespace_cookie==selection[0].socket_cookie,"observed selected independent cookie oracle");
            require(!first.accounting.actors[0].interval_ns.value,"new observed bucket fabricated interval");
            const std::array<std::uint64_t,4> unit{4096,4096,256,256};
            for(unsigned lane=0;lane<4;++lane)require(first.buckets[0].bytes[lane]==unit[lane]&&!first.accounting.actors[0].bytes_per_second[lane].value,"observed new bucket oracle/rate absent");
            go(0,'T');auto measured=take();const auto ns=measured.read_finished_ns-first.read_finished_ns;require(ns>0,"observed selected interval");
            require(measured.accounting.scope.coverage==NetworkAccountingCoverage::selected_observed_namespace_set&&measured.buckets[0].excluded_positive_receives==24,"observed coverage/exclusion");
            for(unsigned lane=0;lane<4;++lane){const auto rate=unit[lane]*2*1'000'000'000/ns;
                require(measured.buckets[0].bytes[lane]==unit[lane]*3&&measured.accounting.actors[0].bytes_per_second[lane].value==rate&&
                    measured.accounting.actors[0].bytes_per_second[lane].support==Support::estimated,"observed selected four-lane rate oracle");
                require(measured.accounting.users[0].bytes_per_second[lane].value==rate&&measured.accounting.users[0].known[lane]==1,"observed partial UID rate oracle");}
            go(1);for(int family:{AF_INET,AF_INET6})for(bool msg:{false,true})traffic(family,msg);
            auto idle=take();for(const auto& rate:idle.accounting.actors[0].bytes_per_second)require(rate.value==0,"observed unrelated process counted");
            const auto copy=::fork();require(copy>=0,"observed selected fork setup");
            if(copy==0){const bool rejected=!sampler.sample_discovered().value;sampler.stop();::_exit(rejected?0:1);}
            int copied=0;require(::waitpid(copy,&copied,0)==copy&&WIFEXITED(copied)&&WEXITSTATUS(copied)==0,"observed selected fork guard");
            require(take().accounting.actors[0].bytes_per_second[0].value==0,"observed parent state changed after fork");
            sampler.discard_window();auto warm=take();require(!warm.accounting.actors[0].interval_ns.value,"observed discarded baseline retained");
            sampler.stop();require(descriptors()==expected_after_stop,"observed selected capture/owned pidfd leak");
            children[0].pidfd=static_cast<int>(::syscall(SYS_pidfd_open,children[0].pid,0));require(children[0].pidfd>=0,"observed selected restart pidfd");
            start();close_fd(children[0].pidfd);empty=take();require(empty.buckets.empty()&&empty.accounting.scope.producer_epoch!=epoch,"observed selected restart registry/epoch");
            go(0);first=take();require(!first.accounting.actors[0].interval_ns.value,"observed restarted new bucket rate");
            for(auto& child:children){char command='X';transfer(child.control[1],&command,1,true);int status=0;const auto reaped=::waitpid(child.pid,&status,0);if(reaped==child.pid)child.pid=-1;
                require(reaped>0&&WIFEXITED(status)&&WEXITSTATUS(status)==0,"observed selected child exit");}
            require(!sampler.sample_discovered().value&&!sampler.sample_discovered().value,"observed exited pidfd accepted");
            sampler.stop();require(descriptors()==expected_after_stop,"observed selected exit FD leak");
            std::cout<<"PASS selected observed-cookie windows: owned pidfd/caller handles closed; no cookie argument, empty then first bucket rate absent; TCP12288/12288 UDP768/768 peek24, two concurrent threads; four rate/partial UID oracles; other child/parent excluded; fork rejected/parent intact; discard resets rates; restart empty/new epoch; exited child absent twice; no FD leak. ONE real netns, cookie independently initialized by SO_NETNS_COOKIE oracle; no public/multiactor/global claim\n";
        }else if(discovery_mode){
            const auto joined=read_private_pidfd_network_join(true,selection[0].pidfd,selection[0].pid,selection[0].start_ticks);
            require(joined.value.has_value(),joined.note.c_str());const auto expected_after_stop=descriptors()-2;
            LinuxPrivateNetworkSession capture;const auto started=capture.start_discovering_selected(true,joined.value->kernel);
            require(started.value.has_value(),started.note.c_str());for(auto& c:children)close_fd(c.pidfd);
            auto take=[&]{auto report=capture.read_discovered();require(metric_is_known(report),report.note.c_str());return *report.value;};
            const auto empty=take();require(empty.buckets.empty()&&empty.completed_updates==0,"discovery not empty before traffic");
            auto go=[&](unsigned i,char command='G'){transfer(children[i].control[1],&command,1,true);char done=0;transfer(children[i].report[0],&done,1,false);require(done=='D',"discovery private workload");};
            auto check=[&](const PrivateDiscoveredNetworkSessionRead& report,unsigned units){
                require(report.buckets.size()==1&&report.buckets[0].socket_namespace_cookie==selection[0].socket_cookie,"discovered cookie differs from independent SO_NETNS_COOKIE oracle");
                require((report.buckets[0].bytes==std::array<std::uint64_t,4>{4096ULL*units,4096ULL*units,256ULL*units,256ULL*units}),"discovered four-lane payload oracle");
                require(report.buckets[0].excluded_positive_receives==8*units&&report.completed_updates>0,"discovery exclusion/sequence oracle");};
            go(0);auto measured=take();check(measured,1);
            go(0,'T');measured=take();check(measured,3); // Same TGID, concurrent socket writers.
            go(1);for(int family:{AF_INET,AF_INET6})for(bool msg:{false,true})traffic(family,msg);
            auto idle=take();check(idle,3);require(idle.completed_updates==measured.completed_updates,"discovery unrelated process counted");
            capture.detach();go(0);auto frozen=take();check(frozen,3);require(frozen.state==PrivateNetworkSessionState::detached&&frozen.completed_updates==idle.completed_updates,"discovery detach changed map");
            capture.stop();require(!capture.read_discovered().value&&descriptors()==expected_after_stop,"discovery capture FD leak");
            require(capture.start_discovering_selected(true,joined.value->kernel).value.has_value(),"discovery restart failed");
            require(take().buckets.empty(),"discovery restart kept registry");go(0,'T');check(take(),2);capture.stop();
            require(descriptors()==expected_after_stop,"discovery restart FD leak");
            for(auto& child:children){char command='X';transfer(child.control[1],&command,1,true);int status=0;const auto reaped=::waitpid(child.pid,&status,0);if(reaped==child.pid)child.pid=-1;
                require(reaped>0&&WIFEXITED(status)&&WEXITSTATUS(status)==0,"discovery private child exit");}
            std::cout<<"PASS private observed-cookie capture: no cookie supplied to producer; empty before activity, one real cookie matches independent SO_NETNS_COOKIE oracle; TCP12288/12288 UDP768/768 peek24, concurrent same-actor threads; other child/parent excluded; detach frozen; restart empty then TCP8192/8192 UDP512/512 peek16; no FD leak. Only ONE real namespace, no public join/rates/global discovery claim\n";
        }else if(bucket_mode){
            auto exercise=[&](auto& sampler){
            const auto& selected=selection[0];const auto other_cookie=selected.socket_cookie^0x8000000000000000ULL;
            require(other_cookie&&other_cookie!=selected.socket_cookie,"bucket declared other cookie");
            std::array<LinuxProcessNetworkSelection,2> requested{{
                {selected.pidfd,selected.pid,selected.start_ticks,selected.socket_cookie},
                {selected.pidfd,selected.pid,selected.start_ticks,other_cookie}}};
            const auto expected_after_stop=descriptors()-2;
            auto duplicate=requested;duplicate[1]=duplicate[0];require(!sampler.start(true,duplicate).value,"bucket duplicate accepted");
            auto epoch=sampler.start(true,requested);require(epoch.value.has_value(),epoch.note.c_str());
            for(auto& c:children)close_fd(c.pidfd);
            auto take=[&](std::size_t count){auto report=sampler.sample();require(metric_is_known(report),report.note.c_str());
                require(report.value->coverage==LinuxProcessNetworkCoverage::declared_tcp_udp_socket_namespace_buckets&&report.value->processes.size()==count&&
                    report.value->selection_epoch==epoch.value&&report.value->users.size()==1&&report.value->users[0].observed_buckets==count,"bucket report coverage/count/epoch");
                for(const auto& row:report.value->processes)require(row.process.pid==selected.pid,"bucket process alias");return *report.value;};
            auto by_cookie=[&](const LinuxProcessNetworkSnapshot& frame,std::uint64_t cookie)->const LinuxProcessNetworkSample&{
                const auto at=std::find_if(frame.processes.begin(),frame.processes.end(),[&](const auto& row){return row.kernel.socket_network_namespace_cookie==cookie;});
                require(at!=frame.processes.end(),"bucket cookie row absent");return *at;};
            auto warm=take(2);require(!warm.previous_counter_sweep,"bucket first bracket");
            for(const auto& row:warm.processes)require(!row.nominal_interval_ns.value,"bucket first fabricated interval");
            auto go=[&](unsigned i){char command='G';transfer(children[i].control[1],&command,1,true);char done=0;transfer(children[i].report[0],&done,1,false);require(done=='D',"bucket private workload");};
            const std::array<std::uint64_t,4> unit{4096,4096,256,256};
            auto check=[&](const auto& before,const auto& after){const auto ns=after.counter_sweep_finished_ns-before.counter_sweep_finished_ns;require(ns>0,"bucket interval");
                const auto& real=by_cookie(after,selected.socket_cookie);const auto& foreign=by_cookie(after,other_cookie);
                for(unsigned lane=0;lane<4;++lane){const auto rate=unit[lane]*1'000'000'000/ns;
                    require((*real.payload_bytes_since_start.value)[lane]==unit[lane]&&real.payload_bytes_per_second[lane].value==rate,"bucket positive payload/rate oracle");
                    require((*foreign.payload_bytes_since_start.value)[lane]==0&&foreign.payload_bytes_per_second[lane].value==0,"bucket double-counted foreign socket namespace");
                    require(after.users[0].partial_payload_bytes_per_second[lane].value==rate&&after.users[0].known[lane]==2,"bucket partial UID duplicated traffic");}
            };
            go(0);auto measured=take(2);check(warm,measured);
            go(1);for(int family:{AF_INET,AF_INET6})for(bool msg:{false,true})traffic(family,msg);
            auto idle=take(2);for(const auto& row:idle.processes)for(const auto& rate:row.payload_bytes_per_second)require(rate.value==0,"bucket unrelated actor counted");
            const auto copy=::fork();require(copy>=0,"bucket fork setup");if(copy==0){const bool rejected=!sampler.sample().value;sampler.stop();::_exit(rejected?0:1);}
            int copied=0;require(::waitpid(copy,&copied,0)==copy&&WIFEXITED(copied)&&WEXITSTATUS(copied)==0,"bucket fork guard");
            const auto prior_epoch=epoch.value;epoch=sampler.update_selection(true,std::span(requested).last(1));require(epoch.value&&epoch.value!=prior_epoch,"bucket removal epoch");
            warm=take(1);require(!warm.previous_counter_sweep,"bucket removal baseline");go(0);idle=take(1);
            for(const auto& rate:idle.processes[0].payload_bytes_per_second)require(rate.value==0,"undeclared nonzero cookie invalidated bucket or counted");
            children[0].pidfd=static_cast<int>(::syscall(SYS_pidfd_open,children[0].pid,0));require(children[0].pidfd>=0,"bucket readd pidfd");requested[0].pidfd=children[0].pidfd;
            epoch=sampler.update_selection(true,requested);require(epoch.value.has_value(),epoch.note.c_str());close_fd(children[0].pidfd);
            warm=take(2);require(!warm.previous_counter_sweep&&by_cookie(warm,selected.socket_cookie).payload_bytes_since_start.value==std::array<std::uint64_t,4>{},"bucket readd counted gap traffic");
            go(0);measured=take(2);check(warm,measured);sampler.stop();require(descriptors()==expected_after_stop,"bucket capture/pidfd FD leak");
            if constexpr(requires {sampler.set_included(false);}){
                children[0].pidfd=static_cast<int>(::syscall(SYS_pidfd_open,children[0].pid,0));require(children[0].pidfd>=0,"bucket Monitor setup pidfd");
                requested[0].pidfd=requested[1].pidfd=children[0].pidfd;
                require(!sampler.monitor.start_linux_process_network(true,requested).value,"Monitor default silently retained bucket mode");
                const auto rearmed=sampler.start(true,requested);require(rearmed.value.has_value(),rearmed.note.c_str());close_fd(children[0].pidfd);
                sampler.set_included(false);require(!sampler.sample().value&&descriptors()==expected_after_stop,"Monitor bucket opt-out leaked capture");
                sampler.set_included(true);require(!sampler.sample().value,"Monitor flag rearmed bucket capture");
                std::cout<<"PASS Monitor bucket integration: explicit mode, default setup remains strict after bucket stop; opt-out closes FDs, flag alone does not rearm\n";
            }
            for(auto& child:children){char command='X';transfer(child.control[1],&command,1,true);int status=0;const auto reaped=::waitpid(child.pid,&status,0);if(reaped==child.pid)child.pid=-1;
                require(reaped>0&&WIFEXITED(status)&&WEXITSTATUS(status)==0,"bucket child exit");}
            std::cout<<"PASS explicit namespace buckets: ONE selected private child/two declared cookies; real cookie four payload/rate oracles; other declared cookie zero/no duplicate UID sum; undeclared traffic excluded; update removal/readd epochs/gap clean; unrelated child/parent excluded; fork rejected; no FD leak. Only ONE real socket namespace exercised; second cookie is deliberately unobserved, NOT namespace discovery\n";
            };
            if(monitor_mode){MonitorNetworkAdapter sampler(LinuxProcessNetworkSelectionMode::declared_namespace_buckets);exercise(sampler);}
            else{LinuxProcessNetworkSampler sampler(LinuxProcessNetworkSelectionMode::declared_namespace_buckets);exercise(sampler);}
        }else if(worker_mode){
            SampleOptions options;options.include_processes=options.include_disks=options.include_networks=options.include_gpus=false;
            options.include_linux_process_network=true;
            Sampler background(std::chrono::milliseconds(200),64,options);
            std::array<LinuxProcessNetworkSelection,2> requested;
            for(unsigned i=0;i<2;++i)requested[i]={selection[i].pidfd,selection[i].pid,selection[i].start_ticks,selection[i].socket_cookie};
            if(bucket_worker){requested[1]=requested[0];requested[1].socket_network_namespace_cookie^=0x8000000000000000ULL;}
            const auto expected_after_stop=descriptors()-2;
            const auto started=bucket_worker?background.start_linux_process_network(true,requested,LinuxProcessNetworkSelectionMode::declared_namespace_buckets):background.start_linux_process_network(true,requested);
            require(started.value.has_value(),started.note.c_str());
            for(auto& c:children)close_fd(c.pidfd);
            background.start();
            auto wait_frame=[&](std::uint64_t epoch,auto accept){
                const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(12);
                while(std::chrono::steady_clock::now()<deadline){
                    for(const auto& frame:background.timed_history()){const auto& report=frame.snapshot.linux_process_network;
                        if(metric_is_known(report)&&report.value->selection_epoch==epoch&&accept(*report.value))return *report.value;}
                    require(background.running(),background.last_error().c_str());std::this_thread::sleep_for(std::chrono::milliseconds(20));
                }
                throw std::runtime_error("worker epoch/report timeout");
            };
            const auto any=[](const auto&){return true;};
            auto baseline_frame=[&](std::uint64_t epoch,std::size_t count){auto frame=wait_frame(epoch,any);
                require(frame.coverage==(bucket_worker?LinuxProcessNetworkCoverage::declared_tcp_udp_socket_namespace_buckets:LinuxProcessNetworkCoverage::selected_tcp_udp_socket_operations),"worker coverage mode");
                require(frame.processes.size()==count&&!frame.previous_counter_sweep,"worker first epoch shape/bracket");
                for(const auto& p:frame.processes){require(!p.nominal_interval_ns.value,"worker first epoch interval");
                    for(const auto& rate:p.payload_bytes_per_second)require(!rate.value,"worker first epoch fabricated rate");}
                return frame;};
            auto go=[&](unsigned i){char command='G';transfer(children[i].control[1],&command,1,true);char done=0;transfer(children[i].report[0],&done,1,false);require(done=='D',"worker private workload");};
            auto find=[&](const LinuxProcessNetworkSnapshot& frame,unsigned i)->const LinuxProcessNetworkSample&{
                const auto p=std::find_if(frame.processes.begin(),frame.processes.end(),[&](const auto& row){return row.process.pid==requested[i].pid&&row.kernel.socket_network_namespace_cookie==requested[i].socket_network_namespace_cookie;});
                require(p!=frame.processes.end(),"worker keyed process absent");return *p;};
            const std::array<std::uint64_t,4> unit{4096,4096,256,256};
            auto totals=[&](std::uint64_t epoch,unsigned a,unsigned b){return wait_frame(epoch,[&](const auto& frame){
                if(frame.processes.size()!=2)return false;
                for(unsigned lane=0;lane<4;++lane)if(!find(frame,0).payload_bytes_since_start.value||!find(frame,1).payload_bytes_since_start.value||
                    (*find(frame,0).payload_bytes_since_start.value)[lane]!=unit[lane]*a||(*find(frame,1).payload_bytes_since_start.value)[lane]!=unit[lane]*b)return false;
                return true;});};
            auto check_oracles=[&](std::uint64_t epoch){std::optional<LinuxProcessNetworkSnapshot> prior;unsigned intervals=0;
                for(const auto& timed:background.timed_history()){const auto& report=timed.snapshot.linux_process_network;
                    if(!metric_is_known(report)||report.value->selection_epoch!=epoch)continue;const auto& frame=*report.value;
                    if(prior){require(frame.processes.size()==prior->processes.size()&&frame.users.size()==1,"worker oracle shape");
                        const auto ns=frame.counter_sweep_finished_ns-prior->counter_sweep_finished_ns;require(ns>0,"worker oracle clock");
                        std::array<std::uint64_t,4> sum{};
                        for(const auto& row:frame.processes){const auto before=std::find_if(prior->processes.begin(),prior->processes.end(),[&](const auto& p){return p.kernel==row.kernel;});
                            require(before!=prior->processes.end()&&row.nominal_interval_ns.value==ns,"worker interval/identity");
                            for(unsigned lane=0;lane<4;++lane){const auto bytes=(*row.payload_bytes_since_start.value)[lane]-(*before->payload_bytes_since_start.value)[lane],rate=bytes*1'000'000'000/ns;
                                require(row.payload_bytes_per_second[lane].value==rate&&row.payload_bytes_per_second[lane].support==Support::estimated,"worker four-lane rate oracle");sum[lane]+=rate;}}
                        for(unsigned lane=0;lane<4;++lane)require(frame.users[0].partial_payload_bytes_per_second[lane].value==sum[lane]&&frame.users[0].known[lane]==frame.processes.size(),"worker partial UID rate oracle");
                        ++intervals;
                    }prior=frame;
                }require(intervals>0,"worker no measured intervals");};
            (void)baseline_frame(*started.value,2);go(0);go(1);(void)totals(*started.value,1,bucket_worker?0:2);check_oracles(*started.value);
            require(!background.start_linux_process_network(true,requested,LinuxProcessNetworkSelectionMode::strict_single_namespace).value,"worker allowed changing mode through running setup");
            require(background.update_linux_process_network_selection(false,requested).support==Support::permission_required,"worker consent bypass");
            const auto removed=background.update_linux_process_network_selection(true,std::span(requested).last(1));
            if(!removed.value)std::cerr<<"worker remove diagnostic: "<<removed.note<<'\n';
            require(removed.value&&removed.value!=started.value&&background.running(),"worker remove failed/stopped");
            auto single=baseline_frame(*removed.value,1);for(unsigned lane=0;lane<4;++lane)
                require((*find(single,1).payload_bytes_since_start.value)[lane]==unit[lane]*(bucket_worker?0:2),"worker retained capture lost counter origin");
            go(0); // Traffic while A is absent must never be replayed after re-addition.
            children[0].pidfd=static_cast<int>(::syscall(SYS_pidfd_open,children[0].pid,0));require(children[0].pidfd>=0,"worker addition pidfd");requested[0].pidfd=children[0].pidfd;
            Metric<std::uint64_t> added;
            std::jthread updater([&]{added=background.update_linux_process_network_selection(true,requested);});
            // Read history while the update owns the Monitor, not the history mutex.
            (void)background.latest();(void)background.timed_history();updater.join();close_fd(children[0].pidfd);
            require(added.value&&added.value!=removed.value&&background.running(),added.note.c_str());
            auto warm=baseline_frame(*added.value,2);require(find(warm,0).payload_bytes_since_start.value==std::array<std::uint64_t,4>{},"worker new actor counted gap traffic");
            go(0);go(1);(void)totals(*added.value,1,bucket_worker?0:4);check_oracles(*added.value);
            const auto invalid=background.update_linux_process_network_selection(true,{});require(!invalid.value&&background.running(),"worker invalid update accepted/stopped whole worker");
            const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(12);bool absent=false;
            while(std::chrono::steady_clock::now()<deadline){const auto latest=background.latest();if(latest&&!latest->linux_process_network.value){absent=true;break;}
                require(background.running(),background.last_error().c_str());std::this_thread::sleep_for(std::chrono::milliseconds(20));}
            require(absent&&!background.update_linux_process_network_selection(true,requested).value,"worker failed update silently rearmed");
            background.stop();require(background.last_error().empty()&&descriptors()==expected_after_stop,"worker update FD leak/error");
            if(bucket_worker){children[0].pidfd=static_cast<int>(::syscall(SYS_pidfd_open,children[0].pid,0));require(children[0].pidfd>=0,"worker default setup pidfd");
                requested[0].pidfd=requested[1].pidfd=children[0].pidfd;
                require(!background.start_linux_process_network(true,requested).value,"Sampler default setup silently retained bucket mode");close_fd(children[0].pidfd);
                require(descriptors()==expected_after_stop,"Sampler default mode rejection leaked FDs");}
            require(!background.update_linux_process_network_selection(true,requested).value,"stopped worker update rearmed");
            background.start();const auto restart_deadline=std::chrono::steady_clock::now()+std::chrono::seconds(12);
            while(!background.latest()&&background.running()&&std::chrono::steady_clock::now()<restart_deadline)std::this_thread::sleep_for(std::chrono::milliseconds(20));
            const auto restarted=background.latest();background.stop();require(restarted&&!restarted->linux_process_network.value&&descriptors()==expected_after_stop,"worker restart reauthorized/leaked");
            for(auto& child:children){char command='X';transfer(child.control[1],&command,1,true);int status=0;const auto reaped=::waitpid(child.pid,&status,0);if(reaped==child.pid)child.pid=-1;
                require(reaped>0&&WIFEXITED(status)&&WEXITSTATUS(status)==0,"worker private child exit");}
            std::cout<<"PASS background dynamic selection: selected process/cookie rows AB->B->AB while worker runs; fresh consent; concurrent updater/history reads; four bucket/partial-UID payload rate oracles; first epoch rates absent; retained counters preserved/new bucket excludes gap; failed update closes capture but worker continues; stopped update/restart never rearm; no FD leak\n";
            if(bucket_worker)std::cout<<"PASS Sampler bucket integration: ONE selected private actor/two declared cookies; other child excluded; one real namespace/second cookie deliberately unobserved, no discovery or two-real-netns claim; default setup strict after stop; running mode switch rejected\n";
        }else if(public_mode){
            std::array<LinuxProcessNetworkSelection,2> requested;
            for(unsigned i=0;i<2;++i)requested[i]={selection[i].pidfd,selection[i].pid,selection[i].start_ticks,selection[i].socket_cookie};
            auto exercise=[&](auto& sampler){
            auto duplicate=requested;duplicate[1]=duplicate[0];require(!sampler.start(true,duplicate).value,"public duplicate accepted");
            const auto expected_after_stop=descriptors()-2;const auto started=sampler.start(true,requested);require(started.value.has_value(),started.note.c_str());
            std::uint64_t expected_epoch=*started.value;
            std::size_t expected_count=2;
            for(auto& c:children)close_fd(c.pidfd);
            auto take=[&](auto& source){auto r=source.sample();require(metric_is_known(r)&&r.support==Support::estimated,r.note.c_str());
                require(r.value->processes.size()==expected_count&&r.value->users.size()==1&&r.value->selection_epoch==expected_epoch&&
                    r.value->coverage==LinuxProcessNetworkCoverage::selected_tcp_udp_socket_operations,"public scope/rows");return std::move(*r.value);};
            auto first=take(sampler);require(!first.previous_counter_sweep,"public first bracket");
            for(const auto& p:first.processes)require(p.payload_bytes_since_start.value.has_value()&&!p.nominal_interval_ns.value&&!p.payload_bytes_per_second[0].value,"public first sample availability");
            for(unsigned i=0;i<2;++i){char command='G';transfer(children[i].control[1],&command,1,true);char done=0;transfer(children[i].report[0],&done,1,false);require(done=='D',"public workload completion");}
            auto next=take(sampler);const auto ns=next.counter_sweep_finished_ns-first.counter_sweep_finished_ns;require(ns&&next.previous_counter_sweep.has_value(),"public nominal bracket");
            std::array<std::uint64_t,4> sum{};const std::array<std::uint64_t,4> unit{4096,4096,256,256};
            for(unsigned i=0;i<2;++i){const auto found=std::find_if(next.processes.begin(),next.processes.end(),[&](const auto& p){return p.process.pid==requested[i].pid;});
                require(found!=next.processes.end()&&found->process.start_ticks==requested[i].process_start_ticks&&found->nominal_interval_ns.value==ns&&
                    found->kernel.socket_network_namespace_cookie==requested[i].socket_network_namespace_cookie&&found->kernel.initial_real_uid==next.users[0].initial_real_uid,"public keyed identity");
                for(unsigned lane=0;lane<4;++lane){const auto bytes=unit[lane]*(i+1),rate=bytes*1'000'000'000/ns;
                    require((*found->payload_bytes_since_start.value)[lane]==bytes&&found->payload_bytes_per_second[lane].value==rate&&
                        found->payload_bytes_per_second[lane].support==Support::estimated,"public actor oracle");sum[lane]+=rate;}}
            for(unsigned lane=0;lane<4;++lane)require(next.users[0].partial_payload_bytes_per_second[lane].value==sum[lane]&&
                next.users[0].known[lane]==2&&next.users[0].missing[lane]==0&&!next.users[0].overflow[lane],"public partial UID oracle");
            auto moved=std::move(sampler);require(!sampler.sample().value,"moved-from public sampler active");
            for(int family:{AF_INET,AF_INET6})for(bool msg:{false,true})traffic(family,msg);
            auto idle=take(moved);for(const auto& p:idle.processes)for(const auto& rate:p.payload_bytes_per_second)require(rate.value==0,"public idle/parent exclusion");
            const auto copy=::fork();require(copy>=0,"public fork setup");if(copy==0){const bool rejected=update_mode?!moved.update_selection(true,requested).value:!moved.sample().value;moved.stop();::_exit(rejected?0:1);}
            int copied_status=0;require(::waitpid(copy,&copied_status,0)==copy&&WIFEXITED(copied_status)&&WEXITSTATUS(copied_status)==0,"public fork guard");
            idle=take(moved);require(idle.users[0].partial_payload_bytes_per_second[0].value==0,"public parent changed after fork");
            if(update_mode){
                const auto full_fds=descriptors();
                const auto denied=moved.update_selection(false,requested);require(!denied.value&&denied.support==Support::permission_required,"update without new consent");
                require(descriptors()==full_fds,"denied update changed resources");
                const auto removed=moved.update_selection(true,std::span(requested).last(1));
                require(removed.value&&*removed.value!=expected_epoch,"remove epoch");expected_epoch=*removed.value;expected_count=1;
                require(descriptors()<full_fds,"removed capture references retained");
                auto before=take(moved);require(!before.previous_counter_sweep&&!before.processes[0].nominal_interval_ns.value,"remove reused rates");
                const auto original_b=std::find_if(next.processes.begin(),next.processes.end(),[&](const auto& p){return p.process.pid==requested[1].pid;});
                require(original_b!=next.processes.end()&&before.processes[0].process.pid==requested[1].pid&&before.processes[0].payload_bytes_since_start.value==original_b->payload_bytes_since_start.value,"retained capture lost origin");
                auto go=[&](unsigned i){char command='G';transfer(children[i].control[1],&command,1,true);char done=0;transfer(children[i].report[0],&done,1,false);require(done=='D',"update private workload");};
                go(0);go(1);auto after=take(moved);
                const auto elapsed=after.counter_sweep_finished_ns-before.counter_sweep_finished_ns;
                for(unsigned lane=0;lane<4;++lane)require((*after.processes[0].payload_bytes_since_start.value)[lane]-(*before.processes[0].payload_bytes_since_start.value)[lane]==unit[lane]*2&&
                    after.processes[0].payload_bytes_per_second[lane].value==unit[lane]*2*1'000'000'000/elapsed,"removed actor traffic crossed into retained actor");
                children[0].pidfd=static_cast<int>(::syscall(SYS_pidfd_open,children[0].pid,0));require(children[0].pidfd>=0,"update addition pidfd");requested[0].pidfd=children[0].pidfd;
                const auto added=moved.update_selection(true,requested);require(added.value&&*added.value!=expected_epoch,"add epoch");expected_epoch=*added.value;expected_count=2;close_fd(children[0].pidfd);
                before=take(moved);require(!before.previous_counter_sweep,"add retained bracket");
                for(const auto& p:before.processes){require(!p.nominal_interval_ns.value,"add bridged interval");
                    if(p.process.pid==requested[0].pid)require(p.payload_bytes_since_start.value==std::array<std::uint64_t,4>{},"added actor counted pre-capture traffic");}
                go(0);go(1);after=take(moved);const auto delta_ns=after.counter_sweep_finished_ns-before.counter_sweep_finished_ns;
                std::array<std::uint64_t,4> total{};
                for(unsigned i=0;i<2;++i){const auto prior=std::find_if(before.processes.begin(),before.processes.end(),[&](const auto& p){return p.process.pid==requested[i].pid;});
                    const auto current=std::find_if(after.processes.begin(),after.processes.end(),[&](const auto& p){return p.process.pid==requested[i].pid;});
                    require(prior!=before.processes.end()&&current!=after.processes.end(),"update keyed actor absent");
                    for(unsigned lane=0;lane<4;++lane){const auto bytes=unit[lane]*(i+1),rate=bytes*1'000'000'000/delta_ns;
                        require((*current->payload_bytes_since_start.value)[lane]-(*prior->payload_bytes_since_start.value)[lane]==bytes&&current->payload_bytes_per_second[lane].value==rate,"updated four lane oracle");total[lane]+=rate;}}
                for(unsigned lane=0;lane<4;++lane)require(after.users[0].partial_payload_bytes_per_second[lane].value==total[lane],"updated partial UID oracle");
                auto reversed=requested;std::swap(reversed[0],reversed[1]);const auto held=descriptors();
                const auto reordered=moved.update_selection(true,reversed);require(reordered.value&&*reordered.value!=expected_epoch,"reorder epoch");expected_epoch=*reordered.value;
                require(descriptors()==held,"reorder recreated capture");before=take(moved);require(!before.previous_counter_sweep,"reorder retained baseline");
                std::cout<<"PASS dynamic explicit selection: AB->B->AB/reorder; retired FDs closed; retained counters preserved; new actor excludes gap traffic; fresh epoch/all baselines reset; four actor/partial-UID payload-rate oracles; denied update unchanged; fork update rejected\n";
            }
            if constexpr(requires {moved.set_included(false);}){
                if(!update_mode){
                moved.set_included(false);require(!moved.sample().value,"Monitor opt-out retained report");
                require(descriptors()==expected_after_stop,"Monitor opt-out retained capture FDs");
                moved.set_included(true);require(!moved.sample().value,"Monitor flag reauthorized capture");
                auto reopen_inputs=[&]{for(unsigned i=0;i<2;++i){children[i].pidfd=static_cast<int>(::syscall(SYS_pidfd_open,children[i].pid,0));require(children[i].pidfd>=0,"Monitor rearm pidfd");requested[i].pidfd=children[i].pidfd;}};
                auto close_inputs=[&]{for(auto& c:children)close_fd(c.pidfd);};
                {
                    auto options=moved.options;Sampler background(std::chrono::milliseconds(100),4,options);
                    reopen_inputs();require(background.start_linux_process_network(true,requested).value.has_value(),"Sampler explicit capture setup");close_inputs();
                    background.start();require(!background.start_linux_process_network(true,requested).value,"Sampler allowed running reconfiguration");
                    auto wait_report=[&](bool known){const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(12);
                        while(std::chrono::steady_clock::now()<deadline){const auto latest=background.latest();
                            if(latest&&(known?metric_is_known(latest->linux_process_network):!latest->linux_process_network.value))return;
                            require(background.running(),background.last_error().c_str());std::this_thread::sleep_for(std::chrono::milliseconds(20));}
                        throw std::runtime_error("Sampler report timeout");};
                    wait_report(true);background.stop();require(background.last_error().empty()&&descriptors()==expected_after_stop,"Sampler stop/capture cleanup");
                    background.start();wait_report(false);background.stop();require(descriptors()==expected_after_stop,"Sampler restart silently rearmed capture");
                }
                reopen_inputs();const auto restarted=moved.start(true,requested);require(restarted.value&&*restarted.value!=expected_epoch,"Monitor explicit rearm epoch");expected_epoch=*restarted.value;close_inputs();
                const auto warm=take(moved);for(const auto& p:warm.processes)require(!p.nominal_interval_ns.value,"Monitor rearm reused baseline");
                std::cout<<"PASS Monitor/Sampler integration: separate report, opt-out closes FDs, flag alone cannot rearm, background capture explicit setup, running reconfiguration rejected, worker stop closes capture, restart remains unarmed, explicit rearm epoch/baseline clean\n";
                }
            }
            auto finish=[&](unsigned i){char command='X';transfer(children[i].control[1],&command,1,true);int result=0;
                const auto reaped=::waitpid(children[i].pid,&result,0);if(reaped==children[i].pid)children[i].pid=-1;
                require(reaped>0&&WIFEXITED(result)&&WEXITSTATUS(result)==0,"public child exit");};
            finish(1);require(!moved.sample().value&&!moved.sample().value,"public partial group after exit");
            if(update_mode){const auto recovered=moved.update_selection(true,std::span(requested).first(1));
                require(recovered.value&&*recovered.value!=expected_epoch,"remove exited actor failed");expected_epoch=*recovered.value;expected_count=1;
                const auto recovered_frame=take(moved);require(!recovered_frame.previous_counter_sweep&&!recovered_frame.processes[0].nominal_interval_ns.value,"exit removal bridged baseline");}
            moved.stop();require(descriptors()==expected_after_stop,"public FD leak");finish(0);
            std::cout<<"PASS public LinuxProcessNetworkSampler: two selected children; TCP/UDP actor and partial initial-UID four-lane oracles; nominal interval_ns="<<ns
                <<"; baseline absence; declared coverage/keyed local-kernel identities; input FDs closed; live move; parent excluded/idle0; fork/exit rejected; no FD leak\n";
            };
            if(monitor_mode){MonitorNetworkAdapter sampler;exercise(sampler);}else{LinuxProcessNetworkSampler sampler;exercise(sampler);}
        }else{
        LinuxMultiNetworkSampler group;
        auto duplicate=selection;duplicate[1]=duplicate[0];require(!group.start(true,duplicate).value,"multi duplicate process accepted");
        const auto expected_after_stop=descriptors()-2;
        auto started=group.start(true,selection);require(started.value.has_value(),started.note.c_str());
        for(auto& c:children)close_fd(c.pidfd); // Group owns separate references.
        std::optional<std::array<std::uint64_t,2>> prior_common;
        auto take=[&]()->PrivateMultiNetworkWindow{
            if(!common_mode){auto r=group.sample();require(r.value.has_value(),r.note.c_str());require(r.value->actors.size()==2,"multi row count");return std::move(*r.value);}
            auto result=group.sample_common();require(result.value.has_value(),result.note.c_str());auto& frame=*result.value;
            require(frame.accounting.actors.size()==2&&frame.associations.size()==2,"common row count");
            require(frame.previous_read_bracket==prior_common&&frame.read_finished_ns>=frame.read_started_ns&&
                frame.read_finished_ns-frame.read_started_ns<=LinuxPrivateNetworkSampler::max_read_span_ns,"common sweep brackets");
            require(frame.accounting.scope.coverage==NetworkAccountingCoverage::selected_actor_set&&frame.accounting.users.size()==1,"common selected coverage/same UID grouping");
            const auto& user=frame.accounting.users[0];
            for(unsigned lane=0;lane<4;++lane){std::uint64_t sum=0;
                for(const auto& actor:frame.accounting.actors){require(actor.actor.real_uid==user.real_uid,"common kernel UID");
                    if(prior_common){require(actor.interval_ns.value==frame.read_finished_ns-(*prior_common)[1]&&actor.bytes_per_second[lane].value.has_value(),"common shared interval");sum+=*actor.bytes_per_second[lane].value;}
                    else require(!actor.interval_ns.value&&!actor.bytes_per_second[lane].value,"common first fabricated rate");}
                if(prior_common)require(user.bytes_per_second[lane].value==sum&&user.bytes_per_second[lane].support==Support::estimated&&user.known[lane]==2&&user.missing[lane]==0,"common partial UID sum oracle");
                else require(!user.bytes_per_second[lane].value&&user.known[lane]==0&&user.missing[lane]==2,"common baseline UID availability");
            }
            prior_common=std::array<std::uint64_t,2>{frame.read_started_ns,frame.read_finished_ns};
            // Test-only per-actor views reuse byte/rate oracles below. The actual
            // provider returns the full common accounting plus keyed associations.
            PrivateMultiNetworkWindow view;view.selection_epoch=frame.accounting.scope.producer_epoch;
            for(const auto& association:frame.associations){const auto found=std::find_if(frame.accounting.actors.begin(),frame.accounting.actors.end(),
                    [&](const auto& row){return row.actor==association.actor;});require(found!=frame.accounting.actors.end(),"common keyed association");
                auto accounting=frame.accounting;accounting.actors={*found};
                view.actors.push_back({{std::move(accounting),association.quality,frame.read_started_ns,frame.read_finished_ns},association.local});}
            return view;
        };
        auto first=take();for(const auto& a:first.actors)require(!a.window.accounting.actors[0].interval_ns.value,"multi first fabricated rate");
        require(first.actors[0].window.accounting.actors[0].actor.tgid!=first.actors[1].window.accounting.actors[0].actor.tgid,"multi kernel actors aliased");
        auto go=[&](unsigned i){char command='G';transfer(children[i].control[1],&command,1,true);char done=0;transfer(children[i].report[0],&done,1,false);require(done=='D',"multi child completion");};
        const std::array<std::uint64_t,4> unit{4096,4096,256,256};
        auto check=[&](const PrivateMultiNetworkWindow& before,const PrivateMultiNetworkWindow& now,unsigned active){
            for(unsigned i=0;i<2;++i){const auto& a=now.actors[i];const auto& previous=before.actors[i];const auto& row=a.window.accounting.actors[0];
                require(a.local.process.local_pid==selection[i].pid&&a.local.process.start_ticks==selection[i].start_ticks,"multi local identity");
                const auto ns=a.window.read_finished_ns-previous.window.read_finished_ns;require(ns&&row.interval_ns.value==ns,"multi per actor interval");
                for(unsigned lane=0;lane<4;++lane){const auto delta=i==active?unit[lane]*(i+1):0;
                    require((*row.bytes.value)[lane]-(*previous.window.accounting.actors[0].bytes.value)[lane]==delta,"multi cross-attributed bytes");
                    require(row.bytes_per_second[lane].value==delta*1'000'000'000/ns,"multi independent rate oracle");}
            }
        };
        go(0);auto second=take();check(first,second,0);
        go(1);auto third=take();check(second,third,1);
        for(int family:{AF_INET,AF_INET6})for(bool msg:{false,true})traffic(family,msg);
        auto idle=take();check(third,idle,2); // Neither actor: unrelated parent traffic excluded.
        const auto copy=::fork();require(copy>=0,"multi collector fork");
        if(copy==0){const bool rejected=common_mode?!group.sample_common().value:!group.sample().value;group.stop();::_exit(rejected?0:1);}
        int status=0;require(::waitpid(copy,&status,0)==copy&&WIFEXITED(status)&&WEXITSTATUS(status)==0,"multi fork guard");
        auto after_copy=take();check(idle,after_copy,2);
        auto finish=[&](unsigned i){char command='X';transfer(children[i].control[1],&command,1,true);int result=0;
            const auto pid=::waitpid(children[i].pid,&result,0);if(pid==children[i].pid)children[i].pid=-1;
            require(pid>0&&WIFEXITED(result)&&WEXITSTATUS(result)==0,"multi child exit");};
        finish(1);if(common_mode)require(!group.sample_common().value&&!group.sample_common().value,"common partial group after target exit");
        else require(!group.sample().value&&!group.sample().value,"multi partial group returned after target exit");
        group.stop();require(descriptors()==expected_after_stop,"multi capture/pidfd FD leak");finish(0);
        std::cout<<"PASS private multi: two distinct selected children; independent four-lane oracles A4096/4096/256/256 B8192/8192/512/512; idle/parent excluded; input FDs closed; duplicate rejected; fork rejected; actor exit rejects whole collection; no FD leak\n";
        if(common_mode)std::cout<<"PASS common nominal window: counter sweep <=1s; previous/current brackets preserved; shared interval and four partial kernel-UID sums match oracles; not atomic or global traffic\n";
        }
    }
    require(descriptors()==baseline,"multi parent FD leak");::alarm(0);return 0;
}
}
int main(int argc,char** argv){
    const bool owned_selection=argc==2&&std::string_view(argv[1])=="--authorized-private-owned-selection";
    const bool observed_worker=argc==2&&std::string_view(argv[1])=="--authorized-private-observed-worker";
    const bool observed_monitor=argc==2&&std::string_view(argv[1])=="--authorized-private-observed-monitor";
    const bool public_observed=owned_selection||observed_monitor||(argc==2&&std::string_view(argv[1])=="--authorized-private-public-observed");
    const bool observed_multi=argc==2&&std::string_view(argv[1])=="--authorized-private-observed-multi";
    const bool discovered_window=argc==2&&std::string_view(argv[1])=="--authorized-private-observed-window";
    const bool discovery_mode=argc==2&&std::string_view(argv[1])=="--authorized-private-observed-cookies";
    const bool bucket_monitor=argc==2&&std::string_view(argv[1])=="--authorized-private-monitor-buckets";
    const bool bucket_worker=argc==2&&std::string_view(argv[1])=="--authorized-private-worker-buckets";
    const bool bucket_mode=bucket_monitor||(argc==2&&std::string_view(argv[1])=="--authorized-private-namespace-buckets");
    const bool worker_mode=bucket_worker||(argc==2&&std::string_view(argv[1])=="--authorized-private-worker-update");
    const bool update_mode=argc==2&&std::string_view(argv[1])=="--authorized-private-update-window";
    const bool monitor_mode=bucket_monitor||update_mode||(argc==2&&std::string_view(argv[1])=="--authorized-private-monitor-window");
    const bool public_mode=monitor_mode||(argc==2&&std::string_view(argv[1])=="--authorized-private-public-window");
    const bool common_mode=public_mode||(argc==2&&std::string_view(argv[1])=="--authorized-private-common-window");
    const bool multi_mode=observed_worker||public_observed||observed_multi||discovered_window||discovery_mode||bucket_mode||worker_mode||common_mode||(argc==2&&std::string_view(argv[1])=="--authorized-private-multi-window");
    const bool selected_window_mode=argc==2&&std::string_view(argv[1])=="--authorized-private-selected-window";
    const bool pidfd_mode=selected_window_mode||(argc==2&&std::string_view(argv[1])=="--authorized-private-pidfd-session");
    const bool private_child_mode=pidfd_mode||(argc==2&&std::string_view(argv[1])=="--authorized-private-child-session");
    const bool joined_mode=argc==2&&std::string_view(argv[1])=="--authorized-self-joined-window";
    const bool window_mode=joined_mode||(argc==2&&std::string_view(argv[1])=="--authorized-self-window");
    const bool namespace_mode=window_mode||(argc==2&&std::string_view(argv[1])=="--authorized-self-namespace-session");
    if(argc!=2||(!multi_mode&&!private_child_mode&&!namespace_mode&&std::string_view(argv[1])!="--authorized-self-session")){
        std::cerr<<"NO capture loaded; requires explicit human authorization and --authorized-self-session\n";return 77;}
    ::alarm(multi_mode?60:selected_window_mode?45:20); // Kernel closes every unpinned FD even on watchdog termination.
    try{
        if(multi_mode)return test_private_multi(common_mode,public_mode,monitor_mode,update_mode,worker_mode,bucket_mode,bucket_worker,discovery_mode,discovered_window,observed_multi,public_observed,observed_monitor,observed_worker,owned_selection);
        if(private_child_mode)return test_private_child(pidfd_mode,selected_window_mode);
        if(window_mode)return test_window(joined_mode);
        const auto baseline=descriptors();LinuxPrivateNetworkSession session;
        const auto cookie=namespace_mode?socket_namespace_cookie():0;
        auto identity=session.start(true,cookie);if(!identity.value){std::cerr<<"UNAVAILABLE: "<<identity.note<<'\n';return 77;}
        auto metadata=session.read();require(metadata.value.has_value(),"metadata read");
        require(metadata.value->expected_socket_network_namespace_cookie==(cookie?std::optional<std::uint64_t>{cookie}:std::nullopt),"namespace metadata");
        auto initial=read(session);require((initial.bytes==std::array<std::uint64_t,4>{}),"fresh counters");
        require(initial.excluded_positive_receives==0&&initial.completed_updates==0,"fresh quality");
        for(int family:{AF_INET,AF_INET6})for(bool message:{false,true})traffic(family,message);
        auto measured=read(session);require((measured.bytes==std::array<std::uint64_t,4>{4096,4096,256,256}),"four lane totals");
        require(measured.excluded_positive_receives==8,"peek exclusion");
        // Inherited descriptors must be closed in the child, not relabeled as child SELF.
        const auto child=::fork();require(child>=0,"fork");if(child==0){session.stop();try{traffic(AF_INET,false);traffic(AF_INET6,true);::_exit(0);}catch(...){::_exit(1);}}
        int status=0;require(::waitpid(child,&status,0)==child&&WIFEXITED(status)&&WEXITSTATUS(status)==0,"child traffic");
        require(same(measured,read(session)),"unrelated TGID counted");
        // Concurrent writers, but serialized session API calls.
        std::exception_ptr first,second;
        std::thread a([&]{try{traffic(AF_INET,false);}catch(...){first=std::current_exception();}});
        std::thread b([&]{try{traffic(AF_INET6,true);}catch(...){second=std::current_exception();}});a.join();b.join();
        if(first)std::rethrow_exception(first);if(second)std::rethrow_exception(second);
        measured=read(session);require((measured.bytes==std::array<std::uint64_t,4>{6144,6144,384,384}),"thread totals");
        require(measured.excluded_positive_receives==12,"thread peek exclusion");
        session.detach();require(session.state()==PrivateNetworkSessionState::detached,"detach state");
        traffic(AF_INET,false);traffic(AF_INET6,true);require(same(measured,read(session)),"detached map changed");
        session.stop();session.stop();require(!session.read().value,"stopped map visible");require(descriptors()==baseline,"FD leak");
        require(session.start(true,cookie).value.has_value(),"restart");require((read(session).bytes==std::array<std::uint64_t,4>{}),"restart stale totals");
        session.stop();require(descriptors()==baseline,"restart FD leak");
        if(namespace_mode){
            const auto wrong=cookie==1?2:cookie^1;
            require(session.start(true,wrong).value.has_value(),"mismatch setup");
            traffic(AF_INET,false);traffic(AF_INET6,true);
            require(!session.read().value&&!session.read().value,"mismatched namespace accepted or not sticky");
            session.stop();require(descriptors()==baseline,"mismatch FD leak");
            std::cout<<"PASS socket netns cookie="<<cookie<<" corroborated by SO_NETNS_COOKIE; wrong cookie rejected (sticky); no setns/unshare\n";
        }
        ::alarm(0);
        std::cout<<"PASS SELF session: TCP6144/6144 UDP384/384 peek12; child excluded; detach frozen; stop/restart clean; no FD leak\n";
    }catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}
}
