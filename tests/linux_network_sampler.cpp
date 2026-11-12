#include "../src/linux_network_sampler.hpp"
#include <bit>
#include <cassert>
#include <stdexcept>
using namespace pcinsight;using namespace pcinsight::detail;
namespace {
struct Fake {
    std::array<bool,128> live{};std::array<unsigned,128> entries{};
    std::array<std::uint64_t,9> values{};
    int next=0,closed=0,capture_calls=0,boot_calls=0,clock_calls=0;
    std::uint64_t process=7,time=1'000'000'000,span=10;
    Metric<std::array<std::uint64_t,2>> boot{{{1,2}},Support::available,{}};
    Support clock_support=Support::available;bool deny=false,throw_lookup=false,throw_clock=false;
    std::array<std::uint64_t,3> id{123,1000,900};
    Metric<LinuxNetworkSelfIdentity> self_identity{LinuxNetworkSelfIdentity{7,42,100,1,1,7,2,3,2,4},Support::available,{}};
    unsigned self_calls=0;int drift=-1;bool deny_identity=false;
    static Fake& self(void* p){return *static_cast<Fake*>(p);}
    int fd(unsigned n=0){assert(next<128);live[next]=true;entries[next]=n;return next++;}
    void clean() const {assert(next==closed);for(auto value:live)assert(!value);}
    NetworkSessionApi capture(){return {this,
        [](void* p)->Metric<LinuxNetworkBtfLayout>{++self(p).capture_calls;LinuxNetworkBtfLayout l;
            l.inet_send_function_id=10;l.receive_function_id=11;l.inet6_send_function_id=12;l.sock_protocol_width=1;
            l.socket_namespace={LinuxSocketNamespaceBtfLayout{64,128},Support::available,{}};return {l,Support::available,{}};},
        [](void* p,unsigned n)->NetworkSessionFd{auto& f=self(p);++f.capture_calls;if(n==9)f.values={};return {f.fd(n),0};},
        [](void* p,std::span<const NetworkProbeInstruction>,std::uint32_t,bool)->NetworkSessionFd{auto& f=self(p);++f.capture_calls;return {f.fd(),0};},
        [](void* p,int)->NetworkSessionRun{++self(p).capture_calls;return {};},
        [](void* p,int fd,std::uint32_t key)->Metric<std::uint64_t>{auto& f=self(p);++f.capture_calls;assert(f.live[fd]);
            if(f.entries[fd]==3){if(f.deny_identity)return {{},Support::permission_required,"identity denied"};return {f.id.at(key),Support::available,{}};}
            if(f.throw_lookup){f.throw_lookup=false;throw std::runtime_error("lookup throw");}
            if(f.deny)return {{},Support::permission_required,"lookup denied"};
            return {f.values.at(key),Support::available,{}};},
        [](void* p,int)->NetworkSessionFd{auto& f=self(p);++f.capture_calls;return {f.fd(),0};},
        [](void* p,int fd) noexcept {auto& f=self(p);assert(f.live[fd]);f.live[fd]=false;++f.closed;}};}
    NetworkSamplerApi source(){return {this,
        [](void* p)->Metric<std::array<std::uint64_t,2>>{auto& f=self(p);++f.boot_calls;return f.boot;},
        [](void* p)->Metric<std::uint64_t>{auto& f=self(p);if(f.throw_clock){f.throw_clock=false;throw std::runtime_error("clock throw");}
            const bool after=(f.clock_calls++%2)!=0;return {f.time+(after?f.span:0),f.clock_support,"clock"};},
        [](void* p) noexcept ->std::uint64_t{return self(p).process;},
        [](void* p)->Metric<LinuxNetworkSelfIdentity>{auto& f=self(p);auto value=f.self_identity;
            if(++f.self_calls%2==0&&f.drift>=0&&value.value){auto& s=*value.value;
                if(f.drift==0)++s.local_pid;if(f.drift==1)++s.local_real_uid;if(f.drift==2)++s.start_ticks;
                if(f.drift==3)++s.procfs_device;if(f.drift==4)++s.procfs_inode;if(f.drift==5)++s.process_inode;
                if(f.drift==6)++s.pid_namespace_device;if(f.drift==7)++s.pid_namespace_inode;
                if(f.drift==8)++s.user_namespace_device;if(f.drift==9)++s.user_namespace_inode;
            }return value;}};}
    void tick(std::uint64_t ns=1'000'000'000){time+=ns;clock_calls=0;}
};
PrivateNetworkWindow sample(LinuxPrivateNetworkSampler& s){auto result=s.sample();assert(result.value);return *result.value;}
void no_rate(const PrivateNetworkWindow& w){assert(w.accounting.actors.size()==1);for(const auto& lane:w.accounting.actors[0].bytes_per_second)assert(!lane.value);}
}
int main(){
    assert(!parse_network_boot_token("").value);
    for(auto text:{"00000000-0000-0000-0000-000000000000","AAAAAAAA-aaaa-aaaa-aaaa-aaaaaaaaaaaa","1000000000000-0000-0000-000000000000","10000000-0000-0000-0000-000000000000\n\n"})assert(!parse_network_boot_token(text).value);
    auto parsed=parse_network_boot_token("01234567-89ab-cdef-fedc-ba9876543210\n");
    assert(parsed.value&&(*parsed.value)[0]==0x0123456789abcdefULL&&(*parsed.value)[1]==0xfedcba9876543210ULL);
    {Fake f;LinuxPrivateNetworkSampler s(f.capture(),f.source());assert(!s.start(false,12).value);assert(!s.sample().value);assert(!s.start(true,0).value);assert(f.capture_calls==0&&f.boot_calls==0);f.clean();}
    for(auto support:{Support::estimated,Support::permission_required,Support::unsupported}){
        Fake f;f.boot.support=support;LinuxPrivateNetworkSampler s(f.capture(),f.source());assert(!s.start(true,12).value);assert(f.capture_calls==0);f.clean();}
    {Fake f;f.boot.value=std::array<std::uint64_t,2>{};LinuxPrivateNetworkSampler s(f.capture(),f.source());assert(!s.start(true,12).value);f.clean();}
    if(std::endian::native!=std::endian::little)return 0;
    {Fake f;{
        LinuxPrivateNetworkSampler s(f.capture(),f.source());assert(s.start(true,12).value);assert(!s.start(true,12).value);
        f.values={100,200,300,400,0,2,0,0,3};auto first=sample(s);no_rate(first);
        const auto scope=first.accounting.scope;assert(scope.pid_domain==NetworkIdentityDomain::kernel_initial&&scope.user_domain==NetworkIdentityDomain::kernel_initial);
        assert(scope.coverage==NetworkAccountingCoverage::self_single_socket_namespace&&!scope.pid_namespace&&!scope.user_namespace);
        assert(scope.producer_tgid==123&&scope.producer_start_boottime_ns==900&&scope.producer_epoch);
        assert(first.quality.excluded_positive_receives==2&&first.read_started_ns==f.time&&first.read_finished_ns==f.time+10);
        f.tick(2'000'000'000);f.values={140,260,380,500,0,2,0,0,4};auto next=sample(s);
        const auto& actor=next.accounting.actors[0];assert(actor.actor.socket_network_namespace_cookie==12&&actor.actor.real_uid==1000);
        for(unsigned lane=0;lane<4;++lane){assert(actor.bytes_per_second[lane].value==20+10*lane);assert(actor.bytes_per_second[lane].support==Support::estimated);
            assert(next.accounting.users[0].bytes_per_second[lane].value==20+10*lane&&next.accounting.users[0].known[lane]==1);}
        assert(actor.interval_ns.value==2'000'000'000);f.tick();next=sample(s);assert(next.accounting.actors[0].bytes_per_second[0].value==0);
        s.stop();s.stop();assert(!s.sample().value);f.clean();assert(s.start(true,12).value);no_rate(sample(s));
        assert(!s.sample().value); // same timestamp rejected, baseline discarded
        f.tick();auto restarted=sample(s);no_rate(restarted);assert(restarted.accounting.scope.producer_epoch!=scope.producer_epoch);
    }f.clean();}
    // Every broken sample drops the baseline; recovery is not a rate across a gap.
    for(unsigned fault=0;fault<11;++fault){Fake f;{
        LinuxPrivateNetworkSampler s(f.capture(),f.source());assert(s.start(true,12).value);no_rate(sample(s));f.tick();
        if(fault==0)f.deny=true;if(fault==1)f.values[4]=1;if(fault==2)f.values[6]=1;
        if(fault==3)f.values[7]=1;if(fault==4)f.clock_support=Support::estimated;
        if(fault==5)f.span=LinuxPrivateNetworkSampler::max_read_span_ns+1;
        if(fault==6)f.time=1;if(fault==7)f.throw_lookup=true;if(fault==8)f.throw_clock=true;
        if(fault==9)f.time=0;if(fault==10)f.span=UINT64_MAX; // end wraps below start in injected clock
        bool caught=false;try{assert(!s.sample().value);}catch(const std::runtime_error&){caught=true;}assert(caught==(fault==7||fault==8));
        f.deny=false;f.values={};f.clock_support=Support::available;f.span=10;f.time=4'000'000'000;f.clock_calls=0;
        no_rate(sample(s));f.tick();auto measured=sample(s);assert(measured.accounting.actors[0].bytes_per_second[0].value==0);
    }f.clean();}
    {Fake f;{LinuxPrivateNetworkSampler s(f.capture(),f.source());assert(s.start(true,12).value);
        f.span=LinuxPrivateNetworkSampler::max_read_span_ns;no_rate(sample(s));f.tick();
        auto boundary=sample(s);assert(boundary.accounting.actors[0].interval_ns.value==1'000'000'000);
        f.tick(LinuxNetworkAccounting::max_interval_ns+1);no_rate(sample(s));f.tick();
        auto recovered=sample(s);assert(recovered.accounting.actors[0].bytes_per_second[0].value==0);
    }f.clean();}
    {Fake f;{LinuxPrivateNetworkSampler s(f.capture(),f.source());assert(s.start(true,12).value);auto first=sample(s);const auto prior=first.accounting.scope.producer_epoch;
        s.stop();assert(s.start(true,13).value);no_rate(sample(s));f.tick();auto second=sample(s);
        assert(second.accounting.scope.producer_epoch!=prior&&second.accounting.actors[0].actor.socket_network_namespace_cookie==13);
        ++f.process;const auto before=f.capture_calls;assert(!s.sample().value);assert(f.capture_calls==before);f.clean();assert(!s.sample().value);
    }f.clean();}
    // Local PID7/UID42 are intentionally NOT kernel TGID123/UID1000.
    {Fake f;{LinuxPrivateNetworkSampler s(f.capture(),f.source());assert(s.start(true,12).value);
        auto joined=s.sample_joined_self();assert(joined.value&&joined.value->local.local_pid==7&&joined.value->local.local_real_uid==42);
        assert(joined.value->window.accounting.actors[0].actor.tgid==123&&joined.value->window.accounting.actors[0].actor.real_uid==1000);
        f.tick();f.values[0]=100;joined=s.sample_joined_self();assert(joined.value&&joined.value->window.accounting.actors[0].bytes_per_second[0].value==100);
        ++f.self_identity.value->local_real_uid;f.tick();joined=s.sample_joined_self();assert(joined.value);no_rate(joined.value->window);
        f.tick();joined=s.sample_joined_self();assert(joined.value&&joined.value->window.accounting.actors[0].bytes_per_second[0].value==0);
        f.tick();sample(s);f.tick();joined=s.sample_joined_self();assert(joined.value);no_rate(joined.value->window); // raw/attributed not bridged
    }f.clean();}
    for(int fault=0;fault<17;++fault){Fake f;{LinuxPrivateNetworkSampler s(f.capture(),f.source());assert(s.start(true,12).value);
        assert(s.sample_joined_self().value);f.tick();
        if(fault<10)f.drift=fault;if(fault==10)f.self_identity.support=Support::estimated;
        if(fault==11)f.self_identity.support=Support::permission_required;if(fault==12)f.deny_identity=true;
        if(fault==13)++f.id[0];if(fault==14)++f.id[1];if(fault==15)++f.id[2];if(fault==16)++(*f.boot.value)[0];
        assert(!s.sample_joined_self().value);f.drift=-1;f.self_identity.support=Support::available;f.deny_identity=false;f.id={123,1000,900};f.boot.value=std::array<std::uint64_t,2>{1,2};
        f.tick();auto recovered=s.sample_joined_self();assert(recovered.value);no_rate(recovered.value->window);
    }f.clean();}
}
