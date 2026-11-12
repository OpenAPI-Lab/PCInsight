#include "../src/linux_drm_client.hpp"
#include <cassert>
#include <iostream>
#include <limits>
#ifdef __linux__
#include <unistd.h>
#endif
using namespace pcinsight;
using namespace pcinsight::detail;
using namespace std::chrono_literals;
std::string record(std::string counters="drm-engine-gfx: 100 ns\ndrm-engine-compute: 200 ns\ndrm-engine-capacity-compute: 2\n"){
    return "pos:\t0\nflags:\t0100002\ndrm-driver:\tamdgpu\ndrm-pdev:\t0000:03:00.0\ndrm-client-id:\t7\n"+counters;
}
LinuxDrmClientUsage snapshot(std::uint64_t ns=100,std::int64_t time=0){auto v=*parse_linux_drm_fdinfo(record());v.pid=42;v.descriptor=5;v.process_start_ticks=1000;v.captured_at=std::chrono::steady_clock::time_point{std::chrono::nanoseconds(time)};v.engines[1].busy_nanoseconds=ns;return v;}
Metric<LinuxDrmClientUsage> available(LinuxDrmClientUsage v){return {std::move(v),Support::available,"fixture"};}
std::string stat_record(std::uint64_t start=1000){return "42 (fixture process) S 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 1 0 "+std::to_string(start)+"\n";}
int main(){
    auto parsed=parse_linux_drm_fdinfo(record());assert(parsed && parsed->driver=="amdgpu" && parsed->client_id==7 && parsed->engines.size()==2);
    assert(parsed->engines[0].name=="compute" && parsed->engines[0].capacity==2 && parsed->engines[1].name=="gfx" && parsed->engines[1].capacity==1);
    assert(parse_linux_drm_fdinfo(record("drm-engine-copy:\t0\tns"))->engines[0].busy_nanoseconds==0);
    assert(parse_linux_drm_fdinfo(record("drm-engine-render: 18446744073709551615 ns\n")));
    for(const auto* key:{"drm-driver", "drm-pdev", "drm-client-id"}){auto r=record();const auto begin=r.find(key),end=r.find('\n',begin);r.erase(begin,end-begin+1);assert(!parse_linux_drm_fdinfo(r));}
    const char nul[]="drm-engine-gfx: 1\0 ns\n";assert(!parse_linux_drm_fdinfo(record(std::string(nul,sizeof(nul)-1))));
    assert(parse_linux_drm_fdinfo(record("drm-engine-sdma0: 12 ns\ndrm-engine-sdma1: 13 ns\ndrm-engine-compute0: 0 ns\ndrm-engine-compute1: 0 ns\ndrm-engine-compute2: 0 ns\n"))->engines.size()==5);
    for(const auto& s:{"", "drm-engine-gfx: 1\n", "drm-engine-gfx: 1 ms\n", "drm-engine-gfx: 1ns\n", "drm-engine-gfx: -1 ns\n", "drm-engine-gfx: +1 ns\n", "drm-engine-gfx: 18446744073709551616 ns\n", "drm-engine-: 1 ns\n", "drm-engine-bad name: 1 ns\n", "drm-engine-gfx: 1 ns\ndrm-engine-gfx: 1 ns\n", "drm-engine-gfx: 1 ns\ndrm-engine-capacity-gfx: 0\n", "drm-engine-gfx: 1 ns\ndrm-engine-capacity-gfx: +1\n", "drm-engine-gfx: 1 ns\ndrm-engine-capacity-gfx: 1 ns\n", "drm-engine-gfx: 1 ns\ndrm-engine-capacity-other: 1\n", "drm-engine-gfx: 1 ns\ndrm-client-id: 8\n", "drm-engine-gfx: 1 ns\ndrm-driver: i915\n", "drm-engine-gfx: 1 ns\ndrm-pdev: 0001:03:00.0\n"})assert(!parse_linux_drm_fdinfo(record(s)));
    auto bad=record();bad.replace(bad.find("0000:03:00.0"),12,"not PCI");assert(!parse_linux_drm_fdinfo(bad));
    bad=record();bad.replace(bad.find("amdgpu"),6,"bad\x01");assert(!parse_linux_drm_fdinfo(bad));
    assert(!parse_linux_drm_fdinfo(std::string(65537,'x')));
    assert(!parse_linux_drm_fdinfo(record()+std::string(4097,'x')));
    std::string many;for(unsigned n=0;n<129;++n)many+="drm-engine-e"+std::to_string(n)+": 1 ns\n";assert(!parse_linux_drm_fdinfo(record(many)));
    std::optional<LinuxDrmClientUsage> prev;
    auto first=update_linux_drm_client_usage(prev,available(snapshot()));assert(first.value && !first.value->engines[1].utilization_percent.value);
    auto v=snapshot(500000100,1000000000);v.engines[0].busy_nanoseconds=1000000200;
    auto next=update_linux_drm_client_usage(prev,available(v));assert(next.value->engines[1].utilization_percent.value==50 && next.value->engines[0].utilization_percent.value==50);
    assert(next.value->engines[1].utilization_percent.support==Support::estimated && next.value->sample_interval.value==1s);
    v.captured_at+=1s;next=update_linux_drm_client_usage(prev,available(v));assert(next.value->engines[1].utilization_percent.value==0);
    v.engines[1].busy_nanoseconds=1;v.captured_at+=1s;next=update_linux_drm_client_usage(prev,available(v));assert(!next.value->engines[1].utilization_percent.value && prev->engines[1].busy_nanoseconds==500000100 && next.value->engines[1].busy_nanoseconds==1);
    v.engines[1].busy_nanoseconds=600000100;v.captured_at+=1s;next=update_linux_drm_client_usage(prev,available(v));assert(next.value->engines[1].utilization_percent.value==10);
    v.engines[1].busy_nanoseconds+=2000000000;v.captured_at+=1s;next=update_linux_drm_client_usage(prev,available(v));assert(!next.value->engines[1].utilization_percent.value);
    v.captured_at+=1s;next=update_linux_drm_client_usage(prev,available(v));assert(next.value->engines[1].utilization_percent.value==0);
    for(int mode=0;mode<6;++mode){prev=snapshot();v=snapshot(200,1000000000);if(mode==0)++v.client_id;if(mode==1)++v.process_start_ticks;if(mode==2)v.pci_address="0001:03:00.0";if(mode==3)v.driver="i915";if(mode==4)++v.pid;if(mode==5)v.engines[1].capacity=2;next=update_linux_drm_client_usage(prev,available(v));assert(!next.value->engines[1].utilization_percent.value);}
    prev=snapshot();v=snapshot(100,0);next=update_linux_drm_client_usage(prev,available(v));assert(!next.value->sample_interval.value);v.captured_at-=1s;next=update_linux_drm_client_usage(prev,available(v));assert(!next.value->engines[1].utilization_percent.value);
    prev=snapshot();auto failure=update_linux_drm_client_usage(prev,{{},Support::permission_required,"denied"});assert(!prev && !failure.value && failure.support==Support::permission_required);
    prev=snapshot();v=snapshot(100000100,1000000000);v.descriptor=9;next=update_linux_drm_client_usage(prev,available(v));assert(next.value->engines[1].utilization_percent.value==10);
    prev=snapshot();v=snapshot(200,1000000000);v.engines.erase(v.engines.begin()+1);next=update_linux_drm_client_usage(prev,available(v));v=snapshot(300,2000000000);next=update_linux_drm_client_usage(prev,available(v));assert(!next.value->engines[1].utilization_percent.value);
    prev=snapshot(std::numeric_limits<std::uint64_t>::max()-100);v=snapshot(std::numeric_limits<std::uint64_t>::max(),1000000000);next=update_linux_drm_client_usage(prev,available(v));assert(next.value->engines[1].utilization_percent.value && *next.value->engines[1].utilization_percent.value>0);
    for(int mode=0;mode<14;++mode){
        unsigned stats=0,records=0,devices=0;
        DrmClientReadApi api{
            [&](const std::string& path)->Metric<std::string>{
                if(path=="/proc/42/stat"){++stats;if(mode==12)return {{},Support::permission_required,"stat denied"};if(mode==13)return {std::string("bad stat"),Support::available,"invalid stat"};return {stat_record(mode==4 && stats>1?2000:1000),Support::available,"fixture stat"};}
                assert(path=="/proc/42/fdinfo/5");++records;
                if(mode==1)return {{},Support::permission_required,"fixture denied"};
                auto r=record();if(mode==2 && records>1)r+="drm-client-id: 9\n";if(mode==5)r="not DRM";
                if(mode==7 && records>1)r.replace(r.find("amdgpu"),6,"i915");
                if(mode==11 && records>1)r.replace(r.find("drm-client-id:\t7"),16,"drm-client-id:\t8");
                return {r,Support::available,"fixture fdinfo"};
            },
            [&](std::uint32_t p,std::uint32_t f)->Metric<std::string>{assert(p==42 && f==5);++devices;if(mode==6)return {{},Support::permission_required,"device denied"};return {std::string(mode==3 || (mode==8 && devices>1)?"0001:03:00.0":"0000:03:00.0"),mode==9?Support::estimated:Support::available,"fixture identity"};},
            []{return std::chrono::steady_clock::time_point{1s};}};
        if(mode==10)api.now={};
        auto r=pcinsight::detail::read_linux_drm_client_usage(42,5,api);if(mode==0)assert(r.value && r.value->process_start_ticks==1000 && r.value->engines.size()==2);else assert(!r.value);
        if(mode==1 || mode==6 || mode==12)assert(r.support==Support::permission_required);
    }
    assert(!pcinsight::read_linux_drm_client_usage(0,0).value);
    assert(!pcinsight::read_linux_drm_client_usage(42,std::numeric_limits<std::uint32_t>::max()).value);
    LinuxDrmClientSampler sampler;assert(!sampler.sample(0,0).value);sampler.reset();
#ifdef __linux__
    int pipes[2];assert(pipe(pipes)==0);const auto non_drm=pcinsight::read_linux_drm_client_usage(getpid(),pipes[0]);assert(!non_drm.value);close(pipes[0]);close(pipes[1]);
#else
    assert(!pcinsight::read_linux_drm_client_usage(42,5).value);
#endif
    std::cout<<"DRM client reader and sampler tests passed\n";
}
