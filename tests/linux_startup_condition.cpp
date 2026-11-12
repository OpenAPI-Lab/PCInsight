#include "../src/linux_startup_eligibility.hpp"
#include "pcinsight/monitor.hpp"
#include <cassert>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <map>
#ifdef __linux__
#include <sys/stat.h>
#include <unistd.h>
#endif
using namespace pcinsight;
using namespace pcinsight::detail;
namespace fs=std::filesystem;
const std::string base="[Desktop Entry]\nType=Application\nName=Condition test\nExec=/never-run\n";
#ifdef __linux__
struct Temp {
    fs::path path;
    Temp() { char p[]="/tmp/pcinsight-condition-XXXXXX"; const auto* r=mkdtemp(p); assert(r); path=r; }
    ~Temp() { std::error_code ec; fs::remove_all(path,ec); }
};
struct Env {
    std::map<std::string,std::optional<std::string>> saved;
    void set(const char* k,const std::string& v) {
        if(!saved.contains(k)) { const auto* s=getenv(k); saved[k]=s?std::optional<std::string>(s):std::nullopt; }
        assert(setenv(k,v.c_str(),1)==0);
    }
    ~Env() { for(const auto& [k,v]:saved) { if(v)setenv(k.c_str(),v->c_str(),1);else unsetenv(k.c_str()); } }
};
void write(const fs::path& path,const std::string& text) { std::ofstream f(path); assert(f); f<<text; f.close(); assert(f); }
#endif
int main() {
    XdgEligibilityContext context; context.config_home="/config";
    std::string tested; unsigned calls=0;
    context.file_exists=[&](const std::string& path) { tested=path; ++calls; return Metric<bool>{true,Support::available,"injected"}; };
    auto check=[&](const std::string& value) { return autostart_condition_satisfied(base+"AutostartCondition="+value+"\n",context); };
    assert(autostart_condition_satisfied(base,context).value==true && calls==0);
    assert(check("if-exists flag").value==true && tested=="/config/flag");
    assert(check("unless-exists flag").value==false);
    assert(check("IF-EXISTS /absolute/flag").value==true && tested=="/absolute/flag");
    assert(check("if-exists name\\swith\\sspaces").value==true && tested=="/config/name with spaces");
    assert(check("if-exists $VAR;literal").value==true && tested=="/config/$VAR;literal");
    assert(check("if-exists ~/literal").value==true && tested=="/config/~/literal");
    assert(check("if-exists 'quoted'").value==true && tested=="/config/'quoted'");
    const auto before=calls;
    for(const auto& value:{"","if-exists","if-exists   ","if flag","GSettings org.example boolean","GNOME3 if-session gnome","if-exists bad\\q","if-exists bad\\npath"})
        assert(!check(value).value);
    assert(!check(std::string("if-exists a\0b",13)).value);
    assert(!check("if-exists "+std::string(32769,'x')).value && calls==before);
    assert(!autostart_condition_satisfied(base+"AutostartCondition=if-exists a\nAutostartCondition=unless-exists b\n",context).value);
    assert(autostart_condition_satisfied(base+"[Desktop Action Other]\nAutostartCondition=if-exists ignored\n",context).value==true);
    assert(!autostart_condition_satisfied("invalid",context).value);
    context.config_home.reset();
    assert(!check("if-exists relative").value);
    assert(check("if-exists /absolute").value==true);
    context.file_exists=[](const std::string&) { return Metric<bool>{false,Support::available,"absent"}; };
    assert(check("if-exists /absolute").value==false && check("unless-exists /absolute").value==true);
    context.file_exists=[](const std::string&) { return Metric<bool>{std::nullopt,Support::permission_required,"denied"}; };
    assert(!check("unless-exists /absolute").value && check("unless-exists /absolute").support==Support::permission_required);
    context.file_exists={}; assert(!check("if-exists /absolute").value);
#ifdef __linux__
    Temp temp; Env env;
    const auto config=temp.path/"config", autostart=config/"autostart", system=temp.path/"system";
    fs::create_directories(autostart); fs::create_directories(system);
    env.set("XDG_CONFIG_HOME",config.string()); env.set("XDG_CONFIG_DIRS",system.string());
    env.set("XDG_CURRENT_DESKTOP","GNOME");
    const auto flag=config/"flag with spaces", desktop=autostart/"app.desktop";
    write(desktop,base+"AutostartCondition=if-exists flag\\swith\\sspaces\n");
    auto native=native_xdg_eligibility_context();
    assert(native.config_home==config.string());
    assert(autostart_condition_satisfied(base+"AutostartCondition=if-exists flag with spaces\n",native).value==false);
    Monitor monitor; SampleOptions options; options.include_startup_entries=true;
    options.include_processes=options.include_disks=options.include_networks=options.include_gpus=false;
    auto sample=[&] {
        const auto snapshot=monitor.sample(options);
        assert(snapshot.startup_entries.size()==1);
        const auto row=snapshot.startup_entries.front();
        assert(row.id==desktop.string() && row.status==StartupStatus::enabled && row.xdg_standard_eligible.value==true);
        return row.autostart_condition_satisfied;
    };
    assert(sample().value==false);
    write(flag,"exists, but never executed\n"); assert(sample().value==true);
    fs::remove(flag); assert(sample().value==false);
    fs::create_directory(flag); assert(sample().value==true); fs::remove(flag);
    fs::create_symlink(config/"missing",flag); assert(sample().value==false); fs::remove(flag);
    write(config/"target","target"); fs::create_symlink(config/"target",flag); assert(sample().value==true); fs::remove(flag);
    write(desktop,base+"AutostartCondition=unless-exists flag\\swith\\sspaces\n"); assert(sample().value==true);
    write(flag,"present"); assert(sample().value==false);
    assert(mkfifo((config/"fifo").c_str(),0600)==0);
    assert(autostart_condition_satisfied(base+"AutostartCondition=if-exists fifo\n",native).value==true); // stat only, never open/block
    fs::create_symlink(config/"loop",config/"loop");
    assert(!autostart_condition_satisfied(base+"AutostartCondition=unless-exists loop\n",native).value);
    const auto private_dir=config/"private"; fs::create_directory(private_dir); write(private_dir/"flag","present");
    if (geteuid()!=0) {
        assert(chmod(private_dir.c_str(),0000)==0);
        const auto denied=autostart_condition_satisfied(base+"AutostartCondition=unless-exists private/flag\n",native);
        assert(chmod(private_dir.c_str(),0700)==0);
        assert(!denied.value && denied.support==Support::permission_required);
    }
    write(desktop,base+"AutostartCondition=GSettings org.example setting\n");
    assert(!sample().value); // never report an unsupported condition as satisfied
#endif
}
