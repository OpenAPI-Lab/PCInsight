#include "../src/linux_network_admission_monitor.hpp"
#include <cassert>
#include <stdexcept>
#include <thread>
#include <atomic>
using namespace pcinsight;using namespace pcinsight::detail;
namespace {
constexpr auto partial=LinuxNetworkAdmissionPolicy::verified_only;
SampleOptions options(){SampleOptions o;o.include_processes=o.include_disks=o.include_networks=o.include_gpus=false;o.include_linux_process_network=true;return o;}
struct Fake {
    int refreshes=0,reads=0,stops=0,fault=0;bool throw_refresh=false,deny_refresh=false;
    LinuxNetworkAdmissionChange report{true,true,3,7,100,200,partial,2,false,{{17,LinuxNetworkInventoryState::permission_denied,"denied row"}}};
    VisibleNetworkMonitorApi api(){return {this,
        [](void* p,bool authorized,LinuxNetworkAdmissionPolicy policy)->Metric<LinuxNetworkAdmissionChange>{auto& f=*static_cast<Fake*>(p);assert(authorized);++f.refreshes;if(f.throw_refresh)throw std::runtime_error("refresh");
            if(f.deny_refresh||(policy==LinuxNetworkAdmissionPolicy::strict_all&&!f.report.excluded.empty()))return {{},Support::permission_required,"strict requires all rows"};f.report.policy=policy;return {f.report,Support::available,{}};},
        [](void* p)->Metric<LinuxAdmittedProcessNetworkSnapshot>{auto& f=*static_cast<Fake*>(p);++f.reads;if(f.fault==1)return {{},Support::permission_required,"sample denied"};if(f.fault==2)throw std::runtime_error("sample");
            LinuxAdmittedProcessNetworkSnapshot frame;frame.admission=f.report;auto& n=frame.network;n.selection_epoch=f.report.selection_epoch;n.coverage=LinuxProcessNetworkCoverage::observed_tcp_udp_socket_namespace_buckets;
            for(unsigned i=0;i<f.report.verified_processes;++i)n.observed_processes.push_back({100+i,0,900+i,{8+i,0,200+i},0,0});
            if(f.fault==3)++frame.admission.selection_epoch;if(f.fault==4)++n.selection_epoch;if(f.fault==5)frame.admission.excluded[0].pid=8;
            if(f.fault==6)frame.admission.complete_encountered_set=true;if(f.fault==7)n.observed_processes.pop_back();if(f.fault==9)n.observed_processes[1]=n.observed_processes[0];
            if(f.fault==10)++frame.admission.inventory_finished_ns;
            if(f.fault==11){LinuxProcessNetworkSample row;row.process.pid=17;n.processes.push_back(row);}
            return {std::move(frame),f.fault==8?Support::available:Support::estimated,{}};},
        [](void* p) noexcept {++static_cast<Fake*>(p)->stops;}};}
};
void adapter(){const auto on=options();auto off=on;off.include_linux_process_network=false;
    {Fake f;LinuxVisibleNetworkMonitor m(f.api());SystemSnapshot s;m.fill(s,on);assert(!s.linux_network_admission.value&&!s.linux_process_network.value&&f.reads==0);
        assert(!m.refresh(false,partial).value&&f.refreshes==0);assert(m.refresh(true,partial).value);m.fill(s,on);
        assert(s.linux_network_admission.value&&s.linux_process_network.value&&s.linux_network_admission.value->excluded[0].pid==17&&s.linux_network_admission.value->selection_epoch==s.linux_process_network.value->selection_epoch);
        const auto saved=s;assert(!m.refresh(false,partial).value&&f.stops==0);m.fill(s,on);assert(f.refreshes==1&&f.reads==2);
        m.fill(s,off);assert(f.stops==1&&!s.linux_network_admission.value&&!s.linux_process_network.value);m.fill(s,on);assert(f.reads==2&&f.refreshes==1);
        assert(saved.linux_network_admission.value->excluded[0].note=="denied row");}
    for(int fault=1;fault<=11;++fault){Fake f;LinuxVisibleNetworkMonitor m(f.api());assert(m.refresh(true,partial).value);f.fault=fault;SystemSnapshot s;m.fill(s,on);
        assert(!s.linux_network_admission.value&&!s.linux_process_network.value&&f.stops==1);m.fill(s,on);assert(f.reads==1);}
    for(unsigned fault=0;fault<9;++fault){Fake f;if(fault==0)f.report.capture_active=false;if(fault==1)f.report.selection_epoch=0;if(fault==2){f.report.verified_processes=LinuxProcessNetworkSampler::max_processes+1;f.report.visible_processes=f.report.verified_processes+1;}
        if(fault==3)f.report.visible_processes=0;if(fault==4)f.report.excluded[0].note.clear();if(fault==5)f.report.inventory_started_ns=0;
        if(fault==6)f.report.inventory_finished_ns=99;if(fault==7)f.report.complete_encountered_set=true;if(fault==8)f.throw_refresh=true;
        LinuxVisibleNetworkMonitor m(f.api());assert(!m.refresh(true,partial).value&&f.stops==1);SystemSnapshot s;m.fill(s,on);assert(!s.linux_network_admission.value&&!s.linux_process_network.value&&f.reads==0);}
    {Fake f;f.report.capture_active=false;f.report.selection_epoch=0;f.report.verified_processes=0;f.report.visible_processes=1;LinuxVisibleNetworkMonitor m(f.api());assert(m.refresh(true,partial).value);
        SystemSnapshot s;m.fill(s,on);assert(s.linux_network_admission.value&&!s.linux_process_network.value&&f.reads==0&&!s.linux_network_admission.value->complete_encountered_set);}
    {Fake f;f.report.excluded[0].pid=8;LinuxVisibleNetworkMonitor m(f.api());assert(m.refresh(true,partial).value);SystemSnapshot s;m.fill(s,on);assert(!s.linux_process_network.value&&!s.linux_network_admission.value&&f.stops==1);}
}
void public_monitor(){Fake f;auto m=MonitorAdmissionAccess::make(f.api());assert(!m.refresh_linux_visible_process_network(false).value&&f.refreshes==0);
    assert(m.refresh_linux_visible_process_network(true,partial).value);auto s=m.sample(options());assert(s.linux_network_admission.value&&s.linux_process_network.value);
    assert(!m.start_linux_process_network(true,{}).value&&!m.update_linux_process_network_selection(true,{}).value&&f.stops==0);
    assert(m.start_linux_process_network(false,{}).support==Support::permission_required);
    auto moved=std::move(m);assert(moved.sample(options()).linux_network_admission.value);assert(!m.sample(options()).linux_network_admission.value);
    assert(!moved.refresh_linux_visible_process_network(true).value);s=moved.sample(options());assert(!s.linux_network_admission.value&&!s.linux_process_network.value);
    moved.stop_linux_process_network();assert(moved.refresh_linux_visible_process_network(true,partial).value);
    auto off=options();off.include_linux_process_network=false;assert(!moved.sample(off).linux_network_admission.value);const auto reads=f.reads;
    assert(!moved.sample(options()).linux_network_admission.value&&f.reads==reads);
    for(auto n:{std::size_t{65},LinuxProcessNetworkSampler::max_processes}){Fake large;large.report.visible_processes=n+1;large.report.verified_processes=n;large.report.excluded[0].pid=2000000000;
        auto scaled=MonitorAdmissionAccess::make(large.api());assert(scaled.refresh_linux_visible_process_network(true,partial).value);auto frame=scaled.sample(options());
        assert(frame.linux_network_admission.value->verified_processes==n&&frame.linux_process_network.value->observed_processes.size()==n);}
}
template<class Accept> void wait_frame(Sampler& s,Accept accept){const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(6);
    while(std::chrono::steady_clock::now()<deadline){auto w=s.latest();if(w&&accept(*w))return;assert(s.running());std::this_thread::sleep_for(std::chrono::milliseconds(5));}throw std::runtime_error("admission worker fixture timeout");}
void worker(){Fake f;auto monitor=MonitorAdmissionAccess::make(f.api());auto s=SamplerAdmissionAccess::make(std::move(monitor),std::chrono::milliseconds(30),128,options());
    assert(!s.refresh_linux_visible_process_network(true,partial).value);assert(!s.start_linux_visible_process_network(false,partial).value&&f.refreshes==0);
    assert(s.start_linux_visible_process_network(true,partial).value);s.start();wait_frame(s,[](const auto& w){return w.linux_network_admission.value&&w.linux_process_network.value;});
    assert(!s.start_linux_visible_process_network(true,partial).value);const auto refreshed=s.refresh_linux_visible_process_network(true,partial);assert(refreshed.value);
    // Context is accessed only inside Monitor's serialized callbacks; no direct
    // mutation of the fake while worker runs. Readers may run concurrently.
    Metric<LinuxNetworkAdmissionChange> changed;std::jthread updater([&]{changed=s.refresh_linux_visible_process_network(true,partial);});(void)s.latest();(void)s.timed_history();updater.join();assert(changed.value);
    assert(!s.refresh_linux_visible_process_network(true).value);wait_frame(s,[](const auto& w){return !w.linux_network_admission.value&&!w.linux_process_network.value;});
    assert(s.running());s.stop();assert(!s.refresh_linux_visible_process_network(true,partial).value);
    s.start();wait_frame(s,[](const auto& w){return !w.linux_network_admission.value&&!w.linux_process_network.value;});s.stop();assert(s.last_error().empty());
    for(const auto& w:s.history())assert(!w.linux_network_admission.value);
    auto disabled=options();disabled.include_linux_process_network=false;Sampler off(std::chrono::milliseconds(30),8,disabled);assert(!off.start_linux_visible_process_network(true).value);
    Fake large;large.report.visible_processes=66;large.report.verified_processes=65;large.report.excluded[0].pid=2000000000;
    auto many=SamplerAdmissionAccess::make(MonitorAdmissionAccess::make(large.api()),std::chrono::milliseconds(30),8,options());
    assert(many.start_linux_visible_process_network(true,partial).value);many.start();wait_frame(many,[](const auto& w){return w.linux_network_admission.value&&w.linux_network_admission.value->verified_processes==65&&w.linux_process_network.value&&w.linux_process_network.value->observed_processes.size()==65;});many.stop();
}
}
int main(){adapter();public_monitor();worker();}
