#include "../src/linux_network_session.hpp"
#include <bit>
#include <cassert>
#include <cerrno>
#include <stdexcept>
using namespace pcinsight;using namespace pcinsight::detail;
namespace {
struct Fake {
    int calls=0,fail=0,throws=0,next=0,closes=0,attaches=0,lookups=0;
    std::array<bool,64> live{};
    std::array<unsigned,64> entries{};
    std::array<int,64> order{};
    std::array<std::uint64_t,3> identity{123,1000,9000000};
    std::array<std::uint64_t,private_network_discovery_entries> values{};
    Support layout_support=Support::available,lookup_support=Support::available;
    bool missing_hook=false,moving=false,lookup_denied=false,bad_return=false;
    bool missing_namespace=false;Support namespace_support=Support::available;
    bool step(){++calls;if(calls==throws)throw std::runtime_error("injected");return calls==fail;}
    int fd(unsigned size=0){assert(next<64);live[next]=true;entries[next]=size;return next++;}
    void clean() const {for(bool value:live)assert(!value);assert(closes==next);}
    static Fake& self(void* p){return *static_cast<Fake*>(p);}
    NetworkSessionApi api(){return {this,
        [](void* p)->Metric<LinuxNetworkBtfLayout>{auto& f=self(p);if(f.step())return {{},Support::permission_required,"layout denied"};
            LinuxNetworkBtfLayout l{};l.inet_send_function_id=10;l.inet6_send_function_id=f.missing_hook?0:12;l.receive_function_id=11;
            l.socket_sk_offset=24;l.sock_family_offset=16;l.sock_protocol_offset=516;l.sock_protocol_width=1;
            l.task_group_leader_offset=2480;l.task_start_boottime_offset=2728;
            if(!f.missing_namespace)l.socket_namespace={LinuxSocketNamespaceBtfLayout{64,128},f.namespace_support,{}};
            return {l,f.layout_support,{}};},
        [](void* p,unsigned n)->NetworkSessionFd{auto& f=self(p);if(f.step())return {-1,EPERM};assert(n==3||n==9||n==private_network_discovery_entries);return {f.fd(n),0};},
        [](void* p,std::span<const NetworkProbeInstruction> code,std::uint32_t hook,bool identity)->NetworkSessionFd{
            auto& f=self(p);if(f.step())return {-1,EPERM};assert(!code.empty());assert(identity?hook==0:hook>=10&&hook<=12);return {f.fd(),0};},
        [](void* p,int fd)->NetworkSessionRun{auto& f=self(p);assert(f.live[fd]);if(f.step())return {EPERM,0};return {0,f.bad_return?1u:0u};},
        [](void* p,int fd,std::uint32_t key)->Metric<std::uint64_t>{auto& f=self(p);assert(f.live[fd]);++f.lookups;
            if(f.step()||f.lookup_denied)return {{},Support::permission_required,"lookup denied"};
            if(f.entries[fd]==3){assert(key<3);return {f.identity[key],f.lookup_support,{}};}
            assert((f.entries[fd]==9||f.entries[fd]==private_network_discovery_entries)&&key<f.entries[fd]);if(f.moving&&key==8)++f.values[8];return {f.values[key],f.lookup_support,{}};},
        [](void* p,int fd)->NetworkSessionFd{auto& f=self(p);assert(f.live[fd]);++f.attaches;if(f.step())return {-1,EPERM};return {f.fd(),0};},
        [](void* p,int fd) noexcept {auto& f=self(p);assert(fd>=0&&f.live[fd]);f.live[fd]=false;f.order[f.closes++]=fd;}};}
};
}
int main(){
    {Fake f;LinuxPrivateNetworkSession s(f.api());assert(!s.start(false).value);assert(f.calls==0);assert(!s.read().value);s.stop();f.clean();}
    {LinuxPrivateNetworkSession s(NetworkSessionApi{});assert(!s.start(true).value);}
    {Fake f;f.missing_hook=true;LinuxPrivateNetworkSession s(f.api());assert(!s.start(true).value);assert(f.next==0);}
    {Fake f;f.layout_support=Support::estimated;LinuxPrivateNetworkSession s(f.api());assert(!s.start(true).value);assert(f.next==0);}
    {Fake f;f.missing_namespace=true;LinuxPrivateNetworkSession s(f.api());assert(!s.start(true,123).value);assert(f.next==0);}
    {Fake f;f.namespace_support=Support::estimated;LinuxPrivateNetworkSession s(f.api());assert(!s.start(true,123).value);assert(f.next==0);}
    const PrivateNetworkIdentity selected{456,0,1234567};
    for(unsigned bad=0;bad<5;++bad){Fake f;LinuxPrivateNetworkSession s(f.api());auto target=selected;auto cookie=std::uint64_t{9};
        if(bad==0)target.kernel_tgid=0;if(bad==1)target.kernel_tgid=2147483648U;if(bad==2)target.leader_start_boottime_ns=0;if(bad==3)cookie=0;
        assert(!s.start_selected(bad!=4,target,cookie).value);assert(f.calls==0);f.clean();}
    if(std::endian::native!=std::endian::little)return 0;
    {Fake f;LinuxPrivateNetworkSession s(f.api());assert(!s.start_discovering_selected(false,selected).value&&f.calls==0);
        assert(!s.read_discovered().value);assert(s.start_discovering_selected(true,selected).value);
        assert(!s.read().value&&!s.read_caller_identity().value);auto report=s.read_discovered();assert(report.value&&report.value->buckets.empty());
        f.values[9]=123;f.values[10]=4096;f.values[14]=8;f.values[8]=1;report=s.read_discovered();
        assert(report.value&&report.value->buckets.size()==1&&report.value->buckets[0].bytes[0]==4096&&report.value->completed_updates==1);
        f.lookup_denied=true;assert(s.read_discovered().support==Support::permission_required);f.lookup_denied=false;
        f.lookup_support=Support::estimated;assert(!s.read_discovered().value);f.lookup_support=Support::available;
        f.values[4]=1;assert(!s.read_discovered().value);f.values[4]=0;
        f.moving=true;assert(!s.read_discovered().value);f.moving=false;
        f.values[8]=1;s.detach();report=s.read_discovered();assert(report.value&&report.value->state==PrivateNetworkSessionState::detached);
        s.stop();assert(!s.read_discovered().value);f.clean();f.values={};assert(s.start_selected(true,selected,9).value);assert(s.read().value&&!s.read_discovered().value);s.stop();f.clean();}
    for(int operation=1;operation<=8;++operation)for(bool throwing:{false,true}){Fake f;if(throwing)f.throws=operation;else f.fail=operation;
        LinuxPrivateNetworkSession s(f.api());bool caught=false;try{assert(!s.start_discovering_selected(true,selected).value);}catch(const std::runtime_error&){caught=true;}
        assert(caught==throwing);f.clean();}
    {Fake f;LinuxPrivateNetworkSession s(f.api());assert(s.start_selected(true,selected,9,true).value);
        auto read=s.read();assert(read.value&&read.value->filters_other_socket_namespaces&&read.value->expected_socket_network_namespace_cookie==9);
        s.stop();assert(s.start_selected(true,selected,9).value);read=s.read();assert(read.value&&!read.value->filters_other_socket_namespaces);
        s.stop();f.clean();}
    for(int operation=1;operation<=8;++operation)for(bool throws:{false,true}){Fake f;if(throws)f.throws=operation;else f.fail=operation;
        {LinuxPrivateNetworkSession s(f.api());bool caught=false;try{assert(!s.start_selected(true,selected,9).value);}catch(const std::runtime_error&){caught=true;}
            assert(caught==throws);assert(s.state()==PrivateNetworkSessionState::stopped);}f.clean();}
    {Fake f;{LinuxPrivateNetworkSession s(f.api());auto started=s.start_selected(true,selected,9);assert(started.value&&started.value->kernel_tgid==456);
        assert(f.calls==8&&f.next==7&&f.closes==0);auto value=s.read();assert(value.value&&value.value->target_kind==PrivateNetworkTargetKind::selected_actor&&value.value->identity.kernel_real_uid==0);
        assert(!s.read_caller_identity().value); // selected metadata must not masquerade as SELF
        assert(!s.start(true).value&&!s.start_selected(true,selected,9).value);s.detach();value=s.read();assert(value.value&&value.value->target_kind==PrivateNetworkTargetKind::selected_actor);
        s.stop();f.clean();assert(s.start(true).value);value=s.read();assert(value.value&&value.value->target_kind==PrivateNetworkTargetKind::self);
    }f.clean();}
    // Every acquisition, discovery lookup and attachment fails/throws in turn.
    for(int operation=1;operation<=14;++operation)for(bool throws:{false,true}){
        Fake f;if(throws)f.throws=operation;else f.fail=operation;
        {LinuxPrivateNetworkSession s(f.api());bool caught=false;try{assert(!s.start(true).value);}catch(const std::runtime_error&){caught=true;}
            assert(caught==throws);assert(s.state()==PrivateNetworkSessionState::stopped);s.stop();}
        f.clean();
    }
    for(unsigned field=0;field<3;++field){Fake f;f.identity[field]=field==1?UINT64_MAX:0;LinuxPrivateNetworkSession s(f.api());
        assert(!s.start(true).value);assert(f.attaches==0);f.clean();}
    {Fake f;f.identity[0]=2147483648ULL;LinuxPrivateNetworkSession s(f.api());assert(!s.start(true).value);f.clean();}
    {Fake f;f.lookup_support=Support::estimated;LinuxPrivateNetworkSession s(f.api());assert(!s.start(true).value);f.clean();}
    {Fake f;f.bad_return=true;LinuxPrivateNetworkSession s(f.api());assert(!s.start(true).value);f.clean();}
    {Fake f;{
        LinuxPrivateNetworkSession s(f.api());auto identity=s.start(true);assert(identity.value&&identity.value->kernel_tgid==123);
        assert(f.calls==14&&f.next==9&&f.closes==2);assert(f.order[0]==1&&f.order[1]==0); // fd0 also closed
        const auto calls=f.calls;assert(!s.start(true).value&&f.calls==calls);
        auto r=s.read();assert((r.value&&r.value->state==PrivateNetworkSessionState::active&&r.value->counters.bytes==std::array<std::uint64_t,4>{}));
        assert(!r.value->expected_socket_network_namespace_cookie);
        f.values={100,200,300,400,0,2,0,0,10};r=s.read();assert(r.value&&r.value->counters.bytes[3]==400&&r.value->counters.excluded_positive_receives==2);
        for(unsigned field:{4u,6u,7u}){f.values[field]=1;const auto before=f.lookups;assert(!s.read().value);
            assert(f.lookups-before==(field==7?88:11));f.values[field]=0;}
        f.moving=true;auto before=f.lookups;assert(!s.read().value&&f.lookups-before==88);f.moving=false;
        f.lookup_denied=true;r=s.read();assert(!r.value&&r.support==Support::permission_required);f.lookup_denied=false;
        f.lookup_support=Support::estimated;assert(!s.read().value);f.lookup_support=Support::available;
        s.detach();assert(s.state()==PrivateNetworkSessionState::detached&&f.closes==8);
        assert((f.order==std::array<int,64>{1,0,8,7,6,5,4,3}));
        r=s.read();assert(r.value&&r.value->state==PrivateNetworkSessionState::detached);
        before=f.calls;assert(!s.start(true).value&&f.calls==before);s.detach();assert(f.closes==8);
        s.stop();s.stop();assert(!s.read().value);f.clean();
        assert(s.start(true).value); // fresh identity and fresh map after stop
    }f.clean();}
    {Fake f;{LinuxPrivateNetworkSession s(f.api());assert(s.start(true).value);}f.clean();}
    {Fake f;{LinuxPrivateNetworkSession s(f.api());assert(s.start(true,0xaabbccddeeff0011ULL).value);
        auto r=s.read();assert(r.value&&r.value->expected_socket_network_namespace_cookie==0xaabbccddeeff0011ULL);
        f.values[4]=1;assert(!s.read().value);s.stop();assert(s.start(true).value);f.values[4]=0;
        r=s.read();assert(r.value&&!r.value->expected_socket_network_namespace_cookie);}f.clean();}
}
