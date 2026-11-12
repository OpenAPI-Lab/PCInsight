#include "../src/linux_disk_properties.hpp"
#include "pcinsight/monitor.hpp"
#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <map>
#include <sstream>
using namespace pcinsight;
using namespace pcinsight::detail;
int main(){
    assert(linux_disk_capacity("0\n").value==0);
    assert(linux_disk_capacity("1").value==512);
    assert(linux_disk_capacity("976562500\n").value==500000000000ULL);
    const auto max=std::numeric_limits<std::uint64_t>::max()/512;
    assert(linux_disk_capacity(std::to_string(max)).value==max*512);
    for(const auto& input:std::vector<std::string>{"","-1","+1"," 1","1junk","1 2",std::string("1\0",2),"18446744073709551615","18446744073709551616",std::to_string(max+1),std::string(129,'1')}){
        const auto value=linux_disk_capacity(input);assert(!value.value&&!value.note.empty());
    }
    assert(linux_disk_capacity("2 \t\r\n").value==1024);
    DiskInfo disk;assert(!disk.rotational.value);
    disk.id="nvme-fake";
    apply_linux_disk_properties(disk,"2","0\n","scsi");
    assert(disk.capacity_bytes.value==1024&&disk.rotational.value==false);
    assert(disk.media_type=="non-rotational"&&disk.bus_type=="scsi");
    assert(!disk.reported_media_type.value); // no invented SSD behind SCSI or virtual disk
    disk.id="sda";
    apply_linux_disk_properties(disk,"3","1\n","nvme");
    assert(disk.rotational.value==true&&disk.media_type=="rotational"&&disk.bus_type=="NVMe");
    assert(!disk.reported_media_type.value); // contradictory hints don't become HDD/SSD
    for(const auto& input:std::vector<std::string>{"","2","-1","+1","00","01"," 0","0junk",std::string("0\0",2),std::string(129,'0')}){
        apply_linux_disk_properties(disk,"bad",input,"");
        assert(!disk.capacity_bytes.value&&!disk.rotational.value);
        assert(disk.media_type=="unknown"&&disk.bus_type=="unknown"&&!disk.reported_media_type.value);
    }
    for(const auto& bus:std::vector<std::string>{"","../nvme","nvme\n",std::string("nvme\0",5),std::string(129,'a')}){
        apply_linux_disk_properties(disk,"0","0",bus);assert(disk.bus_type=="unknown");
    }
    apply_linux_disk_properties(disk,"1","1","virtio");assert(disk.bus_type=="virtio");
    // Reusing a row must clear previously published values when input goes away.
    apply_linux_disk_properties(disk,"","","");
    assert(!disk.capacity_bytes.value&&!disk.rotational.value&&disk.media_type=="unknown");
#ifdef __linux__
    auto read=[](const std::filesystem::path& path){std::ifstream file(path);std::ostringstream text;text<<file.rdbuf();return text.str();};
    std::map<std::string,std::pair<std::string,std::string>> before;
    for(const auto& entry:std::filesystem::directory_iterator("/sys/block"))
        before.emplace(entry.path().filename().string(),std::make_pair(read(entry.path()/"size"),read(entry.path()/"queue/rotational")));
    SampleOptions options;options.include_processes=false;options.include_app_history=false;
    options.include_networks=false;options.include_gpus=false;options.include_user_sessions=false;
    options.include_services=false;options.include_startup_entries=false;
    Monitor monitor;const auto snapshot=monitor.sample(options);
    assert(!snapshot.disks.empty());unsigned checked=0;
    for(const auto& native:snapshot.disks){
        if(native.reported_media_type.value){
            assert(native.linux_details.value);
            assert(native.reported_media_type.value==native.linux_details.value->media_type.value);
            assert(native.reported_media_type.support==Support::estimated);
        }
        const auto found=before.find(native.id);if(found==before.end())continue;
        const auto base=std::filesystem::path("/sys/block")/native.id;
        const auto size=read(base/"size"),rotation=read(base/"queue/rotational");
        if(size!=found->second.first||rotation!=found->second.second)continue;
        DiskInfo expected;apply_linux_disk_properties(expected,size,rotation,"");
        assert(native.capacity_bytes.value==expected.capacity_bytes.value);
        assert(native.rotational.value==expected.rotational.value&&native.media_type==expected.media_type);
        if(native.capacity_bytes.value)assert(*native.capacity_bytes.value%512==0);
        ++checked;
    }
    assert(checked>0);
    options.include_disks=false;assert(monitor.sample(options).disks.empty());
    std::cout<<"Native Linux disk properties: "<<checked<<" stable rows checked against sysfs; read-only\n";
#endif
    std::cout<<"Disk properties: capacity boundaries/malformed input, explicit rotation hint, unknown physical medium and no name-based bus guessing passed\n";
}
