#include <pcinsight/linux_runit_services.hpp>
#include "../src/linux_runit_services.hpp"
#include <cassert>
#include <iostream>
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
#endif
using namespace pcinsight;
std::array<std::uint8_t,20> frame(){std::array<std::uint8_t,20> b{};b[0]=0x40;b[7]=20;b[12]=123;b[17]='u';b[19]=1;return b;}
Metric<LinuxRunitInventory> fixture(){LinuxRunitInventory source;source.root="/private/runit";LinuxRunitServiceInfo row;row.name="alpha";row.directory=source.root+"/alpha";row.status=decode_linux_runit_status(frame());source.services.push_back(row);return {source,Support::available,"fixture"};}
void unavailable(const ServiceInfo& row){assert(!row.pid&&row.description.empty()&&row.group.empty()&&row.load_state.empty()&&row.active_state.empty()&&row.sub_state.empty()&&row.unit_file_state.empty());assert(row.linux_details.value&&row.linux_details.support==Support::estimated);const auto& d=*row.linux_details.value;assert(d.provider=="runit-reported-inventory"&&d.group_kind.empty()&&!d.main_pid.value&&!d.description.value&&!d.slice.value&&!d.load_state.value&&!d.active_state.value&&!d.sub_state.value&&!d.unit_file_state.value);}
int main(){
    auto input=fixture();auto out=detail::project_runit_services(input);assert(out.value&&out.value->services.size()==1);const auto& row=out.value->services[0];assert(row.id=="runit-report:616c706861"&&row.status=="reported-run"&&row.linux_details.value->status.support==Support::estimated&&out.value->source.services[0].status.value->reported_pid==123);unavailable(row);
    for(const auto state:{LinuxRunitState::down,LinuxRunitState::run,LinuxRunitState::finish}){input=fixture();auto& s=*input.value->services[0].status.value;s.state=state;s.reported_pid=state==LinuxRunitState::run?456:0;s.paused=true;out=detail::project_runit_services(input);assert(out.value&&out.value->services[0].status==(state==LinuxRunitState::down?"reported-down/paused":state==LinuxRunitState::run?"reported-run/paused":"reported-finish/paused"));unavailable(out.value->services[0]);}
    for(unsigned i=0;i<5;++i){input=fixture();auto& s=*input.value->services[0].status.value;if(i==0)s.reported_pid=0;if(i==1)s.state=LinuxRunitState::down;if(i==2)s.state=static_cast<LinuxRunitState>(88);if(i==3)s.reported_pid=0x80000000U;if(i==4)s.changed_at_tai64n[8]=255;out=detail::project_runit_services(input);assert(out.value&&out.value->services[0].status=="unknown"&&!out.value->services[0].linux_details.value->status.value);}
    input=fixture();input.value->services[0].status.support=Support::permission_required;out=detail::project_runit_services(input);assert(out.value&&out.value->services[0].status=="unknown"&&out.value->services[0].linux_details.value->status.support==Support::permission_required);
    input=fixture();input.value->complete=false;out=detail::project_runit_services(input);assert(out.value&&out.support==Support::estimated&&out.value->services[0].status=="unknown");
    input=fixture();input.support=Support::estimated;out=detail::project_runit_services(input);assert(out.value&&out.value->services[0].status=="unknown");
    input=fixture();input.value->services[0].description={"not standard",Support::available,"fake"};input.value->services[0].group={"not a group",Support::available,"fake"};out=detail::project_runit_services(input);assert(out.value);unavailable(out.value->services[0]);
    input=fixture();input.value->services.push_back(input.value->services[0]);assert(!detail::project_runit_services(input).value);
    input=fixture();input.value->services[0].directory="/other/alpha";assert(!detail::project_runit_services(input).value);
    input=fixture();input.value->root+='/';out=detail::project_runit_services(input);assert(out.value); // Same textual directory with one slash.
    input=fixture();input.value->services[0].name="x/y";assert(!detail::project_runit_services(input).value);
    input=fixture();input.value->services[0].name=std::string("x:\xff",3);input.value->services[0].directory=input.value->root+"/"+input.value->services[0].name;out=detail::project_runit_services(input);assert(out.value&&out.value->services[0].id=="runit-report:783aff");
    input=fixture();input.value->services.clear();assert(detail::project_runit_services(input).value->services.empty());assert(!detail::project_runit_services({{},Support::unsupported,"no data"}).value);
    assert(!query_linux_runit_service_inventory("relative").value&&!query_linux_runit_service_inventory("/unused",0).value&&!query_linux_runit_service_inventory("/unused",4097).value);
#ifdef __linux__
    ::alarm(20);const auto initial=fds();char pattern[]="/tmp/pcinsight-runit-services-XXXXXX";const auto p=::mkdtemp(pattern);assert(p);const fs::path root=p,service=root/"real",supervise=service/"supervise",catalog=root/"catalog",sentinel=root/"must-not-exist";struct Cleanup{fs::path path;~Cleanup(){std::error_code e;fs::remove_all(path,e);}} cleanup{root};fs::create_directories(supervise);fs::create_directory(catalog);fs::create_directory_symlink(service,catalog/"alpha");fs::create_directory(catalog/"unmanaged");{std::ofstream f(service/"run");f<<"touch '"<<sentinel.string()<<"'\n";}
    const auto b=frame();{std::ofstream f(supervise/"status",std::ios::binary);f.write(reinterpret_cast<const char*>(b.data()),b.size());assert(f.good());}assert(::mkfifo((supervise/"ok").c_str(),0600)==0&&::mkfifo((supervise/"control").c_str(),0600)==0);int ok=::open((supervise/"ok").c_str(),O_RDONLY|O_NONBLOCK|O_CLOEXEC);const int control=::open((supervise/"control").c_str(),O_RDONLY|O_NONBLOCK|O_CLOEXEC);assert(ok>=0&&control>=0);
    auto read=[&]{return query_linux_runit_service_inventory(catalog.string());};out=read();assert(out.value&&out.value->source.complete&&out.value->services.size()==2&&out.value->services[0].status=="reported-run"&&out.value->services[1].status=="unknown");unavailable(out.value->services[0]);
    auto bounded=query_linux_runit_service_inventory(catalog.string(),1);assert(bounded.value&&!bounded.value->source.complete&&bounded.value->services[0].status=="unknown");::close(ok);ok=-1;out=read();assert(out.value&&out.value->services[0].status=="unknown"); // Stale file without ok reader.
    for(unsigned i=0;i<10;++i)assert(read().value->services.size()==2);char bytes[8];assert(::read(control,bytes,sizeof bytes)<=0);::close(control);assert(!fs::exists(sentinel)&&fds()==initial);::alarm(0);
#else
    assert(!query_linux_runit_service_inventory("/unused").value);
#endif
    std::cout<<"Runit ServiceInfo reports/unknown PID/partial/IDs/availability/no-control/noexec/FD checks passed; not real supervisor acceptance\n";
}
