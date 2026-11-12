#include "pcinsight/account_management.hpp"
#include <cassert>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <thread>
#ifdef __linux__
#include <cerrno>
#include <sys/wait.h>
#endif
using namespace pcinsight;
namespace fs=std::filesystem;
std::string utf8(const fs::path& path){const auto s=path.u8string();return {reinterpret_cast<const char*>(s.data()),s.size()};}
int main(int argc,char** argv){
    if(argc==2&&std::string(argv[1])=="--account-host-child"){
        std::ofstream out("opened.marker",std::ios::binary);out<<"private navigation accepted";out.close();return out?0:2;
    }
    AccountManagementIntegration none;
    assert(!none.capability().value&&!none.request_open().host_invoked);
    unsigned queries=0,opens=0;
    Metric<bool> available{true,Support::available,"host account view"};
    auto query=[&]{++queries;return available;};
    auto open=[&]{++opens;return ActionResult{ActionError::none,"navigation accepted"};};
    AccountManagementIntegration integration(query,open);
    assert(queries==0&&opens==0); // Construction never opens UI.
    assert(integration.capability().value==true&&queries==1&&opens==0);
    const auto accepted=integration.request_open();assert(accepted&&accepted.host_invoked&&opens==1&&queries==2);
    for(int bad=0;bad<7;++bad){
        available={true,Support::available,"host"};
        if(bad==0)available.value=false;if(bad==1)available.value.reset();
        if(bad==2)available.support=Support::estimated;if(bad==3)available.support=Support::unsupported;
        if(bad==4)available.support=Support::permission_required;
        if(bad==5)available.note.assign(4097,'x');if(bad==6)available.note=std::string("bad\0note",8);
        const auto result=integration.request_open();assert(!result&&!result.host_invoked&&opens==1);
        assert(result.action.error==(bad==4?ActionError::permission_denied:ActionError::not_supported));
    }
    assert(!AccountManagementIntegration({},open).request_open().host_invoked);
    assert(!AccountManagementIntegration(query,{}).request_open().host_invoked);
    AccountManagementIntegration query_throws([]()->Metric<bool>{throw std::runtime_error("private host details");},open);
    assert(!query_throws.request_open().host_invoked&&opens==1);
    available={true,Support::available,{}};
    AccountManagementIntegration open_throws(query,[]()->ActionResult{throw 7;});
    const auto thrown=open_throws.request_open();assert(!thrown&&thrown.host_invoked&&thrown.action.error==ActionError::system_error);
    for(const auto error:{ActionError::not_supported,ActionError::permission_denied,ActionError::not_found,
        ActionError::invalid_argument,ActionError::system_error,ActionError::stale_target}){
        AccountManagementIntegration rejects(query,[=]{return ActionResult{error,"host rejected"};});
        const auto result=rejects.request_open();assert(!result&&result.host_invoked&&result.action.error==error);
    }
    // Native end-to-end host route, NOT a system account manager: only a private
    // helper process writes a marker in its new directory. No real users/UI edited.
#if defined(__linux__) || defined(_WIN32)
    fs::path temp;const auto prefix="pcinsight-account-host-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
    for(unsigned i=0;i<32;++i){auto candidate=fs::temp_directory_path()/(prefix+"-"+std::to_string(i));
        std::error_code ec;if(fs::create_directory(candidate,ec)){temp=std::move(candidate);break;}}
    assert(!temp.empty());
    struct Cleanup{fs::path directory;~Cleanup(){std::error_code ec;fs::remove(directory/"opened.marker",ec);fs::remove(directory,ec);}} cleanup{temp};
    const auto executable=fs::canonical(fs::absolute(argv[0]));
    auto actions=make_system_actions();
    AccountManagementIntegration native(query,[&]{
        LaunchRequest r;r.executable=utf8(executable);r.arguments={"--account-host-child"};r.working_directory=utf8(temp);
        return actions->launch_process(r);
    });
    const auto child=native.request_open();assert(child&&child.host_invoked&&child.action.process_id>0);
#ifdef __linux__
    int status=0;pid_t waited;do{waited=waitpid(static_cast<pid_t>(child.action.process_id),&status,0);}while(waited<0&&errno==EINTR);
    assert(waited==static_cast<pid_t>(child.action.process_id)&&WIFEXITED(status)&&WEXITSTATUS(status)==0);
#endif
    const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(5);
    std::string text;
    while(std::chrono::steady_clock::now()<deadline){
        std::ifstream input(temp/"opened.marker",std::ios::binary);std::getline(input,text);
        if(text=="private navigation accepted")break;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    assert(text=="private navigation accepted");
    std::cout<<"Host navigation integration PASS: private child accepted request; no system account UI or account changes\n";
#endif
}
