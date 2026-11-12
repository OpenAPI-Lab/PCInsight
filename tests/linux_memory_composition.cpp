#include "../src/linux_memory.hpp"
#include "../src/linux_zram.hpp"
#include "pcinsight/monitor.hpp"
#include <cassert>
#include <iostream>
#include <limits>
#ifdef __linux__
#include <filesystem>
#include <fstream>
#include <cstdlib>
#include <sys/stat.h>
#endif
using namespace pcinsight;
using namespace pcinsight::detail;
int main(){
    const std::string text="MemTotal: 1000 kB\nMemAvailable: 400 kB\nMemFree: 100 kB\nBuffers: 10 kB\nCached: 200 kB\nSReclaimable: 50 kB\nSUnreclaim: 20 kB\nAnonPages: 300 kB\nShmem: 30 kB\nDirty: 5 kB\nWriteback: 2 kB\nZswap: 8 kB\nZswapped: 24 kB\n";
    const auto input=parse_linux_meminfo(text);assert(input.value&&input.value->size()==13);
    MemoryInfo m;apply_linux_meminfo(m,input);const auto& d=m.linux_details;
    assert(d.composition.value&&d.composition.support==Support::estimated);
    assert(d.composition.value->unavailable_estimate_bytes==600*1024);
    assert(d.composition.value->reclaimable_estimate_bytes==300*1024&&d.composition.value->free_bytes==100*1024);
    assert(d.free_bytes.value==100*1024&&d.buffers_bytes.value==10*1024&&d.page_cache_bytes.value==200*1024);
    assert(d.anonymous_bytes.value==300*1024&&d.shared_bytes.value==30*1024);
    assert(d.slab_reclaimable_bytes.value==50*1024&&d.slab_unreclaimable_bytes.value==20*1024);
    assert(d.dirty_bytes.value==5*1024&&d.writeback_bytes.value==2*1024);
    assert(d.zswap_pool_bytes.value==8*1024&&d.zswap_original_bytes.value==24*1024);
    assert(m.compressed_bytes.value==d.zswap_pool_bytes.value&&!m.paged_pool_bytes.value&&!m.nonpaged_pool_bytes.value);
    for(auto field:{"MemTotal","MemAvailable","MemFree"}){auto partial=input;partial.value->erase(field);apply_linux_meminfo(m,partial);assert(!d.composition.value);}
    for(auto field:{"MemFree","Buffers","AnonPages","Shmem","SUnreclaim","Dirty","Writeback","Zswapped"}){
        assert(!parse_linux_meminfo(text+std::string(field)+": 1 kB\n").value);
        assert(!parse_linux_meminfo(std::string(field)+": -1 kB\n").value);
        assert(!parse_linux_meminfo(std::string(field)+": 1 MB\n").value);
    }
    auto edited=input;(*edited.value)["MemFree"]=401*1024;apply_linux_meminfo(m,edited);assert(!d.composition.value&&m.available_bytes.value);
    (*edited.value)["MemFree"]=0;apply_linux_meminfo(m,edited);assert(d.composition.value&&d.composition.value->free_bytes==0);
    (*edited.value)["MemAvailable"]=1000*1024;(*edited.value)["MemFree"]=1000*1024;apply_linux_meminfo(m,edited);
    assert(d.composition.value&&d.composition.value->unavailable_estimate_bytes==0&&d.composition.value->reclaimable_estimate_bytes==0);
    edited=input;(*edited.value)["MemTotal"]=std::numeric_limits<std::uint64_t>::max();(*edited.value)["MemAvailable"]=0;(*edited.value)["MemFree"]=0;apply_linux_meminfo(m,edited);
    assert(d.composition.value&&d.composition.value->unavailable_estimate_bytes==std::numeric_limits<std::uint64_t>::max());

    apply_linux_meminfo(m,input);apply_linux_memory_reservation(m,{1100*1024,Support::available,"fixture"});
    assert(d.firmware_installed_bytes.value==1100*1024&&d.installed_minus_usable_bytes.value==100*1024);
    assert(m.hardware_reserved_bytes.value==d.installed_minus_usable_bytes.value&&m.hardware_reserved_bytes.support==Support::estimated);
    assert(d.installed_minus_usable_bytes.support==Support::estimated);
    assert(d.installed_minus_usable_bytes.note.find("NOT hardware-only")!=std::string::npos);
    apply_linux_memory_reservation(m,{1000*1024,Support::available,"fixture measured installed total"});
    assert(d.installed_minus_usable_bytes.value==0&&d.installed_minus_usable_bytes.support==Support::estimated);
    assert(m.hardware_reserved_bytes.value==0&&m.hardware_reserved_bytes.support==Support::estimated);
    m.reported_total_bytes={{},Support::permission_required,"usable RAM source denied"};
    apply_linux_memory_reservation(m,{1100*1024,Support::available,"fixture"});
    assert(!d.installed_minus_usable_bytes.value&&d.installed_minus_usable_bytes.support==Support::permission_required);
    assert(d.installed_minus_usable_bytes.note=="usable RAM source denied"&&!m.hardware_reserved_bytes.value);
    assert(m.hardware_reserved_bytes.support==Support::permission_required);
    m.reported_total_bytes={1000*1024,Support::permission_required,"contradictory denied total"};
    apply_linux_memory_reservation(m,{1100*1024,Support::available,"fixture"});
    assert(!d.installed_minus_usable_bytes.value&&d.installed_minus_usable_bytes.support==Support::permission_required);
    m.reported_total_bytes={1000*1024,Support::unsupported,"contradictory unsupported total"};
    apply_linux_memory_reservation(m,{1100*1024,Support::available,"fixture"});
    assert(!d.installed_minus_usable_bytes.value&&!m.hardware_reserved_bytes.value);
    m.reported_total_bytes={1000*1024,Support::estimated,"fixture estimated usable total"};
    apply_linux_memory_reservation(m,{1100*1024,Support::available,"fixture"});
    assert(d.installed_minus_usable_bytes.value==100*1024&&d.installed_minus_usable_bytes.support==Support::estimated);
    m.reported_total_bytes={{},Support::unsupported,"usable total absent"};
    apply_linux_memory_reservation(m,{1100*1024,Support::available,"fixture"});
    assert(!d.installed_minus_usable_bytes.value&&!m.hardware_reserved_bytes.value);
    apply_linux_meminfo(m,input);
    apply_linux_memory_reservation(m,{1000*1024,Support::estimated,"fixture"});assert(d.installed_minus_usable_bytes.value==0);
    apply_linux_memory_reservation(m,{999*1024,Support::available,"fixture"});assert(!m.hardware_reserved_bytes.value&&!d.installed_minus_usable_bytes.value);
    apply_linux_memory_reservation(m,{{},Support::permission_required,"denied"});assert(!d.firmware_installed_bytes.value&&d.installed_minus_usable_bytes.support==Support::permission_required);
    apply_linux_memory_reservation(m,{1100*1024,Support::unsupported,"not supported"});assert(!d.firmware_installed_bytes.value&&!d.installed_minus_usable_bytes.value);
    apply_linux_memory_reservation(m,{std::numeric_limits<std::uint64_t>::max(),Support::available,"fixture"});assert(d.installed_minus_usable_bytes.value==std::numeric_limits<std::uint64_t>::max()-1000*1024);
    m.reported_total_bytes={0,Support::available,"invalid total fixture"};apply_linux_memory_reservation(m,{1100*1024,Support::available,"fixture"});assert(!d.installed_minus_usable_bytes.value);
    apply_linux_meminfo(m,{{},Support::permission_required,"denied"});assert(!d.composition.value&&!d.free_bytes.value&&d.free_bytes.support==Support::permission_required&&!d.firmware_installed_bytes.value&&!m.hardware_reserved_bytes.value);

    const auto z=parse_linux_zram_memory("4096 1024 2048 0 2048 0 0 0 0\n");
    assert(z.value&&z.value->original_data_bytes==4096&&z.value->compressed_data_bytes==1024&&z.value->allocated_memory_bytes==2048);
    assert(parse_linux_zram_memory("0 0 0 0 0 0 0").value);
    assert(parse_linux_zram_memory("1 2 3 4 5 6 7 8 9 10").value); // future appended counters
    const auto max=std::to_string(std::numeric_limits<std::uint64_t>::max());assert(parse_linux_zram_memory(max+" 0 0 0 0 0 0").value);
    for(const auto& invalid:std::vector<std::string>{"","1 2 3","1 2 3 4 5 6","-1 2 3 4 5 6 7","+1 2 3 4 5 6 7","1x 2 3 4 5 6 7","18446744073709551616 0 0 0 0 0 0","1 2 3 4 5 6 7 kB","1 2 3\n4 5 6 7",std::string("1 2\0 3 4 5 6 7",15),std::string(4097,'1')})assert(!parse_linux_zram_memory(invalid).value);
    std::string many;for(unsigned n=0;n<64;++n)many+="0 ";assert(parse_linux_zram_memory(many).value);
    many+="0 ";assert(!parse_linux_zram_memory(many).value);
#ifdef __linux__
    namespace fs=std::filesystem;
    char pattern[]="/tmp/pcinsight-memory-XXXXXX";const auto* made=::mkdtemp(pattern);assert(made);
    struct Cleanup{fs::path path;~Cleanup(){std::error_code e;fs::remove_all(path,e);}} cleanup{made};const auto root=cleanup.path;
    auto write=[](const fs::path& path,const std::string& content){std::ofstream f(path,std::ios::binary);f<<content;f.close();assert(f);};
    assert(read_linux_zram(root.string()).value->empty());
    fs::create_directory(root/"zram0");write(root/"zram0/dev","252:0\n");write(root/"zram0/mm_stat","4096 1024 2048 0 2048 0 0\n");
    fs::create_directory(root/"zram1");write(root/"zram1/dev","252:1\n"); // absent stats stays unknown, not0
    fs::create_directory(root/"zram-fake");
    auto inventory=read_linux_zram(root.string());assert(inventory.value&&inventory.value->size()==2);
    assert(inventory.value->at(0).id=="zram0"&&inventory.value->at(0).memory.value->allocated_memory_bytes==2048);
    assert(inventory.value->at(1).id=="zram1"&&!inventory.value->at(1).memory.value);
    fs::create_directories(root/"targets/unit");write(root/"targets/unit/dev","252:2\n");write(root/"targets/unit/mm_stat","0 0 0 0 0 0 0\n");
    fs::create_directory_symlink("targets/unit",root/"zram2");inventory=read_linux_zram(root.string());assert(inventory.value->size()==3&&inventory.value->at(2).memory.value);
    write(root/"zram0/mm_stat",std::string(4097,'1'));assert(!read_linux_zram(root.string()).value->at(0).memory.value);
    fs::remove(root/"zram0/mm_stat");assert(::mkfifo((root/"zram0/mm_stat").c_str(),0600)==0);
    assert(!read_linux_zram(root.string()).value->at(0).memory.value); // no FIFO blocking
    fs::remove(root/"zram0/mm_stat");fs::create_symlink("dev",root/"zram0/mm_stat");assert(!read_linux_zram(root.string()).value->at(0).memory.value);
    write(root/"zram1/dev","bad\n");assert(!read_linux_zram(root.string()).value->at(1).memory.value);
    assert(!read_linux_zram((root/"absent").string()).value);
    assert(!read_linux_zram(std::string("/tmp\0bad",8)).value);
    for(unsigned n=3;n<257;++n)fs::create_directory(root/("zram"+std::to_string(n)));
    assert(!read_linux_zram(root.string()).value); // device bound: no partial result
    fs::create_directory(root/"blocks");for(unsigned n=0;n<4097;++n)fs::create_directory(root/"blocks"/std::to_string(n));
    assert(!read_linux_zram((root/"blocks").string()).value); // enumeration bound even without zram
    SampleOptions options;options.include_processes=false;options.include_app_history=false;options.include_linux_process_descriptions=false;
    options.include_disks=false;options.include_networks=false;options.include_gpus=false;options.include_services=false;options.include_user_sessions=false;options.include_startup_entries=false;
    Monitor monitor;const auto snapshot=monitor.sample(options);const auto& observed=snapshot.memory;
    const auto& c=observed.linux_details.composition;assert(c.value&&c.support==Support::estimated);
    assert(c.value->unavailable_estimate_bytes==observed.in_use_bytes.value);
    assert(c.value->unavailable_estimate_bytes+c.value->reclaimable_estimate_bytes+c.value->free_bytes==observed.reported_total_bytes.value);
    assert(observed.linux_details.zswap_pool_bytes.value==observed.compressed_bytes.value);
    assert(observed.hardware_reserved_bytes.value==observed.linux_details.installed_minus_usable_bytes.value);
    assert(!observed.paged_pool_bytes.value&&!observed.nonpaged_pool_bytes.value);
    const auto native=read_linux_zram();assert(native.value&&observed.linux_details.zram_devices.value);
    std::cout<<"Native firmware installed present="<<observed.linux_details.firmware_installed_bytes.value.has_value()
        <<"; difference support="<<static_cast<int>(observed.linux_details.installed_minus_usable_bytes.support)
        <<"; difference note="<<observed.linux_details.installed_minus_usable_bytes.note<<'\n';
    std::cout<<"Native Linux composition: unavailable="<<c.value->unavailable_estimate_bytes<<" reclaimable estimate="<<c.value->reclaimable_estimate_bytes<<" free="<<c.value->free_bytes<<" bytes; observed zram devices="<<observed.linux_details.zram_devices.value->size()<<"; zswap present="<<observed.linux_details.zswap_pool_bytes.value.has_value()<<"; installed-minus-usable present="<<observed.linux_details.installed_minus_usable_bytes.value.has_value()<<'\n';
#else
    assert(!read_linux_zram().value);
#endif
    std::cout<<"Linux memory composition: partition, overlapping source fields, unknown vs zero, checked reservation proxy, zram counters/strict bounds and reset tests passed\n";
}
