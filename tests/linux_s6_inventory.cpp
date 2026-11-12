#include <pcinsight/linux_s6.hpp>
#include <cassert>
#include <iostream>
#ifdef __linux__
#include <filesystem>
#include <fstream>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <cstdlib>
#endif
using namespace pcinsight;
#ifdef __linux__
namespace fs=std::filesystem;
std::size_t fds(){auto d=::opendir("/proc/self/fd");assert(d);std::size_t n=0;while(auto e=::readdir(d))if(e->d_name[0]!='.')++n;::closedir(d);return n;}
void record(const fs::path& dir){fs::create_directories(dir/"supervise");std::array<unsigned char,35> b{};b[31]=42;b[34]=4;std::ofstream f(dir/"supervise/status",std::ios::binary);f.write(reinterpret_cast<const char*>(b.data()),b.size());f.close();assert(f);}
#endif
int main(){
    assert(!query_linux_s6_inventory("relative").value);assert(!query_linux_s6_inventory("/",0).value);assert(!query_linux_s6_inventory("/",4097).value);assert(!query_linux_s6_inventory(std::string("/tmp\0x",6)).value);
#ifdef __linux__
    ::alarm(20);const auto initial=fds();char pattern[]="/tmp/pcinsight-s6-inventory-XXXXXX";auto d=::mkdtemp(pattern);assert(d);const fs::path root=d,scan=root/"scan",services=root/"services";struct Cleanup{fs::path p;~Cleanup(){std::error_code e;fs::remove_all(p,e);}} cleanup{root};assert(!query_linux_s6_inventory(scan.string()).value&&!fs::exists(scan));fs::create_directory(scan);auto empty=query_linux_s6_inventory(scan.string());assert(empty.value&&empty.value->complete&&empty.value->entries.empty()&&fs::is_empty(scan));
    record(services/"example");record(services/"example/log");record(services/"example/log/log");fs::create_directory_symlink(services/"example",scan/"alpha");fs::create_directory(scan/"unmanaged");fs::create_directory(scan/".hidden");{std::ofstream f(scan/"README");f<<"not a service";}assert(::mkfifo((scan/"plain-fifo").c_str(),0600)==0);fs::create_directory(scan/".s6-svscan");assert(::mkfifo((services/"example/supervise/control").c_str(),0600)==0);
    const int reader=::open((services/"example/supervise/control").c_str(),O_RDONLY|O_NONBLOCK|O_CLOEXEC);assert(reader>=0);
    auto read=[&]{return query_linux_s6_inventory(scan.string());};auto all=read();assert(all.value&&all.value->complete&&all.value->entries.size()==3);const auto& rows=all.value->entries;assert(rows[0].name=="alpha"&&rows[0].status_record.value&&rows[0].status_record.value->reported_pid==42&&rows[0].real_control_fifo.value==true);assert(rows[1].name=="alpha/log"&&rows[1].logger&&rows[1].parent_name=="alpha"&&rows[1].status_record.value&&rows[1].real_control_fifo.value==false);assert(rows[2].name=="unmanaged"&&!rows[2].status_record.value&&rows[2].real_control_fifo.value==false);
    char byte{};assert(::read(reader,&byte,1)<=0);auto bounded=query_linux_s6_inventory(scan.string(),1);assert(bounded.value&&!bounded.value->complete&&bounded.support==Support::estimated&&bounded.value->entries.size()==1&&!bounded.value->diagnostics.empty());
    fs::create_symlink("absent",scan/"broken");all=read();assert(all.value&&!all.value->complete&&all.value->entries.size()==4&&!all.value->entries[2].status_record.value&&!all.value->entries[2].real_control_fifo.value);fs::remove(scan/"broken");
    fs::create_directory_symlink(scan,root/"scan-alias");assert(query_linux_s6_inventory((root/"scan-alias").string()).value->entries.size()==3);
    fs::remove(services/"example/log/supervise/status");assert(::mkfifo((services/"example/log/supervise/status").c_str(),0600)==0);all=read();assert(all.value&&all.value->complete&&!all.value->entries[1].status_record.value);fs::remove(services/"example/log/supervise/status");
    fs::create_symlink("missing",services/"example/log/supervise/control");all=read();assert(all.value&&all.value->entries[1].real_control_fifo.value==false);
    fs::rename(services/"example/log",services/"example/saved-log");fs::create_symlink("missing",services/"example/log");all=read();assert(all.value&&!all.value->complete&&all.value->entries.size()==3&&all.value->entries[1].logger&&!all.value->entries[1].status_record.value&&!all.value->entries[1].real_control_fifo.value);fs::remove(services/"example/log");fs::rename(services/"example/saved-log",services/"example/log");
    for(unsigned i=0;i<10;++i)assert(read().value);::close(reader);assert(fds()==initial);assert(!query_linux_s6_inventory("/proc/self/fd/0").value);::alarm(0);
#else
    assert(!query_linux_s6_inventory("/service").value);
#endif
    std::cout<<"s6 inventory explicit tree/aliases/direct-logger/no-recursion/unknown/control-type/bounds/no-write/no-create/FD checks passed; NOT real s6 acceptance or liveness\n";
}
