#include <pcinsight/linux_s6.hpp>
#include <cassert>
#include <iostream>
#include <limits>
#ifdef __linux__
#include <filesystem>
#include <fstream>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <cstdlib>
#endif
using namespace pcinsight;
std::array<std::uint8_t,35> frame(){std::array<std::uint8_t,35> b{};b[0]=0x40;b[7]=19;b[12]=0x40;b[19]=20;b[30]=0x12;b[31]=0x34;b[32]=0xab;b[33]=0xcd;b[34]=15;return b;}
#ifdef __linux__
namespace fs=std::filesystem;
std::size_t fds(){auto d=::opendir("/proc/self/fd");assert(d);std::size_t n=0;while(auto e=::readdir(d))if(e->d_name[0]!='.')++n;::closedir(d);return n;}
void write(const fs::path& p,const std::array<std::uint8_t,35>& b){std::ofstream f(p,std::ios::binary);assert(f);f.write(reinterpret_cast<const char*>(b.data()),b.size());f.close();assert(f);}
#endif
int main(){
    auto b=frame();auto decoded=decode_linux_s6_status_record(b);assert(decoded.value&&decoded.value->reported_pid==0x1234&&decoded.value->reported_wait_status==0xabcd&&decoded.value->paused&&decoded.value->finishing&&decoded.value->want_up&&decoded.value->ready_reported&&decoded.value->changed_at_tai64n[7]==19&&decoded.value->ready_changed_at_tai64n[7]==20);
    for(unsigned n=0;n<35;++n)assert(!decode_linux_s6_status_record(std::span(b).first(n)).value);std::array<std::uint8_t,36> large{};assert(!decode_linux_s6_status_record(large).value);
    for(unsigned flag=0;flag<16;++flag){b[34]=static_cast<std::uint8_t>(flag);auto d=decode_linux_s6_status_record(b);assert(d.value&&d.value->paused==bool(flag&1)&&d.value->finishing==bool(flag&2)&&d.value->want_up==bool(flag&4)&&d.value->ready_reported==bool(flag&8));}
    for(unsigned flag=16;flag<256;++flag){b[34]=static_cast<std::uint8_t>(flag);assert(!decode_linux_s6_status_record(b).value);}
    for(auto offset:{8,20}){b=frame();b[offset]=0x3b;b[offset+1]=0x9a;b[offset+2]=0xca;assert(!decode_linux_s6_status_record(b).value);b[offset+2]=0xc9;b[offset+3]=0xff;assert(decode_linux_s6_status_record(b).value);}
    b={};b[34]=8;decoded=decode_linux_s6_status_record(b);assert(decoded.value&&decoded.value->reported_pid==0&&decoded.value->ready_reported); // down can report ready!
    for(unsigned i=24;i<32;++i)b[i]=255;assert(decode_linux_s6_status_record(b).value->reported_pid==std::numeric_limits<std::uint64_t>::max()); // no local PID cast
    assert(!read_linux_s6_status_record("relative").value);assert(!read_linux_s6_status_record(std::string("/tmp\0x",6)).value);
#ifdef __linux__
    ::alarm(20);const auto initial=fds();char pattern[]="/tmp/pcinsight-s6-XXXXXX";auto d=::mkdtemp(pattern);assert(d);const fs::path root=d,service=root/"service",supervise=service/"supervise",file=supervise/"status";struct Cleanup{fs::path p;~Cleanup(){std::error_code e;fs::remove_all(p,e);}} cleanup{root};
    assert(!read_linux_s6_status_record(service.string()).value&&!fs::exists(service));fs::create_directory(service);assert(!read_linux_s6_status_record(service.string()).value&&!fs::exists(supervise));fs::create_directory(supervise);assert(!read_linux_s6_status_record(service.string()).value);
    b=frame();write(file,b);auto native=read_linux_s6_status_record(service.string());assert(native.value&&*native.value==*decode_linux_s6_status_record(b).value); // No supervisor or control FIFO exists: RECORD, not alive proof.
    fs::create_directory_symlink(service,root/"alias");assert(read_linux_s6_status_record((root/"alias").string()).value==native.value);
    fs::rename(file,supervise/"saved");fs::create_symlink("saved",file);assert(!read_linux_s6_status_record(service.string()).value);fs::remove(file);assert(::mkfifo(file.c_str(),0600)==0);assert(!read_linux_s6_status_record(service.string()).value);fs::remove(file);fs::create_directory(file);assert(!read_linux_s6_status_record(service.string()).value);fs::remove(file);
    {std::ofstream f(file,std::ios::binary);f<<"20bytes-not-s6-format";}assert(!read_linux_s6_status_record(service.string()).value);fs::remove(file);
    b=frame();b[34]=128;write(file,b);assert(!read_linux_s6_status_record(service.string()).value);b=frame();write(file,b);
    if(::geteuid()!=0){assert(::chmod(file.c_str(),0000)==0);auto denied=read_linux_s6_status_record(service.string());assert(!denied.value&&denied.support==Support::permission_required);assert(::chmod(file.c_str(),0600)==0);}
    for(unsigned i=0;i<10;++i)assert(read_linux_s6_status_record(service.string()).value);
    assert(!read_linux_s6_status_record("/proc/self/fd/0").value);assert(fds()==initial);::alarm(0);
#else
    assert(!read_linux_s6_status_record("/service/example").value);
#endif
    std::cout<<"s6 raw35/endian/TAI/flags/zero-ready/u64/no-create/typeguard/no-follow/FD tests passed; NOT supervisor/process liveness or real s6 acceptance\n";
}
