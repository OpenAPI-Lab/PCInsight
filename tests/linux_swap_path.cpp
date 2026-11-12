#include "../src/linux_swap_disks.hpp"
#include "../src/linux_formatted_capacity.hpp"
#include <cassert>
#include <iostream>
#include <limits>
#ifdef __linux__
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <sys/vfs.h>
#include <linux/magic.h>
#include <unistd.h>
#endif
using namespace pcinsight;
using namespace pcinsight::detail;
int main(){
#ifdef __linux__
    namespace fs=std::filesystem;
    char pattern[]="/tmp/pcinsight-swap-path-XXXXXX";const auto* made=::mkdtemp(pattern);assert(made);
    struct Cleanup{fs::path root;~Cleanup(){std::error_code error;fs::remove_all(root,error);}} cleanup{made};
    const auto path=cleanup.root/"ordinary file\\reference";
    auto create=[&]{std::ofstream file(path,std::ios::binary);file<<"private test fixture, NOT activated swap";file.close();assert(file);fs::resize_file(path,2*1024*1024);};create();
    struct stat identity{};assert(::stat(path.c_str(),&identity)==0);
    const auto device=std::to_string(major(identity.st_dev))+":"+std::to_string(minor(identity.st_dev));
    const auto actual_disk=resolve_linux_block_device(device);
    struct statfs filesystem{};assert(::statfs(path.c_str(),&filesystem)==0);
    if(filesystem.f_type!=EXT2_SUPER_MAGIC||!actual_disk){
        assert(!read_linux_swap_path_disk({path.string(),true,1024,0,-2}).value);
        std::cout<<"Native swap-file positive fixture skipped: /tmp lacks supported ext2/3/4 direct block mapping; source absence verified, no swap activated\n";
        return 0;
    }
    LinuxSwapEntry reference{path.string(),true,1024,0,-2};
    const auto result=read_linux_swap_path_disk(reference);assert(result.value==actual_disk&&result.support==Support::estimated);
    unsigned queries=0;
    auto resolver=[&](const std::string& dev){++queries;assert(dev==device);return actual_disk;};
    assert(read_linux_swap_path_disk_with_resolver(reference,resolver).value==actual_disk&&queries==1);
    auto invalid=reference;invalid.size_kib=2049;queries=0;assert(!read_linux_swap_path_disk_with_resolver(invalid,resolver).value&&queries==0);
    invalid.size_kib=std::numeric_limits<std::uint64_t>::max();assert(!read_linux_swap_path_disk_with_resolver(invalid,resolver).value&&queries==0);
    invalid=reference;invalid.used_kib=1025;assert(!read_linux_swap_path_disk_with_resolver(invalid,resolver).value&&queries==0);
    invalid=reference;invalid.file=false;assert(!read_linux_swap_path_disk_with_resolver(invalid,resolver).value&&queries==0);
    assert(!read_linux_swap_path_disk_with_resolver(reference,{}).value);
    for(int change=0;change<5;++change){
        auto mutate=[&](const std::string& dev)->std::optional<std::string>{
            assert(dev==device);
            if(change==0){fs::rename(path,cleanup.root/"moved");create();}
            if(change==1)fs::resize_file(path,4096);
            if(change==2)fs::remove(path);
            if(change==3){fs::permissions(path,fs::perms::owner_read,fs::perm_options::replace);}
            if(change==4){std::ofstream file(path,std::ios::app);file<<"changed";}
            return actual_disk;
        };
        assert(!read_linux_swap_path_disk_with_resolver(reference,mutate).value);
        if(fs::exists(path))fs::permissions(path,fs::perms::owner_all,fs::perm_options::add);
        create();
    }
    fs::remove(path);assert(::mkfifo(path.c_str(),0600)==0);
    assert(!read_linux_swap_path_disk(reference).value);fs::remove(path);create();
    assert(!read_linux_swap_path_disk_with_resolver(reference,[](const auto&)->std::optional<std::string>{return "../invalid";}).value);
    // Complete production file resolver pipeline, with synthetic inventory only.
    std::string escaped;for(const auto c:path.string()){if(c==' ')escaped+="\\040";else if(c=='\\')escaped+="\\134";else escaped+=c;}
    LinuxSwapApi api;const auto inventory="Filename Type Size Used Priority\n"+escaped+" file 1024 0 -2\n";
    api.inventory=[&]()->Metric<std::string>{return {inventory,Support::available,"SYNTHETIC inventory; ordinary file was never activated as swap"};};
    api.disk=read_linux_swap_path_disk;
    auto sample=sample_linux_swap_disks(api);assert(sample.value&&sample.value->complete&&sample.value->disks.contains(*actual_disk));
    std::vector<DiskInfo> disks(1);disks[0].id=*actual_disk;apply_linux_swap_disk_flags(disks,sample);assert(disks[0].contains_page_file.value==true);
    api.disk=[&](const LinuxSwapEntry& e){return read_linux_swap_path_disk_with_resolver(e,[&](const auto&)->std::optional<std::string>{fs::resize_file(path,4096);return actual_disk;});};
    sample=sample_linux_swap_disks(api);assert(sample.value&&!sample.value->complete&&sample.value->disks.empty());
    apply_linux_swap_disk_flags(disks,sample);assert(!disks[0].contains_page_file.value);
    std::cout<<"Native ordinary-file metadata pipeline: "<<device<<" -> "<<*actual_disk<<"; endpoint replacement/truncation/unlink/mode/write, size overflow, FIFO and unknown-not-false passed. Inventory SYNTHETIC; file NOT activated as swap\n";
#else
    assert(!read_linux_swap_path_disk_with_resolver({"/file",true,1,0,0},[](const auto&)->std::optional<std::string>{return "disk";}).value);
#endif
}
