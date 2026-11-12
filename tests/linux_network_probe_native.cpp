#include "../src/linux_network_probe.hpp"
#include "../src/linux_network_btf.hpp"
#include <array>
#include <cerrno>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string_view>
#include <atomic>
#include <thread>
#include <limits>
#include <linux/bpf.h>
#include <sys/syscall.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <unistd.h>
using namespace pcinsight;using namespace pcinsight::detail;
namespace {
bool classified_mode=false;
bool quality_mode=false;
PrivateNetworkIdentity capture_identity;
struct Fd{
    int value{-1};explicit Fd(int fd=-1):value(fd){}~Fd(){if(value>=0)::close(value);}
    Fd(const Fd&)=delete;Fd& operator=(const Fd&)=delete;
    void close(){if(value>=0){::close(value);value=-1;}}
};
struct Unavailable:std::runtime_error{using std::runtime_error::runtime_error;};
int bpf(enum bpf_cmd cmd,union bpf_attr& attr){return static_cast<int>(::syscall(SYS_bpf,cmd,&attr,sizeof(attr)));}
void require(bool ok,const char* note){if(!ok)throw std::runtime_error(note);}
std::uint64_t pointer(const void* p){return reinterpret_cast<std::uintptr_t>(p);}
int make_map(unsigned entries=6){
    union bpf_attr a{};a.map_type=BPF_MAP_TYPE_ARRAY;a.key_size=4;a.value_size=8;a.max_entries=entries;
    const auto fd=bpf(BPF_MAP_CREATE,a);
    if(fd<0)throw Unavailable(std::string("BPF_MAP_CREATE: ")+std::strerror(errno));
    return fd;
}
std::uint64_t map_value(int map,std::uint32_t key){
    std::uint64_t value=0;union bpf_attr a{};a.map_fd=map;a.key=pointer(&key);a.value=pointer(&value);
    require(bpf(BPF_MAP_LOOKUP_ELEM,a)==0,"map read failed");return value;
}
void set_map_value(int map,std::uint32_t key,std::uint64_t value){
    union bpf_attr a{};a.map_fd=map;a.key=pointer(&key);a.value=pointer(&value);a.flags=BPF_ANY;
    require(bpf(BPF_MAP_UPDATE_ELEM,a)==0,"private fixture map update failed");
}
PrivateNetworkIdentity kernel_identity(const LinuxNetworkBtfLayout& layout){
    Fd map(make_map(3));auto generated=make_private_network_identity_probe(map.value,layout);
    require(metric_is_known(generated),"identity generator rejected layout");
    std::array<char,65536> log{};const char license[]="GPL";union bpf_attr a{};
    a.prog_type=BPF_PROG_TYPE_RAW_TRACEPOINT;a.insn_cnt=generated.value->size();
    a.insns=pointer(generated.value->data());a.license=pointer(license);
    a.log_buf=pointer(log.data());a.log_size=log.size();a.log_level=1;
    Fd prog(bpf(BPF_PROG_LOAD,a));
    if(prog.value<0)throw Unavailable(std::string("identity load: ")+std::strerror(errno)+"\n"+std::string(log.data(),::strnlen(log.data(),log.size())));
    a={};a.test.prog_fd=prog.value;
    if(bpf(BPF_PROG_TEST_RUN,a)<0||a.test.retval)throw Unavailable("identity TEST_RUN/read failed, no attach");
    const auto pid=map_value(map.value,0),uid=map_value(map.value,1),start=map_value(map.value,2);
    if(!pid||pid>2147483647||uid>std::numeric_limits<std::uint32_t>::max()||!start)
        throw Unavailable("invalid caller identity output, no attach");
    return {static_cast<std::uint32_t>(pid),static_cast<std::uint32_t>(uid),start};
}
std::uint32_t kernel_tgid(){
    // Execute an UNATTACHED raw-tracepoint program in this BPF_PROG_TEST_RUN
    // caller, with no data/context or remote-CPU flag. Do not assume that
    // getpid() and helper14 use the same PID namespace. No global hook here.
    const std::array<NetworkProbeInstruction,3> code{{{0x85,0,0,14},{0x77,0,0,32},{0x95,0,0,0}}};
    const char license[]="GPL";union bpf_attr a{};std::array<char,65536> log{};
    a.prog_type=BPF_PROG_TYPE_RAW_TRACEPOINT;a.insn_cnt=code.size();
    a.insns=pointer(code.data());a.license=pointer(license);
    a.log_buf=pointer(log.data());a.log_size=log.size();a.log_level=1;
    Fd prog(bpf(BPF_PROG_LOAD,a));
    if(prog.value<0)throw Unavailable(std::string("unattached identity program: ")+std::strerror(errno)+"\n"+std::string(log.data(),::strnlen(log.data(),log.size())));
    a={};a.test.prog_fd=prog.value; // No flags/cpu/repeat: runs in this caller.
    if(bpf(BPF_PROG_TEST_RUN,a)<0)throw Unavailable(std::string("identity test-run: ")+std::strerror(errno));
    if(!a.test.retval||a.test.retval>2147483647U)throw Unavailable("identity test-run did not return valid kernel TGID");
    return a.test.retval;
}
int load(int map,std::uint32_t id,bool receive,std::uint32_t actor,const LinuxNetworkBtfLayout& layout){
    auto generated=classified_mode?make_private_classified_network_probe(actor,map,receive,layout,quality_mode?&capture_identity:nullptr):make_private_network_probe(actor,map,receive);
    require(metric_is_known(generated),"generator rejected native test");
    static_assert(sizeof(bpf_insn)==sizeof(NetworkProbeInstruction));
    std::array<char,65536> log{};const char license[]="GPL";
    union bpf_attr a{};a.prog_type=BPF_PROG_TYPE_TRACING;a.expected_attach_type=BPF_TRACE_FEXIT;
    a.attach_btf_id=id;a.insn_cnt=static_cast<__u32>(generated.value->size());
    a.insns=pointer(generated.value->data());a.license=pointer(license);
    a.log_buf=pointer(log.data());a.log_size=log.size();a.log_level=1;
    const auto fd=bpf(BPF_PROG_LOAD,a);
    if(fd<0){const auto err=errno;throw Unavailable(std::string("BPF_PROG_LOAD: ")+std::strerror(err)+"\n"+std::string(log.data(),::strnlen(log.data(),log.size())));}
    return fd;
}
int attach(int prog){
    union bpf_attr a{};a.link_create.prog_fd=static_cast<__u32>(prog);a.link_create.attach_type=BPF_TRACE_FEXIT;
    const auto fd=bpf(BPF_LINK_CREATE,a);
    if(fd<0)throw Unavailable(std::string("BPF_LINK_CREATE: ")+std::strerror(errno));
    return fd;
}
Metric<PrivateNetworkQualityCounters> quality_counters(int map){
    Metric<PrivateNetworkQualityCounters> result;
    for(unsigned attempt=0;attempt<8;++attempt){
        PrivateNetworkQualityRead r;r.active_before=map_value(map,7);
        std::atomic_thread_fence(std::memory_order_seq_cst);
        r.sequence_before=map_value(map,8);
        std::atomic_thread_fence(std::memory_order_seq_cst);
        for(std::uint32_t i=0;i<7;++i)r.values[i]=map_value(map,i);
        std::atomic_thread_fence(std::memory_order_seq_cst);
        r.sequence_after=map_value(map,8);
        std::atomic_thread_fence(std::memory_order_seq_cst);
        r.active_after=map_value(map,7);
        result=validate_private_network_quality_read(r);
        if(metric_is_known(result)||(r.active_before==0&&r.active_after==0&&r.sequence_before==r.sequence_after))return result;
    }
    return result; // bounded retries, no fabricated zero while busy
}
std::array<std::uint64_t,6> counters(int map){
    std::array<std::uint64_t,6> out{};
    if(quality_mode){const auto q=quality_counters(map);require(metric_is_known(q),q.note.c_str());
        for(unsigned i=0;i<4;++i)out[i]=q.value->bytes[i];out[5]=q.value->excluded_positive_receives;return out;}
    for(std::uint32_t key=0;key<6;++key){union bpf_attr a{};a.map_fd=map;a.key=pointer(&key);a.value=pointer(&out[key]);
        require(bpf(BPF_MAP_LOOKUP_ELEM,a)==0,"map read failed");}
    return out;
}
void configure(int fd){require(fd>=0,"socket failed");timeval timeout{2,0};
    require(::setsockopt(fd,SOL_SOCKET,SO_RCVTIMEO,&timeout,sizeof(timeout))==0,"receive timeout");
    require(::setsockopt(fd,SOL_SOCKET,SO_SNDTIMEO,&timeout,sizeof(timeout))==0,"send timeout");}
struct Endpoint{sockaddr_storage address{};socklen_t size{};};
Endpoint bind_loopback(int fd,int family){Endpoint result;
    if(family==AF_INET){sockaddr_in a{};a.sin_family=AF_INET;a.sin_addr.s_addr=htonl(INADDR_LOOPBACK);
        std::memcpy(&result.address,&a,sizeof(a));result.size=sizeof(a);}
    else{sockaddr_in6 a{};a.sin6_family=AF_INET6;a.sin6_addr=in6addr_loopback;
        std::memcpy(&result.address,&a,sizeof(a));result.size=sizeof(a);}
    require(::bind(fd,reinterpret_cast<sockaddr*>(&result.address),result.size)==0,"loopback bind");
    require(::getsockname(fd,reinterpret_cast<sockaddr*>(&result.address),&result.size)==0,"getsockname");return result;}
void traffic(int monitored_map=-1,int family=AF_INET,bool message_api=false){
    auto expected=monitored_map>=0?counters(monitored_map):std::array<std::uint64_t,6>{};
    std::array<char,1024> sent{},got{};sent.fill('x');
    Fd listen(::socket(family,SOCK_STREAM|SOCK_CLOEXEC,0));configure(listen.value);auto address=bind_loopback(listen.value,family);
    require(::listen(listen.value,1)==0,"listen");Fd client(::socket(family,SOCK_STREAM|SOCK_CLOEXEC,0));configure(client.value);
    require(::connect(client.value,reinterpret_cast<sockaddr*>(&address.address),address.size)==0,"connect");
    Fd server(::accept4(listen.value,nullptr,nullptr,SOCK_CLOEXEC));configure(server.value);
    if(message_api){iovec data{sent.data(),sent.size()};msghdr message{};message.msg_iov=&data;message.msg_iovlen=1;
        require(::sendmsg(client.value,&message,MSG_NOSIGNAL)==1024,"private TCP sendmsg");}
    else require(::send(client.value,sent.data(),sent.size(),MSG_NOSIGNAL)==1024,"private TCP send");
    if(classified_mode){require(::recv(server.value,got.data(),1,MSG_PEEK)==1,"TCP private peek");++expected[5];}
    std::size_t received=0;while(received<got.size()){
        const auto amount=::recv(server.value,got.data()+received,got.size()-received,0);
        require(amount>0,"private TCP recv");received+=static_cast<std::size_t>(amount);
    }
    require(sent==got,"TCP payload mismatch");
    expected[0]+=1024;expected[1]+=1024;
    if(monitored_map>=0)require(counters(monitored_map)==expected,"TCP phase counters mismatch");
    Fd receiver(::socket(family,SOCK_DGRAM|SOCK_CLOEXEC,0));configure(receiver.value);address=bind_loopback(receiver.value,family);
    Fd sender(::socket(family,SOCK_DGRAM|SOCK_CLOEXEC,0));configure(sender.value);
    if(message_api){iovec data{sent.data(),64};msghdr message{};message.msg_name=&address.address;message.msg_namelen=address.size;
        message.msg_iov=&data;message.msg_iovlen=1;
        require(::sendmsg(sender.value,&message,MSG_NOSIGNAL)==64,"private UDP sendmsg");}
    else require(::sendto(sender.value,sent.data(),64,MSG_NOSIGNAL,reinterpret_cast<sockaddr*>(&address.address),address.size)==64,"private UDP send");
    if(classified_mode){require(::recv(receiver.value,got.data(),1,MSG_PEEK)==1,"UDP private peek");++expected[5];}
    require(::recv(receiver.value,got.data(),got.size(),0)==64,"private UDP receive");
    expected[classified_mode?2:0]+=64;expected[classified_mode?3:1]+=64;
    if(monitored_map>=0)require(counters(monitored_map)==expected,"UDP phase counters mismatch");
}
}
int main(int argc,char** argv){
    if(argc!=2||(std::string_view(argv[1])!="--authorized-private"&&std::string_view(argv[1])!="--authorized-classified"&&std::string_view(argv[1])!="--authorized-quality")){
        std::cerr<<"NO probe loaded: requires an explicit authorized mode after human authorization\n";return 77;
    }
    quality_mode=std::string_view(argv[1])=="--authorized-quality";
    classified_mode=quality_mode||std::string_view(argv[1])=="--authorized-classified";
    try{
        const auto layout=read_linux_network_btf();
        if(!metric_is_known(layout))throw Unavailable("BTF: "+layout.note);
        if(classified_mode&&(!layout.value->inet_send_function_id||!layout.value->inet6_send_function_id))
            throw Unavailable("classified IPv4/IPv6 hooks absent in BTF");
        if(quality_mode)capture_identity=kernel_identity(*layout.value);
        const auto actor=quality_mode?capture_identity.kernel_tgid:kernel_tgid();
        Fd map(make_map(quality_mode?9:6));
        Fd send(load(map.value,classified_mode?layout.value->inet_send_function_id:layout.value->sendto_function_id,false,actor,*layout.value));
        Fd receive(load(map.value,layout.value->receive_function_id,true,actor,*layout.value));
        Fd send6(classified_mode?load(map.value,layout.value->inet6_send_function_id,false,actor,*layout.value):-1);
        Fd send_link(attach(send.value));Fd receive_link(attach(receive.value));
        Fd send6_link(classified_mode?attach(send6.value):-1);
        require(counters(map.value)==std::array<std::uint64_t,6>{},"unexpected initial counters");
        if(classified_mode)traffic(map.value,AF_INET6);
        if(classified_mode){traffic(map.value,AF_INET,true);traffic(map.value,AF_INET6,true);}
        traffic(map.value);auto measured=counters(map.value);
        std::cerr<<"Observed filtered returns: visiblePID="<<::getpid()<<" kernelTGID="<<actor
            <<" first-send="<<measured[0]<<" first-receive="<<measured[1]<<" classified="<<classified_mode<<'\n';
        const std::array<std::uint64_t,6> expected=classified_mode?std::array<std::uint64_t,6>{4096,4096,256,256,0,8}:
            std::array<std::uint64_t,6>{1088,1088,0,0,0,0};
        require(measured==expected,"private TCP/UDP return counters mismatch");
        // A private child is deliberately NOT the allowed TGID. Close inherited
        // BPF FDs in that child, exercise sockets, then verify no added counts.
        const auto child=::fork();require(child>=0,"fork");
        if(child==0){send_link.close();receive_link.close();send6_link.close();send.close();receive.close();send6.close();map.close();
            try{traffic();if(classified_mode){traffic(-1,AF_INET6);traffic(-1,AF_INET,true);traffic(-1,AF_INET6,true);}::_exit(0);}catch(...){::_exit(1);}}
        int status=0;pid_t waited;do{waited=::waitpid(child,&status,0);}while(waited<0&&errno==EINTR);
        require(waited==child&&WIFEXITED(status)&&WEXITSTATUS(status)==0,"private excluded child failed");
        require(counters(map.value)==measured,"TGID filter leaked private child activity");
        if(quality_mode){
            std::atomic<unsigned> done{0},failed{0};
            auto worker=[&]{try{traffic();}catch(...){++failed;}++done;};
            std::jthread one(worker),two(worker);
            unsigned stable=0,unavailable=0;
            for(unsigned tries=0;tries<1000&&done.load()!=2;++tries){
                if(metric_is_known(quality_counters(map.value)))++stable;else ++unavailable;
            }
            one.join();two.join();require(failed==0,"private threaded traffic failed");
            measured=counters(map.value);
            require(measured==std::array<std::uint64_t,6>{6144,6144,384,384,0,12},"leader identity/threaded counters mismatch");
            std::cout<<"QUALITY concurrent snapshots stable="<<stable<<" unavailable="<<unavailable
                <<" completed="<<map_value(map.value,8)<<" writers="<<map_value(map.value,7)<<'\n';
        }
        receive_link.close();send_link.close();send6_link.close(); // No pin/perf/other link reference.
        traffic();if(classified_mode){traffic(-1,AF_INET6);traffic(-1,AF_INET,true);traffic(-1,AF_INET6,true);}
        require(counters(map.value)==measured,"counter changed after detach");
        if(quality_mode){
            const auto good=capture_identity;
            auto negative=[&](PrivateNetworkIdentity identity,int injected){
                capture_identity=identity;Fd test(make_map(9));
                if(injected==0)set_map_value(test.value,0,std::numeric_limits<std::uint64_t>::max()-1);
                if(injected==8)set_map_value(test.value,8,std::numeric_limits<std::uint64_t>::max());
                Fd s(load(test.value,layout.value->inet_send_function_id,false,actor,*layout.value));
                Fd r(load(test.value,layout.value->receive_function_id,true,actor,*layout.value));
                Fd sl(attach(s.value)),rl(attach(r.value));traffic();
                require(!quality_counters(test.value).value,"invalid quality snapshot published");
                require(map_value(test.value,7)==0,"writer guard leaked");
                if(injected<0){for(unsigned i=0;i<4;++i)require(map_value(test.value,i)==0,"mismatched identity got bytes");
                    require(map_value(test.value,4)>0,"identity rejection not reported");}
                else require(map_value(test.value,6)!=0,"overflow not sticky");
            };
            auto bad=good;++bad.leader_start_boottime_ns;negative(bad,-1);
            bad=good;bad.kernel_real_uid^=1;negative(bad,-1);
            negative(good,0);negative(good,8);capture_identity=good;
            std::cout<<"QUALITY identity start_ns="<<good.leader_start_boottime_ns<<" initial_uid="<<good.kernel_real_uid
                <<" wrong-start/UID=rejected; injected-byte/sequence-overflow=unavailable; writers-balanced\n";
        }
        if(classified_mode)std::cout<<"CLASSIFIED PASS IPv4+IPv6 TCP="<<measured[0]<<'/'<<measured[1]
            <<" UDP="<<measured[2]<<'/'<<measured[3]<<" read_failures="<<measured[4]<<" excluded_peeks="<<measured[5]<<'\n';
        std::cout<<"Private fexit probe PASS PID="<<::getpid()<<" kernelTGID="<<actor<<" classified="<<classified_mode
            <<"; excluded-child=unchanged; detached=unchanged. "
            <<"All link/program/map FDs closed on exit; no pin/config changes. NOT production actor accounting.\n";
        return 0;
    }catch(const Unavailable& e){std::cerr<<"UNAVAILABLE (not positive): "<<e.what()<<"; RAII closes owned BPF FDs\n";return 77;}
    catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<"; RAII closes owned BPF FDs\n";return 1;}
}
