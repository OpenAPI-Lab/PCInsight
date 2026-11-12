#include "linux_openrc_control.hpp"
#include "linux_privileged_connection.hpp"
#include <chrono>
#include <cstring>
#include <mutex>
#include <stdexcept>
#ifdef __linux__
#include <cerrno>
#include <fcntl.h>
#include <poll.h>
#include <sys/prctl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/sysmacros.h>
#include <sys/wait.h>
#include <unistd.h>
extern char** environ;
#endif
namespace pcinsight::detail {
struct OpenrcControlChild::Impl {
    mutable std::mutex mutex;
#ifdef __linux__
    int pidfd{-1};pid_t pid{},owner=getpid();uid_t uid=geteuid();bool reaped{};OpenrcRunnerStatus status;
    ~Impl(){if(pidfd>=0)close(pidfd);}
#endif
};
OpenrcControlChild::OpenrcControlChild(std::unique_ptr<Impl> p):impl_(std::move(p)){}
OpenrcControlChild::~OpenrcControlChild()=default;
std::uint32_t OpenrcControlChild::process_id()const{
#ifdef __linux__
    return impl_?std::uint32_t(impl_->pid):0;
#else
    return 0;
#endif
}
Metric<OpenrcRunnerStatus> OpenrcControlChild::wait(std::uint32_t timeout){
    if(!impl_||timeout>30000)return {{},Support::unsupported,"Invalid runner/wait bound"};
    std::lock_guard lock(impl_->mutex);
#ifdef __linux__
    if(impl_->owner!=getpid()||impl_->uid!=geteuid())return {{},Support::unsupported,"Runner owner context changed"};
    if(impl_->reaped)return {impl_->status,Support::available,"Historical runner exit only, NOT service state"};
    pollfd fd{impl_->pidfd,POLLIN,0};int rc;const auto deadline=std::chrono::steady_clock::now()+std::chrono::milliseconds(timeout);
    do{const auto remaining=std::chrono::duration_cast<std::chrono::milliseconds>(deadline-std::chrono::steady_clock::now()).count();rc=poll(&fd,1,int(remaining>0?remaining:0));}while(rc<0&&errno==EINTR&&std::chrono::steady_clock::now()<deadline);
    if(rc<0&&errno==EINTR&&std::chrono::steady_clock::now()>=deadline)rc=0;
    if(rc<0)return {{},Support::unsupported,"Runner poll failed; retain child and retry wait"};
    if(!rc)return {OpenrcRunnerStatus{false,true,{},{}},Support::available,"Wait expired; runner/descendants NOT canceled"};
    // P_PIDFD=3 is the Linux UAPI, even with older libc headers. No bare-PID reap.
    siginfo_t status{};
    if(waitid(static_cast<idtype_t>(3),static_cast<id_t>(impl_->pidfd),&status,WEXITED|WNOHANG)||status.si_pid!=impl_->pid||
        (status.si_code!=CLD_EXITED&&status.si_code!=CLD_KILLED&&status.si_code!=CLD_DUMPED))
        return {{},Support::unsupported,"Runner cannot be reaped by pidfd; external reaper/context may have changed"};
    impl_->status.exited=true;
    if(status.si_code==CLD_EXITED)impl_->status.exit_code=status.si_status;
    else impl_->status.signal=status.si_status;
    impl_->reaped=true;
    return {impl_->status,Support::available,"Runner exit observed; NOT OpenRC liveness/readiness/state"};
#else
    return {{},Support::unsupported,"Runner wait requires Linux"};
#endif
}
ActionResult OpenrcControlChild::terminate_runner([[maybe_unused]] bool force,bool requested){
    if(!requested||!impl_)return {ActionError::invalid_argument,"Explicit runner termination required"};
    std::lock_guard lock(impl_->mutex);
#ifdef __linux__
    if(impl_->owner!=getpid()||impl_->uid!=geteuid()||impl_->reaped)return {ActionError::stale_target,"Runner context changed or already reaped"};
#ifdef SYS_pidfd_send_signal
    if(syscall(SYS_pidfd_send_signal,impl_->pidfd,force?SIGKILL:SIGTERM,nullptr,0)<0)
        return {ActionError::system_error,"pidfd runner signal failed"};
    return {};
#endif
#endif
    return {ActionError::not_supported,"No safe pidfd runner signaling"};
}
namespace {
#ifdef __linux__
struct File {int fd{-1};struct stat stamp{};~File(){if(fd>=0)close(fd);}};
struct GatedChildGuard {
    pid_t pid;int& gate;bool released{};
    ~GatedChildGuard(){if(!released){if(gate>=0){close(gate);gate=-1;}int status{};while(waitpid(pid,&status,0)<0&&errno==EINTR){}}}
};
bool same(const struct stat& a,const struct stat& b){return a.st_dev==b.st_dev&&a.st_ino==b.st_ino&&a.st_size==b.st_size&&a.st_mode==b.st_mode&&a.st_uid==b.st_uid&&a.st_gid==b.st_gid&&a.st_nlink==b.st_nlink&&a.st_mtim.tv_sec==b.st_mtim.tv_sec&&a.st_mtim.tv_nsec==b.st_mtim.tv_nsec&&a.st_ctim.tv_sec==b.st_ctim.tv_sec&&a.st_ctim.tv_nsec==b.st_ctim.tv_nsec;}
struct Selected {std::shared_ptr<PrivilegedDirectory> parent;std::string name;};
Selected select(const std::string& path,const OpenrcControlOptions& options,File& file,bool elf){
    const auto slash=path.rfind('/');if(slash==path.npos||slash+1==path.size()||path.size()>4096||path.find('\0')!=path.npos)throw std::runtime_error("Invalid explicit absolute control path");
    const auto parent=open_privileged_directory(slash?path.substr(0,slash):"/",options.owner_uid,options.allow_sticky_ancestors);if(!parent.value)throw std::runtime_error(parent.note);Selected selected{*parent.value,path.substr(slash+1)};
    for(const auto c:selected.name)if(!((c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='_'||c=='-'||c=='.'))throw std::runtime_error("Invalid literal file name");
    if(selected.name=="."||selected.name==".."||selected.name[0]=='-'||selected.name.size()>255)throw std::runtime_error("Unsafe literal file name");
    const auto dir=selected.parent->native_fd();struct stat before{};if(dir<0||fstatat(dir,selected.name.c_str(),&before,AT_SYMLINK_NOFOLLOW)||!S_ISREG(before.st_mode)||before.st_uid!=options.owner_uid||(before.st_mode&0022)||(before.st_mode&07000)||before.st_nlink!=1||before.st_size<=0||before.st_size>(elf?16*1024*1024:1024*1024))throw std::runtime_error("Untrusted control file owner/mode/type/size/link count");
    file.fd=openat(dir,selected.name.c_str(),O_RDONLY|O_CLOEXEC|O_NOFOLLOW|O_NONBLOCK);if(file.fd<0||fstat(file.fd,&file.stamp)||!same(before,file.stamp)||(elf&&!(file.stamp.st_mode&0111)))throw std::runtime_error("Control target changed/unexecutable");
    if(elf){char magic[4]{};if(pread(file.fd,magic,4,0)!=4||memcmp(magic,"\177ELF",4))throw std::runtime_error("Runtime must be explicit regular ELF; no interpreter fallback");}
    return selected;
}
bool unchanged(const Selected& s,const File& f){struct stat current{},named{};return s.parent->revalidate()&&!fstat(f.fd,&current)&&same(f.stamp,current)&&!fstatat(s.parent->native_fd(),s.name.c_str(),&named,AT_SYMLINK_NOFOLLOW)&&same(f.stamp,named);}
#endif
}
struct OpenrcControlSelection {
    OpenrcControlOptions options;
    std::mutex mutex;
    bool spent{},revoked{};
#ifdef __linux__
    File runtime,script;
    Selected runtime_location,script_location;
    uid_t real_uid=getuid(),effective_uid=geteuid();
    gid_t real_gid=getgid(),effective_gid=getegid();
    bool context_matches()const{return real_uid==getuid()&&effective_uid==geteuid()&&real_gid==getgid()&&effective_gid==getegid();}
#endif
};
void revoke_openrc_control(const std::shared_ptr<OpenrcControlSelection>& selection){
    if(selection){std::lock_guard lock(selection->mutex);selection->revoked=true;}
}
Metric<std::shared_ptr<OpenrcControlSelection>> select_openrc_control(const OpenrcControlOptions& options){
    if(!options.enabled)return {{},Support::unsupported,"OpenRC control disabled; no files queried"};
    if(options.owner_uid==UINT32_MAX)return {{},Support::unsupported,"Invalid configured owner"};
#ifdef __linux__
    try{
        if(getuid()!=geteuid()||getgid()!=getegid())throw std::runtime_error("Set-ID caller context unsupported");
        auto target=std::make_shared<OpenrcControlSelection>();target->options=options;
        target->runtime_location=select(options.runtime_path,options,target->runtime,true);
        target->script_location=select(options.script_path,options,target->script,false);
        if(!target->context_matches()||!unchanged(target->runtime_location,target->runtime)||!unchanged(target->script_location,target->script))throw std::runtime_error("Control selection changed");
        return {std::move(target),Support::available,"Pinned explicit control selection; no runtime executed"};
    }catch(const std::exception& e){return {{},Support::unsupported,e.what()};}
#else
    return {{},Support::unsupported,"OpenRC control selection requires Linux"};
#endif
}
OpenrcControlDispatch dispatch_openrc_control(const OpenrcControlOptions& options,ServiceAction action){
    OpenrcControlDispatch out;
    if(!options.enabled||!options.explicitly_requested){out.result={ActionError::permission_denied,"OpenRC control opt-in and explicit request required; no file queried"};return out;}
    if(action!=ServiceAction::start&&action!=ServiceAction::stop&&action!=ServiceAction::restart){out.result={ActionError::invalid_argument,"Invalid OpenRC action"};return out;}
    auto selection=select_openrc_control(options);
    if(!selection.value){out.result={ActionError::not_supported,selection.note};return out;}
    return dispatch_openrc_control_selected(*selection.value,action,true);
}
OpenrcControlDispatch dispatch_openrc_control_selected(const std::shared_ptr<OpenrcControlSelection>& selection,ServiceAction action,bool requested){
    OpenrcControlDispatch out;
    if(!requested){out.result={ActionError::permission_denied,"Explicit OpenRC action request required; no target IO"};return out;}
    if(!selection){out.result={ActionError::invalid_argument,"Missing OpenRC selection"};return out;}
    std::lock_guard lock(selection->mutex);
    const auto& options=selection->options;
    const char* verb=action==ServiceAction::start?"start":action==ServiceAction::stop?"stop":action==ServiceAction::restart?"restart":nullptr;
    if(!verb||options.owner_uid==UINT32_MAX){out.result={ActionError::invalid_argument,"Closed OpenRC action/owner invalid"};return out;}
    if(selection->spent||selection->revoked){out.result={ActionError::stale_target,"OpenRC selection consumed/revoked; explicit new selection required"};return out;}
#if defined(__linux__) && defined(SYS_pidfd_open) && defined(SYS_execveat) && defined(SYS_close_range)
    try{
        if(!selection->context_matches()){
            selection->revoked=true;out.result={ActionError::stale_target,"OpenRC selection credential context changed"};return out;
        }
        struct sigaction reap{};if(sigaction(SIGCHLD,nullptr,&reap)||reap.sa_handler==SIG_IGN||(reap.sa_flags&SA_NOCLDWAIT))throw std::runtime_error("Caller must retain waitable direct children");
        const auto uid=geteuid();const auto pid=getpid();
        auto& runtime=selection->runtime;auto& script=selection->script;
        const auto& selected_runtime=selection->runtime_location;const auto& selected_script=selection->script_location;
        if(!unchanged(selected_runtime,runtime)||!unchanged(selected_script,script)){
            selection->revoked=true;out.result={ActionError::stale_target,"Pinned OpenRC target/context changed; nothing dispatched"};return out;
        }
        // The ELF is pinned; OpenRC itself opens the original script path for basename/config.
        // Trusted owners must serialize writes/renames; script/config/dependencies NOT atomic.
        std::string executable=options.runtime_path,script_path=options.script_path,operation=verb;
        char* argv[]={executable.data(),script_path.data(),operation.data(),nullptr};
        char path[]="PATH=/usr/sbin:/usr/bin:/sbin:/bin",lang[]="LANG=C",locale[]="LC_ALL=C";char* env[]={path,lang,locale,nullptr};
        File executable_fd;executable_fd.fd=fcntl(runtime.fd,F_DUPFD_CLOEXEC,10);if(executable_fd.fd<0)throw std::runtime_error("Cannot reserve executable descriptor");
        int gates[2],errors[2];if(socketpair(AF_UNIX,SOCK_STREAM|SOCK_CLOEXEC,0,gates))throw std::runtime_error("Cannot gate runner");File gate_read,gate_write;gate_read.fd=gates[0];gate_write.fd=gates[1];if(pipe2(errors,O_CLOEXEC))throw std::runtime_error("Cannot create exec error channel");File err_read,err_write;err_read.fd=errors[0];err_write.fd=errors[1];
        File error_fd;error_fd.fd=fcntl(err_write.fd,F_DUPFD_CLOEXEC,10);if(error_fd.fd<0)throw std::runtime_error("Cannot reserve error descriptor");
        // Prepare diagnostics before a child can execute: string allocation
        // failure must not strand an already released runner.
        ActionResult released{ActionError::none,"Runner released, no exec error reported; NOT proof of execution/service completion/readiness"};
        ActionResult exec_error{ActionError::system_error,"Runner exec/error channel failed; retain child and reap; outcome may be unknown"};
        auto owned=std::shared_ptr<OpenrcControlChild>(new OpenrcControlChild(std::make_unique<OpenrcControlChild::Impl>()));
        auto& child=*owned->impl_;const auto born=fork();if(born<0)throw std::runtime_error("Cannot create runner");
        if(!born){close(gate_write.fd);close(err_read.fd);int failure{};char release{};
            if(read(gate_read.fd,&release,1)!=1||release!=1)_exit(125);
            int channel=error_fd.fd;
            if(prctl(PR_SET_NO_NEW_PRIVS,1,0,0,0)<0||chdir("/")<0)failure=errno;
            sigset_t empty;sigemptyset(&empty);struct sigaction defaults{};defaults.sa_handler=SIG_DFL;sigemptyset(&defaults.sa_mask);
            for(int signal:{SIGTERM,SIGINT,SIGHUP,SIGPIPE,SIGCHLD})if(sigaction(signal,&defaults,nullptr)<0)failure=errno;
            if(sigprocmask(SIG_SETMASK,&empty,nullptr)<0)failure=errno;
            // Reserve two known descriptors; no inherited app/socket fds reach runtime.
            if(!failure){if(dup3(error_fd.fd,4,O_CLOEXEC)<0)failure=errno;else channel=4;}
            if(!failure&&dup3(executable_fd.fd,3,O_CLOEXEC)<0)failure=errno;
            if(!failure&&syscall(SYS_close_range,5U,~0U,0)<0)failure=errno;
            int nullfd=-1;if(!failure){nullfd=open("/dev/null",O_RDWR|O_CLOEXEC);struct stat node{};if(nullfd<0||fstat(nullfd,&node)||!S_ISCHR(node.st_mode)||major(node.st_rdev)!=1||minor(node.st_rdev)!=3)failure=EACCES;}
            if(!failure){for(int fd=0;fd<3;++fd)if(dup2(nullfd,fd)<0||fcntl(fd,F_SETFD,0)<0)failure=errno;if(nullfd>4)close(nullfd);}
            if(!failure){syscall(SYS_execveat,3,"",argv,env,AT_EMPTY_PATH);failure=errno;}
            const auto reported=write(channel,&failure,sizeof failure);(void)reported;_exit(126);
        }
        GatedChildGuard guard{born,gate_write.fd};
        child.pid=born;close(gate_read.fd);gate_read.fd=-1;close(err_write.fd);err_write.fd=-1;close(error_fd.fd);error_fd.fd=-1;child.pidfd=int(syscall(SYS_pidfd_open,born,0));
        const bool valid=child.pidfd>=0&&uid==geteuid()&&pid==getpid()&&selection->context_matches()&&unchanged(selected_runtime,runtime)&&unchanged(selected_script,script);
        if(!valid){selection->revoked=true;out.result={ActionError::stale_target,"Cannot pin/revalidate still-gated runner; nothing released"};return out;}
        out.child=owned;
        selection->spent=true;
        guard.released=true; // From here the returned handle, never a hidden reaper, owns wait.
        const char release=1;const auto sent=send(gate_write.fd,&release,1,MSG_NOSIGNAL);close(gate_write.fd);gate_write.fd=-1;
        out.runtime_may_have_executed=sent==1;
        pollfd ack{err_read.fd,POLLIN,0};int ready{};
        const auto ack_deadline=std::chrono::steady_clock::now()+std::chrono::seconds(2);
        do{const auto left=std::chrono::duration_cast<std::chrono::milliseconds>(ack_deadline-std::chrono::steady_clock::now()).count();ready=poll(&ack,1,int(left>0?left:0));}while(ready<0&&errno==EINTR&&std::chrono::steady_clock::now()<ack_deadline);
        int failure{};const auto count=ready>0?read(err_read.fd,&failure,sizeof failure):-1;
        if(sent!=1||count!=0)out.result=std::move(exec_error);
        else out.result=std::move(released);
        out.result.process_id=out.child->process_id();
    }catch(const std::exception& e){out.result={ActionError::not_supported,e.what()};}
#else
    out.result={ActionError::not_supported,"OpenRC controlled exec needs Linux pidfd/execveat/close_range"};
#endif
    return out;
}
}

namespace pcinsight {
struct LinuxOpenrcRunner::Impl {std::shared_ptr<detail::OpenrcControlChild> child;};
LinuxOpenrcRunner::LinuxOpenrcRunner(std::unique_ptr<Impl> p):impl_(std::move(p)){}
LinuxOpenrcRunner::~LinuxOpenrcRunner()=default;
Metric<LinuxOpenrcRunnerStatus> LinuxOpenrcRunner::wait(std::uint32_t timeout){
    if(!impl_||!impl_->child)return {{},Support::unsupported,"No OpenRC runner"};
    return impl_->child->wait(timeout);
}
ActionResult LinuxOpenrcRunner::terminate_runner(bool force,bool requested){
    if(!impl_||!impl_->child)return {ActionError::invalid_argument,"No OpenRC runner"};
    return impl_->child->terminate_runner(force,requested);
}
std::uint32_t LinuxOpenrcRunner::process_id()const{return impl_&&impl_->child?impl_->child->process_id():0;}
struct LinuxOpenrcService::Impl {std::shared_ptr<detail::OpenrcControlSelection> selection;std::string name;};
LinuxOpenrcService::LinuxOpenrcService(std::unique_ptr<Impl> p):impl_(std::move(p)){}
LinuxOpenrcService::~LinuxOpenrcService()=default;
std::string LinuxOpenrcService::name()const{return impl_->name;}
void LinuxOpenrcService::close(){detail::revoke_openrc_control(impl_->selection);}
Metric<std::shared_ptr<LinuxOpenrcService>> LinuxOpenrcService::open(const LinuxOpenrcControlConfig& config,const std::string& name){
    if(!config.enabled)return {{},Support::unsupported,"OpenRC control disabled; no files queried"};
    if(config.init_directory.empty()||config.init_directory[0]!='/'||config.init_directory.size()>4096||config.init_directory.find('\0')!=std::string::npos||
        (config.init_directory.size()>1&&config.init_directory.back()=='/'))return {{},Support::unsupported,"Explicit absolute init directory required"};
    if(name.empty()||name.size()>255||name=="."||name==".."||name[0]=='-')return {{},Support::unsupported,"Invalid literal OpenRC service name"};
    for(char c:name)if(!((c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='_'||c=='-'||c=='.'))return {{},Support::unsupported,"Invalid literal OpenRC service name"};
    detail::OpenrcControlOptions options;
    options.enabled=true;options.runtime_path=config.runtime_path;options.owner_uid=config.owner_uid;
    options.allow_sticky_ancestors=config.allow_sticky_ancestors;
    options.script_path=config.init_directory+(config.init_directory=="/"?"":"/")+name;
    auto selected=detail::select_openrc_control(options);
    if(!selected.value)return {{},selected.support,std::move(selected.note)};
    auto p=std::make_unique<Impl>();p->selection=std::move(*selected.value);p->name=name;
    return {std::shared_ptr<LinuxOpenrcService>(new LinuxOpenrcService(std::move(p))),Support::available,"Pinned OpenRC selection only; explicit action still required"};
}
LinuxOpenrcControlResult LinuxOpenrcService::request(ServiceAction action,bool requested){
    // Allocate public ownership BEFORE dispatch so allocation failure cannot
    // abandon a released child. Subsequent ownership transfers are noexcept.
    auto runner=std::shared_ptr<LinuxOpenrcRunner>(new LinuxOpenrcRunner(std::make_unique<LinuxOpenrcRunner::Impl>()));
    LinuxOpenrcControlResult out;
    auto native=detail::dispatch_openrc_control_selected(impl_->selection,action,requested);
    out.result=std::move(native.result);out.runtime_may_have_executed=native.runtime_may_have_executed;
    if(native.child){runner->impl_->child=std::move(native.child);out.runner=std::move(runner);}
    return out;
}
}
