#include <pcinsight/linux_sysv.hpp>
#include "../src/linux_sysv_lsb.hpp"
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
void write(const fs::path& p,const std::string& s){std::ofstream f(p);f<<s;f.close();assert(f.good());}
std::string script(const std::string& label,const fs::path& sentinel){return "#!/bin/sh\n### BEGIN INIT INFO\n# Short-Description: "+label+"\n# Description: declared only\n### END INIT INFO\ntouch '"+sentinel.string()+"'\n";}
#endif
using namespace pcinsight;
int main(){
    assert(!query_linux_sysv_described_catalog("relative","/etc").value);
    assert(!query_linux_sysv_described_catalog("/etc/init.d","/etc",1024,16*1024*1024+1).value);
#ifdef __linux__
    ::alarm(20);const auto initial=fds();char pattern[]="/tmp/pcinsight-sysv-described-XXXXXX";auto d=::mkdtemp(pattern);assert(d);const fs::path root=d,init=root/"init.d",rc=root/"rc",sentinel=root/"must-not-exist";
    struct Cleanup{fs::path p;~Cleanup(){std::error_code e;fs::remove_all(p,e);}} cleanup{root};
    auto missing=query_linux_sysv_described_catalog(init.string(),rc.string());assert(missing.value&&!missing.value->catalog.complete&&!missing.value->all_metadata_reads_attempted&&!fs::exists(init));
    fs::create_directory(init);fs::create_directory(rc);auto read=[&](std::uint64_t budget=8*1024*1024){return query_linux_sysv_described_catalog(init.string(),rc.string(),1024,budget);};
    auto empty=read(0);assert(empty.value&&empty.value->catalog.complete&&empty.value->metadata.empty()&&empty.value->all_metadata_reads_attempted&&empty.value->prefix_bytes_reserved==0);
    const auto body=script("original",sentinel);write(init/"alpha",body);fs::create_directory(rc/"rc2.d");fs::create_symlink("../../init.d/alpha",rc/"rc2.d/S01different-name");
    auto good=read();assert(good.value&&good.value->catalog.complete&&good.value->metadata.size()==1&&good.value->all_metadata_reads_attempted&&good.value->metadata[0].value);assert(good.value->metadata[0].value->short_description.value=="original");assert(good.value->catalog.runlevels[2].entries[0].script_index.value==0);assert(good.value->prefix_bytes_reserved==2*body.size());
    auto zero=read(0),short_budget=read(2*body.size()-1),exact=read(2*body.size());assert(zero.value&&zero.value->catalog.complete&&!zero.value->all_metadata_reads_attempted&&!zero.value->metadata[0].value&&zero.value->prefix_bytes_reserved==0);assert(short_budget.value&&!short_budget.value->metadata[0].value&&short_budget.value->prefix_bytes_reserved==0);assert(exact.value&&exact.value->metadata[0].value&&exact.value->all_metadata_reads_attempted);
    fs::create_hard_link(init/"alpha",init/"beta");good=read();assert(good.value&&good.value->catalog.complete&&good.value->metadata.size()==2&&good.value->metadata[0].value&&good.value->metadata[1].value&&!good.value->catalog.runlevels[2].entries[0].script_index.value);fs::remove(init/"beta");
    write(init/"z-no-header","#!/bin/sh\nexit 3\n");good=read();assert(good.value&&good.value->catalog.complete&&good.value->all_metadata_reads_attempted&&good.value->metadata[0].value&&!good.value->metadata[1].value);fs::remove(init/"z-no-header");
    unsigned calls=0;detail::SysVDescribedSource source;source.read=[&](int fd,const detail::SysVLsbFileIdentity& id){++calls;return detail::read_sysv_lsb_pinned(fd,id);};
    auto limited=detail::query_sysv_described_with_source(init.string(),rc.string(),1024,0,source);assert(limited.value&&calls==0);
    fs::create_symlink("missing",rc/"rc2.d/S00broken");auto partial=detail::query_sysv_described_with_source(init.string(),rc.string(),1024,8*1024*1024,source);assert(partial.value&&!partial.value->catalog.complete&&calls==0&&!partial.value->metadata[0].value);fs::remove(rc/"rc2.d/S00broken");
    // Swap after first census, BEFORE pin selection: replacement must not be read.
    source.after_first_scan=[&]{fs::rename(init/"alpha",root/"old");write(init/"alpha",script("replacement",sentinel));};
    auto before_pin=detail::query_sysv_described_with_source(init.string(),rc.string(),1024,8*1024*1024,source);assert(before_pin.value&&!before_pin.value->catalog.complete&&calls==0&&!before_pin.value->metadata[0].value);fs::remove(init/"alpha");fs::rename(root/"old",init/"alpha");source.after_first_scan={};
    // Reader sees old pinned content; replacement under same basename invalidates output.
    source.read=[&](int fd,const detail::SysVLsbFileIdentity& id){++calls;auto value=detail::read_sysv_lsb_pinned(fd,id);assert(value.value&&value.value->short_description.value=="original");auto wrong=id;++wrong.size;assert(!detail::read_sysv_lsb_pinned(fd,wrong).value);fs::rename(init/"alpha",root/"old");write(init/"alpha",script("replacement",sentinel));return value;};
    auto during=detail::query_sysv_described_with_source(init.string(),rc.string(),1024,8*1024*1024,source);assert(during.value&&!during.value->catalog.complete&&calls==1&&!during.value->metadata[0].value&&!during.value->all_metadata_reads_attempted);fs::remove(init/"alpha");fs::rename(root/"old",init/"alpha");
    source.read=detail::read_sysv_lsb_pinned;
    source.after_first_scan=[&]{fs::rename(init,root/"old-init");fs::create_directory(init);write(init/"alpha",script("foreign-root",sentinel));};
    auto root_swap=detail::query_sysv_described_with_source(init.string(),rc.string(),1024,8*1024*1024,source);assert(root_swap.value&&!root_swap.value->catalog.complete&&!root_swap.value->metadata[0].value);fs::remove(init/"alpha");fs::remove(init);fs::rename(root/"old-init",init);
    // Full prefix charge is bounded even for much larger scripts.
    write(init/"alpha",body+std::string(100000,'x'));good=read(131072);assert(good.value&&good.value->catalog.complete&&good.value->metadata[0].value&&good.value->prefix_bytes_reserved==131072);
    assert(::mkfifo((init/"ignored-fifo").c_str(),0600)==0);for(unsigned i=0;i<10;++i){auto value=read();assert(value.value&&value.value->catalog.complete&&value.value->metadata.size()==1);}
    assert(!fs::exists(sentinel)&&fds()==initial);::alarm(0);
#else
    assert(!query_linux_sysv_described_catalog("/etc/init.d","/etc").value);
#endif
    std::cout<<"SysV bound descriptions/budget/identity/same-name replacement/partial/noexec/FD checks passed; not real init acceptance\n";
}
