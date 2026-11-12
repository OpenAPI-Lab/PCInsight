#include "../src/linux_s6_control.hpp"
#include <cassert>
#include <iostream>
#ifdef __linux__
#include <filesystem>
#include <fstream>
#include <fcntl.h>
#include <dirent.h>
#include <pthread.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <cerrno>
#include <unistd.h>
#include <cstdlib>
#endif
using namespace pcinsight;
#ifdef __linux__
namespace fs=std::filesystem;
std::size_t fds(){auto d=::opendir("/proc/self/fd");assert(d);std::size_t n=0;while(auto e=::readdir(d))if(e->d_name[0]!='.')++n;::closedir(d);return n;}
#endif
int main(){
    assert(!LinuxS6Control::open("relative").value);assert(!LinuxS6Control::open(std::string("/tmp\0x",6)).value);assert(!detail::write_s6_control_fd(-1,"x"));
#ifdef __linux__
    ::alarm(20);const auto initial=fds();char temp[]="/tmp/pcinsight-s6-control-XXXXXX";auto d=::mkdtemp(temp);assert(d);const fs::path root=d,service=root/"service",supervise=service/"supervise",fifo=supervise/"control";struct Cleanup{fs::path p;~Cleanup(){std::error_code e;fs::remove_all(p,e);}} cleanup{root};
    assert(!LinuxS6Control::open(service.string()).value&&!fs::exists(service));fs::create_directories(supervise);assert(!LinuxS6Control::open(service.string()).value&&!fs::exists(fifo));
    {std::ofstream file(fifo);file<<"not FIFO";}assert(!LinuxS6Control::open(service.string()).value);fs::remove(fifo);assert(::mkfifo(fifo.c_str(),0600)==0);
    auto opened=LinuxS6Control::open(service.string());assert(opened.value);auto handle=*opened.value;auto denied=handle->request(ServiceAction::start,false);assert(denied.error==ActionError::permission_denied&&denied.written_bytes==0);assert(!handle->request(ServiceAction::start,true));
    int reader=::open(fifo.c_str(),O_RDONLY|O_NONBLOCK|O_CLOEXEC);assert(reader>=0);
    assert(!handle->request(static_cast<ServiceAction>(99),true));
    for(const auto action:{ServiceAction::start,ServiceAction::stop,ServiceAction::restart}){auto result=handle->request(action,true);const std::string expected=action==ServiceAction::start?"u":action==ServiceAction::stop?"d":"cru";assert(result&&result.requested_bytes==expected.size()&&result.written_bytes==expected.size()&&!result.completed.value);char b[8]{};auto n=::read(reader,b,sizeof b);assert(n==static_cast<ssize_t>(expected.size())&&std::string(b,static_cast<std::size_t>(n))==expected);}
    const auto child=::fork();assert(child>=0);if(!child){auto r=handle->request(ServiceAction::start,true);::_exit(r.error==ActionError::stale_target&&!r.written_bytes?0:2);}int status{};assert(::waitpid(child,&status,0)==child&&WIFEXITED(status)&&WEXITSTATUS(status)==0);
    int filler=::open(fifo.c_str(),O_WRONLY|O_NONBLOCK|O_CLOEXEC);assert(filler>=0);std::array<char,4096> fill{};fill.fill('x');std::size_t filled=0;for(;;){auto n=::write(filler,fill.data(),fill.size());if(n<0){assert(errno==EAGAIN||errno==EWOULDBLOCK);break;}assert(n>0);filled+=static_cast<std::size_t>(n);assert(filled<=8*1024*1024);}
    auto blocked=handle->request(ServiceAction::restart,true);assert(!blocked&&blocked.requested_bytes==3&&blocked.written_bytes==0&&!blocked.completed.value);::close(filler);std::size_t drained=0;for(;;){auto n=::read(reader,fill.data(),fill.size());assert(n>=0);if(!n)break;for(ssize_t i=0;i<n;++i)assert(fill[static_cast<std::size_t>(i)]=='x');drained+=static_cast<std::size_t>(n);}assert(drained==filled);
    assert(!fs::exists(service/"down")&&!fs::exists(supervise/"status")&&!fs::exists(supervise/"lock"));
    fs::rename(fifo,supervise/"old");assert(::mkfifo(fifo.c_str(),0600)==0);auto stale=handle->request(ServiceAction::start,true);assert(stale.error==ActionError::stale_target&&stale.written_bytes==0);char byte{};assert(::read(reader,&byte,1)<=0);::close(reader);opened.value.reset();handle.reset();
    fs::remove(fifo);fs::create_symlink("old",fifo);assert(!LinuxS6Control::open(service.string()).value);fs::remove(fifo);assert(::mkfifo(fifo.c_str(),0600)==0);
    reader=::open(fifo.c_str(),O_RDONLY|O_NONBLOCK|O_CLOEXEC);assert(reader>=0);int writer=::open(fifo.c_str(),O_WRONLY|O_NONBLOCK|O_CLOEXEC);assert(writer>=0);::close(reader);
    sigset_t before{},after{},pending{},one{},saved{};assert(::pthread_sigmask(SIG_SETMASK,nullptr,&before)==0);auto broken=detail::write_s6_control_fd(writer,"u");assert(!broken&&broken.requested_bytes==1&&broken.written_bytes==0&&!broken.completed.value);assert(::pthread_sigmask(SIG_SETMASK,nullptr,&after)==0);assert(::sigismember(&before,SIGPIPE)==::sigismember(&after,SIGPIPE));assert(::sigpending(&pending)==0&&::sigismember(&pending,SIGPIPE)==0);
    ::sigemptyset(&one);::sigaddset(&one,SIGPIPE);assert(::pthread_sigmask(SIG_BLOCK,&one,&saved)==0);assert(::raise(SIGPIPE)==0);broken=detail::write_s6_control_fd(writer,"cru");assert(!broken&&broken.requested_bytes==3&&!broken.written_bytes);assert(::sigpending(&pending)==0&&::sigismember(&pending,SIGPIPE)==1);timespec zero{};assert(::sigtimedwait(&one,nullptr,&zero)==SIGPIPE);assert(::pthread_sigmask(SIG_SETMASK,&saved,nullptr)==0);::close(writer);
    int readwrite=::open(fifo.c_str(),O_RDWR|O_NONBLOCK|O_CLOEXEC);assert(readwrite>=0);assert(detail::write_s6_control_fd(readwrite,"u").error==ActionError::invalid_argument);::close(readwrite);
    fs::remove_all(supervise);fs::create_directory(supervise);assert(::mkfifo(fifo.c_str(),0600)==0);opened=LinuxS6Control::open(service.string());assert(opened.value);fs::rename(supervise,service/"old-supervise");fs::create_directory(supervise);assert(::mkfifo(fifo.c_str(),0600)==0);assert((*opened.value)->request(ServiceAction::stop,true).error==ActionError::stale_target);opened.value.reset();
    fs::create_directory_symlink(service,root/"alias");opened=LinuxS6Control::open((root/"alias").string());assert(opened.value);opened.value.reset();assert(fds()==initial);::alarm(0);
#else
    assert(!LinuxS6Control::open("/service/example").value);assert(!detail::write_s6_control_fd(-1,"u"));
#endif
    std::cout<<"s6 pinned FIFO consent/u-d-cru/delivery-only/replacement/EPIPE/pending-SIGPIPE/mask/FD checks passed; NOT real s6 acceptance\n";
}
