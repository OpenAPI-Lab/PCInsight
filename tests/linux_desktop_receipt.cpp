#include "../src/linux_desktop_receipt.hpp"
#include <cassert>
#include <iostream>
#ifdef __linux__
#include <cstdlib>
#include <cerrno>
#include <filesystem>
#include <fstream>
#include <map>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#endif
using namespace pcinsight;using namespace pcinsight::detail;
#ifdef __linux__
namespace fs=std::filesystem;
struct Env{std::map<std::string,std::optional<std::string>> saved;void set(const char* k,const std::string& v){const auto old=::getenv(k);saved.emplace(k,old?std::optional<std::string>(old):std::nullopt);assert(::setenv(k,v.c_str(),1)==0);}~Env(){for(const auto& [k,v]:saved){if(v)::setenv(k.c_str(),v->c_str(),1);else ::unsetenv(k.c_str());}}};
void write(const fs::path& p,const std::string& text){std::ofstream f(p);assert(f);f<<text;f.close();assert(f);}
std::size_t fds(){auto d=::opendir("/proc/self/fd");assert(d);std::size_t n=0;while(auto e=::readdir(d))if(e->d_name[0]!='.')++n;::closedir(d);return n;}
#endif
int main(int argc,char** argv){
#ifdef __linux__
    if(argc==3&&std::string_view(argv[1])=="--child"){std::ofstream f(argv[2]);f<<"private child\n";return f?7:120;}
    if(argc==5&&std::string_view(argv[1])=="--wait-child"){const auto input=std::stoi(argv[3]),output=std::stoi(argv[4]);::close(output);{std::ofstream f(argv[2]);f<<"ready\n";if(!f)return 120;}char c{};const auto n=::read(input,&c,1);::close(input);return n==1&&c=='G'?7:121;}
#endif
    LinuxDesktopRoute route{"/private/autostart/a.desktop",{"/bin/fixture",{"literal"},"",false}};
    PrivatePidfdNetworkJoin selected{{999,0,123456},{{1,2},{7,42,100,1,2,3,4,5,6,7},{8,43,200,1,2,8,4,9,6,10}}};
    SelectedExecFile pin{55,8,1};SelectedExecRaw raw{};raw[0]=1;raw[1]=999;raw[3]=123456;
    auto decoded=decode_selected_exec(raw,selected.kernel,pin,0);assert(decoded.value);
    Metric<SelectedExecRead> read{SelectedExecRead{selected,*decoded.value,false},Support::available,"fixture"};
    auto project=[&](const auto& r,std::uint32_t pid=8,ActionError error=ActionError::none){return project_linux_manual_receipt(route,selected,pin,pid,error,r);};
    auto pending=project(read);assert(pending.value&&!pending.value->entry_first_exec_matches.value&&pending.value->origin==LinuxDesktopLaunchOrigin::manual_request);
    read.value->pidfd_exited=true;assert(!project(read).value->entry_first_exec_matches.value); // Exit is not successful exec.
    assert(project(read,0,ActionError::system_error).value&&!project(read,0,ActionError::system_error).value->entry_first_exec_matches.value);
    raw[15]=1;raw[16]=777;raw[17]=55;raw[18]=(8ULL<<20)|1;auto known=decode_selected_exec(raw,selected.kernel,pin,0);assert(known.value);read.value->evidence=*known.value;
    auto receipt=project(read);assert(receipt.value&&receipt.value->entry_first_exec_matches.value==true&&receipt.value->entry_first_exec_matches.support==Support::available&&receipt.value->caller_owned_pid==8);
    assert(project(read,8,ActionError::system_error).value->entry_first_exec_matches.value==true); // Observed exec survives error-channel uncertainty.
    assert(!project(read,0,ActionError::system_error).value&&!project(read,8,ActionError::stale_target).value&&!project(read,9).value);
    auto different=read;different.value->evidence.first_exec.value->inode=56;different.value->evidence.pinned_file_matches.value=false;assert(project(different).value->entry_first_exec_matches.value==false);
    different.value->evidence.pinned_file_matches.value=true;assert(!project(different).value);
    auto partial=read;partial.value->evidence.quality_flags=64;assert(!project(partial).value->entry_first_exec_matches.value);
    auto zero=read;zero.value->evidence.first_exec.value->inode=zero.value->evidence.first_exec.value->device_major=zero.value->evidence.first_exec.value->device_minor=0;assert(project_linux_manual_receipt(route,selected,{},8,ActionError::none,zero).value->entry_first_exec_matches.value==true);
    partial=read;partial.support=Support::estimated;assert(!project(partial).value->entry_first_exec_matches.value);
    partial=read;partial.value->evidence.first_exec.support=Support::estimated;assert(!project(partial).value->entry_first_exec_matches.value);
    for(unsigned n=0;n<10;++n){auto bad=read;switch(n){
        case 0:++bad.value->selected.kernel.kernel_tgid;break;case 1:++bad.value->selected.kernel.leader_start_boottime_ns;break;
        case 2:++bad.value->selected.local.boot[0];break;case 3:++bad.value->selected.local.process.start_ticks;break;
        case 4:++bad.value->selected.local.observer.pid_namespace_inode;break;case 5:bad.value->evidence.publishing=true;break;
        case 6:bad.value->evidence.quality_flags=1;break;case 7:++bad.value->evidence.first_exec.value->kernel_real_uid;break;
        case 8:bad.value->evidence.first_exec.value->observed_monotonic_ns=0;break;case 9:bad.value->evidence.first_exec.value.reset();break;
    }assert(!project(bad).value);}
    Metric<SelectedExecRead> unavailable{{},Support::permission_required,"denied"};assert(!project(unavailable).value&&project(unavailable).support==Support::permission_required);
    LinuxDesktopLaunchSession denied;assert(denied.start("/never/read.desktop",false,true).error==ActionError::permission_denied);assert(denied.start("/never/read.desktop",true,false).error==ActionError::permission_denied);assert(!denied.sample().value);
#ifdef __linux__
    const bool native=argc==2&&std::string_view(argv[1])=="--authorized-private-receipt";assert(argc==1||native);if(!native){std::cout<<"Pure receipt/identity/quality/manual-only/consent checks passed; NO probes loaded\n";return 0;}
    ::alarm(30);const auto initial=fds();char pattern[]="/tmp/pcinsight-desktop-receipt-XXXXXX";const auto d=::mkdtemp(pattern);assert(d);const fs::path root=d;
    struct Cleanup{fs::path p;~Cleanup(){std::error_code e;fs::remove_all(p,e);}} cleanup{root};Env env;
    fs::create_directories(root/"config/autostart");fs::create_directories(root/"system/autostart");env.set("XDG_CONFIG_HOME",(root/"config").string());env.set("XDG_CONFIG_DIRS",(root/"system").string());env.set("XDG_CURRENT_DESKTOP","GNOME");
    const auto image=fs::read_symlink("/proc/self/exe").string();const auto entry=root/"config/autostart/a.desktop";const auto marker=root/"marker";
    for(unsigned mode=0;mode<3;++mode){auto executable=image;const auto cwd=mode==1?root/"missing":root;
        if(mode==2){const auto broken=root/"bad-elf";write(broken,"\177ELFbroken");assert(::chmod(broken.c_str(),0700)==0);executable=broken.string();}
        write(entry,"[Desktop Entry]\nType=Application\nName=Private\nExec=\""+executable+"\" --child \""+marker.string()+"\"\nPath="+cwd.string()+"\n");
        LinuxDesktopLaunchSession session;const auto launched=session.start(entry.string(),true,true);int status{};
        if(mode==0){if(!launched)std::cerr<<launched.message<<'\n';assert(launched&&launched.process_id);assert(session.start(entry.string(),true,true).error==ActionError::stale_target);assert(::waitpid(static_cast<pid_t>(launched.process_id),&status,0)==static_cast<pid_t>(launched.process_id)&&WIFEXITED(status)&&WEXITSTATUS(status)==7&&fs::exists(marker));fs::remove(marker);}
        else assert(!launched&&!launched.process_id&&!fs::exists(marker));
        const auto observed=session.sample();if(!observed.value)std::cerr<<observed.note<<'\n';assert(observed.value&&observed.value->origin==LinuxDesktopLaunchOrigin::manual_request&&observed.value->route.entry_id==entry.string()&&observed.value->pidfd_exited);
        if(mode==0)assert(observed.value->entry_first_exec_matches.value==true&&observed.value->caller_owned_pid==launched.process_id);
        else {assert(!observed.value->entry_first_exec_matches.value&&!observed.value->evidence.first_exec.value&&!observed.value->caller_owned_pid);assert(::waitpid(static_cast<pid_t>(observed.value->selected.local.process.local_pid),&status,WNOHANG)<0&&errno==ECHILD);}
        const auto replay=session.sample();assert(replay.value&&replay.value->entry_first_exec_matches.value==observed.value->entry_first_exec_matches.value&&replay.value->evidence.first_exec.value==observed.value->evidence.first_exec.value);
        session.stop();assert(!session.sample().value&&session.start(entry.string(),true,true).error==ActionError::stale_target);if(mode==0)assert(observed.value->entry_first_exec_matches.value==true);assert(fds()==initial);
        std::cout<<"Private manual receipt mode"<<mode<<": "<<(mode?"pre-exec failure UNKNOWN":"first exec MATCH after reap")<<"; replay/stop/one-shot/FD cleanup passed\n";
    }
    // Closing telemetry must NOT terminate/reap an already released live child.
    int gate[2];assert(::pipe(gate)==0);write(entry,"[Desktop Entry]\nType=Application\nName=Private live\nExec=\""+image+"\" --wait-child \""+marker.string()+"\" "+std::to_string(gate[0])+" "+std::to_string(gate[1])+"\n");
    LinuxDesktopLaunchSession live;const auto running=live.start(entry.string(),true,true);assert(running&&running.process_id);::close(gate[0]);
    for(unsigned i=0;i<200&&!fs::exists(marker);++i)::usleep(10000);assert(fs::exists(marker));
    const auto historical=live.sample();assert(historical.value&&historical.value->entry_first_exec_matches.value==true&&!historical.value->pidfd_exited);live.stop();assert(!live.sample().value);
    int status{};assert(::waitpid(static_cast<pid_t>(running.process_id),&status,WNOHANG)==0);assert(::write(gate[1],"G",1)==1);::close(gate[1]);assert(::waitpid(static_cast<pid_t>(running.process_id),&status,0)==static_cast<pid_t>(running.process_id)&&WIFEXITED(status)&&WEXITSTATUS(status)==7);
    assert(historical.value->entry_first_exec_matches.value==true&&!historical.value->pidfd_exited&&fds()==initial);::alarm(0);std::cout<<"Live-child stop: NO hidden kill/reap, caller released/waited; historical receipt unchanged, FDs clean\n";
#else
    (void)argc;(void)argv;assert(denied.start("/tmp/a.desktop",true,true).error==ActionError::not_supported);
#endif
}
