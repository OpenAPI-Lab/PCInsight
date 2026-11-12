#include "../src/linux_network_pidfd.hpp"
#include <bit>
#include <cassert>
#include <array>
#include <stdexcept>
using namespace pcinsight;using namespace pcinsight::detail;
namespace {
struct Scenario {std::uint64_t caller=(123ULL<<32)|456,armed=(123ULL<<32)|456,task=5000,leader=5000,cred=6000,start=999,uid=1000;std::uint32_t mode=0x12,tgid=789;std::int32_t result=0;int fail=-1;bool no_map=false;};
struct Result {std::array<std::uint64_t,6> values{};unsigned contexts=0,reads=0,maps=0;};
struct QueryFake {
    std::array<bool,256> live{};std::array<unsigned,256> kind{};
    std::array<std::uint64_t,3> caller_values{};std::array<std::uint64_t,5> result{};
    int next=0,maps=0,loads=0,updates=0,lookups=0,attaches=0,detaches=0,link=-1;
    int fail_map=-1,fail_load=-1,fail_update=-1,fail_lookup=-1,fail_run=0;
    bool deny_attach=false,throw_attach=false,throw_update=false;
    std::uint64_t process=7,kernel_pid=123,uid=1000,start=900;std::uint32_t tid=456;
    std::uint64_t gate=0;
    Support quality=Support::available;
    LinuxNetworkBtfLayout layout;
    QueryFake(){layout.task_group_leader_offset=8;layout.task_start_boottime_offset=16;
        layout.pidfd_identity={LinuxPidfdIdentityBtfLayout{37,32,40,8},Support::available,{}};}
    static QueryFake& f(void* p){return *static_cast<QueryFake*>(p);}
    int fd(unsigned type){assert(next<256);live[next]=true;kind[next]=type;return next++;}
    static std::uint64_t guard(void* p) noexcept {return f(p).process;}
    void clean(){assert(link<0);for(bool n:live)assert(!n);}
    void trigger(std::uint64_t target_uid=2000){assert(link>=0&&gate==((kernel_pid<<32)|tid));result={789,target_uid,999,1,0};}
    NetworkSessionApi api(){return {this,
        [](void* p)->Metric<LinuxNetworkBtfLayout>{return {f(p).layout,Support::available,{}};},
        [](void* p,unsigned n)->NetworkSessionFd{auto& s=f(p);assert(s.link<0);if(++s.maps==s.fail_map)return {-1,13};assert(n==3||n==6);return {s.fd(n),0};},
        [](void* p,std::span<const NetworkProbeInstruction> code,std::uint32_t hook,bool unattached)->NetworkSessionFd{auto& s=f(p);assert(s.gate==0);if(++s.loads==s.fail_load)return {-1,13};
            assert((hook==0&&unattached)||(hook==37&&!unattached));return {s.fd(hook?12:code.size()==2?11:10),0};},
        [](void* p,int fd)->NetworkSessionRun{auto& s=f(p);assert(s.live[fd]&&(s.gate==0||s.link<0));if(s.kind[fd]==10){if(s.fail_run==1)return {13,0};s.caller_values={s.kernel_pid,s.uid,s.start};return {};}
            assert(s.kind[fd]==11);return s.fail_run==2?NetworkSessionRun{13,0}:NetworkSessionRun{0,s.tid};},
        [](void* p,int fd,std::uint32_t key)->Metric<std::uint64_t>{auto& s=f(p);assert(s.live[fd]&&(s.gate==0||s.link<0));if(++s.lookups==s.fail_lookup)return {{},Support::permission_required,"lookup denied"};
            return {s.kind[fd]==3?s.caller_values.at(key):s.result.at(key),s.quality,{}};},
        [](void* p,int fd)->NetworkSessionFd{auto& s=f(p);assert(s.live[fd]&&s.kind[fd]==12&&s.link<0);++s.attaches;if(s.throw_attach)throw std::runtime_error("attach");if(s.deny_attach)return {-1,13};s.link=s.fd(13);return {s.link,0};},
        [](void* p,int fd) noexcept {auto& s=f(p);assert(s.live[fd]);if(fd==s.link){s.link=-1;++s.detaches;}s.live[fd]=false;},
        [](void* p,int fd,std::uint32_t key,std::uint64_t value)->int{auto& s=f(p);assert(s.live[fd]&&(s.gate==0||s.link<0||(s.kind[fd]==6&&key==5&&value==0)));if(s.throw_update)throw std::runtime_error("update");if(++s.updates==s.fail_update)return 13;
            if(s.kind[fd]==6&&key==5){s.gate=value;return 0;}assert(value==0);
            if(s.kind[fd]==3)s.caller_values.at(key)=value;else{assert(s.kind[fd]==6);s.result.at(key)=value;}return 0;}};}
};
void test_reusable(){
    {QueryFake f;LinuxPidfdIdentityQuery q(f.api(),&f,QueryFake::guard);assert(!q.start(false,f.layout).value&&f.maps==0);assert(!q.begin().value&&!q.finish().value);f.clean();}
    for(unsigned fault=0;fault<6;++fault){QueryFake f;auto api=f.api();auto l=f.layout;
        if(fault<2)f.fail_map=1+fault;if(fault==2||fault==3)f.fail_load=fault-1;if(fault==4)api.update=nullptr;if(fault==5)l.pidfd_identity.value.reset();
        LinuxPidfdIdentityQuery q(api,&f,QueryFake::guard);assert(!q.start(true,l).value);f.clean();}
    {QueryFake f;LinuxPidfdIdentityQuery q(f.api(),&f,QueryFake::guard);assert(q.start(true,f.layout).value&&f.loads==2);assert(!q.start(true,f.layout).value);
        for(unsigned i=0;i<40;++i){assert(q.begin().value&&f.loads==3);assert(!q.begin().value&&f.link>=0);f.trigger(i);const auto r=q.finish();assert(r.value&&r.value->kernel_real_uid==i&&f.gate==0);}
        assert(f.attaches==1&&f.detaches==0);++f.tid;assert(q.begin().value&&f.loads==3&&f.attaches==1);f.trigger();assert(q.finish().value);
        assert(q.begin().value);assert(!q.finish().value); // No trigger: stale previous success was erased.
        assert(q.begin().value);f.trigger();f.result[3]=2;assert(!q.finish().value);
        assert(q.begin().value);q.cancel();assert(!q.finish().value);q.stop();q.stop();f.clean();
        assert(q.start(true,f.layout).value);assert(q.begin().value);f.trigger();assert(q.finish().value);q.stop();f.clean();}
    for(unsigned fault=0;fault<17;++fault){QueryFake f;{
        LinuxPidfdIdentityQuery q(f.api(),&f,QueryFake::guard);assert(q.start(true,f.layout).value);
        if(fault==0)f.fail_update=1;if(fault==1)f.fail_update=4;if(fault==2)f.fail_run=1;if(fault==3)f.fail_run=2;
        if(fault==4)f.fail_lookup=1;if(fault==5)f.quality=Support::estimated;if(fault==6)f.kernel_pid=0;
        if(fault==7)f.uid=UINT64_MAX;if(fault==8)f.start=0;if(fault==9)f.tid=0;if(fault==10)f.tid=UINT32_MAX;
        if(fault==11)f.fail_load=3;if(fault==12)f.deny_attach=true;if(fault==13)f.throw_attach=true;if(fault==14)f.throw_update=true;
        if(fault==15)f.process=8;if(fault==16)f.kernel_pid=2147483648ULL;
        bool threw=false;try{assert(!q.begin().value);}catch(const std::runtime_error&){threw=true;}assert(threw==(fault==13||fault==14));assert(f.link<0);
    }f.clean();}
    for(unsigned fault=0;fault<4;++fault){QueryFake f;{
        LinuxPidfdIdentityQuery q(f.api(),&f,QueryFake::guard);assert(q.start(true,f.layout).value);assert(q.begin().value);f.trigger();
        if(fault==0)f.fail_lookup=f.lookups+1;if(fault==1)f.quality=Support::estimated;if(fault==2)f.process=8;if(fault==3)f.result[4]=1;
        assert(!q.finish().value&&(f.gate==0||fault==2));
    }f.clean();}
    for(unsigned fault=0;fault<3;++fault){QueryFake f;LinuxPidfdIdentityQuery q(f.api(),&f,QueryFake::guard);assert(q.start(true,f.layout).value);assert(q.begin().value);f.trigger();
        if(fault==0)f.fail_update=f.updates+1;if(fault==1)f.throw_update=true;
        if(fault==2){f.process=8;const auto updates=f.updates;assert(!q.finish().value&&f.updates==updates);f.clean();continue;}
        assert(!q.finish().value&&f.link<0);f.throw_update=false;f.fail_update=-1;
        // The stale gate is reset before attaching any replacement link.
        assert(q.begin().value);f.trigger();assert(q.finish().value);q.stop();f.clean();}
    for(int at=5;at<=10;++at){QueryFake f;LinuxPidfdIdentityQuery q(f.api(),&f,QueryFake::guard);assert(q.start(true,f.layout).value);f.fail_update=at;
        assert(!q.begin().value&&f.gate==0);f.fail_update=-1;assert(q.begin().value);f.trigger();assert(q.finish().value);q.stop();f.clean();}
}
Result run(const std::vector<NetworkProbeInstruction>& code,const LinuxNetworkBtfLayout& l,const Scenario& s){
    std::array<std::uint64_t,11> r{};r[1]=1000;r[10]=2000;std::array<std::uint64_t,6> stack{};Result out;unsigned steps=0;std::uint32_t key=99;
    for(std::size_t pc=0;pc<code.size();++pc){assert(++steps<256);const auto i=code[pc];const auto d=i.registers&15,v=i.registers>>4;assert(d<11&&v<11);
        switch(i.code){
        case 0xbf:r[d]=r[v];break;case 0xbc:r[d]=static_cast<std::uint32_t>(r[v]);break;
        case 0x18:r[d]=static_cast<std::uint32_t>(i.immediate)|(std::uint64_t{static_cast<std::uint32_t>(code.at(++pc).immediate)}<<32);break;
        case 0xb7:r[d]=static_cast<std::uint64_t>(i.immediate);break;case 0x07:r[d]+=i.immediate;break;
        case 0x5d:if(r[d]!=r[v])pc+=i.offset;break;case 0x55:if(r[d]!=static_cast<std::uint64_t>(i.immediate))pc+=i.offset;break;
        case 0x15:if(r[d]==static_cast<std::uint64_t>(i.immediate))pc+=i.offset;break;case 0x05:pc+=i.offset;break;
        case 0x85:
            if(i.immediate==14)r[0]=s.caller;
            else if(i.immediate==113){assert(r[1]==1984&&(r[2]==4||r[2]==8));int field=-1;std::uint64_t value=0;++out.reads;
                if(r[3]==1000){field=0;value=s.task;++out.contexts;}
                else if(r[3]==5000+l.task_group_leader_offset){field=1;value=s.leader;}
                else if(r[3]==5000+l.pidfd_identity.value->task_tgid_offset){field=2;value=s.tgid;}
                else if(r[3]==5000+l.task_start_boottime_offset){field=3;value=s.start;}
                else if(r[3]==5000+l.pidfd_identity.value->task_cred_offset){field=4;value=s.cred;}
                else if(r[3]==6000+l.pidfd_identity.value->cred_uid_offset){field=5;value=s.uid;}
                assert(field>=0);if(r[2]==4)value=static_cast<std::uint32_t>(value);stack[2]=value;r[0]=field==s.fail?static_cast<std::uint64_t>(-14LL):0;
            }else{assert(i.immediate==1&&r[1]==50&&r[2]==1996);assert(key<6);++out.maps;r[0]=s.no_map?0:3000+key*8;}
            break;
        case 0x79:
            if(v==6){assert(r[6]==1000);++out.contexts;assert(i.offset==8||i.offset==16);r[d]=i.offset==8?s.mode:static_cast<std::uint64_t>(s.result);}
            else if(v==0){assert(r[0]==3040&&i.offset==0);r[d]=s.armed;}
            else{assert(v==10&&i.offset<0&&i.offset%8==0);r[d]=stack[-i.offset/8];}break;
        case 0x61:assert(v==10&&i.offset==-16);r[d]=static_cast<std::uint32_t>(stack[2]);break;
        case 0x7a:assert(d==10&&i.offset==-16);stack[2]=0;break;
        case 0x62:assert(d==10&&i.offset==-4);key=static_cast<std::uint32_t>(i.immediate);break;
        case 0x7b:if(d==10){assert(i.offset<0&&i.offset%8==0);stack[-i.offset/8]=r[v];}
            else{assert(r[d]>=3000&&(r[d]-3000)%8==0);out.values[(r[d]-3000)/8]=r[v];}break;
        case 0xdb:assert(r[d]>=3000&&(r[d]-3000)%8==0);out.values[(r[d]-3000)/8]+=r[v];break;
        case 0x95:assert(r[0]==0);return out;
        default:assert(false);
        }
    }assert(false);return out;
}
}
int main(){
    LinuxNetworkBtfLayout l;l.task_group_leader_offset=8;l.task_start_boottime_offset=16;l.pidfd_identity={LinuxPidfdIdentityBtfLayout{37,32,40,8},Support::available,{}};
    const auto caller=(123ULL<<32)|456;auto code=make_private_pidfd_identity_probe(caller,50,l);
    assert(!read_private_pidfd_network_identity(false,0).value);assert(!read_private_pidfd_network_identity(true,-1).value);
    if(std::endian::native!=std::endian::little){assert(!code.value);return 0;}assert(code.value);
    test_reusable();
    Scenario s;auto out=run(*code.value,l,s);assert((out.values==std::array<std::uint64_t,6>{789,1000,999,1,0,0}));
    const auto gated=make_private_pidfd_identity_probe(caller,50,l,true);assert(gated.value);out=run(*gated.value,l,s);assert(out.values[0]==789&&out.values[3]==1);
    for(auto gate:{0ULL,caller+1,caller+(1ULL<<32)}){s={};s.armed=gate;out=run(*gated.value,l,s);assert(out.contexts==0&&out.reads==0&&out.maps==1);}
    s={};s.no_map=true;out=run(*gated.value,l,s);assert(out.contexts==0&&out.reads==0);s={};
    for(auto mode:{0u,1u,0x11u,0x22u}){s={};s.mode=mode;out=run(*gated.value,l,s);assert(out.reads==0&&out.maps==1);}
    for(int result:{-1,-13,1}){s={};s.result=result;out=run(*gated.value,l,s);assert(out.reads==0&&out.maps==1);}
    for(int failure=0;failure<6;++failure){s={};s.fail=failure;out=run(*gated.value,l,s);assert(out.values[4]==1&&out.values[3]==0);}s={};
    for(auto uid:{0ULL,4294967295ULL}){s.uid=uid;out=run(*code.value,l,s);assert(out.values[1]==uid&&out.values[3]==1);}
    for(unsigned kind=0;kind<2;++kind){s={};s.caller+=kind?1:(1ULL<<32);out=run(*code.value,l,s);assert(out.contexts==0&&out.reads==0&&out.maps==0);}
    for(unsigned mode:{0u,1u,2u,0x11u,0x22u}){s={};s.mode=mode;out=run(*code.value,l,s);assert(out.reads==0&&out.maps==0);}
    for(int result:{-1,-13,1,2147483647}){s={};s.result=result;out=run(*code.value,l,s);assert(out.reads==0&&out.maps==0);}
    for(int failure=0;failure<6;++failure){s={};s.fail=failure;out=run(*code.value,l,s);assert(out.values[4]==1&&out.values[3]==0);}
    for(unsigned bad=0;bad<3;++bad){s={};if(bad==0)s.task=0;if(bad==1)s.leader=7000;if(bad==2)s.cred=0;
        out=run(*code.value,l,s);assert(out.values[4]==1&&out.values[3]==0);}
    s={};s.no_map=true;out=run(*code.value,l,s);assert((out.values==std::array<std::uint64_t,6>{}));
    for(unsigned bad=0;bad<6;++bad){auto invalid=l;if(bad==0)invalid.pidfd_identity.support=Support::estimated;if(bad==1)invalid.pidfd_identity.value.reset();
        if(bad==2)invalid.pidfd_identity.value->function_id=0;if(bad==3)invalid.pidfd_identity.value->task_tgid_offset=1048577;
        if(bad==4)invalid.pidfd_identity.value->task_cred_offset=1048577;if(bad==5)invalid.pidfd_identity.value->cred_uid_offset=1048577;
        assert(!make_private_pidfd_identity_probe(caller,50,invalid).value);}
    assert(!make_private_pidfd_identity_probe(0,50,l).value);assert(!make_private_pidfd_identity_probe(caller,-1,l).value);
    const std::array<std::uint64_t,5> valid{789,0,999,1,0};assert(validate_private_pidfd_identity_values(valid).value);
    for(unsigned fault=0;fault<7;++fault){auto bad=valid;if(fault==0)bad[0]=0;if(fault==1)bad[0]=2147483648ULL;
        if(fault==2)bad[1]=4294967296ULL;if(fault==3)bad[2]=0;if(fault==4)bad[3]=0;if(fault==5)bad[3]=2;if(fault==6)bad[4]=1;
        assert(!validate_private_pidfd_identity_values(bad).value);}
}
