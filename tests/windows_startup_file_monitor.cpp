#include "pcinsight/monitor.hpp"
#include "pcinsight/sampler.hpp"
#include "../src/windows_startup_file_monitor.hpp"
#include <cassert>
#include <iostream>
#include <thread>
using namespace pcinsight;
namespace {
void check(const SystemSnapshot& snapshot){
    assert(snapshot.startup_entries.empty());
    assert(!snapshot.windows_run_registrations.value); // No implicit Run inventory.
    const auto& report=snapshot.windows_startup_file_backups;
    assert(!report.note.empty());
#ifdef _WIN32
    if(report.value){
        assert(report.support==Support::estimated);
        for(const auto& item:*report.value){
            const auto& row=item.registration;
            assert(!item.name.empty() && row.entry_id.starts_with("winfolder:user:"));
            assert(row.state==WindowsStartupFileState::retained_by_pcinsight || row.state==WindowsStartupFileState::conflict);
            assert(row.file_identity.has_value()==(row.state==WindowsStartupFileState::retained_by_pcinsight));
        }
    }else assert(report.support==Support::unsupported || report.support==Support::permission_required);
#else
    assert(!report.value && report.support==Support::unsupported && report.note.find("requires Windows")!=std::string::npos);
#endif
}
}
int main(){
    SampleOptions options;assert(!options.include_windows_startup_file_backups);
    options.include_processes=options.include_disks=options.include_networks=options.include_gpus=false;
    Metric<std::vector<WindowsStartupFileBackup>> provided{std::vector<WindowsStartupFileBackup>{},Support::estimated,"private provider report"};
    WindowsStartupFileBackup item;item.name="Private backup";item.registration.entry_id="private-test-id";
    item.registration.state=WindowsStartupFileState::retained_by_pcinsight;item.registration.file_identity=WindowsStartupFileIdentity{1,2,3,4,0};
    provided.value->push_back(item);unsigned calls=0;
    detail::WindowsStartupFileBackupProvider provider=[&]{++calls;return provided;};
    SystemSnapshot injected;
    detail::fill_windows_startup_file_backups(injected,options,provider);
    assert(calls==0 && !injected.windows_startup_file_backups.value && injected.windows_startup_file_backups.note.find("disabled")!=std::string::npos);
    options.include_windows_startup_file_backups=true;
    detail::fill_windows_startup_file_backups(injected,options,provider);
    assert(calls==1 && injected.windows_startup_file_backups.value->front().name==item.name);
    const auto owned=injected;
    provided.value->clear();provided.note="known empty";
    detail::fill_windows_startup_file_backups(injected,options,provider);
    assert(injected.windows_startup_file_backups.value && injected.windows_startup_file_backups.value->empty() && injected.windows_startup_file_backups.note=="known empty");
    assert(owned.windows_startup_file_backups.value->size()==1 && owned.windows_startup_file_backups.note=="private provider report");
    provided={{},Support::permission_required,"private denied"};
    detail::fill_windows_startup_file_backups(injected,options,provider);
    assert(!injected.windows_startup_file_backups.value && injected.windows_startup_file_backups.support==Support::permission_required && injected.windows_startup_file_backups.note=="private denied");
    detail::fill_windows_startup_file_backups(injected,options,{});
    assert(!injected.windows_startup_file_backups.value && injected.windows_startup_file_backups.note.find("unavailable")!=std::string::npos);
    options.include_windows_startup_file_backups=false;
    detail::fill_windows_startup_file_backups(injected,options,provider);assert(calls==3);
    Monitor monitor;
    const auto disabled=monitor.sample(options);
    assert(!disabled.windows_startup_file_backups.value && disabled.windows_startup_file_backups.note.find("disabled")!=std::string::npos);
    options.include_windows_startup_file_backups=true;
    const auto enabled=monitor.sample(options);check(enabled);
    options.include_windows_startup_file_backups=false;
    const auto disabled_again=monitor.sample(options);
    assert(!disabled_again.windows_startup_file_backups.value && disabled_again.windows_startup_file_backups.note.find("disabled")!=std::string::npos);
    check(enabled);
    options.include_windows_startup_file_backups=true;
    Sampler sampler(std::chrono::hours(1),1,options);sampler.start();
    const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(5);
    while(!sampler.latest() && sampler.running() && std::chrono::steady_clock::now()<deadline)std::this_thread::sleep_for(std::chrono::milliseconds(5));
    sampler.stop();assert(sampler.last_error().empty() && sampler.latest());check(*sampler.latest());
    const auto history=sampler.history();assert(history.size()==1);check(history.front());
    const auto timed=sampler.timed_history();assert(timed.size()==1);check(timed.front().snapshot);
    auto latest=sampler.latest();latest->windows_startup_file_backups={{},Support::unsupported,"caller changed its copy"};
    check(*sampler.latest());
    std::cout<<"Startup backup report: opt-in/out, provider not invoked while disabled, error/empty propagation, owned snapshots, native Monitor and Sampler latest/history passed\n";
}
