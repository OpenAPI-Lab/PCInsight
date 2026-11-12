#include <pcinsight/linux_sysv_services.hpp>
#include "../src/linux_sysv_services.hpp"
#include <cassert>
#include <iostream>
#ifdef __linux__
#include <filesystem>
#include <fstream>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cstdlib>
namespace fs=std::filesystem;
std::size_t fds(){auto d=::opendir("/proc/self/fd");assert(d);std::size_t n=0;while(auto e=::readdir(d))if(e->d_name[0]!='.')++n;::closedir(d);return n;}
void write(const fs::path& path,const std::string& text){std::ofstream f(path);f<<text;f.close();assert(f.good());}
#endif
using namespace pcinsight;
void unknown_state(const ServiceInfo& row){
    assert(row.status=="unknown"&&!row.pid&&row.group.empty()&&row.load_state.empty()&&row.active_state.empty()&&row.sub_state.empty()&&row.unit_file_state.empty());
    assert(row.linux_details.value&&row.linux_details.support==Support::estimated);const auto& d=*row.linux_details.value;assert(d.provider=="sysv-file-catalog"&&d.group_kind.empty());
    assert(!d.status.value&&!d.main_pid.value&&!d.load_state.value&&!d.active_state.value&&!d.sub_state.value&&!d.unit_file_state.value&&!d.slice.value);
}
Metric<LinuxSysVDescribedCatalog> fixture(){
    LinuxSysVDescribedCatalog source;source.catalog.init_directory="/private/init.d";source.catalog.runlevel_root="/private/rc";
    source.catalog.scripts={{"alpha",{true,Support::available,"mode only"}},{"beta",{false,Support::available,"mode only"}}};
    source.metadata={parse_linux_sysv_lsb_metadata("### BEGIN INIT INFO\n# Short-Description: short alpha\n# Description: long alpha\n### END INIT INFO\n"),parse_linux_sysv_lsb_metadata("### BEGIN INIT INFO\n# Description: long beta\n### END INIT INFO\n")};
    return {std::move(source),Support::available,"fixture"};
}
int main(){
    auto source=fixture();source.value->all_metadata_reads_attempted=false;auto projected=detail::project_sysv_services(source);assert(projected.value&&projected.support==Support::available&&projected.value->services.size()==2);
    const auto& a=projected.value->services[0];const auto& b=projected.value->services[1];assert(a.id=="sysv-file:616c706861"&&a.id!=b.id&&a.name=="alpha"&&b.name=="beta");assert(a.description=="short alpha"&&b.description=="long beta");unknown_state(a);unknown_state(b);assert(a.linux_details.value->description.note.find("Short-Description")!=std::string::npos&&b.linux_details.value->description.note.find("fallback")!=std::string::npos);assert(projected.value->source.metadata[0].value->description.value=="long alpha");
    source=fixture();source.value->catalog.complete=false;projected=detail::project_sysv_services(source);assert(projected.value&&projected.support==Support::estimated&&projected.value->services[0].description.empty()&&!projected.value->services[0].linux_details.value->description.value);unknown_state(projected.value->services[0]);
    source=fixture();source.support=Support::estimated;projected=detail::project_sysv_services(source);assert(projected.value&&!projected.value->services[0].linux_details.value->description.value);
    source=fixture();source.value->metadata[0]={{},Support::permission_required,"denied"};projected=detail::project_sysv_services(source);assert(projected.value&&projected.value->services[0].description.empty()&&projected.value->services[0].linux_details.value->description.support==Support::permission_required);
    source=fixture();source.value->metadata[0].support=Support::unsupported;projected=detail::project_sysv_services(source);assert(projected.value&&projected.value->services[0].description.empty());
    source=fixture();source.value->metadata[0].value->short_description.support=Support::permission_required;projected=detail::project_sysv_services(source);assert(projected.value&&projected.value->services[0].description=="long alpha");
    source=fixture();source.value->metadata.pop_back();assert(!detail::project_sysv_services(source).value);
    source=fixture();source.value->catalog.scripts[1].name="alpha";assert(!detail::project_sysv_services(source).value);
    for(const auto name:{"", ".hidden", "x/y"}){source=fixture();source.value->catalog.scripts[0].name=name;assert(!detail::project_sysv_services(source).value);}
    source=fixture();source.value->catalog.scripts[0].name=std::string("x\0y",3);assert(!detail::project_sysv_services(source).value);
    source=fixture();source.value->catalog.scripts[0].name=std::string("x:\xff",3);projected=detail::project_sysv_services(source);assert(projected.value&&projected.value->services[0].id=="sysv-file:783aff"&&projected.value->services[0].name==std::string("x:\xff",3));
    source=fixture();source.value->catalog.scripts.clear();source.value->metadata.clear();projected=detail::project_sysv_services(source);assert(projected.value&&projected.value->services.empty());
    assert(!detail::project_sysv_services({{},Support::unsupported,"no source"}).value);assert(!detail::project_sysv_services({{},Support::permission_required,"denied"}).value);
    assert(!query_linux_sysv_service_inventory("relative","/etc").value);assert(!query_linux_sysv_service_inventory("/etc/init.d","/etc",0).value);assert(!query_linux_sysv_service_inventory("/etc/init.d","/etc",1024,16*1024*1024+1).value);
#ifdef __linux__
    ::alarm(20);const auto initial=fds();char pattern[]="/tmp/pcinsight-sysv-services-XXXXXX";auto path=::mkdtemp(pattern);assert(path);const fs::path root=path,init=root/"init.d",rc=root/"rc",sentinel=root/"must-not-exist";struct Cleanup{fs::path path;~Cleanup(){std::error_code e;fs::remove_all(path,e);}} cleanup{root};
    auto missing=query_linux_sysv_service_inventory(init.string(),rc.string());assert(missing.value&&!missing.value->source.catalog.complete&&missing.value->services.empty()&&!fs::exists(init));fs::create_directory(init);fs::create_directory(rc);
    write(init/"alpha","#!/bin/sh\n### BEGIN INIT INFO\n# Short-Description: native alpha\n# Description: long native alpha\n### END INIT INFO\ntouch '"+sentinel.string()+"'\n");fs::create_directory(rc/"rc2.d");fs::create_symlink("../../init.d/alpha",rc/"rc2.d/S01not-alpha");assert(::chmod((init/"alpha").c_str(),0700)==0);
    auto read=[&](std::uint64_t budget=8*1024*1024){return query_linux_sysv_service_inventory(init.string(),rc.string(),1024,budget);};auto native=read();assert(native.value&&native.value->source.catalog.complete&&native.value->services.size()==1&&native.value->services[0].description=="native alpha"&&native.value->source.catalog.runlevels[2].entries[0].script_index.value==0);unknown_state(native.value->services[0]);
    auto disabled=read(0);assert(disabled.value&&disabled.value->source.catalog.complete&&disabled.value->services[0].description.empty()&&!disabled.value->services[0].linux_details.value->description.value);
    fs::create_symlink("missing",rc/"rc2.d/S02broken");auto partial=read();assert(partial.value&&!partial.value->source.catalog.complete&&partial.support==Support::estimated&&partial.value->services[0].description.empty());unknown_state(partial.value->services[0]);fs::remove(rc/"rc2.d/S02broken");
    assert(::mkfifo((init/"ignored-fifo").c_str(),0600)==0);for(unsigned i=0;i<10;++i)assert(read().value->services.size()==1);assert(!fs::exists(sentinel)&&fds()==initial);::alarm(0);
#else
    assert(!query_linux_sysv_service_inventory("/etc/init.d","/etc").value);
#endif
    std::cout<<"SysV ServiceInfo projection/field availability/scoped IDs/alignment/native noexec/FD checks passed; not running state or real init acceptance\n";
}
