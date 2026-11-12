#include <pcinsight/monitor.hpp>
#include <pcinsight/actions.hpp>
#include "../src/linux_service_selection.hpp"
#include <cassert>
#include <iostream>
#include <stdexcept>
#ifdef __linux__
#include <filesystem>
#include <fstream>
#include <dirent.h>
#include <unistd.h>
#include <cstdlib>
namespace fs=std::filesystem;
std::size_t fds(){auto d=::opendir("/proc/self/fd");assert(d);std::size_t n=0;while(auto e=::readdir(d))if(e->d_name[0]!='.')++n;::closedir(d);return n;}
#endif
using namespace pcinsight;
using namespace pcinsight::detail;
int main(){
    assert(SampleOptions{}.linux_service_selection.provider==LinuxServiceProvider::legacy_default);
    std::array<unsigned,7> calls{};LinuxServiceSelectionApi api;
    std::array<SelectedServiceQuery*,7> queries{&api.systemd_system,&api.systemd_user,&api.sysv,&api.runit,&api.s6,&api.openrc_configured,&api.openrc_marked};
    for(unsigned i=0;i<7;++i)*queries[i]=[&,i](const auto&)->Metric<std::vector<ServiceInfo>>{++calls[i];ServiceInfo r;r.id=std::to_string(i);return {{std::vector<ServiceInfo>{r}},Support::available,"fixture"};};
    LinuxServiceSelection selection;selection.root="/private/root";selection.init_directory="/private/init.d";selection.runlevel_directory="/private/rc";SystemSnapshot snapshot;
    for(unsigned i=0;i<7;++i){calls.fill(0);selection.provider=static_cast<LinuxServiceProvider>(i+1);fill_linux_selected_services(snapshot,selection,api);assert(snapshot.services.size()==1&&snapshot.services[0].id==std::to_string(i)&&metric_is_known(snapshot.linux_service_inventory_source));for(unsigned j=0;j<7;++j)assert(calls[j]==(i==j?1U:0U));}
    calls.fill(0);selection.provider=LinuxServiceProvider::s6;selection.root="relative";fill_linux_selected_services(snapshot,selection,api);assert(snapshot.services.empty()&&!snapshot.linux_service_inventory_source.value);for(auto c:calls)assert(!c);
    selection.root="/private/root";selection.max_entries=0;fill_linux_selected_services(snapshot,selection,api);assert(snapshot.services.empty());for(auto c:calls)assert(!c);selection.max_entries=1024;
    selection.provider=LinuxServiceProvider::sysv;selection.sysv_prefix_budget=16*1024*1024+1;fill_linux_selected_services(snapshot,selection,api);assert(snapshot.services.empty());for(auto c:calls)assert(!c);selection.sysv_prefix_budget=8*1024*1024;
    selection.provider=static_cast<LinuxServiceProvider>(88);fill_linux_selected_services(snapshot,selection,api);assert(snapshot.services.empty()&&!snapshot.linux_service_inventory_source.value);for(auto c:calls)assert(!c);
    selection.provider=LinuxServiceProvider::legacy_default;fill_linux_selected_services(snapshot,selection,api);assert(snapshot.services.empty());for(auto c:calls)assert(!c);
    selection.provider=LinuxServiceProvider::runit;api.runit=[&](const auto&)->Metric<std::vector<ServiceInfo>>{++calls[3];return {{},Support::permission_required,"denied"};};fill_linux_selected_services(snapshot,selection,api);assert(snapshot.services.empty()&&!snapshot.linux_service_inventory_source.value&&snapshot.linux_service_inventory_source.support==Support::permission_required&&calls[3]==1);for(unsigned i=0;i<7;++i)if(i!=3)assert(!calls[i]);
    api.runit=[](const auto&)->Metric<std::vector<ServiceInfo>>{throw std::runtime_error("fixture failure");};fill_linux_selected_services(snapshot,selection,api);assert(snapshot.services.empty()&&!snapshot.linux_service_inventory_source.value&&snapshot.linux_service_inventory_source.note.find("no fallback")!=std::string::npos);
    api.runit=[](const auto&)->Metric<std::vector<ServiceInfo>>{return {std::vector<ServiceInfo>{},Support::available,"known empty"};};fill_linux_selected_services(snapshot,selection,api);assert(snapshot.services.empty()&&metric_is_known(snapshot.linux_service_inventory_source));
    api.runit=[](const auto&)->Metric<std::vector<ServiceInfo>>{ServiceInfo r;r.id="partial";return {std::vector<ServiceInfo>{r},Support::estimated,"partial"};};fill_linux_selected_services(snapshot,selection,api);assert(snapshot.services.size()==1&&snapshot.linux_service_inventory_source.support==Support::estimated);
    // Unused fields are ignored, not consulted as additional providers/scopes.
    selection.provider=LinuxServiceProvider::systemd_system;selection.root="relative";selection.max_entries=0;fill_linux_selected_services(snapshot,selection,api);assert(snapshot.services[0].id=="0");
#ifdef __linux__
    ::alarm(25);char pattern[]="/tmp/pcinsight-service-selection-XXXXXX";auto p=::mkdtemp(pattern);assert(p);const fs::path root=p,init=root/"init.d",rc=root/"rc",runtime=root/"runtime",scan=root/"scan",sentinel=root/"must-not-exist";struct Cleanup{fs::path path;~Cleanup(){std::error_code e;fs::remove_all(path,e);}} cleanup{root};fs::create_directory(init);fs::create_directory(rc);fs::create_directories(runtime/"started");fs::create_directories(scan/"alpha/supervise");{std::ofstream f(init/"alpha");f<<"#!/bin/sh\n### BEGIN INIT INFO\n# Short-Description: selected sysv\n### END INIT INFO\ntouch '"<<sentinel.string()<<"'\n";}fs::create_symlink("/not-followed",runtime/"started/alpha");{std::array<unsigned char,35> b{};b[31]=42;std::ofstream f(scan/"alpha/supervise/status",std::ios::binary);f.write(reinterpret_cast<const char*>(b.data()),b.size());assert(f.good());}
    SampleOptions options;options.include_processes=options.include_disks=options.include_networks=options.include_gpus=false;options.include_linux_process_descriptions=false;options.include_services=true;
    Monitor monitor;const auto initial=fds();auto& choice=options.linux_service_selection;choice.init_directory=init.string();choice.runlevel_directory=rc.string();choice.root=scan.string();
    choice.provider=LinuxServiceProvider::sysv;auto sample=monitor.sample(options);assert(sample.services.size()==1&&sample.services[0].description=="selected sysv"&&sample.linux_service_inventory_source.value=="sysv-file-catalog");
    // A filesystem inventory ID is not a systemd service name or permission to
    // execute the corresponding script. All actions must reject this exact ID.
    const auto sysv_id=sample.services.front().id;
    auto actions=make_system_actions();
    for(const auto action:{ServiceAction::start,ServiceAction::stop,ServiceAction::restart}) {
        const auto result=actions->control_service(sysv_id,action);
        assert(!result&&result.error==ActionError::invalid_argument&&!result.process_id);
        assert(!fs::exists(sentinel));
    }
    choice.provider=LinuxServiceProvider::s6;sample=monitor.sample(options);assert(sample.services.size()==1&&sample.services[0].status=="record-pid-present"&&sample.linux_service_inventory_source.value=="s6-status-record-inventory");
    choice.provider=LinuxServiceProvider::runit;sample=monitor.sample(options);assert(sample.services.size()==1&&sample.services[0].status=="unknown"&&sample.linux_service_inventory_source.value=="runit-reported-inventory");
    choice.provider=LinuxServiceProvider::openrc_configured;choice.root="relative unused";sample=monitor.sample(options);assert(sample.services.size()==1&&sample.services[0].status=="unknown"&&sample.linux_service_inventory_source.value=="openrc-file-catalog"&&sample.linux_service_inventory_source.support==Support::available);
    choice.provider=LinuxServiceProvider::openrc_marked;choice.root=runtime.string();choice.init_directory="relative unused";sample=monitor.sample(options);assert(sample.services.size()==1&&sample.services[0].status=="marked-started"&&sample.linux_service_inventory_source.value=="openrc-runtime-marks"&&sample.linux_service_inventory_source.support==Support::available);
    choice.root=(root/"missing").string();sample=monitor.sample(options);assert(sample.services.empty()&&!sample.linux_service_inventory_source.value&&!fs::exists(choice.root));
    choice.provider=static_cast<LinuxServiceProvider>(99);sample=monitor.sample(options);assert(sample.services.empty()&&!sample.linux_service_inventory_source.value);
    options.include_services=false;choice.provider=LinuxServiceProvider::s6;choice.root=scan.string();sample=monitor.sample(options);assert(sample.services.empty()&&!sample.linux_service_inventory_source.value&&sample.linux_service_inventory_source.note.find("disabled")!=std::string::npos);
    assert(!fs::exists(sentinel)&&fds()==initial);::alarm(0);
#endif
    std::cout<<"Explicit service selector routes/arguments/no fallback/errors/empty/partial/Monitor private providers/disabled/noexec checks passed; not real init acceptance\n";
}
