#include "../src/linux_process_memory.hpp"
#include "pcinsight/monitor.hpp"
#include <algorithm>
#include <cassert>
#include <chrono>
#include <fstream>
#include <iostream>
#include <limits>
#ifdef __linux__
#include <unistd.h>
#endif
using namespace pcinsight;
using namespace pcinsight::detail;
namespace fs=std::filesystem;
std::string stat_text(std::uint32_t pid=123,std::uint64_t start=100){return std::to_string(pid)+" (memory fixture) S 12 0 0 0 -1 0 0 0 0 0 0 0 0 0 20 0 3 0 "+std::to_string(start)+" 12345 0 0";}
void write(const fs::path& path,const std::string& text){std::ofstream out(path,std::ios::binary);assert(out);out<<text;out.close();assert(out);}
int main(){
    const auto zero=parse_linux_process_rss("Name: fixture\nVmRSS: 0 kB\n");assert(zero.value==0&&zero.support==Support::estimated);
    const auto value=parse_linux_process_rss("Pid:\t123\nVmRSS:\t2512 kB\nOther: unknown fields ignored\n",123);assert(value.value==2572288&&value.support==Support::estimated);
    assert(parse_linux_process_rss("VmRSS: 1 kB").value==1024);
    assert(parse_linux_process_rss("Pid:123\nVmRSS:1 kB",123).value==1024);
    assert(!parse_linux_process_rss("VmRSS: 1 kB\nVmRSS:1 kB").value);
    assert(!parse_linux_process_rss("Name: kernel_task\nPid: 123\n",123).value); // absent not measured zero
    const auto max=std::numeric_limits<std::uint64_t>::max()/1024;
    assert(parse_linux_process_rss("VmRSS: "+std::to_string(max)+" kB\n").value==max*1024);
    for(const auto& bad:std::vector<std::string>{"","VmRSS: -1 kB\n","VmRSS: +1 kB\n","VmRSS: 1.5 kB\n","VmRSS: 1e3 kB\n","VmRSS: 1 B\n","VmRSS: 1 KB\n","VmRSS: 1\n","VmRSS: 1 kB extra\n","VmRSS: 1 kB x y z\n","VmRSS: 1bad kB\n","VmRSS: 18446744073709551616 kB\n","VmRSS: "+std::to_string(max+1)+" kB\n","VmRSS: 0 kB\nVmRSS: 0 kB\n",std::string("VmRSS: 1 kB\0",12),"Other: "+std::string(65536,'x'),std::string(1024*1024+1,'x')})assert(!parse_linux_process_rss(bad).value);
    for(const auto& bad:std::vector<std::string>{"Pid: -1\nVmRSS: 1 kB\n","Pid: 0\nVmRSS: 1 kB\n","Pid: 4294967296\nVmRSS: 1 kB\n","Pid: 123 extra\nVmRSS: 1 kB\n","Pid: 123\nPid: 123\nVmRSS: 1 kB\n"})assert(!parse_linux_process_rss(bad).value);
    assert(!parse_linux_process_rss("VmRSS: 1 kB",123).value);
    assert(!parse_linux_process_rss("Pid: 124\nVmRSS: 1 kB",123).value);
    assert(!parse_linux_process_rss("Pid: 123\nVmRSS: 1 kB",0).value);
    std::string many;for(unsigned n=0;n<10001;++n)many+="Other: 1\n";assert(!parse_linux_process_rss(many).value);
    const auto expected=parse_linux_process_stat(stat_text());assert(expected&&expected->pid==123&&expected->start_ticks==100);
    assert(linux_process_rss_for_identity(*expected,expected,expected,value).value==value.value);
    for(int mode=0;mode<5;++mode){
        auto before=expected,after=expected;
        if(mode==0)before.reset();
        if(mode==1)after.reset();
        if(mode==2)after->start_ticks=101;
        if(mode==3)before->pid=124;
        if(mode==4)before->start_ticks=101;
        assert(!linux_process_rss_for_identity(*expected,before,after,value).value);
    }
    auto boot=*expected;boot.start_ticks=0;assert(linux_process_rss_for_identity(boot,boot,boot,zero).value==0); // start tick0 is a legitimate token value
    auto invalid=*expected;invalid.pid=0;assert(!linux_process_rss_for_identity(invalid,invalid,invalid,value).value);
    const Metric<std::uint64_t> denied{{},Support::permission_required,"denied fixture"};assert(linux_process_rss_for_identity(*expected,expected,expected,denied).support==Support::permission_required);
#ifdef __linux__
    const auto root=fs::temp_directory_path()/("pcinsight-rss-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));assert(fs::create_directory(root));
    struct Cleanup{fs::path p;~Cleanup(){std::error_code ec;fs::remove_all(p,ec);}} cleanup{root};
    assert(!read_linux_process_memory(root,*expected).value);
    write(root/"stat",stat_text());write(root/"status","Pid: 123\nVmRSS: 2512 kB\n");assert(read_linux_process_memory(root,*expected).value==2572288);
    write(root/"status","Pid: 123\nVmRSS: 0 kB\n");assert(read_linux_process_memory(root,*expected).value==0);
    for(const auto& bad:std::vector<std::string>{"Pid: 123\n","Pid: 124\nVmRSS: 10 kB\n","Pid: 123\nVmRSS: 18446744073709551615 kB\n",std::string(1024*1024+1,'x')}){write(root/"status",bad);assert(!read_linux_process_memory(root,*expected).value);}
    write(root/"status","Pid: 123\nVmRSS: 10 kB\n");
    write(root/"stat",stat_text(123,101));assert(!read_linux_process_memory(root,*expected).value);
    write(root/"stat",stat_text(124,100));assert(!read_linux_process_memory(root,*expected).value);
    write(root/"stat",std::string(65537,'x'));assert(!read_linux_process_memory(root,*expected).value);
    write(root/"stat",stat_text());assert(fs::remove(root/"status"));assert(!read_linux_process_memory(root,*expected).value);
    write(root/"other","Pid: 123\nVmRSS: 1 kB\n");fs::create_symlink(root/"other",root/"status");assert(!read_linux_process_memory(root,*expected).value); // don't follow final source symlinks
    std::ifstream file("/proc/self/stat");std::string text;std::getline(file,text,'\0');const auto own=parse_linux_process_stat(text);assert(own&&own->pid==static_cast<std::uint32_t>(::getpid()));
    const auto native=read_linux_process_memory("/proc/self",*own);assert(native.value&&*native.value>0&&native.support==Support::estimated);
    auto reused=*own;++reused.start_ticks;assert(!read_linux_process_memory("/proc/self",reused).value);
    // Reader and Monitor affect their own RSS; do not assert exact equality of
    // independently sampled asynchronous counters.
    SampleOptions options;options.include_app_history=false;options.include_disks=false;options.include_networks=false;options.include_gpus=false;options.include_user_sessions=false;options.include_services=false;options.include_startup_entries=false;
    Monitor monitor;const auto sample=monitor.sample(options);
    const auto row=std::find_if(sample.processes.begin(),sample.processes.end(),[&](const auto& p){return p.pid==own->pid;});assert(row!=sample.processes.end());
    assert(row->linux_start_ticks.value==own->start_ticks&&row->memory_bytes.value&&*row->memory_bytes.value>0&&row->memory_bytes.support==Support::estimated);
    for(const auto& p:sample.processes)if(p.memory_bytes.value)assert(p.memory_bytes.support==Support::estimated&&*p.memory_bytes.value%1024==0);
    std::cout<<"Native own PID="<<own->pid<<" start_ticks="<<own->start_ticks<<" RSS="<<*native.value<<" bytes; Monitor RSS="<<*row->memory_bytes.value<<" bytes estimated; wrong identity rejected\n";
    std::cout<<"Linux files: pinned-directory bounded stat/status reads, missing/malformed/oversized sources, PID mismatch/reuse and no final symlink passed\n";
#else
    assert(!read_linux_process_memory("unused",*expected).value);
#endif
    std::cout<<"Process RSS: strict kB/uint64, absence vs zero, selected-field duplicates, PID/start identity guard and overflow/bounds passed\n";
}
