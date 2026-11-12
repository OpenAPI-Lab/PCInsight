#include "pcinsight/linux_runit.hpp"
#include "linux_runit.hpp"
#include "linux_network_selection.hpp"
#include <algorithm>
#include <mutex>
#ifdef __linux__
#include <cerrno>
#include <dirent.h>
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
template<class T>Metric<T> absent(std::string n,Support s=Support::unsupported){return {{},s,std::move(n)};}
bool absolute(const std::string& s){return !s.empty()&&s.front()=='/'&&s.size()<=32768&&s.find('\0')==s.npos;}
#ifdef __linux__
struct Fd{int n{-1};~Fd(){if(n>=0)::close(n);}};
int open_safe(int dir,const std::string& p,int flags){open_how h{};h.flags=static_cast<std::uint64_t>(flags|O_CLOEXEC);h.resolve=RESOLVE_NO_MAGICLINKS;return static_cast<int>(::syscall(SYS_openat2,dir,p.c_str(),&h,sizeof h));}
int reopen(int pin,int flags){return ::open(("/proc/self/fd/"+std::to_string(pin)).c_str(),flags|O_CLOEXEC|O_NONBLOCK);}
bool same(int a,int b){struct stat x{},y{};return ::fstat(a,&x)==0&&::fstat(b,&y)==0&&x.st_dev==y.st_dev&&x.st_ino==y.st_ino;}
Support access_support(){return errno==EACCES||errno==EPERM?Support::permission_required:Support::unsupported;}
using Context=detail::NetworkSelectionContext;
Metric<Context> context(){const auto api=detail::native_network_selection_source_api();return api.observe(api.context);}
bool exact(const Metric<Context>& m){return m.value&&m.support==Support::available;}
struct SigpipeGuard {
    sigset_t old{},one{};bool active{},pending{};
    bool block(){::sigemptyset(&one);::sigaddset(&one,SIGPIPE);if(::pthread_sigmask(SIG_BLOCK,&one,&old)!=0)return false;active=true;sigset_t p{};if(::sigpending(&p)!=0)return false;pending=::sigismember(&p,SIGPIPE)==1;return true;}
    void consume(){if(!pending){timespec zero{};while(::sigtimedwait(&one,nullptr,&zero)<0&&errno==EINTR){}}}
    ~SigpipeGuard(){if(active)::pthread_sigmask(SIG_SETMASK,&old,nullptr);}
};
#endif
}
Metric<LinuxRunitStatus> decode_linux_runit_status(std::span<const std::uint8_t> b){
    if(b.size()!=20)return absent<LinuxRunitStatus>("Expected runsv 20-byte status, not daemontools18 or s6");
    std::uint32_t ns=0,pid=0;for(unsigned n=0;n<4;++n){ns=(ns<<8)|b[8+n];pid|=std::uint32_t{b[12+n]}<<(8*n);}
    if(ns>=1000000000||pid>2147483647U||b[16]>1||b[18]>1||(b[17]!='u'&&b[17]!='d')||b[19]>2||(b[19]==0&&pid)||(b[19]==1&&!pid))return absent<LinuxRunitStatus>("Malformed runsv state/PID/flags/TAI nanoseconds");
    LinuxRunitStatus out;std::copy_n(b.begin(),12,out.changed_at_tai64n.begin());out.reported_pid=pid;out.state=static_cast<LinuxRunitState>(b[19]);out.paused=b[16]!=0;out.want_up=b[17]=='u';out.term_sent=b[18]!=0;
    return {out,Support::available,"Supervisor-reported runit state, not kernel liveness/readiness or PID namespace proof"};
}
struct LinuxRunitService::Impl {
    mutable std::mutex mutex;
#ifdef __linux__
    Fd service,supervise,control,ok;Context owner;std::uint32_t euid{};
    bool owner_valid()const{const auto c=context();return exact(c)&&*c.value==owner&&euid==static_cast<std::uint32_t>(::geteuid());}
    bool endpoints()const{Fd s{open_safe(service.n,"supervise",O_PATH|O_DIRECTORY)};Fd c{::openat(supervise.n,"control",O_PATH|O_CLOEXEC|O_NOFOLLOW)};Fd o{::openat(supervise.n,"ok",O_PATH|O_CLOEXEC|O_NOFOLLOW)};return s.n>=0&&c.n>=0&&o.n>=0&&same(s.n,supervise.n)&&same(c.n,control.n)&&same(o.n,ok.n);}
    int ready()const{int fd=reopen(ok.n,O_WRONLY);if(fd>=0&&!same(fd,ok.n)){::close(fd);return -1;}return fd;}
    std::optional<std::array<std::uint8_t,20>> status()const{
        Fd pin{::openat(supervise.n,"status",O_PATH|O_CLOEXEC|O_NOFOLLOW)};struct stat before{},after{};
        if(pin.n<0||::fstat(pin.n,&before)!=0||!S_ISREG(before.st_mode)||before.st_size!=20)return {};
        Fd fd{reopen(pin.n,O_RDONLY)};if(fd.n<0||!same(fd.n,pin.n))return {};
        std::array<std::uint8_t,21> b{};ssize_t n;do{n=::read(fd.n,b.data(),b.size());}while(n<0&&errno==EINTR);
        if(n!=20||::fstat(fd.n,&after)!=0||before.st_size!=after.st_size||before.st_mtim.tv_sec!=after.st_mtim.tv_sec||before.st_mtim.tv_nsec!=after.st_mtim.tv_nsec||before.st_ctim.tv_sec!=after.st_ctim.tv_sec||before.st_ctim.tv_nsec!=after.st_ctim.tv_nsec)return {};
        std::array<std::uint8_t,20> out{};std::copy_n(b.begin(),20,out.begin());return out;
    }
#endif
};
LinuxRunitService::LinuxRunitService(std::unique_ptr<Impl> p):impl_(std::move(p)){}
LinuxRunitService::~LinuxRunitService()=default;
Metric<std::shared_ptr<LinuxRunitService>> LinuxRunitService::open_at(int dir,const std::string& path){
#ifdef __linux__
    const auto before=context();if(!exact(before))return absent<std::shared_ptr<LinuxRunitService>>(before.note,before.support);
    auto p=std::make_unique<Impl>();p->owner=*before.value;p->euid=static_cast<std::uint32_t>(::geteuid());
    p->service.n=open_safe(dir,path,O_PATH|O_DIRECTORY);if(p->service.n<0)return absent<std::shared_ptr<LinuxRunitService>>("Cannot pin runit service directory (openat2 required)",access_support());
    p->supervise.n=open_safe(p->service.n,"supervise",O_PATH|O_DIRECTORY);if(p->supervise.n<0)return absent<std::shared_ptr<LinuxRunitService>>("No usable runit supervise directory",access_support());
    p->control.n=::openat(p->supervise.n,"control",O_PATH|O_CLOEXEC|O_NOFOLLOW);p->ok.n=::openat(p->supervise.n,"ok",O_PATH|O_CLOEXEC|O_NOFOLLOW);
    struct stat c{},o{};if(p->control.n<0||p->ok.n<0||::fstat(p->control.n,&c)!=0||::fstat(p->ok.n,&o)!=0||!S_ISFIFO(c.st_mode)||!S_ISFIFO(o.st_mode))return absent<std::shared_ptr<LinuxRunitService>>("Runit endpoints must be real FIFO nodes");
    if(!p->owner_valid()||!p->endpoints())return absent<std::shared_ptr<LinuxRunitService>>("Runit caller/endpoints changed during open");
    return {std::shared_ptr<LinuxRunitService>(new LinuxRunitService(std::move(p))),Support::available,"Pinned caller-trusted runit protocol endpoints, not authenticated supervisor identity"};
#else
    (void)dir;(void)path;return absent<std::shared_ptr<LinuxRunitService>>("Runit provider requires Linux");
#endif
}
Metric<std::shared_ptr<LinuxRunitService>> LinuxRunitService::open(const std::string& directory){
    if(!absolute(directory))return absent<std::shared_ptr<LinuxRunitService>>("Absolute runit directory required");
    return open_at(-100,directory); // AT_FDCWD on Linux, unused on other platforms.
}
Metric<LinuxRunitStatus> LinuxRunitService::read()const{
    std::lock_guard lock(impl_->mutex);
#ifdef __linux__
    if(!impl_->owner_valid()||!impl_->endpoints())return absent<LinuxRunitStatus>("Runit owner/endpoints changed");
    Fd ready{impl_->ready()};if(ready.n<0)return absent<LinuxRunitStatus>("No reader on runit supervise/ok; stale status is not running evidence",access_support());
    const auto a=impl_->status(),b=impl_->status();if(!a||!b||a!=b||!impl_->endpoints()||!impl_->owner_valid())return absent<LinuxRunitStatus>("Runit status/context changed or malformed; retry, not zero");
    return decode_linux_runit_status(*b);
#else
    return absent<LinuxRunitStatus>("Runit provider requires Linux");
#endif
}
LinuxRunitRequestResult LinuxRunitService::request(ServiceAction action,bool authorized){
    if(!authorized)return {ActionError::permission_denied,0,0,{},"Explicit runit control authorization required"};
    std::string command;switch(action){case ServiceAction::start:command="u";break;case ServiceAction::stop:command="d";break;case ServiceAction::restart:command="tcu";break;default:return {ActionError::invalid_argument,0,0,{},"Invalid runit action"};}
    std::lock_guard lock(impl_->mutex);
#ifdef __linux__
    if(!impl_->owner_valid()||!impl_->endpoints())return {ActionError::stale_target,0,0,{},"Runit owner/endpoints changed"};
    Fd ready{impl_->ready()};if(ready.n<0)return {ActionError::not_supported,0,0,{},"Runit ok FIFO has no usable reader"};
    Fd fd{reopen(impl_->control.n,O_WRONLY)};
    if(fd.n<0||!same(fd.n,impl_->control.n)||!impl_->endpoints()||!impl_->owner_valid())return {ActionError::stale_target,0,0,{},"Runit control unavailable/replaced; nothing written"};
    return detail::write_runit_control_fd(fd.n,command);
#else
    return {ActionError::not_supported,0,0,{},"Runit control requires Linux"};
#endif
}
Metric<LinuxRunitInventory> query_linux_runit_inventory(const std::string& root,std::uint32_t max_entries){
    if(!absolute(root)||!max_entries||max_entries>4096)return absent<LinuxRunitInventory>("Invalid runit root/bound");
#ifdef __linux__
    const auto before=context();if(!exact(before))return absent<LinuxRunitInventory>(before.note,before.support);
    Fd fd{open_safe(AT_FDCWD,root,O_RDONLY|O_DIRECTORY)};if(fd.n<0)return absent<LinuxRunitInventory>("Cannot open explicit runit root",access_support());
    const int enumeration=::fcntl(fd.n,F_DUPFD_CLOEXEC,3);if(enumeration<0)return absent<LinuxRunitInventory>("Cannot duplicate runit root");
    DIR* raw=::fdopendir(enumeration);if(!raw){::close(enumeration);return absent<LinuxRunitInventory>("Cannot enumerate runit root");}
    struct Dir{DIR* p;~Dir(){::closedir(p);}} dir{raw};LinuxRunitInventory out;out.root=root;std::size_t bytes=0;struct stat initial{},last{};if(::fstat(fd.n,&initial)!=0)return absent<LinuxRunitInventory>("Runit root stat failed");
    std::uint32_t visited=0;
    for(;;){errno=0;const auto* e=::readdir(raw);if(!e){if(errno){out.complete=false;out.diagnostics.push_back("Runit enumeration failed partway");}break;}
        if(++visited>16384){out.complete=false;out.diagnostics.push_back("Runit visit bound reached");break;}
        const std::string name=e->d_name;if(name=="."||name==".."||name.starts_with('.'))continue;
        Fd candidate{open_safe(fd.n,name,O_PATH|O_DIRECTORY)};if(candidate.n<0&&errno==ENOTDIR)continue;
        const auto cost=root.size()+2*name.size()+1;
        if(out.services.size()>=max_entries||cost>8*1024*1024-bytes){out.complete=false;out.diagnostics.push_back("Runit inventory bound reached; no silent truncation");break;}
        LinuxRunitServiceInfo row;row.name=name;row.directory=root+(root.back()=='/'?"":"/")+name;bytes+=row.directory.size()+name.size();
        const auto service=LinuxRunitService::open_at(fd.n,name);row.status=service.value?(*service.value)->read():absent<LinuxRunitStatus>(service.note,service.support);
        row.description=absent<std::string>("No standard runit description metadata");row.group=absent<std::string>("Runit tree/root is not a Windows service group");out.services.push_back(std::move(row));
    }
    const auto after=context();if(!exact(after)||*after.value!=*before.value)return absent<LinuxRunitInventory>("Runit inventory caller context changed");
    if(::fstat(fd.n,&last)!=0||last.st_mtim.tv_sec!=initial.st_mtim.tv_sec||last.st_mtim.tv_nsec!=initial.st_mtim.tv_nsec||last.st_ctim.tv_sec!=initial.st_ctim.tv_sec||last.st_ctim.tv_nsec!=initial.st_ctim.tv_nsec){out.complete=false;out.diagnostics.push_back("Runit directory membership changed during enumeration");}
    std::sort(out.services.begin(),out.services.end(),[](const auto& a,const auto& b){return a.name<b.name;});const auto support=out.complete?Support::available:Support::estimated;
    return {std::move(out),support,"Explicit runit directory inventory; per-service status availability is separate, not an atomic census"};
#else
    return absent<LinuxRunitInventory>("Runit inventory requires Linux");
#endif
}
}
namespace pcinsight::detail {
LinuxRunitRequestResult write_runit_control_fd(int fd,std::string_view command){
    if(command!="u"&&command!="d"&&command!="tcu")return {ActionError::invalid_argument,0,0,{},"Invalid runit transport command"};
#ifdef __linux__
    struct stat st{};const auto flags=::fcntl(fd,F_GETFL);if(::fstat(fd,&st)!=0||!S_ISFIFO(st.st_mode)||flags<0||!(flags&O_NONBLOCK))return {ActionError::invalid_argument,0,0,{},"Nonblocking FIFO writer required"};
    LinuxRunitRequestResult result{ActionError::none,static_cast<std::uint32_t>(command.size()),0,{{},Support::unsupported,"Completion/readiness unknown; no check script executed"},"Runit FIFO delivery only; service completion NOT certified"};
    std::string failed="Runit write failed/partial; no retry, inspect written_bytes, completion unknown";
    SigpipeGuard mask;if(!mask.block())return {ActionError::system_error,0,0,{},"Cannot isolate SIGPIPE for runit write"};
    ssize_t n;do{n=::write(fd,command.data(),command.size());}while(n<0&&errno==EINTR);
    const int code=errno;if(n<0&&code==EPIPE)mask.consume();
    if(n>0)result.written_bytes=static_cast<std::uint32_t>(n);
    if(n!=static_cast<ssize_t>(command.size())){result.error=ActionError::system_error;result.message.swap(failed);}return result;
#else
    (void)fd;return {ActionError::not_supported,0,0,{},"Runit transport requires Linux"};
#endif
}
}
