#include <pcinsight/linux_openrc_services.hpp>
#include "../src/linux_openrc_services.hpp"
#include <cassert>
#include <iostream>
#ifdef __linux__
#include <filesystem>
#include <fstream>
#include <dirent.h>
#include <unistd.h>
#include <cstdlib>
namespace fs=std::filesystem;
std::size_t fds(){auto d=::opendir("/proc/self/fd");assert(d);std::size_t n=0;while(auto e=::readdir(d))if(e->d_name[0]!='.')++n;::closedir(d);return n;}
#endif
using namespace pcinsight;
Metric<LinuxOpenrcCatalog> catalog(){LinuxOpenrcCatalog c;c.init_directory="/private/init.d";c.runlevel_directory="/private/runlevels";c.scripts={{"alpha",{true,Support::available,"mode only"}}};return {c,Support::available,"fixture"};}
Metric<LinuxOpenrcMarkerInventory> markers(){LinuxOpenrcMarkerInventory m;m.root="/private/runtime";m.entries={{"alpha",1,decode_linux_openrc_markers(1)}};return {m,Support::available,"fixture"};}
void unavailable(const ServiceInfo& r){assert(!r.pid&&r.description.empty()&&r.group.empty()&&r.load_state.empty()&&r.active_state.empty()&&r.sub_state.empty()&&r.unit_file_state.empty());assert(r.linux_details.value&&r.linux_details.support==Support::estimated);const auto& d=*r.linux_details.value;assert(d.group_kind.empty()&&!d.main_pid.value&&!d.description.value&&!d.slice.value&&!d.load_state.value&&!d.active_state.value&&!d.sub_state.value&&!d.unit_file_state.value);}
int main(){
    auto c=catalog();auto m=markers();auto out=detail::project_openrc_services(c,m);assert(out.value&&out.support==Support::available&&out.value->configured_services.size()==1&&out.value->marked_services.size()==1);const auto& file=out.value->configured_services[0];const auto& mark=out.value->marked_services[0];assert(file.name==mark.name&&file.id=="openrc-file:616c706861"&&mark.id=="openrc-mark:616c706861"&&file.id!=mark.id);assert(file.status=="unknown"&&!file.linux_details.value->status.value&&mark.status=="marked-started"&&mark.linux_details.value->status.support==Support::estimated);assert(file.linux_details.value->provider=="openrc-file-catalog"&&mark.linux_details.value->provider=="openrc-runtime-marks");unavailable(file);unavailable(mark);
    for(unsigned i=0;i<4;++i){m=markers();const auto mask=(1U<<i)|16;m.value->entries[0].marker_mask=mask;m.value->entries[0].marked_state=decode_linux_openrc_markers(mask);out=detail::project_openrc_services(c,m);assert(out.value&&out.value->marked_services[0].status==std::string(i==0?"marked-started":i==1?"marked-stopping":i==2?"marked-starting":"marked-inactive")&&out.value->markers.value->entries[0].marker_mask==mask&&out.value->configured_services[0].status=="unknown");}
    for(auto mask:{0U,3U,16U,128U}){m=markers();m.value->entries[0].marker_mask=mask;out=detail::project_openrc_services(c,m);assert(out.value&&out.value->marked_services[0].status=="unknown");}
    m=markers();m.value->entries[0].marked_state.value=LinuxOpenrcMarkedState::inactive;out=detail::project_openrc_services(c,m);assert(out.value&&out.value->marked_services[0].status=="unknown");
    m=markers();m.value->entries[0].marked_state.support=Support::permission_required;out=detail::project_openrc_services(c,m);assert(out.value&&out.value->marked_services[0].linux_details.value->status.support==Support::permission_required&&!out.value->marked_services[0].linux_details.value->status.value);
    c=catalog();c.value->complete=false;m=markers();out=detail::project_openrc_services(c,m);assert(out.value&&out.support==Support::estimated&&out.value->marked_services[0].status=="marked-started"); // Separate source, not a join.
    c=catalog();m.value->complete=false;out=detail::project_openrc_services(c,m);assert(out.value&&out.value->marked_services[0].status=="unknown"&&out.value->configured_services[0].linux_details.value->name.support==Support::available);
    m=markers();out=detail::project_openrc_services({{},Support::permission_required,"catalog denied"},m);assert(out.value&&out.support==Support::estimated&&out.value->configured_services.empty()&&out.value->marked_services.size()==1&&out.value->catalog.support==Support::permission_required);
    out=detail::project_openrc_services(catalog(),{{},Support::unsupported,"no runtime"});assert(out.value&&out.value->marked_services.empty()&&out.value->configured_services.size()==1);
    assert(!detail::project_openrc_services({{},Support::permission_required,"denied"},{{},Support::unsupported,"missing"}).value);
    c=catalog();c.value->scripts.push_back(c.value->scripts[0]);assert(!detail::project_openrc_services(c,markers()).value);
    m=markers();m.value->entries.push_back(m.value->entries[0]);assert(!detail::project_openrc_services(catalog(),m).value);
    c=catalog();c.value->scripts[0].name="x/y";assert(!detail::project_openrc_services(c,markers()).value);
    m=markers();m.value->entries[0].name=std::string("x:\xff",3);out=detail::project_openrc_services(catalog(),m);assert(out.value&&out.value->marked_services[0].id=="openrc-mark:783aff");
    c=catalog();m=markers();c.value->scripts.clear();m.value->entries.clear();out=detail::project_openrc_services(c,m);assert(out.value&&out.value->configured_services.empty()&&out.value->marked_services.empty());
    assert(!query_linux_openrc_service_inventory("relative","/unused","/unused").value&&!query_linux_openrc_service_inventory("/unused","/unused","relative").value&&!query_linux_openrc_service_inventory("/unused","/unused","/unused",0).value&&!query_linux_openrc_service_inventory("/unused","/unused","/unused",4097).value);
#ifdef __linux__
    ::alarm(20);const auto initial=fds();char pattern[]="/tmp/pcinsight-openrc-services-XXXXXX";auto p=::mkdtemp(pattern);assert(p);const fs::path root=p,init=root/"init.d",levels=root/"runlevels",runtime=root/"runtime",sentinel=root/"must-not-exist";struct Cleanup{fs::path path;~Cleanup(){std::error_code e;fs::remove_all(path,e);}} cleanup{root};
    fs::create_directory(init);fs::create_directory(levels);fs::create_directory(levels/"default");fs::create_directories(runtime/"started");fs::create_directory(runtime/"starting");{std::ofstream f(init/"alpha");f<<"touch '"<<sentinel.string()<<"'\n";}{std::ofstream f(init/"beta");f<<"exit 0\n";}fs::create_symlink("../../init.d/alpha",levels/"default/alpha");fs::create_symlink("/missing/unrelated",runtime/"started/alpha");fs::create_symlink("/missing/ghost",runtime/"started/ghost");auto read=[&]{return query_linux_openrc_service_inventory(init.string(),levels.string(),runtime.string());};out=read();assert(out.value&&out.support==Support::available&&out.value->configured_services.size()==2&&out.value->marked_services.size()==2&&out.value->configured_services[0].status=="unknown"&&out.value->marked_services[0].status=="marked-started"&&out.value->marked_services[1].name=="ghost"&&out.value->catalog.value->runlevels[0].references[0].script_index.value==0);for(const auto& r:out.value->configured_services)unavailable(r);for(const auto& r:out.value->marked_services)unavailable(r);
    fs::create_symlink("/missing/another",runtime/"starting/alpha");out=read();assert(out.value&&out.value->marked_services[0].status=="unknown");fs::remove(runtime/"starting/alpha");
    {std::ofstream f(runtime/"started/bad");f<<"not a marker";}out=read();assert(out.value&&out.support==Support::estimated&&!out.value->markers.value->complete&&out.value->catalog.value->complete&&out.value->marked_services[0].status=="unknown");fs::remove(runtime/"started/bad");
    auto bound=query_linux_openrc_service_inventory(init.string(),levels.string(),runtime.string(),1);assert(bound.value&&bound.support==Support::estimated&&!bound.value->catalog.value->complete&&!bound.value->markers.value->complete);
    auto missing=query_linux_openrc_service_inventory(init.string(),levels.string(),(root/"missing").string());assert(missing.value&&missing.support==Support::estimated&&missing.value->configured_services.size()==2&&missing.value->marked_services.empty()&&!fs::exists(root/"missing"));for(unsigned i=0;i<10;++i)assert(read().value->marked_services.size()==2);assert(!fs::exists(sentinel)&&fds()==initial);::alarm(0);
#else
    assert(!query_linux_openrc_service_inventory("/unused","/unused","/unused").value);
#endif
    std::cout<<"OpenRC separate ServiceInfo views/no name identity join/marks/partial/permission/IDs/noexec/FD checks passed; not real init acceptance\n";
}
