#include "../src/linux_cpu_topology.hpp"
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
LinuxTopologySnapshot fixture(){
    LinuxTopologySnapshot s;s.online={0,1,2,3,4,5,6,7};
    for(auto cpu:s.online){
        const auto core=cpu/2*2,package=cpu/4*4;
        s.rows.push_back({cpu,static_cast<std::int32_t>(cpu/4*11),std::vector<std::uint32_t>{package,package+1,package+2,package+3},std::vector<std::uint32_t>{core,core+1}});
    }
    return s;
}
void expected(const LinuxTopologySnapshot& s,unsigned sockets,unsigned cores,unsigned logical){
    const auto c=linux_topology_counts(s);
    assert(c.sockets.value==sockets&&c.cores.value==cores&&c.logical.value==logical);
    assert(c.sockets.support==Support::estimated&&c.cores.support==Support::estimated&&c.logical.support==Support::estimated);
}
void write(const fs::path& p,const std::string& text){std::ofstream f(p,std::ios::binary);assert(f);f<<text;f.close();assert(f);}
std::string list(const std::vector<std::uint32_t>& ids){std::string s;for(auto cpu:ids){if(!s.empty())s+=',';s+=std::to_string(cpu);}return s+'\n';}
int main(){
    assert(parse_linux_package_id("0\n")==0&&parse_linux_package_id(" 17\t")==17);
    assert(parse_linux_package_id("2147483647")==std::numeric_limits<std::int32_t>::max());
    for(const auto& s:std::vector<std::string>{""," ","-1","-0","+0","1x","1 2","2147483648",std::string("1\0",2),std::string(65,'1')})assert(!parse_linux_package_id(s));
    auto s=fixture();expected(s,2,4,8);
    std::reverse(s.rows.begin(),s.rows.end());expected(s,2,4,8);
    // Sparse IDs, non-contiguous package identifiers, CPU0 offline, SMT and
    // single-threaded cores can coexist. Core numeric IDs/die IDs aren't keys.
    s={{2,10,900},{{2,17,std::vector<std::uint32_t>{2,10},std::vector<std::uint32_t>{2,10}},{10,17,std::vector<std::uint32_t>{2,10},std::vector<std::uint32_t>{2,10}},{900,29,std::vector<std::uint32_t>{900},std::vector<std::uint32_t>{900}}}};expected(s,2,2,3);
    s=fixture();s.online={1,2,4};s.rows.erase(std::remove_if(s.rows.begin(),s.rows.end(),[](const auto& r){return r.cpu!=1&&r.cpu!=2&&r.cpu!=4;}),s.rows.end());expected(s,2,3,3);
    s={{0,1},{{0,0,std::vector<std::uint32_t>{0,9},std::vector<std::uint32_t>{0}},{1,1,std::vector<std::uint32_t>{1,9},std::vector<std::uint32_t>{1}}}};
    auto conflict=linux_topology_counts(s);assert(conflict.logical.value==2&&!conflict.sockets.value&&!conflict.cores.value); // overlapping offline package member
    s={{0,1},{{0,0,std::vector<std::uint32_t>{0,1,9},std::vector<std::uint32_t>{0,9}},{1,0,std::vector<std::uint32_t>{0,1,9},std::vector<std::uint32_t>{1,9}}}};
    conflict=linux_topology_counts(s);assert(conflict.sockets.value==1&&!conflict.cores.value); // overlapping offline core member
    s=fixture();s.rows[0].core_cpus.reset();auto c=linux_topology_counts(s);assert(c.sockets.value==2&&!c.cores.value&&c.logical.value==8);
    for(int mode=0;mode<10;++mode){
        s=fixture();auto& row=s.rows[0];
        if(mode==0)row.package_id.reset();
        if(mode==1)row.package_id=-1;
        if(mode==2)row.package_cpus.reset();
        if(mode==3)row.package_cpus=std::vector<std::uint32_t>{0,1}; // inconsistent sharing
        if(mode==4)row.package_cpus=std::vector<std::uint32_t>{1,2,3}; // excludes self
        if(mode==5)row.package_id=11; // same ID used by distinct package groups
        if(mode==6)row.package_id=22; // same group disagrees about ID
        if(mode==7)row.package_cpus=std::vector<std::uint32_t>{0,0,1,2,3};
        if(mode==8)s.rows[1].cpu=0; // duplicate row/missing CPU
        if(mode==9)s.rows.pop_back();
        c=linux_topology_counts(s);assert(!c.sockets.value&&!c.cores.value&&c.logical.value==8);
    }
    for(int mode=0;mode<5;++mode){
        s=fixture();auto& row=s.rows[0];
        if(mode==0)row.core_cpus=std::vector<std::uint32_t>{0};
        if(mode==1)row.core_cpus=std::vector<std::uint32_t>{0,1,4,5};
        if(mode==2)row.core_cpus=std::vector<std::uint32_t>{1};
        if(mode==3)row.core_cpus=std::vector<std::uint32_t>{1,0};
        if(mode==4)row.core_cpus=std::vector<std::uint32_t>{0,0,1};
        c=linux_topology_counts(s);assert(c.sockets.value==2&&!c.cores.value&&c.logical.value==8);
    }
    // Matching core groups across package boundaries still cannot become cores.
    s=fixture();for(auto& row:s.rows)row.core_cpus=s.online;
    c=linux_topology_counts(s);assert(c.sockets.value==2&&!c.cores.value);
    s=fixture();s.rows.clear();c=linux_topology_counts(s);assert(c.logical.value==8&&!c.cores.value&&!c.sockets.value);
    s=fixture();for(auto& row:s.rows)row.package_cpus=std::vector<std::uint32_t>(131073,0);
    c=linux_topology_counts(s);assert(c.logical.value==8&&!c.sockets.value&&!c.cores.value); // aggregate memory budget
    for(int mode=0;mode<4;++mode){
        s=fixture();
        if(mode==0)s.online.clear();
        if(mode==1)s.online.push_back(7);
        if(mode==2)std::reverse(s.online.begin(),s.online.end());
        if(mode==3)s.online.resize(4097);
        c=linux_topology_counts(s);assert(!c.logical.value&&!c.cores.value&&!c.sockets.value);
    }
    CpuInfo cpu;Metric<LinuxTopologySnapshot> source{fixture(),Support::available,"fixture"};
    apply_linux_cpu_topology(cpu,source,source);assert(cpu.sockets.value==2&&cpu.physical_cores.value==4&&cpu.logical_processors.value==8);
    auto changed=source;changed.value->rows[0].package_id=99;
    apply_linux_cpu_topology(cpu,source,changed);assert(!cpu.sockets.value&&!cpu.physical_cores.value&&!cpu.logical_processors.value);
    apply_linux_cpu_topology(cpu,source,source);apply_linux_cpu_topology(cpu,{},source);assert(!cpu.sockets.value&&!cpu.physical_cores.value&&!cpu.logical_processors.value);
    changed=source;changed.value->online.pop_back();apply_linux_cpu_topology(cpu,source,changed);assert(!cpu.logical_processors.value);
#ifdef __linux__
    const auto root=fs::temp_directory_path()/("pcinsight-topology-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    assert(fs::create_directory(root));
    struct Cleanup{fs::path path;~Cleanup(){std::error_code ec;fs::remove_all(path,ec);}} cleanup{root};
    fill_linux_cpu_topology(cpu,root);assert(!cpu.logical_processors.value&&!cpu.sockets.value);
    s=fixture();write(root/"online",list(s.online));
    for(const auto& row:s.rows){
        auto dir=root/("cpu"+std::to_string(row.cpu))/"topology";assert(fs::create_directories(dir));
        write(dir/"physical_package_id",std::to_string(*row.package_id));
        write(dir/"package_cpus_list",list(*row.package_cpus));write(dir/"core_cpus_list",list(*row.core_cpus));
        write(dir/"core_id","0\n");write(dir/"die_id","0\n"); // repeated IDs don't collapse distinct sibling groups
        write(dir/"core_siblings_list",list(*row.package_cpus));write(dir/"thread_siblings_list",list(*row.core_cpus));
    }
    auto input=read_linux_cpu_topology(root);assert(input.value);expected(*input.value,2,4,8);
    fill_linux_cpu_topology(cpu,root);assert(cpu.sockets.value==2&&cpu.physical_cores.value==4&&cpu.logical_processors.value==8);
    const auto dir=root/"cpu0/topology";
    write(dir/"core_cpus_list","bad\n");fill_linux_cpu_topology(cpu,root);assert(cpu.sockets.value==2&&!cpu.physical_cores.value&&cpu.logical_processors.value==8);
    fs::remove(dir/"core_cpus_list");fill_linux_cpu_topology(cpu,root);assert(cpu.physical_cores.value==4); // old ABI alias only when new path absent
    fs::remove(dir/"package_cpus_list");fill_linux_cpu_topology(cpu,root);assert(cpu.sockets.value==2);
    write(dir/"physical_package_id","-1\n");fill_linux_cpu_topology(cpu,root);assert(!cpu.sockets.value&&!cpu.physical_cores.value&&cpu.logical_processors.value==8);
    write(dir/"physical_package_id","0\n");write(dir/"package_cpus_list",std::string(65537,'1'));fill_linux_cpu_topology(cpu,root);assert(!cpu.sockets.value&&cpu.logical_processors.value==8);
    write(root/"online","0-8\n");fill_linux_cpu_topology(cpu,root);assert(cpu.logical_processors.value==9&&!cpu.sockets.value); // missing cpu8 topology is not one invented core/socket
    write(root/"online","garbage\n");fill_linux_cpu_topology(cpu,root);assert(!cpu.logical_processors.value&&!cpu.sockets.value&&!cpu.physical_cores.value);
    const auto before=read_linux_cpu_topology();
    SampleOptions options;options.include_processes=false;options.include_app_history=false;options.include_disks=false;options.include_networks=false;options.include_gpus=false;options.include_user_sessions=false;options.include_services=false;options.include_startup_entries=false;
    Monitor monitor;const auto observed=monitor.sample(options);const auto after=read_linux_cpu_topology();
    if(before.value&&after.value&&before.value==after.value){
        c=linux_topology_counts(*before.value);
        assert(observed.cpu.logical_processors.value==c.logical.value&&observed.cpu.physical_cores.value==c.cores.value&&observed.cpu.sockets.value==c.sockets.value);
        auto display=[](const Metric<unsigned>& m){return m.value?std::to_string(*m.value):std::string("unavailable");};
        std::cout<<"Native stable sysfs topology: online="<<display(c.logical)<<", core groups="<<display(c.cores)<<", package groups="<<display(c.sockets)<<"; kernel-visible/estimated, not physical hardware audit\n";
    }else std::cout<<"Native topology unavailable/unstable; no hardware count claimed\n";
    std::cout<<"Linux filesystem fixtures: modern/legacy ABI, repeated numeric core IDs, missing/invalid/oversized files, missing CPU rows and stale clearing passed\n";
#else
    assert(!read_linux_cpu_topology().value);
#endif
    std::cout<<"Topology: SMT, multiple packages, sparse IDs, offline scope, unknown/contradictory partitions, limits and two-read changes passed\n";
}
