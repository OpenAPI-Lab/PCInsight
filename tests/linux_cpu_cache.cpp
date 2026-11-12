#include "../src/linux_cpu_cache.hpp"
#include "pcinsight/monitor.hpp"
#include <algorithm>
#include <cassert>
#include <chrono>
#include <fstream>
#include <iostream>
#include <limits>
using namespace pcinsight;
using namespace pcinsight::detail;
namespace fs=std::filesystem;
LinuxCacheSnapshot fixture(){
    LinuxCacheSnapshot s;s.online={0,1,2,3,4,5,6,7};
    // Two sockets, two cores/socket, two SMT threads/core. Split L1,
    // core-shared L2 and socket-shared L3: not per-thread capacities.
    for(auto cpu:s.online){
        const auto core=cpu/2*2,socket=cpu/4*4;
        std::vector<std::uint32_t> pair{core,core+1},quad{socket,socket+1,socket+2,socket+3};
        s.entries.push_back({cpu,1,"Data",32768,pair});
        s.entries.push_back({cpu,1,"Instruction",32768,pair});
        s.entries.push_back({cpu,2,"Unified",262144,pair});
        s.entries.push_back({cpu,3,"Unified",8388608,quad});
    }
    return s;
}
void write(const fs::path& p,const std::string& s){std::ofstream f(p,std::ios::binary);assert(f);f<<s;f.close();assert(f);}
int main(){
    assert(parse_linux_cache_cpulist(" 4-5,0-3\n")==std::optional(std::vector<std::uint32_t>{0,1,2,3,4,5}));
    assert(parse_linux_cache_cpulist("4294967295")->front()==std::numeric_limits<std::uint32_t>::max());
    assert(parse_linux_cache_cpulist("0-4095")->size()==4096);
    for(const auto& s:std::vector<std::string>{""," ","-1","+1","1-0","0,","0,,1",",0","0-1,1","0,0","0-4096","0-4294967295","4294967296","0 -1","0-1-2",std::string("0\0",2),std::string(65537,'1')})assert(!parse_linux_cache_cpulist(s));
    assert(parse_linux_cache_size("32K\n")==32768);
    const auto max=std::numeric_limits<std::uint64_t>::max()/1024;
    assert(parse_linux_cache_size(std::to_string(max)+"K")==max*1024);
    for(const auto& s:std::vector<std::string>{"","0K","32","32k","32M","-1K","+1K","32 K","32Kx",std::to_string(max+1)+"K",std::string("1\0K",3),std::string(65,'1')})assert(!parse_linux_cache_size(s));
    auto s=fixture();auto totals=linux_cache_totals(s);
    assert(totals[0].value==262144&&totals[1].value==1048576&&totals[2].value==16777216);
    for(const auto& m:totals)assert(m.support==Support::estimated&&!m.note.empty());
    LinuxCacheSnapshot unified{{2,900},{{2,1,"Unified",65536,{2,900}},{900,1,"Unified",65536,{2,900}}}};
    assert(linux_cache_totals(unified)[0].value==65536); // sparse IDs, unified L1 counted once
    std::reverse(s.entries.begin(),s.entries.end());assert(linux_cache_totals(s)[2].value==16777216);
    // Partial/mismatched sharing invalidates that level, not a different intact level.
    for(int mode=0;mode<10;++mode){
        s=fixture();auto& e=s.entries[0];
        if(mode==0)s.entries.erase(s.entries.begin());
        if(mode==1)e.bytes+=1024;
        if(mode==2)e.shared={0};
        if(mode==3)e.shared={1};
        if(mode==4)e.shared={0,0,1};
        if(mode==5)e.type="Unified";
        if(mode==6)e.type="Unknown";
        if(mode==7)e.bytes=0;
        if(mode==8)s.entries.push_back(e);
        if(mode==9)e.shared={1,0};
        const auto t=linux_cache_totals(s);assert(!t[0].value&&t[1].value==1048576&&t[2].value==16777216);
    }
    s=fixture();s.online.erase(s.online.begin());s.entries.erase(std::remove_if(s.entries.begin(),s.entries.end(),[](const auto& e){return e.cpu==0;}),s.entries.end());
    // CPU0 offline: its shared caches are still reachable through CPU1.
    assert(linux_cache_totals(s)[0].value==262144);
    s=fixture();s.online={0,2};s.entries.erase(std::remove_if(s.entries.begin(),s.entries.end(),[](const auto& e){return e.cpu!=0&&e.cpu!=2;}),s.entries.end());
    totals=linux_cache_totals(s);assert(totals[0].value==131072&&totals[1].value==524288&&totals[2].value==8388608);
    s=fixture();s.entries.erase(std::remove_if(s.entries.begin(),s.entries.end(),[](const auto& e){return e.level==3;}),s.entries.end());assert(!linux_cache_totals(s)[2].value);
    s={{0,1},{{0,1,"Unified",std::numeric_limits<std::uint64_t>::max(),{0}},{1,1,"Unified",1,{1}}}};assert(!linux_cache_totals(s)[0].value);
    for(int mode=0;mode<6;++mode){
        s=fixture();
        if(mode==0)s.online.clear();
        if(mode==1)s.online.push_back(7);
        if(mode==2)s.entries[0].cpu=100;
        if(mode==3)s.entries[0].level=0;
        if(mode==4)s.entries.resize(65537);
        if(mode==5)for(auto& e:s.entries)e.shared.resize(32769); // aggregate sharing budget
        for(const auto& m:linux_cache_totals(s))assert(!m.value);
    }
#ifdef __linux__
    const auto root=fs::temp_directory_path()/("pcinsight-cache-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    assert(fs::create_directory(root));
    struct Cleanup{fs::path p;~Cleanup(){std::error_code ec;fs::remove_all(p,ec);}} cleanup{root};
    s=fixture();write(root/"online","0-7\n");
    std::vector<fs::path> paths;
    for(std::size_t i=0;i<s.entries.size();++i){
        const auto& e=s.entries[i];auto p=root/("cpu"+std::to_string(e.cpu))/"cache"/("index"+std::to_string(i%4));
        assert(fs::create_directories(p));paths.push_back(p);
        write(p/"level",std::to_string(e.level)+"\n");write(p/"type",e.type+"\n");write(p/"size",std::to_string(e.bytes/1024)+"K\n");
        std::string shared;for(auto cpu:e.shared){if(!shared.empty())shared+=',';shared+=std::to_string(cpu);}write(p/"shared_cpu_list",shared+"\n");
    }
    CpuInfo cpu;fill_linux_cpu_cache(cpu,root);assert(cpu.l1_bytes.value==262144&&cpu.l3_bytes.value==16777216);
    write(paths[0]/"size","-1K\n");fill_linux_cpu_cache(cpu,root);assert(!cpu.l1_bytes.value&&!cpu.l2_bytes.value&&!cpu.l3_bytes.value);
    write(paths[0]/"size","32K\n");write(paths[0]/"shared_cpu_list","0\n");fill_linux_cpu_cache(cpu,root);assert(!cpu.l1_bytes.value&&cpu.l2_bytes.value);
    write(paths[0]/"shared_cpu_list","0,1\n");write(root/"online","0-8\n");fill_linux_cpu_cache(cpu,root);assert(!cpu.l1_bytes.value&&!cpu.l2_bytes.value);
    write(root/"online","0-7\n");fs::remove(paths[0]/"level");assert(!read_linux_cache_snapshot(root).value);
    write(paths[0]/"level",std::string(65537,'1'));assert(!read_linux_cache_snapshot(root).value);
    write(paths[0]/"level","1\n");write(paths[0]/"size","32M\n");assert(!read_linux_cache_snapshot(root).value);
    write(paths[0]/"size","32K\n");write(paths[0]/"type","Other\n");assert(!read_linux_cache_snapshot(root).value);
    write(paths[0]/"type","Data\n");fs::create_directory(root/"cpu0/cache/indexbad");assert(!read_linux_cache_snapshot(root).value);
    const auto before=read_linux_cache_snapshot();
    SampleOptions options;options.include_processes=false;options.include_app_history=false;options.include_disks=false;options.include_networks=false;options.include_gpus=false;options.include_user_sessions=false;options.include_services=false;options.include_startup_entries=false;
    Monitor monitor;const auto native=monitor.sample(options);const auto after=read_linux_cache_snapshot();
    if(before.value&&after.value&&before.value==after.value){
        const auto expected=linux_cache_totals(*before.value);
        assert(native.cpu.l1_bytes.value==expected[0].value&&native.cpu.l2_bytes.value==expected[1].value&&native.cpu.l3_bytes.value==expected[2].value);
        std::cout<<"Native stable cache inventory: "<<before.value->online.size()<<" online CPUs, "<<before.value->entries.size()<<" rows\n";
    }else std::cout<<"Native cache topology unavailable/unstable; no physical cache total claimed\n";
    for(const auto* m:{&native.cpu.l1_bytes,&native.cpu.l2_bytes,&native.cpu.l3_bytes}){
        if(m->value){assert(*m->value>0&&m->support==Support::estimated);std::cout<<"Cache bytes="<<*m->value<<" estimated\n";}
        else std::cout<<"Cache unavailable: "<<m->note<<'\n';
    }
    std::cout<<"Linux sysfs fixtures: multi-socket/SMT split/shared cache totals, unreadable/malformed/partial inventory and stale-value clearing passed\n";
#else
    assert(!read_linux_cache_snapshot().value);
#endif
    std::cout<<"Cache parsing/aggregation: deduplication, online scope, per-level completeness, conflicts and overflow passed\n";
}
