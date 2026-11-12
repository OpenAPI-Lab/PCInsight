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
using namespace pcinsight;using namespace pcinsight::detail;
int main(int argc,char**){
    PrivilegedServiceRequest request{"private.service",ServiceAction::start,true};std::vector<PrivilegedServiceRule> rules{{request.unit,7}};PrivilegedIdentityConfig c;PrivilegedAuthorizationContext context;
    assert(!check_native_privileged_authorization(nullptr,c,context,request,rules).value);
#ifdef __linux__
    if(argc>1){::alarm(25);const auto initial=fds();char pattern[]="/tmp/pcinsight-native-auth-XXXXXX";auto p=::mkdtemp(pattern);assert(p);fs::path root=p;struct Cleanup{fs::path p;~Cleanup(){std::error_code e;fs::remove_all(p,e);}} cleanup{root};void* library=::dlopen("libsystemd.so.0",RTLD_NOW|RTLD_LOCAL);assert(library);auto create=reinterpret_cast<void*(*)(const char*,const char*)>(::dlsym(library,"identity_fixture_create"));auto destroy=reinterpret_cast<void(*)(void*)>(::dlsym(library,"identity_fixture_destroy"));auto authcalls=reinterpret_cast<unsigned(*)(void*)>(::dlsym(library,"identity_fixture_authcalls"));assert(create&&destroy&&authcalls);
        c.socket_path=(root/"bus").string();c.bus_uid=::geteuid();c.authority_uid=100;c.connection_generation=c.policy_generation=1;
        for(const auto mode:{"auth-ok","auth-deny","auth-challenge","auth-dismissed","auth-retained","auth-temporary","auth-unknown","auth-spoof","auth-signature","auth-bad-cookie","auth-bad-bool","auth-truncated","auth-no-struct","auth-no-array","auth-bad-entry","auth-duplicate","auth-long-key","auth-long-value","auth-many","auth-payload","auth-trailing","auth-owner-change","auth-call-error","auth-build-fail","auth-interaction","auth-reply-type","auth-max-details","auth-max-payload"}){
            void* message=create(c.socket_path.c_str(),mode);auto api=native_privileged_authorization_api(message,c,request,rules,std::string(mode)=="auth-interaction");auto report=observe_privileged_authorization(request,rules,api);const std::string name=mode;
            if(name=="auth-ok"||name=="auth-unknown"||name=="auth-interaction"||name=="auth-max-details"||name=="auth-max-payload"){if(report.authorization.value!=true)std::cerr<<mode<<": "<<report.note<<'\n';assert(report.authorization.value==true&&report.state==PrivilegedAuthorizationState::authorization_observed);}
            else if(name=="auth-deny"||name=="auth-challenge"||name=="auth-dismissed"||name=="auth-retained"||name=="auth-temporary")assert(report.authorization.value==false&&report.state==PrivilegedAuthorizationState::denied);
            else assert(!report.authorization.value);
            assert(authcalls(message)==(name=="auth-build-fail"?0U:1U));destroy(message);
        }
        void* message=create(c.socket_path.c_str(),"auth-ok");auto observed=observe_native_privileged_identity(message,c);assert(observed.value);auto wrong=*observed.value;wrong.caller_unique_name=":1.99";assert(!check_native_privileged_authorization(message,c,wrong,request,rules).value&&authcalls(message)==0);auto no_intent=request;no_intent.explicitly_requested=false;assert(!check_native_privileged_authorization(message,c,*observed.value,no_intent,rules).value&&authcalls(message)==0);destroy(message);
        message=create(c.socket_path.c_str(),"auth-ok");auto frozen_api=native_privileged_authorization_api(message,c,request,rules);auto different=request;different.unit="other.service";auto expanded=rules;expanded.push_back({different.unit,7});auto mismatch=observe_privileged_authorization(different,expanded,frozen_api);assert(!mismatch.authorization.value&&authcalls(message)==0);destroy(message);
        ::dlclose(library);assert(fds()==initial);::alarm(0);
    }
#else
    (void)argc;
#endif
    std::cout<<"Native authorization protocol/subject/closed target/flags/correlation/bounds/revalidation/coordinator checks passed; isolated ABI fixture, NO host polkit/dialogue/helper/execution\n";
}
