#include "../src/linux_network_selected_sampler.hpp"
#include <cassert>
#include <bit>
#include <stdexcept>
using namespace pcinsight;using namespace pcinsight::detail;
namespace {
struct Fake {
    std::array<bool,256> live{};std::array<unsigned,256> entries{};int next=1,held=0,calls=0,joins=0,clocks=0,drift_at=-1;
    bool deny_hold=false,deny_join=false,throw_join=false,throw_read=false,deny_capture=false;
    bool throw_hold=false,deny_layout=false,deny_attach=false;
    std::uint64_t process=7,time=1'000'000'000,span=10;
    Support support=Support::available,clock_support=Support::available;
    std::array<std::uint64_t,private_network_discovery_entries> values{};
    PrivatePidfdNetworkJoin identity{{999,2000,800},{{1,2},{7,42,100,1,2,3,4,5,6,7},{8,43,200,1,2,8,4,9,6,10}}};
    static Fake& f(void* p){return *static_cast<Fake*>(p);}
    int fd(unsigned n=0){assert(next<256);live[next]=true;entries[next]=n;return next++;}
    void clean(){assert(!held);for(bool n:live)assert(!n);}
    NetworkSessionApi capture(){return {this,
        [](void* p)->Metric<LinuxNetworkBtfLayout>{auto& s=f(p);++s.calls;if(s.deny_layout)return {{},Support::permission_required,"layout denied"};
            LinuxNetworkBtfLayout l;l.inet_send_function_id=10;l.inet6_send_function_id=11;l.receive_function_id=12;l.sock_protocol_width=1;
            l.socket_namespace={LinuxSocketNamespaceBtfLayout{64,128},Support::available,{}};return {l,Support::available,{}};},
        [](void* p,unsigned n)->NetworkSessionFd{auto& s=f(p);++s.calls;if(n==9||n==private_network_discovery_entries)s.values={};return {s.fd(n),0};},
        [](void* p,std::span<const NetworkProbeInstruction>,std::uint32_t,bool)->NetworkSessionFd{auto& s=f(p);++s.calls;return {s.fd(),0};},
        [](void*,int)->NetworkSessionRun{return {};},
        [](void* p,int fd,std::uint32_t key)->Metric<std::uint64_t>{auto& s=f(p);assert(s.live[fd]);++s.calls;
            if(s.entries[fd]==3){const std::array<std::uint64_t,3> creator{123,1000,900};return {creator.at(key),Support::available,{}};}
            if(s.throw_read){s.throw_read=false;throw std::runtime_error("read");}
            if(s.deny_capture)return {{},Support::permission_required,"read denied"};return {s.values.at(key),Support::available,{}};},
        [](void* p,int)->NetworkSessionFd{auto& s=f(p);++s.calls;if(s.deny_attach)return {-1,1};return {s.fd(),0};},
        [](void* p,int fd) noexcept {auto& s=f(p);assert(s.live[fd]);s.live[fd]=false;}};}
    NetworkSamplerApi clock(){return {this,nullptr,
        [](void* p)->Metric<std::uint64_t>{auto& s=f(p);return {s.time+((s.clocks++%2)?s.span:0),s.clock_support,{}};},
        [](void* p) noexcept ->std::uint64_t{return f(p).process;},nullptr};}
    SelectedNetworkSourceApi target(){return {this,
        [](void* p,int fd)->NetworkSessionFd{auto& s=f(p);++s.calls;assert(fd==55);if(s.throw_hold)throw std::runtime_error("hold");
            if(s.deny_hold)return {-1,13};assert(!s.held);s.held=1;return {0,0};},
        [](void* p,int fd) noexcept {auto& s=f(p);assert(fd==0&&s.held);s.held=0;},
        [](void* p,int fd,std::uint32_t pid,std::uint64_t start)->Metric<PrivatePidfdNetworkJoin>{auto& s=f(p);assert(fd==0&&s.held&&pid==8&&start==200);++s.joins;
            if(s.throw_join){s.throw_join=false;throw std::runtime_error("join");}
            if(s.deny_join)return {{},Support::permission_required,"join denied"};auto result=s.identity;
            if(s.joins==s.drift_at)++result.local.process.local_real_uid;
            return {result,s.support,{}};}};}
    void tick(std::uint64_t ns=1'000'000'000){time+=ns;clocks=0;}
};
PrivateSelectedNetworkWindow sample(LinuxSelectedNetworkSampler& s){auto r=s.sample();assert(r.value);return *r.value;}
void baseline(const PrivateSelectedNetworkWindow& w){assert(w.window.accounting.actors.size()==1);assert(!w.window.accounting.actors[0].interval_ns.value);
    for(const auto& lane:w.window.accounting.actors[0].bytes_per_second)assert(!lane.value);}
void discovered_bucket(Fake& f,unsigned slot,std::uint64_t cookie,std::uint64_t bytes){
    const auto base=9+slot*6;f.values[base]=cookie;for(unsigned i=1;i<=4;++i)f.values[base+i]=bytes;f.values[base+5]=1;++f.values[8];
}
PrivateSelectedDiscoveredNetworkWindow discovered(LinuxSelectedNetworkSampler& s){auto r=s.sample_discovered();assert(r.value&&r.support==Support::available);return *r.value;}
void no_rates(const PrivateSelectedDiscoveredNetworkWindow& w){for(const auto& a:w.accounting.actors){assert(!a.interval_ns.value);for(const auto& r:a.bytes_per_second)assert(!r.value);}}
void test_session_layout(){
    Fake f;auto api=f.target();int cached=0;
    // Session-owned BTF is reused, but identity/permission checks still run on EVERY join.
    api.join_layout=[](void* p,int fd,std::uint32_t pid,std::uint64_t start,const LinuxNetworkBtfLayout& l){
        assert(l.inet_send_function_id==10&&l.socket_namespace.value);
        auto& state=Fake::f(p);return state.target().join(p,fd,pid,start);
    };
    // Disable the legacy callback's work to prove that the layout-aware route is selected.
    api.join=[](void*,int,std::uint32_t,std::uint64_t)->Metric<PrivatePidfdNetworkJoin>{assert(false);return {};};
    LinuxSelectedNetworkSampler s(f.capture(),f.clock(),api);
    assert(s.start_discovering(true,55,8,200).value);cached=f.joins;assert(cached==1);
    no_rates(discovered(s));assert(f.joins==cached+2);cached=f.joins;
    f.deny_join=true;assert(!s.sample_discovered().value&&f.joins==cached+1);
    f.deny_join=false;f.tick();no_rates(discovered(s));
    f.throw_join=true;bool thrown=false;try{(void)s.sample_discovered();}catch(const std::runtime_error&){thrown=true;}assert(thrown);
    f.tick();no_rates(discovered(s));s.stop();f.clean();
    f.deny_layout=true;cached=f.joins;assert(!s.start_discovering(true,55,8,200).value&&f.joins==cached);f.clean();
    f.deny_layout=false;assert(s.start(true,55,8,200,12).value);cached=f.joins;
    baseline(sample(s));assert(f.joins==cached+2);s.stop();f.clean();
}
void test_discovered(){
    {Fake f;LinuxSelectedNetworkSampler s(f.capture(),f.clock(),f.target());assert(!s.start_discovering(false,55,8,200).value);
        assert(!s.start_discovering(true,-1,8,200).value);assert(!s.start_discovering(true,55,0,200).value);
        assert(!s.start_discovering(true,55,8,0).value);assert(!s.sample_discovered().value&&f.calls==0);f.clean();}
    for(unsigned fault=0;fault<8;++fault){Fake f;{
        if(fault==0)f.deny_hold=true;if(fault==1)f.deny_join=true;if(fault==2)f.throw_hold=true;if(fault==3)f.throw_join=true;
        if(fault==4)f.support=Support::estimated;if(fault==5)f.deny_layout=true;if(fault==6)f.deny_attach=true;if(fault==7)f.identity.local.observer.local_pid=9;
        LinuxSelectedNetworkSampler s(f.capture(),f.clock(),f.target());bool thrown=false;
        try{assert(!s.start_discovering(true,55,8,200).value);}catch(const std::runtime_error&){thrown=true;}assert(thrown==(fault==2||fault==3));f.clean();
    }f.clean();}
    {Fake f;LinuxSelectedNetworkSampler s(f.capture(),f.clock(),f.target());assert(s.start_discovering(true,55,8,200).value);
        assert(!s.start(true,55,8,200,12).value&&!s.sample().value);
        auto empty=discovered(s);assert(empty.buckets.empty()&&empty.accounting.actors.empty()&&empty.accounting.users.empty());
        const auto epoch=empty.accounting.scope.producer_epoch;assert(empty.accounting.scope.coverage==NetworkAccountingCoverage::selected_observed_namespace_set);
        f.tick();discovered_bucket(f,0,12,100);auto first=discovered(s);no_rates(first);assert(first.local.process.local_pid==8&&first.buckets.size()==1);
        assert(first.accounting.actors[0].actor.tgid==999&&first.accounting.actors[0].actor.real_uid==2000);
        f.tick(2'000'000'000);discovered_bucket(f,0,12,140);discovered_bucket(f,1,13,50);auto next=discovered(s);
        for(unsigned i=0;i<4;++i){assert(next.accounting.actors[0].bytes_per_second[i].value==20);
            assert(next.accounting.actors[0].bytes_per_second[i].support==Support::estimated&&!next.accounting.actors[1].bytes_per_second[i].value);
            assert(next.accounting.users[0].known[i]==1&&next.accounting.users[0].missing[i]==1&&next.accounting.users[0].bytes_per_second[i].value==20);}
        f.tick();discovered_bucket(f,0,12,150);discovered_bucket(f,1,13,80);next=discovered(s);
        for(unsigned i=0;i<4;++i)assert(next.accounting.users[0].known[i]==2&&next.accounting.users[0].bytes_per_second[i].value==40);
        ++f.identity.local.observer.user_namespace_inode;f.tick();no_rates(discovered(s));
        f.tick();assert(discovered(s).accounting.actors[0].bytes_per_second[0].value==0);
        s.discard_window();f.tick();no_rates(discovered(s));f.tick(LinuxNetworkAccounting::max_interval_ns+1);no_rates(discovered(s));
        s.stop();f.clean();assert(s.start_discovering(true,55,8,200).value);empty=discovered(s);
        assert(empty.accounting.actors.empty()&&empty.accounting.scope.producer_epoch!=epoch);
        f.process=9;const auto calls=f.calls,joins=f.joins;assert(!s.sample_discovered().value&&f.calls==calls&&f.joins==joins);f.clean();
        f.process=7;assert(s.start(true,55,8,200,12).value);assert(!s.sample_discovered().value);baseline(sample(s));s.stop();f.clean();}
    {Fake f;LinuxSelectedNetworkSampler s(f.capture(),f.clock(),f.target());assert(s.start_discovering(true,55,8,200).value);
        for(unsigned i=0;i<16;++i)discovered_bucket(f,i,100+i,0); // Excluded-only activity is real, not a fabricated zero bucket.
        auto full=discovered(s);assert(full.accounting.actors.size()==16&&full.accounting.users[0].observed_buckets==16);no_rates(full);
        f.tick();full=discovered(s);for(unsigned lane=0;lane<4;++lane)assert(full.accounting.users[0].known[lane]==16&&full.accounting.users[0].bytes_per_second[lane].value==0);
        // The reducer's new scope allows empty activity, but never multiple actors or17 cookies.
        NetworkAccountingInput input{full.accounting.scope,f.time,0,true,{}};
        LinuxNetworkAccounting reducer;assert(reducer.sample({input,Support::available,{}}).value);
        input.rows={{{999,2000,800,12},{},true,false},{{998,2000,800,13},{},true,false}};++input.observed_at_ns;
        assert(!reducer.sample({input,Support::available,{}}).value);
        input.rows.clear();for(unsigned i=0;i<17;++i)input.rows.push_back({{999,2000,800,100+i},{},true,false});
        assert(!reducer.sample({input,Support::available,{}}).value);s.stop();f.clean();}
    for(unsigned fault=0;fault<21;++fault){Fake f;{
        LinuxSelectedNetworkSampler s(f.capture(),f.clock(),f.target());assert(s.start_discovering(true,55,8,200).value);
        discovered_bucket(f,0,12,100);no_rates(discovered(s));f.tick();
        if(fault==0)f.deny_join=true;if(fault==1)f.support=Support::estimated;if(fault==2)f.drift_at=f.joins+2;
        if(fault==3)++f.identity.kernel.kernel_real_uid;if(fault==4)++f.identity.local.boot[0];
        if(fault==5)f.deny_capture=true;if(fault==6)f.values[4]=1;if(fault==7)f.values[6]=1;if(fault==8)f.values[7]=1;
        if(fault==9)f.clock_support=Support::estimated;if(fault==10)f.span=1'000'000'001;if(fault==11)f.time=0;
        if(fault==12)f.throw_join=true;if(fault==13)f.throw_read=true;if(fault==14)f.time=1;
        if(fault==15)f.values[8]=0;if(fault==16)f.values[9]=13;if(fault==17)f.values[10]=99;
        if(fault==18)f.values[14]=0;if(fault==19)f.values={};if(fault==20)f.values[1]=1;
        bool thrown=false;try{assert(!s.sample_discovered().value);}catch(const std::runtime_error&){thrown=true;}assert(thrown==(fault==12||fault==13));
        f.deny_join=f.deny_capture=false;f.support=f.clock_support=Support::available;f.drift_at=-1;f.identity.kernel.kernel_real_uid=2000;f.identity.local.boot={1,2};
        f.values={};discovered_bucket(f,0,12,100);f.time=5'000'000'000;f.span=10;f.clocks=0;no_rates(discovered(s));
        f.tick();assert(discovered(s).accounting.actors[0].bytes_per_second[0].value==0);
    }f.clean();}
}
}
int main(){
    {Fake f;LinuxSelectedNetworkSampler s(f.capture(),f.clock(),f.target());assert(!s.start(false,55,8,200,12).value);assert(!s.start(true,-1,8,200,12).value);
        assert(!s.start(true,55,0,200,12).value);assert(!s.start(true,55,8,0,12).value);assert(!s.start(true,55,8,200,0).value);assert(!s.sample().value);assert(f.calls==0);f.clean();}
    if(std::endian::native!=std::endian::little)return 0;
    test_session_layout();
    test_discovered();
    for(unsigned fault=0;fault<8;++fault){Fake f;{
        if(fault==0)f.deny_hold=true;if(fault==1)f.deny_join=true;if(fault==2)f.throw_hold=true;if(fault==3)f.throw_join=true;
        if(fault==4)f.support=Support::estimated;if(fault==5)f.deny_layout=true;if(fault==6)f.deny_attach=true;if(fault==7)f.identity.local.observer.local_pid=9;
        LinuxSelectedNetworkSampler s(f.capture(),f.clock(),f.target());bool thrown=false;
        try{assert(!s.start(true,55,8,200,12).value);}catch(const std::runtime_error&){thrown=true;}assert(thrown==(fault==2||fault==3));f.clean();
    }f.clean();}
    {Fake f;{
        LinuxSelectedNetworkSampler s(f.capture(),f.clock(),f.target());assert(s.start(true,55,8,200,12).value);assert(!s.start(true,55,8,200,12).value);
        auto first=sample(s);baseline(first);const auto epoch=first.window.accounting.scope.producer_epoch;
        assert(first.local.process.local_pid==8&&first.local.process.local_real_uid==43);
        const auto& scope=first.window.accounting.scope;assert(scope.coverage==NetworkAccountingCoverage::selected_single_socket_namespace&&scope.producer_tgid==123);
        assert(scope.pid_domain==NetworkIdentityDomain::kernel_initial&&scope.user_domain==NetworkIdentityDomain::kernel_initial&&!scope.pid_namespace&&!scope.user_namespace);
        f.tick(2'000'000'000);f.values={40,60,80,100,0,8,0,0,4};auto next=sample(s);const auto& a=next.window.accounting.actors[0];
        assert(a.actor.tgid==999&&a.actor.real_uid==2000&&a.actor.socket_network_namespace_cookie==12&&a.interval_ns.value==2'000'000'000);
        for(unsigned i=0;i<4;++i){assert(a.bytes_per_second[i].value==20+10*i&&a.bytes_per_second[i].support==Support::estimated);
            assert(next.window.accounting.users[0].bytes_per_second[i].value==20+10*i&&next.window.accounting.users[0].known[i]==1);}
        f.tick();assert(sample(s).window.accounting.actors[0].bytes_per_second[0].value==0);
        ++f.identity.local.observer.user_namespace_inode;f.tick();baseline(sample(s)); // context changes between calls: no crossing interval
        f.tick();assert(sample(s).window.accounting.actors[0].bytes_per_second[0].value==0);
        s.stop();s.stop();f.clean();assert(s.start(true,55,8,200,13).value);auto restarted=sample(s);baseline(restarted);assert(restarted.window.accounting.scope.producer_epoch!=epoch);
        f.process=9;const auto calls=f.calls,joins=f.joins;assert(!s.sample().value&&f.calls==calls&&f.joins==joins);f.clean();
    }f.clean();}
    for(unsigned fault=0;fault<15;++fault){Fake f;{
        LinuxSelectedNetworkSampler s(f.capture(),f.clock(),f.target());assert(s.start(true,55,8,200,12).value);baseline(sample(s));f.tick();
        if(fault==0)f.deny_join=true;if(fault==1)f.support=Support::estimated;if(fault==2)f.drift_at=f.joins+2;
        if(fault==3)++f.identity.kernel.kernel_real_uid;if(fault==4)++f.identity.local.boot[0];
        if(fault==5)f.deny_capture=true;if(fault==6)f.values[4]=1;if(fault==7)f.values[6]=1;if(fault==8)f.values[7]=1;
        if(fault==9)f.clock_support=Support::estimated;if(fault==10)f.span=1'000'000'001;if(fault==11)f.time=0;
        if(fault==12)f.throw_join=true;if(fault==13)f.throw_read=true;if(fault==14)f.time=1;
        bool thrown=false;try{assert(!s.sample().value);}catch(const std::runtime_error&){thrown=true;}assert(thrown==(fault==12||fault==13));
        f.deny_join=f.deny_capture=false;f.support=f.clock_support=Support::available;f.drift_at=-1;f.identity.kernel.kernel_real_uid=2000;f.identity.local.boot={1,2};
        f.values={};f.time=5'000'000'000;f.span=10;f.clocks=0;baseline(sample(s));f.tick();assert(sample(s).window.accounting.actors[0].bytes_per_second[0].value==0);
    }f.clean();}
    {Fake f;LinuxSelectedNetworkSampler s(f.capture(),f.clock(),f.target());assert(s.start(true,55,8,200,12).value);baseline(sample(s));
        f.tick(LinuxNetworkAccounting::max_interval_ns+1);baseline(sample(s));f.tick();assert(sample(s).window.accounting.actors[0].bytes_per_second[0].value==0);}
    // Both sampler types draw epochs from the same non-wrapping sequence.
    const auto epoch=next_private_network_epoch();assert(next_private_network_epoch()>epoch);
}
