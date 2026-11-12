#include "pcinsight/startup_store.hpp"
#include <cerrno>
#include <csignal>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string_view>
#include <thread>
#include <fcntl.h>
#include <poll.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
using namespace pcinsight;using namespace std::chrono_literals;namespace fs=std::filesystem;
namespace {
void require(bool value,const char* message){if(!value)throw std::runtime_error(message);}
struct Child {pid_t pid=-1;int output=-1,input=-1;~Child(){if(input>=0)::close(input);if(output>=0)::close(output);if(pid>0){::kill(pid,SIGKILL);while(::waitpid(pid,nullptr,0)<0&&errno==EINTR){}}}};
void wait(Child& child,int expected){const auto end=std::chrono::steady_clock::now()+8s;int status=0;
    while(std::chrono::steady_clock::now()<end){const auto done=::waitpid(child.pid,&status,WNOHANG);if(done==child.pid){child.pid=-1;require(WIFEXITED(status)&&WEXITSTATUS(status)==expected,"unexpected recorder/fixture exit");return;}require(done==0||(done<0&&errno==EINTR),"waitpid");std::this_thread::sleep_for(1ms);}throw std::runtime_error("recorder timeout");}
void spawn(Child& child,const char* executable,std::vector<std::string> args){int output[2];require(::pipe2(output,O_CLOEXEC)==0,"stdout pipe");
    std::vector<char*> argv;argv.push_back(const_cast<char*>(executable));for(auto& a:args)argv.push_back(a.data());argv.push_back(nullptr);
    child.pid=::fork();require(child.pid>=0,"recorder fork");if(!child.pid){::close(output[0]);if(::dup2(output[1],STDOUT_FILENO)<0)::_exit(120);::close(output[1]);::execv(executable,argv.data());::_exit(121);}::close(output[1]);child.output=output[0];}
void ready(Child& child){std::string text;const auto end=std::chrono::steady_clock::now()+5s;
    while(std::chrono::steady_clock::now()<end){pollfd p{child.output,POLLIN,0};const auto rc=::poll(&p,1,100);if(rc<0&&errno==EINTR)continue;require(rc>=0,"ready poll");if(!rc)continue;
        char buffer[512];const auto n=::read(child.output,buffer,sizeof buffer);require(n>0,"recorder exited before readiness");text.append(buffer,static_cast<std::size_t>(n));require(text.size()<4096,"readiness output bound");if(text.find("Recorder ready:")!=text.npos)return;}throw std::runtime_error("recorder readiness timeout");}
std::string bytes(const fs::path& p){std::ifstream f(p,std::ios::binary);return {std::istreambuf_iterator<char>(f),{}};}
}
int main(int argc,char** argv){
    if(argc==4&&std::string_view(argv[1])=="--held-child"){
        const auto input=std::atoi(argv[2]),output=std::atoi(argv[3]);if(::write(output,"r",1)!=1)return 120;::close(output);char c;while(::read(input,&c,1)>0){}::close(input);return 0;}
    try{require(argc==2,"recorder executable argument");::alarm(20);::signal(SIGPIPE,SIG_IGN);
        char pattern[]="/tmp/pcinsight-recorder-XXXXXX";const auto directory=::mkdtemp(pattern);require(directory,"private directory");const fs::path root=directory;
        const auto config=root/"config",system=root/"system",entry=config/"autostart"/"private.desktop",archive=root/"capture.bin",corrupt=root/"corrupt.bin";
        fs::create_directories(entry.parent_path());fs::create_directories(system/"autostart");
        struct Cleanup {fs::path root,entry,archive,corrupt;~Cleanup(){::unlink(archive.c_str());::unlink(corrupt.c_str());::unlink(entry.c_str());::rmdir(entry.parent_path().c_str());::rmdir((root/"config").c_str());::rmdir((root/"system"/"autostart").c_str());::rmdir((root/"system").c_str());::rmdir(root.c_str());}} cleanup{root,entry,archive,corrupt};
        {std::ofstream out(entry);out<<"[Desktop Entry]\nType=Application\nName=Private late fixture\nExec=/never-execute-this\n";require(out.good(),"private desktop fixture");}
        const auto original=bytes(entry);require(::setenv("XDG_CONFIG_HOME",config.c_str(),1)==0&&::setenv("XDG_CONFIG_DIRS",system.c_str(),1)==0,"private XDG");
        ::unsetenv("GIO_LAUNCHED_DESKTOP_FILE");::unsetenv("GIO_LAUNCHED_DESKTOP_FILE_PID");
        Child recorder;spawn(recorder,argv[1],{"--accept-estimated-gio",archive.string(),"private-early-host","1200"});ready(recorder);
        // The capture host is ALREADY running before this app identity exists.
        int input[2],output[2];require(::pipe2(input,O_CLOEXEC)==0&&::pipe2(output,O_CLOEXEC)==0,"fixture pipes");Child app;
        const auto in=std::to_string(input[0]),out=std::to_string(output[1]);app.pid=::fork();require(app.pid>=0,"fixture fork");
        if(!app.pid){::close(input[1]);::close(output[0]);if(::fcntl(input[0],F_SETFD,0)<0||::fcntl(output[1],F_SETFD,0)<0)::_exit(120);
            const auto pid=std::to_string(::getpid());if(::setenv("GIO_LAUNCHED_DESKTOP_FILE",entry.c_str(),1)||::setenv("GIO_LAUNCHED_DESKTOP_FILE_PID",pid.c_str(),1))::_exit(120);
            ::execl("/proc/self/exe","/proc/self/exe","--held-child",in.c_str(),out.c_str(),static_cast<char*>(nullptr));::_exit(121);}
        ::close(input[0]);::close(output[1]);app.input=input[1];pollfd p{output[0],POLLIN,0};require(::poll(&p,1,5000)>0,"fixture ready timeout");char b;require(::read(output[0],&b,1)==1,"fixture ready");::close(output[0]);
        const auto app_pid=app.pid;wait(recorder,0);auto loaded=StartupImpactStore::load(archive);require(loaded&&loaded.archive->observations.size()==1,"late app archive");
        const auto& row=loaded.archive->observations.front();require(row.pid==static_cast<std::uint32_t>(app_pid)&&row.startup_entry_id==entry.string()&&row.closed&&row.sample_count>=2&&row.measurement.has_value(),"late app identity/measurement");
        require(loaded.archive->diagnostics.has_value(),"recorder diagnostics not persisted");
        const auto& diagnostics=*loaded.archive->diagnostics;
        require(diagnostics.requested_duration==1200ms&&diagnostics.scan_interval==250ms&&diagnostics.elapsed>=1200ms&&diagnostics.scans>0&&diagnostics.registered==1&&!diagnostics.exit_disk_enabled&&!diagnostics.automatic_startup_origin.value,"native capture diagnostic content");
        require(!row.window_complete&&!row.sampling_gap&&!row.measurement->impact.value,"late arrival incorrectly certified full window");
        require(row.measurement->cpu_time.value.has_value()&&bytes(entry)==original,"real counters or unchanged startup fixture");
        struct stat info{};require(::stat(archive.c_str(),&info)==0&&(info.st_mode&0777)==0600,"archive private permissions");
        ::close(app.input);app.input=-1;wait(app,0);
        const auto saved=bytes(archive);
        for(const auto bad:{"0","abc","3600001"}){Child c;spawn(c,argv[1],{"--accept-estimated-gio",archive.string(),"invalid",bad});wait(c,2);require(bytes(archive)==saved,"invalid request overwrote capture");}
        {Child c;spawn(c,argv[1],{"--no-consent",archive.string(),"invalid","10"});wait(c,2);require(bytes(archive)==saved,"missing consent overwrote capture");}
        {std::ofstream f(corrupt);f<<"preserve-foreign-file";} {Child c;spawn(c,argv[1],{"--accept-estimated-gio",corrupt.string(),"invalid","10"});wait(c,4);require(bytes(corrupt)=="preserve-foreign-file","foreign capture overwritten");}
        {Child c;spawn(c,argv[1],{"--accept-estimated-gio",archive.string(),"empty","10"});wait(c,0);loaded=StartupImpactStore::load(archive);require(loaded&&loaded.archive->observations.empty(),"empty capture not represented honestly");}
        require(loaded.archive->diagnostics&&loaded.archive->diagnostics->registered==0&&loaded.archive->diagnostics->scans>0,"empty capture diagnostics missing");
        require(bytes(entry)==original,"startup entry mutated");::alarm(0);
        std::cout<<"native recorder host ready BEFORE child launch; late app discovered/partial real counters/archive0600; consent/invalid/foreign preserved; no real login activated\n";
        return 0;
    }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
