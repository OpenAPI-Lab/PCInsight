#include <pcinsight/linux_openrc.hpp>
#include <cassert>
#include <iostream>
#ifdef __linux__
#include <filesystem>
#include <fstream>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <cstdlib>
#endif
using namespace pcinsight;
#ifdef __linux__
namespace fs=std::filesystem;
std::size_t fds(){auto d=::opendir("/proc/self/fd");assert(d);std::size_t n=0;while(auto e=::readdir(d))if(e->d_name[0]!='.')++n;::closedir(d);return n;}
#endif
int main(){
    assert(!query_linux_openrc_catalog("relative","/etc/runlevels").value);assert(!query_linux_openrc_catalog("/etc/init.d","/etc/runlevels",0).value);assert(!query_linux_openrc_catalog("/etc/init.d","/etc/runlevels",4097).value);
#ifdef __linux__
    ::alarm(20);const auto initial=fds();char temp[]="/tmp/pcinsight-openrc-catalog-XXXXXX";auto d=::mkdtemp(temp);assert(d);const fs::path root=d,init=root/"init.d",levels=root/"runlevels";struct Cleanup{fs::path p;~Cleanup(){std::error_code e;fs::remove_all(p,e);}} cleanup{root};
    auto missing=query_linux_openrc_catalog(init.string(),levels.string());assert(missing.value&&!missing.value->complete&&!fs::exists(init)&&!fs::exists(levels));
    fs::create_directory(init);fs::create_directory(levels);auto read=[&]{return query_linux_openrc_catalog(init.string(),levels.string());};auto empty=read();assert(empty.value&&empty.value->complete&&empty.value->scripts.empty()&&empty.value->runlevels.empty());
    {std::ofstream f(init/"alpha");f<<"#!/bin/sh\nexit 99\n";}{std::ofstream f(init/"zeta");f<<"not necessarily an executable script";}{std::ofstream f(init/"ignored.sh");f<<"ignore";}{std::ofstream f(init/".hidden");f<<"ignore";}
    assert(::chmod((init/"alpha").c_str(),0700)==0&&::chmod((init/"zeta").c_str(),0600)==0);fs::create_directory(init/"not-a-service");assert(::mkfifo((init/"fifo").c_str(),0600)==0);
    fs::create_directory(levels/"boot");fs::create_directory(levels/"default");fs::create_symlink("../../init.d/alpha",levels/"boot/alpha");fs::create_symlink("../boot",levels/"default/boot");fs::create_symlink("../../init.d/zeta",levels/"default/alias-zeta");fs::create_symlink("../default",levels/"boot/default");
    auto all=read();assert(all.value&&all.value->complete&&all.value->scripts.size()==2&&all.value->scripts[0].executable_mode.value==true&&all.value->scripts[1].executable_mode.value==false);assert(all.value->runlevels.size()==2);
    const auto& boot=all.value->runlevels[0];const auto& def=all.value->runlevels[1];assert(boot.references[0].script_index.value==0&&boot.references[1].stacked_level.value=="default");assert(def.references[0].script_index.value==1&&def.references[1].stacked_level.value=="boot"); // Cycles are raw edges, not recursively followed.
    auto bounded=query_linux_openrc_catalog(init.string(),levels.string(),1);assert(bounded.value&&!bounded.value->complete&&bounded.support==Support::estimated&&!bounded.value->scripts[0].executable_mode.value);
    fs::create_hard_link(init/"alpha",init/"alpha-hardlink");all=read();assert(all.value&&all.value->complete&&!all.value->runlevels[0].references[0].script_index.value);fs::remove(init/"alpha-hardlink");
    fs::create_symlink("missing",levels/"default/broken");all=read();assert(all.value&&!all.value->complete&&!all.value->runlevels[0].references[0].script_index.value);fs::remove(levels/"default/broken");
    assert(::mkfifo((levels/"default/fifo").c_str(),0600)==0);all=read();assert(all.value&&!all.value->complete);fs::remove(levels/"default/fifo");
    fs::create_directory_symlink("boot",levels/"top-alias");all=read();assert(all.value&&!all.value->complete);fs::remove(levels/"top-alias");
    fs::create_symlink("missing",init/"broken");all=read();assert(all.value&&!all.value->complete);fs::remove(init/"broken");
    fs::create_symlink("/proc/self/fd/0",init/"magic");all=read();assert(all.value&&!all.value->complete);fs::remove(init/"magic");
    {std::ofstream f(root/"external");f<<"outside supplied init.d";}fs::create_symlink("../../external",levels/"default/external");all=read();assert(all.value&&all.value->complete);const auto& external=all.value->runlevels[1].references[2];assert(external.name=="external"&&external.kind==LinuxOpenrcReferenceKind::service&&!external.script_index.value);fs::remove(levels/"default/external");
    fs::create_symlink("../../init.d/alpha",levels/"default/ignored.sh");all=read();assert(all.value&&all.value->complete&&all.value->runlevels[1].references.size()==2);fs::remove(levels/"default/ignored.sh");
    for(unsigned i=0;i<10;++i){all=read();assert(all.value&&all.value->complete);}
    assert(fds()==initial);::alarm(0);
#else
    assert(!query_linux_openrc_catalog("/etc/init.d","/etc/runlevels").value);
#endif
    std::cout<<"OpenRC explicit catalog/metadata/link-identity/stacks-cycle/ambiguity/bounds/no-create/no-exec/FD checks passed; NOT real OpenRC acceptance\n";
}
