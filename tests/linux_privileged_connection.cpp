#include "../src/linux_privileged_connection.hpp"
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
#ifdef __linux__
std::size_t fds(){auto* d=opendir("/proc/self/fd");assert(d);std::size_t n{};while(auto* e=readdir(d))if(e->d_name[0]!='.')++n;closedir(d);return n;}
void native(){alarm(20);const auto initial=fds();char temp[]="/tmp/pcinsight-connection-XXXXXX";const auto* p=mkdtemp(temp);assert(p);const std::filesystem::path root=p;
    struct Cleanup{std::filesystem::path p;~Cleanup(){std::error_code e;std::filesystem::remove_all(p,e);}} cleanup{root};
    const auto owner=std::uint32_t(geteuid());
    {const auto result=open_privileged_directory(root.string(),owner,true);assert(result.value);auto directory=*result.value;assert(directory->revalidate()&&directory->absolute_path()==root.string());const auto fd=directory->native_fd();assert(fd>=0&&(fcntl(fd,F_GETFD)&FD_CLOEXEC));
        struct stat tmp{};assert(stat("/tmp",&tmp)==0);if(tmp.st_mode&0022)assert(!open_privileged_directory(root.string(),owner).value);
        assert(!open_privileged_directory(root.string(),owner==0?1:0,true).value);
        assert(!connect_privileged_bus({directory,"missing",owner,owner}).value);
        {std::ofstream out(root/"plain");out<<"not socket";}assert(chmod((root/"plain").c_str(),0600)==0);assert(!connect_privileged_bus({directory,"plain",owner,owner}).value);
        assert(symlink("plain",(root/"alias").c_str())==0);assert(!connect_privileged_bus({directory,"alias",owner,owner}).value);assert(!open_privileged_directory((root/"alias").string(),owner,true).value);
        for(const auto* s:{"","..","../bus","bus;unix:path=other","bus,other","bus%2f","bus name"})assert(!connect_privileged_bus({directory,s,owner,owner}).value);
        assert(!connect_privileged_bus({directory,"bus",owner,owner,0}).value);assert(!connect_privileged_bus({directory,"bus",owner,owner,5000001}).value);
        assert(mkdir((root/"nested").c_str(),0700)==0);const auto nested=open_privileged_directory((root/"nested").string(),owner,true);assert(nested.value);
        assert(rename((root/"nested").c_str(),(root/"moved").c_str())==0);assert(mkdir((root/"nested").c_str(),0700)==0);assert(!(*nested.value)->revalidate());
        assert(chmod(root.c_str(),0777)==0);assert(!directory->revalidate()&&directory->native_fd()==-1);assert(!open_privileged_directory(root.string(),owner,true).value);assert(chmod(root.c_str(),0700)==0);assert(directory->revalidate());
    }
    assert(fds()==initial);alarm(0);
}
#endif
int main(){
    for(const auto* path:{"","relative","/tmp/../tmp","/tmp//other","/tmp/","/tmp/with space"})assert(!open_privileged_directory(path,0).value);
    assert(!open_privileged_directory(std::string(4097,'x'),0).value);assert(!open_privileged_directory("/",UINT32_MAX).value);assert(!connect_privileged_bus({}).value);
#ifdef __linux__
    native();
#else
    assert(!open_privileged_directory("/",0).value);
#endif
}
