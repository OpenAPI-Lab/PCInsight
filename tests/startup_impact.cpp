#include "pcinsight/startup_impact.hpp"
#include "pcinsight/monitor.hpp"
#include <cassert>
#include <limits>
#include <thread>
#ifdef __linux__
#include <cerrno>
#include <csignal>
#include <cstdlib>
#include <filesystem>
#include <fcntl.h>
#include <poll.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#endif
using namespace pcinsight;
using namespace std::chrono_literals;
int main() {
    StartupProcessSample first;first.pid=10;first.linux_start_ticks=100;first.native_context="boot:namespace";
    first.sampled_at=std::chrono::steady_clock::time_point(1s);
    first.cumulative_cpu_time={0ns,Support::available,"zero"};first.cumulative_disk_bytes={0,Support::available,"zero"};
    auto last=first;last.sampled_at+=1s;
    auto measure=[&]{return measure_startup_impact("selected.desktop",first,last);};
    auto result=measure();assert(result && result.value->impact.value==StartupImpact::low && result.value->impact.support==Support::estimated);
    last.cumulative_cpu_time.value=300ms;assert(measure().value->impact.value==StartupImpact::medium);
    last.cumulative_cpu_time.value=1s;assert(measure().value->impact.value==StartupImpact::medium);
    last.cumulative_cpu_time.value=1s+1ns;assert(measure().value->impact.value==StartupImpact::high);
    last.cumulative_cpu_time.value=0ns;last.cumulative_disk_bytes.value=300000;assert(measure().value->impact.value==StartupImpact::medium);
    last.cumulative_disk_bytes.value=3000000;assert(measure().value->impact.value==StartupImpact::medium);
    last.cumulative_disk_bytes.value=3000001;assert(measure().value->impact.value==StartupImpact::high);
    last.cumulative_disk_bytes={std::nullopt,Support::permission_required,"denied"};
    result=measure();assert(result && !result.value->impact.value && result.value->impact.support==Support::permission_required);
    last.cumulative_cpu_time.value=1s+1ns;assert(measure().value->impact.value==StartupImpact::high);
    last.cumulative_cpu_time={1s+1ns,Support::unsupported,"invalid provenance"};assert(!measure().value->impact.value);
    last=first;last.sampled_at+=1s;first.cumulative_cpu_time.value=100ms;
    assert(!measure().value->cpu_time.value && !measure().value->impact.value); // no negative/reset delta
    first.cumulative_cpu_time.value=0ns;
    last.pid=11;assert(measure().error==ActionError::stale_target);last.pid=first.pid;
    last.linux_start_ticks=101;assert(measure().error==ActionError::stale_target);last.linux_start_ticks=first.linux_start_ticks;
    last.native_context="other boot";assert(measure().error==ActionError::stale_target);last.native_context=first.native_context;
    last.sampled_at=first.sampled_at;assert(measure().error==ActionError::invalid_argument);last.sampled_at+=1s;
    last.cumulative_cpu_time.value=-1ns;assert(measure().error==ActionError::invalid_argument);last.cumulative_cpu_time.value=0ns;
    assert(!measure_startup_impact("",first,last));
    StartupImpactPolicy invalid;invalid.high_cpu=1ms;assert(!measure_startup_impact("id",first,last,invalid));
    invalid={};invalid.high_disk_bytes=1;assert(!measure_startup_impact("id",first,last,invalid));
    auto min=first,max=last;min.sampled_at=std::chrono::steady_clock::time_point::min();max.sampled_at=std::chrono::steady_clock::time_point::max();
    assert(!measure_startup_impact("id",min,max));
    assert(!read_startup_process_sample(0,0));
    assert(!read_startup_scheduler_process_sample(0,0));
    auto different=last;different.cpu_counter=StartupCpuCounter::linux_scheduler_runtime;
    assert(measure_startup_impact("kind",first,different).error==ActionError::invalid_argument);
    auto scheduler_first=first;scheduler_first.cpu_counter=StartupCpuCounter::linux_scheduler_runtime;
    assert(measure_startup_impact("kind",scheduler_first,different).value->cpu_time.note.find("scheduler-runtime")!=std::string::npos);
    different.cpu_counter=static_cast<StartupCpuCounter>(255);assert(!measure_startup_impact("kind",scheduler_first,different));
#ifdef __linux__
    namespace fs=std::filesystem;
    char path[]="/tmp/pcinsight-impact-XXXXXX";const auto* dir=mkdtemp(path);assert(dir);const fs::path root=dir;
    struct Cleanup {fs::path p;~Cleanup(){std::error_code ec;fs::remove_all(p,ec);}} cleanup{root};
    int control[2],done[2];assert(pipe2(control,O_CLOEXEC)==0 && pipe2(done,O_CLOEXEC)==0);
    const auto file=open((root/"fixture.data").c_str(),O_WRONLY|O_CREAT|O_EXCL|O_CLOEXEC,0600);assert(file>=0);
    const auto pid=fork();assert(pid>=0);
    if(!pid) {
        close(control[1]);close(done[0]);char command{};
        // A worker has already died BEFORE the baseline. Group scheduler CPU
        // must include its runtime; reading only the leader would omit it.
        std::thread warmup([]{timespec a{},b{};if(clock_gettime(CLOCK_THREAD_CPUTIME_ID,&a))_exit(10);
            do{if(clock_gettime(CLOCK_THREAD_CPUTIME_ID,&b))_exit(11);}while((b.tv_sec-a.tv_sec)*1000000000LL+b.tv_nsec-a.tv_nsec<20000000LL);});
        warmup.join();if(write(done[1],"b",1)!=1)_exit(12);
        if(read(control[0],&command,1)!=1)_exit(2);
        timespec start{},now{};if(clock_gettime(CLOCK_PROCESS_CPUTIME_ID,&start)!=0)_exit(3);
        std::uint64_t value=1;volatile std::uint64_t sink{};
        do {for(unsigned i=0;i<10000;++i)value=value*6364136223846793005ULL+1;sink=value;
            if(clock_gettime(CLOCK_PROCESS_CPUTIME_ID,&now)!=0)_exit(4);
        } while((now.tv_sec-start.tv_sec)*1000000000LL+now.tv_nsec-start.tv_nsec<1100000000LL);
        (void)sink;char block[4096]{};
        for(unsigned i=0;i<1024;++i)if(write(file,block,sizeof block)!=sizeof block)_exit(5);
        if(fsync(file)!=0)_exit(6);close(file);
        if(write(done[1],"d",1)!=1)_exit(7);
        if(read(control[0],&command,1)!=1)_exit(8);
        _exit(0);
    }
    close(control[0]);close(done[1]);close(file);
    struct Child {pid_t pid;~Child(){if(pid>0){kill(pid,SIGKILL);while(waitpid(pid,nullptr,0)<0 && errno==EINTR){}}}} child{pid};
    Monitor monitor;SampleOptions options;options.include_disks=options.include_networks=options.include_gpus=false;
    const auto snapshot=monitor.sample(options);std::uint64_t ticks=0;
    for(const auto& process:snapshot.processes)if(process.pid==static_cast<std::uint32_t>(pid))ticks=process.linux_start_ticks.value.value_or(0);
    assert(ticks);
    pollfd warm{done[0],POLLIN,0};assert(poll(&warm,1,10000)>0);char baseline_ready{};assert(read(done[0],&baseline_ready,1)==1&&baseline_ready=='b');
    const auto before=read_startup_process_sample(pid,ticks);assert(before && before.value->cumulative_cpu_time.value && before.value->cumulative_disk_bytes.value);
    const auto scheduler_before=read_startup_scheduler_process_sample(pid,ticks);assert(scheduler_before&&scheduler_before.value->cpu_counter==StartupCpuCounter::linux_scheduler_runtime&&metric_is_known(scheduler_before.value->cumulative_cpu_time));
    assert(*scheduler_before.value->cumulative_cpu_time.value>=20ms);
    assert(read_startup_scheduler_process_sample(pid,ticks+1).error==ActionError::stale_target);
    assert(read_startup_process_sample(pid,ticks+1).error==ActionError::stale_target);
    assert(write(control[1],"g",1)==1);
    pollfd ready{done[0],POLLIN,0};assert(poll(&ready,1,10000)>0 && (ready.revents&POLLIN));char command{};assert(read(done[0],&command,1)==1);
    const auto after=read_startup_process_sample(pid,ticks);assert(after);
    const auto scheduler_after=read_startup_scheduler_process_sample(pid,ticks);assert(scheduler_after);
    const auto scheduler_measured=measure_startup_impact("scheduler-fixture",*scheduler_before.value,*scheduler_after.value);
    assert(scheduler_measured&&scheduler_measured.value->cpu_time.value&&*scheduler_measured.value->cpu_time.value>1s&&scheduler_measured.value->impact.value==StartupImpact::high);
    assert(!measure_startup_impact("mixed-fixture",*before.value,*scheduler_after.value));
    const auto measured=measure_startup_impact("fixture-startup-id",*before.value,*after.value);assert(measured);
    assert(measured.value->cpu_time.value && *measured.value->cpu_time.value>1s);
    assert(measured.value->disk_bytes.value && *measured.value->disk_bytes.value>=4194304);
    assert(measured.value->impact.value==StartupImpact::high && measured.value->observation_interval>0s);
    assert(write(control[1],"q",1)==1);int status{};assert(waitpid(pid,&status,0)==pid && WIFEXITED(status) && WEXITSTATUS(status)==0);child.pid=-1;
    close(control[1]);close(done[0]);
    assert(read_startup_process_sample(pid,ticks).error==ActionError::not_found);
    assert(read_startup_scheduler_process_sample(pid,ticks).error==ActionError::not_found);
#else
    assert(read_startup_process_sample(10,100).error==ActionError::not_supported);
#endif
}
