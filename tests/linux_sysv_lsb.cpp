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
const std::string begin="### BEGIN INIT INFO\n",end="### END INIT INFO\n";
#ifdef __linux__
namespace fs=std::filesystem;
void put(const fs::path& p,const std::string& text){std::ofstream f(p,std::ios::binary);assert(f);f.write(text.data(),text.size());f.close();assert(f);}
std::size_t fds(){auto d=::opendir("/proc/self/fd");assert(d);std::size_t n=0;while(auto e=::readdir(d))if(e->d_name[0]!='.')++n;::closedir(d);return n;}
#endif
int main(){
    const std::string fields="# Provides: $network\n# Short-Description:  Service\t example\n# Description: Long\n#  detailed text\n#  \n#\twith UTF-8 \xe2\x82\xac and $(literal) ${HOME}\n";
    auto parsed=parse_linux_sysv_lsb_metadata("#!/bin/sh\n# license\n\n"+begin+fields+end+"exit 99\n");assert(parsed.value&&parsed.value->short_description.value=="Service example"&&parsed.value->description.value=="Long detailed text with UTF-8 \xe2\x82\xac and $(literal) ${HOME}");
    assert(parse_linux_sysv_lsb_metadata(begin+fields+end+"exec command",false).value);assert(!parse_linux_sysv_lsb_metadata(begin+fields+end,false).value);
    auto absent=parse_linux_sysv_lsb_metadata(begin+"# Provides: nothing\n"+end);assert(absent.value&&!absent.value->short_description.value&&!absent.value->description.value);
    auto empty=parse_linux_sysv_lsb_metadata(begin+"# Short-Description: \n# Description:\n#  continuation\n"+end);assert(empty.value&&!empty.value->short_description.value&&empty.value->description.value=="continuation");
    const std::string crlf="### BEGIN INIT INFO \t\r\n# Short-Description: windows line endings\r\n### END INIT INFO\t\r\n";assert(parse_linux_sysv_lsb_metadata(crlf).value->short_description.value=="windows line endings");
    for(const auto& text:{std::string{},std::string("exit 0\n")+begin+fields+end,std::string("cat <<'EOF'\n")+begin+fields+end+"EOF\n",begin+fields,begin+"echo not-comment\n"+end,begin+" # Short-Description: bad column\n"+end,begin+"#\torphan\n"+end,begin+"# Short-Description: one\n# Short-Description: two\n"+end,begin+fields+end+begin+end,begin+begin+end,std::string("### END INIT INFO\n")+begin+end})assert(!parse_linux_sysv_lsb_metadata(text).value);
    for(const auto& bad:{std::string("\xff",1),std::string("\xc0\xaf",2),std::string("\xed\xa0\x80",3),std::string("\xc2\x85",2),std::string("\x1b",1),std::string("\0",1)})assert(!parse_linux_sysv_lsb_metadata(begin+"# Description: "+bad+"\n"+end).value);
    assert(!parse_linux_sysv_lsb_metadata(std::string(65537,'#')).value);assert(!parse_linux_sysv_lsb_metadata(begin+"# Description: "+std::string(1025,'x')+"\n"+end).value);assert(!parse_linux_sysv_lsb_metadata(std::string(4097,'#')+"\n"+begin+end).value);
    std::string many;for(unsigned i=0;i<1025;++i)many+="#\n";assert(!parse_linux_sysv_lsb_metadata(many+begin+end).value);
    std::string exact=begin+"# Description: "+std::string(1024,'a')+"\n#  "+std::string(1024,'b')+"\n#  "+std::string(1024,'c')+"\n#  "+std::string(1021,'d')+"\n";const auto boundary=parse_linux_sysv_lsb_metadata(exact+end);assert(boundary.value&&boundary.value->description.value&&boundary.value->description.value->size()==4096);assert(!parse_linux_sysv_lsb_metadata(exact+"#  extra\n"+end).value);
    assert(parse_linux_sysv_lsb_metadata(begin+fields+end+std::string(60000,'x'),false).value); // Body not parsed as metadata.
    assert(!read_linux_sysv_lsb_metadata("relative").value);assert(!read_linux_sysv_lsb_metadata(std::string("/tmp\0x",6)).value);
#ifdef __linux__
    ::alarm(20);const auto initial=fds();char pattern[]="/tmp/pcinsight-sysv-lsb-XXXXXX";auto d=::mkdtemp(pattern);assert(d);const fs::path root=d,file=root/"service",sentinel=root/"MUST-NOT-EXIST";struct Cleanup{fs::path p;~Cleanup(){std::error_code e;fs::remove_all(p,e);}} cleanup{root};assert(!read_linux_sysv_lsb_metadata(file.string()).value&&!fs::exists(file));
    put(file,"#!/bin/sh\n"+begin+fields+end+"touch '"+sentinel.string()+"'\n"+std::string(100000,'x'));auto native=read_linux_sysv_lsb_metadata(file.string());assert(native.value&&native.value->description.value==parsed.value->description.value&&!fs::exists(sentinel));
    fs::create_symlink(file,root/"alias");assert(read_linux_sysv_lsb_metadata((root/"alias").string()).value->short_description.value=="Service example");assert(::mkfifo((root/"fifo").c_str(),0600)==0);assert(!read_linux_sysv_lsb_metadata((root/"fifo").string()).value);assert(!read_linux_sysv_lsb_metadata(root.string()).value);fs::create_symlink("/proc/self/fd/0",root/"magic");assert(!read_linux_sysv_lsb_metadata((root/"magic").string()).value);
    put(root/"truncated",begin+"# Description: unterminated\n"+std::string(100000,'#'));assert(!read_linux_sysv_lsb_metadata((root/"truncated").string()).value);
    if(::geteuid()!=0){assert(::chmod(file.c_str(),0000)==0);auto denied=read_linux_sysv_lsb_metadata(file.string());assert(!denied.value&&denied.support==Support::permission_required);assert(::chmod(file.c_str(),0600)==0);}
    for(unsigned i=0;i<10;++i)assert(read_linux_sysv_lsb_metadata(file.string()).value);assert(!fs::exists(sentinel)&&fds()==initial);::alarm(0);
#else
    assert(!read_linux_sysv_lsb_metadata("/etc/init.d/example").value);
#endif
    std::cout<<"LSB bounded leading-comment/UTF-8/duplicate/truncation/heredoc/literal/no-exec/pinnedfile/FD checks passed; NOT publisher/live-state/boot-policy proof\n";
}
