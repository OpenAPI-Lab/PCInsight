#include "../src/startup_executable_matches.hpp"
#include "../src/linux_startup_properties.hpp"
#include "startup_match_files.hpp"
#include <cassert>
#include <iostream>
#include <map>
#ifdef __linux__
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <fcntl.h>
#endif
using namespace pcinsight;using namespace pcinsight::detail;
StartupProperties properties(const std::string& command){auto p=parse_startup_properties("[Desktop Entry]\nType=Application\nName=Fixture\nExec="+command+"\n");assert(p);return *p;}
struct Fixture {
    NetworkSelectionContext ctx;StartupBirthObserverReport report;StartupBirthProcessKey key;
    XdgStartupInventory inventory;std::map<std::string,StartupProperties> props;unsigned inventories{},contexts{},files{};bool change_inventory{},change_context{},change_props{},throw_file{};
    Fixture(){ctx.boot={1,2};auto& o=ctx.observer;o.local_pid=11;o.start_ticks=3;o.procfs_inode=4;o.process_inode=5;o.pid_namespace_device=6;o.pid_namespace_inode=7;o.user_namespace_inode=8;
        report.capture_id="capture";report.boot=ctx.boot;report.pid_namespace_device=6;report.pid_namespace_inode=7;report.state=StartupBirthObserverState::closed;report.first_exec_enabled=true;
        key={"capture",ctx.boot,100,0,123};StartupBirthObservation row;row.key=key;row.birth_monotonic_ns=100;row.first_executable={StartupBirthObservation::Executable{150,50,8,1,0},Support::available,"first exec"};report.observations.push_back(row);}
    void add(const std::string& id,const std::string& command,bool disabled=false){auto p=properties(command);p.entry.id=id;if(disabled)p.entry.status=StartupStatus::disabled;inventory.entries.push_back(p.entry);props[id]=p;}
    StartupExecutableMatchApi api(){return {[this]{auto c=ctx;if(change_context&&++contexts>1)++c.observer.pid_namespace_inode;return Metric<NetworkSelectionContext>{c,Support::available,"context"};},
        [this]{auto i=inventory;if(change_inventory&&++inventories>1&&!i.entries.empty())i.entries[0].command+=" changed";return i;},
        [this](const std::string& id){return StartupPropertiesResult{props.at(id),ActionError::none,"properties"};},
        [this](const std::string& path){++files;if(throw_file)throw 1;if(change_props)props.begin()->second.fields.back().raw_value+=" changed";if(path=="/missing")return Metric<StartupExecutableFileId>{{},Support::permission_required,"denied"};return Metric<StartupExecutableFileId>{StartupExecutableFileId{path=="/different"?51U:50U,8,1},Support::estimated,"file"};}};}
    StartupExecutableMatchReport run(){return detail::discover_startup_executable_matches(report,key,api());}
};
int main(){
    for(const auto& raw:{"/bin/app","/bin/app --literal %U","\"/a path/app\" --name=%c %f","/bin/percent%%name","/bin/app \"literal ; argument\""})assert(metric_is_known(direct_xdg_exec_path(properties(raw))));
    for(const auto& raw:{"app","../app","/bad=program","/bin/app %Z","/bin/app %Ftail","/bin/app %f %u","/bin/app \"%U\"","/bin/app;touch","/bin/app \"unterminated","/bin/app \"quoted\"concat","/bin/app $HOME","/bin/app \\q","%k /app","/bin/app %"})assert(!direct_xdg_exec_path(properties(raw)).value);
    {auto p=properties("/app");p.fields.push_back({"Desktop Entry","DBusActivatable","true"});assert(!direct_xdg_exec_path(p).value);p.fields.back().key="Terminal";assert(!direct_xdg_exec_path(p).value);}
    {Fixture f;f.add("a.desktop","/same");auto r=f.run();assert(r.complete&&r.matched==1&&r.unique_candidate.value=="a.desktop"&&r.unique_candidate.support==Support::estimated&&!r.automatic_startup_origin.value);assert(!f.report.observations[0].startup_entry_id.value);}
    {Fixture f;f.add("a.desktop","/same --profile=one");f.add("b.desktop","/same --profile=two",true);auto r=f.run();assert(r.complete&&r.matched==2&&!r.unique_candidate.value&&r.candidates[1].current_status==StartupStatus::disabled);}
    {Fixture f;f.add("a.desktop","/same");f.add("b.desktop","/different");auto r=f.run();assert(r.complete&&r.matched==1&&r.candidates[1].same_file_identifiers.value==false);}
    for(const auto& command:{"/missing","bare-name","/same %X"}){Fixture f;f.add("a.desktop","/same");f.add("b.desktop",command);auto r=f.run();assert(!r.complete&&r.matched==1&&r.unresolved==1&&!r.unique_candidate.value);}
    for(unsigned n=0;n<3;++n){Fixture f;f.add("a.desktop","/same");if(n==0)f.change_context=true;if(n==1)f.change_inventory=true;if(n==2)f.throw_file=true;auto r=f.run();assert(!r.complete&&r.error!=ActionError::none&&!r.matched&&!r.unique_candidate.value);}
    {Fixture f;f.add("a.desktop","/same");f.change_props=true;auto r=f.run();assert(!r.complete&&!r.unique_candidate.value&&!r.candidates[0].same_file_identifiers.value);}
    {Fixture f;f.add("a.desktop","/same");f.inventory.complete=false;assert(!f.run().unique_candidate.value);}
    {Fixture f;for(unsigned n=0;n<10;++n){const auto id=std::to_string(n)+".desktop";f.add(id,"/same");f.props[id].file_size_bytes=1048576;}const auto r=f.run();assert(!r.complete&&r.matched==8&&r.unresolved==2&&f.files==8&&!r.unique_candidate.value);}
    {Fixture f;f.add("a.desktop","/same");++f.report.boot[0];assert(f.run().error!=ActionError::none&&!f.files);}
    {Fixture f;f.add("a.desktop","/same");++f.key.kernel_start_boottime_ns;assert(f.run().error==ActionError::stale_target&&!f.files);}
    {Fixture f;f.add("a.desktop","/same");f.report.observations[0].first_executable.value.reset();assert(f.run().error==ActionError::not_supported&&!f.files);}
#ifdef __linux__
    StartupMatchFiles tmp;const auto binary=tmp.path/"not-run";const auto marker=tmp.path/"MUST-NOT-EXIST";{std::ofstream file(binary);file<<"#!/bin/sh\ntouch "<<marker.string()<<"\n";}assert(::chmod(binary.c_str(),0755)==0);
    struct stat st{};assert(::stat(binary.c_str(),&st)==0);Fixture f;const auto context=native_network_selection_source_api();const auto c=context.observe(context.context);assert(c.value);f.report.boot=c.value->boot;f.key.boot=f.report.boot;f.report.pid_namespace_device=c.value->observer.pid_namespace_device;f.report.pid_namespace_inode=c.value->observer.pid_namespace_inode;f.report.observations[0].key=f.key;
    auto& exe=*f.report.observations[0].first_executable.value;exe.inode=st.st_ino;exe.device_major=major(st.st_dev);exe.device_minor=minor(st.st_dev);
    const auto a=tmp.desktop("a.desktop","\""+binary.string()+"\" --unused %U");auto r=pcinsight::discover_startup_executable_matches(f.report,f.key);assert(r.complete&&r.unique_candidate.value==a&&!std::filesystem::exists(marker));
    const auto b=tmp.desktop("b.desktop","\""+binary.string()+"\" --other-profile",true);r=pcinsight::discover_startup_executable_matches(f.report,f.key);assert(r.complete&&r.matched==2&&!r.unique_candidate.value);std::filesystem::remove(b);
    tmp.desktop("b.desktop","/proc/self/exe");r=pcinsight::discover_startup_executable_matches(f.report,f.key);assert(!r.complete&&!r.unique_candidate.value&&r.unresolved==1);std::filesystem::remove(tmp.personal/"b.desktop");
    const auto fifo=tmp.path/"fifo";assert(::mkfifo(fifo.c_str(),0600)==0);tmp.desktop("b.desktop",fifo.string());r=pcinsight::discover_startup_executable_matches(f.report,f.key);assert(!r.complete&&r.unresolved==1);std::filesystem::remove(tmp.personal/"b.desktop");
    const auto pinned=::open(binary.c_str(),O_PATH|O_CLOEXEC);assert(pinned>=0);std::filesystem::remove(binary);{std::ofstream file(binary);file<<"replacement";}r=pcinsight::discover_startup_executable_matches(f.report,f.key);assert(r.complete&&r.matched==0&&r.candidates[0].same_file_identifiers.value==false);::close(pinned);
#endif
    assert(pcinsight::discover_startup_executable_matches({},{}).error==ActionError::invalid_argument);
    std::cout<<"startup executable matches: strict Exec subset/ambiguity/unknown/stale/current-only/nonexecution passed\n";
}
