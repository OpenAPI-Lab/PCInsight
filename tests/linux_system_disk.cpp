#include "../src/linux_system_disk.hpp"
#include "pcinsight/monitor.hpp"
#include <cassert>
#include <iostream>
#ifdef __linux__
#include <sys/stat.h>
#include <sys/sysmacros.h>
#endif
using namespace pcinsight;
using namespace pcinsight::detail;
std::string mount(std::string dev="8:1",std::string path="/",std::string fs="ext4"){
    return "30 20 "+dev+" / "+path+" rw optional:future - "+fs+" /dev/untrusted-source rw\n";
}
int main(){
    LinuxSystemDiskApi api;unsigned root_calls=0,map_calls=0;
    api.root_device=[&]()->Metric<std::string>{++root_calls;return {"8:1",Support::available,"fixture"};};
    api.disk=[&](const std::string& dev)->std::optional<std::string>{++map_calls;assert(dev=="8:1");return "disk0";};
    auto result=identify_linux_system_disk(mount(),api);
    assert(result.value=="disk0"&&result.support==Support::estimated&&root_calls==1&&map_calls==1);
    result=identify_linux_system_disk(mount("0:9")+mount(),api);assert(result.value=="disk0"); // different hidden root row ignored
    result=identify_linux_system_disk(mount()+mount(),api);assert(result.value=="disk0"); // same association
    for(auto fs:{"overlay","tmpfs","btrfs","f2fs","xfs","unknown"}){
        const auto before=map_calls;assert(!identify_linux_system_disk(mount("8:1","/",fs),api).value);assert(map_calls==before);
    }
    assert(!identify_linux_system_disk(mount()+mount("8:1","/","btrfs"),api).value);
    assert(!identify_linux_system_disk(mount()+mount("8:1","/","ext3"),api).value);
    assert(!identify_linux_system_disk(mount("8:2"),api).value);
    root_calls=0;assert(!identify_linux_system_disk(mount("8:1","/home"),api).value&&root_calls==0);
    assert(!identify_linux_system_disk(mount()+"bad\n",api).value&&root_calls==0);
    assert(!identify_linux_system_disk(std::string(16*1024*1024+1,'x'),api).value);
    std::string many;const auto row=mount();for(unsigned i=0;i<100001;++i)many+=row;
    assert(!identify_linux_system_disk(many,api).value&&root_calls==0);
    assert(!identify_linux_system_disk(row,{}).value);
    api.root_device=[]()->Metric<std::string>{return {{},Support::permission_required,"denied root"};};
    result=identify_linux_system_disk(row,api);assert(!result.value&&result.support==Support::permission_required);
    api.root_device=[]()->Metric<std::string>{return {"bad",Support::available,"fixture"};};assert(!identify_linux_system_disk(row,api).value);
    api.root_device=[]()->Metric<std::string>{return {"8:1",Support::unsupported,"inconsistent"};};assert(!identify_linux_system_disk(row,api).value);
    api.root_device=[]()->Metric<std::string>{return {"8:1",Support::available,"fixture"};};
    api.disk=[](const std::string&)->std::optional<std::string>{return {};};assert(!identify_linux_system_disk(row,api).value);
    for(const auto& id:std::vector<std::string>{"",".","..","../bad",std::string("x\0y",3),std::string(4097,'x')}){
        api.disk=[&](const std::string&)->std::optional<std::string>{return id;};assert(!identify_linux_system_disk(row,api).value);
    }
    std::vector<DiskInfo> disks(2);disks[0].id="disk0";disks[1].id="disk1";
    result={"disk0",Support::estimated,"Root direct block mapping"};
    apply_linux_system_disk_flags(disks,result);
    assert(disks[0].system_disk.value==true&&disks[1].system_disk.value==false);
    assert(disks[0].system_disk.support==Support::estimated);
    apply_linux_system_disk_flags(disks,{{},Support::permission_required,"denied"});
    for(const auto& disk:disks)assert(!disk.system_disk.value&&disk.system_disk.support==Support::permission_required);
    apply_linux_system_disk_flags(disks,{"missing",Support::estimated,"fixture"});
    for(const auto& disk:disks)assert(!disk.system_disk.value);
    disks[1].id="disk0";apply_linux_system_disk_flags(disks,result);
    for(const auto& disk:disks)assert(!disk.system_disk.value); // ambiguous root identity
    disks[1].id="disk1";apply_linux_system_disk_flags(disks,{"disk0",Support::unsupported,"inconsistent"});
    for(const auto& disk:disks)assert(!disk.system_disk.value);
#ifdef __linux__
    struct stat identity{};assert(stat("/",&identity)==0);
    const auto device=read_linux_root_device();assert(device.value);
    assert(*device.value==std::to_string(major(identity.st_dev))+":"+std::to_string(minor(identity.st_dev)));
    const auto native=read_native_linux_system_disk();
    SampleOptions options;options.include_processes=false;options.include_app_history=false;
    options.include_networks=false;options.include_gpus=false;options.include_user_sessions=false;
    options.include_services=false;options.include_startup_entries=false;
    Monitor monitor;const auto snapshot=monitor.sample(options);assert(!snapshot.disks.empty());
    unsigned yes=0,no=0,unknown=0;
    for(const auto& disk:snapshot.disks){
        if(!disk.system_disk.value){++unknown;assert(!disk.system_disk.note.empty());}
        else {assert(disk.system_disk.support==Support::estimated);if(*disk.system_disk.value)++yes;else ++no;}
    }
    if(native.value){assert(yes==1&&unknown==0);for(const auto& disk:snapshot.disks)assert(*disk.system_disk.value==(disk.id==*native.value));}
    else assert(yes==0&&no==0&&unknown==snapshot.disks.size());
    options.include_disks=false;assert(monitor.sample(options).disks.empty());
    std::cout<<"Native Linux system disk: "<<native.value.value_or("unavailable")<<"; yes="<<yes<<", no="<<no<<", unknown="<<unknown<<"; root fd identity verified read-only\n";
#else
    assert(!read_linux_root_device().value&&!read_native_linux_system_disk().value);
#endif
    std::cout<<"System disk: root/mount agreement, hidden rows, topology, permissions, bounds, yes/no/unknown and inventory ambiguity passed\n";
}
