#include "../src/linux_privileged_policy.hpp"
#include <cassert>
#ifdef __linux__
#include <dirent.h>
#include <filesystem>
#include <fstream>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#endif
using namespace pcinsight;using namespace pcinsight::detail;
namespace {
constexpr auto header="PCINSIGHT-SERVICE-POLICY 1\n";
const std::string good=std::string(header)+"private.service 7\nblocked.service 0\n";
#ifdef __linux__
std::size_t fds(){auto* d=opendir("/proc/self/fd");assert(d);std::size_t n{};while(auto* e=readdir(d))if(e->d_name[0]!='.')++n;closedir(d);return n;}
void native(){
    alarm(20);const auto initial=fds();char temp[]="/tmp/pcinsight-policy-XXXXXX";const auto* p=mkdtemp(temp);assert(p);const std::filesystem::path root=p;
    struct Cleanup{std::filesystem::path root;~Cleanup(){std::error_code e;std::filesystem::remove_all(root,e);}}cleanup{root};
    assert(mkdir((root/"cfg").c_str(),0700)==0);const auto file=root/"cfg"/"policy.conf";
    auto write=[&](const std::string& data){std::ofstream out(file);out<<data;out.close();assert(out.good());assert(chmod(file.c_str(),0600)==0);};write(good);
    const int anchor=open(root.c_str(),O_RDONLY|O_DIRECTORY|O_CLOEXEC);assert(anchor>=0);const auto owner=std::uint32_t(geteuid());
    auto load=[&](std::string_view path="cfg/policy.conf"){return load_privileged_policy(anchor,path,owner);};assert(load().value&&load().value->size()==2);
    for(const auto* path:{"","/cfg/policy.conf","../cfg/policy.conf","./cfg/policy.conf","cfg//policy.conf","cfg/../cfg/policy.conf","cfg/","cfg","missing","cfg/bad name"})assert(!load(path).value);
    assert(!load(std::string("cfg/policy.conf\0more",20)).value);assert(!load_privileged_policy(anchor,"cfg/policy.conf",owner==0?1:0).value);assert(!load_privileged_policy(anchor,"cfg/policy.conf",UINT32_MAX).value);
    assert(symlink("cfg",(root/"alias").c_str())==0);assert(!load("alias/policy.conf").value);
    assert(symlink("policy.conf",(root/"cfg/link").c_str())==0);assert(!load("cfg/link").value);
    assert(link(file.c_str(),(root/"cfg/hard").c_str())==0);assert(!load().value);assert(unlink((root/"cfg/hard").c_str())==0);assert(load().value);
    assert(mkfifo((root/"cfg/fifo").c_str(),0600)==0);assert(!load("cfg/fifo").value);
    for(const auto mode:{0620,0602,0670,04600}){assert(chmod(file.c_str(),mode)==0);assert(!load().value);}assert(chmod(file.c_str(),0600)==0);
    assert(chmod((root/"cfg").c_str(),0770)==0);assert(!load().value);assert(chmod((root/"cfg").c_str(),0700)==0);
    assert(chmod(root.c_str(),0777)==0);assert(!load().value);assert(chmod(root.c_str(),0700)==0);
    write(std::string(96*1024+1,'x'));assert(!load().value);write(std::string(header)+"private.service 8\n");assert(!load().value);write(good);
    PrivilegedPolicyStore store;assert(!store.snapshot().value&&!store.is_current(0));auto first=store.refresh(anchor,"cfg/policy.conf",owner);assert(first.value&&first.value->generation==1&&store.is_current(1));
    const auto copy=*first.value;auto second=store.refresh(anchor,"cfg/policy.conf",owner);assert(second.value&&second.value->generation==2&&!store.is_current(1)&&store.is_current(2));
    assert(chmod(file.c_str(),0666)==0);assert(!store.refresh(anchor,"cfg/policy.conf",owner).value&&!store.snapshot().value&&!store.is_current(2));assert(copy.rules.size()==2&&copy.generation==1);
    write(good);auto recovered=store.refresh(anchor,"cfg/policy.conf",owner);assert(recovered.value&&recovered.value->generation==4);store.invalidate();assert(!store.snapshot().value&&!store.is_current(4));
    assert(unlink(file.c_str())==0);assert(!store.refresh(anchor,"cfg/policy.conf",owner).value);write(std::string(header));assert(store.refresh(anchor,"cfg/policy.conf",owner).value->rules.empty());
    assert(close(anchor)==0);assert(fds()==initial);alarm(0);
}
#endif
}
int main(){
    assert(parse_privileged_policy(header).value->empty());const auto parsed=parse_privileged_policy(good);assert(parsed.value&&parsed.value->size()==2&&parsed.value->front().operations==7&&parsed.value->back().operations==0);
    for(unsigned i=0;i<8;++i){const auto single=parse_privileged_policy(std::string(header)+"private.service "+std::to_string(i)+"\n");assert(single.value&&single.value->front().operations==i);}
    for(const auto* bad:{"","PCINSIGHT-SERVICE-POLICY 2\n","PCINSIGHT-SERVICE-POLICY 1\r\n","PCINSIGHT-SERVICE-POLICY 1","PCINSIGHT-SERVICE-POLICY 1\n\n"})assert(!parse_privileged_policy(bad).value);
    for(const auto* rule:{"private.service 8\n","private.service -1\n","private.service 01\n","private.service 1 \n","private.service\t1\n","private.service 1\r\n","../private.service 1\n","*.service 1\n","private.service 1\nprivate.service 2\n","private.service 1\n# comment\n"})assert(!parse_privileged_policy(std::string(header)+rule).value);
    auto nul=good;nul.push_back('\0');assert(!parse_privileged_policy(nul).value);
    std::string many=header;for(unsigned i=0;i<256;++i)many+="private"+std::to_string(i)+".service 7\n";assert(parse_privileged_policy(many).value->size()==256);many+="extra.service 0\n";assert(!parse_privileged_policy(many).value);
    assert(!load_privileged_policy(-1,"policy.conf").value);
#ifdef __linux__
    native();
#else
    assert(!load_privileged_policy(1,"policy.conf").value);
#endif
}
