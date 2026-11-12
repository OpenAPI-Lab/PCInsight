#include <pcinsight/linux_s6_services.hpp>
#include "../src/linux_s6_services.hpp"
#include <cassert>
#include <iostream>
#include <limits>
#ifdef __linux__
#include <filesystem>
#include <fstream>
#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cstdlib>
namespace fs=std::filesystem;
std::size_t fds(){auto d=::opendir("/proc/self/fd");assert(d);std::size_t n=0;while(auto e=::readdir(d))if(e->d_name[0]!='.')++n;::closedir(d);return n;}
void record(const fs::path& dir,const std::array<std::uint8_t,35>& bytes){fs::create_directories(dir/"supervise");std::ofstream f(dir/"supervise/status",std::ios::binary);f.write(reinterpret_cast<const char*>(bytes.data()),bytes.size());f.close();assert(f.good());}
#endif
using namespace pcinsight;
std::array<std::uint8_t,35> frame(){std::array<std::uint8_t,35> b{};b[0]=0x40;b[12]=0x40;b[31]=42;b[32]=0x12;b[33]=0x34;b[34]=12;return b;}
Metric<LinuxS6Inventory> fixture(){LinuxS6Inventory source;source.root="/private/s6";LinuxS6InventoryEntry e;e.name="alpha";e.directory=source.root+"/alpha";e.status_record=decode_linux_s6_status_record(frame());e.real_control_fifo={true,Support::available,"type only"};source.entries.push_back(e);e.name="alpha/log";e.directory=source.root+"/alpha/log";e.logger=true;e.parent_name="alpha";source.entries.push_back(e);return {source,Support::available,"fixture"};}
void unavailable(const ServiceInfo& row){assert(!row.pid&&row.description.empty()&&row.group.empty()&&row.load_state.empty()&&row.active_state.empty()&&row.sub_state.empty()&&row.unit_file_state.empty());assert(row.linux_details.value&&row.linux_details.support==Support::estimated);const auto& d=*row.linux_details.value;assert(d.provider=="s6-status-record-inventory"&&d.group_kind.empty()&&!d.main_pid.value&&!d.description.value&&!d.slice.value&&!d.load_state.value&&!d.active_state.value&&!d.sub_state.value&&!d.unit_file_state.value);}
int main(){
    auto input=fixture();auto out=detail::project_s6_services(input);assert(out.value&&out.value->services.size()==2&&out.value->services[0].id=="s6-record:616c706861"&&out.value->services[1].id=="s6-record:616c7068612f6c6f67");assert(out.value->services[0].status=="record-pid-present"&&out.value->services[0].linux_details.value->status.support==Support::estimated&&out.value->source.entries[1].logger&&out.value->source.entries[1].parent_name=="alpha");assert(out.value->source.entries[0].status_record.value==input.value->entries[0].status_record.value);for(const auto& row:out.value->services)unavailable(row);
    input=fixture();auto& raw=*input.value->entries[0].status_record.value;raw.reported_pid=std::numeric_limits<std::uint64_t>::max();raw.paused=true;raw.finishing=true;out=detail::project_s6_services(input);assert(out.value&&out.value->services[0].status=="record-pid-present/finishing/paused"&&out.value->source.entries[0].status_record.value->reported_pid==std::numeric_limits<std::uint64_t>::max());unavailable(out.value->services[0]);
    input=fixture();input.value->entries[0].status_record.value->reported_pid=0;out=detail::project_s6_services(input);assert(out.value&&out.value->services[0].status=="record-pid-zero"&&out.value->source.entries[0].status_record.value->ready_reported); // ready+PID0 is valid raw data, not a contradiction.
    for(unsigned which=0;which<2;++which){input=fixture();auto& s=*input.value->entries[0].status_record.value;(which?s.ready_changed_at_tai64n:s.changed_at_tai64n)[8]=255;out=detail::project_s6_services(input);assert(out.value&&out.value->services[0].status=="unknown"&&!out.value->services[0].linux_details.value->status.value);}
    input=fixture();input.value->entries[0].status_record.support=Support::permission_required;out=detail::project_s6_services(input);assert(out.value&&out.value->services[0].status=="unknown"&&out.value->services[0].linux_details.value->status.support==Support::permission_required);
    input=fixture();input.value->complete=false;out=detail::project_s6_services(input);assert(out.value&&out.support==Support::estimated&&out.value->services[0].status=="unknown");
    input=fixture();input.support=Support::estimated;out=detail::project_s6_services(input);assert(out.value&&out.value->services[0].status=="unknown");
    input=fixture();input.value->entries[0].real_control_fifo={false,Support::available,"absent"};out=detail::project_s6_services(input);assert(out.value&&out.value->services[0].status=="record-pid-present"); // Not a control/life inference.
    input=fixture();input.value->entries.push_back(input.value->entries[0]);assert(!detail::project_s6_services(input).value);
    input=fixture();input.value->entries.erase(input.value->entries.begin());assert(!detail::project_s6_services(input).value);
    input=fixture();input.value->entries[1].name="alpha/log/log";assert(!detail::project_s6_services(input).value);
    input=fixture();input.value->entries[0].directory="/other/alpha";assert(!detail::project_s6_services(input).value);
    input=fixture();input.value->root+='/';assert(detail::project_s6_services(input).value);
    input=fixture();input.value->entries.resize(1);input.value->entries[0].name=std::string("x:\xff",3);input.value->entries[0].directory=input.value->root+"/"+input.value->entries[0].name;out=detail::project_s6_services(input);assert(out.value&&out.value->services[0].id=="s6-record:783aff");
    input=fixture();input.value->entries.clear();assert(detail::project_s6_services(input).value->services.empty());assert(!detail::project_s6_services({{},Support::unsupported,"no data"}).value);
    assert(!query_linux_s6_service_inventory("relative").value&&!query_linux_s6_service_inventory("/unused",0).value&&!query_linux_s6_service_inventory("/unused",4097).value);
#ifdef __linux__
    ::alarm(20);const auto initial=fds();char pattern[]="/tmp/pcinsight-s6-services-XXXXXX";auto p=::mkdtemp(pattern);assert(p);const fs::path root=p,scan=root/"scan",service=root/"service",sentinel=root/"must-not-exist";struct Cleanup{fs::path path;~Cleanup(){std::error_code e;fs::remove_all(path,e);}} cleanup{root};fs::create_directory(scan);record(service,frame());record(service/"log",frame());record(service/"log/log",frame());fs::create_directory_symlink(service,scan/"alpha");fs::create_directory(scan/"unmanaged");{std::ofstream f(service/"run");f<<"touch '"<<sentinel.string()<<"'\n";}assert(::mkfifo((service/"supervise/control").c_str(),0600)==0);const int control=::open((service/"supervise/control").c_str(),O_RDONLY|O_NONBLOCK|O_CLOEXEC);assert(control>=0);
    auto read=[&]{return query_linux_s6_service_inventory(scan.string());};out=read();assert(out.value&&out.value->source.complete&&out.value->services.size()==3&&out.value->services[0].status=="record-pid-present"&&out.value->services[1].name=="alpha/log"&&out.value->services[2].status=="unknown"&&out.value->source.entries[0].real_control_fifo.value==true);for(const auto& row:out.value->services)unavailable(row);
    auto partial=query_linux_s6_service_inventory(scan.string(),1);assert(partial.value&&!partial.value->source.complete&&partial.value->services[0].status=="unknown");char bytes[8];assert(::read(control,bytes,sizeof bytes)<=0);::close(control);out=read();assert(out.value&&out.value->services[0].status=="record-pid-present"); // No FIFO reader, no supervisor: file record remains historical.
    fs::rename(service/"log",service/"saved-log");fs::create_symlink("missing",service/"log");partial=read();assert(partial.value&&!partial.value->source.complete&&partial.value->services[0].status=="unknown");fs::remove(service/"log");fs::rename(service/"saved-log",service/"log");for(unsigned i=0;i<10;++i)assert(read().value->services.size()==3);assert(!fs::exists(sentinel)&&fds()==initial);::alarm(0);
#else
    assert(!query_linux_s6_service_inventory("/unused").value);
#endif
    std::cout<<"s6 ServiceInfo historical records/logger/unknown host PID/partial/IDs/no-control/noexec/FD checks passed; not real supervisor acceptance\n";
}
