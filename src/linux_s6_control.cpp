#include "linux_s6_control.hpp"
#include "linux_network_selection.hpp"
#include "linux_s6_inventory.hpp"
#include <mutex>
#ifdef __linux__
#include <cerrno>
#include <fcntl.h>
#include <linux/openat2.h>
#include <pthread.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <unistd.h>
#endif
namespace pcinsight {
namespace {
template<class T>Metric<T> absent(std::string note,Support s=Support::unsupported){return {{},s,std::move(note)};}
#ifdef __linux__
struct Fd{int n=-1;explicit Fd(int value=-1):n(value){}~Fd(){if(n>=0)::close(n);}Fd(const Fd&)=delete;};
int directory(int base,const std::string& path){open_how h{};h.flags=O_PATH|O_DIRECTORY|O_CLOEXEC;h.resolve=RESOLVE_NO_MAGICLINKS;return static_cast<int>(::syscall(SYS_openat2,base,path.c_str(),&h,sizeof h));}
bool same(int a,int b){struct stat x{},y{};return ::fstat(a,&x)==0&&::fstat(b,&y)==0&&x.st_dev==y.st_dev&&x.st_ino==y.st_ino;}
using Context=detail::NetworkSelectionContext;
Metric<Context> context(){const auto a=detail::native_network_selection_source_api();return a.observe(a.context);}
bool exact(const Metric<Context>& c){return c.value&&c.support==Support::available;}
Support access_support(){return errno==EACCES||errno==EPERM?Support::permission_required:Support::unsupported;}
struct SigpipeGuard {
    sigset_t old{},one{};bool active{},pending{};
    bool block(){::sigemptyset(&one);::sigaddset(&one,SIGPIPE);if(::pthread_sigmask(SIG_BLOCK,&one,&old)!=0)return false;active=true;sigset_t p{};if(::sigpending(&p)!=0)return false;pending=::sigismember(&p,SIGPIPE)==1;return true;}
    void consume(){if(!pending){timespec zero{};while(::sigtimedwait(&one,nullptr,&zero)<0&&errno==EINTR){}}}
    ~SigpipeGuard(){if(active)::pthread_sigmask(SIG_SETMASK,&old,nullptr);}
};
#endif
}
struct LinuxS6Control::Impl {
    std::mutex mutex;
#ifdef __linux__
    Fd service,supervise,control;Context owner;std::uint32_t euid{};
    bool valid_owner()const{const auto c=context();return exact(c)&&*c.value==owner&&euid==static_cast<std::uint32_t>(::geteuid());}
    bool selected()const{Fd s(directory(service.n,"supervise"));Fd f(::openat(supervise.n,"control",O_PATH|O_NOFOLLOW|O_CLOEXEC));return s.n>=0&&f.n>=0&&same(s.n,supervise.n)&&same(f.n,control.n);}
#endif
};
LinuxS6Control::LinuxS6Control(std::unique_ptr<Impl> p):impl_(std::move(p)){}
LinuxS6Control::~LinuxS6Control()=default;
Metric<std::shared_ptr<LinuxS6Control>> LinuxS6Control::open(const std::string& root){
    if(root.empty()||root.front()!='/'||root.size()>32768||root.find('\0')!=root.npos)return absent<std::shared_ptr<LinuxS6Control>>("Absolute explicit s6 service directory required");
#ifdef __linux__
    const auto c=context();if(!exact(c))return absent<std::shared_ptr<LinuxS6Control>>(c.note,c.support);
    auto p=std::make_unique<Impl>();p->owner=*c.value;p->euid=static_cast<std::uint32_t>(::geteuid());
    p->service.n=directory(AT_FDCWD,root);if(p->service.n<0)return absent<std::shared_ptr<LinuxS6Control>>("Cannot pin s6 service directory (openat2 required)",access_support());
    p->supervise.n=directory(p->service.n,"supervise");if(p->supervise.n<0)return absent<std::shared_ptr<LinuxS6Control>>("Cannot pin s6 supervise directory",access_support());
    p->control.n=::openat(p->supervise.n,"control",O_PATH|O_NOFOLLOW|O_CLOEXEC);struct stat st{};
    if(p->control.n<0)return absent<std::shared_ptr<LinuxS6Control>>("Cannot pin s6 control node",access_support());
    if(::fstat(p->control.n,&st)!=0||!S_ISFIFO(st.st_mode))return absent<std::shared_ptr<LinuxS6Control>>("s6 control must be a real FIFO, not a link/file/device");
    if(!p->valid_owner()||!p->selected())return absent<std::shared_ptr<LinuxS6Control>>("s6 control owner/selection changed during open");
    return {std::shared_ptr<LinuxS6Control>(new LinuxS6Control(std::move(p))),Support::available,"Pinned caller-trusted s6 FIFO; NOT supervisor identity/version/liveness proof"};
#else
    return absent<std::shared_ptr<LinuxS6Control>>("s6 control requires Linux");
#endif
}
LinuxS6RequestResult LinuxS6Control::request(ServiceAction action,bool authorized){
    if(!authorized)return {ActionError::permission_denied,0,0,{},"Explicit s6 control authorization required"};
    std::string_view command;switch(action){case ServiceAction::start:command="u";break;case ServiceAction::stop:command="d";break;case ServiceAction::restart:command="cru";break;default:return {ActionError::invalid_argument,0,0,{},"Invalid s6 action"};}
    std::lock_guard lock(impl_->mutex);
#ifdef __linux__
    if(!impl_->valid_owner()||!impl_->selected())return {ActionError::stale_target,0,0,{},"s6 caller/endpoints changed; nothing written"};
    // Only reopen the held type-checked FIFO. Path replacement cannot open a device.
    Fd fd(::open(("/proc/self/fd/"+std::to_string(impl_->control.n)).c_str(),O_WRONLY|O_NONBLOCK|O_CLOEXEC));
    if(fd.n<0){const auto e=errno;return {e==EACCES||e==EPERM?ActionError::permission_denied:ActionError::not_supported,0,0,{},"s6 FIFO unavailable/no reader; nothing written"};}
    if(!same(fd.n,impl_->control.n)||!impl_->selected()||!impl_->valid_owner())return {ActionError::stale_target,0,0,{},"s6 endpoint/context replaced before delivery; nothing written"};
    return detail::write_s6_control_fd(fd.n,command);
#else
    return {ActionError::not_supported,0,0,{},"s6 control requires Linux"};
#endif
}
Metric<LinuxS6StatusRecord> LinuxS6Control::read_record()const{
    std::lock_guard lock(impl_->mutex);
#ifdef __linux__
    if(!impl_->valid_owner()||!impl_->selected())return absent<LinuxS6StatusRecord>("s6 pinned control target/context changed before record read");
    auto record=detail::read_linux_s6_record_at(impl_->service.n,".");
    if(!impl_->selected()||!impl_->valid_owner())return absent<LinuxS6StatusRecord>("s6 pinned control target/context changed during record read");
    return record;
#else
    return absent<LinuxS6StatusRecord>("s6 pinned record observation requires Linux");
#endif
}
}
namespace pcinsight::detail {
LinuxS6RequestResult write_s6_control_fd(int fd,std::string_view command){
    if(command!="u"&&command!="d"&&command!="cru")return {ActionError::invalid_argument,0,0,{},"Invalid s6 control command"};
#ifdef __linux__
    struct stat st{};const auto flags=::fcntl(fd,F_GETFL);
    if(::fstat(fd,&st)!=0||!S_ISFIFO(st.st_mode)||flags<0||!(flags&O_NONBLOCK)||(flags&O_ACCMODE)!=O_WRONLY)return {ActionError::invalid_argument,0,0,{},"Nonblocking WRONLY FIFO descriptor required"};
    // Prepare ALL strings before possible delivery. No throwing work after write.
    LinuxS6RequestResult out{ActionError::none,static_cast<std::uint32_t>(command.size()),0,{{},Support::unsupported,"Completion/readiness unknown; only FIFO delivery measured"},"s6 FIFO bytes delivered, NOT service completion"};
    std::string failed="s6 write failed/partial; retained counts, no positive partial retry";
    SigpipeGuard mask;if(!mask.block())return {ActionError::system_error,0,0,{},"Cannot isolate SIGPIPE in calling thread"};
    ssize_t n;do{n=::write(fd,command.data(),command.size());}while(n<0&&errno==EINTR);
    const auto code=errno;if(n<0&&code==EPIPE)mask.consume();if(n>0)out.written_bytes=static_cast<std::uint32_t>(n);
    if(n!=static_cast<ssize_t>(command.size())){out.error=ActionError::system_error;out.message.swap(failed);}return out;
#else
    (void)fd;return {ActionError::not_supported,0,0,{},"s6 transport requires Linux"};
#endif
}
}
