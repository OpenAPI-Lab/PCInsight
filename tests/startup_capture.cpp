#include "pcinsight/startup_store.hpp"
#include <cerrno>
#include <csignal>
#include <cstdio>
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
#include <sys/wait.h>
#include <unistd.h>
using namespace pcinsight;
using namespace std::chrono_literals;
namespace fs=std::filesystem;
void check(bool ok) {if(!ok)throw std::runtime_error("startup capture fixture check failed");}
struct Child {
    pid_t pid{-1};int control{-1};
    ~Child(){if(control>=0)close(control);if(pid>0){kill(pid,SIGKILL);while(waitpid(pid,nullptr,0)<0 && errno==EINTR){}}}
};
std::string read_file(const fs::path& path) {std::ifstream in(path,std::ios::binary);return {std::istreambuf_iterator<char>(in),{}};}
int run(const char* executable,const fs::path& path,const std::string& id,const std::string& duration) {
    const auto name=path.string();Child child;child.pid=fork();check(child.pid>=0);
    if(!child.pid){execl(executable,executable,name.c_str(),id.c_str(),duration.c_str(),static_cast<char*>(nullptr));_exit(120);}
    const auto deadline=std::chrono::steady_clock::now()+10s;int status{};
    while(std::chrono::steady_clock::now()<deadline){
        const auto done=waitpid(child.pid,&status,WNOHANG);
        if(done==child.pid){child.pid=-1;check(WIFEXITED(status));return WEXITSTATUS(status);}
        check(done==0 || (done<0 && errno==EINTR));std::this_thread::sleep_for(1ms);
    }
    throw std::runtime_error("startup capture fixture timed out");
}
int main(int argc,char** argv) {
    if(argc==4 && std::string_view(argv[1])=="--held-child") {
        const auto input=std::atoi(argv[2]),output=std::atoi(argv[3]);
        if(write(output,"r",1)!=1)return 120;
        close(output);char byte{};while(read(input,&byte,1)>0){};close(input);return 0;
    }
    try {
        check(argc==2);char name[]="/tmp/pcinsight-capture-XXXXXX";const auto* temp=mkdtemp(name);check(temp);
        const fs::path root=temp;
        struct Cleanup {fs::path path;~Cleanup(){std::error_code ec;fs::remove_all(path,ec);}} cleanup{root};
        const auto config=root/"config",desktop=config/"autostart/app.desktop",system=root/"system";
        fs::create_directories(desktop.parent_path());fs::create_directories(system/"autostart");
        {std::ofstream out(desktop);out<<"[Desktop Entry]\nType=Application\nName=Private capture fixture\nExec=/never-run\n";check(out.good());}
        const auto desktop_bytes=read_file(desktop);
        check(setenv("XDG_CONFIG_HOME",config.c_str(),1)==0 && setenv("XDG_CONFIG_DIRS",system.c_str(),1)==0);
        check(unsetenv("GIO_LAUNCHED_DESKTOP_FILE")==0 && unsetenv("GIO_LAUNCHED_DESKTOP_FILE_PID")==0);
        int control[2],ready[2];check(pipe2(control,O_CLOEXEC)==0 && pipe2(ready,O_CLOEXEC)==0);
        const auto input=std::to_string(control[0]),output=std::to_string(ready[1]);Child fixture;fixture.pid=fork();check(fixture.pid>=0);
        if(!fixture.pid) {
            close(control[1]);close(ready[0]);char marker[32];std::snprintf(marker,sizeof marker,"%ld",static_cast<long>(getpid()));
            if(fcntl(control[0],F_SETFD,0)<0 || fcntl(ready[1],F_SETFD,0)<0 ||
                setenv("GIO_LAUNCHED_DESKTOP_FILE",desktop.c_str(),1)!=0 || setenv("GIO_LAUNCHED_DESKTOP_FILE_PID",marker,1)!=0)_exit(120);
            execl("/proc/self/exe","/proc/self/exe","--held-child",input.c_str(),output.c_str(),static_cast<char*>(nullptr));_exit(121);
        }
        close(control[0]);close(ready[1]);fixture.control=control[1];
        pollfd item{ready[0],POLLIN,0};check(poll(&item,1,5000)>0 && (item.revents&POLLIN));char byte{};check(read(ready[0],&byte,1)==1);close(ready[0]);
        const auto archive_path=root/"capture.bin";
        check(run(argv[1],archive_path,"private-capture","300")==0);
        auto loaded=StartupImpactStore::load(archive_path);check(static_cast<bool>(loaded));
        check(loaded.archive->capture_id=="private-capture" && !loaded.archive->native_context.empty());
        check(loaded.archive->observations.size()==1);
        const auto& row=loaded.archive->observations.front();
        check(row.pid==static_cast<std::uint32_t>(fixture.pid) && row.startup_entry_id==desktop.string());
        check(row.closed && row.window_complete && !row.sampling_gap && row.sample_count>=2 && row.measurement.has_value());
        check(row.measurement->cpu_time.value.has_value() && row.measurement->cpu_time.support==Support::estimated);
        check(read_file(desktop)==desktop_bytes); // never modified or Exec-launched
        const auto original=read_file(archive_path);
        for(const auto duration:{"0","-1","3600001","abc","10x","999999999999999999999999"})
            check(run(argv[1],archive_path,"private-capture",duration)==2 && read_file(archive_path)==original);
        check(run(argv[1],archive_path,"","10")==2 && read_file(archive_path)==original);
        const auto corrupt=root/"corrupt.bin";{std::ofstream out(corrupt);out<<"keep-corrupt-file";check(out.good());}
        check(run(argv[1],corrupt,"private-capture","10")==4 && read_file(corrupt)=="keep-corrupt-file");
        check(run(argv[1],root/"missing"/"archive.bin","private-capture","10")==4);
        // Drop our own marker-bearing fixture: next capture is empty, not zero impact.
        close(fixture.control);fixture.control=-1;int status{};check(waitpid(fixture.pid,&status,0)==fixture.pid);fixture.pid=-1;
        check(WIFEXITED(status) && WEXITSTATUS(status)==0);
        check(run(argv[1],archive_path,"empty-capture","10")==0);
        loaded=StartupImpactStore::load(archive_path);check(loaded && loaded.archive->capture_id=="empty-capture" && loaded.archive->observations.empty());
        check(read_file(desktop)==desktop_bytes);
        return 0;
    }catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
}
