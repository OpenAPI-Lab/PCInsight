#include "pcinsight/startup_impact.hpp"
#include <limits>
#ifdef __linux__
#include <cerrno>
#include <sys/syscall.h>
#include <sys/wait.h>
#include <unistd.h>
#endif
namespace pcinsight {
StartupChildExitSampleResult read_startup_child_exit_sample(std::uint32_t pid,std::uint64_t expected) {
    StartupChildExitSampleResult result;
    if(!pid || pid>static_cast<std::uint32_t>(std::numeric_limits<int>::max()) || !expected) {
        result.error=ActionError::invalid_argument;result.message="invalid child PID/start_ticks";return result;
    }
#if defined(__linux__) && defined(SYS_pidfd_open)
    struct Fd {int value;~Fd(){if(value>=0)close(value);}} fd{static_cast<int>(syscall(SYS_pidfd_open,pid,0))};
    const auto failure=[&](int code,const char* message) {
        result.sample.reset();
        result.error=(code==EACCES || code==EPERM)?ActionError::permission_denied:
            (code==ENOENT || code==ESRCH)?ActionError::not_found:
            (code==ECHILD || code==EINVAL || code==ENOSYS)?ActionError::not_supported:ActionError::system_error;
        result.message=message;return result;
    };
    if(fd.value<0)return failure(errno,"pidfd unavailable for startup child");
    // Both counter reads validate genuine procfs, caller PID namespace and
    // expected start_ticks. No wait by bare PID, no global SIGCHLD changes.
    const auto initial=read_startup_process_sample(pid,expected);
    if(!initial){result.error=initial.error;result.message=initial.message;return result;}
    siginfo_t info{};
    constexpr auto pidfd_type=static_cast<idtype_t>(3); // Linux P_PIDFD ABI, since 5.4
    if(waitid(pidfd_type,static_cast<id_t>(fd.value),&info,WEXITED|WNOHANG|WNOWAIT)!=0)
        return failure(errno,"child exit observation unavailable; requires an unreaped caller child and pidfd waitid");
    if(!info.si_pid){result.message="child has not exited; no final sample";return result;}
    if(info.si_pid!=static_cast<pid_t>(pid) ||
        (info.si_code!=CLD_EXITED && info.si_code!=CLD_KILLED && info.si_code!=CLD_DUMPED))
        return failure(EIO,"unexpected child exit identity/state");
    result.exited=true;
    if(info.si_code==CLD_EXITED)result.exit_code=info.si_status;
    else result.termination_signal=info.si_status;
    auto final=read_startup_process_sample(pid,expected);
    if(!final){result.error=final.error;result.message=final.message;return result;}
    if(final.value->native_context!=initial.value->native_context) {
        result.error=ActionError::stale_target;result.message="child native context changed";return result;
    }
    siginfo_t confirmed{};
    if(waitid(pidfd_type,static_cast<id_t>(fd.value),&confirmed,WEXITED|WNOHANG|WNOWAIT)!=0)
        return failure(errno,"child was reaped concurrently; final sample not accepted");
    if(confirmed.si_pid!=info.si_pid || confirmed.si_code!=info.si_code || confirmed.si_status!=info.si_status) {
        result.error=ActionError::stale_target;result.message="child exit identity/state changed; no final sample";return result;
    }
    result.sample=std::move(final.value);
    result.message="final procfs counters of unreaped child; caller still owns waitpid";
    return result;
#else
    result.error=ActionError::not_supported;result.message="startup child exit sampling requires Linux pidfd/waitid";return result;
#endif
}
}
