#include "../src/linux_startup_properties.hpp"
#include "pcinsight/startup.hpp"
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
const std::string original="# comment\r\n[Desktop Entry]\r\nType=Application\r\nName=Demo\r\nName[es]=Ejemplo\r\nExec=/never-run --literal\r\nComment=Description\\swith escape\r\nIcon=demo-icon\r\nPath=/not-resolved\r\nTerminal=false\r\nOnlyShowIn=KDE;\r\nX-Vendor=first\r\nX-Vendor=second\r\n[Desktop Action Extra]\r\nName=Other\r\nExec=/also-never-run\r\n";
#ifdef __linux__
struct Temp {
    fs::path path;
    Temp(){char p[]="/tmp/pcinsight-properties-XXXXXX";const auto* r=mkdtemp(p);assert(r);path=r;}
    ~Temp(){std::error_code ec;fs::remove_all(path,ec);}
};
struct Env {
    std::map<std::string,std::optional<std::string>> saved;
    void set(const char* k,const std::string& v){if(!saved.contains(k)){const auto* s=getenv(k);saved[k]=s?std::optional<std::string>(s):std::nullopt;}assert(setenv(k,v.c_str(),1)==0);}
    ~Env(){for(const auto& [k,v]:saved){if(v)setenv(k.c_str(),v->c_str(),1);else unsetenv(k.c_str());}}
};
void write(const fs::path& p,const std::string& s){std::ofstream f(p,std::ios::binary);assert(f);f<<s;f.close();assert(f);}
#endif
int main() {
    const auto properties=parse_startup_properties(original);assert(properties);
    assert(properties->file_size_bytes==original.size() && properties->entry.name=="Demo" && properties->entry.publisher.empty());
    assert(properties->source_file_size_bytes.value==original.size() && properties->source_file_size_bytes.support==Support::available);
    assert(properties->entry.command=="/never-run --literal" && properties->entry.status==StartupStatus::enabled);
    unsigned vendor=0,actions=0,localized=0,comment=0;
    for(const auto& f:properties->fields){
        if(f.key=="X-Vendor")++vendor;
        if(f.group=="Desktop Action Extra")++actions;
        if(f.key=="Name[es]"){++localized;assert(f.raw_value=="Ejemplo");}
        if(f.key=="Comment"){++comment;assert(f.raw_value=="Description\\swith escape");}
    }
    assert(vendor==2 && actions==2 && localized==1 && comment==1);
    assert(!parse_startup_properties("invalid"));
    assert(!parse_startup_properties("[Desktop Entry]\nName=x\nName=y\n"));
    assert(!parse_startup_properties("[Desktop Entry]\nmissing-equals\n"));
    assert(!parse_startup_properties("[Desktop Entry]\n=empty-key\n"));
    assert(!parse_startup_properties("[Desktop Entry]\nComment="+std::string(32769,'x')+"\n"));
    assert(!parse_startup_properties(std::string(1048577,'x')));
    assert(!get_startup_properties("") && get_startup_properties("").error==ActionError::invalid_argument);
    assert(!get_startup_properties(std::string("x\0.desktop",10)));
#ifdef __linux__
    Temp temp;Env env;
    const auto config=temp.path/"config",personal=config/"autostart",system=temp.path/"system/autostart";
    fs::create_directories(personal);fs::create_directories(system);
    env.set("XDG_CONFIG_HOME",config.string());env.set("XDG_CONFIG_DIRS",system.parent_path().string());env.set("XDG_CURRENT_DESKTOP","GNOME");
    const auto src=system/"app.desktop",dst=personal/"app.desktop";
    write(src,original);assert(chmod(src.c_str(),0444)==0);
    struct stat baseline{},after{};assert(stat(src.c_str(),&baseline)==0);
    auto result=get_startup_properties(src.string());assert(result);
    assert(result.value->entry.id==src.string() && result.value->file_size_bytes==original.size());
    assert(result.value->entry.xdg_standard_eligible.value==false && result.value->entry.autostart_condition_satisfied.value==true);
    assert(result.value->entry.publisher.empty() && result.value->entry.impact==StartupImpact::not_measured);
    assert(stat(src.c_str(),&after)==0 && after.st_ino==baseline.st_ino && after.st_mode==baseline.st_mode && after.st_mtim.tv_sec==baseline.st_mtim.tv_sec && after.st_mtim.tv_nsec==baseline.st_mtim.tv_nsec);
    write(dst,"[Desktop Entry]\nType=Application\nName=Custom\nExec=/custom\nHidden=true\n");
    assert(get_startup_properties(src.string()).error==ActionError::stale_target);
    result=get_startup_properties(dst.string());assert(result && result.value->entry.name=="Custom" && result.value->entry.status==StartupStatus::disabled);
    write(dst,"malformed");assert(get_startup_properties(dst.string()).error==ActionError::invalid_argument);
    assert(get_startup_properties(src.string()).error==ActionError::stale_target); // no fallthrough malformed personal
    fs::remove(dst);fs::create_symlink(src,dst);
    result=get_startup_properties(dst.string());assert(result && result.value->entry.name=="Demo");fs::remove(dst);
    write(dst,original);
    if(geteuid()!=0){assert(chmod(dst.c_str(),0000)==0);assert(get_startup_properties(dst.string()).error==ActionError::permission_denied);assert(chmod(dst.c_str(),0600)==0);}
    const auto fifo=personal/"pipe.desktop";assert(mkfifo(fifo.c_str(),0600)==0);assert(!get_startup_properties(fifo.string()));
    assert(get_startup_properties((personal/"missing.desktop").string()).error==ActionError::not_found);
    assert(get_startup_properties((temp.path/"outside.desktop").string()).error==ActionError::invalid_argument);
    assert(get_startup_properties("relative.desktop").error==ActionError::invalid_argument);
    assert(get_startup_properties((personal/"not-desktop.txt").string()).error==ActionError::invalid_argument);
    // Directory access uncertainty must not certify lower-priority properties.
    if(geteuid()!=0){assert(chmod(personal.c_str(),0000)==0);assert(get_startup_properties(src.string()).error==ActionError::system_error);assert(chmod(personal.c_str(),0700)==0);}
#else
    assert(get_startup_properties("C:/example.desktop").error==ActionError::not_supported);
#endif
}
