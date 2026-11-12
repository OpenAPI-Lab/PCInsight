#include "linux_tcp.hpp"
#include "linux_process_details.hpp"
#include <cerrno>
#include <cmath>
#include <limits>
#include <exception>
#include <charconv>
#include <array>
#ifdef __linux__
#include <fcntl.h>
#include <linux/tcp.h>
#include <netinet/in.h>
#include <poll.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/statfs.h>
#include <sys/syscall.h>
#include <unistd.h>
#endif
namespace pcinsight::detail {
namespace {
template<class T> Metric<T> absent(const std::string& note,Support support=Support::unsupported){return {{},support,note};}
Metric<std::uint64_t> holder_rate(const Metric<std::uint64_t>& now,const Metric<std::uint64_t>& old,long double seconds){
    if(!metric_is_known(now)||!metric_is_known(old)||*now.value<*old.value||!std::isfinite(seconds)||seconds<=0)
        return absent<std::uint64_t>("Held TCP counter absent/decreased or invalid clock");
    const auto rate=static_cast<long double>(*now.value-*old.value)/seconds;
    if(!std::isfinite(rate)||rate<0||rate>=std::ldexp(1.0L,64))return absent<std::uint64_t>("Held TCP rate outside uint64 range");
    return {static_cast<std::uint64_t>(rate),Support::estimated,"Socket counter delta / own monotonic interval; holder association is not exclusive/causal process traffic"};
}
bool valid(const LinuxTcpSocketHolder& h){return h.pid>0&&h.pid<=static_cast<std::uint32_t>(std::numeric_limits<int>::max())&&h.process_start_ticks>0&&h.descriptor>=0;}
}
Metric<std::uint32_t> parse_linux_tcp_pidfd_pid(std::string_view text){
    if(text.empty()||text.size()>65536||text.find('\0')!=text.npos)return absent<std::uint32_t>("Invalid/bounded pidfd fdinfo");
    std::optional<std::uint32_t> pid;unsigned lines=0;
    while(!text.empty()){
        if(++lines>10000)return absent<std::uint32_t>("Too many pidfd fdinfo lines");
        const auto end=text.find('\n');const auto line=text.substr(0,end);
        if(line.starts_with("Pid:")){
            if(pid)return absent<std::uint32_t>("Duplicate pidfd Pid field");
            auto value=line.substr(4);const auto first=value.find_first_not_of(" \t\r"),last=value.find_last_not_of(" \t\r");
            if(first==value.npos)return absent<std::uint32_t>("Missing pidfd Pid value");
            value=value.substr(first,last-first+1);std::uint32_t number{};const auto [p,error]=std::from_chars(value.data(),value.data()+value.size(),number);
            if(error!=std::errc{}||p!=value.data()+value.size()||number==0||number>static_cast<std::uint32_t>(std::numeric_limits<int>::max()))return absent<std::uint32_t>("Pidfd process exited/not mapped or invalid Pid");
            pid=number;
        }
        if(end==text.npos)break;
        text.remove_prefix(end+1);
    }
    if(!pid)return absent<std::uint32_t>("No pidfd Pid field; no numeric PID fallback");
    return {*pid,Support::available,"Pidfd target PID projected into this procfs mount's PID namespace"};
}
Metric<LinuxTcpHeldSocketSample> update_linux_tcp_holder(std::optional<LinuxTcpHeldSocketSample>& previous,Metric<LinuxTcpHeldSocketSample> current){
    if(!metric_is_known(current)){previous.reset();return absent<LinuxTcpHeldSocketSample>(current.note,current.support);}
    auto& s=*current.value;
    if(!valid(s.holder)||!s.socket_inode||!metric_is_known(s.cookie)||(s.ip_version!=4&&s.ip_version!=6)||(s.state!=1&&s.state!=8)){
        previous.reset();return absent<LinuxTcpHeldSocketSample>("Invalid held TCP observation identity/state");
    }
    s.acknowledged_bytes_per_second=s.received_bytes_per_second=absent<std::uint64_t>("Requires consecutive matching holder and pinned socket observations");
    if(previous){
        const auto& p=*previous;
        const bool ns_same=metric_is_known(p.network_namespace_cookie)==metric_is_known(s.network_namespace_cookie)&&
            (!metric_is_known(s.network_namespace_cookie)||p.network_namespace_cookie.value==s.network_namespace_cookie.value);
        const bool decreased=(metric_is_known(s.acknowledged_bytes)&&metric_is_known(p.acknowledged_bytes)&&*s.acknowledged_bytes.value<*p.acknowledged_bytes.value)||
            (metric_is_known(s.received_bytes)&&metric_is_known(p.received_bytes)&&*s.received_bytes.value<*p.received_bytes.value);
        if(s.holder==p.holder&&s.cookie.value==p.cookie.value&&s.socket_inode==p.socket_inode&&s.ip_version==p.ip_version&&ns_same&&!decreased&&s.captured_at>p.captured_at){
            const auto seconds=std::chrono::duration<long double>(s.captured_at.time_since_epoch()).count()-std::chrono::duration<long double>(p.captured_at.time_since_epoch()).count();
            s.acknowledged_bytes_per_second=holder_rate(s.acknowledged_bytes,p.acknowledged_bytes,seconds);
            s.received_bytes_per_second=holder_rate(s.received_bytes,p.received_bytes,seconds);
        }
    }
    previous=s;current.support=Support::estimated;
    current.note="Pinned TCP socket observed at the selected process descriptor endpoints; may be shared/transferred, never exclusive process traffic";
    return current;
}
#ifdef __linux__
namespace {
struct Fd {int n{-1};~Fd(){if(n>=0)close(n);}};
template<class T> Metric<T> failure(const char* operation,int error){return absent<T>(std::string(operation)+": errno="+std::to_string(error),error==EPERM||error==EACCES?Support::permission_required:Support::unsupported);}
bool same_process(const Metric<LinuxProcessStat>& identity,const LinuxTcpSocketHolder& h){return metric_is_known(identity)&&identity.value->pid==h.pid&&identity.value->start_ticks==h.process_start_ticks;}
Metric<bool> alive(int pidfd){
    pollfd p{pidfd,POLLIN,0};const auto ready=poll(&p,1,0);
    if(ready<0)return failure<bool>("poll pinned holder",errno);
    if(ready!=0||p.revents)return absent<bool>("Pinned holder exited or pidfd readiness is invalid");
    return {true,Support::available,"Pinned process not exited"};
}
int duplicate(int pidfd,int descriptor,bool self){
    if(self)return fcntl(descriptor,F_DUPFD_CLOEXEC,0);
#ifdef SYS_pidfd_getfd
    return static_cast<int>(syscall(SYS_pidfd_getfd,pidfd,descriptor,0));
#else
    (void)pidfd;(void)descriptor;errno=ENOSYS;return -1;
#endif
}
Metric<std::uint32_t> mapped_pid(int pidfd){
    const auto path="/proc/self/fdinfo/"+std::to_string(pidfd);
    const Fd file{open(path.c_str(),O_RDONLY|O_CLOEXEC|O_NOFOLLOW|O_NONBLOCK)};
    if(file.n<0)return failure<std::uint32_t>("read pidfd fdinfo",errno);
    struct statfs fs{};
    if(fstatfs(file.n,&fs)<0)return failure<std::uint32_t>("verify pidfd fdinfo procfs",errno);
    if(fs.f_type!=0x9fa0)return absent<std::uint32_t>("Pidfd fdinfo not provided by procfs");
    std::string text;std::array<char,4096> buffer{};unsigned interrupts=0;
    for(;;){
        const auto count=read(file.n,buffer.data(),buffer.size());
        if(count<0){if(errno==EINTR&&++interrupts<=4)continue;return failure<std::uint32_t>("read bounded pidfd fdinfo",errno);}
        if(!count)break;
        if(static_cast<std::size_t>(count)>65536-text.size())return absent<std::uint32_t>("Pidfd fdinfo exceeds64KiB");
        text.append(buffer.data(),static_cast<std::size_t>(count));
    }
    return parse_linux_tcp_pidfd_pid(text);
}
template<class T> Metric<T> option(int fd,int level,int name,const char* label){
    T out{};socklen_t size=sizeof(out);
    if(getsockopt(fd,level,name,&out,&size)<0)return failure<T>(label,errno);
    if(size!=sizeof(out))return absent<T>(std::string(label)+": unexpected result length");
    return {out,Support::available,label};
}
Metric<LinuxTcpHeldSocketSample> native_holder(const LinuxTcpSocketHolder& h){
    struct statfs fs{};
    if(statfs("/proc",&fs)<0)return failure<LinuxTcpHeldSocketSample>("verify procfs",errno);
    if(fs.f_type!=0x9fa0)return absent<LinuxTcpHeldSocketSample>("Not native procfs; no holder identity fallback");
    const auto self_identity=read_linux_process_identity("/proc/self");
    if(!metric_is_known(self_identity))return absent<LinuxTcpHeldSocketSample>(self_identity.note,self_identity.support);
    if(self_identity.value->pid!=static_cast<std::uint32_t>(getpid()))
        return absent<LinuxTcpHeldSocketSample>("procfs PID view differs from caller; no numeric PID fallback");
    const bool self=h.pid==static_cast<std::uint32_t>(getpid());
    int handle=-1;
    if(!self){
#ifdef SYS_pidfd_open
        handle=static_cast<int>(syscall(SYS_pidfd_open,static_cast<pid_t>(h.pid),0));
        if(handle<0)return failure<LinuxTcpHeldSocketSample>("pidfd_open selected holder",errno);
#else
        return absent<LinuxTcpHeldSocketSample>("pidfd_open unavailable; no proc-fd/inode fallback");
#endif
    }
    const Fd process{handle};const auto path="/proc/"+std::to_string(h.pid);
    if(!self){
        const auto projected=mapped_pid(process.n);
        if(!metric_is_known(projected))return absent<LinuxTcpHeldSocketSample>(projected.note,projected.support);
        if(*projected.value!=h.pid)return absent<LinuxTcpHeldSocketSample>("Pinned target PID differs in procfs namespace; no descriptor query attempted");
    }
    const auto before=read_linux_process_identity(path);
    if(!same_process(before,h))return absent<LinuxTcpHeldSocketSample>("Holder PID/start_ticks absent or changed before duplication",before.support==Support::permission_required?Support::permission_required:Support::unsupported);
    if(!self){const auto live=alive(process.n);if(!metric_is_known(live))return absent<LinuxTcpHeldSocketSample>(live.note,live.support);}
    const Fd socket{duplicate(process.n,h.descriptor,self)};
    if(socket.n<0)return failure<LinuxTcpHeldSocketSample>("duplicate selected descriptor (pidfd_getfd for other processes)",errno);
    struct stat st{};
    if(fstat(socket.n,&st)<0)return failure<LinuxTcpHeldSocketSample>("stat pinned descriptor",errno);
    if(!S_ISSOCK(st.st_mode)||!st.st_ino)return absent<LinuxTcpHeldSocketSample>("Selected descriptor is not a socket");
    const auto domain=option<int>(socket.n,SOL_SOCKET,SO_DOMAIN,"SO_DOMAIN"),protocol=option<int>(socket.n,SOL_SOCKET,SO_PROTOCOL,"SO_PROTOCOL"),type=option<int>(socket.n,SOL_SOCKET,SO_TYPE,"SO_TYPE");
    for(const auto* field:{&domain,&protocol,&type})if(!metric_is_known(*field))return absent<LinuxTcpHeldSocketSample>(field->note,field->support);
    if((*domain.value!=AF_INET&&*domain.value!=AF_INET6)||*protocol.value!=IPPROTO_TCP||*type.value!=SOCK_STREAM)
        return absent<LinuxTcpHeldSocketSample>("Selected socket is not a readable IPv4/IPv6 TCP stream");
    const auto cookie=option<std::uint64_t>(socket.n,SOL_SOCKET,SO_COOKIE,"SO_COOKIE");
    if(!metric_is_known(cookie))return absent<LinuxTcpHeldSocketSample>(cookie.note,cookie.support);
    LinuxTcpHeldSocketSample s;s.holder=h;s.socket_inode=static_cast<std::uint64_t>(st.st_ino);s.cookie=cookie;s.ip_version=*domain.value==AF_INET?4:6;
#ifdef SO_NETNS_COOKIE
    s.network_namespace_cookie=option<std::uint64_t>(socket.n,SOL_SOCKET,SO_NETNS_COOKIE,"SO_NETNS_COOKIE");
#else
    s.network_namespace_cookie=absent<std::uint64_t>("SO_NETNS_COOKIE absent in build headers; socket cookie still identifies pinned object");
#endif
    tcp_info info{};socklen_t size=sizeof(info);
    if(getsockopt(socket.n,IPPROTO_TCP,TCP_INFO,&info,&size)<0)return failure<LinuxTcpHeldSocketSample>("TCP_INFO on pinned socket",errno);
    if(size>sizeof(info)||size<1)return absent<LinuxTcpHeldSocketSample>("Invalid TCP_INFO length");
    s.state=info.tcpi_state;s.captured_at=std::chrono::steady_clock::now();
    if(s.state!=1&&s.state!=8)return absent<LinuxTcpHeldSocketSample>("Pinned socket outside supported TCP ESTABLISHED/CLOSE_WAIT scope");
    s.acknowledged_bytes=size>=offsetof(tcp_info,tcpi_bytes_acked)+sizeof(info.tcpi_bytes_acked)?Metric<std::uint64_t>{info.tcpi_bytes_acked,Support::available,"Native pinned-socket TCP_INFO tcpi_bytes_acked"}:absent<std::uint64_t>("TCP_INFO too short for tcpi_bytes_acked");
    s.received_bytes=size>=offsetof(tcp_info,tcpi_bytes_received)+sizeof(info.tcpi_bytes_received)?Metric<std::uint64_t>{info.tcpi_bytes_received,Support::available,"Native pinned-socket TCP_INFO tcpi_bytes_received"}:absent<std::uint64_t>("TCP_INFO too short for tcpi_bytes_received");
    // Re-duplicate the target's numeric descriptor, not a /proc symlink: do not
    // report endpoint holder association if its slot now names another socket.
    const Fd final_socket{duplicate(process.n,h.descriptor,self)};
    if(final_socket.n<0)return failure<LinuxTcpHeldSocketSample>("recheck selected descriptor",errno);
    const auto final_cookie=option<std::uint64_t>(final_socket.n,SOL_SOCKET,SO_COOKIE,"final SO_COOKIE");
    if(!metric_is_known(final_cookie))return absent<LinuxTcpHeldSocketSample>(final_cookie.note,final_cookie.support);
    struct stat final_stat{};if(fstat(final_socket.n,&final_stat)<0)return failure<LinuxTcpHeldSocketSample>("recheck pinned socket inode",errno);
    if(cookie.value!=final_cookie.value||st.st_ino!=final_stat.st_ino||st.st_dev!=final_stat.st_dev)
        return absent<LinuxTcpHeldSocketSample>("Holder descriptor changed socket at observation endpoints");
    const auto after=read_linux_process_identity(path);
    if(!same_process(after,h))return absent<LinuxTcpHeldSocketSample>("Holder PID/start_ticks absent or changed after socket read",after.support==Support::permission_required?Support::permission_required:Support::unsupported);
    if(!self){const auto live=alive(process.n);if(!metric_is_known(live))return absent<LinuxTcpHeldSocketSample>(live.note,live.support);}
    return {s,Support::estimated,"Selected descriptor held the same pinned TCP cookie at observation endpoints; no exclusive/causal process attribution"};
}
}
#endif
}
namespace pcinsight {
Metric<LinuxTcpHeldSocketSample> read_linux_tcp_socket_holder(const LinuxTcpSocketHolder& target){
    if(!detail::valid(target))return {{},Support::unsupported,"Invalid selected holder PID/start_ticks/descriptor"};
#ifdef __linux__
    try{return detail::native_holder(target);}
    catch(const std::exception& e){return {{},Support::unsupported,e.what()};}
#else
    return {{},Support::unsupported,"Pinned TCP socket-holder query requires Linux"};
#endif
}
Metric<LinuxTcpHeldSocketSample> LinuxTcpSocketHolderSampler::sample(const LinuxTcpSocketHolder& target){
    try{return detail::update_linux_tcp_holder(previous_,read_linux_tcp_socket_holder(target));}
    catch(...){reset();throw;}
}
}
