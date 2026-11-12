#include "../src/linux_startup_override.hpp"
#include "../src/linux_startup.hpp"
#include "pcinsight/audit.hpp"
#include "pcinsight/monitor.hpp"
#include <cassert>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include <map>
#ifdef __linux__
#include <sys/file.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#endif
using namespace pcinsight;
using namespace pcinsight::detail;
namespace fs=std::filesystem;
std::string read(const fs::path& p) { std::ifstream f(p,std::ios::binary); assert(f); return {std::istreambuf_iterator<char>(f),{}}; }
void write(const fs::path& p,const std::string& s) { std::ofstream f(p,std::ios::binary); assert(f); f<<s; f.close(); assert(f); }
#ifdef __linux__
struct Temp {
    fs::path path;
    Temp() { char dir[]="/tmp/pcinsight-override-XXXXXX"; auto* p=mkdtemp(dir); assert(p); path=p; }
    ~Temp() { std::error_code ec; fs::permissions(path,fs::perms::owner_all,ec); fs::remove_all(path,ec); }
};
struct Env {
    std::map<std::string,std::optional<std::string>> old;
    void set(const char* key,const std::string& value) {
        if (!old.contains(key)) { const auto* s=getenv(key); old[key]=s?std::optional<std::string>(s):std::nullopt; }
        assert(setenv(key,value.c_str(),1)==0);
    }
    ~Env() { for (const auto& [key,value]:old) { if(value)setenv(key.c_str(),value->c_str(),1);else unsetenv(key.c_str()); } }
};
#endif
int main() {
#ifdef _WIN32
    const fs::path root="C:/pcinsight-plan";
#else
    const fs::path root="/pcinsight-plan";
#endif
    const auto personal=root/"user/autostart", system=root/"system/autostart";
    std::vector<XdgAutostartDirectory> dirs{{personal,true},{system,false}};
    auto plan=plan_startup_override((system/"app.desktop").string(),dirs);
    assert(plan && plan->destination==personal/"app.desktop" && plan->source==system/"app.desktop");
    assert(plan_startup_override((personal/"app.desktop").string(),dirs));
    assert(!plan_startup_override((root/"other/app.desktop").string(),dirs));
    assert(!plan_startup_override("relative.desktop",dirs));
    assert(!plan_startup_override((system/"app.exe").string(),dirs));
    assert(!plan_startup_override(std::string("a\0.desktop",10),dirs));
    assert(!plan_startup_override((system/"app.desktop").string(),{{system,false}}));
    assert(!plan_startup_override((system/"app.desktop").string(),{{system,true},{system,false}}));
    assert(!plan_startup_override((system/"app.desktop").string(),{{personal,true},{root/"extra",true},{system,false}}));
#ifdef __linux__
    Temp temp; Env env;
    const auto sys=temp.path/"system/autostart", cfg=temp.path/"user/config", user=cfg/"autostart";
    fs::create_directories(sys);
    env.set("XDG_CONFIG_HOME",cfg.string()); env.set("XDG_CONFIG_DIRS",sys.parent_path().string());
    env.set("XDG_CURRENT_DESKTOP","GNOME");
    const std::string original="# preserve\r\n[Desktop Entry]\r\nType=Application\r\nName=Vendor\r\nName[es]=Nombre\r\nExec=/no-run --test\r\nOnlyShowIn=KDE;\r\n[Desktop Action Extra]\r\nExec=/other\r\nHidden=false\r\n";
    const auto source=sys/"app.desktop", destination=user/"app.desktop";
    write(source,original); assert(chmod(source.c_str(),0444)==0);
    struct stat baseline{}; assert(stat(source.c_str(),&baseline)==0);
    const auto malformed=sys/"bad.desktop"; write(malformed,"[Desktop Entry]\nHidden=true\nHidden=false\n");
    auto actions=make_system_actions();
    assert(actions->set_startup_enabled_for_current_user(malformed.string(),false).error==ActionError::invalid_argument);
    assert(!fs::exists(cfg)); // preflight source errors do not bootstrap directories
    std::vector<AuditEvent> events;
    AuditedActions audited(make_system_actions(),[&](const AuditEvent& event){events.push_back(event);});
    assert(audited.set_startup_enabled_for_current_user(source.string(),false));
    assert(events.size()==2 && events[0].operation==AuditOperation::startup_current_user && !events[0].enabled && events[1].result==ActionError::none);
    assert(read(source)==original && read(destination)==*rewrite_xdg_startup(original,false));
    struct stat current{}, copied{}, dir{};
    assert(stat(source.c_str(),&current)==0 && current.st_ino==baseline.st_ino && current.st_mode==baseline.st_mode && current.st_mtim.tv_sec==baseline.st_mtim.tv_sec && current.st_mtim.tv_nsec==baseline.st_mtim.tv_nsec);
    assert(stat(destination.c_str(),&copied)==0 && copied.st_uid==geteuid() && (copied.st_mode&0777)==0600 && copied.st_nlink==1);
    assert(stat(user.c_str(),&dir)==0 && (dir.st_mode&0777)==0700);
    Monitor monitor; SampleOptions options; options.include_startup_entries=true;
    options.include_gpus=options.include_disks=options.include_networks=options.include_processes=false;
    auto snapshot=monitor.sample(options);
    bool found=false;
    for(const auto& row:snapshot.startup_entries) if(row.id==destination.string()) { found=true;assert(row.status==StartupStatus::disabled); }
    assert(found);
    assert(actions->set_startup_enabled_for_current_user(source.string(),true));
    assert(read(destination)==*rewrite_xdg_startup(original,true) && read(source)==original);
    snapshot=monitor.sample(options); found=false;
    for(const auto& row:snapshot.startup_entries) if(row.id==destination.string()) {
        found=true; assert(row.status==StartupStatus::enabled && row.xdg_standard_eligible.value==false); // flags != desktop eligibility
    }
    assert(found);
    // Existing custom entry stays authoritative even when caller selects system source.
    const std::string custom="[Desktop Entry]\nType=Application\nName=Custom\nExec=/custom --mine\nHidden=false\n";
    write(destination,custom);
    assert(actions->set_startup_enabled_for_current_user(source.string(),false));
    assert(read(destination)==*rewrite_xdg_startup(custom,false) && read(source)==original);
    assert(actions->set_startup_enabled_for_current_user(destination.string(),true));
    assert(read(destination)==*rewrite_xdg_startup(custom,true));
    fs::remove(destination); fs::create_symlink(source,destination);
    assert(actions->set_startup_enabled_for_current_user(source.string(),true).error==ActionError::invalid_argument);
    assert(read(source)==original); fs::remove(destination);
    fs::create_hard_link(source,destination);
    assert(actions->set_startup_enabled_for_current_user(source.string(),true).error==ActionError::invalid_argument); fs::remove(destination);
    int lock=open(user.c_str(),O_RDONLY|O_DIRECTORY|O_CLOEXEC); assert(lock>=0 && flock(lock,LOCK_EX|LOCK_NB)==0);
    assert(actions->set_startup_enabled_for_current_user(source.string(),true).error==ActionError::stale_target);
    assert(!fs::exists(destination)); close(lock);
    const auto other=sys/"other.desktop"; write(other,original);
    const auto decoy=user/(".pcinsight-override-"+std::to_string(getpid())+"-1"); write(decoy,"sentinel");
    assert(actions->set_startup_enabled_for_current_user(other.string(),true)); assert(read(decoy)=="sentinel");
    fs::remove(decoy);
    for(const auto& entry:fs::directory_iterator(user)) assert(entry.path().filename().string().starts_with(".pcinsight-")==false);
    const auto outsider=temp.path/"outsider.desktop"; write(outsider,original);
    assert(actions->set_startup_enabled_for_current_user(outsider.string(),true).error==ActionError::invalid_argument);
    assert(!fs::exists(user/"outsider.desktop"));
    assert(chmod(user.c_str(),0777)==0);
    const auto unsafe=actions->set_startup_enabled_for_current_user(source.string(),true);
    assert(unsafe.error==ActionError::permission_denied && !fs::exists(destination));
    assert(chmod(user.c_str(),0700)==0);
    env.set("XDG_CONFIG_HOME",sys.parent_path().string());
    assert(actions->set_startup_enabled_for_current_user(source.string(),false).error==ActionError::invalid_argument);
    assert(read(source)==original);
    env.set("XDG_CONFIG_HOME",cfg.string());
#endif
}
