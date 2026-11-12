#include <pcinsight/sampler.hpp>
#include <cerrno>
#include <cstddef>
#include <csignal>
#include <iostream>
#include <thread>
#include <fcntl.h>
#include <linux/audit.h>
#include <linux/filter.h>
#include <linux/seccomp.h>
#include <sys/prctl.h>
#include <sys/resource.h>
#include <sys/syscall.h>
#include <sys/wait.h>
#include <unistd.h>

namespace {
volatile sig_atomic_t violation_pipe=-1;
void execution_attempt(int) {
    const char marker='x';
    // Async-signal-safe and inherited across fork; the parent sees attempts
    // even when a collector ignores a helper child's failure.
    if(write(violation_pipe,&marker,1)!=1)_exit(98);
    _exit(99);
}
#if defined(__x86_64__)
constexpr auto architecture=AUDIT_ARCH_X86_64;
#elif defined(__aarch64__)
constexpr auto architecture=AUDIT_ARCH_AARCH64;
#endif
bool block_execution() {
#if defined(__x86_64__) || defined(__aarch64__)
    sock_filter filter[]={
        BPF_STMT(BPF_LD|BPF_W|BPF_ABS,offsetof(seccomp_data,arch)),
        BPF_JUMP(BPF_JMP|BPF_JEQ|BPF_K,architecture,1,0),
        BPF_STMT(BPF_RET|BPF_K,SECCOMP_RET_KILL_PROCESS),
        BPF_STMT(BPF_LD|BPF_W|BPF_ABS,offsetof(seccomp_data,nr)),
#ifdef __x86_64__
        BPF_STMT(BPF_ALU|BPF_AND|BPF_K,~0x40000000U), // Also reject x32 exec calls.
#endif
        BPF_JUMP(BPF_JMP|BPF_JEQ|BPF_K,__NR_execve,0,1),
        BPF_STMT(BPF_RET|BPF_K,SECCOMP_RET_TRAP),
        BPF_JUMP(BPF_JMP|BPF_JEQ|BPF_K,__NR_execveat,0,1),
        BPF_STMT(BPF_RET|BPF_K,SECCOMP_RET_TRAP),
        BPF_STMT(BPF_RET|BPF_K,SECCOMP_RET_ALLOW)
    };
    sock_fprog program{static_cast<unsigned short>(sizeof filter/sizeof *filter),filter};
    return prctl(PR_SET_NO_NEW_PRIVS,1,0,0,0)==0 &&
           prctl(PR_SET_SECCOMP,SECCOMP_MODE_FILTER,&program)==0;
#else
    return false;
#endif
}
}

int main() {
    alarm(20);
    const rlimit no_core{0,0};
    if(setrlimit(RLIMIT_CORE,&no_core)!=0)return 1;
    int violations[2];
    if(pipe2(violations,O_NONBLOCK|O_CLOEXEC)!=0)return 1;
    struct PipeOwner {int* fds;~PipeOwner(){close(fds[0]);close(fds[1]);}} pipe_owner{violations};
    violation_pipe=violations[1];
    struct sigaction handler{};
    handler.sa_handler=execution_attempt;
    sigemptyset(&handler.sa_mask);
    if(sigaction(SIGSYS,&handler,nullptr)!=0)return 1;
    // Prove the filter is effective before trusting a successful collection.
    const auto child=fork();
    if(child<0)return 1;
    if(child==0) {
        alarm(5);
        if(!block_execution())_exit(77);
        syscall(__NR_execve,"/pcinsight-noexec-calibration",nullptr,nullptr);
        _exit(1); // Even a nonexistent executable must trigger the filter.
    }
    int status{};
    pid_t waited;
    do {waited=waitpid(child,&status,0);} while(waited<0&&errno==EINTR);
    if(waited!=child)return 1;
    if(WIFEXITED(status)&&WEXITSTATUS(status)==77) {
        std::cout<<"Seccomp unavailable; no-exec collection not verified\n";
        return 77;
    }
    char marker{};
    if(!WIFEXITED(status)||WEXITSTATUS(status)!=99||
       read(violations[0],&marker,1)!=1||marker!='x'||!block_execution())return 1;
    pcinsight::Monitor monitor;
    pcinsight::SampleOptions options;
    options.include_app_history=true;
    options.include_services=options.include_startup_entries=options.include_user_sessions=true;
    monitor.sample(options);
    const auto snapshot=monitor.sample(options);
    if(snapshot.processes.empty()||snapshot.networks.empty())return 1;
    pcinsight::Sampler sampler(std::chrono::milliseconds(100),4,options);
    sampler.start();
    const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(5);
    while(sampler.running()&&sampler.timed_history().size()<2&&std::chrono::steady_clock::now()<deadline)
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    sampler.stop();
    const auto history=sampler.timed_history();
    if(sampler.running()||!sampler.last_error().empty()||history.size()<2)return 1;
    for(const auto& sample:history)if(sample.started_at>sample.finished_at)return 1;
    if(read(violations[0],&marker,1)!=-1||errno!=EAGAIN)return 1;
    std::cout<<"Background sampler produced history and stopped with execution blocked\n";
    std::cout<<"Collection completed with execve/execveat blocked; optional captures and actions not activated\n";
}
