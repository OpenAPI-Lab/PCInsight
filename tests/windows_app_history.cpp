#include "../src/windows_app_history.hpp"
#include "pcinsight/monitor.hpp"
#include <cassert>
#include <iostream>
#include <limits>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <atomic>
#endif
using namespace pcinsight;
using namespace pcinsight::detail;
ProcessInfo process(std::uint32_t pid,std::uint64_t created,std::uint64_t ticks,std::string path="C:\\App.exe"){
    ProcessInfo p;p.pid=pid;p.name="fixture";p.executable=std::move(path);
    p.windows_creation_time_100ns={created,Support::available,"fixture"};p.windows_cpu_time_100ns={ticks,Support::available,"fixture"};return p;
}
int main(){
    WindowsAppHistory history;using C=std::chrono::system_clock;auto now=C::time_point(std::chrono::seconds(1));
    auto p=process(7,100,100);auto sample=[&](std::vector<ProcessInfo> rows){now+=std::chrono::seconds(1);return history.sample(rows,now);};
    auto value=sample({p});assert(value.value && value.support==Support::estimated && value.value->size()==1 && value.value->front().observed_cpu_time.value->count()==0);
    const auto start=value.value->front().period_start;
    p.windows_cpu_time_100ns.value=150;value=sample({p});assert(value.value->front().observed_cpu_time.value->count()==5000);
    auto q=process(8,200,20);p.windows_cpu_time_100ns.value=170;value=sample({p,q});assert(value.value->size()==1 && value.value->front().observed_cpu_time.value->count()==7000);
    p=process(7,101,900);q.windows_cpu_time_100ns.value=25;value=sample({p,q});assert(value.value->front().observed_cpu_time.value->count()==7500);
    value=sample({});assert(value.value->front().observed_cpu_time.value->count()==7500);
    p.windows_cpu_time_100ns.value=950;(void)sample({p});p.windows_cpu_time_100ns.value=960;value=sample({p});assert(value.value->front().observed_cpu_time.value->count()==8500);
    p.windows_cpu_time_100ns.value=5;(void)sample({p});p.windows_cpu_time_100ns.value=10;value=sample({p});assert(value.value->front().observed_cpu_time.value->count()==9000);
    assert(value.value->front().period_start==start && value.value->front().period_end==now && !value.value->front().network_bytes.value && !value.value->front().notifications.value);
    p.windows_cpu_time_100ns.support=Support::permission_required;(void)sample({p});p.windows_cpu_time_100ns.support=Support::available;p.windows_cpu_time_100ns.value=1000;value=sample({p});assert(value.value->front().observed_cpu_time.value->count()==9000);
    p.executable="C:\\Other.exe";value=sample({p});assert(value.value->size()==2 && value.value->back().observed_cpu_time.value->count()==0);
    value=sample({process(0,123,999)});assert(value.value->size()==2);
    assert(!sample({p,p}).value); // Rejected input must not retain a continuity baseline.
    history.reset();p=process(7,100,0);(void)sample({p});p.windows_cpu_time_100ns.value=std::numeric_limits<std::uint64_t>::max();assert(!sample({p}).value);
    history.reset();value=sample({process(1,100,123,"")});assert(value.value && value.value->empty());
    assert(!history.sample({},C::time_point{}).value);
    history.reset();(void)sample({p});assert(!history.sample({},now-std::chrono::seconds(1)).value);
    history.reset();std::vector<ProcessInfo> apps;for(unsigned i=1;i<=4097;++i)apps.push_back(process(i,i,0,"C:\\App"+std::to_string(i)+".exe"));assert(!sample(apps).value);
    history.reset();p=process(1,100,0,std::string(32769,'x'));assert(!sample({p}).value);
#ifdef _WIN32
    Monitor monitor;SampleOptions options;options.include_disks=options.include_gpus=options.include_networks=false;options.include_app_history=true;
    auto snapshot=monitor.sample(options);assert(snapshot.windows_app_history.value && snapshot.app_history.empty());
    std::string executable;for(const auto& row:snapshot.processes)if(row.pid==GetCurrentProcessId()){executable=row.executable;assert(row.windows_cpu_time_100ns.value);}
    assert(!executable.empty());options.include_processes=false;
    std::int64_t measured=0;std::atomic<std::uint64_t> work{0};const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(3);
    do{
        const auto until=std::chrono::steady_clock::now()+std::chrono::milliseconds(60);
        while(std::chrono::steady_clock::now()<until)work.fetch_add(1,std::memory_order_relaxed);
        snapshot=monitor.sample(options);assert(snapshot.processes.empty() && snapshot.windows_app_history.value);
        for(const auto& row:*snapshot.windows_app_history.value)if(row.id==executable){assert(row.observed_cpu_time.value && !row.network_bytes.value && !row.notifications.value);measured=row.observed_cpu_time.value->count();}
    }while(measured==0 && std::chrono::steady_clock::now()<deadline);
    assert(measured>0);std::cout<<"Native own-process observed CPU history ns="<<measured<<'\n';
    options.include_app_history=false;snapshot=monitor.sample(options);assert(!snapshot.windows_app_history.value);
    options.include_app_history=true;snapshot=monitor.sample(options);assert(snapshot.windows_app_history.value);for(const auto& row:*snapshot.windows_app_history.value)assert(row.observed_cpu_time.value->count()==0);
#endif
    std::cout<<"Windows CPU app history: identity, deltas, grouping, gaps, denials, resets, bounds and overflow passed\n";
}
