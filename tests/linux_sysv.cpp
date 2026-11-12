#include <pcinsight/linux_sysv.hpp>
#include <cassert>
#include <iostream>
#ifdef __linux__
#include <filesystem>
#include <fstream>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cstdlib>
#endif
using namespace pcinsight;
#ifdef __linux__
namespace fs=std::filesystem;
std::size_t fds(){auto d=::opendir("/proc/self/fd");assert(d);std::size_t n=0;while(auto e=::readdir(d))if(e->d_name[0]!='.')++n;::closedir(d);return n;}
#endif
int main(){
    auto start=decode_linux_sysv_link_name("S00example.sh"),stop=decode_linux_sysv_link_name("K99alias");assert(start.value&&start.value->action==LinuxSysVLinkAction::start&&start.value->sequence==0&&start.value->suffix=="example.sh");assert(stop.value&&stop.value->action==LinuxSysVLinkAction::stop&&stop.value->sequence==99);
    for(const auto name:{"","S","S00","S1foo","s00foo","Kxyfoo","S00foo/bar","README"})assert(!decode_linux_sysv_link_name(name).value);assert(!decode_linux_sysv_link_name(std::string("S01x\0y",6)).value);assert(decode_linux_sysv_link_name("S100foo").value->suffix=="0foo");
    assert(!query_linux_sysv_catalog("relative","/etc").value);assert(!query_linux_sysv_catalog("/etc/init.d","/etc",0).value);assert(!query_linux_sysv_catalog("/etc/init.d","/etc",4097).value);
#ifdef __linux__
    ::alarm(20);const auto initial=fds();char pattern[]="/tmp/pcinsight-sysv-XXXXXX";auto d=::mkdtemp(pattern);assert(d);const fs::path root=d,init=root/"init.d",rc=root/"rc";struct Cleanup{fs::path p;~Cleanup(){std::error_code e;fs::remove_all(p,e);}} cleanup{root};auto missing=query_linux_sysv_catalog(init.string(),rc.string());assert(missing.value&&!missing.value->complete&&!fs::exists(init)&&!fs::exists(rc));fs::create_directory(init);fs::create_directory(rc);auto read=[&]{return query_linux_sysv_catalog(init.string(),rc.string());};auto empty=read();assert(empty.value&&empty.value->complete&&empty.value->scripts.empty()&&empty.value->runlevels.size()==11&&empty.value->runlevels[0].directory_present.value==false);
    {std::ofstream f(init/"alpha");f<<"#!/bin/sh\nexit 99\n";}{std::ofstream f(init/"zeta.sh");f<<"metadata only";}{std::ofstream f(init/".hidden");f<<"ignore";}assert(::chmod((init/"alpha").c_str(),0700)==0&&::chmod((init/"zeta.sh").c_str(),0600)==0);fs::create_directory(init/"not-script");assert(::mkfifo((init/"fifo").c_str(),0600)==0);
    fs::create_directory(rc/"rc2.d");fs::create_directory(rc/"rcS.d");fs::create_symlink("../../init.d/alpha",rc/"rc2.d/S10different-name");fs::create_symlink("../../init.d/zeta.sh",rc/"rc2.d/K90zeta.sh");fs::create_symlink("../../init.d/alpha",rc/"rcS.d/S05alpha");fs::create_directory_symlink("rc2.d",rc/"rc3.d");
    auto all=read();assert(all.value&&all.value->complete&&all.value->scripts.size()==2&&all.value->scripts[0].executable_mode.value==true&&all.value->scripts[1].executable_mode.value==false);const auto& level=all.value->runlevels[2];assert(level.directory_present.value==true&&level.entries.size()==2&&level.entries[0].convention.value->action==LinuxSysVLinkAction::stop&&level.entries[0].script_index.value==1&&level.entries[1].script_index.value==0);assert(all.value->runlevels[3].entries.size()==2&&all.value->runlevels[10].entries.size()==1);
    auto bound=query_linux_sysv_catalog(init.string(),rc.string(),1);assert(bound.value&&!bound.value->complete&&bound.support==Support::estimated&&!bound.value->diagnostics.empty()&&!bound.value->scripts[0].executable_mode.value);
    fs::create_hard_link(init/"alpha",init/"hardlink");all=read();assert(all.value&&all.value->complete&&!all.value->runlevels[2].entries[1].script_index.value);fs::remove(init/"hardlink");
    {std::ofstream f(root/"external");f<<"external";}fs::create_symlink("../../external",rc/"rc2.d/S01external");all=read();assert(all.value&&all.value->complete&&all.value->runlevels[2].entries[1].regular_target.value==true&&!all.value->runlevels[2].entries[1].script_index.value);fs::remove(rc/"rc2.d/S01external");
    fs::create_symlink("missing",rc/"rc2.d/S00broken");all=read();assert(all.value&&!all.value->complete&&!all.value->runlevels[2].entries[1].script_index.value);fs::remove(rc/"rc2.d/S00broken");
    assert(::mkfifo((rc/"rc2.d/S00fifo").c_str(),0600)==0);all=read();assert(all.value&&all.value->complete&&all.value->runlevels[2].entries[1].regular_target.value==false);fs::remove(rc/"rc2.d/S00fifo");
    fs::create_symlink("/proc/self/fd/0",init/"magic");all=read();assert(all.value&&!all.value->complete);fs::remove(init/"magic");
    fs::create_symlink("missing",rc/"rc4.d");all=read();assert(all.value&&!all.value->complete&&!all.value->runlevels[4].directory_present.value);fs::remove(rc/"rc4.d");
    for(unsigned i=0;i<10;++i)assert(read().value->complete);assert(fds()==initial);::alarm(0);
#else
    assert(!query_linux_sysv_catalog("/etc/init.d","/etc").value);
#endif
    std::cout<<"SysV filename convention/explicit catalog/identity/aliases/ambiguity/broken-FIFO/magic/bounds/noexec/nocreate/FD checks passed; NOT live service state/enablement or real init acceptance\n";
}
