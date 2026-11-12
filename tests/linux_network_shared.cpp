#include "../src/linux_network_shared.hpp"
#include <bit>
#include <cassert>
#include <cstring>
#include <map>
#include <functional>
#include <stdexcept>
using namespace pcinsight;using namespace pcinsight::detail;
namespace {
LinuxNetworkBtfLayout layout(){LinuxNetworkBtfLayout l;l.inet_send_function_id=10;l.inet6_send_function_id=11;l.receive_function_id=12;
    l.socket_sk_offset=24;l.sock_family_offset=16;l.sock_protocol_offset=516;l.sock_protocol_width=1;
    l.task_group_leader_offset=2480;l.task_start_boottime_offset=2728;l.socket_namespace={LinuxSocketNamespaceBtfLayout{64,128},Support::available,{}};return l;}
std::vector<PrivateNetworkIdentity> actors(unsigned n){std::vector<PrivateNetworkIdentity> a;for(unsigned i=0;i<n;++i)a.push_back({123+i,i%2?1000u:0u,900+i});return a;}
struct Fake {
    std::array<bool,128> live{},is_link{};int next=1,array=-1,hash=-1,calls=0,fail=-1,throw_at=-1,loads=0,links=0,closed_links=0;
    std::uint64_t process=7,time=1'000'000'000,span=100;int clocks=0;
    bool overlap=false;Support quality=Support::available;
    std::vector<std::uint64_t> values;std::map<std::uint32_t,std::uint64_t> index;
    std::function<void()> after_hash;
    static Fake& f(void* p){return *static_cast<Fake*>(p);}
    bool fault(){++calls;if(calls==throw_at)throw std::runtime_error("injected");return calls==fail;}
    int fd(){assert(next<128);live[next]=true;return next++;}
    void clean(){for(bool b:live)assert(!b);assert(links==closed_links);}
    NetworkSessionApi api(){return {this,
        [](void* p)->Metric<LinuxNetworkBtfLayout>{auto& s=f(p);if(s.fault())return {{},Support::permission_required,"layout denied"};return {layout(),s.quality,{}};},
        [](void* p,unsigned n)->NetworkSessionFd{auto& s=f(p);if(s.fault())return {-1,13};s.values.assign(n,0);s.array=s.fd();return {s.array,0};},
        [](void* p,std::span<const NetworkProbeInstruction> c,std::uint32_t h,bool u)->NetworkSessionFd{auto& s=f(p);if(s.fault())return {-1,13};assert(!u&&(h==10||h==11||h==12)&&c.size()<4096);++s.loads;return {s.fd(),0};},
        [](void*,int)->NetworkSessionRun{assert(false);return {};},
        [](void* p,int fd,std::uint32_t k)->Metric<std::uint64_t>{auto& s=f(p);assert(s.live[fd]);if(s.fault())return {{},Support::permission_required,"read denied"};
            if(fd==s.hash){auto at=s.index.find(k);if(at==s.index.end())return {{},Support::unsupported,"missing"};return {at->second,s.quality,{}};}
            assert(fd==s.array&&k<s.values.size());if(s.overlap&&k==8)++s.values[8];return {s.values.at(k),s.quality,{}};},
        [](void* p,int fd)->NetworkSessionFd{auto& s=f(p);assert(s.live[fd]);if(s.fault())return {-1,13};++s.links;const auto link=s.fd();s.is_link[link]=true;return {link,0};},
        [](void* p,int fd) noexcept {auto& s=f(p);assert(s.live[fd]);if(s.is_link[fd])++s.closed_links;s.live[fd]=false;},
        [](void* p,int fd,std::uint32_t k,std::uint64_t v)->int{auto& s=f(p);assert(s.live[fd]);if(s.fault())return 13;if(fd==s.hash)s.index[k]=v;else{assert(fd==s.array);s.values.at(k)=v;}return 0;},
        [](void* p,unsigned n)->NetworkSessionFd{auto& s=f(p);if(s.fault())return {-1,13};assert(n>=1&&n<=64);s.index.clear();s.hash=s.fd();return {s.hash,0};},
        [](void* p,int fd,std::uint32_t k)->int{auto& s=f(p);assert(fd==s.hash&&s.live[fd]);if(s.fault())return 13;return s.index.erase(k)==1?0:2;}};}
    NetworkSamplerApi clock(){return {this,nullptr,
        [](void* p)->Metric<std::uint64_t>{auto& s=f(p);return {s.time+((s.clocks++%2)?s.span:0),s.quality,{}};},
        [](void* p) noexcept ->std::uint64_t{return f(p).process;},nullptr};}
    void tick(){time+=1'000'000;clocks=0;}
};
struct Event {std::uint32_t actor=123,uid=0,flags=0;std::uint64_t start=900,cookie=10;int result=1024,failed=-1;unsigned family=2,protocol=6;bool null_task=false,null_socket=false;};
struct Seen {unsigned contexts=0,reads=0,hashes=0,counters=0;};
// Narrow instruction interpreter for this generator, not a kernel verifier.
Seen execute(const std::vector<NetworkProbeInstruction>& code,const LinuxNetworkBtfLayout& l,Fake& f,const Event& e){
    constexpr std::uint64_t memory=0x100000,table=0x200000;Seen seen;std::array<std::uint64_t,11> r{};r[1]=1000;r[10]=4096;std::array<unsigned char,128> stack{};
    auto write=[&](std::uint64_t at,unsigned size,std::uint64_t value){assert(at>=3968&&at+size<=4096);std::memcpy(stack.data()+at-3968,&value,size);};
    auto read=[&](std::uint64_t at,unsigned size){assert(at>=3968&&at+size<=4096);std::uint64_t v=0;std::memcpy(&v,stack.data()+at-3968,size);return v;};
    std::uint64_t selected_slot=0;unsigned steps=0;
    for(std::size_t pc=0;pc<code.size();++pc){assert(++steps<4096);const auto i=code[pc];const auto d=i.registers&15,s=i.registers>>4;assert(d<11&&s<11);
        auto address=[&]{return r[s]+static_cast<std::int64_t>(i.offset);};
        switch(i.code){
        case 0xbf:r[d]=r[s];break;case 0xbc:r[d]=static_cast<std::uint32_t>(r[s]);break;
        case 0x85:
            if(i.immediate==14)r[0]=(std::uint64_t{e.actor}<<32)|42;
            else if(i.immediate==15)r[0]=(1234ULL<<32)|e.uid;
            else if(i.immediate==35)r[0]=e.null_task?0:7000;
            else if(i.immediate==113){unsigned field=99;std::uint64_t v=0;++seen.reads;
                if(r[3]==7000+l.task_group_leader_offset){field=0;v=8000;}
                if(r[3]==8000+l.task_start_boottime_offset){field=1;v=e.start;}
                if(r[3]==5000+l.socket_sk_offset){field=2;v=6000;}
                if(r[3]==6000+l.socket_namespace.value->sock_net_pointer_offset){field=3;v=9000;}
                if(r[3]==9000+l.socket_namespace.value->net_cookie_offset){field=4;v=e.cookie;}
                if(r[3]==6000+l.sock_family_offset){field=5;v=e.family;}
                if(r[3]==6000+l.sock_protocol_offset){field=6;v=e.protocol;}
                assert(field!=99&&(r[2]==1||r[2]==2||r[2]==8));write(r[1],static_cast<unsigned>(r[2]),v);r[0]=static_cast<int>(field)==e.failed?static_cast<std::uint64_t>(-14LL):0;
            }else{assert(i.immediate==1);const auto key=static_cast<std::uint32_t>(read(r[2],4));if(r[1]==static_cast<unsigned>(f.hash)){++seen.hashes;const auto at=f.index.find(key);selected_slot=at==f.index.end()?0:at->second;r[0]=at==f.index.end()?0:table;}
                else{assert(r[1]==static_cast<unsigned>(f.array));++seen.counters;r[0]=key<f.values.size()?memory+8*key:0;}
                if(r[1]==static_cast<unsigned>(f.hash)&&f.after_hash){auto callback=std::move(f.after_hash);f.after_hash={};callback();}}
            break;
        case 0x79:if(r[s]==1000){++seen.contexts;if(i.offset==0)r[d]=e.null_socket?0:5000;else if(i.offset==16)r[d]=e.flags;else{assert(i.offset==24);r[d]=static_cast<std::uint32_t>(e.result);}}
            else if(r[s]==table){assert(i.offset==0);r[d]=selected_slot;}
            else if(address()>=memory){assert(address()<memory+8*f.values.size());r[d]=f.values.at((address()-memory)/8);}
            else r[d]=read(address(),8);break;
        case 0x61:r[d]=read(address(),4);break;case 0x69:r[d]=read(address(),2);break;case 0x71:r[d]=read(address(),1);break;
        case 0x7b:write(r[d]+static_cast<std::int64_t>(i.offset),8,r[s]);break;
        case 0x7a:write(r[d]+static_cast<std::int64_t>(i.offset),8,static_cast<std::uint64_t>(i.immediate));break;
        case 0x63:write(r[d]+static_cast<std::int64_t>(i.offset),4,r[s]);break;
        case 0x62:write(r[d]+static_cast<std::int64_t>(i.offset),4,static_cast<std::uint32_t>(i.immediate));break;
        case 0x18:r[d]=static_cast<std::uint32_t>(i.immediate)|(std::uint64_t{static_cast<std::uint32_t>(code.at(++pc).immediate)}<<32);break;
        case 0x77:r[d]>>=i.immediate;break;case 0x67:r[d]<<=i.immediate;break;case 0xc7:r[d]=static_cast<std::uint64_t>(std::bit_cast<std::int64_t>(r[d])>>i.immediate);break;
        case 0x07:r[d]+=static_cast<std::int64_t>(i.immediate);break;case 0x27:r[d]*=i.immediate;break;case 0x0f:r[d]+=r[s];break;case 0x57:r[d]&=i.immediate;break;
        case 0x15:if(r[d]==static_cast<std::uint64_t>(i.immediate))pc+=i.offset;break;case 0x55:if(r[d]!=static_cast<std::uint64_t>(i.immediate))pc+=i.offset;break;
        case 0x25:if(r[d]>static_cast<std::uint64_t>(i.immediate))pc+=i.offset;break;case 0x5d:if(r[d]!=r[s])pc+=i.offset;break;case 0x3d:if(r[d]>=r[s])pc+=i.offset;break;
        case 0xd5:if(std::bit_cast<std::int64_t>(r[d])<=i.immediate)pc+=i.offset;break;case 0x05:pc+=i.offset;break;
        case 0xdb:{assert(r[d]>=memory&&(r[d]-memory)%8==0);const auto key=(r[d]-memory)/8;auto& value=f.values.at(key);if(key!=7)assert(f.values[7]>0);const auto old=value;
            if(i.immediate==0xf1){if(old==r[0])value=r[s];r[0]=old;}else if(i.immediate==0x41){value|=r[s];r[s]=old;}else{assert(i.immediate==0||i.immediate==1);value+=r[s];if(i.immediate==1)r[s]=old;}break;}
        case 0xb7:r[d]=static_cast<std::uint64_t>(i.immediate);break;case 0x95:assert(r[0]==0);return seen;default:assert(false);
        }
    }assert(false);return seen;
}
void probe_tests(){const auto l=layout();auto selected=actors(64);Fake f;LinuxSharedNetworkSession session(f.api(),f.clock());assert(session.start(true,selected).value);assert(f.loads==3&&f.links==3);
    auto code=make_private_shared_network_probe(f.hash,f.array,64,false,l);auto receive=make_private_shared_network_probe(f.hash,f.array,64,true,l);assert(code.value&&receive.value&&code.value->size()<4096);
    Event e;e.actor=999;auto seen=execute(*code.value,l,f,e);assert(seen.hashes==1&&!seen.counters&&!seen.contexts&&!seen.reads);
    e={};f.values[0]=0;seen=execute(*code.value,l,f,e);assert(!seen.contexts&&!seen.reads);f.values[0]=1;
    for(unsigned i=0;i<64;++i)for(unsigned cookie=0;cookie<16;++cookie){const auto& a=selected[i];e={};e.actor=a.kernel_tgid;e.uid=a.kernel_real_uid;e.start=a.leader_start_boottime_ns;e.cookie=100+cookie;e.protocol=cookie%2?17:6;
        e.result=1+static_cast<int>(i);execute(cookie%4<2?*code.value:*receive.value,l,f,e);}
    auto all=session.read();assert(all.value&&all.value->actors.size()==64&&all.value->completed_updates==1024);for(unsigned i=0;i<64;++i){assert(all.value->actors[i].buckets.size()==16);for(const auto& b:all.value->actors[i].buckets){std::uint64_t sum=0;for(auto bytes:b.bytes)sum+=bytes;assert(sum==i+1);}}
    // Only the final actor's registry fills; failure rejects the whole frame.
    e.actor=selected.back().kernel_tgid;e.uid=selected.back().kernel_real_uid;e.start=selected.back().leader_start_boottime_ns;e.cookie=1000;execute(*code.value,l,f,e);f.tick();assert(!session.read().value);
    assert(session.retain(true,std::span(selected).first(63)).value);f.tick();all=session.read();assert(all.value&&all.value->actors.size()==63&&all.value->completed_updates==1025);
    f.values[4]=1;f.tick();assert(!session.read().value); // GLOBAL quality cannot be hidden by removing an actor.
    session.stop();f.clean();
    for(unsigned fault=0;fault<17;++fault){Fake s;LinuxSharedNetworkSession capture(s.api(),s.clock());const auto a=actors(2);assert(capture.start(true,a).value);const auto send=make_private_shared_network_probe(s.hash,s.array,2,false,l);const auto recv=make_private_shared_network_probe(s.hash,s.array,2,true,l);e={};
        if(fault==0)++e.start;if(fault==1)++e.uid;if(fault==2)e.null_task=true;if(fault==3)e.null_socket=true;if(fault>=4&&fault<=10)e.failed=static_cast<int>(fault)-4;
        if(fault==11)e.cookie=0;if(fault==12)s.index[123]=0;if(fault==13)s.index[123]=3;if(fault==14)s.values[0]=2;if(fault==15)s.values[8]=UINT64_MAX;
        if(fault==16){e.flags=2;execute(*recv.value,l,s,e);auto r=capture.read();assert(r.value&&r.value->actors[0].buckets[0].excluded_positive_receives==1&&r.value->actors[1].buckets.empty());}
        else{execute(*send.value,l,s,e);assert(s.values[7]==0);assert(!capture.read().value);}capture.stop();s.clean();}
    for(bool recv:{false,true})for(unsigned family:{2u,10u})for(unsigned protocol:{6u,17u}){Fake s;LinuxSharedNetworkSession c(s.api(),s.clock());assert(c.start(true,actors(1)).value);auto p=make_private_shared_network_probe(s.hash,s.array,1,recv,l);e={};e.family=family;e.protocol=protocol;execute(*p.value,l,s,e);auto r=c.read();assert(r.value&&r.value->actors[0].buckets[0].bytes[(protocol==17?2:0)+(recv?1:0)]==1024);c.stop();s.clean();}
}
void session_tests(){const auto a=actors(2);
    {Fake f;LinuxSharedNetworkSession s(f.api(),f.clock());assert(!s.start(false,a).value&&f.calls==0);assert(!s.start(true,{}).value);assert(!s.start(true,actors(65)).value);auto bad=a;bad[1].kernel_tgid=bad[0].kernel_tgid;assert(!s.start(true,bad).value&&f.calls==0);assert(!s.read().value);f.clean();}
    Fake count;{LinuxSharedNetworkSession s(count.api(),count.clock());assert(s.start(true,a).value);}count.clean();
    for(bool throwing:{false,true})for(int at=1;at<=count.calls;++at){Fake f;{LinuxSharedNetworkSession s(f.api(),f.clock());if(throwing)f.throw_at=at;else f.fail=at;bool caught=false;try{assert(!s.start(true,a).value);}catch(const std::runtime_error&){caught=true;}assert(caught==throwing);}f.clean();}
    for(unsigned fault=0;fault<16;++fault){Fake f;LinuxSharedNetworkSession s(f.api(),f.clock());assert(s.start(true,a).value);auto first=s.read();assert(first.value&&first.value->actors.size()==2&&first.value->actors[0].buckets.empty());f.tick();
        if(fault==0)f.values[4]=1;if(fault==1)f.values[6]=1;if(fault==2)f.values[7]=1;if(fault==3)f.values[8]=1;if(fault==4)f.index[123]=2;
        if(fault==5)f.values[9]++;if(fault==6)f.values[10]++;if(fault==7)f.values[0]=0;if(fault==8)f.quality=Support::estimated;
        if(fault==9)f.span=1'000'000'001;if(fault==10)f.time=1;if(fault==11)f.overlap=true;if(fault==12)f.fail=f.calls+1;
        if(fault==13)f.process=8;if(fault==14)f.throw_at=f.calls+1;if(fault==15)f.values[2]=1;
        bool caught=false;try{assert(!s.read().value);}catch(const std::runtime_error&){caught=true;}assert(caught==(fault==14));s.stop();f.clean();}
    for(unsigned fault=0;fault<4;++fault){Fake f;LinuxSharedNetworkSession s(f.api(),f.clock());assert(s.start(true,actors(1)).value);const auto p=make_private_shared_network_probe(f.hash,f.array,1,false,layout());Event e;
        execute(*p.value,layout(),f,e);execute(*p.value,layout(),f,e);assert(s.read().value);const auto saved=f.values;f.tick();
        if(fault==0)f.values[21]=1024;if(fault==1)++f.values[20];if(fault==2){f.values[8]=1;f.values[19]=1;}if(fault==3)f.values[20]=0;
        assert(!s.read().value);f.values=saved;f.tick();assert(s.read().value);s.stop();f.clean();}
}
void retention_tests(){const auto a=actors(3);const auto l=layout();
    {Fake f;LinuxSharedNetworkSession s(f.api(),f.clock());assert(s.start(true,a).value);auto code=make_private_shared_network_probe(f.hash,f.array,3,false,l);Event e;
        execute(*code.value,l,f,e);auto first=s.read();assert(first.value&&first.value->actors[0].buckets[0].bytes[0]==1024);f.tick();
        const auto calls=f.calls;assert(!s.retain(false,a).value&&f.calls==calls);auto bad=a;bad[0].kernel_real_uid=99;assert(!s.retain(true,bad).value&&f.calls==calls);
        bad={a[0],a[0]};assert(!s.retain(true,bad).value&&f.calls==calls);
        // Pause an event AFTER lookup captured the soon-to-be-retired slot.
        // Removal does not detach/reset the shared producer or reuse that slot.
        f.after_hash=[&]{const std::array keep{a[2],a[0]};assert(s.retain(true,keep).value);};
        e.actor=a[1].kernel_tgid;e.uid=a[1].kernel_real_uid;e.start=a[1].leader_start_boottime_ns;execute(*code.value,l,f,e);
        assert(f.loads==3&&f.links==3&&f.closed_links==0&&!f.index.contains(a[1].kernel_tgid));
        auto r=s.read();assert(r.value&&r.value->actors.size()==2&&r.value->completed_updates==2);
        assert(r.value->actors[0].identity.kernel_tgid==a[0].kernel_tgid&&r.value->actors[0].buckets[0].bytes[0]==1024&&r.value->actors[1].buckets.empty());
        auto seen=execute(*code.value,l,f,e);assert(seen.hashes==1&&!seen.contexts&&!seen.counters&&!seen.reads);
        assert(!s.retain(true,a).value); // no re-admission into a retired slot
        e={};execute(*code.value,l,f,e);f.tick();r=s.read();assert(r.value&&r.value->actors[0].buckets[0].bytes[0]==2048);
        // Retained regression guard survives the selection change.
        --f.values[21];f.tick();assert(!s.read().value);++f.values[21];f.tick();assert(s.read().value);
        assert(s.retain(true,{}).value);assert(!s.read().value);f.clean();}
    // All removed entries, including failure/exception after a partial delete,
    // close every owned FD rather than leave an ambiguous selected set.
    for(bool throwing:{false,true})for(int at:{1,2}){Fake f;LinuxSharedNetworkSession s(f.api(),f.clock());assert(s.start(true,a).value);
        if(throwing)f.throw_at=f.calls+at;else f.fail=f.calls+at;bool caught=false;try{assert(!s.retain(true,std::span(a).first(1)).value);}catch(const std::runtime_error&){caught=true;}
        assert(caught==throwing);assert(!s.read().value);f.clean();}
    {Fake f;auto api=f.api();api.erase=nullptr;LinuxSharedNetworkSession s(api,f.clock());assert(s.start(true,a).value);
        assert(s.retain(true,a).value&&!s.retain(true,std::span(a).first(1)).value);assert(s.read().value->actors.size()==3);s.stop();f.clean();}
    {Fake f;LinuxSharedNetworkSession s(f.api(),f.clock());assert(s.start(true,a).value);f.process=8;
        assert(!s.retain(true,std::span(a).first(1)).value&&f.index.size()==3);f.clean();}
    {Fake f;LinuxSharedNetworkSession s(f.api(),f.clock());assert(s.start(true,a).value);Event e;e.actor=a[2].kernel_tgid;e.uid=a[2].kernel_real_uid;e.start=a[2].leader_start_boottime_ns+1;
        auto code=make_private_shared_network_probe(f.hash,f.array,3,false,l);execute(*code.value,l,f,e);assert(!s.read().value);
        assert(s.retain(true,std::span(a).first(2)).value);f.tick();assert(s.read().value->actors.size()==2);s.stop();f.clean();}
}
}
int main(){if(std::endian::native!=std::endian::little)return 0;probe_tests();session_tests();retention_tests();}
