#include "pcinsight/startup_impact.hpp"
#include "linux_process_stat.hpp"
#include <limits>
#ifdef __linux__
#include <cerrno>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/vfs.h>
#include <sys/syscall.h>
#include <poll.h>
#include <time.h>
#include <unistd.h>
#endif
namespace pcinsight {
namespace {
template<class T> bool usable(const Metric<T>& m) {return m.value && (m.support==Support::available || m.support==Support::estimated);}
template<class T> Metric<T> delta(const Metric<T>& before,const Metric<T>& after) {
    if(!usable(before) || !usable(after))
        return {std::nullopt,before.support==Support::permission_required || after.support==Support::permission_required?Support::permission_required:Support::unsupported,"counter unavailable in one/both samples"};
    if(*after.value<*before.value)return {std::nullopt,Support::unsupported,"counter decreased/reset; no fabricated delta"};
    return {*after.value-*before.value,Support::estimated,"observed process counter delta; partial startup coverage"};
}
#ifdef __linux__
struct Fd {int fd{-1};~Fd(){if(fd>=0)close(fd);}};
struct Read {std::optional<std::string> text;int error{};};
Read read_at(int dir,const char* name) {
    Fd file{openat(dir,name,O_RDONLY|O_CLOEXEC|O_NOFOLLOW|O_NONBLOCK)};
    if(file.fd<0)return {{},errno};
    std::string text;char buffer[4096];
    for(;;){
        const auto n=read(file.fd,buffer,sizeof buffer);
        if(n<0){if(errno==EINTR)continue;return {{},errno};}
        if(!n)break;
        if(static_cast<std::size_t>(n)>65536-text.size())return {{},EOVERFLOW};
        text.append(buffer,static_cast<std::size_t>(n));
    }
    return {std::move(text),0};
}
StartupProcessSampleResult failure(int code,const char* message) {
    return {std::nullopt,(code==EACCES || code==EPERM)?ActionError::permission_denied:
        (code==ENOENT || code==ESRCH)?ActionError::not_found:ActionError::system_error,message};
}
std::optional<std::chrono::nanoseconds> cpu_time(std::uint64_t ticks,std::uint64_t hz) {
    if(!hz)return {};
    constexpr auto max=static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max());
    const auto seconds=ticks/hz;if(seconds>max/1000000000)return {};
    const auto fraction=static_cast<std::uint64_t>(static_cast<long double>(ticks%hz)*1000000000/hz);
    const auto whole=seconds*1000000000;
    if(fraction>max-whole)return {};
    return std::chrono::nanoseconds(static_cast<std::int64_t>(whole+fraction));
}
#endif
}
StartupImpactMeasurementResult measure_startup_impact(const std::string& id,const StartupProcessSample& before,
    const StartupProcessSample& after,const StartupImpactPolicy& policy) {
    const auto earlier=before.sampled_at.time_since_epoch().count(),later=after.sampled_at.time_since_epoch().count();
    const bool interval_overflow=earlier<0 && later>std::numeric_limits<std::chrono::steady_clock::duration::rep>::max()+earlier;
    if(static_cast<unsigned>(before.cpu_counter)>static_cast<unsigned>(StartupCpuCounter::linux_scheduler_runtime)||
       static_cast<unsigned>(after.cpu_counter)>static_cast<unsigned>(StartupCpuCounter::linux_scheduler_runtime)||before.cpu_counter!=after.cpu_counter)
        return {std::nullopt,ActionError::invalid_argument,"CPU counter kinds differ/invalid; scheduler runtime and procfs ticks cannot be subtracted"};
    if(id.empty() || id.size()>32768 || id.find('\0')!=id.npos || !before.pid || !before.linux_start_ticks ||
        before.native_context.empty() || before.native_context.size()>1024 || before.native_context.find('\0')!=before.native_context.npos ||
        after.sampled_at<=before.sampled_at || interval_overflow || policy.medium_cpu.count()<=0 || policy.high_cpu<policy.medium_cpu ||
        !policy.medium_disk_bytes || policy.high_disk_bytes<policy.medium_disk_bytes ||
        (before.cumulative_cpu_time.value && before.cumulative_cpu_time.value->count()<0) ||
        (after.cumulative_cpu_time.value && after.cumulative_cpu_time.value->count()<0))
        return {std::nullopt,ActionError::invalid_argument,"invalid impact identity, chronology, counters or policy"};
    if(before.pid!=after.pid || before.linux_start_ticks!=after.linux_start_ticks || before.native_context!=after.native_context)
        return {std::nullopt,ActionError::stale_target,"process or native context changed; no impact attribution"};
    StartupImpactMeasurement m;m.startup_entry_id=id;m.pid=before.pid;m.linux_start_ticks=before.linux_start_ticks;
    m.observation_interval=after.sampled_at-before.sampled_at;
    m.cpu_time=delta(before.cumulative_cpu_time,after.cumulative_cpu_time);
    m.cpu_time.note=std::string(before.cpu_counter==StartupCpuCounter::linux_scheduler_runtime?"Linux scheduler-runtime group CPU: ":"Linux procfs CPU ticks: ")+m.cpu_time.note;
    m.disk_bytes=delta(before.cumulative_disk_bytes,after.cumulative_disk_bytes);
    // A valid high counter dominates unknown other counters; otherwise do not
    // under-classify an unobserved resource as medium/low.
    if((m.cpu_time.value && *m.cpu_time.value>policy.high_cpu) || (m.disk_bytes.value && *m.disk_bytes.value>policy.high_disk_bytes))
        m.impact={StartupImpact::high,Support::estimated,"high observed resource delta; caller-supplied association/window"};
    else if(m.cpu_time.value && m.disk_bytes.value)
        m.impact={(*m.cpu_time.value>=policy.medium_cpu || *m.disk_bytes.value>=policy.medium_disk_bytes)?StartupImpact::medium:StartupImpact::low,
            Support::estimated,"classification of observed resource deltas only; not complete boot impact"};
    else m.impact={std::nullopt,m.cpu_time.support==Support::permission_required || m.disk_bytes.support==Support::permission_required?Support::permission_required:Support::unsupported,
        "incomplete counters cannot certify low/medium impact"};
    return {std::move(m),ActionError::none,"measured interval only; no automatic startup association"};
}
static StartupProcessSampleResult read_process_sample(std::uint32_t pid,std::uint64_t expected,bool scheduler) {
    if(!pid || pid>static_cast<std::uint32_t>(std::numeric_limits<int>::max()) || !expected)
        return {std::nullopt,ActionError::invalid_argument,"invalid PID/start_ticks for startup sampling"};
#ifdef __linux__
    Fd root{open("/proc",O_RDONLY|O_DIRECTORY|O_CLOEXEC|O_NOFOLLOW)};
    if(root.fd<0)return failure(errno,"procfs unavailable");
    struct statfs type{};
    if(fstatfs(root.fd,&type)!=0)return failure(errno,"procfs type query failed");
    if(type.f_type!=0x9fa0)return {std::nullopt,ActionError::not_supported,"genuine Linux procfs required"};
    const auto self=read_at(root.fd,"self/stat");
    const auto own=self.text?detail::parse_linux_process_stat(*self.text):std::nullopt;
    if(!own || own->pid!=static_cast<std::uint32_t>(getpid()))return {std::nullopt,ActionError::not_supported,"procfs PID namespace differs from caller"};
    const auto boot=read_at(root.fd,"sys/kernel/random/boot_id");
    if(!boot.text)return failure(boot.error,"boot context unavailable");
    auto boot_id=*boot.text;if(!boot_id.empty() && boot_id.back()=='\n')boot_id.pop_back();
    if(boot_id.size()!=36)return {std::nullopt,ActionError::not_supported,"boot context invalid"};
    for(const auto c:boot_id)if(!((c>='a'&&c<='f') || (c>='0'&&c<='9') || c=='-'))
        return {std::nullopt,ActionError::not_supported,"boot context invalid"};
    struct stat ns{};if(fstatat(root.fd,"self/ns/pid",&ns,0)!=0)return failure(errno,"PID namespace context unavailable");
    Fd selected;
    auto alive=[&]{pollfd p{selected.fd,POLLIN,0};const auto rc=::poll(&p,1,0);
        if(rc<0)return errno;
        if(p.revents&~(POLLIN|POLLHUP))return EIO;
        return rc?ESRCH:0;
    };
    if(scheduler){
#ifdef SYS_pidfd_open
        selected.fd=static_cast<int>(::syscall(SYS_pidfd_open,pid,0));
        if(selected.fd<0){if(errno==ENOSYS)return {std::nullopt,ActionError::not_supported,"Scheduler sampling requires pidfd_open"};return failure(errno,"Scheduler pidfd open failed");}
        if(const auto error=alive())return failure(error,"Scheduler pidfd guard unavailable/exited before sampling");
#else
        return {std::nullopt,ActionError::not_supported,"Scheduler sampling requires pidfd_open"};
#endif
    }
    Fd process{openat(root.fd,std::to_string(pid).c_str(),O_RDONLY|O_DIRECTORY|O_CLOEXEC|O_NOFOLLOW)};
    if(process.fd<0)return failure(errno,"startup process not accessible");
    const auto first=read_at(process.fd,"stat");
    if(!first.text)return failure(first.error,"startup process stat unavailable");
    const auto stat=detail::parse_linux_process_stat(*first.text);
    if(!stat)return {std::nullopt,ActionError::system_error,"startup process stat malformed"};
    if(stat->pid!=pid || stat->start_ticks!=expected)return {std::nullopt,ActionError::stale_target,"startup process identity changed"};
    const auto io=read_at(process.fd,"io");
    Metric<std::chrono::nanoseconds> scheduler_cpu;
    if(scheduler){
        clockid_t clock{};const auto code=::clock_getcpuclockid(static_cast<pid_t>(pid),&clock);timespec time{};
        // Linux process CPUCLOCK_SCHED encoding, independently checked rather
        // than trusting a libc to supply a different CPU counter silently.
        const auto encoded=(~pid<<3)|2U;
        int error=code;
        if(!error&&(sizeof(clock)!=4||static_cast<std::uint32_t>(clock)!=encoded||pid>0x1fffffffU))error=ENOTSUP;
        if(!error&&::clock_gettime(clock,&time)<0)error=errno;
        constexpr auto max=static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max());
        if(!error&&time.tv_sec>=0&&time.tv_nsec>=0&&time.tv_nsec<1000000000&&static_cast<std::uint64_t>(time.tv_sec)<=max/1000000000ULL){
            const auto whole=static_cast<std::uint64_t>(time.tv_sec)*1000000000ULL;
            if(static_cast<std::uint64_t>(time.tv_nsec)<=max-whole)scheduler_cpu={std::chrono::nanoseconds(static_cast<std::int64_t>(whole+time.tv_nsec)),Support::estimated,"POSIX Linux process CPUCLOCK_SCHED; all group threads, excludes waited-child CPU"};
        }
        if(!scheduler_cpu.value)scheduler_cpu={{},error==EPERM||error==EACCES?Support::permission_required:Support::unsupported,"Compatible Linux process scheduler CPU clock unavailable/overflow"};
    }
    const auto last=read_at(process.fd,"stat");
    if(!last.text)return failure(last.error,"process vanished during sampling");
    const auto final=detail::parse_linux_process_stat(*last.text);
    if(!final || final->pid!=pid || final->start_ticks!=expected || final->ticks<stat->ticks)
        return {std::nullopt,ActionError::stale_target,"startup process changed during sampling"};
    if(scheduler)if(const auto error=alive())return failure(error,"Scheduler pidfd guard unavailable/exited during sampling; no PID-reuse clock publication");
    StartupProcessSample s;s.pid=pid;s.linux_start_ticks=expected;
    s.native_context=boot_id+":"+std::to_string(ns.st_dev)+":"+std::to_string(ns.st_ino);
    s.sampled_at=std::chrono::steady_clock::now();
    const auto hz=sysconf(_SC_CLK_TCK);
    const auto time=hz>0?cpu_time(final->ticks,static_cast<std::uint64_t>(hz)):std::nullopt;
    s.cumulative_cpu_time={time,time?Support::estimated:Support::unsupported,"procfs CPU ticks, quantized; no guest time double-count or child CPU sum"};
    if(scheduler){s.cumulative_cpu_time=std::move(scheduler_cpu);s.cpu_counter=StartupCpuCounter::linux_scheduler_runtime;}
    if(io.text) {
        const auto bytes=detail::parse_linux_process_io(*io.text);
        s.cumulative_disk_bytes={bytes,bytes?Support::estimated:Support::unsupported,"procfs read_bytes+write_bytes; includes kernel accounting, not application logical I/O"};
    } else s.cumulative_disk_bytes={std::nullopt,(io.error==EACCES || io.error==EPERM)?Support::permission_required:Support::unsupported,"process I/O counter unavailable"};
    return {std::move(s),ActionError::none,"read-only process counter sample; no startup association inferred"};
#else
    (void)scheduler;
    return {std::nullopt,ActionError::not_supported,"startup process counter provider requires Linux"};
#endif
}
StartupProcessSampleResult read_startup_process_sample(std::uint32_t pid,std::uint64_t expected){return read_process_sample(pid,expected,false);}
StartupProcessSampleResult read_startup_scheduler_process_sample(std::uint32_t pid,std::uint64_t expected){return read_process_sample(pid,expected,true);}
}
