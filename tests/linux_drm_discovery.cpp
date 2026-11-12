#include "../src/linux_drm_discovery.hpp"
#include <cassert>
#include <iostream>
using namespace pcinsight;
using namespace pcinsight::detail;
using namespace std::chrono_literals;
using Clock=std::chrono::steady_clock;
int main(){
    unsigned phase=0,read_calls=0;int mode=0;unsigned ticks=0;
    DrmDiscoveryApi api{
        [&](const std::string& path,std::uint32_t limit)->Metric<DrmDirectoryEntries>{
            if(mode==1 && path=="/proc")return {{},Support::permission_required,"denied"};
            DrmDirectoryEntries d;
            auto set=[&](std::initializer_list<std::uint32_t> ids){d.ids.clear();for(auto id:ids)d.ids.push_back(id);};
            if(path=="/proc")set({42,43});
            else if(path=="/proc/42/fdinfo")set({5,6,9});
            else if(path=="/proc/43/fdinfo"){
                if(mode==2)return {{},Support::permission_required,"denied"};
                set({7});
            }else assert(false);
            if(mode==3 && path=="/proc")set({42,42});
            if(mode==4 && path=="/proc")set({0});
            if(mode==5 && path=="/proc/42/fdinfo")set({5,5});
            if(mode==6 && path=="/proc/42/fdinfo")set({0xffffffff});
            if(mode==7 && path=="/proc/42/fdinfo")set({5,6,9,10});
            if(d.ids.size()>limit){if(mode!=7){d.ids.resize(limit);d.limit_reached=true;}}
            return {d,Support::available,"fixture directory"};
        },
        [&](std::uint32_t,std::uint32_t fd)->Metric<bool>{
            if(mode==8 && fd==6)return {{},Support::permission_required,"denied"};
            return {fd!=6,Support::available,"fixture character-device classification"};
        },
        [&](std::uint32_t pid,std::uint32_t fd)->Metric<LinuxDrmClientUsage>{
            ++read_calls;if(mode==9 && fd==7)return {{},Support::permission_required,"client denied"};
            if(mode==10 && fd==7)return {{},Support::unsupported,"driver lacks ns stats"};
            LinuxDrmClientUsage v;v.pid=pid;v.descriptor=fd;v.process_start_ticks=100;v.client_id=pid==42?1:2;
            v.driver="amdgpu";v.pci_address="0000:03:00.0";v.captured_at=Clock::time_point{std::chrono::seconds(phase)};
            v.engines.push_back({"gfx",phase*100000000ULL,1,{}});
            if(mode==11)v.pid=999;
            if(mode==12)v.engines.clear();
            if(mode==13)v.descriptor=99;
            return {v,Support::available,"verified reader fixture"};
        },
        [&]{if(mode==14)return Clock::time_point{std::chrono::milliseconds(100*ticks++)};if(mode==15)return Clock::time_point{-std::chrono::milliseconds(ticks++)};return Clock::time_point{std::chrono::seconds(phase)};}
    };
    LinuxDrmDiscoveryOptions o;auto scan=pcinsight::detail::discover_linux_drm_clients(o,api);
    assert(scan.value && scan.support==Support::estimated && scan.value->processes_seen==2 && scan.value->descriptors_seen==4 && scan.value->drm_candidates==3 && scan.value->observations.size()==3 && read_calls==3);
    assert(scan.value->permission_denials==0 && !scan.value->limit_reached && !scan.value->time_limit_reached && scan.value->scope.find("no all-process")!=std::string::npos);
    // End-to-end: discover automatically, then deduplicate and sample selected rates.
    LinuxDrmGpuSampler tracker;auto first=tracker.sample(scan.value->observations);assert(first.value->at(0).unique_clients==2 && first.value->at(0).input_records==3);
    ++phase;scan=pcinsight::detail::discover_linux_drm_clients(o,api);auto next=tracker.sample(scan.value->observations);assert(next.value->at(0).engines[0].summed_client_active_percent.value==20);
    for(mode=1;mode<=15;++mode){ticks=0;auto opts=o;if(mode==7)opts.max_descriptors=3;scan=pcinsight::detail::discover_linux_drm_clients(opts,api);
        if(mode==1 || (mode>=3 && mode<=7) || mode==15){assert(!scan.value);if(mode==1)assert(scan.support==Support::permission_required);continue;}
        assert(scan.value && scan.support==Support::estimated);
        if(mode==2)assert(scan.value->directory_failures==1 && scan.value->permission_denials==1);
        if(mode==8)assert(scan.value->unclassified_descriptors==1 && scan.value->permission_denials==1);
        if(mode==9)assert(scan.value->unreadable_drm_clients==1 && scan.value->permission_denials==1);
        if(mode==10)assert(scan.value->unreadable_drm_clients==1 && scan.value->observations.size()==2);
        if(mode>=11 && mode<=13)assert(scan.value->observations.empty() && scan.value->unreadable_drm_clients==3);
        if(mode==14)assert(scan.value->time_limit_reached);
    }
    mode=0;ticks=0;
    o.max_processes=1;scan=pcinsight::detail::discover_linux_drm_clients(o,api);assert(scan.value->limit_reached && scan.value->processes_seen==1 && scan.value->observations.size()==2);
    o={};o.max_descriptors=1;scan=pcinsight::detail::discover_linux_drm_clients(o,api);assert(scan.value->limit_reached && scan.value->descriptors_seen==1);
    o={};o.max_observations=1;scan=pcinsight::detail::discover_linux_drm_clients(o,api);assert(scan.value->limit_reached && scan.value->observations.size()==1);
    for(int k=0;k<8;++k){o={};if(k==0)o.max_processes=0;if(k==1)o.max_processes=65537;if(k==2)o.max_descriptors=0;if(k==3)o.max_descriptors=262145;if(k==4)o.max_observations=0;if(k==5)o.max_observations=2049;if(k==6)o.max_duration=0ms;if(k==7)o.max_duration=60001ms;assert(!pcinsight::detail::discover_linux_drm_clients(o,api).value);}
    o={};auto missing=api;missing.now={};assert(!pcinsight::detail::discover_linux_drm_clients(o,missing).value);
    // The complete batch must remain consumable by the aggregate's engine budget.
    auto large=api;
    large.list=[](const std::string& path,std::uint32_t)->Metric<DrmDirectoryEntries>{DrmDirectoryEntries d;if(path=="/proc")d.ids.push_back(42);else for(unsigned f=0;f<513;++f)d.ids.push_back(f);return {d,Support::available,"large fixture"};};
    large.is_drm=[](std::uint32_t,std::uint32_t)->Metric<bool>{return {true,Support::available,"fixture DRM"};};
    large.read_client=[](std::uint32_t p,std::uint32_t f)->Metric<LinuxDrmClientUsage>{LinuxDrmClientUsage v;v.pid=p;v.descriptor=f;v.client_id=f;v.process_start_ticks=100;v.driver="fixture";v.pci_address="0000:03:00.0";for(unsigned n=0;n<128;++n)v.engines.push_back({"e"+std::to_string(n),0,1,{}});return {v,Support::available,"fixture engines"};};
    o={};o.max_observations=2048;scan=pcinsight::detail::discover_linux_drm_clients(o,large);assert(scan.value && scan.value->limit_reached && scan.value->observations.size()==512);
    // Bounded real scan: no GPU device is opened to create a positive hardware case.
    o.max_processes=8;o.max_descriptors=64;o.max_observations=8;o.max_duration=50ms;
    const auto native=pcinsight::discover_linux_drm_clients(o);
#ifdef __linux__
    assert(native.value || native.support==Support::permission_required || native.support==Support::unsupported);
    if(native.value){assert(native.value->processes_seen<=8 && native.value->descriptors_seen<=64 && native.value->observations.size()<=8);std::cout<<"Native scan: processes="<<native.value->processes_seen<<" descriptors="<<native.value->descriptors_seen<<" clients="<<native.value->observations.size()<<" denied="<<native.value->permission_denials<<" limit="<<native.value->limit_reached<<" time="<<native.value->time_limit_reached<<'\n';}
#else
    assert(!native.value && native.support==Support::unsupported);
#endif
    std::cout<<"DRM discovery tests passed\n";
}
