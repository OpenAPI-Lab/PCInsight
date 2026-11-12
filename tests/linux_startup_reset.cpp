#include "pcinsight/actions.hpp"
#include "pcinsight/audit.hpp"
#include "pcinsight/monitor.hpp"
#include <cassert>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <map>
#include <optional>
#ifdef __linux__
#include <fcntl.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <unistd.h>
#endif
using namespace pcinsight;
namespace fs=std::filesystem;
#ifdef __linux__
struct Temp {
    fs::path path;
    Temp() { char p[]="/tmp/pcinsight-reset-XXXXXX"; const auto* r=mkdtemp(p); assert(r); path=r; }
    ~Temp() { std::error_code ec; fs::remove_all(path,ec); }
};
struct Env {
    std::map<std::string,std::optional<std::string>> old;
    void set(const char* key,const std::string& value) {
        if(!old.contains(key)) { const auto* v=getenv(key); old[key]=v?std::optional<std::string>(v):std::nullopt; }
        assert(setenv(key,value.c_str(),1)==0);
    }
    ~Env() { for(const auto& [k,v]:old) { if(v)setenv(k.c_str(),v->c_str(),1);else unsetenv(k.c_str()); } }
};
void write(const fs::path& p,const std::string& s) { std::ofstream f(p,std::ios::binary); assert(f); f<<s; f.close(); assert(f); }
std::string read(const fs::path& p) { std::ifstream f(p,std::ios::binary); assert(f); return {std::istreambuf_iterator<char>(f),{}}; }
#endif
int main() {
    auto actions=make_system_actions();
    assert(!actions->reset_startup_for_current_user(""));
#ifdef __linux__
    assert(actions->reset_startup_for_current_user("relative.desktop").error==ActionError::invalid_argument);
    Temp temp; Env env;
    const auto config=temp.path/"user", personal=config/"autostart", system=temp.path/"system/autostart", lower=temp.path/"lower/autostart";
    fs::create_directories(personal); fs::create_directories(system); fs::create_directories(lower);
    assert(chmod(personal.c_str(),0700)==0);
    env.set("XDG_CONFIG_HOME",config.string());
    env.set("XDG_CONFIG_DIRS",system.parent_path().string()+":"+lower.parent_path().string());
    env.set("XDG_CURRENT_DESKTOP","GNOME");
    const auto src=system/"app.desktop", dst=personal/"app.desktop";
    const std::string original="[Desktop Entry]\nType=Application\nName=System\nExec=/not-run\nHidden=false\n";
    const std::string custom="[Desktop Entry]\nType=Application\nName=Custom\nExec=/custom\nHidden=true\n";
    write(src,original); assert(chmod(src.c_str(),0444)==0);
    write(dst,custom); assert(chmod(dst.c_str(),0600)==0);
    struct stat baseline{}, user_before{}, after{};
    assert(stat(src.c_str(),&baseline)==0 && stat(dst.c_str(),&user_before)==0);
    assert(actions->reset_startup_for_current_user(src.string()).error==ActionError::invalid_argument);
    const auto orphan=personal/"orphan.desktop"; write(orphan,custom);
    assert(actions->reset_startup_for_current_user(orphan.string()).error==ActionError::not_found && read(orphan)==custom);
    int lock=open(personal.c_str(),O_RDONLY|O_DIRECTORY|O_CLOEXEC); assert(lock>=0 && flock(lock,LOCK_EX|LOCK_NB)==0);
    assert(actions->reset_startup_for_current_user(dst.string()).error==ActionError::stale_target && read(dst)==custom); close(lock);
    assert(chmod(personal.c_str(),0777)==0);
    assert(actions->reset_startup_for_current_user(dst.string()).error==ActionError::permission_denied);
    assert(chmod(personal.c_str(),0700)==0);
    fs::remove(dst); fs::create_symlink(src,dst);
    assert(actions->reset_startup_for_current_user(dst.string()).error==ActionError::invalid_argument); fs::remove(dst);
    fs::create_hard_link(src,dst);
    assert(actions->reset_startup_for_current_user(dst.string()).error==ActionError::invalid_argument); fs::remove(dst);
    write(dst,custom); assert(chmod(dst.c_str(),0600)==0); assert(stat(dst.c_str(),&user_before)==0);
    // Malformed first system entry must not fall through to a valid lower entry.
    const auto bad=system/"bad.desktop", bad_dst=personal/"bad.desktop";
    write(bad,"[Desktop Entry]\nHidden=true\nHidden=false\n"); write(lower/"bad.desktop",original); write(bad_dst,custom);
    assert(actions->reset_startup_for_current_user(bad_dst.string()).error==ActionError::invalid_argument && read(bad_dst)==custom);
    fs::remove(bad); fs::create_symlink(src,bad);
    assert(actions->reset_startup_for_current_user(bad_dst.string()).error==ActionError::invalid_argument && read(bad_dst)==custom);
    env.set("XDG_CONFIG_HOME",system.parent_path().string());
    assert(actions->reset_startup_for_current_user(src.string()).error==ActionError::invalid_argument && read(src)==original);
    env.set("XDG_CONFIG_HOME",config.string());
    const auto decoy=personal/(".pcinsight-reset-"+std::to_string(getpid())+"-0.backup"); write(decoy,"sentinel");
    std::vector<AuditEvent> events;
    AuditedActions audited(make_system_actions(),[&](const AuditEvent& e){events.push_back(e);});
    const auto result=audited.reset_startup_for_current_user(dst.string());
    assert(result && !result.startup_backup_path.empty() && !fs::exists(dst));
    assert(read(result.startup_backup_path)==custom && read(decoy)=="sentinel" && read(src)==original);
    assert(stat(result.startup_backup_path.c_str(),&after)==0 && after.st_ino==user_before.st_ino && after.st_mode==user_before.st_mode && after.st_uid==user_before.st_uid && after.st_nlink==1);
    assert(stat(src.c_str(),&after)==0 && after.st_ino==baseline.st_ino && after.st_mode==baseline.st_mode && after.st_mtim.tv_sec==baseline.st_mtim.tv_sec && after.st_mtim.tv_nsec==baseline.st_mtim.tv_nsec);
    assert(events.size()==2 && events[0].operation==AuditOperation::startup_reset_current_user && events[1].result==ActionError::none);
    assert(actions->reset_startup_for_current_user(dst.string()).error==ActionError::not_found);
    Monitor monitor; SampleOptions options;
    options.include_startup_entries=true;
    options.include_gpus=options.include_disks=options.include_networks=options.include_processes=false;
    const auto snapshot=monitor.sample(options);
    bool found=false;
    for(const auto& row:snapshot.startup_entries) {
        assert(row.id!=result.startup_backup_path && row.id!=dst.string());
        if(row.id==src.string()) { found=true; assert(row.status==StartupStatus::enabled && row.xdg_standard_eligible.value==true); }
    }
    assert(found);
    // Recover the original custom file explicitly; reset never purges the backup.
    fs::rename(result.startup_backup_path,dst);
    assert(read(dst)==custom);
    // Missing first counterpart permits lower priority; reset does NOT enable it.
    const auto disabled=lower/"disabled.desktop", disabled_personal=personal/"disabled.desktop";
    write(disabled,"[Desktop Entry]\nType=Application\nName=Disabled system\nExec=/not-run\nHidden=true\n");
    write(disabled_personal,"broken personal content\n");
    const auto disabled_result=actions->reset_startup_for_current_user(disabled_personal.string());
    assert(disabled_result && read(disabled_result.startup_backup_path)=="broken personal content\n");
    const auto next=monitor.sample(options); found=false;
    for(const auto& row:next.startup_entries) if(row.id==disabled.string()) {
        found=true; assert(row.status==StartupStatus::disabled && row.xdg_standard_eligible.value==false);
    }
    assert(found);
#else
    assert(actions->reset_startup_for_current_user("anything.desktop").error==ActionError::not_supported);
    std::vector<AuditEvent> events;
    AuditedActions audited(make_system_actions(),[&](const AuditEvent& e){events.push_back(e);});
    assert(audited.reset_startup_for_current_user("anything.desktop").error==ActionError::not_supported);
    assert(events.size()==2 && events[0].operation==AuditOperation::startup_reset_current_user && events[1].result==ActionError::not_supported);
#endif
}
