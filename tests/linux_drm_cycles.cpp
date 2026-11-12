#include "../src/linux_drm_client.hpp"
#include "../src/linux_drm_monitor.hpp"
#include <cassert>
#include <iostream>
#include <limits>
using namespace pcinsight;
using namespace pcinsight::detail;
using namespace std::chrono_literals;
std::string header(){return "drm-driver: xe\ndrm-pdev: 0000:03:00.0\ndrm-client-id: 1\n";}
LinuxDrmClientUsage raw(std::uint64_t busy=100,std::uint64_t total=1000,std::uint64_t cap=1){
    auto v=parse_linux_drm_fdinfo(header()+"drm-cycles-rcs: "+std::to_string(busy)+"\ndrm-total-cycles-rcs: "+std::to_string(total)+"\ndrm-engine-capacity-rcs: "+std::to_string(cap)+"\n");assert(v);v->pid=42;v->descriptor=5;v->process_start_ticks=100;v->captured_at={};return *v;
}
Metric<LinuxDrmClientUsage> available(LinuxDrmClientUsage v){return {std::move(v),Support::available,"fixture GPU cycles"};}
int main(){
    auto v=raw();assert(v.engines[0].counter_kind==LinuxDrmCounterKind::cycles && v.engines[0].busy_cycles.value==100 && v.engines[0].total_cycles.value==1000 && v.engines[0].busy_nanoseconds==0 && v.engines[0].engine_class.value==LinuxDrmEngineClass::graphics_compute);
    for(const auto* fields:{"drm-cycles-rcs: 1\n", "drm-total-cycles-rcs: 1\n", "drm-cycles-rcs: 1\ndrm-total-cycles-rcs: -1\n", "drm-cycles-rcs: +1\ndrm-total-cycles-rcs: 2\n", "drm-cycles-rcs: 1 ns\ndrm-total-cycles-rcs: 2\n", "drm-cycles-rcs: 1\ndrm-total-cycles-rcs: 18446744073709551616\n", "drm-cycles-rcs: 1\ndrm-cycles-rcs: 1\ndrm-total-cycles-rcs: 2\n", "drm-cycles-rcs: 1\ndrm-total-cycles-rcs: 2\ndrm-total-cycles-rcs: 2\n", "drm-cycles-rcs: 1\ndrm-total-cycles-rcs: 2\ndrm-maxfreq-rcs: 100 MHz\n", "drm-cycles-bad name: 1\ndrm-total-cycles-bad name: 2\n"})assert(!parse_linux_drm_fdinfo(header()+fields));
    auto ns=parse_linux_drm_fdinfo(header()+"drm-engine-rcs: 25 ns\ndrm-cycles-rcs: 1\ndrm-total-cycles-rcs: 2\n");assert(ns && ns->engines[0].counter_kind==LinuxDrmCounterKind::nanoseconds && ns->engines[0].busy_nanoseconds==25);
    auto frequency=parse_linux_drm_fdinfo(header()+"drm-engine-rcs: 25 ns\ndrm-cycles-rcs: 1\ndrm-maxfreq-rcs: 100 MHz\n");
    assert(frequency && frequency->engines[0].counter_kind==LinuxDrmCounterKind::cycles_max_frequency);
    std::optional<LinuxDrmClientUsage> previous;auto first=update_linux_drm_client_usage(previous,available(raw()));assert(!first.value->engines[0].utilization_percent.value);
    // GPU clock domain advances even when CPU observation timestamps are equal.
    auto next=update_linux_drm_client_usage(previous,available(raw(600,2000)));assert(next.value->engines[0].utilization_percent.value==50 && !next.value->sample_interval.value);
    next=update_linux_drm_client_usage(previous,available(raw(600,2000)));assert(!next.value->engines[0].utilization_percent.value);
    next=update_linux_drm_client_usage(previous,available(raw(600,3000)));assert(next.value->engines[0].utilization_percent.value==0);
    next=update_linux_drm_client_usage(previous,available(raw(1,4000)));assert(!next.value->engines[0].utilization_percent.value && previous->engines[0].busy_cycles.value==600 && next.value->engines[0].busy_cycles.value==1);
    next=update_linux_drm_client_usage(previous,available(raw(700,5000)));assert(next.value->engines[0].utilization_percent.value==10);
    // A GPU total-counter reset establishes a new epoch, not a fabricated rate.
    next=update_linux_drm_client_usage(previous,available(raw(0,1)));assert(!next.value->engines[0].utilization_percent.value);
    next=update_linux_drm_client_usage(previous,available(raw(50,101)));assert(next.value->engines[0].utilization_percent.value==50);
    previous=raw(100,1000);next=update_linux_drm_client_usage(previous,available(raw(150,1000)));assert(!next.value->engines[0].utilization_percent.value && previous->engines[0].busy_cycles.value==100);
    next=update_linux_drm_client_usage(previous,available(raw(200,2000)));assert(next.value->engines[0].utilization_percent.value==10);
    previous=raw(0,0,2);next=update_linux_drm_client_usage(previous,available(raw(400,1000,2)));assert(next.value->engines[0].utilization_percent.value==20);
    next=update_linux_drm_client_usage(previous,available(raw(5000,2000,2)));assert(!next.value->engines[0].utilization_percent.value);
    const auto max=std::numeric_limits<std::uint64_t>::max();previous=raw(max-100,max-1000);next=update_linux_drm_client_usage(previous,available(raw(max-50,max-900)));assert(next.value->engines[0].utilization_percent.value==50);
    LinuxDrmGpuSampler aggregate;auto a=available(raw()),b=a;b.value->descriptor=9;
    aggregate.sample({a,b});a=available(raw(600,2000));b=a;b.value->descriptor=9;auto groups=aggregate.sample({a,b});const auto& e=groups.value->front().engines[0];assert(e.summed_client_cycle_percent.value==50 && !e.summed_client_active_percent.value && e.measured_clients==1 && !e.shortest_interval.value);
    // Mixed time/cycle clients in one class cannot be summed.
    aggregate.reset();auto time=*ns;time.pid=42;time.descriptor=6;time.client_id=2;time.process_start_ticks=100;time.captured_at={};groups=aggregate.sample({available(raw()),available(time)});assert(!groups.value->front().engines[0].counter_kind.value);
    time.captured_at+=1s;time.engines[0].busy_nanoseconds+=100000000;groups=aggregate.sample({available(raw(600,2000)),available(time)});assert(!groups.value->front().engines[0].summed_client_cycle_percent.value && !groups.value->front().engines[0].summed_client_active_percent.value);
    // Shared-client snapshots with conflicting GPU denominators fail closed.
    aggregate.reset();a=available(raw());b=a;++*b.value->engines[0].total_cycles.value;assert(!aggregate.sample({a,b}).value);
    b=a;b.value->engines[0].total_cycles.support=Support::estimated;assert(!aggregate.sample({b}).value);
    // Reader identity checks still apply to a cycle-only record.
    DrmClientReadApi api{
        [](const std::string& path)->Metric<std::string>{if(path.ends_with("/stat"))return {std::string("42 (fixture) S 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 1 0 100\n"),Support::available,"stat"};return {header()+"drm-cycles-rcs: 100\ndrm-total-cycles-rcs: 1000\n",Support::available,"fdinfo"};},
        [](std::uint32_t,std::uint32_t)->Metric<std::string>{return {std::string("0000:03:00.0"),Support::available,"fixture PCI"};},[]{return std::chrono::steady_clock::time_point{};}};
    assert(pcinsight::detail::read_linux_drm_client_usage(42,5,api).value->engines[0].counter_kind==LinuxDrmCounterKind::cycles);
    unsigned tick=0;LinuxDrmMonitor bridge([&](const LinuxDrmDiscoveryOptions&)->Metric<LinuxDrmDiscovery>{LinuxDrmDiscovery d;d.scope="fixture";d.observations.push_back(available(raw(100+500*tick,1000+1000*tick)));++tick;return {d,Support::estimated,"fixture"};});
    SystemSnapshot snapshot;GpuInfo gpu;gpu.id="card0";gpu.physical_location={std::string("0000:03:00.0"),Support::available,"DRM"};snapshot.gpus.push_back(gpu);SampleOptions options;options.include_linux_drm_clients=true;bridge.fill(snapshot,options);bridge.fill(snapshot,options);assert(snapshot.linux_drm.aggregates.value->front().engines[0].summed_client_cycle_percent.value==50 && snapshot.linux_drm.aggregates.value->front().inventory_gpu_id.value=="card0");
    std::cout<<"DRM GPU-cycle tests passed\n";
}
