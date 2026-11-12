#include "../src/linux_uptime.hpp"
#include "pcinsight/monitor.hpp"
#include <cassert>
#include <chrono>
#include <fstream>
#include <iostream>
#include <limits>
#ifdef __linux__
#include <time.h>
#endif
using namespace pcinsight;
using namespace pcinsight::detail;
namespace fs=std::filesystem;
using Seconds=std::chrono::seconds;
void write(const fs::path& path,const std::string& text){std::ofstream out(path,std::ios::binary);assert(out);out<<text;out.close();assert(out);}
int main(){
    for(const auto& text:{"0.00 0.00\n","0.99 100.00"," \t0.01\t999.99\r\n"}){
        const auto metric=parse_linux_uptime(text);assert(metric.value==Seconds(0)&&metric.support==Support::available&&!metric.note.empty());
    }
    assert(parse_linux_uptime("12345.99 999999.01\n").value==Seconds(12345)); // floor, not nearest second; idle may exceed uptime
    assert(parse_linux_uptime("00012.50 00000.00").value==Seconds(12));
    const auto max=static_cast<std::uintmax_t>(std::numeric_limits<Seconds::rep>::max());
    assert(parse_linux_uptime(std::to_string(max)+".99 18446744073709551615.99").value==Seconds::max()); // no multiplication or float rounding at the boundary
    assert(!parse_linux_uptime(std::to_string(max+1)+".00 0.00").value);
    for(const auto& text:std::vector<std::string>{""," \n","1.00","1.00 ","1.00 2.00 extra","1.00 2.00\n3.00 4.00","-1.00 0.00","-0.00 0.00","+1.00 0.00","1 0.00","1.0 0.00","1.000 0.00",".00 0.00","1. 0.00","nan 0.00","inf 0.00","1e2 0.00","1,00 0.00","1.0x 0.00","1.00x 0.00","18446744073709551616.00 0.00","1.00 18446744073709551616.00",std::string("1.00 2.00\0",10),std::string(257,' '),std::string(62,'0')+".00 0.00"}){
        const auto bad=parse_linux_uptime(text);assert(!bad.value&&bad.support==Support::unsupported&&!bad.note.empty());
    }
    // Both fields are validated, not just a numeric prefix in the first one.
    for(const auto& idle:{"-1.00","+0.00","NaN","1.0","1.00oops","1.2.3",".01"})assert(!parse_linux_uptime(std::string("10.00 ")+idle).value);
    const std::string record="123.45 678.90";
    for(std::size_t i=0;i<record.size();++i)assert(!parse_linux_uptime(std::string_view(record).substr(0,i)).value);
    const auto exact=std::string(247,' ')+"0.00 0.00";assert(exact.size()==256&&parse_linux_uptime(exact).value==Seconds(0));
#ifdef __linux__
    const auto root=fs::temp_directory_path()/("pcinsight-uptime-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    assert(fs::create_directory(root));struct Cleanup{fs::path p;~Cleanup(){std::error_code ec;fs::remove_all(p,ec);}} cleanup{root};
    const auto path=root/"uptime";
    CpuInfo cpu;cpu.uptime={Seconds(999),Support::available,"old"};fill_linux_uptime(cpu,path);assert(!cpu.uptime.value);
    write(path,"1.99 999.99\n");fill_linux_uptime(cpu,path);assert(cpu.uptime.value==Seconds(1));
    write(path,"0.00 0.00\n");fill_linux_uptime(cpu,path);assert(cpu.uptime.value==Seconds(0));
    for(const auto& bad:std::vector<std::string>{"","broken","1.00 bad","1.00 2.00 extra",std::string(257,' '),std::to_string(max+1)+".99 0.00"}){
        write(path,"10.00 20.00");fill_linux_uptime(cpu,path);assert(cpu.uptime.value==Seconds(10));
        write(path,bad);fill_linux_uptime(cpu,path);assert(!cpu.uptime.value);
    }
    write(path,exact);assert(read_linux_uptime(path).value==Seconds(0));
    assert(!read_linux_uptime(root).value); // directory is not a readable uptime record
    assert(fs::remove(path));fill_linux_uptime(cpu,path);assert(!cpu.uptime.value);
    SampleOptions options;options.include_processes=false;options.include_app_history=false;options.include_disks=false;options.include_networks=false;options.include_gpus=false;options.include_user_sessions=false;options.include_services=false;options.include_startup_entries=false;
#ifdef CLOCK_BOOTTIME
    timespec clock_before{},clock_after{};assert(clock_gettime(CLOCK_BOOTTIME,&clock_before)==0);
#endif
    const auto before=read_linux_uptime();assert(before.value);
    Monitor monitor;const auto first=monitor.sample(options),second=monitor.sample(options);
    const auto after=read_linux_uptime();assert(after.value);
#ifdef CLOCK_BOOTTIME
    assert(clock_gettime(CLOCK_BOOTTIME,&clock_after)==0);
    assert(before.value->count()>=clock_before.tv_sec&&after.value->count()<=clock_after.tv_sec);
    std::cout<<"Native CLOCK_BOOTTIME bracket: "<<clock_before.tv_sec<<".."<<clock_after.tv_sec<<" whole seconds\n";
#endif
    assert(first.cpu.uptime.value&&second.cpu.uptime.value&&first.cpu.uptime.support==Support::available);
    assert(*before.value<=*first.cpu.uptime.value&&*first.cpu.uptime.value<=*second.cpu.uptime.value&&*second.cpu.uptime.value<=*after.value);
    std::cout<<"Native Linux uptime: "<<first.cpu.uptime.value->count()<<" seconds; proc/Monitor sequence nondecreasing and inside clock bracket when available\n";
    std::cout<<"Linux file fixtures: missing/directory/empty/malformed/over-limit source, valid zero, exact bounds and stale clearing passed\n";
#else
    assert(!read_linux_uptime().value);
#endif
    std::cout<<"Uptime: strict two decimal fields, idle above uptime, zero, fractional truncation, signed-rep boundary, uint64 overflow, truncations and text limits passed\n";
}
