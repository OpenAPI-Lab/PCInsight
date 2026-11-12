// Manual expanded-scope test. NEVER register with CTest or run on private-only consent.
#include <pcinsight/monitor.hpp>
#include <algorithm>
#include <array>
#include <chrono>
#include <iostream>
#include <stdexcept>
#include <string_view>
#include <dirent.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
using namespace pcinsight;
namespace {
void require(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
std::size_t descriptors(){auto d=::opendir("/proc/self/fd");require(d,"FD census");std::size_t n=0;while(auto e=::readdir(d))if(e->d_name[0]!='.')++n;::closedir(d);return n;}
struct Socket {int fd=-1;~Socket(){if(fd>=0)::close(fd);}};
SampleOptions options(){SampleOptions o;o.include_processes=o.include_disks=o.include_networks=o.include_gpus=false;o.include_linux_process_descriptions=false;o.include_linux_process_network=true;return o;}
int run(){
    // Leave headroom within the human's 30s budget for short diagnostic attempts.
    ::alarm(20);const auto begin=std::chrono::steady_clock::now();const auto base=descriptors();
    int result=0;
    {
        Socket socket;socket.fd=::socket(AF_INET,SOCK_DGRAM|SOCK_CLOEXEC,0);require(socket.fd>=0,"private UDP socket");
        timeval timeout{1,0};require(::setsockopt(socket.fd,SOL_SOCKET,SO_RCVTIMEO,&timeout,sizeof(timeout))==0,"private receive timeout");
        sockaddr_in a{};a.sin_family=AF_INET;a.sin_addr.s_addr=htonl(INADDR_LOOPBACK);socklen_t len=sizeof(a);
        require(::bind(socket.fd,reinterpret_cast<sockaddr*>(&a),len)==0&&::getsockname(socket.fd,reinterpret_cast<sockaddr*>(&a),&len)==0,"private UDP bind");
        std::uint64_t cookie=0;len=sizeof(cookie);require(::getsockopt(socket.fd,SOL_SOCKET,SO_NETNS_COOKIE,&cookie,&len)==0&&cookie,"private cookie oracle");
        auto go=[&]{std::array<char,97> data{};require(::sendto(socket.fd,data.data(),data.size(),0,reinterpret_cast<sockaddr*>(&a),sizeof(a))==97&&::recv(socket.fd,data.data(),data.size(),0)==97,"private UDP oracle");};
        Monitor m;const auto idle=descriptors();
        auto change=m.refresh_linux_visible_process_network(true); // Real native /proc, no injection.
        if(!change.value){
            std::cout<<"strict absent support="<<static_cast<int>(change.support)<<" note="<<change.note<<'\n';
            // Partial is explicit, never a hidden retry/filter of runtime failures.
            change=m.refresh_linux_visible_process_network(true,LinuxNetworkAdmissionPolicy::verified_only);
        }
        if(!change.value){
            std::cout<<"BLOCKED real visible admission support="<<static_cast<int>(change.support)<<" note="<<change.note<<'\n';result=2;
        }else{
            std::cout<<"admitted visible="<<change.value->visible_processes<<" verified="<<change.value->verified_processes<<" excluded="<<change.value->excluded.size()<<" policy="<<static_cast<int>(change.value->policy)<<'\n';
            const auto epoch=change.value->selection_epoch;
            require(change.value->capture_active,"real view has no eligible actors");
            auto take=[&]{auto s=m.sample(options());
                if(!s.linux_process_network.value)throw std::runtime_error("Real view sample absent: "+s.linux_process_network.note);
                require(s.linux_network_admission.value&&s.linux_network_admission.value->selection_epoch==epoch&&s.linux_process_network.value->selection_epoch==epoch,"real view metadata/epoch pairing");
                require(s.linux_process_network.value->observed_processes.size()==change.value->verified_processes,"real view admitted cardinality");
                return std::move(*s.linux_process_network.value);};
            const auto pid=static_cast<std::uint32_t>(::getpid());
            auto row=[&](const LinuxProcessNetworkSnapshot& s)->const LinuxProcessNetworkSample&{
                auto at=std::find_if(s.processes.begin(),s.processes.end(),[&](const auto& r){return r.process.pid==pid&&r.kernel.socket_network_namespace_cookie==cookie;});
                require(at!=s.processes.end(),"caller UDP row absent from real visible capture");return *at;};
            auto empty=take();require(std::any_of(empty.observed_processes.begin(),empty.observed_processes.end(),[&](const auto& p){return p.process.pid==pid;}),"caller excluded unexpectedly");
            go();auto first=take();require(row(first).payload_bytes_since_start.value.has_value(),"private counters absent");
            const auto previous=*row(first).payload_bytes_since_start.value;
            go();auto second=take();const auto& r=row(second);const auto dt=second.counter_sweep_finished_ns-first.counter_sweep_finished_ns;
            require(dt&&r.nominal_interval_ns.value==dt&&r.payload_bytes_since_start.value,"real visible nominal interval");
            for(unsigned lane=2;lane<4;++lane)require(r.payload_bytes_since_start.value->at(lane)-previous[lane]==97&&r.payload_bytes_per_second[lane].value==97'000'000'000ULL/dt,"real visible private payload/rate oracle");
            std::cout<<"PASS real /proc enumeration -> public Monitor admission -> three paired snapshots, private UDP delta97 in send/receive and integer rate oracle; no injected enumeration\n";
        }
        m.stop_linux_process_network();require(descriptors()==idle,"real visible capture FD leak");
        auto off=m.sample(options());require(!off.linux_process_network.value&&!off.linux_network_admission.value,"stop/sample rearmed real capture");
        std::cout<<"capture stopped, FD census restored, sample does not rearm\n";
    }
    require(descriptors()==base,"real visible harness FD leak");::alarm(0);
    std::cout<<"elapsed_ms="<<std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now()-begin).count()<<"; no payload/arguments/environment recorded; no persistent changes\n";
    return result;
}
}
int main(int argc,char** argv){
    if(argc!=2||std::string_view(argv[1])!="--authorized-visible-wsl-30s"){std::cerr<<"NO capture: expanded-scope human consent and explicit flag required\n";return 77;}
    try{return run();}catch(const std::exception& e){std::cerr<<"FAIL real visible validation: "<<e.what()<<"; RAII closes captures/sockets\n";return 1;}
}
