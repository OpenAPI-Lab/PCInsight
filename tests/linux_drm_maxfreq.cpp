#include "../src/linux_drm_client.hpp"
#include "../src/linux_drm_monitor.hpp"
#include <cassert>
#include <cmath>
#include <iostream>
#include <limits>
using namespace pcinsight;
using namespace pcinsight::detail;
using namespace std::chrono_literals;
std::string header(){return "drm-driver: fixture\ndrm-pdev: 0000:03:00.0\ndrm-client-id: 1\n";}
std::string fields(std::uint64_t busy,std::string frequency="100 MHz",std::uint64_t capacity=1){
    return "drm-cycles-copy: "+std::to_string(busy)+"\ndrm-maxfreq-copy: "+frequency+"\ndrm-engine-capacity-copy: "+std::to_string(capacity)+"\n";
}
LinuxDrmClientUsage raw(std::uint64_t busy=0,std::chrono::nanoseconds time=0ns,std::string frequency="100 MHz",std::uint64_t capacity=1){
    auto v=parse_linux_drm_fdinfo(header()+fields(busy,frequency,capacity));assert(v);
    v->pid=42;v->descriptor=5;v->process_start_ticks=100;v->captured_at=std::chrono::steady_clock::time_point(time);return *v;
}
Metric<LinuxDrmClientUsage> available(LinuxDrmClientUsage v){return {std::move(v),Support::available,"fixture"};}
int main(){
    // Regression: the old parser rejected the standardized maxfreq alternative.
    assert(parse_linux_drm_fdinfo(header()+fields(100)));
    const auto kind=LinuxDrmCounterKind::cycles_max_frequency;
    auto v=raw();assert(v.engines[0].counter_kind==kind && v.engines[0].max_frequency_hz.value==100000000 && !v.engines[0].total_cycles.value && !v.engines[0].utilization_percent.value);
    for(const auto* frequency:{"100000000 Hz","100000 KHz","100 MHz","100\tMHz"})assert(raw(0,0ns,frequency).engines[0].max_frequency_hz.value==100000000);
    assert(raw(0,0ns,"18446744073709551615 Hz").engines[0].max_frequency_hz.value==std::numeric_limits<std::uint64_t>::max());
    for(const auto* frequency:{"0 Hz","100","100MHz","100 kHz","100 GHz","100 MHz junk","-1 Hz","+1 Hz","1.5 MHz","18446744073709551616 Hz","18446744073709552 KHz","18446744073710 MHz",""})assert(!parse_linux_drm_fdinfo(header()+fields(1,frequency)));
    for(const auto* extra:{"drm-maxfreq-copy: 100 MHz\n","drm-total-cycles-copy: 100\n","drm-maxfreq-other: 100 MHz\n","drm-engine-capacity-other: 1\n"})assert(!parse_linux_drm_fdinfo(header()+fields(1)+extra));
    assert(!parse_linux_drm_fdinfo(header()+"drm-maxfreq-copy: 100 MHz\n"));
    auto both=parse_linux_drm_fdinfo(header()+fields(1)+"drm-engine-copy: 100 ns\n");assert(both && both->engines[0].counter_kind==kind && both->engines[0].busy_nanoseconds==100);
    // Full uint64 conversion bounds, no wrap while scaling units.
    assert(raw(0,0ns,"18446744073709551 KHz").engines[0].max_frequency_hz.value==18446744073709551000ULL);
    assert(raw(0,0ns,"18446744073709 MHz").engines[0].max_frequency_hz.value==18446744073709000000ULL);
    std::optional<LinuxDrmClientUsage> previous;
    auto sample=[&](LinuxDrmClientUsage value){return update_linux_drm_client_usage(previous,available(std::move(value)));};
    auto first=sample(raw());assert(!first.value->engines[0].utilization_percent.value);
    auto next=sample(raw(50000000,1s));assert(next.value->engines[0].utilization_percent.value==50 && next.value->sample_interval.value==1s);
    next=sample(raw(50000000,2s));assert(next.value->engines[0].utilization_percent.value==0);
    next=sample(raw(1,3s));assert(!next.value->engines[0].utilization_percent.value && previous->engines[0].busy_cycles.value==50000000 && next.value->engines[0].busy_cycles.value==1);
    next=sample(raw(75000000,4s));assert(next.value->engines[0].utilization_percent.value==25);
    next=sample(raw(90000000,4s));assert(!next.value->engines[0].utilization_percent.value && previous->engines[0].busy_cycles.value==75000000);
    next=sample(raw(100000000,5s));assert(next.value->engines[0].utilization_percent.value==25);
    next=sample(raw(250000000,6s));assert(!next.value->engines[0].utilization_percent.value); // no clamp
    next=sample(raw(350000000,7s));assert(next.value->engines[0].utilization_percent.value==100);
    next=sample(raw(400000000,8s,"200 MHz"));assert(!next.value->engines[0].utilization_percent.value); // frequency change primes
    next=sample(raw(500000000,9s,"200 MHz"));assert(next.value->engines[0].utilization_percent.value==50);
    next=sample(raw(600000000,10s,"200 MHz",2));assert(!next.value->engines[0].utilization_percent.value);
    next=sample(raw(800000000,11s,"200 MHz",2));assert(next.value->engines[0].utilization_percent.value==50);
    next=sample(raw(900000000,10s,"200 MHz",2));assert(!next.value->engines[0].utilization_percent.value); // backward CPU clock
    next=sample(raw(1000000000,11s,"200 MHz",2));assert(next.value->engines[0].utilization_percent.value==25);
    auto changed=raw(1100000000,12s,"200 MHz",2);++changed.process_start_ticks;next=sample(changed);assert(!next.value->engines[0].utilization_percent.value);
    const auto max=std::numeric_limits<std::uint64_t>::max();previous=raw(max-50000000);next=sample(raw(max,1s));assert(next.value->engines[0].utilization_percent.value==50);
    previous=raw(0,0ns,"1 Hz");next=sample(raw(1,1s,"1 Hz"));assert(next.value->engines[0].utilization_percent.value==100);
    previous=raw(0,0ns,"1 MHz");next=sample(raw(1000,1ms,"1 MHz"));assert(next.value->engines[0].utilization_percent.value==100);

    LinuxDrmGpuSampler aggregate;auto a=available(raw()),b=a;b.value->descriptor=9;
    aggregate.sample({a,b});a=available(raw(50000000,1s));b=a;b.value->descriptor=9;
    auto groups=aggregate.sample({a,b});auto engine=groups.value->front().engines[0];
    assert(engine.counter_kind.value==kind && engine.max_frequency_hz.value==100000000 && engine.summed_client_frequency_percent.value==50 && !engine.summed_client_cycle_percent.value && !engine.summed_client_active_percent.value && engine.observed_clients==1 && engine.measured_clients==1 && engine.shortest_interval.value==1s);
    aggregate.reset();a=available(raw());b=a;b.value->client_id=2;aggregate.sample({a,b});a=available(raw(80000000,1s));b=a;b.value->client_id=2;
    groups=aggregate.sample({a,b});assert(groups.value->front().engines[0].summed_client_frequency_percent.value==160); // observed sum, not GPU utilization
    aggregate.reset();a=available(raw());b=a;b.value->client_id=2;b.value->engines[0].max_frequency_hz.value=200000000;
    groups=aggregate.sample({a,b});assert(!groups.value->front().engines[0].counter_kind.value && !groups.value->front().engines[0].max_frequency_hz.value);
    a=available(raw(50000000,1s));b=available(raw(50000000,1s,"200 MHz"));b.value->client_id=2;groups=aggregate.sample({a,b});engine=groups.value->front().engines[0];assert(!engine.summed_client_frequency_percent.value && !engine.summed_client_active_percent.value && !engine.summed_client_cycle_percent.value);
    aggregate.reset();a=available(raw());b=a;b.value->engines[0].counter_kind=LinuxDrmCounterKind::nanoseconds;b.value->client_id=2;groups=aggregate.sample({a,b});assert(!groups.value->front().engines[0].counter_kind.value);
    aggregate.reset();a=available(raw());b=a;b.value->engines[0].max_frequency_hz.value=200000000;assert(!aggregate.sample({a,b}).value);
    b=a;++*b.value->engines[0].busy_cycles.value;assert(!aggregate.sample({a,b}).value); // equal timestamp conflict
    b=a;b.value->engines[0].max_frequency_hz.support=Support::estimated;assert(!aggregate.sample({b}).value);
    b=a;b.value->engines[0].max_frequency_hz.value=0;assert(!aggregate.sample({b}).value);
    b=a;b.value->engines[0].total_cycles={1,Support::available,"invalid combination"};assert(!aggregate.sample({b}).value);
    // Do not trust percentages injected by the consumer.
    aggregate.sample({a});b=available(raw(50000000,1s));b.value->engines[0].utilization_percent={99,Support::estimated,"forged"};groups=aggregate.sample({b});assert(groups.value->front().engines[0].summed_client_frequency_percent.value==50);
    unsigned tick=0;LinuxDrmMonitor bridge([&](const LinuxDrmDiscoveryOptions&)->Metric<LinuxDrmDiscovery>{LinuxDrmDiscovery d;d.scope="fixture";d.observations.push_back(available(raw(50000000*tick,std::chrono::seconds(tick))));++tick;return {d,Support::estimated,"fixture"};});
    SystemSnapshot snapshot;GpuInfo gpu;gpu.id="card0";gpu.physical_location={std::string("0000:03:00.0"),Support::available,"DRM"};snapshot.gpus.push_back(gpu);SampleOptions options;options.include_linux_drm_clients=true;
    bridge.fill(snapshot,options);bridge.fill(snapshot,options);assert(snapshot.linux_drm.aggregates.value->front().engines[0].summed_client_frequency_percent.value==50 && snapshot.linux_drm.aggregates.value->front().inventory_gpu_id.value=="card0");
    assert(!snapshot.gpus[0].utilization_percent.value); // Never replace hardware utilization.
    std::cout<<"DRM maximum-frequency tests passed\n";
}
