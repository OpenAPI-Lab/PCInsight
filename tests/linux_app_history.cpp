#include "../src/linux_app_history.hpp"
#include "pcinsight/monitor.hpp"
#include <algorithm>
#include <cassert>
#include <climits>
#include <iostream>
#include <limits>
#ifdef __linux__
#include <unistd.h>
#endif
using namespace pcinsight;
using namespace pcinsight::detail;
using Clock=std::chrono::steady_clock;
using namespace std::chrono_literals;
LinuxCpuCounters context(){LinuxCpuCounters c;c.fields=4;c.cpu_ids={0};c.boot_time=1000;return c;}
LinuxHistoryObservation row(std::uint64_t ticks,int second,std::uint32_t pid=123,std::uint64_t start=10,std::string path="/app"){
    LinuxProcessStat s;s.pid=pid;s.start_ticks=start;s.user_ticks=ticks;s.ticks=ticks;
    LinuxExecutableInfo exe{{path,Support::estimated,"path"},{"x64",Support::available,"ELF"},{1,2,64,0100755,100,0,200,0}};
    return {pid,{s,Support::available,"stat"},{exe,Support::estimated,"exe"},"app",Clock::time_point{}+std::chrono::seconds(second)};
}
auto sample(LinuxAppHistory& h,const LinuxHistoryObservation& r,long hz=3){return h.sample(context(),hz,std::span(&r,1),std::chrono::system_clock::time_point{}+1h+std::chrono::seconds(std::chrono::duration_cast<std::chrono::seconds>(r.at.time_since_epoch()).count()));}
const LinuxAppHistoryEntry& app(const Metric<std::vector<LinuxAppHistoryEntry>>& m,std::string path="/app"){if(!m.value)std::cerr<<"Required app-history report absent for "<<path<<": "<<m.note<<std::endl;assert(m.value);const auto i=std::find_if(m.value->begin(),m.value->end(),[&](const auto& e){return e.id==path;});assert(i!=m.value->end());return *i;}
int main(){
    LinuxAppHistory h;auto result=sample(h,row(0,1));assert(!app(result).observed_cpu_time.value&&!app(result).network_bytes.value&&!app(result).notifications.value);
    result=sample(h,row(1,2));assert(app(result).observed_cpu_time.value==333333333ns);
    result=sample(h,row(2,3));assert(app(result).observed_cpu_time.value==666666666ns);
    result=sample(h,row(3,4));assert(app(result).observed_cpu_time.value==1s);
    result=sample(h,row(3,4));assert(app(result).observed_cpu_time.value==1s); // repeated snapshot, no double count
    result=sample(h,row(3,5));assert(app(result).observed_cpu_time.value==1s);
    assert(app(result).observed_cpu_time.support==Support::estimated);
    h.reset();sample(h,row(0,1),7);for(int n=1;n<=7;++n)result=sample(h,row(n,n+1),7);assert(app(result).observed_cpu_time.value==1s);
    h.reset();sample(h,row(0,1),LONG_MAX);for(int n=1;n<=3;++n)result=sample(h,row(n,n+1),LONG_MAX);assert(app(result).observed_cpu_time.value==std::chrono::nanoseconds(3000000000ULL/static_cast<std::uint64_t>(LONG_MAX)));
    h.reset();std::vector<LinuxHistoryObservation> rows{row(0,1),row(0,1,124)};h.sample(context(),3,rows,std::chrono::system_clock::time_point{}+1h);
    rows={row(1,2),row(2,2,124)};result=h.sample(context(),3,rows,std::chrono::system_clock::time_point{}+2h);assert(app(result).observed_cpu_time.value==1s); // grouped processes, shared exact remainder
    for(int mode=0;mode<8;++mode){
        h.reset();sample(h,row(0,1));auto bad=row(100,2);
        if(mode==0)bad.counters.value->start_ticks=11;if(mode==1)++bad.executable.value->stamp.inode;if(mode==2)bad.executable.value->path.value="/other";
        if(mode==3)bad.counters={{},Support::permission_required,"denied"};if(mode==4)bad.executable={{},Support::unsupported,"missing"};if(mode==5)bad.at=Clock::time_point{};if(mode==6)++bad.counters.value->ticks;if(mode==7)bad.counters.value->pid=124;
        const auto bad_report=sample(h,bad);if(bad_report.value)for(const auto& e:*bad_report.value)assert(!e.observed_cpu_time.value);
        // Restore original identity, and then consume only a new interval.
        result=sample(h,row(101,3));if(mode!=3&&mode!=4&&mode!=6&&mode!=7&&mode!=5)assert(!app(result).observed_cpu_time.value);
    }
    h.reset();auto a=row(10,1);a.counters.value->kernel_ticks=10;a.counters.value->ticks=20;sample(h,a);
    auto b=row(9,2);b.counters.value->kernel_ticks=20;b.counters.value->ticks=29;assert(!app(sample(h,b)).observed_cpu_time.value); // masked individual reset
    h.reset();sample(h,row(0,1));h.sample(context(),3,{},std::chrono::system_clock::time_point{}+1h+2s);assert(!app(sample(h,row(100,3))).observed_cpu_time.value);
    h.break_continuity();assert(!app(sample(h,row(101,4))).observed_cpu_time.value);assert(app(sample(h,row(102,5))).observed_cpu_time.value==333333333ns);
    h.reset();sample(h,row(0,1));auto reverse=row(3,0);assert(!app(h.sample(context(),3,std::span(&reverse,1),std::chrono::system_clock::time_point{}+1h+2s)).observed_cpu_time.value);assert(app(sample(h,row(4,3))).observed_cpu_time.value==333333333ns);
    for(int mode=0;mode<6;++mode){
        h.reset();sample(h,row(0,1));auto r=row(1,2);auto c=std::optional{context()};long hz=3;auto now=std::chrono::system_clock::time_point{}+2h;
        if(mode==0)c.reset();if(mode==1)hz=0;if(mode==2)hz=4;if(mode==3)c->boot_time=1001;if(mode==4)now=std::chrono::system_clock::time_point{};if(mode==5)r.name=std::string(16385,'x');
        const auto invalid=h.sample(c,hz,std::span(&r,1),now);
        assert(!invalid.value);if(mode==4)assert(invalid.note.find("positive wall-clock timestamp")!=std::string::npos);
        assert(!app(sample(h,row(2,3))).observed_cpu_time.value);assert(app(sample(h,row(3,4))).observed_cpu_time.value==333333333ns);
    }
    // A backward jump to a valid post-epoch date breaks the CPU baseline,
    // but must not erase previously committed CPU or bridge the missed interval.
    h.reset();sample(h,row(0,1));assert(app(sample(h,row(1,2))).observed_cpu_time.value==333333333ns);
    const auto after_jump=row(2,3);
    const auto rejected=h.sample(context(),3,std::span(&after_jump,1),std::chrono::system_clock::time_point{}+1min);
    assert(!rejected.value&&rejected.support==Support::unsupported);
    assert(rejected.note.find("wall clock moved backward")!=std::string::npos);
    assert(app(sample(h,row(3,4))).observed_cpu_time.value==333333333ns);
    assert(app(sample(h,row(4,5))).observed_cpu_time.value==666666666ns);
    for(int mode=0;mode<3;++mode){h.reset();sample(h,row(0,1));std::vector<LinuxHistoryObservation> bad{row(1,2)};if(mode==0)bad.push_back(bad.front());if(mode==1)bad.front().pid=0;if(mode==2)bad.resize(100001);const auto invalid=h.sample(context(),3,bad,std::chrono::system_clock::time_point{}+2h);assert(!invalid.value);if(mode==2)assert(invalid.note.find("exceeds 100000 observations")!=std::string::npos);}
    h.reset();sample(h,row(0,1));sample(h,row(1,2));std::vector<LinuxHistoryObservation> excess;
    for(unsigned n=0;n<4097;++n)excess.push_back(row(0,3,1000+n,10,"/app/"+std::to_string(n)));
    assert(!h.sample(context(),3,excess,std::chrono::system_clock::time_point{}+2h).value);assert(app(sample(h,row(2,3))).observed_cpu_time.value==333333333ns); // failed update preserves committed CPU
    h.reset();sample(h,row(0,1),1);assert(!sample(h,row(std::numeric_limits<std::uint64_t>::max(),2),1).value);assert(!app(sample(h,row(1,3),1)).observed_cpu_time.value);
    const auto max=static_cast<std::uint64_t>(std::numeric_limits<std::chrono::nanoseconds::rep>::max());
    h.reset();sample(h,row(0,1),1000000000);assert(app(sample(h,row(max,2),1000000000)).observed_cpu_time.value==std::chrono::nanoseconds(max));assert(!sample(h,row(max+1,3),1000000000).value);assert(app(sample(h,row(max+1,4),1000000000)).observed_cpu_time.value==std::chrono::nanoseconds(max));
#ifdef __linux__
    const auto identity=read_linux_process_identity("/proc/self");assert(identity.value);const auto executable=read_linux_process_executable("/proc/self",*identity.value);assert(executable.value&&executable.value->path.value);const auto path=*executable.value->path.value;
    SampleOptions options;options.include_app_history=true;options.include_processes=false;options.include_linux_process_descriptions=false;options.include_disks=false;options.include_networks=false;options.include_gpus=false;options.include_services=false;options.include_startup_entries=false;options.include_user_sessions=false;
    Monitor monitor;auto snapshot=monitor.sample(options);assert(snapshot.processes.empty()&&snapshot.app_history.empty());assert(!app(snapshot.linux_app_history,path).observed_cpu_time.value);
    std::uint64_t work=1;bool positive=false;std::chrono::nanoseconds cpu{};
    for(unsigned attempt=0;attempt<5&&!positive;++attempt){const auto until=Clock::now()+100ms;while(Clock::now()<until)for(unsigned n=0;n<10000;++n)work=work*6364136223846793005ULL+1;snapshot=monitor.sample(options);const auto& own=app(snapshot.linux_app_history,path);assert(!own.network_bytes.value&&!own.notifications.value);if(own.observed_cpu_time.value&&own.observed_cpu_time.value->count()>0){positive=true;cpu=*own.observed_cpu_time.value;}}
    assert(positive);options.include_app_history=false;snapshot=monitor.sample(options);assert(!snapshot.linux_app_history.value&&snapshot.app_history.empty());options.include_app_history=true;snapshot=monitor.sample(options);assert(!app(snapshot.linux_app_history,path).observed_cpu_time.value);
    std::cout<<"Native own PID="<<::getpid()<<" observed CPU="<<cpu.count()<<"ns, hidden process list, baseline/enable/disable/reset and unknown network/notifications passed; work="<<work<<'\n';
#endif
    std::cout<<"Linux app history: exact fractional ticks, identity/exe continuity, gaps, duplicates/bounds, units/boot/time validation and atomic overflow rejection passed\n";
}
