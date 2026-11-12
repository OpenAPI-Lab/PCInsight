#include "pcinsight/startup_overlay.hpp"
#include "pcinsight/monitor.hpp"
#include <cassert>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <map>
#include <thread>
#ifdef __linux__
#include <unistd.h>
#endif
using namespace pcinsight;
using namespace std::chrono_literals;
StartupImpactObservation row(std::string id="app",std::uint32_t pid=10,std::chrono::nanoseconds cpu=500ms) {
    StartupProcessSample first;first.pid=pid;first.linux_start_ticks=100;first.native_context="context";
    first.sampled_at=std::chrono::steady_clock::time_point(1s);first.cumulative_cpu_time={0ns,Support::available,"cpu"};first.cumulative_disk_bytes={0,Support::available,"io"};
    auto last=first;last.sampled_at+=1s;last.cumulative_cpu_time.value=cpu;
    auto result=measure_startup_impact(id,first,last);assert(result);
    StartupImpactObservation o;o.startup_entry_id=id;o.pid=pid;o.linux_start_ticks=100;o.measurement=*result.value;
    o.sample_count=2;o.closed=o.window_complete=true;o.native_context="context";return o;
}
SystemSnapshot view() {
    SystemSnapshot s;StartupEntry a;a.id="app";a.name="preserve";a.status=StartupStatus::disabled;
    StartupEntry b;b.id="unrelated";b.impact=StartupImpact::none;b.impact_scope="other provider";
    s.startup_entries={a,b};return s;
}
#ifdef __linux__
struct Env {
    std::map<std::string,std::optional<std::string>> saved;
    void set(const char* k,const std::string& v){if(!saved.contains(k)){const auto* p=getenv(k);saved[k]=p?std::optional<std::string>(p):std::nullopt;}assert(setenv(k,v.c_str(),1)==0);}
    ~Env(){for(const auto& [k,v]:saved){if(v)setenv(k.c_str(),v->c_str(),1);else unsetenv(k.c_str());}}
};
#endif
int main() {
    auto snapshot=view();auto o=row();
    auto result=apply_startup_impact_observations(snapshot,{o},"context");assert(result && result.applied_entries==1);
    const auto& a=snapshot.startup_entries[0];assert(a.impact==StartupImpact::medium && a.observed_startup_impact.support==Support::estimated);
    assert(a.observed_startup_cpu_time.value==500ms && a.observed_startup_disk_bytes.value==0 && a.observed_startup_interval.value==1s);
    assert(a.name=="preserve" && a.status==StartupStatus::disabled && a.impact_scope.find("NOT certified")!=std::string::npos);
    assert(snapshot.startup_entries[1].impact==StartupImpact::none && snapshot.startup_entries[1].impact_scope=="other provider");
    assert(apply_startup_impact_observations(snapshot,{o},"context"));assert(snapshot.startup_entries[0].observed_startup_cpu_time.value==500ms); // overwrite, no accumulation
    o.closed=o.window_complete=false;assert(apply_startup_impact_observations(snapshot,{o},"context"));
    assert(snapshot.startup_entries[0].impact==StartupImpact::not_measured && !snapshot.startup_entries[0].observed_startup_impact.value);
    auto high=row("app",10,2s);high.window_complete=high.closed=false;
    assert(apply_startup_impact_observations(snapshot,{high},"context"));assert(snapshot.startup_entries[0].impact==StartupImpact::high);
    auto other=row("app",11,500ms);
    assert(apply_startup_impact_observations(snapshot,{row(),other},"context"));assert(!snapshot.startup_entries[0].observed_startup_impact.value);
    assert(!snapshot.startup_entries[0].observed_startup_cpu_time.value && !snapshot.startup_entries[0].observed_startup_disk_bytes.value);
    assert(apply_startup_impact_observations(snapshot,{high,other},"context"));assert(snapshot.startup_entries[0].impact==StartupImpact::high);
    StartupImpactObservation pending;pending.startup_entry_id="app";pending.pid=10;pending.linux_start_ticks=100;pending.native_context="context";pending.sample_count=1;
    assert(apply_startup_impact_observations(snapshot,{pending},"context"));assert(!snapshot.startup_entries[0].observed_startup_cpu_time.value);
    pending.last_error=ActionError::permission_denied;assert(apply_startup_impact_observations(snapshot,{pending},"context"));
    assert(snapshot.startup_entries[0].observed_startup_impact.support==Support::permission_required);
    auto unavailable=row();unavailable.measurement->impact={std::nullopt,Support::unsupported,"unknown"};
    unavailable.measurement->cpu_time.support=Support::unsupported;
    assert(apply_startup_impact_observations(snapshot,{unavailable},"context") && !snapshot.startup_entries[0].observed_startup_cpu_time.value);
    result=apply_startup_impact_observations(snapshot,{row("missing")},"context");assert(result && !result.applied_entries && result.not_present_observations==1);
    // Invalid later row must not partially apply an earlier valid row.
    snapshot=view();auto bad=row("unrelated",11);bad.native_context="other boot";
    result=apply_startup_impact_observations(snapshot,{row(),bad},"context");assert(result.error==ActionError::stale_target);
    assert(snapshot.startup_entries[0].impact==StartupImpact::not_measured && snapshot.startup_entries[1].impact==StartupImpact::none);
    assert(!apply_startup_impact_observations(snapshot,{row(),row()},"context"));
    assert(!apply_startup_impact_observations(snapshot,{row(),row("unrelated")},"context")); // same identity, other app
    bad=row();bad.measurement->pid=999;assert(!apply_startup_impact_observations(snapshot,{bad},"context"));
    bad=row();bad.closed=false;assert(!apply_startup_impact_observations(snapshot,{bad},"context"));
    bad=row();bad.measurement->cpu_time.value=-1ns;assert(!apply_startup_impact_observations(snapshot,{bad},"context"));
    bad=row();bad.measurement->impact.note=std::string(4097,'x');assert(!apply_startup_impact_observations(snapshot,{bad},"context"));
    bad=row();bad.measurement->disk_bytes={std::nullopt,Support::unsupported,"missing"};assert(!apply_startup_impact_observations(snapshot,{bad},"context"));
    bad=row();bad.measurement->cpu_time.value=0ns;assert(!apply_startup_impact_observations(snapshot,{bad},"context"));
    assert(!apply_startup_impact_observations(snapshot,{},""));
    snapshot.startup_entries.push_back(snapshot.startup_entries[0]);assert(!apply_startup_impact_observations(snapshot,{},"context"));
#ifdef __linux__
    namespace fs=std::filesystem;char path[]="/tmp/pcinsight-overlay-XXXXXX";const auto* raw=mkdtemp(path);assert(raw);const fs::path root=raw;
    struct Cleanup {fs::path p;~Cleanup(){std::error_code ec;fs::remove_all(p,ec);}} cleanup{root};Env env;
    fs::create_directory(root/"autostart");fs::create_directory(root/"system");
    env.set("XDG_CONFIG_HOME",root.string());env.set("XDG_CONFIG_DIRS",(root/"system").string());
    const auto file=root/"autostart/test.desktop";{std::ofstream f(file);f<<"[Desktop Entry]\nType=Application\nName=Overlay fixture\nExec=/never-run\n";f.close();assert(f);}
    Monitor monitor;SampleOptions options;options.include_startup_entries=true;options.include_disks=options.include_networks=options.include_gpus=false;
    auto current=monitor.sample(options);assert(current.startup_entries.size()==1);std::uint64_t ticks=0;
    for(const auto& p:current.processes)if(p.pid==static_cast<std::uint32_t>(getpid()))ticks=p.linux_start_ticks.value.value_or(0);
    const auto initial=read_startup_process_sample(getpid(),ticks);assert(initial);
    StartupImpactObserver observer(1ms);assert(observer.register_process(file.string(),*initial.value));
    std::this_thread::sleep_for(5ms);assert(observer.poll().updated==1);
    auto records=observer.observations();assert(records.size()==1 && records[0].window_complete && records[0].native_context==initial.value->native_context);
    result=apply_startup_impact_observations(current,records,initial.value->native_context);assert(result && result.applied_entries==1);
    assert(current.startup_entries[0].observed_startup_impact.value && current.startup_entries[0].observed_startup_cpu_time.value && current.startup_entries[0].observed_startup_disk_bytes.value);
    assert(current.startup_entries[0].impact_scope.find("NOT certified")!=std::string::npos && current.startup_entries[0].status==StartupStatus::enabled);
    // A fresh Monitor sample does not apply prior observations implicitly.
    assert(monitor.sample(options).startup_entries[0].impact==StartupImpact::not_measured);
#endif
}
