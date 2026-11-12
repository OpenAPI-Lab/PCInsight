#include "../src/linux_process_cpu.hpp"
#include "../src/linux_process_details.hpp"
#include "pcinsight/monitor.hpp"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
#ifdef __linux__
#include <time.h>
#include <unistd.h>
#endif
using namespace pcinsight;
using namespace pcinsight::detail;
using Clock=std::chrono::steady_clock;
using namespace std::chrono_literals;
LinuxCpuCounters context(std::uint64_t advance=0){LinuxCpuCounters c;c.fields=4;c.cpu_ids={0,1,2,3};c.boot_time=1000;c.ticks={100+advance,100,100,100,0,0,0,0};return c;}
LinuxProcessCpuObservation row(std::uint64_t u,std::uint64_t s,int second,std::uint64_t start=10){LinuxProcessStat stat;stat.pid=123;stat.start_ticks=start;stat.user_ticks=u;stat.kernel_ticks=s;stat.ticks=u+s;return {123,{stat,Support::available,"fixture"},Clock::time_point{}+std::chrono::seconds(second)};}
Metric<double> take(LinuxProcessCpuTracker& t,const LinuxCpuCounters& c,const LinuxProcessCpuObservation& r,long hz=100){return t.sample(c,hz,std::span(&r,1)).at(r.pid);}
int main(){
    LinuxProcessCpuTracker t;
    assert(!take(t,context(),row(0,0,1)).value);
    auto metric=take(t,context(400),row(100,100,2));assert(metric.value==50&&metric.support==Support::estimated);
    assert(take(t,context(800),row(100,100,3)).value==0);
    assert(take(t,context(1200),row(500,100,4)).value==100);
    assert(!take(t,context(1600),row(500,100,5,11)).value); // reused PID
    assert(take(t,context(2000),row(600,100,6,11)).value==25);
    assert(!take(t,context(2400),row(500,300,7,11)).value); // user reset masked by growing sum
    assert(!take(t,context(2800),row(800,200,8,11)).value); // kernel reset
    assert(!take(t,context(3200),row(1800,200,9,11)).value); // >100: unknown, not clamp
    assert(take(t,context(3600),row(1900,200,10,11)).value==25);
    // Independent per-process elapsed intervals despite a shared batch context.
    t.reset();std::vector<LinuxProcessCpuObservation> rows{row(0,0,1),row(0,0,2)};rows[1].pid=124;rows[1].counters.value->pid=124;t.sample(context(),100,rows);
    rows[0]=row(100,0,3);rows[1]=row(100,0,3);rows[1].pid=124;rows[1].counters.value->pid=124;const auto rates=t.sample(context(400),100,rows);assert(rates.at(123).value==12.5&&rates.at(124).value==25);
    for(int mode=0;mode<8;++mode){
        t.reset();take(t,context(),row(0,0,1));auto c=context(400);auto r=row(10,10,2);long hz=100;
        if(mode==0)c.cpu_ids={0,1};if(mode==1)c.boot_time=1001;if(mode==2)c.fields=5;if(mode==3)c.ticks[1]=99;if(mode==4)c=context();if(mode==5)hz=250;if(mode==6)r.at=Clock::time_point{}+1s;if(mode==7)r.at=Clock::time_point{};
        assert(!take(t,c,r,hz).value);
    }
    for(int mode=0;mode<10;++mode){
        t.reset();take(t,context(),row(0,0,1));auto r=row(10,10,2);auto c=std::optional{context(400)};long hz=100;
        if(mode==0)c.reset();if(mode==1)hz=0;if(mode==2)hz=-1;if(mode==3)c->cpu_ids.clear();if(mode==4)r.counters={{},Support::permission_required,"denied"};if(mode==5)++r.counters.value->ticks;
        if(mode==6)r.counters.value->pid=124;if(mode==7)r.counters.support=Support::unsupported;if(mode==8)r.counters.value.reset();if(mode==9){r.counters.value->user_ticks=std::numeric_limits<std::uint64_t>::max();r.counters.value->kernel_ticks=1;r.counters.value->ticks=0;}
        const auto result=t.sample(c,hz,std::span(&r,1)).at(123);assert(!result.value);if(mode==4)assert(result.support==Support::permission_required);
        assert(!take(t,context(800),row(20,20,3)).value); // invalid/denied gap clears baseline
        assert(take(t,context(1200),row(30,30,4)).value==5);
    }
    t.reset();take(t,context(),row(0,0,1));t.sample(context(400),100,{});assert(!take(t,context(800),row(20,0,3)).value);
    t.reset();assert(!take(t,context(),row(0,0,1)).value);
    const auto max=std::numeric_limits<std::uint64_t>::max();
    t.reset();take(t,context(),row(max-100,0,1));assert(take(t,context(400),row(max,0,2)).value==25);
    auto edge=row(0,0,0);edge.at=Clock::time_point::min();t.reset();take(t,context(),edge);edge=row(1,0,0);edge.at=Clock::time_point::max();const auto large_interval=take(t,context(400),edge);assert(large_interval.value&&std::isfinite(*large_interval.value));
    auto large=context();for(unsigned n=0;n<4;++n)large.ticks[n]=max-100;t.reset();take(t,large,row(0,0,1));for(unsigned n=0;n<4;++n)++large.ticks[n];assert(take(t,large,row(100,0,2)).value==25); // global absolute sum need not fit
    t.reset();take(t,context(),row(0,0,1));auto fast=row(max,0,1);fast.at+=1ns;assert(!take(t,context(400),fast).value);
    for(int mode=0;mode<3;++mode){t.reset();take(t,context(),row(0,0,1));std::vector<LinuxProcessCpuObservation> bad{row(10,0,2)};if(mode==0)bad.push_back(bad.front());if(mode==1)bad.front().pid=0;if(mode==2)bad.resize(100001,bad.front());bool threw=false;try{t.sample(context(400),100,bad);}catch(const std::invalid_argument&){threw=true;}assert(threw);assert(!take(t,context(800),row(20,0,3)).value);}
#ifdef __linux__
    const auto native_context=read_linux_cpu_counters();assert(native_context&&!native_context->cpu_ids.empty());const auto hz=::sysconf(_SC_CLK_TCK);assert(hz>0);
    SampleOptions options;options.include_linux_process_descriptions=false;options.include_app_history=false;options.include_disks=false;options.include_networks=false;options.include_gpus=false;options.include_user_sessions=false;options.include_services=false;options.include_startup_entries=false;
    Monitor monitor;const auto first=monitor.sample(options);for(const auto& p:first.processes)assert(!p.cpu_percent.value);
    std::uint64_t work=1;bool positive=false;double own_percentage=0;
    for(unsigned attempt=0;attempt<5&&!positive;++attempt){const auto until=Clock::now()+100ms;while(Clock::now()<until)for(unsigned n=0;n<10000;++n)work=work*6364136223846793005ULL+1;
        const auto sample=monitor.sample(options);for(const auto& p:sample.processes){if(p.cpu_percent.value)assert(p.cpu_percent.support==Support::estimated&&std::isfinite(*p.cpu_percent.value)&&*p.cpu_percent.value>=0&&*p.cpu_percent.value<=100);if(p.pid==static_cast<std::uint32_t>(::getpid())&&p.cpu_percent.value&&*p.cpu_percent.value>0){positive=true;own_percentage=*p.cpu_percent.value;}}
    }
    assert(positive);options.include_processes=false;assert(monitor.sample(options).processes.empty());options.include_processes=true;for(const auto& p:monitor.sample(options).processes)assert(!p.cpu_percent.value);
    // Independent own-process CPU clock corroborates ticks without asserting
    // equality across two different, quantized observation windows.
    timespec a{},b{};assert(::clock_gettime(CLOCK_PROCESS_CPUTIME_ID,&a)==0);const auto stat_a=read_linux_process_identity("/proc/self");assert(stat_a.value);
    const auto until=Clock::now()+120ms;while(Clock::now()<until)for(unsigned n=0;n<10000;++n)work=work*6364136223846793005ULL+1;
    const auto stat_b=read_linux_process_identity("/proc/self");assert(stat_b.value);assert(::clock_gettime(CLOCK_PROCESS_CPUTIME_ID,&b)==0);
    const auto tick_delta=linux_process_tick_delta(*stat_a.value,*stat_b.value);assert(tick_delta&&*tick_delta>0);const auto cpu_seconds=(b.tv_sec-a.tv_sec)+(b.tv_nsec-a.tv_nsec)/1e9;assert(cpu_seconds>0);
    std::cout<<"Native own PID="<<::getpid()<<" CLK_TCK="<<hz<<" visible CPUs="<<native_context->cpu_ids.size()<<" host-normalized CPU="<<own_percentage<<"%; extra own busy interval ticks="<<*tick_delta<<" CPU clock seconds="<<cpu_seconds<<" work="<<work<<'\n';
#endif
    std::cout<<"Process CPU: per-row monotonic interval, separate utime/stime, host normalization, resets/identity/context/permissions/overflow/bounds passed\n";
}
