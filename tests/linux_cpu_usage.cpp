#include "../src/linux_cpu_usage.hpp"
#include "pcinsight/monitor.hpp"
#include <cassert>
#include <cmath>
#include <iostream>
#include <limits>
#include <sstream>
#ifdef __linux__
#include <unistd.h>
#endif
using namespace pcinsight;
using namespace pcinsight::detail;
using namespace std::chrono_literals;
std::string frame(std::array<std::uint64_t,8> ticks,std::uint64_t guest=0){
    std::ostringstream text;
    for(auto label:{"cpu","cpu0"}){text<<label;for(auto value:ticks)text<<' '<<value;text<<' '<<guest<<" 0\n";}
    text<<"intr 1 2 3\nbtime 1000\nprocs_running 1\n";return text.str();
}
int main(){
    std::array<std::uint64_t,8> ticks{};ticks.fill(100);
    auto a=parse_linux_cpu_counters(frame(ticks,50));assert(a&&a->fields==10&&a->cpu_ids==std::vector<std::uint32_t>{0});
    auto b=a;b->ticks[0]+=20;b->ticks[3]+=20;
    const auto delta=linux_cpu_interval(a,b);assert(delta&&delta->total==40&&delta->idle==20);
    const auto usage=linux_cpu_usage(a,b);assert(usage.value==50&&usage.support==Support::estimated);
    auto changed=ticks;changed[0]+=20;changed[3]+=20;
    auto guest=parse_linux_cpu_counters(frame(changed,60));assert(linux_cpu_usage(a,guest).value==50); // guest already included, not added again
    const auto rejected=[](const auto& before,const auto& after,std::string_view reason){
        const auto metric=linux_cpu_usage(before,after);
        assert(!metric.value&&metric.support==Support::unsupported);
        assert(metric.note==reason);
        assert(!linux_cpu_interval(before,after));
    };
    rejected(std::optional<LinuxCpuCounters>{},a,"CPU interval unavailable: previous counters missing (initial sample or baseline reset)");
    rejected(a,std::optional<LinuxCpuCounters>{},"CPU interval unavailable: current counters missing (read/parse failure; exact source error unknown)");
    rejected(a,a,"CPU interval unavailable: counters did not advance");
    b=a;b->ticks[3]+=20;b->ticks[4]+=20;assert(linux_cpu_usage(a,b).value==0);
    b=a;b->ticks[0]+=20;assert(linux_cpu_usage(a,b).value==100);
    constexpr std::array<std::string_view,8> states{"user","nice","system","idle","iowait","irq","softirq","steal"};
    for(unsigned field=0;field<8;++field){
        b=a;b->ticks[(field+1)%8]+=200;b->ticks[field]=99;
        rejected(a,b,"CPU interval unavailable: counter decreased: "+std::string(states[field])+" previous=100 current=99");
    }
    b=a;b->cpu_ids.push_back(1);rejected(a,b,"CPU interval unavailable: visible CPU IDs changed");
    b=a;b->cpu_ids={1};assert(!linux_cpu_interval(a,b));
    b=a;b->boot_time=1001;rejected(a,b,"CPU interval unavailable: boot token changed (value or presence)");
    b=a;b->boot_time.reset();rejected(a,b,"CPU interval unavailable: boot token changed (value or presence)");
    auto legacy=parse_linux_cpu_counters("cpu 1 2 3 4\ncpu0 1 2 3 4\n");assert(legacy&&legacy->fields==4);
    rejected(a,legacy,"CPU interval unavailable: schema changed from 10 to 4");
    b=legacy;b->ticks[0]+=5;assert(linux_cpu_usage(legacy,b).value==100);
    b=legacy;b->ticks[7]=1;rejected(legacy,b,"CPU interval unavailable: invalid current counters");
    rejected(b,legacy,"CPU interval unavailable: invalid previous counters");
    const auto max=std::numeric_limits<std::uint64_t>::max();
    ticks.fill(max-100);a=parse_linux_cpu_counters(frame(ticks));b=a;for(auto& value:b->ticks)++value;
    assert(linux_cpu_usage(a,b).value==75); // absolute state sum may exceed uint64; small interval remains valid
    ticks.fill(0);a=parse_linux_cpu_counters(frame(ticks));b=a;b->ticks.fill(max);
    rejected(a,b,"CPU interval unavailable: delta sum overflow at nice");
    b=a;b->ticks[0]=max;assert(linux_cpu_usage(a,b).value==100);
    // A missing input clears the caller baseline; recovery requires two valid frames.
    std::optional<LinuxCpuCounters> previous=a;
    assert(!linux_cpu_usage(previous,{}).value);previous={};
    assert(!linux_cpu_usage(previous,b).value);previous=b;
    auto next=b;next->ticks[1]=1;assert(linux_cpu_usage(previous,next).value==100);
    for(const auto& text:std::vector<std::string>{"","cpu 1 2 3\ncpu0 1 2 3\n","cpu 1 2 3 4\n","cpu0 1 2 3 4\n","cpu 1 2 -3 4\ncpu0 1 2 3 4\n","cpu 1 2 3 4 junk\ncpu0 1 2 3 4\n","cpu 1 2 3 4\ncpu0 1 2 3 4 5\n","cpu 1 2 3 4\ncpu0 1 2 3 4\ncpu0 1 2 3 4\n","cpu 1 2 3 4\ncpu4294967296 1 2 3 4\n",frame(ticks)+"btime 1001\n",frame(ticks)+"cpu 1 2 3 4\n",frame(ticks)+"btime invalid\n"})assert(!parse_linux_cpu_counters(text));
    assert(!parse_linux_cpu_counters(std::string(16*1024*1024+1,'x')));
    assert(!parse_linux_cpu_counters("cpu "+std::string(65536,' ')+"1 2 3 4\ncpu0 1 2 3 4\n"));
    std::string many="cpu 1 2 3 4\n";for(unsigned i=0;i<100001;++i)many+="cpu"+std::to_string(i)+" 1 2 3 4\n";
    assert(!parse_linux_cpu_counters(many));
    const auto ordered=parse_linux_cpu_counters("cpu 1 2 3 4\ncpu1 1 2 3 4\ncpu0 1 2 3 4\n");
    assert(ordered&&ordered->cpu_ids==std::vector<std::uint32_t>({0,1}));
#ifdef __linux__
    const auto raw=read_linux_cpu_counters();assert(raw&&!raw->cpu_ids.empty());
    SampleOptions options;options.include_app_history=false;options.include_disks=false;
    options.include_networks=false;options.include_gpus=false;options.include_user_sessions=false;
    options.include_services=false;options.include_startup_entries=false;
    Monitor monitor;const auto first=monitor.sample(options);assert(!first.cpu.utilization_percent.value);
    for(const auto& p:first.processes)assert(!p.cpu_percent.value);
    bool measured=false;double cpu=0,process=0;std::uint64_t work=1;
    for(unsigned attempt=0;attempt<5&&!measured;++attempt){
        const auto until=std::chrono::steady_clock::now()+40ms;
        while(std::chrono::steady_clock::now()<until)for(unsigned i=0;i<10000;++i)work=work*6364136223846793005ULL+1;
        const auto sample=monitor.sample(options);
        for(const auto& p:sample.processes)if(p.pid==static_cast<std::uint32_t>(getpid())&&p.cpu_percent.value&&sample.cpu.utilization_percent.value){
            cpu=*sample.cpu.utilization_percent.value;process=*p.cpu_percent.value;
            assert(sample.cpu.utilization_percent.support==Support::estimated);
            assert(std::isfinite(cpu)&&cpu>=0&&cpu<=100&&std::isfinite(process)&&process>=0&&process<=100);
            if(process>0)measured=true;
        }
    }
    assert(measured);
    options.include_processes=false;assert(monitor.sample(options).processes.empty());
    options.include_processes=true;for(const auto& p:monitor.sample(options).processes)assert(!p.cpu_percent.value);
    std::cout<<"Native Linux CPU ratio="<<cpu<<"%, own-process normalized ratio="<<process<<"%, visible CPUs="<<raw->cpu_ids.size()<<", work="<<work<<'\n';
#else
    assert(!read_linux_cpu_counters());
#endif
    std::cout<<"CPU usage: strict counters, guest exclusion, reset/iowait retreat, boot/topology/schema, zero/100, overflow and recovery passed\n";
}
