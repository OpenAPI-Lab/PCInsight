#include "../src/linux_startup_candidates.hpp"
#include "pcinsight/startup_candidates.hpp"
#include "pcinsight/startup_observer.hpp"
#include "pcinsight/monitor.hpp"
#include <cassert>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <map>
#ifdef __linux__
#include <cerrno>
#include <cstdio>
#include <poll.h>
#include <sys/wait.h>
#include <unistd.h>
#endif
using namespace pcinsight;
using namespace pcinsight::detail;
namespace fs=std::filesystem;
std::string environment(const std::string& file,const std::string& pid) {
    return "GIO_LAUNCHED_DESKTOP_FILE="+file+'\0'+"GIO_LAUNCHED_DESKTOP_FILE_PID="+pid+'\0';
}
#ifdef __linux__
struct Env {
    std::map<std::string,std::optional<std::string>> old;
    void set(const char* k,const std::string& v){if(!old.contains(k)){const auto* s=getenv(k);old[k]=s?std::optional<std::string>(s):std::nullopt;}assert(setenv(k,v.c_str(),1)==0);}
    ~Env(){for(const auto& [k,v]:old){if(v)setenv(k.c_str(),v->c_str(),1);else unsetenv(k.c_str());}}
};
struct Temp {
    fs::path path;Temp(){char p[]="/tmp/pcinsight-candidate-XXXXXX";const auto* r=mkdtemp(p);assert(r);path=r;}
    ~Temp(){std::error_code ec;fs::remove_all(path,ec);}
};
struct Child {
    pid_t pid{};int control{-1};
    Child(const fs::path& file,bool direct) {
        int pipe[2],ready[2];assert(::pipe(pipe)==0 && ::pipe(ready)==0);
        const auto input=std::to_string(pipe[0]),output=std::to_string(ready[1]);const auto parent=getpid();
        pid=fork();assert(pid>=0);
        if(!pid){
            close(pipe[1]);close(ready[0]);char marker[32];std::snprintf(marker,sizeof marker,"%ld",static_cast<long>(direct?getpid():parent));
            if(setenv("GIO_LAUNCHED_DESKTOP_FILE",file.c_str(),1)!=0 || setenv("GIO_LAUNCHED_DESKTOP_FILE_PID",marker,1)!=0)_exit(120);
            char* args[]{const_cast<char*>("/proc/self/exe"),const_cast<char*>("--held-child"),const_cast<char*>(input.c_str()),const_cast<char*>(output.c_str()),nullptr};
            execv(args[0],args);_exit(121);
        }
        close(pipe[0]);close(ready[1]);control=pipe[1];
        pollfd item{ready[0],POLLIN,0};assert(poll(&item,1,5000)>0 && (item.revents&POLLIN));char value{};assert(read(ready[0],&value,1)==1);close(ready[0]);
    }
    ~Child(){if(control>=0)close(control);int status{};while(waitpid(pid,&status,0)<0 && errno==EINTR){};}
};
void write(const fs::path& p){std::ofstream f(p);f<<"[Desktop Entry]\nType=Application\nName=Fixture\nExec=/never-run\n";f.close();assert(f);}
#endif
int main(int argc,char** argv) {
#ifdef __linux__
    if(argc==4 && std::string(argv[1])=="--held-child") {
        const auto input=std::stoi(argv[2]),output=std::stoi(argv[3]);
        if(::write(output,"r",1)!=1)return 2;close(output);char value{};
        while(read(input,&value,1)>0){};return 0;
    }
#else
    (void)argc;(void)argv;
#endif
    auto match=parse_gio_launch_evidence(environment("/config/autostart/app.desktop","10"),10);
    assert(match.well_formed && match.desktop_file=="/config/autostart/app.desktop");
    assert(!parse_gio_launch_evidence(environment("/config/autostart/app.desktop","10"),11).desktop_file); // inherited marker PID
    assert(parse_gio_launch_evidence("",10).well_formed);
    auto malformed=environment("/config/app.desktop","10");malformed.pop_back();assert(!parse_gio_launch_evidence(malformed,10).well_formed);
    assert(!parse_gio_launch_evidence(environment("relative.desktop","10"),10).well_formed);
    assert(!parse_gio_launch_evidence(environment("/config/app.desktop","10")+environment("/config/app.desktop","10"),10).well_formed);
    assert(!parse_gio_launch_evidence(environment("/config/app.desktop","+10"),10).well_formed);
    assert(!parse_gio_launch_evidence(environment("/config/app.desktop","2147483648"),10).well_formed);
    assert(!parse_gio_launch_evidence(std::string(65537,'a'),10).well_formed);
    auto secret=environment("/config/app.desktop","10");secret+="SECRET=do-not-retain";secret.push_back('\0');
    assert(parse_gio_launch_evidence(secret,10).desktop_file=="/config/app.desktop");
#ifdef __linux__
    Temp temp;Env env;
    const auto config=temp.path/"config",personal=config/"autostart",system=temp.path/"system/autostart";
    fs::create_directories(personal);fs::create_directories(system);
    env.set("XDG_CONFIG_HOME",config.string());env.set("XDG_CONFIG_DIRS",system.parent_path().string());
    write(personal/"app.desktop");write(system/"app.desktop");
    Child direct(personal/"app.desktop",true);
    Child inherited(personal/"app.desktop",false);
    Child shadowed(system/"app.desktop",true);
    Monitor monitor;SampleOptions options;options.include_startup_entries=true;
    options.include_disks=options.include_networks=options.include_gpus=false;
    auto snapshot=monitor.sample(options);
    std::vector<ProcessInfo> own;
    for(const auto& p:snapshot.processes)if(p.pid==static_cast<std::uint32_t>(direct.pid) || p.pid==static_cast<std::uint32_t>(inherited.pid) || p.pid==static_cast<std::uint32_t>(shadowed.pid))own.push_back(p);
    assert(own.size()==3);snapshot.processes=own;
    auto report=discover_startup_process_candidates(snapshot);
    assert(report.error==ActionError::none && report.complete && report.candidates.size()==1 && report.examined==3);
    const auto& candidate=report.candidates[0];assert(candidate.startup_entry_id==(personal/"app.desktop").string());
    assert(candidate.baseline.pid==static_cast<std::uint32_t>(direct.pid) && candidate.association_support==Support::estimated && !candidate.automatic_startup_origin.value);
    StartupImpactObserver observer;assert(observer.register_process(candidate.startup_entry_id,candidate.baseline));
    assert(observer.poll().updated==1);observer.close();assert(!observer.observations()[0].measurement->impact.value); // no full boot impact inferred
    auto duplicate=snapshot;duplicate.processes.push_back(duplicate.processes[0]);assert(discover_startup_process_candidates(duplicate).error==ActionError::invalid_argument);
    auto stale_pid=snapshot;for(auto& p:stale_pid.processes)if(p.pid==candidate.baseline.pid)++*p.linux_start_ticks.value;
    report=discover_startup_process_candidates(stale_pid);assert(report.candidates.empty() && !report.complete && report.unavailable==1);
    fs::remove(personal/"app.desktop");report=discover_startup_process_candidates(snapshot);
    assert(report.error==ActionError::stale_target && report.candidates.empty());
#else
    assert(discover_startup_process_candidates({}).error==ActionError::not_supported);
#endif
}
