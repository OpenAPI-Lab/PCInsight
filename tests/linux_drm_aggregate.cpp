#include "pcinsight/linux_drm.hpp"
#include <cassert>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>
using namespace pcinsight;
using namespace std::chrono_literals;
using Row=Metric<LinuxDrmClientUsage>;
Row row(std::uint64_t client=1,std::uint64_t busy=0,std::int64_t time=0){
    LinuxDrmClientUsage v;v.pid=42;v.descriptor=5;v.process_start_ticks=100;v.client_id=client;v.driver="amdgpu";v.pci_address="0000:03:00.0";
    v.captured_at=std::chrono::steady_clock::time_point{std::chrono::nanoseconds{time}};
    v.engines.push_back({"gfx",busy,1,{999.0,Support::estimated,"must be ignored"}});
    return {v,Support::available,"synthetic raw counters"};
}
const LinuxDrmEngineAggregate& engine(const Metric<std::vector<LinuxDrmGpuAggregate>>& r,std::size_t gpu=0){assert(r.value && r.value->size()>gpu && !r.value->at(gpu).engines.empty());return r.value->at(gpu).engines[0];}
void percent(const Metric<std::vector<LinuxDrmGpuAggregate>>& r,double expected){assert(engine(r).summed_client_active_percent.value && std::abs(*engine(r).summed_client_active_percent.value-expected)<1e-8);}
int main(){
    LinuxDrmGpuSampler s;auto a=row(),dup=a;dup.value->descriptor=9;
    auto r=s.sample({a,dup,row(2)});assert(r.support==Support::estimated && r.value->at(0).input_records==3 && r.value->at(0).unique_clients==2 && !engine(r).summed_client_active_percent.value);
    a=row(1,300000000,1000000000);dup=a;dup.value->descriptor=9;
    r=s.sample({a,dup,row(2,200000000,1000000000)});percent(r,50);assert(engine(r).observed_clients==2 && engine(r).measured_clients==2);
    assert(engine(r).shortest_interval.value==1s && engine(r).longest_interval.value==1s);
    a.value->captured_at+=1s;dup=a;dup.value->descriptor=9;auto b=row(2,200000000,2000000000);r=s.sample({a,dup,b});percent(r,0);
    r=s.sample({row(1,400000000,3000000000),row(2,300000000,3000000000),row(3,0,3000000000)});assert(!engine(r).summed_client_active_percent.value && engine(r).measured_clients==2 && engine(r).observed_clients==3);
    r=s.sample({row(1,500000000,4000000000),row(2,400000000,4000000000),row(3,100000000,4000000000)});percent(r,30);
    // Missing clients are removed; reappearance requires a new baseline.
    s.sample({row(1,600000000,5000000000)});r=s.sample({row(1,700000000,6000000000),row(2,500000000,6000000000)});assert(!engine(r).summed_client_active_percent.value);
    // A sum is not hardware utilization: 2 independent 80% rates are reported as 160.
    s.reset();s.sample({row(1),row(2)});r=s.sample({row(1,800000000,1000000000),row(2,800000000,1000000000)});percent(r,160);
    // Different observation intervals are explicit, not claimed to be a common window.
    s.reset();s.sample({row(1),row(2)});r=s.sample({row(1,300000000,1000000000),row(2,400000000,2000000000)});percent(r,50);assert(engine(r).shortest_interval.value==1s && engine(r).longest_interval.value==2s);assert(r.value->at(0).earliest_observation<r.value->at(0).latest_observation);
    // Duplicate observations select one latest coherent snapshot, regardless of order.
    s.reset();s.sample({row()});a=row(1,200000000,800000000);dup=row(1,300000000,1000000000);dup.value->descriptor=9;r=s.sample({dup,a});percent(r,30);assert(r.value->at(0).unique_clients==1 && r.value->at(0).input_records==2);
    // PCI domains are part of the dedup key; canonical spelling aliases are not.
    s.reset();a=row();dup=a;dup.value->pci_address="0001:03:00.0";r=s.sample({a,dup});assert(r.value->size()==2);
    s.reset();dup.value->pci_address="00000000:03:00.0";r=s.sample({a,dup});assert(r.value->size()==1 && r.value->at(0).unique_clients==1);
    // Shared client across processes is one client, but an owner change re-primes.
    s.reset();s.sample({row()});a=row(1,100000000,1000000000);dup=a;dup.value->pid=43;r=s.sample({a,dup});assert(r.value->at(0).unique_clients==1 && !engine(r).summed_client_active_percent.value);
    // Different engine order in duplicates is normalized; distinct native engines survive.
    a=row();a.value->engines.push_back({"compute2",0,2,{}});dup=a;std::reverse(dup.value->engines.begin(),dup.value->engines.end());s.reset();r=s.sample({a,dup});assert(r.value->at(0).engines.size()==2);
    // Different capacities across clients invalidate the class sum, not masquerade as 0.
    s.reset();a=row();b=row(2);b.value->engines[0].capacity=2;s.sample({a,b});a=row(1,300000000,1000000000);b=row(2,400000000,1000000000);b.value->engines[0].capacity=2;r=s.sample({a,b});assert(!engine(r).capacity.value && !engine(r).summed_client_active_percent.value);
    // Matching capacity is normalized per client.
    s.reset();a=row();b=row(2);a.value->engines[0].capacity=b.value->engines[0].capacity=2;s.sample({a,b});a.value->engines[0].busy_nanoseconds=600000000;b.value->engines[0].busy_nanoseconds=400000000;a.value->captured_at+=1s;b.value->captured_at+=1s;r=s.sample({a,b});percent(r,50);
    // A single invalid delta prevents a partial class sum.
    s.reset();s.sample({row(1),row(2)});r=s.sample({row(1,2000000000,1000000000),row(2,100000000,1000000000)});assert(!engine(r).summed_client_active_percent.value && engine(r).measured_clients==1);
    // Counter regression retains the high-water mark until recovery.
    s.reset();s.sample({row(1,500000000)});r=s.sample({row(1,1,1000000000)});assert(!engine(r).summed_client_active_percent.value);r=s.sample({row(1,600000000,2000000000)});percent(r,10);
    // Invalid/conflicting batches fail atomically and reset all baselines.
    for(int mode=0;mode<14;++mode){s.reset();s.sample({row()});a=row(1,200000000,1000000000);dup=a;
        if(mode==0){dup.value.reset();dup.support=Support::permission_required;}
        if(mode==1)dup.value->driver="i915";
        if(mode==2)dup.value->engines[0].capacity=2;
        if(mode==3)++dup.value->engines[0].busy_nanoseconds;
        if(mode==4){dup.value->captured_at+=1s;--dup.value->engines[0].busy_nanoseconds;}
        if(mode==5)dup.value->engines.push_back(dup.value->engines[0]);
        if(mode==6)dup.value->pci_address="bad";
        if(mode==7)dup.value->engines[0].capacity=0;
        if(mode==8)dup.value->pid=0;
        if(mode==9)dup.value->engines.clear();
        if(mode==10)dup.value->engines[0].name="bad name";
        if(mode==11)dup.support=Support::estimated;
        if(mode==12)dup.value->descriptor=std::numeric_limits<std::uint32_t>::max();
        if(mode==13)dup.value->driver=std::string(129,'x');
        r=s.sample({a,dup});assert(!r.value);if(mode==0)assert(r.support==Support::permission_required);
        r=s.sample({row(1,300000000,2000000000)});assert(!engine(r).summed_client_active_percent.value);
    }
    std::vector<Row> huge(2049,row());assert(!s.sample(huge).value);
    auto many=row();many.value->engines.clear();for(unsigned n=0;n<128;++n)many.value->engines.push_back({"e"+std::to_string(n),0,1,{}});huge.assign(513,many);assert(!s.sample(huge).value);
    r=s.sample({});assert(r.value && r.value->empty());r=s.sample({row()});assert(!engine(r).summed_client_active_percent.value);
    std::cout<<"DRM selected-client aggregation tests passed\n";
}
