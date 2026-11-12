#include "../src/linux_desktop_launch.hpp"
#include "../src/linux_startup_properties.hpp"
#include "../src/linux_selected_exec.hpp"
#include <cassert>
#include <iostream>
#ifdef __linux__
#include <cstdlib>
#include <cerrno>
#include <filesystem>
#include <fstream>
#include <map>
#include <dirent.h>
#include <sys/wait.h>
#include <unistd.h>
#endif
using namespace pcinsight;using namespace pcinsight::detail;
Metric<LinuxDesktopRoute> resolve(std::string exec,std::string extra={}){
    auto p=parse_startup_properties("[Desktop Entry]\nType=Application\nName=Demo\nExec="+exec+"\n"+extra);if(!p)return {{},Support::unsupported,"bad fixture source"};p->entry.id="/private/autostart/demo.desktop";return resolve_linux_desktop_route(*p);
}
#ifdef __linux__
namespace fs=std::filesystem;
struct Env{std::map<std::string,std::optional<std::string>> saved;void set(const char* key,const std::string& value){const auto v=::getenv(key);saved.emplace(key,v?std::optional<std::string>(v):std::nullopt);assert(::setenv(key,value.c_str(),1)==0);}~Env(){for(const auto& [k,v]:saved){if(v)::setenv(k.c_str(),v->c_str(),1);else ::unsetenv(k.c_str());}}};
void write(const fs::path& p,const std::string& s){std::ofstream f(p);assert(f);f<<s;f.close();assert(f);}
std::size_t fds(){auto d=::opendir("/proc/self/fd");assert(d);std::size_t n=0;while(auto e=::readdir(d))if(e->d_name[0]!='.')++n;::closedir(d);return n;}
#endif
int main(int argc,char** argv){
#ifdef __linux__
    if(argc>=3&&std::string_view(argv[1])=="--child"){std::ofstream f(argv[2]);for(int i=3;i<argc;++i)f<<argv[i]<<'\n';return f?7:120;}
#endif
    const auto route=resolve(R"(/bin/tool "two words" "" a%%b %U)","Path=/private/a\\sb\nTerminal=false\nDBusActivatable=false\n");assert(route.value&&route.support==Support::available);
    assert(route.value->request.executable=="/bin/tool"&&route.value->request.arguments==std::vector<std::string>({"two words","","a%b"})&&route.value->request.working_directory=="/private/a b"&&!route.value->request.elevated);
    auto escaped=resolve(R"(/bin/tool "\\$HOME" "\\`" "\\\\")");assert(escaped.value&&escaped.value->request.arguments==std::vector<std::string>({"$HOME","`","\\"}));
    auto literal=resolve(R"(/bin/tool "; touch NO" "a&b")");assert(literal.value&&literal.value->request.arguments[0]=="; touch NO");
    auto deprecated=resolve("/bin/tool %d %D %n %N %v %m");assert(deprecated.value&&deprecated.value->request.arguments.empty());
    auto actions=resolve("/bin/tool","[Desktop Action Other]\nExec=/wrong\nName=Other\n");assert(actions.value&&actions.value->request.executable=="/bin/tool");
    for(const auto* bad:{"tool x","/bin/a=b","/bin/tool ;touch","/bin/tool 'x'","/bin/tool \"unfinished","/bin/tool \"a\"b","/bin/tool \"$HOME\"","/bin/tool %Z","/bin/tool %c","/bin/tool %i","/bin/tool %k","/bin/tool x%f","/bin/tool %U %f","/bin/tool \"%U\"","/bin/tool %","/bin/tool \\q","/bin/tool\tx"})assert(!resolve(bad).value);
    for(const auto* extra:{"Terminal=true\n","Terminal=TRUE\n","DBusActivatable=true\n","PrefersNonDefaultGPU=true\n","Path=relative\n","Hidden=true\n","X-GNOME-Autostart-enabled=false\n","X-KDE-autostart-condition=something\n","Terminal=false\nTerminal=false\n"})assert(!resolve("/bin/tool",extra).value);
    auto malformed=parse_startup_properties("[Desktop Entry]\nName=Demo\nExec=/bin/tool\n");assert(malformed);malformed->entry.id="/tmp/a.desktop";assert(!resolve_linux_desktop_route(*malformed).value);
    assert(launch_linux_desktop_entry("/never/read.desktop",false,{}).error==ActionError::permission_denied);
#ifdef __linux__
    const bool native=argc==2&&std::string_view(argv[1])=="--authorized-private-desktop";assert(argc==1||native);::alarm(25);const auto initial_fds=fds();
    char pattern[]="/tmp/pcinsight-desktop-launch-XXXXXX";const auto created=::mkdtemp(pattern);assert(created);const fs::path root=created;
    struct Cleanup{fs::path p;~Cleanup(){std::error_code e;fs::remove_all(p,e);}} cleanup{root};Env env;
    fs::create_directories(root/"config/autostart");fs::create_directories(root/"system/autostart");env.set("XDG_CONFIG_HOME",(root/"config").string());env.set("XDG_CONFIG_DIRS",(root/"system").string());env.set("XDG_CURRENT_DESKTOP","GNOME");
    const auto image=fs::read_symlink("/proc/self/exe").string(),out=(root/"marker").string();const auto entry=root/"config/autostart/demo.desktop";
    const std::string text="[Desktop Entry]\nType=Application\nName=Private fixture\nExec=\""+image+"\" --child \""+out+"\" \"two words\" \"; touch NO\" %U\nPath="+root.string()+"\nTerminal=false\n";write(entry,text);
    LinuxSelectedExecCapture capture;std::uint64_t inode{};unsigned calls=0;
    auto arm=[&](const LinuxDesktopRoute& r,const LinuxGatedLaunchTarget& t){++calls;assert(r.entry_id==entry.string()&&r.request.executable==image&&r.request.arguments.size()==4&&!fs::exists(out));inode=t.executable.inode;
        if(native){const auto started=capture.start(true,t);if(!started.value)std::cerr<<started.note<<'\n';assert(started.value);const auto pending=capture.sample();assert(pending.value&&!pending.value->evidence.first_exec.value);}
        return ActionResult{};
    };
    auto result=launch_linux_desktop_entry(entry.string(),true,arm);
    if(result.error==ActionError::not_supported&&!calls){assert(!result.process_id&&!fs::exists(out));std::cout<<"Pure route passed; native gate unsupported, POSITIVE NOT verified\n";return 0;}
    if(!result)std::cerr<<result.message<<'\n';assert(result&&calls==1);int status{};assert(::waitpid(static_cast<pid_t>(result.process_id),&status,0)==static_cast<pid_t>(result.process_id)&&WIFEXITED(status)&&WEXITSTATUS(status)==7&&fs::exists(out));
    {std::ifstream f(out);std::string s;std::getline(f,s);assert(s=="two words");std::getline(f,s);assert(s=="; touch NO");}assert(!fs::exists(root/"NO"));
    if(native){const auto observed=capture.sample();assert(observed.value&&observed.value->pidfd_exited&&observed.value->evidence.first_exec.value&&observed.value->evidence.first_exec.value->inode==inode&&observed.value->evidence.pinned_file_matches.value==true&&!observed.value->evidence.quality_flags);capture.stop();std::cout<<"Private desktop -> gated child -> successful pinned first exec AFTER reap: POSITIVE\n";}
    fs::remove(out);
    std::uint32_t cancelled{};
    for(const auto* mutation:{"Comment=changed snapshot\n","Hidden=true\n"}){write(entry,text);result=launch_linux_desktop_entry(entry.string(),true,[&](const auto&,const auto& t){cancelled=t.child.local_pid;write(entry,text+mutation);return ActionResult{};});assert(result.error==ActionError::stale_target&&!result.process_id&&!fs::exists(out)&&::waitpid(static_cast<pid_t>(cancelled),&status,WNOHANG)<0&&errno==ECHILD);}
    assert(launch_linux_desktop_entry(entry.string(),true,arm).error==ActionError::not_supported);write(entry,text+"AutostartCondition=Unknown condition\n");assert(launch_linux_desktop_entry(entry.string(),true,arm).error==ActionError::not_supported);
    write(entry,text);const auto system=root/"system/autostart/demo.desktop";write(system,text);assert(launch_linux_desktop_entry(system.string(),true,arm).error==ActionError::stale_target);
    assert(calls==1&&fds()==initial_fds&&!fs::exists(out));::alarm(0);std::cout<<"Desktop direct-route POSITIVE/literal argv/field omission/shadow/disabled/unknown condition/stale-arm cancellation/reap/FD cleanup passed; NOT automatic startup cause\n";
#else
    (void)argc;(void)argv;assert(launch_linux_desktop_entry("/tmp/a.desktop",true,{}).error==ActionError::not_supported);
#endif
}
