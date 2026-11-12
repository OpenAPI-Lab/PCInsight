#include "linux_gated_launch.hpp"
#if defined(__linux__) && __has_include(<linux/openat2.h>) && __has_include(<linux/sched.h>)
#include <linux/openat2.h>
#include <linux/sched.h>
#include <sys/syscall.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/prctl.h>
#include <poll.h>
#include <signal.h>
#include <fcntl.h>
#include <unistd.h>
#include <cerrno>
#include <cstring>
extern char** environ;
#endif
namespace pcinsight::detail {
#if defined(__linux__) && defined(SYS_clone3) && defined(SYS_execveat) && defined(SYS_openat2) && defined(SYS_pidfd_send_signal) && defined(SYS_rt_sigprocmask) && defined(SYS_recvfrom)
namespace {
struct Fd {int n=-1;~Fd(){if(n>=0)::close(n);}void close(){if(n>=0)::close(n);n=-1;}};
struct Child {
    pid_t pid=-1;Fd pidfd;bool released{};
    ~Child(){if(pid>0&&!released){::syscall(SYS_pidfd_send_signal,pidfd.n,SIGKILL,nullptr,0);while(::waitpid(pid,nullptr,0)<0&&errno==EINTR){}}}
};
struct SignalMask {std::uint64_t old{};bool active{};~SignalMask(){restore();}void restore(){if(active){active=false;::syscall(SYS_rt_sigprocmask,SIG_SETMASK,&old,nullptr,sizeof old);}}};
ActionError kind(int e){return e==ENOSYS||e==EOPNOTSUPP?ActionError::not_supported:e==EPERM||e==EACCES?ActionError::permission_denied:e==ENOENT||e==ENOTDIR?ActionError::not_found:e==EINVAL||e==E2BIG||e==ENOEXEC?ActionError::invalid_argument:ActionError::system_error;}
bool text(const std::string& s){return s.size()<=131071&&s.find('\0')==s.npos;}
LinuxExecutableStamp stamp(const struct stat& s){return {static_cast<std::uint64_t>(s.st_dev),static_cast<std::uint64_t>(s.st_ino),static_cast<std::uint64_t>(s.st_size),static_cast<std::uint64_t>(s.st_mode),s.st_mtim.tv_sec,s.st_mtim.tv_nsec,s.st_ctim.tv_sec,s.st_ctim.tv_nsec};}
bool alive(int fd){pollfd p{fd,POLLIN,0};return ::poll(&p,1,0)==0&&!p.revents;}
}
ActionResult launch_linux_gated(const LaunchRequest& r,bool authorized,const LinuxLaunchArm& arm){
    if(!authorized)return {ActionError::permission_denied,"Explicit gated child launch authorization required"};
    if(!arm||r.executable.empty()||r.executable[0]!='/'||!text(r.executable)||!text(r.working_directory)||(!r.working_directory.empty()&&r.working_directory[0]!='/')||r.arguments.size()>4096)
        return {ActionError::invalid_argument,"Absolute executable/cwd, bounded literal argv and arming callback required"};
    if(r.elevated||::getuid()!=::geteuid()||::getgid()!=::getegid())return {ActionError::permission_denied,"Gated launch never elevates or accepts split caller credentials"};
    std::size_t bytes=r.executable.size()+r.working_directory.size()+2;
    for(const auto& s:r.arguments){if(!text(s)||s.size()+1>1048576-bytes)return {ActionError::invalid_argument,"Gated argv bound exceeded"};bytes+=s.size()+1;}
    const auto caller_uid=::getuid(),caller_euid=::geteuid();const auto caller_gid=::getgid(),caller_egid=::getegid();
    open_how how{};how.flags=O_PATH|O_CLOEXEC;how.resolve=RESOLVE_NO_MAGICLINKS;
    Fd image{static_cast<int>(::syscall(SYS_openat2,AT_FDCWD,r.executable.c_str(),&how,sizeof how))};
    if(image.n<0)return {kind(errno),"Cannot pin gated executable with openat2 NO_MAGICLINKS"};
    struct stat info{};unsigned char elf[4]{};
    if(::fstat(image.n,&info)!=0||!S_ISREG(info.st_mode)||(info.st_mode&(S_ISUID|S_ISGID)))
        return {ActionError::invalid_argument,"Gated executable must be regular non-setid ELF; no script/shell fallback"};
    const auto pinned=stamp(info);const auto own_fd="/proc/self/fd/"+std::to_string(image.n);Fd reader{::open(own_fd.c_str(),O_RDONLY|O_NONBLOCK|O_CLOEXEC)};
    if(reader.n<0)return {kind(errno),"Cannot read the already pinned regular executable"};
    if(::fstat(reader.n,&info)!=0||stamp(info)!=pinned||::pread(reader.n,elf,4,0)!=4||std::memcmp(elf,"\177ELF",4))return {ActionError::invalid_argument,"Pinned executable lacks ELF magic or changed; no fallback"};
    const auto api=native_network_selection_source_api();const auto before=api.observe(api.context);
    if(!before.value||before.support!=Support::available)return {ActionError::not_supported,"Gated caller context unavailable"};
    std::vector<std::string> args{r.executable},env_strings;args.insert(args.end(),r.arguments.begin(),r.arguments.end());
    std::size_t env_bytes=0;
    if(environ)for(auto p=environ;*p;++p){const auto n=::strnlen(*p,131072);if(n>=131072||n+1>2097152-env_bytes||env_strings.size()>=32768)return {ActionError::invalid_argument,"Gated environment bound exceeded"};env_bytes+=n+1;env_strings.emplace_back(*p,n);}
    std::vector<char*> argv,env;for(auto& s:args)argv.push_back(s.data());argv.push_back(nullptr);for(auto& s:env_strings)env.push_back(s.data());env.push_back(nullptr);
    int gate_fds[2],error_fds[2];if(::socketpair(AF_UNIX,SOCK_SEQPACKET|SOCK_CLOEXEC,0,gate_fds)!=0)return {kind(errno),"Cannot create private exec gate"};
    Fd gate_read{gate_fds[0]},gate_write{gate_fds[1]};if(::pipe2(error_fds,O_CLOEXEC)!=0)return {kind(errno),"Cannot create private exec error channel"};Fd error_read{error_fds[0]},error_write{error_fds[1]};
    // Prepare all child pointers and parent result allocations before clone.
    const char* cwd=r.working_directory.empty()?nullptr:r.working_directory.c_str();auto arg_data=argv.data(),env_data=env.data();
    ActionResult success{ActionError::none,"Gated child released; no pre-exec error reported, successful exec/app/startup cause NOT certified"};
    ActionResult unknown{ActionError::system_error,"Gated child released; error channel failed, caller owns unknown child state"};
    ActionResult failed{ActionError::system_error,"Gated pre-exec failed; child cancelled/reaped"};
    // Kernel sigset ABI is8 bytes on supported architectures. If not, reject
    // before clone instead of guessing. Raw masking also covers libc's reserved
    // signals; child calls below avoid libc cancellation/atfork machinery.
    Child child;SignalMask mask;const std::uint64_t full=UINT64_MAX;
    if(::syscall(SYS_rt_sigprocmask,SIG_SETMASK,&full,&mask.old,sizeof full)!=0)return {ActionError::not_supported,"Unsupported kernel signal-mask ABI; no child created"};
    mask.active=true;
    clone_args clone{};clone.flags=CLONE_PIDFD;clone.pidfd=reinterpret_cast<std::uint64_t>(&child.pidfd.n);clone.exit_signal=SIGCHLD;
    const auto pid=::syscall(SYS_clone3,&clone,sizeof clone);
    if(pid==0){
        // No C++ allocation/streams/environment lookup/libc atfork callbacks.
        ::syscall(SYS_close,gate_write.n);::syscall(SYS_close,error_read.n);int error=0;char go{};ssize_t n;
        if(::syscall(SYS_prctl,PR_SET_NO_NEW_PRIVS,1,0,0,0)!=0)error=errno;
        if(!error){do{n=::syscall(SYS_recvfrom,gate_read.n,&go,1,0,nullptr,nullptr);}while(n<0&&errno==EINTR);if(n!=1||go!='G')error=ECANCELED;}
        ::syscall(SYS_close,gate_read.n);
        if(!error&&cwd&&::syscall(SYS_chdir,cwd)!=0)error=errno;
        if(!error&&::syscall(SYS_rt_sigprocmask,SIG_SETMASK,&mask.old,nullptr,sizeof mask.old)!=0)error=errno;
        if(!error){::syscall(SYS_execveat,image.n,"",arg_data,env_data,AT_EMPTY_PATH);error=errno;}
        const char* data=reinterpret_cast<const char*>(&error);std::size_t sent=0;
        while(sent<sizeof error){const auto count=::syscall(SYS_write,error_write.n,data+sent,sizeof error-sent);if(count>0)sent+=static_cast<std::size_t>(count);else if(count<0&&errno==EINTR)continue;else break;}
        for(;;){::syscall(SYS_exit_group,127);::syscall(SYS_exit,127);}
    }
    const auto clone_error=errno;if(pid>0)child.pid=static_cast<pid_t>(pid);mask.restore();if(pid<0)return {kind(clone_error),"clone3 CLONE_PIDFD unavailable/denied; no unsafe fork fallback"};
    gate_read.close();error_write.close();success.process_id=unknown.process_id=static_cast<std::uint32_t>(pid);
    const auto identity=read_linux_process_identity(std::filesystem::path("/proc")/std::to_string(pid));
    if(!identity.value||!alive(child.pidfd.n))return failed;
    const auto local=read_linux_network_pidfd_process(child.pidfd.n,static_cast<std::uint32_t>(pid),identity.value->start_ticks);
    if(!local.value||local.support!=Support::available)return failed;
    LinuxGatedLaunchTarget target{child.pidfd.n,image.n,*before.value,*local.value,pinned};
    try{auto accepted=arm(target);if(!accepted){accepted.process_id=0;return accepted;}}
    catch(...){return failed;}
    const auto after=api.observe(api.context);const auto again=read_linux_network_pidfd_process(child.pidfd.n,static_cast<std::uint32_t>(pid),identity.value->start_ticks);
    if(!after.value||after.support!=Support::available||*after.value!=*before.value||!again.value||*again.value!=*local.value||!alive(child.pidfd.n)||::getuid()!=caller_uid||::geteuid()!=caller_euid||::getgid()!=caller_gid||::getegid()!=caller_egid||::fstat(image.n,&info)!=0||stamp(info)!=pinned)return failed;
    ssize_t sent;do{sent=::send(gate_write.n,"G",1,MSG_NOSIGNAL);}while(sent<0&&errno==EINTR);
    if(sent!=1)return failed;
    child.released=true;gate_write.close();int error=0;std::size_t received=0;
    while(received<sizeof error){const auto n=::read(error_read.n,reinterpret_cast<char*>(&error)+received,sizeof error-received);if(n>0)received+=static_cast<std::size_t>(n);else if(n==0)break;else if(errno!=EINTR)return unknown;}
    if(!received)return success; // EOF is NOT kernel proof of successful exec.
    child.released=false;failed.error=received==sizeof error&&error?kind(error):ActionError::system_error;return failed;
}
#else
ActionResult launch_linux_gated(const LaunchRequest&,bool authorized,const LinuxLaunchArm&){return {authorized?ActionError::not_supported:ActionError::permission_denied,"Gated launch requires Linux clone3/execveat/openat2/pidfd; no fallback"};}
#endif
}
