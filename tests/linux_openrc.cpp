#include <pcinsight/linux_openrc.hpp>
#include <cassert>
#include <iostream>
#ifdef __linux__
#include <filesystem>
#include <fstream>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <cstdlib>
#endif
using namespace pcinsight;
#ifdef __linux__
namespace fs=std::filesystem;
std::size_t fds(){DIR* d=::opendir("/proc/self/fd");assert(d);std::size_t n=0;while(const auto e=::readdir(d))if(e->d_name[0]!='.')++n;::closedir(d);return n;}
#endif
int main(){
    for(unsigned i=0;i<4;++i){auto s=decode_linux_openrc_markers((1U<<i)|16U);assert(s.value&&static_cast<unsigned>(*s.value)==i);}
    for(auto mask:{0U,16U,3U,15U,128U,~0U})assert(!decode_linux_openrc_markers(mask).value);
    assert(!query_linux_openrc_markers("relative").value);assert(!query_linux_openrc_markers("/",0).value);assert(!query_linux_openrc_markers("/",4097).value);assert(!query_linux_openrc_markers(std::string("/tmp\0x",6)).value);
#ifdef __linux__
    ::alarm(20);const auto initial=fds();char path[]="/tmp/pcinsight-openrc-XXXXXX";const auto d=::mkdtemp(path);assert(d);const fs::path root=d;struct Cleanup{fs::path p;~Cleanup(){std::error_code e;fs::remove_all(p,e);}} cleanup{root};
    assert(!query_linux_openrc_markers((root/"missing").string()).value);assert(!fs::exists(root/"missing"));
    auto empty=query_linux_openrc_markers(root.string());assert(empty.value&&empty.value->complete&&empty.value->entries.empty()&&fs::is_empty(root));
    for(auto category:{"started","stopping","starting","inactive","failed","hotplugged","wasinactive"})fs::create_directory(root/category);
    fs::create_symlink("/absent-init-script",root/"started/zeta");fs::create_symlink("/also-missing",root/"failed/zeta");fs::create_symlink("/not-followed",root/"inactive/alpha");fs::create_symlink("/not-followed",root/"started/.hidden");
    auto read=[&]{return query_linux_openrc_markers(root.string());};
    auto all=read();assert(all.value&&all.value->complete&&all.value->entries.size()==2&&all.value->entries[0].name=="alpha"&&all.value->entries[1].marker_mask==17&&all.value->entries[1].marked_state.value==LinuxOpenrcMarkedState::started);
    fs::create_symlink("/not-followed",root/"starting/zeta");all=read();assert(all.value&&all.value->complete&&!all.value->entries[1].marked_state.value);fs::remove(root/"starting/zeta");
    auto bounded=query_linux_openrc_markers(root.string(),1);assert(bounded.value&&!bounded.value->complete&&bounded.support==Support::estimated&&bounded.value->entries.size()==1&&!bounded.value->entries[0].marked_state.value&&!bounded.value->diagnostics.empty());
    {std::ofstream file(root/"started/bad");file<<"not an OpenRC marker";}all=read();assert(all.value&&!all.value->complete&&!all.value->entries[0].marked_state.value);fs::remove(root/"started/bad");
    assert(::mkfifo((root/"started/fifo").c_str(),0600)==0);all=read();assert(all.value&&!all.value->complete);fs::remove(root/"started/fifo");
    fs::rename(root/"inactive",root/"saved-inactive");fs::create_directory_symlink(root/"saved-inactive",root/"inactive");all=read();assert(all.value&&!all.value->complete&&all.value->entries.size()==1);fs::remove(root/"inactive");fs::rename(root/"saved-inactive",root/"inactive");
    fs::remove_all(root/"failed");assert(::mkfifo((root/"failed").c_str(),0600)==0);all=read();assert(all.value&&!all.value->complete);fs::remove(root/"failed");fs::create_directory(root/"failed");
    assert(!query_linux_openrc_markers("/proc/self/fd/0").value);
    for(unsigned i=0;i<10;++i){all=read();assert(all.value&&all.value->complete);}
    assert(fds()==initial);::alarm(0);
#else
    assert(!query_linux_openrc_markers("/run/openrc").value);
#endif
    std::cout<<"OpenRC primary/conflict/unknown/mask/bounds/private markers/no-follow/no-create/FD checks passed; NOT real OpenRC acceptance or process liveness\n";
}
