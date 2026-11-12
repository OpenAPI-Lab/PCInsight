#include "pcinsight/startup_impact.hpp"
#include "pcinsight/startup_observer.hpp"
#include "pcinsight/monitor.hpp"
#include <cassert>
#include <limits>
#include <thread>
#ifdef __linux__
#include <cerrno>
#include <csignal>
#include <fcntl.h>
#include <poll.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#endif
using namespace pcinsight;
using namespace std::chrono_literals;
int main() {
    assert(read_startup_child_exit_sample(0,1).error==ActionError::invalid_argument);
    assert(read_startup_child_exit_sample(1,0).error==ActionError::invalid_argument);
    assert(read_startup_child_exit_sample(std::numeric_limits<std::uint32_t>::max(),1).error==ActionError::invalid_argument);
    StartupProcessSample baseline;baseline.pid=10;baseline.linux_start_ticks=100;baseline.native_context="fixture";
    baseline.sampled_at=std::chrono::steady_clock::time_point(1s);
    baseline.cumulative_cpu_time={0ns,Support::available,"known"};baseline.cumulative_disk_bytes={0,Support::available,"known"};
    StartupImpactObserver observer;
    assert(observer.register_process("app",baseline));auto other=baseline;other.pid=11;
    assert(observer.register_process("other",other));
    auto invalid=baseline;invalid.linux_start_ticks++;invalid.sampled_at+=1ms;
    assert(observer.finish_process(invalid).error==ActionError::stale_target);
    assert(!observer.finish_process(baseline));assert(!observer.observations()[0].closed);
    auto final=baseline;final.sampled_at+=50ms;final.cumulative_cpu_time.value=20ms;
    assert(observer.finish_process(final));auto rows=observer.observations();
    assert(rows[0].closed && !rows[0].window_complete && rows[0].sample_count==2);
    assert(rows[0].measurement->cpu_time.value==20ms && !rows[0].measurement->impact.value);
    assert(!rows[1].closed && !observer.finish_process(final));
    observer.close();assert(!observer.finish_process(other));
#ifdef __linux__
    int control[2];assert(pipe2(control,O_CLOEXEC)==0);
    const auto pid=fork();assert(pid>=0);
    if(!pid) {
        close(control[1]);char c{};if(read(control[0],&c,1)!=1)_exit(2);
        timespec start{},now{};if(clock_gettime(CLOCK_PROCESS_CPUTIME_ID,&start))_exit(3);
        volatile std::uint64_t sink{};std::uint64_t value=1;
        do {for(unsigned i=0;i<10000;++i)value=value*6364136223846793005ULL+1;sink=value;
            if(clock_gettime(CLOCK_PROCESS_CPUTIME_ID,&now))_exit(4);
        } while((now.tv_sec-start.tv_sec)*1000000000LL+now.tv_nsec-start.tv_nsec<50000000LL);
        (void)sink;close(control[0]);_exit(7);
    }
    close(control[0]);
    struct Child {pid_t pid;~Child(){if(pid>0){kill(pid,SIGKILL);while(waitpid(pid,nullptr,0)<0 && errno==EINTR){}}}} child{pid};
    Monitor monitor;SampleOptions options;options.include_disks=options.include_networks=options.include_gpus=false;
    auto snapshot=monitor.sample(options);std::uint64_t ticks=0,own_ticks=0;
    for(const auto& p:snapshot.processes) {
        if(p.pid==static_cast<std::uint32_t>(pid))ticks=p.linux_start_ticks.value.value_or(0);
        if(p.pid==static_cast<std::uint32_t>(getpid()))own_ticks=p.linux_start_ticks.value.value_or(0);
    }
    assert(ticks && own_ticks);
    assert(read_startup_child_exit_sample(pid,ticks+1).error==ActionError::stale_target);
    assert(read_startup_child_exit_sample(getpid(),own_ticks).error==ActionError::not_supported);
    auto pending=read_startup_child_exit_sample(pid,ticks);
    assert(pending && !pending.exited && !pending.sample && !pending.exit_code && !pending.termination_signal);
    const auto before=read_startup_process_sample(pid,ticks);assert(before);
    StartupImpactObserver native;assert(native.register_process("private-child-not-certified-startup",*before.value));
    assert(write(control[1],"g",1)==1);close(control[1]);
    // Wait for exit readiness without consuming it or taking another live sample.
    // Only test code uses waitid(P_PID); library uses stable pidfd exclusively.
    siginfo_t info{};
    const auto deadline=std::chrono::steady_clock::now()+5s;
    do {
        assert(waitid(P_PID,pid,&info,WEXITED|WNOHANG|WNOWAIT)==0);
        if(!info.si_pid)std::this_thread::sleep_for(1ms);
    }while(!info.si_pid && std::chrono::steady_clock::now()<deadline);
    assert(info.si_pid==pid);
    auto exited=read_startup_child_exit_sample(pid,ticks);
    assert(exited && exited.exited && exited.sample && exited.exit_code==7 && !exited.termination_signal);
    assert(exited.sample->cumulative_cpu_time.value && *exited.sample->cumulative_cpu_time.value>*before.value->cumulative_cpu_time.value);
    assert(native.finish_process(*exited.sample));rows=native.observations();
    assert(rows[0].closed && rows[0].measurement->cpu_time.value && *rows[0].measurement->cpu_time.value>0ns);
    assert(!rows[0].window_complete && !rows[0].measurement->impact.value);
    auto repeated=read_startup_child_exit_sample(pid,ticks);
    assert(repeated && repeated.sample && repeated.exit_code==7);
    assert(repeated.sample->cumulative_cpu_time.value==exited.sample->cumulative_cpu_time.value);
    assert(!native.finish_process(*repeated.sample));
    int status{};assert(waitpid(pid,&status,0)==pid && WIFEXITED(status) && WEXITSTATUS(status)==7);child.pid=-1;
    assert(!read_startup_child_exit_sample(pid,ticks)); // no recovery after reaping
    const auto signaled_pid=fork();assert(signaled_pid>=0);
    if(!signaled_pid){for(;;)pause();}
    Child signaled{signaled_pid};snapshot=monitor.sample(options);ticks=0;
    for(const auto& p:snapshot.processes)if(p.pid==static_cast<std::uint32_t>(signaled_pid))ticks=p.linux_start_ticks.value.value_or(0);
    assert(ticks && kill(signaled_pid,SIGTERM)==0);
    info={};const auto signal_deadline=std::chrono::steady_clock::now()+5s;
    do {
        assert(waitid(P_PID,signaled_pid,&info,WEXITED|WNOHANG|WNOWAIT)==0);
        if(!info.si_pid)std::this_thread::sleep_for(1ms);
    }while(!info.si_pid && std::chrono::steady_clock::now()<signal_deadline);
    assert(info.si_pid==signaled_pid);
    exited=read_startup_child_exit_sample(signaled_pid,ticks);
    assert(exited && exited.exited && exited.sample && !exited.exit_code && exited.termination_signal==SIGTERM);
    assert(waitpid(signaled_pid,&status,0)==signaled_pid && WIFSIGNALED(status) && WTERMSIG(status)==SIGTERM);
    signaled.pid=-1;
#else
    assert(read_startup_child_exit_sample(10,100).error==ActionError::not_supported);
#endif
}
