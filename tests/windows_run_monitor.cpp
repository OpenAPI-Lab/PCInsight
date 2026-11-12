#include "pcinsight/monitor.hpp"
#include "pcinsight/sampler.hpp"
#include <cassert>
#include <iostream>
#include <thread>
using namespace pcinsight;
namespace {
void check(const SystemSnapshot& snapshot){
    const auto& report=snapshot.windows_run_registrations;
    assert(snapshot.startup_entries.empty()); // This option must not request generic Startup inventory.
#ifdef _WIN32
    if(report.value){
        assert(report.support==Support::estimated);
        for(const auto& row:*report.value){
            assert(row.entry_id.starts_with("winrun:hkcu:shared:") && row.state!=WindowsRunRegistrationState::absent);
            if(row.state==WindowsRunRegistrationState::conflict)assert(row.command.empty() && row.native_data.empty());
            else assert(!row.command.empty() && !row.native_data.empty() && (row.native_type==1 || row.native_type==2));
        }
    }else assert(report.support==Support::unsupported || report.support==Support::permission_required);
#else
    assert(!report.value && report.support==Support::unsupported && report.note.find("requires Windows")!=std::string::npos);
#endif
    assert(!report.note.empty());
}
}
int main(){
    SampleOptions options;
    assert(!options.include_windows_managed_run);
    options.include_processes=options.include_disks=options.include_networks=options.include_gpus=false;
    Monitor monitor;
    auto disabled=monitor.sample(options);
    assert(!disabled.windows_run_registrations.value && disabled.windows_run_registrations.note.find("disabled")!=std::string::npos);
    options.include_windows_managed_run=true;
    const auto enabled=monitor.sample(options);check(enabled);
    if(enabled.windows_run_registrations.value)std::cout<<"Monitor native managed Run rows="<<enabled.windows_run_registrations.value->size()<<'\n';
    options.include_windows_managed_run=false;
    disabled=monitor.sample(options);assert(!disabled.windows_run_registrations.value && disabled.windows_run_registrations.note.find("disabled")!=std::string::npos);
    // A previous snapshot owns its report; disabling later samples does not mutate it.
    check(enabled);
    options.include_windows_managed_run=true;
    Sampler sampler(std::chrono::hours(1),1,options);sampler.start();
    const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(5);
    while(!sampler.latest() && sampler.running() && std::chrono::steady_clock::now()<deadline)std::this_thread::sleep_for(std::chrono::milliseconds(5));
    sampler.stop();assert(sampler.last_error().empty());
    const auto latest=sampler.latest();assert(latest);check(*latest);
    const auto history=sampler.history();assert(history.size()==1);check(history.front());
    std::cout<<"Run metadata report: Monitor opt-in/out, independent inventory, owned snapshots, Sampler latest/history and platform support passed\n";
}
