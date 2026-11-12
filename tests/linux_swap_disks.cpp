#include "../src/linux_swap_disks.hpp"
#include "../src/linux_formatted_capacity.hpp"
#include "pcinsight/monitor.hpp"
#include <cassert>
#include <iostream>
#ifdef __linux__
#include <sys/stat.h>
#include <sys/sysmacros.h>
#endif
using namespace pcinsight;
using namespace pcinsight::detail;
const std::string header="Filename\tType\tSize\tUsed\tPriority\n";
std::string entry(std::string path="/swapfile",std::string type="file",std::string used="0"){
    return path+" "+type+" 1024 "+used+" -2\n";
}
int main(){
    auto parsed=parse_linux_swaps(header);assert(parsed.value&&parsed.value->empty());
    parsed=parse_linux_swaps(header+entry("/a\\040b\\134040"));
    assert(parsed.value&&parsed.value->at(0).path=="/a b\\040"&&parsed.value->at(0).file);
    parsed=parse_linux_swaps(header+entry("/a\\011b\\012c","partition","12"));
    assert(parsed.value&&parsed.value->at(0).path=="/a\tb\nc"&&!parsed.value->at(0).file&&parsed.value->at(0).used_kib==12);
    for(const auto& text:std::vector<std::string>{"",entry(),header+entry("relative"),header+entry("/x\\000"),header+entry("/x","other"),header+entry("/x","file","1025"),header+"/x file -1 0 -2\n",header+"/x file 18446744073709551616 0 -2\n",header+"/x file 10 0 2147483648\n",header+entry()+entry(),header+entry()+"bad\n"})assert(!parse_linux_swaps(text).value);
    assert(!parse_linux_swaps(header+entry(std::string("/x\0y",4))).value);
    assert(!parse_linux_swaps(header+entry("/"+std::string(16384,'x'))).value);
    assert(!parse_linux_swaps(std::string(16*1024*1024+1,'x')).value);
    std::string many=header;for(unsigned i=0;i<100001;++i)many+=entry("/swap"+std::to_string(i));
    assert(!parse_linux_swaps(many).value);
    LinuxSwapApi api;std::string first=header+entry()+entry("/dev/sdb1","partition"),second=first;
    unsigned reads=0,queries=0;
    api.inventory=[&]()->Metric<std::string>{return {++reads%2?first:second,Support::available,"fixture"};};
    api.disk=[&](const LinuxSwapEntry& e)->Metric<std::string>{++queries;return {e.file?"disk0":"disk1",Support::estimated,"fixture"};};
    auto report=sample_linux_swap_disks(api);assert(report.value&&report.value->complete&&report.value->disks.size()==2&&queries==2&&reads==2);
    second=header+entry("/dev/sdb1","partition","128")+entry("/swapfile","file","256");
    report=sample_linux_swap_disks(api);assert(report.value&&report.value->complete); // Used/order changes aren't configuration changes
    second=first+entry("/extra");assert(!sample_linux_swap_disks(api).value);
    second=header+entry()+"/dev/sdb1 partition 1024 0 -3\n";assert(!sample_linux_swap_disks(api).value);
    second="bad";assert(!sample_linux_swap_disks(api).value);second=first;
    api.may_exclude_others=[](const std::string&){return false;};
    report=sample_linux_swap_disks(api);assert(report.value&&!report.value->complete&&report.value->disks.size()==2);
    std::vector<DiskInfo> layered(3);for(unsigned i=0;i<3;++i)layered[i].id="disk"+std::to_string(i);
    apply_linux_swap_disk_flags(layered,report);
    assert(layered[0].contains_page_file.value==true&&layered[1].contains_page_file.value==true&&!layered[2].contains_page_file.value);
    api.may_exclude_others={};
    api.disk=[](const LinuxSwapEntry& e)->Metric<std::string>{if(e.file)return {"disk0",Support::estimated,"fixture"};return {{},Support::permission_required,"denied"};};
    report=sample_linux_swap_disks(api);assert(report.value&&!report.value->complete&&report.value->disks.contains("disk0"));
    std::vector<DiskInfo> disks(3);for(unsigned i=0;i<3;++i)disks[i].id="disk"+std::to_string(i);
    apply_linux_swap_disk_flags(disks,report);
    assert(disks[0].contains_page_file.value==true);
    assert(!disks[1].contains_page_file.value&&disks[1].contains_page_file.support==Support::permission_required);
    assert(!disks[2].contains_page_file.value);
    LinuxSwapDiskReport complete;complete.disks.insert("disk1");
    apply_linux_swap_disk_flags(disks,{complete,Support::estimated,"fixture"});
    assert(disks[0].contains_page_file.value==false&&disks[1].contains_page_file.value==true&&disks[2].contains_page_file.value==false);
    complete.disks.insert("missing");apply_linux_swap_disk_flags(disks,{complete,Support::estimated,"fixture"});
    assert(disks[1].contains_page_file.value==true&&!disks[0].contains_page_file.value);
    disks[0].id="disk1";apply_linux_swap_disk_flags(disks,{complete,Support::estimated,"fixture"});
    assert(!disks[0].contains_page_file.value&&!disks[1].contains_page_file.value);disks[0].id="disk0";
    apply_linux_swap_disk_flags(disks,{{},Support::unsupported,"inventory unavailable"});
    for(const auto& disk:disks)assert(!disk.contains_page_file.value);
    reads=queries=0;first=second=header;report=sample_linux_swap_disks(api);assert(report.value&&report.value->complete&&queries==0);
    apply_linux_swap_disk_flags(disks,report);for(const auto& disk:disks)assert(disk.contains_page_file.value==false);
    first=second="bad";reads=queries=0;assert(!sample_linux_swap_disks(api).value&&queries==0&&reads==1);
    assert(!sample_linux_swap_disks({}).value);
    api.inventory=[]()->Metric<std::string>{return {{},Support::permission_required,"denied inventory"};};
    report=sample_linux_swap_disks(api);assert(!report.value&&report.support==Support::permission_required);
    first=second=header+entry();reads=0;
    api.inventory=[&]()->Metric<std::string>{if(++reads==2)return {{},Support::unsupported,"lost inventory"};return {first,Support::available,"fixture"};};
    assert(!sample_linux_swap_disks(api).value);
    api.inventory=[&]()->Metric<std::string>{return {first,Support::available,"fixture"};};
    for(const auto& bad:std::vector<std::string>{"",".","../x",std::string("x\0y",3),std::string(4097,'x')}){
        api.disk=[&](const LinuxSwapEntry&)->Metric<std::string>{return {bad,Support::available,"fixture"};};
        report=sample_linux_swap_disks(api);assert(report.value&&!report.value->complete&&report.value->disks.empty());
    }
#ifdef __linux__
    assert(!read_linux_swap_path_disk({"relative",true}).value);
    assert(!read_linux_swap_path_disk({"/dev/null",false}).value); // character device is not swap partition
    assert(!read_linux_swap_path_disk({"/dev/null",true}).value);
    assert(!read_linux_swap_path_disk({"/proc/self/stat",true}).value); // unsupported filesystem, not guessed
    struct stat identity{};assert(stat("/etc/passwd",&identity)==0);
    const auto reference=read_linux_swap_path_disk({"/etc/passwd",true}); // metadata-only reference probe; NOT an active swap file
    if(reference.value)assert(reference.value==resolve_linux_block_device(std::to_string(major(identity.st_dev))+":"+std::to_string(minor(identity.st_dev))));
    const auto native=read_native_linux_swap_disks();assert(native.value||!native.note.empty());
    SampleOptions options;options.include_processes=false;options.include_app_history=false;
    options.include_networks=false;options.include_gpus=false;options.include_user_sessions=false;
    options.include_services=false;options.include_startup_entries=false;
    Monitor monitor;const auto snapshot=monitor.sample(options);assert(!snapshot.disks.empty());
    unsigned yes=0,no=0,unknown=0;
    for(const auto& disk:snapshot.disks){
        assert(!disk.contains_page_file.note.empty());
        if(!disk.contains_page_file.value)++unknown;
        else {assert(disk.contains_page_file.support==Support::estimated);if(*disk.contains_page_file.value)++yes;else ++no;}
    }
    options.include_disks=false;assert(monitor.sample(options).disks.empty());
    std::cout<<"Native Linux swap flags: yes="<<yes<<", no="<<no<<", unknown="<<unknown<<"; ordinary-file metadata probe="<<reference.value.value_or("unavailable")<<" (not activated swap)\n";
#else
    assert(!read_native_linux_swap_disks().value);
#endif
    std::cout<<"Swap disks: strict header/escapes/types/bounds, stable configuration, used changes, incomplete mapping and yes/no/unknown passed\n";
}
