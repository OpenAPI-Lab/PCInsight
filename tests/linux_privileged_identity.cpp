#include "../src/linux_privileged_identity.hpp"
#include <cassert>
#include <iostream>
#ifdef __linux__
#include <dlfcn.h>
#include <filesystem>
#include <dirent.h>
#include <unistd.h>
#include <cstdlib>
namespace fs=std::filesystem;
std::size_t fds(){auto d=::opendir("/proc/self/fd");assert(d);std::size_t n=0;while(auto e=::readdir(d))if(e->d_name[0]!='.')++n;::closedir(d);return n;}
#endif
using namespace pcinsight;
using namespace pcinsight::detail;
int main(int argc,char**){
    PrivilegedIdentityConfig c;assert(!observe_native_privileged_identity(nullptr,c).value);assert(!observe_native_privileged_identity(reinterpret_cast<void*>(1),c).value);
#ifdef __linux__
    if(argc>1){::alarm(20);const auto initial=fds();char pattern[]="/tmp/pcinsight-native-identity-XXXXXX";auto p=::mkdtemp(pattern);assert(p);const fs::path root=p;struct Cleanup{fs::path p;~Cleanup(){std::error_code e;fs::remove_all(p,e);}} cleanup{root};void* library=::dlopen("libsystemd.so.0",RTLD_NOW|RTLD_LOCAL);assert(library);auto create=reinterpret_cast<void*(*)(const char*,const char*)>(::dlsym(library,"identity_fixture_create"));auto destroy=reinterpret_cast<void(*)(void*)>(::dlsym(library,"identity_fixture_destroy"));auto calls=reinterpret_cast<unsigned(*)(void*)>(::dlsym(library,"identity_fixture_calls"));assert(create&&destroy&&calls);
        c.socket_path=(root/"bus").string();c.bus_uid=::geteuid();c.authority_uid=100;c.connection_generation=1;c.policy_generation=1;
        for(const auto mode:{"ok","not-current","bad-sender","wrong-destination","wrong-type","wrong-address","not-client","disconnected","unbounded","missing-authority","changed-id","changed-owner","changed-caller","changed-authority-uid","spoof-reply","bad-signature","truncated"}){void* message=create(c.socket_path.c_str(),mode);auto result=observe_native_privileged_identity(message,c);if(std::string(mode)=="ok"){if(!result.value)std::cerr<<result.note<<'\n';assert(result.value&&result.value->caller_uid==1000&&result.value->authority_uid==100&&result.value->caller_unique_name==":1.23"&&result.value->authority_unique_owner==":1.7"&&calls(message)==8);}else assert(!result.value);destroy(message);}
        void* message=create(c.socket_path.c_str(),"ok");auto bad=c;bad.bus_uid=(*c.bus_uid==0?1:0);assert(!observe_native_privileged_identity(message,bad).value&&calls(message)==0);bad=c;bad.socket_path=(root/"other").string();assert(!observe_native_privileged_identity(message,bad).value&&calls(message)==0);bad=c;bad.authority_uid=101;assert(!observe_native_privileged_identity(message,bad).value);destroy(message);
        ::dlclose(library);assert(fds()==initial);::alarm(0);
    }
#else
    (void)argc;c.bus_uid=0;c.authority_uid=100;c.connection_generation=c.policy_generation=1;assert(!observe_native_privileged_identity(reinterpret_cast<void*>(1),c).value);
#endif
    std::cout<<"Native identity argument/fallback or isolated loader+real Unix peer/endpoint/metadata/revalidation/FD tests passed; NOT real bus/polkit authorization\n";
}
