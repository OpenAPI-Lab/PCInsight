#include "../src/linux_process_io.hpp"
#include "pcinsight/monitor.hpp"
#include <algorithm>
#include <cassert>
#include <fstream>
#include <iostream>
#include <limits>
#include <stdexcept>
#ifdef __linux__
#include <fcntl.h>
#include <unistd.h>
#endif
using namespace pcinsight;
using namespace pcinsight::detail;
using Clock=std::chrono::steady_clock;
namespace fs=std::filesystem;
std::string stat_text(std::uint32_t pid=123,std::uint64_t start=100){return std::to_string(pid)+" (io fixture) S 12 0 0 0 -1 0 0 0 0 0 0 0 0 0 20 0 3 0 "+std::to_string(start)+" 12345 0 0";}
void write(const fs::path& path,const std::string& text){std::ofstream out(path,std::ios::binary);assert(out);out<<text;out.close();assert(out);}
LinuxProcessIoObservation observation(std::uint64_t r,std::uint64_t w,int seconds,std::uint64_t start=100){return {123,start,{LinuxProcessIoCounters{r,w},Support::available,"fixture"},Clock::time_point{}+std::chrono::seconds(seconds)};}
Metric<std::uint64_t> take(LinuxProcessIoRateTracker& tracker,const LinuxProcessIoObservation& row){return tracker.sample(std::span(&row,1)).at(row.pid);}
int main(){
    const auto zero=parse_linux_process_io_counters("read_bytes:0\nwrite_bytes:0\n");assert(zero.value&&zero.value->read_bytes==0&&zero.value->write_bytes==0);
    const auto value=parse_linux_process_io_counters("rchar: 999\nwchar: 999\nread_bytes: 100\nwrite_bytes:\t200\ncancelled_write_bytes: 999\n");assert(value.value&&value.value->read_bytes==100&&value.value->write_bytes==200);
    const auto max=std::numeric_limits<std::uint64_t>::max();
    const auto huge=parse_linux_process_io_counters("read_bytes: "+std::to_string(max)+"\nwrite_bytes: "+std::to_string(max));assert(huge.value&&huge.value->read_bytes==max&&huge.value->write_bytes==max);
    assert(!parse_linux_process_io("read_bytes: "+std::to_string(max)+"\nwrite_bytes: 1")); // legacy absolute sum still checked
    for(const auto& bad:std::vector<std::string>{"","read_bytes:1","read_bytes: -1\nwrite_bytes: 2","read_bytes: +1\nwrite_bytes: 2","read_bytes: 1.5\nwrite_bytes: 2","read_bytes: 1 kB\nwrite_bytes: 2","read_bytes: 1 2\nwrite_bytes: 2","read_bytes: 18446744073709551616\nwrite_bytes: 2","read_bytes: 0\nread_bytes:0\nwrite_bytes:0","read_bytes: 0\nwrite_bytes:0\nwrite_bytes:0",std::string("read_bytes:0\nwrite_bytes:0\0",27),std::string(65537,'x')})assert(!parse_linux_process_io_counters(bad).value);
    std::string many;for(unsigned n=0;n<10001;++n)many+="x\n";assert(!parse_linux_process_io_counters(many).value);
    const auto expected=parse_linux_process_stat(stat_text());assert(expected&&expected->pid==123&&expected->start_ticks==100);
    assert(linux_process_io_for_identity(*expected,expected,expected,value).value);
    for(int mode=0;mode<5;++mode){auto before=expected,after=expected;if(mode==0)before.reset();if(mode==1)after.reset();if(mode==2)before->pid=124;if(mode==3)before->start_ticks=101;if(mode==4)after->start_ticks=101;assert(!linux_process_io_for_identity(*expected,before,after,value).value);}
    auto boot=*expected;boot.start_ticks=0;assert(linux_process_io_for_identity(boot,boot,boot,zero).value);
    auto invalid=*expected;invalid.pid=0;assert(!linux_process_io_for_identity(invalid,invalid,invalid,zero).value);
    const Metric<LinuxProcessIoCounters> denied{{},Support::permission_required,"denied fixture"};assert(linux_process_io_for_identity(*expected,expected,expected,denied).support==Support::permission_required);
    LinuxProcessIoRateTracker tracker;
    {
        auto before=Metric<LinuxProcessStatus>{LinuxProcessStatus{123,0,{},{}},Support::estimated,"fixture"};
        auto after=before;
        assert(linux_process_io_uid(before,after).value==0); // UID0 is not absence.
        after.value->real_uid=7;assert(!linux_process_io_uid(before,after).value);
        after=before;after.value->pid=124;assert(!linux_process_io_uid(before,after).value);
        after=before;after.support=Support::permission_required;
        assert(!linux_process_io_uid(before,after).value&&linux_process_io_uid(before,after).support==Support::permission_required);
        auto a=observation(0,0,1);a.real_uid={0,Support::estimated,"fixture"};
        take(tracker,a);assert(!tracker.attributed_uid(123).value);
        a=observation(1,1,2);a.real_uid={0,Support::estimated,"fixture"};
        assert(take(tracker,a).value==2&&tracker.attributed_uid(123).value==0);
        a=observation(2,2,3);a.real_uid={7,Support::estimated,"fixture"};
        assert(take(tracker,a).value==2&&!tracker.attributed_uid(123).value); // process rate valid, user interval discarded.
        a=observation(3,3,4);a.real_uid={7,Support::estimated,"fixture"};
        assert(take(tracker,a).value==2&&tracker.attributed_uid(123).value==7);
        a=observation(4,4,5);a.real_uid={{},Support::permission_required,"denied"};
        assert(take(tracker,a).value==2&&!tracker.attributed_uid(123).value&&tracker.attributed_uid(123).support==Support::permission_required);
        a=observation(5,5,6);a.real_uid={7,Support::estimated,"fixture"};
        assert(take(tracker,a).value==2&&!tracker.attributed_uid(123).value);
        a=observation(6,6,7);a.real_uid={7,Support::estimated,"fixture"};
        assert(take(tracker,a).value==2&&tracker.attributed_uid(123).value==7);
        tracker.reset();assert(!tracker.attributed_uid(123).value);
    }
    assert(!take(tracker,observation(10,20,1)).value);
    auto rate=take(tracker,observation(110,220,3));assert(rate.value==150&&rate.support==Support::estimated);
    assert(take(tracker,observation(110,220,4)).value==0);
    assert(!take(tracker,observation(110,220,5,101)).value); // reused PID
    assert(take(tracker,observation(210,320,6,101)).value==200);
    assert(!take(tracker,observation(200,1000,7,101)).value); // read reset hidden by increasing total
    assert(!take(tracker,observation(2000,900,8,101)).value); // write reset
    assert(take(tracker,observation(2001,901,9,101)).value==2);
    auto missing=observation(0,0,10,101);missing.counters=denied;assert(take(tracker,missing).support==Support::permission_required);
    assert(!take(tracker,observation(2001,901,11,101)).value);
    assert(take(tracker,observation(2001,901,12,101)).value==0);
    tracker.sample({});assert(!take(tracker,observation(2001,901,13,101)).value);
    tracker.reset();assert(!take(tracker,observation(0,0,2)).value);
    assert(!take(tracker,observation(10,10,2)).value); // equal time
    assert(!take(tracker,observation(20,20,1)).value); // backwards time
    assert(take(tracker,observation(30,30,3)).value==10);
    tracker.reset();auto edge=observation(0,0,0);edge.at=Clock::time_point::min();assert(!take(tracker,edge).value);edge.at=Clock::time_point::max();edge.counters.value=LinuxProcessIoCounters{100,100};assert(take(tracker,edge).value); // no signed duration overflow
    tracker.reset();assert(!take(tracker,observation(max-10,max-10,1)).value);assert(take(tracker,observation(max,max,3)).value==10); // absolute sum need not fit
    tracker.reset();take(tracker,observation(0,0,1));assert(!take(tracker,observation(max,max,2)).value); // rate outside uint64
    tracker.reset();take(tracker,observation(0,0,1));auto fast=observation(max,0,1);fast.at+=std::chrono::milliseconds(1);assert(!take(tracker,fast).value);
    for(int mode=0;mode<3;++mode){tracker.reset();take(tracker,observation(0,0,1));std::vector<LinuxProcessIoObservation> rows{observation(1,1,2)};if(mode==0)rows.push_back(rows.front());if(mode==1)rows.front().pid=0;if(mode==2)rows.resize(100001,rows.front());bool threw=false;try{tracker.sample(rows);}catch(const std::invalid_argument&){threw=true;}assert(threw);assert(!take(tracker,observation(2,2,3)).value);}
#ifdef __linux__
    const auto root=fs::temp_directory_path()/("pcinsight-io-"+std::to_string(Clock::now().time_since_epoch().count()));assert(fs::create_directory(root));
    struct Cleanup{fs::path p;~Cleanup(){std::error_code ec;fs::remove_all(p,ec);}} cleanup{root};
    assert(!read_linux_process_io_counters(root,*expected).value);write(root/"stat",stat_text());write(root/"io","read_bytes:0\nwrite_bytes:0\n");assert(read_linux_process_io_counters(root,*expected).value);
    write(root/"io",std::string(65537,'x'));assert(!read_linux_process_io_counters(root,*expected).value);
    write(root/"io","read_bytes:1\nwrite_bytes:2\n");write(root/"stat",stat_text(123,101));assert(!read_linux_process_io_counters(root,*expected).value);
    write(root/"stat",stat_text(124));assert(!read_linux_process_io_counters(root,*expected).value);
    write(root/"stat",std::string(65537,'x'));assert(!read_linux_process_io_counters(root,*expected).value);
    write(root/"stat",stat_text());assert(fs::remove(root/"io"));assert(!read_linux_process_io_counters(root,*expected).value);
    write(root/"other","read_bytes:1\nwrite_bytes:2");fs::create_symlink(root/"other",root/"io");assert(!read_linux_process_io_counters(root,*expected).value);
    std::ifstream statfile("/proc/self/stat");std::string text;std::getline(statfile,text,'\0');const auto own=parse_linux_process_stat(text);assert(own&&own->pid==static_cast<std::uint32_t>(::getpid()));
    const auto initial=read_linux_process_io_counters("/proc/self",*own);assert(initial.value);auto reused=*own;++reused.start_ticks;assert(!read_linux_process_io_counters("/proc/self",reused).value);
    tracker.reset();LinuxProcessIoObservation first{own->pid,own->start_ticks,initial,Clock::now()};assert(!take(tracker,first).value);
    const int fd=::open((root/"private-data").c_str(),O_CREAT|O_EXCL|O_WRONLY|O_CLOEXEC,0600);assert(fd>=0);const std::string block(131072,'x');std::size_t offset=0;while(offset<block.size()){const auto bytes=::write(fd,block.data()+offset,block.size()-offset);assert(bytes>0);offset+=static_cast<std::size_t>(bytes);}assert(::fsync(fd)==0);assert(::close(fd)==0);
    LinuxProcessIoObservation last{own->pid,own->start_ticks,read_linux_process_io_counters("/proc/self",*own),Clock::now()};assert(last.counters.value);const auto native_rate=take(tracker,last);assert(native_rate.value&&native_rate.support==Support::estimated);
    SampleOptions options;options.include_app_history=false;options.include_disks=false;options.include_networks=false;options.include_gpus=false;options.include_user_sessions=false;options.include_services=false;options.include_startup_entries=false;
    Monitor monitor;auto sample=monitor.sample(options);auto find_own=[&](const SystemSnapshot& s)->const ProcessInfo&{const auto it=std::find_if(s.processes.begin(),s.processes.end(),[&](const auto& p){return p.pid==own->pid;});assert(it!=s.processes.end());assert(it->linux_start_ticks.value==own->start_ticks);return *it;};assert(!find_own(sample).disk_bytes_per_second.value);
    sample=monitor.sample(options);assert(find_own(sample).disk_bytes_per_second.value&&find_own(sample).disk_bytes_per_second.support==Support::estimated);
    options.include_processes=false;monitor.sample(options);options.include_processes=true;sample=monitor.sample(options);assert(!find_own(sample).disk_bytes_per_second.value);sample=monitor.sample(options);assert(find_own(sample).disk_bytes_per_second.value);
    std::cout<<"Native PID="<<own->pid<<" start_ticks="<<own->start_ticks<<" read delta="<<last.counters.value->read_bytes-initial.value->read_bytes<<" write delta="<<last.counters.value->write_bytes-initial.value->write_bytes<<" bytes, estimated rate="<<*native_rate.value<<" B/s; private-file write/fsync only (zero is valid on non-storage-backed filesystems)\n";
    std::cout<<"Native bounded pinned-directory reads, missing/malformed/oversized/symlink/reused identities and Monitor first/second/opt-out sampling passed\n";
#else
    assert(!read_linux_process_io_counters("unused",*expected).value);
#endif
    std::cout<<"Process I/O: strict separate uint64 counters, identity, own clock, independent resets, missing/permission gaps, overflow and batch validation passed\n";
}
