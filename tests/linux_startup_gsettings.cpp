#include "../src/linux_startup_gsettings.hpp"
#include "../src/linux_startup_eligibility.hpp"
#include "pcinsight/monitor.hpp"
#include <cassert>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#ifdef __linux__
#include <dlfcn.h>
#include <unistd.h>
#endif
using namespace pcinsight;
using namespace pcinsight::detail;
namespace {
int failure{}, creates{}, releases{}, booleans{};
void* pointer=reinterpret_cast<void*>(static_cast<std::uintptr_t>(1));
GSettingsApi fake() {
    return {
        []()->void* { return failure==1?nullptr:pointer; },
        [](void*,const char*,int recursive)->void* { assert(recursive==1);return failure==2?nullptr:pointer; },
        [](void*){++releases;},
        [](void*)->const char*{return failure==3?nullptr:"/example/";},
        [](void*,const char*)->int{return failure!=4;},
        [](void*,const char*)->void*{return failure==5?nullptr:pointer;},
        [](void*){++releases;},
        [](void*)->const void*{return failure==6?nullptr:pointer;},
        [](const void*,const void* expected)->int{assert(std::string(static_cast<const char*>(expected))=="b");return failure!=7;},
        [](void*,void* backend,const char* path)->void*{++creates;assert(!backend&&!path);return failure==8?nullptr:pointer;},
        [](void*){++releases;},
        [](void*,const char*)->void*{return failure==9?nullptr:pointer;},
        [](void*)->const char*{return failure==10?"i":"b";},
        [](void*)->int{++booleans;return failure!=11;},
        [](void*){++releases;}
    };
}
const std::string base="[Desktop Entry]\nType=Application\nName=GSettings test\nExec=/never-run\n";
}
int main() {
    auto api=fake();
    assert(!read_gsettings_boolean("org.example","flag",{}).value);
    assert(!read_gsettings_boolean("bad schema","flag",api).value);
    assert(!read_gsettings_boolean("org.example",std::string(1025,'a'),api).value);
    for(failure=1;failure<=10;++failure) {
        creates=releases=booleans=0;
        assert(!read_gsettings_boolean("org.example","flag",api).value);
        assert(booleans==0);
        if(failure<=7) assert(creates==0); // invalid keys/types/paths never reach fatal constructor
        if(failure==1 || failure==2) assert(releases==0);
        if(failure>=8) assert(releases>=2);
    }
    failure=0; creates=releases=booleans=0;
    auto result=read_gsettings_boolean("org.example","flag",api);
    assert(result.value==true && result.support==Support::estimated && releases==4 && booleans==1);
    failure=11; assert(read_gsettings_boolean("org.example","flag",api).value==false);
    XdgEligibilityContext context; unsigned calls=0;
    context.setting_boolean=[&](const std::string& schema,const std::string& key) {
        ++calls; assert(schema=="org.example" && key=="flag"); return Metric<bool>{false,Support::estimated,"injected"};
    };
    auto check=[&](const std::string& condition) {return autostart_condition_satisfied(base+"AutostartCondition="+condition+"\n",context);};
    assert(check("GSettings org.example flag").value==false && calls==1);
    assert(check("gsettings\torg.example\tflag").support==Support::estimated && calls==2);
    for(const auto* s:{"GSettings org.example","GSettings org.example flag extra","GSettings org.example /flag","GSettings org.example ","GSettings org.example flag\\n"}) assert(!check(s).value);
    assert(calls==2);
    context.setting_boolean=[](const std::string&,const std::string&) {return Metric<bool>{std::nullopt,Support::permission_required,"denied"};};
    assert(!check("GSettings org.example flag").value && check("GSettings org.example flag").support==Support::permission_required);
#ifdef __linux__
    // No host preference writes: select private in-process memory backend BEFORE
    // the first GIO initialization. Environment belongs only to this test process.
    assert(setenv("GSETTINGS_BACKEND","memory",1)==0);
    auto* library=dlopen("libgio-2.0.so.0",RTLD_NOW|RTLD_LOCAL);
    if(!library) { std::cout<<"SKIP native GIO unavailable\n"; return 0; }
    auto initial=read_native_gsettings_boolean("org.gnome.desktop.interface","enable-animations");
    if(!initial.value) { std::cout<<"SKIP native test schema unavailable: "<<initial.note<<'\n'; return 0; }
    assert(initial.support==Support::estimated);
    assert(!read_native_gsettings_boolean("org.pcinsight.absent.schema","flag").value);
    assert(!read_native_gsettings_boolean("org.gnome.desktop.interface","pcinsight-missing-key").value);
    assert(!read_native_gsettings_boolean("org.gnome.desktop.interface","font-name").value);
    using New=void* (*)(const char*);
    using Set=int (*)(void*,const char*,int);
    using Unref=void (*)(void*);
    const auto create=reinterpret_cast<New>(dlsym(library,"g_settings_new"));
    const auto set=reinterpret_cast<Set>(dlsym(library,"g_settings_set_boolean"));
    const auto unref=reinterpret_cast<Unref>(dlsym(library,"g_object_unref"));
    assert(create && set && unref);
    auto* settings=create("org.gnome.desktop.interface"); assert(settings);
    struct Drop {void* p;Unref unref;~Drop(){unref(p);}} drop{settings,unref};
    char dir[]="/tmp/pcinsight-gsettings-XXXXXX"; const auto* raw=mkdtemp(dir); assert(raw);
    namespace fs=std::filesystem; const fs::path root=raw;
    struct Cleanup {fs::path p;~Cleanup(){std::error_code ec;fs::remove_all(p,ec);}} cleanup{root};
    fs::create_directory(root/"autostart"); fs::create_directory(root/"system");
    assert(setenv("XDG_CONFIG_HOME",root.c_str(),1)==0 && setenv("XDG_CONFIG_DIRS",(root/"system").c_str(),1)==0);
    const auto desktop=root/"autostart/test.desktop";
    {std::ofstream f(desktop); f<<base<<"AutostartCondition=GSettings org.gnome.desktop.interface enable-animations\n";f.close();assert(f);}
    Monitor monitor; SampleOptions options;options.include_startup_entries=true;
    options.include_processes=options.include_disks=options.include_networks=options.include_gpus=false;
    auto sample=[&] {
        const auto snapshot=monitor.sample(options);assert(snapshot.startup_entries.size()==1);
        const auto& entry=snapshot.startup_entries[0];
        assert(entry.status==StartupStatus::enabled && entry.xdg_standard_eligible.value==true);
        assert(entry.autostart_condition_satisfied.support==Support::estimated);
        return entry.autostart_condition_satisfied.value;
    };
    assert(set(settings,"enable-animations",0)); assert(sample()==false);
    assert(set(settings,"enable-animations",1)); assert(sample()==true);
    std::cout<<"Native GIO/schema/boolean provider verified, dynamic Monitor false/true with PRIVATE memory backend only\n";
#else
    assert(!read_native_gsettings_boolean("org.example","flag").value);
#endif
}
