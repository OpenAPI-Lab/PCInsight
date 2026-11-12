#include "../src/linux_tcp.hpp"
#include <cassert>
#include <algorithm>
#include <cerrno>
#include <cstring>
#include <iostream>
#include <limits>
#include <stdexcept>
#ifdef __linux__
#include <linux/inet_diag.h>
#include <linux/netlink.h>
#include <linux/sock_diag.h>
#include <linux/tcp.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#endif
using namespace pcinsight;
using namespace pcinsight::detail;
using Clock=std::chrono::steady_clock;
template<class T> Metric<T> known(T v){return {v,Support::available,"fixture"};}
LinuxTcpSnapshot snapshot(int seconds,std::uint64_t acked,std::uint64_t received){
    LinuxTcpSnapshot s;s.network_namespace_device=1;s.network_namespace_inode=2;s.user_namespace_device=1;s.user_namespace_inode=3;
    s.started_at=s.finished_at=Clock::time_point{}+std::chrono::seconds(seconds);
    s.ipv4_dump_completed=s.ipv6_dump_completed=known(true);
    LinuxTcpSocketSample row;row.ip_version=4;row.state=1;row.inode=10;row.socket_uid=0;
    row.cookie=known<std::uint64_t>(99);row.captured_at=s.started_at;
    row.acknowledged_bytes=known(acked);row.received_bytes=known(received);s.sockets.push_back(row);return s;
}
template<class F> void fails(F f){bool failed=false;try{f();}catch(const std::exception&){failed=true;}assert(failed);}
#ifdef __linux__
template<class T> void put(std::vector<std::byte>& b,std::size_t offset,const T& value){assert(offset+sizeof(T)<=b.size());std::memcpy(b.data()+offset,&value,sizeof(value));}
std::vector<std::byte> packet(bool info=true,std::size_t length=sizeof(tcp_info)){
    const auto size=sizeof(nlmsghdr)+sizeof(inet_diag_msg)+(info?sizeof(nlattr)+length:0);
    std::vector<std::byte> b((size+3)&~std::size_t{3});
    nlmsghdr h{};h.nlmsg_len=static_cast<std::uint32_t>(size);h.nlmsg_type=SOCK_DIAG_BY_FAMILY;h.nlmsg_seq=4;h.nlmsg_pid=77;h.nlmsg_flags=NLM_F_MULTI;put(b,0,h);
    inet_diag_msg m{};m.idiag_family=AF_INET;m.idiag_state=1;m.idiag_inode=10;m.idiag_uid=0;m.id.idiag_cookie[0]=99;put(b,sizeof(h),m);
    if(info){nlattr a{};a.nla_len=static_cast<std::uint16_t>(sizeof(a)+length);a.nla_type=INET_DIAG_INFO;put(b,sizeof(h)+sizeof(m),a);
        tcp_info t{};t.tcpi_bytes_acked=123;t.tcpi_bytes_received=456;
        std::memcpy(b.data()+sizeof(h)+sizeof(m)+sizeof(a),&t,std::min(length,sizeof(t)));
    }
    return b;
}
LinuxTcpDumpParser parser(){LinuxTcpDumpParser p;p.sequence=4;p.port=77;p.ip_version=4;return p;}
void wire_tests(){
    auto p=parser();auto b=packet();p.consume(b,{});
    assert(p.rows.size()==1&&p.rows[0].socket_uid==0&&p.rows[0].cookie.value==99);
    assert(p.rows[0].acknowledged_bytes.value==123&&p.rows[0].received_bytes.value==456);
    auto ipv6=packet();inet_diag_msg v6{};std::memcpy(&v6,ipv6.data()+sizeof(nlmsghdr),sizeof(v6));
    v6.idiag_family=AF_INET6;put(ipv6,sizeof(nlmsghdr),v6);auto p6=parser();p6.ip_version=6;p6.consume(ipv6,{});
    assert(p6.rows[0].ip_version==6&&p6.rows[0].received_bytes.value==456);
    auto short_p=parser();short_p.consume(packet(true,offsetof(tcp_info,tcpi_bytes_received)),{});
    assert(short_p.rows[0].acknowledged_bytes.value==123&&!short_p.rows[0].received_bytes.value);
    short_p=parser();short_p.consume(packet(false),{});assert(!short_p.rows[0].acknowledged_bytes.value);
    for(unsigned mode=0;mode<10;++mode){
        b=packet();auto h=nlmsghdr{};std::memcpy(&h,b.data(),sizeof(h));
        if(mode==0)++h.nlmsg_seq;
        if(mode==1)++h.nlmsg_pid;
        if(mode==2)h.nlmsg_flags|=NLM_F_DUMP_INTR;
        if(mode==3)h.nlmsg_len=1;
        if(mode==4)h.nlmsg_len=static_cast<std::uint32_t>(b.size()+1);
        if(mode==5)h.nlmsg_type=NLMSG_OVERRUN;
        put(b,0,h);
        if(mode==6){inet_diag_msg m{};m.idiag_family=AF_INET6;m.idiag_state=1;put(b,sizeof(h),m);}
        if(mode==7){nlattr a{3,INET_DIAG_INFO};put(b,sizeof(h)+sizeof(inet_diag_msg),a);}
        if(mode==8){nlattr a{65535,INET_DIAG_INFO};put(b,sizeof(h)+sizeof(inet_diag_msg),a);}
        if(mode==9)b.resize(3);
        auto invalid=parser();fails([&]{invalid.consume(b,{});});
    }
    b=packet();inet_diag_msg m{};m.idiag_family=AF_INET;m.idiag_state=1;m.id.idiag_cookie[0]=m.id.idiag_cookie[1]=INET_DIAG_NOCOOKIE;
    put(b,sizeof(nlmsghdr),m);p=parser();p.consume(b,{});assert(!p.rows[0].cookie.value);
    std::vector<std::byte> done(sizeof(nlmsghdr)+sizeof(int));nlmsghdr h{};h.nlmsg_len=static_cast<std::uint32_t>(done.size());h.nlmsg_type=NLMSG_DONE;h.nlmsg_seq=4;h.nlmsg_pid=77;put(done,0,h);
    p=parser();p.consume(done,{});assert(p.done&&p.rows.empty());fails([&]{p.consume(packet(),{});});
    put(done,sizeof(h),-EACCES);p=parser();fails([&]{p.consume(done,{});});
    put(done,sizeof(h),std::numeric_limits<int>::min());p=parser();fails([&]{p.consume(done,{});});
    // Duplicate TCP_INFO attributes and post-DONE payload are not accepted.
    b=packet();auto duplicate=packet();b.insert(b.end(),duplicate.begin()+sizeof(nlmsghdr)+sizeof(inet_diag_msg),duplicate.end());
    std::memcpy(&h,b.data(),sizeof(h));h.nlmsg_len=static_cast<std::uint32_t>(b.size());put(b,0,h);p=parser();fails([&]{p.consume(b,{});});
}
struct Fd {int n{-1};~Fd(){if(n>=0)close(n);}};
int native_test(){
    LinuxTcpSampler sampler;const auto probe=sampler.sample();
    if(!metric_is_known(probe)||!metric_is_known(probe.value->ipv4_dump_completed)){
        std::cout<<"SKIP native IPv4 TCP diagnostic source: "<<probe.note<<'\n';return 77;
    }
    const Fd listener{socket(AF_INET,SOCK_STREAM|SOCK_CLOEXEC,0)},client{socket(AF_INET,SOCK_STREAM|SOCK_CLOEXEC,0)};
    assert(listener.n>=0&&client.n>=0);sockaddr_in address{};address.sin_family=AF_INET;address.sin_addr.s_addr=htonl(INADDR_LOOPBACK);
    assert(bind(listener.n,reinterpret_cast<sockaddr*>(&address),sizeof(address))==0&&listen(listener.n,1)==0);
    socklen_t length=sizeof(address);assert(getsockname(listener.n,reinterpret_cast<sockaddr*>(&address),&length)==0);
    assert(connect(client.n,reinterpret_cast<sockaddr*>(&address),sizeof(address))==0);
    const Fd server{accept4(listener.n,nullptr,nullptr,SOCK_CLOEXEC)};assert(server.n>=0);
    timeval timeout{1,0};for(int fd:{client.n,server.n}){
        assert(setsockopt(fd,SOL_SOCKET,SO_RCVTIMEO,&timeout,sizeof(timeout))==0);
        assert(setsockopt(fd,SOL_SOCKET,SO_SNDTIMEO,&timeout,sizeof(timeout))==0);
    }
    std::uint64_t cookie{};length=sizeof(cookie);assert(getsockopt(client.n,SOL_SOCKET,SO_COOKIE,&cookie,&length)==0&&length==sizeof(cookie));
    std::uint64_t server_cookie{};length=sizeof(server_cookie);assert(getsockopt(server.n,SOL_SOCKET,SO_COOKIE,&server_cookie,&length)==0&&length==sizeof(server_cookie));
    sampler.reset();auto first=sampler.sample();assert(metric_is_known(first));
    auto selected=[&](const LinuxTcpSnapshot& s)->const LinuxTcpSocketSample*{for(const auto& r:s.sockets)if(r.cookie.value==cookie)return &r;return nullptr;};
    const auto* before=selected(*first.value);assert(before&&metric_is_known(before->acknowledged_bytes)&&!before->acknowledged_bytes_per_second.value);
    const auto initial=*before->acknowledged_bytes.value;
    const auto server_before=std::find_if(first.value->sockets.begin(),first.value->sockets.end(),[&](const auto& r){return r.cookie.value==server_cookie;});
    assert(server_before!=first.value->sockets.end()&&server_before->received_bytes.value);
    const auto initial_received=*server_before->received_bytes.value;
    const std::string data(8192,'x');std::size_t sent=0,received=0;
    while(sent<data.size()){const auto n=send(client.n,data.data()+sent,data.size()-sent,MSG_NOSIGNAL);assert(n>0);sent+=static_cast<std::size_t>(n);}
    std::string buffer(data.size(),' ');while(received<data.size()){const auto n=recv(server.n,buffer.data()+received,buffer.size()-received,0);assert(n>0);received+=static_cast<std::size_t>(n);}assert(buffer==data);
    int quick=1;assert(setsockopt(server.n,IPPROTO_TCP,TCP_QUICKACK,&quick,sizeof(quick))==0);
    bool observed=false;
    for(unsigned attempt=0;attempt<3;++attempt){
        const auto next=sampler.sample();assert(metric_is_known(next));const auto* row=selected(*next.value);assert(row);
        if(row->acknowledged_bytes.value&&*row->acknowledged_bytes.value>=initial+data.size()){
            const auto server_after=std::find_if(next.value->sockets.begin(),next.value->sockets.end(),[&](const auto& r){return r.cookie.value==server_cookie;});
            assert(server_after!=next.value->sockets.end()&&server_after->received_bytes.value&&*server_after->received_bytes.value>=initial_received+data.size());
            assert(metric_is_known(server_after->received_bytes_per_second));
            assert(metric_is_known(row->acknowledged_bytes_per_second));observed=true;
            std::cout<<"Native IPv4 loopback client cookie="<<cookie<<" socket_uid="<<row->socket_uid<<" acknowledged delta="<<*row->acknowledged_bytes.value-initial
                <<" bytes; server received delta="<<*server_after->received_bytes.value-initial_received<<" bytes; sampled ACKed rate="<<*row->acknowledged_bytes_per_second.value
                <<" B/s; receive rate="<<*server_after->received_bytes_per_second.value<<" B/s; sockets="<<next.value->sockets.size()<<" UID groups="<<next.value->users.size()<<'\n';break;
        }
    }
    assert(observed);sampler.reset();const auto reset=sampler.sample();assert(metric_is_known(reset));const auto* row=selected(*reset.value);assert(row&&!row->acknowledged_bytes_per_second.value);
    return 0;
}
#endif
int main(int argc,char** argv){
#ifdef __linux__
    if(argc==2&&std::string(argv[1])=="--native-loopback")return native_test();
    wire_tests();
#else
    (void)argc;(void)argv;LinuxTcpSampler sampler;assert(!sampler.sample().value);sampler.reset();
#endif
    LinuxTcpTracker tracker;auto first=tracker.apply(snapshot(1,10,20));assert(!first.sockets[0].acknowledged_bytes_per_second.value);
    auto next=tracker.apply(snapshot(3,110,220));assert(next.sockets[0].acknowledged_bytes_per_second.value==50&&next.sockets[0].received_bytes_per_second.value==100);
    assert(next.users.size()==1&&next.users[0].socket_uid==0&&next.users[0].acknowledged_coverage.known==1);
    next=tracker.apply(snapshot(4,110,220));assert(next.users[0].acknowledged_bytes_per_second.value==0);
    for(unsigned mode=0;mode<7;++mode){
        tracker.reset();tracker.apply(snapshot(1,10,20));auto input=snapshot(2,110,220);
        if(mode==0)input.sockets[0].cookie=known<std::uint64_t>(100);
        if(mode==1)++input.sockets[0].socket_uid;
        if(mode==2)++input.sockets[0].inode;
        if(mode==3)++input.network_namespace_inode;
        if(mode==4)++input.user_namespace_inode;
        if(mode==5)input.sockets[0].acknowledged_bytes=known<std::uint64_t>(9);
        if(mode==6)input.started_at=input.finished_at=input.sockets[0].captured_at=Clock::time_point{}+std::chrono::seconds(1);
        next=tracker.apply(input);assert(!next.sockets[0].acknowledged_bytes_per_second.value&&!next.sockets[0].received_bytes_per_second.value);
    }
    tracker.reset();tracker.apply(snapshot(1,10,20));auto missing=snapshot(2,110,220);missing.sockets[0].received_bytes.value.reset();
    next=tracker.apply(missing);assert(next.sockets[0].acknowledged_bytes_per_second.value==100&&!next.sockets[0].received_bytes_per_second.value);
    next=tracker.apply(snapshot(3,210,320));assert(!next.sockets[0].received_bytes_per_second.value);
    next=tracker.apply(snapshot(4,310,420));assert(next.sockets[0].received_bytes_per_second.value==100);
    auto empty=snapshot(5,0,0);empty.sockets.clear();assert(tracker.apply(empty).users.empty());
    assert(!tracker.apply(snapshot(6,410,520)).users[0].received_bytes_per_second.value);
    // One failed family loses its references, while the other remains observable.
    tracker.reset();tracker.apply(snapshot(1,10,20));
    auto partial=snapshot(2,0,0);partial.sockets.clear();partial.ipv4_dump_completed={{},Support::permission_required,"denied"};
    partial.sockets=snapshot(2,10,20).sockets;partial.sockets[0].ip_version=6;partial.sockets[0].cookie=known<std::uint64_t>(100);
    next=tracker.apply(partial);assert(next.sockets.size()==1&&!next.users[0].received_bytes_per_second.value);
    next=tracker.apply(snapshot(3,110,220));assert(!next.sockets[0].received_bytes_per_second.value);
    tracker.reset();auto no_cookie=snapshot(1,10,20);no_cookie.sockets[0].cookie.value.reset();
    assert(!tracker.apply(no_cookie).users[0].acknowledged_bytes_per_second.value);
    assert(!tracker.apply(snapshot(2,110,220)).users[0].acknowledged_bytes_per_second.value);
    // No signed duration overflow when observations span the clock extremes.
    tracker.reset();auto edge=snapshot(1,0,0);edge.started_at=edge.finished_at=edge.sockets[0].captured_at=Clock::time_point::min();tracker.apply(edge);
    edge.started_at=edge.finished_at=edge.sockets[0].captured_at=Clock::time_point::max();edge.sockets[0].acknowledged_bytes=known<std::uint64_t>(1);
    assert(tracker.apply(edge).sockets[0].acknowledged_bytes_per_second.value==0);
    tracker.reset();tracker.apply(snapshot(1,0,0));auto too_fast=snapshot(1,std::numeric_limits<std::uint64_t>::max(),0);
    too_fast.started_at=too_fast.finished_at=too_fast.sockets[0].captured_at=Clock::time_point{}+std::chrono::seconds(1)+std::chrono::milliseconds(1);
    assert(!tracker.apply(too_fast).users[0].acknowledged_bytes_per_second.value);
    for(unsigned mode=0;mode<4;++mode){
        auto invalid=snapshot(8,1,1);
        if(mode==0)invalid.sockets.push_back(invalid.sockets[0]);
        if(mode==1)invalid.ipv4_dump_completed={{},Support::permission_required,"denied"};
        if(mode==2)invalid.sockets[0].state=10;
        if(mode==3)invalid.sockets[0].captured_at+=std::chrono::seconds(1);
        fails([&]{tracker.apply(invalid);});assert(!tracker.apply(snapshot(9,100,100)).sockets[0].acknowledged_bytes_per_second.value);
    }
    // Known rates may sum outside uint64 even though each socket rate fits.
    tracker.reset();auto a=snapshot(1,0,0);a.sockets.push_back(a.sockets[0]);a.sockets[1].cookie=known<std::uint64_t>(100);tracker.apply(a);
    a=snapshot(2,1ull<<63,0);a.sockets.push_back(a.sockets[0]);a.sockets[1].cookie=known<std::uint64_t>(100);next=tracker.apply(a);
    assert(!next.users[0].acknowledged_bytes_per_second.value&&next.users[0].acknowledged_coverage.overflow&&next.users[0].acknowledged_coverage.known==2);
}
