#include "../src/linux_process_network_monitor.hpp"
#include <pcinsight/monitor.hpp>
#include <pcinsight/sampler.hpp>
#include <cassert>
#include <stdexcept>
#include <thread>
using namespace pcinsight;using namespace pcinsight::detail;
namespace {
struct Fake {
    int starts=0,reads=0,stops=0,updates=0;bool throw_start=false,throw_read=false,throw_update=false;
    Metric<std::uint64_t> result{7,Support::available,{}};
    LinuxProcessNetworkSelectionMode last_mode=LinuxProcessNetworkSelectionMode::strict_single_namespace;
    Metric<LinuxProcessNetworkSnapshot> frame{LinuxProcessNetworkSnapshot{},Support::estimated,"selected fixture only"};
    ProcessNetworkMonitorApi api(){return {this,
        [](void* p,bool authorized,std::span<const LinuxProcessNetworkSelection>)->Metric<std::uint64_t>{auto& f=*static_cast<Fake*>(p);assert(authorized);++f.starts;if(f.throw_start)throw std::runtime_error("start");return f.result;},
        [](void* p)->Metric<LinuxProcessNetworkSnapshot>{auto& f=*static_cast<Fake*>(p);++f.reads;if(f.throw_read)throw std::runtime_error("read");return f.frame;},
        [](void* p) noexcept {++static_cast<Fake*>(p)->stops;},
        [](void* p,bool authorized,std::span<const LinuxProcessNetworkSelection>)->Metric<std::uint64_t>{auto& f=*static_cast<Fake*>(p);assert(authorized);++f.updates;if(f.throw_update)throw std::runtime_error("update");return f.result;},
        [](void* p,bool authorized,std::span<const LinuxProcessNetworkSelection>,LinuxProcessNetworkSelectionMode mode)->Metric<std::uint64_t>{
            auto& f=*static_cast<Fake*>(p);assert(authorized);++f.starts;f.last_mode=mode;
            if(f.frame.value)f.frame.value->coverage=mode==LinuxProcessNetworkSelectionMode::observed_namespace_buckets?LinuxProcessNetworkCoverage::observed_tcp_udp_socket_namespace_buckets:mode==LinuxProcessNetworkSelectionMode::declared_namespace_buckets?LinuxProcessNetworkCoverage::declared_tcp_udp_socket_namespace_buckets:LinuxProcessNetworkCoverage::selected_tcp_udp_socket_operations;
            if(f.throw_start)throw std::runtime_error("start mode");return f.result;}};}
};
SampleOptions minimal(){SampleOptions o;o.include_processes=o.include_disks=o.include_networks=o.include_gpus=false;return o;}
}
int main(){
    auto options=minimal();SystemSnapshot snapshot;
    options.include_linux_process_network=true;
    constexpr auto buckets=LinuxProcessNetworkSelectionMode::declared_namespace_buckets;
    for(const auto mode:{buckets,LinuxProcessNetworkSelectionMode::observed_namespace_buckets}){Fake f;LinuxProcessNetworkMonitor m(f.api());
        const auto coverage=mode==buckets?LinuxProcessNetworkCoverage::declared_tcp_udp_socket_namespace_buckets:LinuxProcessNetworkCoverage::observed_tcp_udp_socket_namespace_buckets;
        assert(!m.start(false,{},buckets).value&&f.starts==0);
        assert(!m.start(true,{},static_cast<LinuxProcessNetworkSelectionMode>(99)).value&&f.starts==0&&f.stops==0);
        assert(m.start(true,{},mode).value&&f.last_mode==mode);m.fill(snapshot,options);
        assert(snapshot.linux_process_network.value->coverage==coverage);
        assert(!m.start(true,{}).value&&f.starts==1&&f.last_mode==mode);assert(!m.start(false,{}).value&&f.stops==0);
        f.result={8,Support::available,{}};assert(m.update_selection(true,{}).value==8);m.fill(snapshot,options);
        assert(snapshot.linux_process_network.value->coverage==coverage);
        auto off=options;off.include_linux_process_network=false;m.fill(snapshot,off);assert(f.stops==1);
        m.fill(snapshot,options);assert(!snapshot.linux_process_network.value&&f.starts==1);
        assert(m.start(true,{}).value&&f.last_mode==LinuxProcessNetworkSelectionMode::strict_single_namespace);
        m.fill(snapshot,options);assert(snapshot.linux_process_network.value->coverage==LinuxProcessNetworkCoverage::selected_tcp_udp_socket_operations);m.stop();}
    {Fake f;auto api=f.api();api.start_mode=nullptr;LinuxProcessNetworkMonitor m(api);
        assert(!m.start(true,{},LinuxProcessNetworkSelectionMode::observed_namespace_buckets).value&&f.starts==0&&f.stops==0);
        assert(!m.start(true,{},buckets).value&&f.starts==0&&f.stops==0);assert(m.start(true,{}).value&&f.starts==1);m.stop();}
    for(const auto mode:{buckets,LinuxProcessNetworkSelectionMode::observed_namespace_buckets})for(unsigned fault=0;fault<4;++fault){Fake f;LinuxProcessNetworkMonitor m(f.api());
        if(fault==0)f.result={{},Support::permission_required,"bucket start denied"};if(fault==1)f.result={0,Support::available,"zero epoch"};
        if(fault==2)f.result={7,Support::estimated,"unverified"};if(fault==3)f.throw_start=true;
        assert(!m.start(true,{},mode).value&&f.starts==1&&f.stops==1);m.fill(snapshot,options);assert(!snapshot.linux_process_network.value);}
    {Fake f;LinuxProcessNetworkMonitor m(f.api());f.frame.value->observed_processes.push_back({999,2000,800,{8,43,200},0,0});
        assert(m.start(true,{},LinuxProcessNetworkSelectionMode::observed_namespace_buckets).value);m.fill(snapshot,options);
        assert(snapshot.linux_process_network.value->processes.empty()&&snapshot.linux_process_network.value->observed_processes.size()==1);
        assert(snapshot.linux_process_network.value->observed_processes[0].process.pid==8);
        f.frame={{},Support::permission_required,"observed denied"};m.fill(snapshot,options);assert(!snapshot.linux_process_network.value);
        f.frame={LinuxProcessNetworkSnapshot{},Support::estimated,"observed recovered"};f.frame.value->coverage=LinuxProcessNetworkCoverage::observed_tcp_udp_socket_namespace_buckets;
        m.fill(snapshot,options);assert(snapshot.linux_process_network.value&&f.starts==1&&f.stops==0);
        f.throw_read=true;m.fill(snapshot,options);assert(!snapshot.linux_process_network.value&&f.stops==1);}
    {Fake f;LinuxProcessNetworkMonitor m(f.api());assert(!m.update_selection(true,{}).value&&f.updates==0);
        assert(m.start(true,{}).value);assert(!m.update_selection(false,{}).value&&f.updates==0&&f.stops==0);
        f.result={8,Support::available,{}};assert(m.update_selection(true,{}).value==8&&f.updates==1&&f.stops==0);
        m.fill(snapshot,options);assert(snapshot.linux_process_network.value&&f.reads==1);m.stop();assert(f.stops==1);
        assert(!m.update_selection(true,{}).value&&f.updates==1);}
    for(const auto mode:{LinuxProcessNetworkSelectionMode::strict_single_namespace,LinuxProcessNetworkSelectionMode::observed_namespace_buckets})for(unsigned fault=0;fault<5;++fault){Fake f;auto api=f.api();if(fault==4)api.update=nullptr;
        LinuxProcessNetworkMonitor m(api);assert(m.start(true,{},mode).value);
        if(fault==0)f.result={{},Support::permission_required,"update denied"};if(fault==1)f.result={0,Support::available,"zero epoch"};
        if(fault==2)f.result={8,Support::estimated,"unverified"};if(fault==3)f.throw_update=true;
        const auto result=m.update_selection(true,{});assert(!result.value&&f.stops==1&&f.updates==(fault==4?0:1));
        m.fill(snapshot,options);assert(!snapshot.linux_process_network.value&&f.reads==0);
        assert(snapshot.linux_process_network.support==(fault==0?Support::permission_required:Support::unsupported));
        assert(!m.update_selection(true,{}).value);m.stop();assert(f.stops==1);}
    options.include_linux_process_network=false;
    {Fake f;LinuxProcessNetworkMonitor m(f.api());m.fill(snapshot,options);assert(!snapshot.linux_process_network.value&&f.starts==0&&f.reads==0);
        options.include_linux_process_network=true;m.fill(snapshot,options);assert(!snapshot.linux_process_network.value&&f.starts==0&&f.reads==0);
        auto denied=m.start(false,{});assert(!denied.value&&denied.support==Support::permission_required&&f.starts==0);
        m.fill(snapshot,options);assert(snapshot.linux_process_network.support==Support::permission_required&&f.reads==0);
        assert(m.start(true,{}).value);m.fill(snapshot,options);assert(metric_is_known(snapshot.linux_process_network)&&f.starts==1&&f.reads==1);
        assert(!m.start(true,{}).value&&f.starts==1);assert(!m.start(false,{}).value);m.fill(snapshot,options);assert(f.reads==2);
        f.frame={{},Support::permission_required,"sample denied"};m.fill(snapshot,options);assert(!snapshot.linux_process_network.value&&snapshot.linux_process_network.note=="sample denied");
        f.frame={LinuxProcessNetworkSnapshot{},Support::estimated,"recovered"};m.fill(snapshot,options);assert(snapshot.linux_process_network.value);
        options.include_linux_process_network=false;m.fill(snapshot,options);assert(!snapshot.linux_process_network.value&&f.stops==1);
        options.include_linux_process_network=true;m.fill(snapshot,options);assert(!snapshot.linux_process_network.value&&f.starts==1&&f.reads==4);
        m.stop();assert(f.stops==1);
    }
    for(unsigned fault=0;fault<4;++fault){Fake f;LinuxProcessNetworkMonitor m(f.api());
        if(fault==0)f.result={{},Support::permission_required,"start denied"};if(fault==1)f.result={0,Support::available,"zero epoch"};
        if(fault==2)f.result={7,Support::estimated,"unverified"};if(fault==3)f.throw_start=true;
        assert(!m.start(true,{}).value&&f.stops==1);m.fill(snapshot,options);assert(!snapshot.linux_process_network.value&&f.reads==0);
        assert(snapshot.linux_process_network.support==(fault==0?Support::permission_required:Support::unsupported));
    }
    {Fake f;LinuxProcessNetworkMonitor m(f.api());assert(m.start(true,{}).value);f.throw_read=true;m.fill(snapshot,options);
        assert(!snapshot.linux_process_network.value&&f.stops==1);f.throw_read=false;m.fill(snapshot,options);assert(f.reads==1&&!snapshot.linux_process_network.value);}
    {LinuxProcessNetworkMonitor unavailable(ProcessNetworkMonitorApi{});assert(!unavailable.start(true,{}).value);unavailable.fill(snapshot,options);assert(!snapshot.linux_process_network.value);}
    // Never modify the legacy per-process/user/app totals when attaching the report.
    {Fake f;LinuxProcessNetworkMonitor m(f.api());assert(m.start(true,{}).value);snapshot.processes.emplace_back();snapshot.user_sessions.emplace_back();snapshot.app_history.emplace_back();
        snapshot.processes[0].network_bytes_per_second={123,Support::estimated,"process sentinel"};
        snapshot.user_sessions[0].network_bytes_per_second={456,Support::estimated,"user sentinel"};snapshot.app_history[0].network_bytes=789;
        m.fill(snapshot,options);assert(snapshot.processes.size()==1&&snapshot.user_sessions.size()==1&&snapshot.app_history.size()==1);
        assert(snapshot.processes[0].network_bytes_per_second.value==123&&snapshot.processes[0].network_bytes_per_second.note=="process sentinel");
        assert(snapshot.user_sessions[0].network_bytes_per_second.value==456&&snapshot.app_history[0].network_bytes==789);}
    // Public surface: flag alone, denied setup and invalid selection never capture.
    {Monitor m;auto s=m.sample(options);assert(!s.linux_process_network.value);assert(!m.start_linux_process_network(false,{}).value);
        assert(m.update_linux_process_network_selection(false,{}).support==Support::permission_required);
        assert(!m.update_linux_process_network_selection(true,{}).value);
        s=m.sample(options);assert(s.linux_process_network.support==Support::permission_required);m.stop_linux_process_network();
        auto moved=std::move(m);assert(!m.update_linux_process_network_selection(true,{}).value);
        assert(!moved.sample(options).linux_process_network.value);assert(!m.sample(options).linux_process_network.value);}
    {Monitor m;assert(m.start_linux_process_network(false,{},buckets).support==Support::permission_required);
        assert(!m.start_linux_process_network(true,{},static_cast<LinuxProcessNetworkSelectionMode>(99)).value);
        assert(!m.sample(options).linux_process_network.value);m.stop_linux_process_network();}
    {Sampler s(std::chrono::milliseconds(20),2,minimal());assert(!s.start_linux_process_network(true,{}).value);assert(!s.start_linux_process_network(false,{}).value);
        assert(!s.start_linux_process_network(true,{},LinuxProcessNetworkSelectionMode::observed_namespace_buckets).value);
        assert(!s.start_linux_process_network(true,{},buckets).value);assert(s.start_linux_process_network(false,{},buckets).support==Support::permission_required);s.stop();}
    {Sampler s(std::chrono::milliseconds(20),2,options);assert(!s.start_linux_process_network(true,{}).value);s.start();
        assert(!s.start_linux_process_network(true,{},LinuxProcessNetworkSelectionMode::observed_namespace_buckets).value);
        assert(!s.start_linux_process_network(true,{}).value); // Running rejects before touching descriptors.
        assert(!s.start_linux_process_network(true,{},buckets).value);
        const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(5);
        while(!s.latest()&&s.running()&&std::chrono::steady_clock::now()<deadline)std::this_thread::sleep_for(std::chrono::milliseconds(5));
        const auto latest=s.latest();s.stop();assert(latest&&!latest->linux_process_network.value&&s.last_error().empty());}
}
