#include "pcinsight/linux_openrc_control.hpp"
#include <cassert>
#include <filesystem>
#include <fstream>
#ifdef __linux__
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <csignal>
#endif
using namespace pcinsight;
namespace fs=std::filesystem;
int main(int argc,char** argv){
    LinuxOpenrcControlConfig config;
    auto selection=LinuxOpenrcService::open(config,"anything");assert(!selection.value);
    config.enabled=true;
    config.init_directory="/not-a-real-init-directory";
    for(const auto& name:{"",".","..","-x","a/b","a;stop","a\nb"}){
        auto bad=LinuxOpenrcService::open(config,name);
        assert(!bad.value&&bad.note=="Invalid literal OpenRC service name");
    }
    auto nul=LinuxOpenrcService::open(config,std::string("a\0b",3));
    assert(!nul.value&&nul.note=="Invalid literal OpenRC service name");
#ifdef __linux__
    alarm(20);assert(argc==2);
    auto fd_count=[](){std::size_t count=0;for(const auto& e:fs::directory_iterator("/proc/self/fd")){(void)e;++count;}return count;};
    const auto before=fd_count();
    char pattern[]="/tmp/pcinsight-openrc-public-XXXXXX";const auto raw=mkdtemp(pattern);assert(raw);const fs::path root(raw);
    const auto runtime=root/"runtime",init=root/"init.d",script=init/"private-service";
    fs::create_directory(init);assert(!chmod(init.c_str(),0700));fs::copy_file(argv[1],runtime);assert(!chmod(runtime.c_str(),0700));
    auto write=[&](const char* value){std::ofstream out(script);out<<value<<'\n';out.close();assert(out);assert(!chmod(script.c_str(),0600));};write("quick");
    config.owner_uid=geteuid();config.runtime_path=runtime.string();config.init_directory=init.string();
    assert(!LinuxOpenrcService::open(config,"private-service").value);
    config.allow_sticky_ancestors=true;
    const auto configured_init=config.init_directory;config.init_directory.clear();assert(!LinuxOpenrcService::open(config,"private-service").value);config.init_directory=configured_init;
    selection=LinuxOpenrcService::open(config,"private-service");assert(selection.value);
    auto service=*selection.value;assert(service->name()=="private-service");
    assert(!service->request(ServiceAction::start,false).runner);
    auto invalid=service->request(ServiceAction(999),true);assert(invalid.result.error==ActionError::invalid_argument&&!invalid.runner);
    // Read-only selection + invalid/unauthorized requests have not consumed it.
    config.runtime_path="/changed-after-selection";config.init_directory="/changed-after-selection";
    auto run=service->request(ServiceAction::start,true);assert(run.result&&run.runner&&run.runtime_may_have_executed);
    auto exit=run.runner->wait(2000);assert(exit.value&&exit.value->exit_code==3);
    assert(run.runner->process_id()!=0);
    auto duplicate=service->request(ServiceAction::start,true);assert(duplicate.result.error==ActionError::stale_target&&!duplicate.runner);
    config.runtime_path=runtime.string();config.init_directory=init.string();
    auto open=[&](){auto s=LinuxOpenrcService::open(config,"private-service");assert(s.value);return *s.value;};
    service=open();fs::rename(script,init/"old");write("replacement");
    auto stale=service->request(ServiceAction::stop,true);assert(stale.result.error==ActionError::stale_target&&!stale.runner&&!stale.runtime_may_have_executed);
    fs::remove(script);fs::rename(init/"old",script);
    assert(service->request(ServiceAction::stop,true).result.error==ActionError::stale_target); // No automatic revival.
    service=open();fs::rename(runtime,root/"old-runtime");fs::copy_file(root/"old-runtime",runtime);chmod(runtime.c_str(),0700);
    assert(service->request(ServiceAction::stop,true).result.error==ActionError::stale_target);
    fs::remove(runtime);fs::rename(root/"old-runtime",runtime);
    service=open();chmod(script.c_str(),0622);assert(service->request(ServiceAction::stop,true).result.error==ActionError::stale_target);chmod(script.c_str(),0600);
    service=open();fs::rename(init,root/"old-init");fs::create_directory(init);chmod(init.c_str(),0700);write("replacement");
    assert(service->request(ServiceAction::stop,true).result.error==ActionError::stale_target);
    fs::remove_all(init);fs::rename(root/"old-init",init);
    service=open();service->close();assert(service->request(ServiceAction::start,true).result.error==ActionError::stale_target);
    for(auto action:{ServiceAction::stop,ServiceAction::restart}){
        service=open();run=service->request(action,true);assert(run.result&&run.runner);
        // Service/selection destruction never takes ownership of the runner.
        service.reset();auto status=run.runner->wait(2000);assert(status.value&&status.value->exit_code==(action==ServiceAction::stop?7:11));
    }
    service=open();const auto child=fork();assert(child>=0);if(!child){auto d=service->request(ServiceAction::start,true);_exit(d.result.error==ActionError::stale_target&&!d.runner?0:1);}int status{};assert(waitpid(child,&status,0)==child&&WIFEXITED(status)&&WEXITSTATUS(status)==0);
    run=service->request(ServiceAction::restart,true);assert(run.runner&&run.runner->wait(2000).value->exit_code==11);
    write("slow");service=open();run=service->request(ServiceAction::start,true);assert(run.runner);
    auto pending=run.runner->wait(10);assert(pending.value&&pending.value->timed_out);
    assert(!run.runner->terminate_runner(true,false));assert(run.runner->terminate_runner(true,true));
    assert(run.runner->wait(2000).value->signal==SIGKILL);
    run={};service.reset();selection={};assert(fd_count()==before);fs::remove_all(root);
#else
    (void)argc;(void)argv;config.runtime_path="/unqueried";config.init_directory="/unqueried";assert(!LinuxOpenrcService::open(config,"private-service").value);
#endif
}
