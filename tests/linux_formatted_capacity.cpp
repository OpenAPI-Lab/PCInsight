#include "../src/linux_formatted_capacity.hpp"
#include "pcinsight/monitor.hpp"
#include <cassert>
#include <iostream>
#include <limits>
#ifdef __linux__
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <sys/sysmacros.h>
#endif
using namespace pcinsight;
using namespace pcinsight::detail;
std::string mount(std::string device="8:1",std::string path="/mnt/a",std::string fs="ext4"){
    return "30 20 "+device+" / "+path+" rw shared:1 - "+fs+" /dev/source rw\n";
}
int main(){
    const auto parsed=parse_linux_capacity_mount("30 20 08:01 /sub /mnt/a\\040b\\134040 rw unknown:tag - ext4 /dev/x rw");
    assert(parsed&&parsed->device=="8:1"&&parsed->path=="/mnt/a b\\040"); // escapes decoded exactly once
    assert(parse_linux_capacity_mount("30 20 8:1 / /x\\011y\\012z rw - ext4 /dev/x rw")->path=="/x\ty\nz");
    for(auto text:{"","30 20 8:1 / /x rw","30 20 -1:1 / /x rw - ext4 x rw","30 20 8:1 / relative rw - ext4 x rw","30 20 8:1 / /x\\000 rw - ext4 x rw","30 20 8:1 / /x rw - ext4 x rw extra"})assert(!parse_linux_capacity_mount(text));
    assert(!parse_linux_capacity_mount(std::string(65537,'x')));
    assert(linux_filesystem_capacity(0,4096).value==0);
    assert(linux_filesystem_capacity(10,4096).value==40960);
    assert(!linux_filesystem_capacity(10,0).value);
    assert(linux_filesystem_capacity(std::numeric_limits<std::uint64_t>::max(),1).value);
    assert(!linux_filesystem_capacity(std::numeric_limits<std::uint64_t>::max(),2).value);
    LinuxFormattedApi api;unsigned queries=0;
    api.disk=[](const std::string& dev)->std::optional<std::string>{return dev.rfind("8:",0)==0?std::optional<std::string>("disk0"):std::nullopt;};
    api.capacity=[&](const LinuxCapacityMount& row)->Metric<std::uint64_t>{++queries;return {row.device=="8:1"?100u:200u,Support::available,"fixture"};};
    const auto text=mount()+mount("8:1","/alias")+mount("8:2","/mnt/b")+mount("0:1","/tmp","tmpfs");
    auto result=aggregate_linux_formatted_capacity(text,api);
    assert(result.value&&result.value->at("disk0").value==300&&queries==3);
    assert(result.value->at("disk0").support==Support::estimated);
    api.capacity=[&](const LinuxCapacityMount& row)->Metric<std::uint64_t>{
        ++queries;if(row.path=="/mnt/a")return {{},Support::permission_required,"denied"};
        return {row.device=="8:1"?100u:200u,Support::available,"fixture"};
    };
    result=aggregate_linux_formatted_capacity(text,api);assert(result.value->at("disk0").value==300); // retry accessible alias
    result=aggregate_linux_formatted_capacity(mount()+mount("8:2"),api);
    assert(!result.value->at("disk0").value); // never publish the successful partial sum
    api.capacity=[](const LinuxCapacityMount& row)->Metric<std::uint64_t>{return {row.path=="/alias"?101u:100u,Support::available,"fixture"};};
    result=aggregate_linux_formatted_capacity(text,api);assert(!result.value->at("disk0").value); // conflicting aliases
    api.capacity=[](const LinuxCapacityMount&)->Metric<std::uint64_t>{return {std::numeric_limits<std::uint64_t>::max(),Support::available,"fixture"};};
    result=aggregate_linux_formatted_capacity(mount()+mount("8:2"),api);assert(!result.value->at("disk0").value);
    api.capacity=[](const LinuxCapacityMount&)->Metric<std::uint64_t>{return {0,Support::available,"measured zero"};};
    result=aggregate_linux_formatted_capacity(mount(),api);assert(result.value->at("disk0").value==0);
    result=aggregate_linux_formatted_capacity(mount("8:1","/a","btrfs"),api);assert(!result.value->at("disk0").value);
    for(auto fs:{"xfs","f2fs","erofs","bcachefs","futurefs"}){
        result=aggregate_linux_formatted_capacity(mount("8:1","/a",fs),api);assert(!result.value->at("disk0").value);
    }
    result=aggregate_linux_formatted_capacity(mount()+mount("8:1","/b","btrfs"),api);assert(!result.value->at("disk0").value);
    result=aggregate_linux_formatted_capacity(mount("0:1","/tmp","tmpfs"),api);assert(result.value&&result.value->empty()); // not disk capacity zero
    queries=0;api.capacity=[&](const LinuxCapacityMount&)->Metric<std::uint64_t>{++queries;return {1,Support::available,"fixture"};};
    assert(!aggregate_linux_formatted_capacity(mount()+"malformed\n",api).value&&queries==0);
    assert(!aggregate_linux_formatted_capacity(std::string(16*1024*1024+1,'x'),api).value);
    const auto row=mount();std::string many;for(unsigned i=0;i<100001;++i)many+=row;
    assert(!aggregate_linux_formatted_capacity(many,api).value&&queries==0);
    assert(!aggregate_linux_formatted_capacity(row,{}).value);
    api.disk=[](const std::string&)->std::optional<std::string>{return "../bad";};
    assert(!aggregate_linux_formatted_capacity(row,api).value);
#ifdef __linux__
    struct stat identity{};struct statvfs stats{};
    assert(stat("/",&identity)==0&&statvfs("/",&stats)==0);
    LinuxCapacityMount root{std::to_string(major(identity.st_dev))+":"+std::to_string(minor(identity.st_dev)),"/","ext4"};
    const auto capacity=read_linux_mount_capacity(root);
    assert(capacity.value&&capacity.value==linux_filesystem_capacity(stats.f_blocks,stats.f_frsize).value);
    root.device="4294967295:4294967295";assert(!read_linux_mount_capacity(root).value);
    root.path="relative";assert(!read_linux_mount_capacity(root).value);
    const auto native=read_native_linux_formatted_capacity();assert(native.value);
    SampleOptions options;options.include_processes=false;options.include_app_history=false;
    options.include_networks=false;options.include_gpus=false;options.include_user_sessions=false;
    options.include_services=false;options.include_startup_entries=false;
    Monitor monitor;const auto snapshot=monitor.sample(options);assert(!snapshot.disks.empty());
    unsigned measured=0,absent=0;
    for(const auto& disk:snapshot.disks){
        if(disk.formatted_capacity_bytes.value){++measured;assert(disk.formatted_capacity_bytes.support==Support::estimated);}
        else {++absent;assert(!disk.formatted_capacity_bytes.note.empty());}
    }
    std::cout<<"Native mounted-filesystem capacity: "<<measured<<" measured disk rows, "<<absent<<" absent; pinned root reference and wrong-device rejection passed\n";
#else
    assert(!read_native_linux_formatted_capacity().value);
#endif
    std::cout<<"Formatted capacity: strict mountinfo/escapes, dedup/alias retry/conflicts, partial failure, zero vs absence, overflow and bounds passed\n";
}
