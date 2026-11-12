#include "pcinsight/linux_runit.hpp"
#include "../src/linux_runit.hpp"
#include <cassert>
#include <iostream>
#ifdef __linux__
#include <cerrno>
#include <filesystem>
#include <fstream>
#include <cstdlib>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#endif
using namespace pcinsight;
std::array<std::uint8_t,20> frame(){std::array<std::uint8_t,20> b{};b[0]=0x40;b[7]=20;b[11]=1;b[12]=0x34;b[13]=0x12;b[17]='u';b[19]=1;return b;}
#ifdef __linux__
namespace fs=std::filesystem;
void write(const fs::path& p,const std::array<std::uint8_t,20>& b){std::ofstream f(p,std::ios::binary);assert(f);f.write(reinterpret_cast<const char*>(b.data()),b.size());f.close();assert(f);}
std::size_t fds(){auto d=::opendir("/proc/self/fd");assert(d);std::size_t n=0;while(auto e=::readdir(d))if(e->d_name[0]!='.')++n;::closedir(d);return n;}
#endif
int main(){
    const auto b=frame();const auto decoded=decode_linux_runit_status(b);assert(decoded.value&&decoded.value->reported_pid==0x1234&&decoded.value->want_up&&!decoded.value->paused&&decoded.value->state==LinuxRunitState::run);
    for(unsigned n=0;n<9;++n){auto bad=b;if(n==0)bad[8]=255;if(n==1)bad[15]=128;if(n==2)bad[16]=2;if(n==3)bad[17]='x';if(n==4)bad[18]=2;if(n==5)bad[19]=3;if(n==6)bad[19]=0;if(n==7)bad[12]=bad[13]=0;if(n==8)bad[8]=0x3b,bad[9]=0x9a,bad[10]=0xca,bad[11]=0;assert(!decode_linux_runit_status(bad).value);}
    auto down=b;down[12]=down[13]=down[19]=0;assert(decode_linux_runit_status(down).value->reported_pid==0);down[19]=2;assert(decode_linux_runit_status(down).value->state==LinuxRunitState::finish); // finish with PID0 is valid runsv transition.
    assert(!decode_linux_runit_status(std::span(b).first(18)).value&&!LinuxRunitService::open("relative").value&&!query_linux_runit_inventory("relative").value&&!query_linux_runit_inventory("/unused",0).value);
#ifdef __linux__
    ::alarm(20);const auto initial=fds();char pattern[]="/tmp/pcinsight-runit-XXXXXX";const auto d=::mkdtemp(pattern);assert(d);const fs::path root=d;struct Cleanup{fs::path p;~Cleanup(){std::error_code e;fs::remove_all(p,e);}} cleanup{root};const auto service=root/"real-service",supervise=service/"supervise",catalog=root/"catalog";
    fs::create_directories(supervise);fs::create_directories(catalog);fs::create_directory_symlink(service,catalog/"alias");fs::create_directories(catalog/"unmanaged");{std::ofstream f(catalog/"README");f<<"not a service";}write(supervise/"status",b);
    assert(::mkfifo((supervise/"ok").c_str(),0600)==0&&::mkfifo((supervise/"control").c_str(),0600)==0);
    int ok=::open((supervise/"ok").c_str(),O_RDONLY|O_NONBLOCK|O_CLOEXEC),control=::open((supervise/"control").c_str(),O_RDONLY|O_NONBLOCK|O_CLOEXEC);assert(ok>=0&&control>=0);
    auto handle=LinuxRunitService::open((catalog/"alias").string());if(!handle.value)std::cerr<<handle.note<<'\n';assert(handle.value&&(*handle.value)->read().value==decoded.value);
    const auto inventory=query_linux_runit_inventory(catalog.string());assert(inventory.value&&inventory.value->complete&&inventory.value->services.size()==2&&inventory.value->services[0].name=="alias"&&inventory.value->services[0].status.value&&!inventory.value->services[0].description.value&&!inventory.value->services[0].group.value&&!inventory.value->services[1].status.value);
    const auto bounded=query_linux_runit_inventory(catalog.string(),1);assert(bounded.value&&!bounded.value->complete&&bounded.support==Support::estimated);
    auto denied=(*handle.value)->request(ServiceAction::restart,false);assert(denied.error==ActionError::permission_denied&&!denied.written_bytes);char out[8]{};assert(::read(control,out,sizeof out)<=0);
    for(const auto action:{ServiceAction::start,ServiceAction::stop,ServiceAction::restart}){const auto result=(*handle.value)->request(action,true);const std::string expected=action==ServiceAction::start?"u":action==ServiceAction::stop?"d":"tcu";assert(result&&result.requested_bytes==expected.size()&&result.written_bytes==expected.size()&&!result.completed.value);const auto n=::read(control,out,sizeof out);assert(n==static_cast<ssize_t>(expected.size())&&std::string(out,static_cast<std::size_t>(n))==expected);}
    // File replacement refuses stale handle without writing to either pipe.
    fs::rename(supervise/"control",supervise/"old-control");assert(::mkfifo((supervise/"control").c_str(),0600)==0);const auto stale=(*handle.value)->request(ServiceAction::start,true);assert(stale.error==ActionError::stale_target&&!stale.written_bytes&&!(*handle.value)->read().value);handle.value.reset();::close(control);
    control=::open((supervise/"control").c_str(),O_RDONLY|O_NONBLOCK|O_CLOEXEC);assert(control>=0);handle=LinuxRunitService::open(service.string());assert(handle.value&&(*handle.value)->read().value);
    ::close(ok);ok=-1;assert(!(*handle.value)->read().value&&!(*handle.value)->request(ServiceAction::start,true)); // stale status file is not running evidence.
    // Deterministic broken-pipe transport: no reader after a writer was opened.
    const int writer=::open((supervise/"control").c_str(),O_WRONLY|O_NONBLOCK|O_CLOEXEC);assert(writer>=0);::close(control);control=-1;
    sigset_t before{},after{},one{};assert(::pthread_sigmask(SIG_SETMASK,nullptr,&before)==0);const auto broken=detail::write_runit_control_fd(writer,"u");assert(!broken&&broken.written_bytes==0&&!broken.completed.value);assert(::pthread_sigmask(SIG_SETMASK,nullptr,&after)==0);for(int s=1;s<NSIG;++s)assert(::sigismember(&before,s)==::sigismember(&after,s));
    ::sigemptyset(&one);::sigaddset(&one,SIGPIPE);assert(::pthread_sigmask(SIG_BLOCK,&one,nullptr)==0&&::raise(SIGPIPE)==0);assert(!detail::write_runit_control_fd(writer,"tcu"));sigset_t pending{};assert(::sigpending(&pending)==0&&::sigismember(&pending,SIGPIPE)==1);timespec zero{};assert(::sigtimedwait(&one,nullptr,&zero)==SIGPIPE&&::pthread_sigmask(SIG_SETMASK,&before,nullptr)==0);::close(writer);
    handle.value.reset();assert(fds()==initial);::alarm(0);std::cout<<"Runit native private protocol: status/alias/inventory/unknown/bound/u-d-tcu/consent/replacedFIFO/stale-file/EPIPE/pendingSIGPIPE/mask/FD cleanup passed; NOT real supervisor/service acceptance\n";
#else
    assert(!LinuxRunitService::open("/unused").value&&!query_linux_runit_inventory("/unused").value);std::cout<<"Runit decoder and non-Linux unsupported contract passed\n";
#endif
}
