#include "../src/linux_drm_monitor.hpp"
#include "pcinsight/monitor.hpp"
#include "pcinsight/sampler.hpp"
#include <cassert>
#include <chrono>
#include <stdexcept>
#include <thread>
#include <iostream>
using namespace pcinsight;
using namespace pcinsight::detail;
using namespace std::chrono_literals;
SystemSnapshot inventory(){SystemSnapshot s;GpuInfo g;g.id="card0";g.physical_location={std::string("0000:03:00.0"),Support::available,"DRM"};g.utilization_percent={17.0,Support::available,"hardware"};g.engines.push_back({"overall",g.utilization_percent});s.gpus.push_back(g);g.id="card1";g.physical_location.value="0001:03:00.0";s.gpus.push_back(g);return s;}
const LinuxDrmGpuAggregate& group(const SystemSnapshot& s){assert(s.linux_drm.aggregates.value && s.linux_drm.aggregates.value->size()==1);return s.linux_drm.aggregates.value->front();}
int main(){
    unsigned calls=0,phase=0;int mode=0;LinuxDrmDiscoveryOptions passed;
    LinuxDrmMonitor bridge([&](const LinuxDrmDiscoveryOptions& limits)->Metric<LinuxDrmDiscovery>{
        ++calls;passed=limits;if(mode==1)return {{},Support::permission_required,"fixture denied"};if(mode==2)throw std::runtime_error("fixture throw");
        LinuxDrmDiscovery d;d.processes_seen=2;d.descriptors_seen=3;d.drm_candidates=2;d.permission_denials=mode==3?1:0;d.limit_reached=mode==3;d.scope="fixture observed subset";
        LinuxDrmClientUsage c;c.pid=42;c.descriptor=5;c.client_id=7;c.process_start_ticks=100;c.driver="amdgpu";c.pci_address="00000000:03:00.0";c.captured_at=std::chrono::steady_clock::time_point{std::chrono::seconds{phase++}};c.engines.push_back({"gfx",phase*100000000ULL,1,{}});
        d.observations.push_back({c,Support::available,"fixture raw"});c.descriptor=9;d.observations.push_back({c,Support::available,"duplicate"});if(mode==4)++d.observations.back().value->engines.front().busy_nanoseconds;
        return {d,Support::estimated,"fixture discovery"};
    });
    SampleOptions o;assert(!o.include_linux_drm_clients);auto s=inventory();bridge.fill(s,o);assert(!calls && !s.linux_drm.discovery.value && s.linux_drm.discovery.note.find("disabled")!=std::string::npos);
    o.include_linux_drm_clients=true;o.include_gpus=false;bridge.fill(s,o);assert(!calls && !s.linux_drm.discovery.value);
    o.include_gpus=true;o.linux_drm_limits.max_processes=8;o.linux_drm_limits.max_descriptors=64;o.linux_drm_limits.max_observations=8;o.linux_drm_limits.max_duration=20ms;
    bridge.fill(s,o);assert(calls==1 && passed==o.linux_drm_limits && s.linux_drm.discovery.value->descriptors_seen==3);
    assert(group(s).inventory_gpu_id.value=="card0" && group(s).unique_clients==1 && !group(s).engines[0].summed_client_active_percent.value);
    assert(group(s).engines[0].engine_class.value==LinuxDrmEngineClass::graphics_compute);
    bridge.fill(s,o);assert(group(s).engines[0].summed_client_active_percent.value==10);
    assert(s.gpus[0].utilization_percent.value==17 && s.gpus[0].engines.size()==1 && s.gpus[0].engines[0].name=="overall");
    // Inventory order does not reset, but changed identity does.
    std::swap(s.gpus[0],s.gpus[1]);bridge.fill(s,o);assert(group(s).engines[0].summed_client_active_percent.value==10);
    s.gpus[1].id="new-card";bridge.fill(s,o);assert(group(s).inventory_gpu_id.value=="new-card" && !group(s).engines[0].summed_client_active_percent.value);
    // Duplicate PCI, estimated PCI, missing GPU and duplicate output IDs never match.
    for(int test=0;test<4;++test){s=inventory();if(test==0)s.gpus[1].physical_location=s.gpus[0].physical_location;if(test==1)s.gpus[0].physical_location.support=Support::estimated;if(test==2)s.gpus.clear();if(test==3)s.gpus[1].id="card0";bridge.fill(s,o);assert(!group(s).inventory_gpu_id.value);}
    s=inventory();bridge.fill(s,o);bridge.fill(s,o);assert(group(s).engines[0].summed_client_active_percent.value==10);
    mode=3;bridge.fill(s,o);assert(s.linux_drm.discovery.value->limit_reached && s.linux_drm.discovery.value->permission_denials==1 && group(s).engines[0].summed_client_active_percent.value==10);mode=0;
    // Opt-out, GPU suppression and changed limits must re-prime after reactivation.
    o.include_linux_drm_clients=false;auto before=calls;bridge.fill(s,o);assert(calls==before && !s.linux_drm.aggregates.value);o.include_linux_drm_clients=true;bridge.fill(s,o);assert(!group(s).engines[0].summed_client_active_percent.value);
    o.include_gpus=false;before=calls;bridge.fill(s,o);assert(calls==before);o.include_gpus=true;bridge.fill(s,o);assert(!group(s).engines[0].summed_client_active_percent.value);
    ++o.linux_drm_limits.max_descriptors;bridge.fill(s,o);assert(!group(s).engines[0].summed_client_active_percent.value);
    mode=1;bridge.fill(s,o);assert(!s.linux_drm.discovery.value && s.linux_drm.discovery.support==Support::permission_required && !s.linux_drm.aggregates.value);mode=0;bridge.fill(s,o);assert(!group(s).engines[0].summed_client_active_percent.value);
    mode=2;bool threw=false;try{bridge.fill(s,o);}catch(const std::runtime_error&){threw=true;}assert(threw);mode=0;bridge.fill(s,o);assert(!group(s).engines[0].summed_client_active_percent.value);
    mode=4;bridge.fill(s,o);assert(s.linux_drm.discovery.value && !s.linux_drm.aggregates.value);mode=0;bridge.fill(s,o);assert(!group(s).engines[0].summed_client_active_percent.value);
    // Native Monitor and background Sampler wire the same report, without raw histories.
    o.include_processes=false;o.include_disks=false;o.include_networks=false;o.include_disk_media=false;
    Monitor monitor;auto native=monitor.sample(o);
#ifdef __linux__
    if(native.linux_drm.discovery.value)assert(native.linux_drm.discovery.support==Support::estimated);
    else assert(!native.linux_drm.aggregates.value);
#else
    assert(!native.linux_drm.discovery.value && native.linux_drm.discovery.support==Support::unsupported);
#endif
    auto disabled=o;disabled.include_linux_drm_clients=false;assert(!monitor.sample(disabled).linux_drm.discovery.value);
    Sampler worker(10ms,2,o);worker.start();const auto deadline=std::chrono::steady_clock::now()+5s;
    while(!worker.latest() && worker.running() && std::chrono::steady_clock::now()<deadline)std::this_thread::sleep_for(5ms);
    worker.stop();assert(worker.latest() && worker.last_error().empty() && worker.history().size()<=2);
    assert(!worker.latest()->linux_drm.discovery.note.empty());
    std::cout<<"Monitor DRM opt-in integration tests passed\n";
}
