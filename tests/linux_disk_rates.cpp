#include "../src/linux_disk_rates.hpp"
#include "pcinsight/monitor.hpp"
#include <cassert>
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <thread>
using namespace pcinsight;
using namespace pcinsight::detail;
using namespace std::chrono_literals;
using Clock=std::chrono::steady_clock;
int main(){
    const auto parsed=parse_linux_disk_stat(" 10 0 100 40 20 0 200 60 1 50 100\n");
    assert(parsed&&(*parsed)[0]==10&&(*parsed)[9]==50);
    assert(parse_linux_disk_stat("1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17"));
    for(auto text:{"","1 2 3 4","1 2 3 4 5 6 7 8 9 10 -1","1 2 3 4 5 6 7 8 9 10 +1","1 2 3 4 5 6 7 8 9 10 18446744073709551616","1 2 3 4 5 6 7 8 9 10 11 garbage"})assert(!parse_linux_disk_stat(text));
    assert(!parse_linux_disk_stat(std::string(16385,'1')));
    std::string fields;for(int i=0;i<65;++i)fields+="1 ";assert(!parse_linux_disk_stat(fields));
    assert(valid_linux_disk_device("259:0"));
    for(auto device:{"","259","-1:0","1:+2","1:2:3","4294967296:0","1:2x"})assert(!valid_linux_disk_device(device));
    LinuxDiskRateTracker tracker;
    std::vector<LinuxDiskCounters> rows{{"disk0","8:0",123,*parsed}};
    const Clock::time_point start(10s);
    auto sample=[&](auto at){return tracker.sample(rows,at).at("disk0");};
    assert(!sample(start).read.value);
    rows[0].stat={12,0,108,48,23,0,204,72,0,550,200};
    auto rates=sample(start+2s);
    assert(rates.read.value==2048&&rates.write.value==1024&&rates.transfer.value==3072);
    assert(rates.active.value&&std::abs(*rates.active.value-25.0)<1e-6);
    assert(rates.response.value&&std::abs(*rates.response.value-4.0)<1e-6);
    assert(rates.read.support==Support::estimated&&rates.response.support==Support::estimated);
    rates=sample(start+3s);
    assert(rates.read.value==0&&rates.write.value==0&&rates.active.value==0);
    assert(!rates.response.value); // no completed operations: no artificial zero latency
    rows[0].stat[9]+=2000;
    rates=sample(start+4s);assert(rates.active.value==100); // capped estimate
    for(unsigned field=0;field<11;++field){
        tracker.reset();rows[0].stat.fill(100);sample(start);
        rows[0].stat[field]=99;rates=sample(start+1s);
        if(field==8)assert(rates.read.value==0); // in-flight is a gauge, not a counter
        else assert(!rates.read.value&&!rates.active.value&&!rates.response.value);
        rows[0].stat[field]+=1;assert(sample(start+2s).read.value);
    }
    ++*rows[0].disk_sequence;assert(!sample(start+3s).read.value);
    rows[0].disk_sequence.reset();assert(!sample(start+4s).read.value);
    assert(sample(start+5s).read.value); // old kernels without diskseq still usable
    rows[0].device="8:16";assert(!sample(start+6s).read.value);
    tracker.sample({},start+7s);assert(!sample(start+8s).read.value);
    assert(!sample(start+8s).read.value);
    assert(!sample(start+7s).read.value);
    assert(sample(start+9s).read.value);
    rows[0].name="renamed";assert(!tracker.sample(rows,start+10s).at("renamed").read.value);
    rows[0].name="disk0";tracker.reset();rows[0].stat.fill(0);sample(start);
    rows[0].stat.fill(std::numeric_limits<std::uint64_t>::max());
    rates=sample(start+1ns);
    assert(!rates.read.value&&!rates.write.value&&!rates.transfer.value);
    assert(rates.active.value==100&&rates.response.value&&std::isfinite(*rates.response.value));
    // Sector multiplication and combined sums must not wrap even on a long interval.
    tracker.reset();rows[0].stat.fill(0);sample(start);
    rows[0].stat[2]=rows[0].stat[6]=std::numeric_limits<std::uint64_t>::max()/512;
    rates=sample(start+4s);assert(rates.read.value&&rates.write.value&&rates.transfer.value);
    assert(*rates.transfer.value>*rates.read.value);
    tracker.reset();rows[0].stat.fill(0);sample(start);
    rows[0].stat[2]=3;rates=sample(start+2s);assert(rates.read.value==768);
    rows.push_back(rows[0]);bool threw=false;
    try{tracker.sample(rows,start);}catch(const std::invalid_argument&){threw=true;}
    assert(threw);rows.pop_back();assert(!sample(start+3s).read.value);
    for(const auto& bad:std::vector<LinuxDiskCounters>{{"","8:0",{},{}},{"x","bad",{},{}},{std::string("a\0b",3),"8:0",{},{}},{std::string(4097,'x'),"8:0",{},{}}}){
        threw=false;try{tracker.sample(std::span(&bad,1),start);}catch(const std::invalid_argument&){threw=true;}assert(threw);
    }
    std::vector<LinuxDiskCounters> too_many(100001);threw=false;
    try{tracker.sample(too_many,start);}catch(const std::invalid_argument&){threw=true;}assert(threw);
    tracker.reset();sample(Clock::time_point::min());assert(sample(Clock::time_point::max()).read.value==0);
#ifdef __linux__
    SampleOptions options;options.include_processes=false;options.include_app_history=false;
    options.include_networks=false;options.include_gpus=false;options.include_user_sessions=false;
    options.include_services=false;options.include_startup_entries=false;
    Monitor monitor;const auto first=monitor.sample(options);assert(!first.disks.empty());
    for(const auto& disk:first.disks)assert(!disk.read_bytes_per_second.value&&!disk.active_percent.value);
    std::this_thread::sleep_for(30ms);
    const auto second=monitor.sample(options);unsigned measured=0;
    for(const auto& disk:second.disks)if(disk.active_percent.value){
        ++measured;assert(disk.read_bytes_per_second.value&&disk.write_bytes_per_second.value);
        assert(disk.transfer_bytes_per_second.value&&*disk.active_percent.value>=0&&*disk.active_percent.value<=100);
        assert(disk.active_percent.support==Support::estimated);
    }
    assert(measured>0);
    options.include_disks=false;assert(monitor.sample(options).disks.empty());
    options.include_disks=true;
    for(const auto& disk:monitor.sample(options).disks)assert(!disk.read_bytes_per_second.value&&!disk.active_percent.value);
    std::cout<<"Native Linux disk rates: "<<measured<<" measured devices; opt-out/re-enable resets verified\n";
#endif
    std::cout<<"Disk rates: parser, reset of all cumulative fields, identity, interval, gauge, zero, response, overflow and limits passed\n";
}
