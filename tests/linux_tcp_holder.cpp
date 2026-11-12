#include "../src/linux_tcp.hpp"
#include "../src/linux_process_details.hpp"
#include <cassert>
#include <cerrno>
#include <iostream>
#include <limits>
#ifdef __linux__
#include <csignal>
#include <fcntl.h>
#include <linux/tcp.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>
#endif
using namespace pcinsight;using namespace pcinsight::detail;
template<class T> Metric<T> known(T v){return {v,Support::available,"fixture"};}
Metric<LinuxTcpHeldSocketSample> observation(int seconds,std::uint64_t acked,std::uint64_t received){
    LinuxTcpHeldSocketSample s;s.holder={20,100,5};s.ip_version=4;s.state=1;s.socket_inode=1000;
    s.cookie=known<std::uint64_t>(55);s.network_namespace_cookie=known<std::uint64_t>(12);
    s.captured_at=std::chrono::steady_clock::time_point{}+std::chrono::seconds(seconds);
    s.acknowledged_bytes=known(acked);s.received_bytes=known(received);
    return {s,Support::estimated,"fixture pinned reference"};
}
#ifdef __linux__
struct Fd{int n{-1};~Fd(){if(n>=0)close(n);}};
int native(){
    const Fd listener{socket(AF_INET,SOCK_STREAM|SOCK_CLOEXEC,0)},client{socket(AF_INET,SOCK_STREAM|SOCK_CLOEXEC,0)};
    assert(listener.n>=0&&client.n>=0);
    sockaddr_in address{};address.sin_family=AF_INET;address.sin_addr.s_addr=htonl(INADDR_LOOPBACK);
    assert(bind(listener.n,reinterpret_cast<sockaddr*>(&address),sizeof(address))==0&&listen(listener.n,1)==0);
    socklen_t size=sizeof(address);assert(getsockname(listener.n,reinterpret_cast<sockaddr*>(&address),&size)==0);
    assert(connect(client.n,reinterpret_cast<sockaddr*>(&address),sizeof(address))==0);
    const Fd server{accept4(listener.n,nullptr,nullptr,SOCK_CLOEXEC)};assert(server.n>=0);
    timeval timeout{1,0};for(int fd:{client.n,server.n}){
        assert(setsockopt(fd,SOL_SOCKET,SO_RCVTIMEO,&timeout,sizeof(timeout))==0);
        assert(setsockopt(fd,SOL_SOCKET,SO_SNDTIMEO,&timeout,sizeof(timeout))==0);
    }
    const auto identity=read_linux_process_identity("/proc/self");assert(metric_is_known(identity));
    LinuxTcpSocketHolder target{static_cast<std::uint32_t>(getpid()),identity.value->start_ticks,client.n};
    const auto probe=read_linux_tcp_socket_holder(target);
    if(!metric_is_known(probe)){std::cout<<"SKIP native held TCP source: "<<probe.note<<'\n';return 77;}
    assert(probe.value->holder==target&&probe.value->cookie.value);
    LinuxTcpSampler diagnostic;
    const auto inventory=diagnostic.sample();
    if(metric_is_known(inventory)&&metric_is_known(inventory.value->ipv4_dump_completed)){
        bool found=false;for(const auto& row:inventory.value->sockets)if(row.cookie.value==probe.value->cookie.value)found=true;
        assert(found); // The descriptor's cookie identifies the diagnostic socket too.
    }
    LinuxTcpSocketHolderSampler sampler;
    const auto first=sampler.sample(target);assert(metric_is_known(first)&&!first.value->acknowledged_bytes_per_second.value);
    const int flags=fcntl(client.n,F_GETFL);assert(flags>=0);
    const std::string data(8192,'x');std::size_t offset=0;
    while(offset<data.size()){const auto n=send(client.n,data.data()+offset,data.size()-offset,MSG_NOSIGNAL);assert(n>0);offset+=static_cast<std::size_t>(n);}
    auto server_target=target;server_target.descriptor=server.n;
    const auto server_observed=read_linux_tcp_socket_holder(server_target);
    assert(metric_is_known(server_observed)&&server_observed.value->received_bytes.value&&*server_observed.value->received_bytes.value>=data.size());
    std::string buffer(data.size(),' ');offset=0;
    while(offset<buffer.size()){const auto n=recv(server.n,buffer.data()+offset,buffer.size()-offset,0);assert(n>0);offset+=static_cast<std::size_t>(n);}
    assert(buffer==data); // Queries did NOT consume queued data.
    int quick=1;assert(setsockopt(server.n,IPPROTO_TCP,TCP_QUICKACK,&quick,sizeof(quick))==0);
    const auto second=sampler.sample(target);
    assert(metric_is_known(second)&&metric_is_known(second.value->acknowledged_bytes_per_second));
    assert(*second.value->acknowledged_bytes.value>=*first.value->acknowledged_bytes.value+data.size());
    assert(fcntl(client.n,F_GETFL)==flags);
    const Fd alias{fcntl(client.n,F_DUPFD_CLOEXEC,0)};assert(alias.n>=0);
    auto alias_target=target;alias_target.descriptor=alias.n;
    const auto alias_view=read_linux_tcp_socket_holder(alias_target);
    assert(metric_is_known(alias_view)&&alias_view.value->cookie.value==probe.value->cookie.value); // same socket, not independent traffic.
    LinuxTcpSocketHolderSampler alias_sampler;assert(metric_is_known(alias_sampler.sample(alias_target)));
    assert(dup2(server.n,alias.n)==alias.n);
    const auto replacement=alias_sampler.sample(alias_target);
    assert(metric_is_known(replacement)&&replacement.value->cookie.value!=probe.value->cookie.value&&!replacement.value->acknowledged_bytes_per_second.value);
    auto stale=target;++stale.process_start_ticks;assert(!read_linux_tcp_socket_holder(stale).value);
    stale=target;stale.descriptor=std::numeric_limits<int>::max();assert(!read_linux_tcp_socket_holder(stale).value);
    const Fd udp{socket(AF_INET,SOCK_DGRAM|SOCK_CLOEXEC,0)};assert(udp.n>=0);
    stale=target;stale.descriptor=udp.n;assert(!read_linux_tcp_socket_holder(stale).value);
    stale=target;stale.descriptor=listener.n;assert(!read_linux_tcp_socket_holder(stale).value);
    int pipes[2]{};assert(pipe2(pipes,O_CLOEXEC)==0);
    const auto child=fork();assert(child>=0);
    if(child==0){close(pipes[1]);alarm(5);char byte{};while(read(pipes[0],&byte,1)<0&&errno==EINTR){};_exit(0);}
    close(pipes[0]);
    struct Child {pid_t pid;int release;~Child(){close(release);int status{};while(waitpid(pid,&status,0)<0&&errno==EINTR){}}} child_guard{child,pipes[1]};
    const auto child_identity=read_linux_process_identity("/proc/"+std::to_string(child));assert(metric_is_known(child_identity));
    const LinuxTcpSocketHolder child_target{static_cast<std::uint32_t>(child),child_identity.value->start_ticks,client.n};
    const auto foreign=read_linux_tcp_socket_holder(child_target);
    if(metric_is_known(foreign)){
        assert(foreign.value->holder.pid==static_cast<std::uint32_t>(child)&&foreign.value->cookie.value==probe.value->cookie.value);
        std::cout<<"Native private child pidfd_getfd positive; parent/child hold SAME cookie="<<*foreign.value->cookie.value<<"; no exclusive attribution\n";
    }else{
        assert(foreign.support==Support::permission_required||foreign.note.find("errno=38")!=std::string::npos||foreign.note.find("pidfd_open unavailable")!=std::string::npos);
        std::cout<<"Native private child pin unavailable/denied (not a positive child-provider validation): "<<foreign.note<<'\n';
    }
    std::cout<<"Native self holder PID="<<target.pid<<" start_ticks="<<target.process_start_ticks<<" FD="<<target.descriptor<<" cookie="<<*second.value->cookie.value
        <<" ACKed delta="<<*second.value->acknowledged_bytes.value-*first.value->acknowledged_bytes.value<<" received="<<*server_observed.value->received_bytes.value
        <<" bytes; rate="<<*second.value->acknowledged_bytes_per_second.value<<" B/s. Alias/replaced FD/stale token/UDP/listener rejected or reset; data/flags unchanged\n";
    return 0;
}
#endif
int main(int argc,char** argv){
#ifdef __linux__
    if(argc==2&&std::string(argv[1])=="--native")return native();
#else
    (void)argc;(void)argv;
    assert(!read_linux_tcp_socket_holder({20,100,5}).value);
#endif
    assert(!read_linux_tcp_socket_holder({0,100,5}).value);
    assert(!read_linux_tcp_socket_holder({20,0,5}).value);
    assert(!read_linux_tcp_socket_holder({20,100,-1}).value);
    assert(parse_linux_tcp_pidfd_pid("pos:0\nflags:02000002\nPid:\t123\nNSpid:\t123 1\n").value==123);
    for(const auto& bad:{"","Pid: -1\n","Pid:0\n","Pid: +12\n","Pid:12junk\n","Pid:12\nPid:12\n","Pid:2147483648\n","NSpid:12\n"})assert(!parse_linux_tcp_pidfd_pid(bad).value);
    assert(!parse_linux_tcp_pidfd_pid(std::string(65537,'x')).value);
    std::optional<LinuxTcpHeldSocketSample> previous;
    assert(!update_linux_tcp_holder(previous,observation(1,10,20)).value->acknowledged_bytes_per_second.value);
    auto r=update_linux_tcp_holder(previous,observation(3,110,220));
    assert(r.value->acknowledged_bytes_per_second.value==50&&r.value->received_bytes_per_second.value==100);
    assert(update_linux_tcp_holder(previous,observation(4,110,220)).value->received_bytes_per_second.value==0);
    for(unsigned mode=0;mode<10;++mode){
        previous.reset();update_linux_tcp_holder(previous,observation(1,10,20));auto next=observation(2,110,220);auto& s=*next.value;
        if(mode==0)++s.holder.pid;if(mode==1)++s.holder.process_start_ticks;if(mode==2)++s.holder.descriptor;
        if(mode==3)s.cookie=known<std::uint64_t>(56);if(mode==4)++s.socket_inode;
        if(mode==5)s.network_namespace_cookie=known<std::uint64_t>(13);
        if(mode==6)s.network_namespace_cookie.value.reset();if(mode==7)s.acknowledged_bytes=known<std::uint64_t>(9);
        if(mode==8)s.captured_at=previous->captured_at;if(mode==9)s.ip_version=6;
        r=update_linux_tcp_holder(previous,next);assert(metric_is_known(r)&&!r.value->acknowledged_bytes_per_second.value&&!r.value->received_bytes_per_second.value);
    }
    for(auto support:{Support::permission_required,Support::unsupported}){
        previous.reset();update_linux_tcp_holder(previous,observation(1,10,20));
        auto denied=observation(2,110,220);denied.support=support; // Even a caller-constructed present value must not leak.
        r=update_linux_tcp_holder(previous,denied);assert(!r.value&&r.support==support&&!previous);
        assert(!update_linux_tcp_holder(previous,observation(3,210,320)).value->received_bytes_per_second.value);
    }
    previous.reset();update_linux_tcp_holder(previous,observation(1,10,20));auto missing=observation(2,110,220);missing.value->received_bytes.value.reset();
    r=update_linux_tcp_holder(previous,missing);assert(r.value->acknowledged_bytes_per_second.value==100&&!r.value->received_bytes_per_second.value);
    assert(!update_linux_tcp_holder(previous,observation(3,210,320)).value->received_bytes_per_second.value);
    assert(update_linux_tcp_holder(previous,observation(4,310,420)).value->received_bytes_per_second.value==100);
    previous.reset();auto edge=observation(1,0,0);edge.value->captured_at=std::chrono::steady_clock::time_point::min();update_linux_tcp_holder(previous,edge);
    edge.value->captured_at=std::chrono::steady_clock::time_point::max();edge.value->acknowledged_bytes=known<std::uint64_t>(1);
    assert(update_linux_tcp_holder(previous,edge).value->acknowledged_bytes_per_second.value==0);
    auto invalid=observation(2,1,1);invalid.value->cookie.value.reset();assert(!update_linux_tcp_holder(previous,invalid).value&&!previous);
    LinuxTcpSocketHolderSampler sampler;assert(!sampler.sample({}).value);sampler.reset();
}
