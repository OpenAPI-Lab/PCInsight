#include "../src/linux_openrc_control.hpp"
#include <cassert>
#include <filesystem>
#include <fstream>
#ifdef __linux__
#include <fcntl.h>
#include <sys/prctl.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <cstring>
#include <csignal>
extern char** environ;
#endif
using namespace pcinsight;
using namespace pcinsight::detail;
namespace fs=std::filesystem;
int main(int argc,char** argv){
#ifdef __linux__
    // PRIVATE MODEL of the openrc-run argv ABI; not an OpenRC implementation.
    if(argc==3){
        assert(prctl(PR_GET_NO_NEW_PRIVS,0,0,0,0)==1);
        int environment=0;for(auto p=environ;*p;++p)++environment;assert(environment==3);
        assert(!getenv("PCINSIGHT_POISON")&&!getenv("RC_SVCNAME")&&!getenv("LD_PRELOAD"));
        assert(std::string(getenv("PATH"))=="/usr/sbin:/usr/bin:/sbin:/bin");
        for(int fd=3;fd<512;++fd)assert(fcntl(fd,F_GETFD)<0);
        assert(fcntl(10000,F_GETFD)<0);
        std::ifstream input(argv[1]);std::string mode;std::getline(input,mode);assert(input&&fs::path(argv[1]).is_absolute());
        assert(fs::current_path()=="/");
        if(mode=="slow"){for(;;)pause();}
        return !strcmp(argv[2],"start")?3:!strcmp(argv[2],"stop")?7:!strcmp(argv[2],"restart")?11:99;
    }
    alarm(20);
    const auto fd_count=[](){std::size_t count=0;for(const auto& ignored:fs::directory_iterator("/proc/self/fd")){(void)ignored;++count;}return count;};
    const auto initial_fds=fd_count();
#endif
    OpenrcControlOptions options;
    auto result=dispatch_openrc_control(options,ServiceAction::start);assert(!result.result&&!result.child&&!result.runtime_may_have_executed);
    options.enabled=true;result=dispatch_openrc_control(options,ServiceAction::start);assert(!result.result&&!result.child);
    options.explicitly_requested=true;result=dispatch_openrc_control(options,ServiceAction(987));assert(result.result.error==ActionError::invalid_argument&&!result.child);
#ifdef __linux__
    char pattern[]="/tmp/pcinsight-openrc-control-XXXXXX";const auto raw=mkdtemp(pattern);assert(raw);const fs::path root(raw);
    const auto runtime=root/"openrc-run-model",script=root/"private-service";
    fs::copy_file("/proc/self/exe",runtime);assert(!chmod(runtime.c_str(),0700));
    auto write_script=[&](const char* text){std::ofstream out(script);out<<text<<'\n';out.close();assert(out);assert(!chmod(script.c_str(),0600));};write_script("quick");
    options.owner_uid=geteuid();options.runtime_path=runtime.string();options.script_path=script.string();
    result=dispatch_openrc_control(options,ServiceAction::start);assert(!result.result&&!result.child); // sticky /tmp not implicitly accepted
    options.allow_sticky_ancestors=true;
    int sentinel=open("/dev/null",O_RDWR);assert(sentinel>=0);rlimit fd_limit{};assert(!getrlimit(RLIMIT_NOFILE,&fd_limit));
    const int sentinel_number=fd_limit.rlim_cur>10000?10000:64;
    int high_sentinel=fcntl(sentinel,F_DUPFD,sentinel_number);assert(high_sentinel>=sentinel_number);
    setenv("PCINSIGHT_POISON","must not leak",1);setenv("RC_SVCNAME","not-the-selected-service",1);
    const int expected[]={3,7,11};int index=0;
    for(auto action:{ServiceAction::start,ServiceAction::stop,ServiceAction::restart}){
        result=dispatch_openrc_control(options,action);assert(result.result&&result.child&&result.runtime_may_have_executed);
        auto status=result.child->wait(2000);assert(status.value&&status.value->exited&&status.value->exit_code==expected[index++]&&!status.value->signal);
        auto historical=result.child->wait(0);assert(historical.value&&historical.value->exited);
        assert(!result.child->terminate_runner(true,true));
    }
    unsetenv("PCINSIGHT_POISON");unsetenv("RC_SVCNAME");
    assert(fcntl(high_sentinel,F_GETFD)>=0);close(high_sentinel);close(sentinel);
    auto denied=[&]{auto d=dispatch_openrc_control(options,ServiceAction::start);assert(!d.result&&!d.child&&!d.runtime_may_have_executed);};
    chmod(script.c_str(),0622);denied();chmod(script.c_str(),0600);
    fs::rename(script,root/"backup");fs::create_symlink(root/"backup",script);denied();fs::remove(script);fs::rename(root/"backup",script);
    fs::create_hard_link(script,root/"hard");denied();fs::remove(root/"hard");
    auto original=options.script_path;options.script_path=root.string()+"/../private-service";denied();options.script_path=original;
    auto original_runtime=options.runtime_path;options.runtime_path=script.string();denied();options.runtime_path=original_runtime;
    options.owner_uid=geteuid()==0?1:0;denied();options.owner_uid=geteuid();
    options.script_path.push_back('\0');denied();options.script_path=original;
    fs::rename(script,root/"backup");assert(!mkfifo(script.c_str(),0600));denied();fs::remove(script);fs::rename(root/"backup",script);
    write_script("slow");result=dispatch_openrc_control(options,ServiceAction::restart);assert(result.result&&result.child);
    auto pending=result.child->wait(10);assert(pending.value&&!pending.value->exited&&pending.value->timed_out);
    const auto other=fork();assert(other>=0);if(!other){auto copied=result.child->wait(0);_exit(copied.value?1:0);}int other_status{};assert(waitpid(other,&other_status,0)==other&&WIFEXITED(other_status)&&WEXITSTATUS(other_status)==0);
    assert(!result.child->terminate_runner(true,false));assert(result.child->terminate_runner(true,true));
    auto killed=result.child->wait(2000);assert(killed.value&&killed.value->signal==SIGKILL&&!killed.value->exit_code);
    auto invalid=result.child->wait(30001);assert(!invalid.value);
    result=dispatch_openrc_control(options,ServiceAction::stop);assert(result.result&&result.child);
    assert(result.child->wait(10).value->timed_out);assert(result.child->terminate_runner(false,true));
    auto terminated=result.child->wait(2000);assert(terminated.value&&terminated.value->signal==SIGTERM);
    write_script("quick");result=dispatch_openrc_control(options,ServiceAction::start);assert(result.child);
    int stolen{};assert(waitpid(result.child->process_id(),&stolen,0)>0);
    assert(!result.child->wait(0).value); // Fault injection: never reap a reused bare PID.
    // ELF-looking malformed file: native exec error, no shell fallback; still reap.
    const auto bad=root/"bad-runtime";{std::ofstream out(bad,std::ios::binary);out.write("\177ELFbad",7);}chmod(bad.c_str(),0700);options.runtime_path=bad.string();
    result=dispatch_openrc_control(options,ServiceAction::start);assert(!result.result&&result.child&&result.runtime_may_have_executed);
    auto failed=result.child->wait(2000);assert(failed.value&&failed.value->exit_code==126);
    result={};assert(fd_count()==initial_fds);
    fs::remove_all(root);
#else
    options.runtime_path="/unqueried";options.script_path="/unqueried";result=dispatch_openrc_control(options,ServiceAction::start);assert(result.result.error==ActionError::not_supported&&!result.child);
#endif
}
