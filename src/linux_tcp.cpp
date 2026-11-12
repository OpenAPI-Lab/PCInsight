#include "linux_tcp.hpp"
#include <algorithm>
#include <cerrno>
#include <cmath>
#include <cstring>
#include <limits>
#include <map>
#include <set>
#include <stdexcept>
#include <system_error>
#ifdef __linux__
#include <fcntl.h>
#include <linux/inet_diag.h>
#include <linux/netlink.h>
#include <linux/sock_diag.h>
#include <linux/tcp.h>
#include <poll.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <unistd.h>
#endif
namespace pcinsight::detail {
namespace {
using Clock=std::chrono::steady_clock;
template<class T> Metric<T> absent(std::string reason){return {{},Support::unsupported,std::move(reason)};}
Metric<std::uint64_t> rate(const Metric<std::uint64_t>& now,const Metric<std::uint64_t>& old,long double seconds){
    if(!metric_is_known(now)||!metric_is_known(old)||*now.value<*old.value||seconds<=0||!std::isfinite(seconds))
        return absent<std::uint64_t>("TCP counter missing/decreased or sample clock did not advance");
    const auto value=static_cast<long double>(*now.value-*old.value)/seconds;
    if(!std::isfinite(value)||value<0||value>=std::ldexp(1.0L,64))return absent<std::uint64_t>("TCP byte rate exceeds uint64 range");
    return {static_cast<std::uint64_t>(value),Support::estimated,"Native TCP counter delta / socket observation interval, rounded down; not wire or all-network throughput"};
}
bool completed(const Metric<bool>& m){return metric_is_known(m)&&*m.value;}
bool same_namespace(const LinuxTcpSnapshot& a,const LinuxTcpSnapshot& b){
    return a.network_namespace_device==b.network_namespace_device&&a.network_namespace_inode==b.network_namespace_inode&&
        a.user_namespace_device==b.user_namespace_device&&a.user_namespace_inode==b.user_namespace_inode;
}
struct Total {
    std::uint64_t value{};LinuxTcpRateCoverage coverage;
    void add(const Metric<std::uint64_t>& m){
        if(!metric_is_known(m)){++coverage.missing;return;}
        ++coverage.known;
        if(*m.value>std::numeric_limits<std::uint64_t>::max()-value)coverage.overflow=true;
        if(!coverage.overflow)value+=*m.value;
    }
    Metric<std::uint64_t> result() const{
        if(coverage.overflow)return absent<std::uint64_t>("Visible TCP socket rate sum overflow; no saturation/wrap");
        if(!coverage.known)return absent<std::uint64_t>("No known socket rates; absence is not zero");
        return {value,Support::estimated,"Sum for kernel-reported socket UID, not process/user responsibility; observed persistent TCP sockets only, partial coverage and different per-socket intervals"};
    }
};
}
LinuxTcpSnapshot LinuxTcpTracker::apply(LinuxTcpSnapshot snapshot){
    try{
        if(snapshot.sockets.size()>100000||snapshot.finished_at<snapshot.started_at||!snapshot.network_namespace_inode||!snapshot.user_namespace_inode)
            throw std::invalid_argument("Invalid/bounded TCP observation batch");
        std::map<std::uint64_t,const LinuxTcpSocketSample*> old;
        if(previous_&&same_namespace(snapshot,*previous_))
            for(const auto& s:previous_->sockets)if(metric_is_known(s.cookie))old.emplace(*s.cookie.value,&s);
        std::set<std::uint64_t> cookies;
        struct User {std::uint64_t count{};Total acked,received;};
        std::map<std::uint32_t,User> users;
        for(auto& s:snapshot.sockets){
            if((s.ip_version!=4&&s.ip_version!=6)||(s.state!=1&&s.state!=8)||s.captured_at<snapshot.started_at||s.captured_at>snapshot.finished_at||
                !completed(s.ip_version==4?snapshot.ipv4_dump_completed:snapshot.ipv6_dump_completed))throw std::invalid_argument("TCP row outside declared family/state/time scope");
            s.acknowledged_bytes_per_second=s.received_bytes_per_second=absent<std::uint64_t>("Requires consecutive matching socket-cookie/UID/inode/namespace observations");
            if(metric_is_known(s.cookie)){
                if(!cookies.insert(*s.cookie.value).second)throw std::invalid_argument("Duplicate TCP socket cookie; no double counting");
                const auto found=old.find(*s.cookie.value);
                if(found!=old.end()){
                    const auto& p=*found->second;
                    if(p.ip_version==s.ip_version&&p.inode==s.inode&&p.socket_uid==s.socket_uid&&s.captured_at>p.captured_at){
                        const auto seconds=std::chrono::duration<long double>(s.captured_at.time_since_epoch()).count()-std::chrono::duration<long double>(p.captured_at.time_since_epoch()).count();
                        const bool decreased=(metric_is_known(s.acknowledged_bytes)&&metric_is_known(p.acknowledged_bytes)&&*s.acknowledged_bytes.value<*p.acknowledged_bytes.value)||
                            (metric_is_known(s.received_bytes)&&metric_is_known(p.received_bytes)&&*s.received_bytes.value<*p.received_bytes.value);
                        if(!decreased){
                            s.acknowledged_bytes_per_second=rate(s.acknowledged_bytes,p.acknowledged_bytes,seconds);
                            s.received_bytes_per_second=rate(s.received_bytes,p.received_bytes,seconds);
                        }
                    }
                }
            }
            auto& user=users[s.socket_uid];++user.count;
            user.acked.add(s.acknowledged_bytes_per_second);user.received.add(s.received_bytes_per_second);
        }
        snapshot.users.clear();
        for(const auto& [uid,u]:users)snapshot.users.push_back({uid,u.count,u.acked.result(),u.received.result(),u.acked.coverage,u.received.coverage});
        previous_=snapshot;return snapshot;
    }catch(...){reset();throw;}
}
#ifdef __linux__
namespace {
template<class T> T load(std::span<const std::byte> bytes,std::size_t offset=0){
    if(offset>bytes.size()||sizeof(T)>bytes.size()-offset)throw std::runtime_error("Truncated TCP diagnostic wire object");
    T out;std::memcpy(&out,bytes.data()+offset,sizeof(out));return out;
}
std::size_t aligned(std::size_t size){return (size+3)&~std::size_t{3};}
Metric<std::uint64_t> tcp_counter(std::span<const std::byte> data,std::size_t offset,const char* name){
    if(offset>data.size()||sizeof(std::uint64_t)>data.size()-offset)return absent<std::uint64_t>(std::string("TCP_INFO too short for ")+name);
    return {load<std::uint64_t>(data,offset),Support::available,std::string("Native TCP_INFO ")+name+" cumulative bytes"};
}
}
void LinuxTcpDumpParser::consume(std::span<const std::byte> packet,Clock::time_point at){
    if((ip_version!=4&&ip_version!=6)||done||packet.empty()||wire_bytes>16*1024*1024||packet.size()>16*1024*1024-wire_bytes)throw std::runtime_error("TCP dump ended/empty/invalid family or wire bound exceeded");
    wire_bytes+=packet.size();
    while(!packet.empty()){
        const auto h=load<nlmsghdr>(packet);
        if(++messages>100000||h.nlmsg_len<sizeof(h)||h.nlmsg_len>packet.size()||h.nlmsg_seq!=sequence||h.nlmsg_pid!=port||
            (h.nlmsg_flags&NLM_F_DUMP_INTR))throw std::runtime_error("Invalid/interrupted/unmatched TCP netlink dump");
        const auto payload=packet.subspan(sizeof(h),h.nlmsg_len-sizeof(h));
        if(h.nlmsg_type==NLMSG_DONE){
            if(!payload.empty()){
                const auto error=load<int>(payload);
                if(error>0||error< -4095)throw std::runtime_error("Invalid TCP dump completion error");
                if(error)throw std::system_error(-error,std::generic_category(),"TCP dump completion");
            }
            done=true;
        }else if(h.nlmsg_type==NLMSG_ERROR){
            const auto e=load<nlmsgerr>(payload);
            if(e.error>0||e.error< -4095)throw std::runtime_error("Invalid TCP diagnostic error code");
            if(e.error)throw std::system_error(-e.error,std::generic_category(),"TCP diagnostic request");
        }else if(h.nlmsg_type==SOCK_DIAG_BY_FAMILY){
            const auto msg=load<inet_diag_msg>(payload);
            if(msg.idiag_family!=(ip_version==4?AF_INET:AF_INET6)||(msg.idiag_state!=1&&msg.idiag_state!=8)||rows.size()>=50000)
                throw std::runtime_error("Invalid/bounded TCP diagnostic family/state");
            LinuxTcpSocketSample s;s.ip_version=ip_version;s.state=msg.idiag_state;s.inode=msg.idiag_inode;s.socket_uid=msg.idiag_uid;s.captured_at=at;
            const auto cookie=static_cast<std::uint64_t>(msg.id.idiag_cookie[0])|(static_cast<std::uint64_t>(msg.id.idiag_cookie[1])<<32);
            s.cookie=cookie==std::numeric_limits<std::uint64_t>::max()?absent<std::uint64_t>("INET_DIAG_NOCOOKIE; cannot retain socket identity"):
                Metric<std::uint64_t>{cookie,Support::available,"Native diagnostic socket cookie, not PID/inode alone"};
            s.acknowledged_bytes=s.received_bytes=absent<std::uint64_t>("TCP_INFO attribute absent");
            auto attrs=payload.subspan(sizeof(msg));bool info=false;
            while(!attrs.empty()){
                const auto a=load<nlattr>(attrs);
                if(a.nla_len<sizeof(a)||a.nla_len>attrs.size())throw std::runtime_error("Invalid TCP diagnostic attribute length");
                if((a.nla_type&NLA_TYPE_MASK)==INET_DIAG_INFO){
                    if(info||(a.nla_type&~NLA_TYPE_MASK))throw std::runtime_error("Duplicate/flagged TCP_INFO attribute");
                    info=true;const auto data=attrs.subspan(sizeof(a),a.nla_len-sizeof(a));
                    s.acknowledged_bytes=tcp_counter(data,offsetof(tcp_info,tcpi_bytes_acked),"tcpi_bytes_acked");
                    s.received_bytes=tcp_counter(data,offsetof(tcp_info,tcpi_bytes_received),"tcpi_bytes_received");
                }
                const auto step=aligned(a.nla_len);
                if(step>attrs.size()){if(a.nla_len!=attrs.size())throw std::runtime_error("Truncated TCP attribute padding");attrs={};}
                else attrs=attrs.subspan(step);
            }
            rows.push_back(std::move(s));
        }else throw std::runtime_error("Unexpected TCP netlink message type");
        const auto step=aligned(h.nlmsg_len);
        if(step>packet.size()){if(h.nlmsg_len!=packet.size())throw std::runtime_error("Truncated TCP netlink padding");packet={};}
        else packet=packet.subspan(step);
        if(done&&!packet.empty())throw std::runtime_error("TCP data after dump completion");
    }
}
namespace {
struct Fd{int n{-1};~Fd(){if(n>=0)close(n);}};
void check(int result,const char* what){if(result<0)throw std::system_error(errno,std::generic_category(),what);}
struct Namespace {std::uint64_t dev{},inode{};bool operator==(const Namespace&)const=default;};
Namespace current_ns(const char* path){
    const Fd fd{open(path,O_RDONLY|O_CLOEXEC)};check(fd.n,"open thread namespace");
    struct stat st{};check(fstat(fd.n,&st),"stat thread namespace");
    if(!st.st_ino)throw std::runtime_error("Invalid thread namespace identity");
    return {static_cast<std::uint64_t>(st.st_dev),static_cast<std::uint64_t>(st.st_ino)};
}
Metric<bool> dump(unsigned ip_version,LinuxTcpSnapshot& out){
    try{
        const Fd fd{socket(AF_NETLINK,SOCK_DGRAM|SOCK_NONBLOCK|SOCK_CLOEXEC,NETLINK_SOCK_DIAG)};check(fd.n,"open SOCK_DIAG");
        sockaddr_nl local{};local.nl_family=AF_NETLINK;check(bind(fd.n,reinterpret_cast<sockaddr*>(&local),sizeof(local)),"bind SOCK_DIAG");
        socklen_t size=sizeof(local);check(getsockname(fd.n,reinterpret_cast<sockaddr*>(&local),&size),"SOCK_DIAG local port");
        if(size!=sizeof(local)||local.nl_family!=AF_NETLINK||!local.nl_pid)throw std::runtime_error("Invalid diagnostic local port");
        struct Request {nlmsghdr header;inet_diag_req_v2 body;} request{};
        request.header.nlmsg_len=sizeof(request);request.header.nlmsg_type=SOCK_DIAG_BY_FAMILY;
        request.header.nlmsg_flags=NLM_F_REQUEST|NLM_F_DUMP;request.header.nlmsg_seq=ip_version;
        request.body.sdiag_family=ip_version==4?AF_INET:AF_INET6;request.body.sdiag_protocol=IPPROTO_TCP;
        request.body.idiag_ext=1<<(INET_DIAG_INFO-1);request.body.idiag_states=(1u<<1)|(1u<<8);
        request.body.id.idiag_cookie[0]=request.body.id.idiag_cookie[1]=INET_DIAG_NOCOOKIE;
        sockaddr_nl kernel{};kernel.nl_family=AF_NETLINK;
        const auto sent=sendto(fd.n,&request,sizeof(request),MSG_DONTWAIT,reinterpret_cast<sockaddr*>(&kernel),sizeof(kernel));
        check(static_cast<int>(sent),"send SOCK_DIAG");if(static_cast<std::size_t>(sent)!=sizeof(request))throw std::runtime_error("Incomplete SOCK_DIAG request send");
        LinuxTcpDumpParser parser;parser.sequence=ip_version;parser.port=local.nl_pid;parser.ip_version=static_cast<std::uint8_t>(ip_version);
        const auto deadline=Clock::now()+std::chrono::seconds(2);unsigned interrupts=0;
        std::vector<std::byte> buffer(256*1024);
        while(!parser.done){
            if(Clock::now()>=deadline)throw std::runtime_error("TCP diagnostic dump deadline exceeded");
            pollfd poller{fd.n,POLLIN,0};const auto ready=poll(&poller,1,100);
            if(ready<0&&errno==EINTR&&++interrupts<=8)continue;
            check(ready,"poll SOCK_DIAG");if(!ready)continue;
            sockaddr_nl sender{};iovec iov{buffer.data(),buffer.size()};msghdr message{};
            message.msg_name=&sender;message.msg_namelen=sizeof(sender);message.msg_iov=&iov;message.msg_iovlen=1;
            const auto bytes=recvmsg(fd.n,&message,MSG_DONTWAIT|MSG_TRUNC);
            if(bytes<0&&(errno==EAGAIN||errno==EWOULDBLOCK))continue;
            if(bytes<0&&errno==EINTR&&++interrupts<=8)continue;
            check(static_cast<int>(bytes),"receive SOCK_DIAG");
            if(message.msg_namelen!=sizeof(sender)||sender.nl_family!=AF_NETLINK||sender.nl_pid!=0||
                (message.msg_flags&(MSG_TRUNC|MSG_CTRUNC))||bytes<=0||static_cast<std::size_t>(bytes)>buffer.size())throw std::runtime_error("Untrusted/truncated TCP diagnostic datagram");
            parser.consume(std::span(buffer).first(static_cast<std::size_t>(bytes)),Clock::now());
        }
        out.sockets.insert(out.sockets.end(),std::make_move_iterator(parser.rows.begin()),std::make_move_iterator(parser.rows.end()));
        return {true,Support::available,"Completed TCP ESTABLISHED/CLOSE_WAIT SOCK_DIAG dump of this family; not all traffic or an atomic snapshot"};
    }catch(const std::system_error& e){return {{},e.code().value()==EACCES||e.code().value()==EPERM?Support::permission_required:Support::unsupported,e.what()};}
    catch(const std::exception& e){return absent<bool>(e.what());}
}
}
#endif
}
namespace pcinsight {
struct LinuxTcpSampler::Impl {detail::LinuxTcpTracker tracker;};
LinuxTcpSampler::LinuxTcpSampler():impl_(std::make_unique<Impl>()){}
LinuxTcpSampler::~LinuxTcpSampler()=default;
LinuxTcpSampler::LinuxTcpSampler(LinuxTcpSampler&&) noexcept=default;
LinuxTcpSampler& LinuxTcpSampler::operator=(LinuxTcpSampler&&) noexcept=default;
void LinuxTcpSampler::reset(){if(impl_)impl_->tracker.reset();}
Metric<LinuxTcpSnapshot> LinuxTcpSampler::sample(){
    if(!impl_)impl_=std::make_unique<Impl>();
#ifdef __linux__
    try{
        LinuxTcpSnapshot s;s.started_at=std::chrono::steady_clock::now();
        const auto net=detail::current_ns("/proc/thread-self/ns/net"),user=detail::current_ns("/proc/thread-self/ns/user");
        s.network_namespace_device=net.dev;s.network_namespace_inode=net.inode;s.user_namespace_device=user.dev;s.user_namespace_inode=user.inode;
        s.ipv4_dump_completed=detail::dump(4,s);s.ipv6_dump_completed=detail::dump(6,s);
        if(net!=detail::current_ns("/proc/thread-self/ns/net")||user!=detail::current_ns("/proc/thread-self/ns/user"))throw std::runtime_error("Thread namespaces changed during TCP sample");
        if(!detail::completed(s.ipv4_dump_completed)&&!detail::completed(s.ipv6_dump_completed)){
            reset();return {{},s.ipv4_dump_completed.support==Support::permission_required||s.ipv6_dump_completed.support==Support::permission_required?Support::permission_required:Support::unsupported,
                "No completed TCP diagnostic family: IPv4="+s.ipv4_dump_completed.note+"; IPv6="+s.ipv6_dump_completed.note};
        }
        s.finished_at=std::chrono::steady_clock::now();
        return {impl_->tracker.apply(std::move(s)),Support::estimated,"Observed TCP socket counters/rates only, grouped by socket UID; not PID/app/all-network traffic; inspect each family source and coverage"};
    }catch(const std::system_error& e){reset();return {{},e.code().value()==EACCES||e.code().value()==EPERM?Support::permission_required:Support::unsupported,e.what()};}
    catch(const std::exception& e){reset();return {{},Support::unsupported,e.what()};}
#else
    reset();return {{},Support::unsupported,"Native TCP diagnostic sampler requires Linux"};
#endif
}
}
