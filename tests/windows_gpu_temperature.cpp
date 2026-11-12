#include "../src/windows_gpu_temperature.hpp"
#include "pcinsight/monitor.hpp"
#include <cassert>
#include <iostream>
using namespace pcinsight;
using namespace pcinsight::detail;
namespace {
int mode;unsigned opens,closes,counts,perf_calls,raw_temperature;
LONG APIENTRY open(GpuOpenLuid* out){++opens;out->handle=out->luid.LowPart;if(mode==1)return static_cast<LONG>(0xc0000022);if(mode==2)out->handle=0;if(mode==3)++out->luid.LowPart;return 0;}
LONG APIENTRY close(const GpuCloseDevice* data){assert(data->handle);++closes;return 0;}
LONG APIENTRY query(GpuAdapterQuery* q){
    assert(q->handle==1 || q->handle==2);
    if(q->type==30){assert(q->size==4);++counts;if(mode==4)return static_cast<LONG>(0xc0000022);
        *static_cast<UINT*>(q->data)=mode==5?2:mode==6?0:mode==13 && counts%2==0?2:1;
        if(mode==7)--q->size;return 0;
    }
    assert(q->type==62 && q->size==64);++perf_calls;auto& perf=*static_cast<GpuAdapterPerfData*>(q->data);assert(perf.physical_index==0);
    if(mode==8)return static_cast<LONG>(0xc0000022);
    if(mode==9)return 0; // success without writing output sentinel
    perf.temperature=q->handle==1?raw_temperature:512;
    if(mode==10)perf.temperature=2551;if(mode==11)perf.physical_index=1;if(mode==12)q->data=nullptr;
    return 0;
}
GpuTemperatureApi api{open,query,close};
std::vector<GpuInfo> targets(){std::vector<GpuInfo> g(2);g[0].id="dxgi-luid:00000000:00000001";g[1].id="dxgi-luid:ffffffff:00000002";return g;}
std::vector<GpuInfo> run(int scenario,unsigned temperature=481){mode=scenario;opens=closes=counts=perf_calls=0;raw_temperature=temperature;auto g=targets();fill_windows_gpu_temperature(g,api);assert(closes==(mode==2?0:opens));return g;}
}
int main(){
    auto g=run(0);assert(g[0].temperature_celsius.value==48.1 && g[1].temperature_celsius.value==51.2);assert(counts==4 && perf_calls==2);
    g=run(0,0);assert(g[0].temperature_celsius.value==0 && g[0].temperature_celsius.support==Support::available);
    g=run(0,2550);assert(g[0].temperature_celsius.value==255);
    for(int m=1;m<=13;++m){g=run(m);assert(!g[0].temperature_celsius.value);if(m==1 || m==4 || m==8)assert(g[0].temperature_celsius.support==Support::permission_required);if(m==5 || m==6)assert(perf_calls==0);}
    g=run(0);raw_temperature=607;fill_windows_gpu_temperature(g,api);assert(g[0].temperature_celsius.value==60.7);
    mode=8;fill_windows_gpu_temperature(g,api);assert(!g[0].temperature_celsius.value); // no stale data
    run(0);g=targets();g.push_back(g[0]);fill_windows_gpu_temperature(g,api);assert(!g[0].temperature_celsius.value && !g[2].temperature_celsius.value && g[1].temperature_celsius.value);
    g=targets();g[0].id="dxgi-luid:00000000:00000001-extra";fill_windows_gpu_temperature(g,api);assert(!g[0].temperature_celsius.value);
    g=targets();auto missing=api;missing.close=nullptr;fill_windows_gpu_temperature(g,missing);assert(!g[0].temperature_celsius.value);
    g.assign(257,GpuInfo{});fill_windows_gpu_temperature(g,api);for(const auto& gpu:g)assert(!gpu.temperature_celsius.value);
    auto native=read_windows_gpus();fill_windows_gpu_temperature(native);
    for(const auto& gpu:native)std::cout<<gpu.model<<" | temperature="<<(gpu.temperature_celsius.value?std::to_string(*gpu.temperature_celsius.value):"unavailable")<<" | "<<gpu.temperature_celsius.note<<'\n';
    Monitor monitor;SampleOptions options;options.include_gpus=false;assert(monitor.sample(options).gpus.empty());options.include_gpus=true;
    for(const auto& gpu:monitor.sample(options).gpus)if(gpu.temperature_celsius.value)assert(*gpu.temperature_celsius.value>=0 && *gpu.temperature_celsius.value<=255);
    options.include_gpus=false;assert(monitor.sample(options).gpus.empty());
    std::cout<<"GPU temperature tests passed\n";
}
