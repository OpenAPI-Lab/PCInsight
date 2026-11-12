#include "../src/linux_network_shared_selected.hpp"
#include "../src/linux_process_network.hpp"
#include <cassert>
#include <algorithm>
#include <bit>
#include <map>
#include <stdexcept>
using namespace pcinsight;using namespace pcinsight::detail;
namespace {
struct Fake {
    std::array<bool,4096> live{};std::map<int,unsigned> held;int next=1,next_held=10000,array=-1,hash=-1,self_map=-1,calls=0,fail=-1,throw_at=-1;
    unsigned loads=0,links=0,joins=0,holds=0,clocks=0,batches=0;int deny_actor=-1,throw_actor=-1;unsigned deny_join=0,drift_join=0,fail_batch=0,throw_batch=0;
    std::uint64_t process=7,time=1'000'000'000,span=10;Support quality=Support::available,join_quality=Support::available;
    std::vector<std::uint64_t> values;std::map<std::uint32_t,std::uint64_t> index;
    std::map<int,std::vector<std::uint64_t>> arrays;std::map<int,std::map<std::uint32_t,std::uint64_t>> hashes;std::map<int,int> table_arrays;
    std::vector<PrivatePidfdNetworkJoin> identities=std::vector<PrivatePidfdNetworkJoin>(LinuxProcessNetworkSampler::max_processes);
    Fake(){for(unsigned i=0;i<identities.size();++i)identities[i]={{123+i,i%2?2000u:0u,900+i},{{1,2},{7,43,100,1,2,3,4,5,6,7},{8+i,44,200+i,1,2,8+i,4,9,6,10+i}}};}
    static Fake& f(void* p){return *static_cast<Fake*>(p);}
    bool fault(){++calls;if(calls==throw_at)throw std::runtime_error("source");return calls==fail;}
    int fd(){assert(next<4096);live[next]=true;return next++;}
    auto& array_values(int fd){return fd==array?values:arrays.at(fd);}
    auto& hash_values(int fd){return fd==hash?index:hashes.at(fd);}
    void clean(){assert(held.empty());for(bool b:live)assert(!b);}
    NetworkSessionApi api(){return {this,
        [](void* p)->Metric<LinuxNetworkBtfLayout>{auto& s=f(p);if(s.fault())return {{},Support::permission_required,"layout"};LinuxNetworkBtfLayout l;l.inet_send_function_id=10;l.inet6_send_function_id=11;l.receive_function_id=12;l.sock_protocol_width=1;l.socket_namespace={LinuxSocketNamespaceBtfLayout{64,128},Support::available,{}};return {l,s.quality,{}};},
        [](void* p,unsigned n)->NetworkSessionFd{auto& s=f(p);if(s.fault())return {-1,13};if(n!=3){++s.batches;if(s.batches==s.fail_batch)return {-1,13};if(s.batches==s.throw_batch)throw std::runtime_error("batch allocation");}int fd=s.fd();if(n==3)s.self_map=fd;else{if(s.array>=0)s.arrays[s.array]=std::move(s.values);s.array=fd;s.values.assign(n,0);}return {fd,0};},
        [](void* p,std::span<const NetworkProbeInstruction> code,std::uint32_t hook,bool local)->NetworkSessionFd{auto& s=f(p);if(s.fault())return {-1,13};assert(code.size()<4096);if(!local){assert(hook==10||hook==11||hook==12);++s.loads;}return {s.fd(),0};},
        [](void* p,int fd)->NetworkSessionRun{auto& s=f(p);assert(s.live[fd]);return s.fault()?NetworkSessionRun{13,0}:NetworkSessionRun{};},
        [](void* p,int fd,std::uint32_t key)->Metric<std::uint64_t>{auto& s=f(p);assert(s.live[fd]);if(s.fault())return {{},Support::permission_required,"lookup"};
            if(fd==s.self_map){const std::array<std::uint64_t,3> caller{777,1000,8888};return {caller.at(key),s.quality,{}};}
            if(fd==s.hash||s.hashes.contains(fd)){auto& h=s.hash_values(fd);auto at=h.find(key);if(at==h.end())return {{},Support::unsupported,"missing"};return {at->second,s.quality,{}};}
            return {s.array_values(fd).at(key),s.quality,{}};},
        [](void* p,int fd)->NetworkSessionFd{auto& s=f(p);assert(s.live[fd]);if(s.fault())return {-1,13};++s.links;return {s.fd(),0};},
        [](void* p,int fd) noexcept {auto& s=f(p);assert(s.live[fd]);s.live[fd]=false;},
        [](void* p,int fd,std::uint32_t key,std::uint64_t value)->int{auto& s=f(p);assert(s.live[fd]);if(s.fault())return 13;if(fd==s.hash||s.hashes.contains(fd))s.hash_values(fd)[key]=value;else s.array_values(fd).at(key)=value;return 0;},
        [](void* p,unsigned n)->NetworkSessionFd{auto& s=f(p);assert(n<=64);if(s.fault())return {-1,13};if(s.hash>=0)s.hashes[s.hash]=std::move(s.index);s.hash=s.fd();s.index.clear();s.table_arrays[s.hash]=s.array;return {s.hash,0};},
        [](void* p,int fd,std::uint32_t key)->int{auto& s=f(p);assert(s.live[fd]);if(s.fault())return 13;return s.hash_values(fd).erase(key)==1?0:2;}};}
    NetworkSamplerApi clock(){return {this,nullptr,
        [](void* p)->Metric<std::uint64_t>{auto& s=f(p);return {s.time+(s.clocks++)*s.span,s.quality,{}};},
        [](void* p) noexcept ->std::uint64_t{return f(p).process;},nullptr};}
    SelectedNetworkSourceApi source(){return {this,
        [](void* p,int fd)->NetworkSessionFd{auto& s=f(p);assert(fd>=100&&static_cast<std::size_t>(fd-100)<s.identities.size());if(s.fault())return {-1,13};const auto i=static_cast<unsigned>(fd-100);const int copy=s.next_held++;s.held[copy]=i;++s.holds;return {copy,0};},
        [](void* p,int fd) noexcept {auto& s=f(p);assert(s.held.erase(fd)==1);},
        [](void* p,int fd,std::uint32_t pid,std::uint64_t start)->Metric<PrivatePidfdNetworkJoin>{auto& s=f(p);const auto i=s.held.at(fd);assert(pid==8+i&&start==200+i);++s.joins;if(s.fault()||static_cast<int>(i)==s.deny_actor||s.joins==s.deny_join)return {{},Support::permission_required,"join denied"};if(static_cast<int>(i)==s.throw_actor)throw std::runtime_error("join");auto j=s.identities[i];if(s.joins==s.drift_join)++j.local.process.local_real_uid;return {j,s.join_quality,{}};}};}
    void tick(){time+=2'000'000'000;clocks=0;}
    void bucket(unsigned actor,unsigned slot,std::uint64_t cookie,std::uint64_t bytes){const auto base=11+actor*private_network_shared_stride;const auto k=base+9+slot*6;values[k]=cookie;for(unsigned lane=0;lane<4;++lane)values[k+1+lane]=bytes;
        ++values[base+8];++values[8];}
    void traffic(unsigned actor,std::uint64_t delta=40,unsigned slot=0){assert(slot<16);const auto tgid=identities.at(actor).kernel.kernel_tgid;
        for(const auto& [table,arr]:table_arrays)if(live[table]){auto& h=hash_values(table);const auto at=h.find(tgid);if(at==h.end())continue;
            auto& v=array_values(arr);const auto base=11+(at->second-1)*private_network_shared_stride;v[base+9+6*slot]=100+actor+64*slot;
            for(unsigned lane=0;lane<4;++lane)v[base+10+6*slot+lane]+=delta;++v[base+8];++v[8];}}
    unsigned descriptors(){unsigned n=0;for(bool b:live)if(b)++n;return n;}
};
std::vector<PrivateNetworkSelection> rows(unsigned n){std::vector<PrivateNetworkSelection> v;for(unsigned i=0;i<n;++i)v.push_back({100+static_cast<int>(i),8+i,200+i,0});return v;}
PrivateCommonNetworkWindow sample(LinuxSharedSelectedNetworkSampler& s){auto r=s.sample_common();assert(r.value&&r.support==Support::available);assert(publish_linux_process_network(*r.value).value);return std::move(*r.value);}
void no_rates(const PrivateCommonNetworkWindow& w){for(const auto& a:w.accounting.actors){assert(!a.interval_ns.value);for(const auto& r:a.bytes_per_second)assert(!r.value);}}
void success(){Fake f;LinuxSharedSelectedNetworkSampler s(f.api(),f.clock(),f.source());const auto first=s.start(true,rows(64));assert(first.value&&f.held.size()==64&&f.loads==3&&f.links==3&&f.joins==128);assert(!s.start(true,rows(64)).value);
    auto empty=sample(s);assert(empty.observed.size()==64&&empty.accounting.actors.empty()&&empty.accounting.users.empty()&&f.joins==256);
    f.tick();for(unsigned i=0;i<64;++i)for(unsigned slot=0;slot<16;++slot)f.bucket(i,slot,100+slot,100);auto warm=sample(s);no_rates(warm);assert(warm.accounting.actors.size()==1024);
    f.tick();for(unsigned i=0;i<64;++i)for(unsigned slot=0;slot<16;++slot)f.bucket(i,slot,100+slot,140);auto measured=sample(s);
    assert(measured.accounting.users.size()==2&&measured.previous_read_bracket&&measured.accounting.scope.producer_epoch==first.value);
    for(const auto& a:measured.accounting.actors)for(const auto& rate:a.bytes_per_second)assert(rate.value==20&&rate.support==Support::estimated);
    for(const auto& u:measured.accounting.users)for(unsigned lane=0;lane<4;++lane)assert(u.known[lane]==512&&u.missing[lane]==0&&u.bytes_per_second[lane].value==10240);
    const auto joins=f.joins;f.tick();f.deny_join=joins+128;assert(!s.sample_common().value&&f.joins==joins+128); // final AFTER join rejects all
    f.deny_join=0;f.tick();no_rates(sample(s));f.tick();const auto idle=sample(s);for(const auto& r:idle.accounting.actors[0].bytes_per_second)assert(r.value==0);
    s.discard_window();f.tick();no_rates(sample(s));s.stop();f.clean();assert(s.start(true,rows(2)).value.value()!=first.value.value());auto restarted=sample(s);assert(restarted.observed.size()==2&&restarted.accounting.actors.empty());s.stop();f.clean();}
void failures(){
    {Fake f;auto source=f.source();source.join_layout=[](void* p,int fd,std::uint32_t pid,std::uint64_t start,const LinuxNetworkBtfLayout& l){assert(l.sock_protocol_width==1&&l.socket_namespace.value);return Fake::f(p).source().join(p,fd,pid,start);};
        source.join=[](void*,int,std::uint32_t,std::uint64_t)->Metric<PrivatePidfdNetworkJoin>{assert(false);return {};};
        LinuxSharedSelectedNetworkSampler s(f.api(),f.clock(),source);assert(s.start(true,rows(2)).value);assert(sample(s).observed.size()==2&&f.joins==8);s.stop();f.clean();}
    {Fake f;LinuxSharedSelectedNetworkSampler s(f.api(),f.clock(),f.source());assert(!s.start(false,rows(2)).value&&f.calls==0);assert(!s.start(true,{}).value);assert(!s.start(true,rows(LinuxProcessNetworkSampler::max_processes+1)).value);auto bad=rows(2);bad[1]=bad[0];assert(!s.start(true,bad).value);bad=rows(2);bad[1].socket_cookie=1;assert(!s.start(true,bad).value&&f.calls==0);f.clean();}
    int operations=0;{Fake f;{LinuxSharedSelectedNetworkSampler s(f.api(),f.clock(),f.source());assert(s.start(true,rows(2)).value);operations=f.calls;}f.clean();}
    for(bool throwing:{false,true})for(int at=1;at<=operations;++at){Fake f;{LinuxSharedSelectedNetworkSampler s(f.api(),f.clock(),f.source());if(throwing)f.throw_at=at;else f.fail=at;bool caught=false;try{assert(!s.start(true,rows(2)).value);}catch(const std::runtime_error&){caught=true;}assert(caught==throwing);}f.clean();}
    for(unsigned fault=0;fault<9;++fault){Fake f;{
        if(fault==0)f.join_quality=Support::estimated;if(fault==1)f.identities[1].kernel=f.identities[0].kernel;if(fault==2)++f.identities[1].local.boot[0];
        if(fault==3)++f.identities[1].local.observer.user_namespace_inode;if(fault==4)f.identities[1].kernel.leader_start_boottime_ns=0;
        if(fault==5)++f.identities[1].local.process.start_ticks;if(fault==6)f.drift_join=4;if(fault==7)f.identities[1].local.observer.local_pid=9;if(fault==8)f.deny_actor=1;
        LinuxSharedSelectedNetworkSampler s(f.api(),f.clock(),f.source());assert(!s.start(true,rows(2)).value);
    }f.clean();}
    for(unsigned fault=0;fault<12;++fault){Fake f;LinuxSharedSelectedNetworkSampler s(f.api(),f.clock(),f.source());assert(s.start(true,rows(2)).value);f.bucket(0,0,10,100);f.bucket(1,0,11,100);no_rates(sample(s));f.tick();
        if(fault==0)f.deny_actor=1;if(fault==1)f.throw_actor=1;if(fault==2)f.drift_join=f.joins+4;if(fault==3)++f.identities[1].kernel.kernel_real_uid;
        if(fault==4)f.join_quality=Support::estimated;if(fault==5)f.span=1'000'000'001;if(fault==6)f.quality=Support::estimated;if(fault==7)f.values[4]=1;
        if(fault==8)++f.identities[1].local.process.user_namespace_inode;if(fault==9)++f.identities[1].local.boot[0];if(fault==10)f.process=8;if(fault==11)f.deny_join=f.joins+4;
        bool caught=false;try{assert(!s.sample_common().value);}catch(const std::runtime_error&){caught=true;}assert(caught==(fault==1));s.stop();f.clean();}
}
void updates(){const auto r=rows(3);
    {Fake f;LinuxSharedSelectedNetworkSampler s(f.api(),f.clock(),f.source());auto epoch=s.start(true,std::span(r).first(2));assert(epoch.value);
        f.traffic(0,100);f.traffic(1,100);no_rates(sample(s));f.tick();f.traffic(0);f.traffic(1);sample(s);
        const auto calls=f.calls;assert(!s.update_selection(false,r).value&&f.calls==calls);auto bad=r;bad[2]=bad[0];assert(!s.update_selection(true,bad).value&&f.calls==calls);
        assert(!s.update_selection(true,{}).value&&f.calls==calls);
        const std::array replace{r[1],r[2]};auto next=s.update_selection(true,replace);assert(next.value&&next.value!=epoch.value&&f.held.size()==2&&f.descriptors()==16&&f.holds==3);
        f.tick();auto w=sample(s);no_rates(w);assert(!w.previous_read_bracket&&w.observed.size()==2&&w.observed[0].identity.kernel_tgid==124&&w.observed[0].buckets[0].bytes[0]==140&&w.observed[1].buckets.empty());
        f.traffic(0,1000);f.traffic(1);f.traffic(2); // actor0 absent: no routing entry
        const auto readmitted=s.update_selection(true,r);assert(readmitted.value&&readmitted.value!=next.value&&f.descriptors()==24&&f.held.size()==3);
        f.tick();w=sample(s);no_rates(w);assert(w.observed[0].buckets.empty()&&w.observed[1].buckets[0].bytes[0]==180&&w.observed[2].buckets[0].bytes[0]==40);
        f.tick();for(unsigned i=0;i<3;++i)f.traffic(i);sample(s);f.tick();for(unsigned i=0;i<3;++i)f.traffic(i);w=sample(s);
        for(const auto& a:w.accounting.actors)for(const auto& rate:a.bytes_per_second)assert(rate.value==20&&rate.support==Support::estimated);
        assert(w.observed[0].buckets[0].bytes[0]==80&&w.observed[1].buckets[0].bytes[0]==260&&w.observed[2].buckets[0].bytes[0]==120);
        const auto loads=f.loads,holds=f.holds;auto reordered=r;std::reverse(reordered.begin(),reordered.end());assert(s.update_selection(true,reordered).value);
        assert(f.loads==loads&&f.holds==holds);f.tick();w=sample(s);no_rates(w);assert(w.observed.front().identity.kernel_tgid==125);
        assert(s.update_selection(true,std::span(r).last(1)).value&&f.descriptors()==8&&f.held.size()==1);f.tick();w=sample(s);assert(w.observed[0].buckets[0].bytes[0]==120);
        s.stop();f.clean();}
    // No lifetime admission limit: empty batches close, and re-admission gets a
    // fresh batch without disturbing the single retained actor.
    {Fake f;LinuxSharedSelectedNetworkSampler s(f.api(),f.clock(),f.source());assert(s.start(true,std::span(r).first(1)).value);
        for(unsigned i=0;i<80;++i){const std::array choice{r[0],r[1+i%2]};assert(s.update_selection(true,choice).value);
            assert(f.descriptors()==16&&f.held.size()==2);f.traffic(0);f.traffic(1);f.traffic(2);f.tick();const auto w=sample(s);
            assert(w.observed[0].buckets[0].bytes[0]==40*(i+1)&&w.observed[1].buckets[0].bytes[0]==40);no_rates(w);}
        s.stop();f.clean();}
    int operations=0;{Fake f;LinuxSharedSelectedNetworkSampler s(f.api(),f.clock(),f.source());assert(s.start(true,std::span(r).first(2)).value);const auto base=f.calls;
        const std::array choice{r[1],r[2]};assert(s.update_selection(true,choice).value);operations=f.calls-base;s.stop();f.clean();}
    for(bool throwing:{false,true})for(int at=1;at<=operations;++at){Fake f;LinuxSharedSelectedNetworkSampler s(f.api(),f.clock(),f.source());assert(s.start(true,std::span(r).first(2)).value);
        if(throwing)f.throw_at=f.calls+at;else f.fail=f.calls+at;bool caught=false;const std::array choice{r[1],r[2]};
        try{assert(!s.update_selection(true,choice).value);}catch(const std::runtime_error&){caught=true;}assert(caught==throwing);assert(!s.sample_common().value);f.clean();}
    for(unsigned fault=0;fault<7;++fault){Fake f;LinuxSharedSelectedNetworkSampler s(f.api(),f.clock(),f.source());assert(s.start(true,std::span(r).first(2)).value);
        if(fault==0)++f.identities[1].kernel.kernel_real_uid;if(fault==1)f.identities[2].kernel=f.identities[1].kernel;
        if(fault==2)++f.identities[2].local.boot[0];if(fault==3)f.drift_join=f.joins+4;if(fault==4)f.process=8;
        if(fault==5)f.join_quality=Support::estimated;if(fault==6)++f.identities[2].local.observer.user_namespace_inode;
        const std::array choice{r[1],r[2]};assert(!s.update_selection(true,choice).value);f.clean();}
    // Quality loss in any batch rejects the ENTIRE window/reset ALL baselines.
    {Fake f;LinuxSharedSelectedNetworkSampler s(f.api(),f.clock(),f.source());assert(s.start(true,std::span(r).first(1)).value);assert(s.update_selection(true,r).value);
        for(unsigned i=0;i<3;++i)f.traffic(i);sample(s);f.tick();f.values[4]=1;assert(!s.sample_common().value);f.values[4]=0;f.tick();no_rates(sample(s));s.stop();f.clean();}
    // Fragmentation exercise:64 live admission batches,1024 buckets. The
    // bound is live actors, not lifetime changes; no truncation at the limit.
    {Fake f;LinuxSharedSelectedNetworkSampler s(f.api(),f.clock(),f.source());const auto all=rows(64);assert(s.start(true,std::span(all).first(1)).value);
        for(unsigned n=2;n<=64;++n)assert(s.update_selection(true,std::span(all).first(n)).value);
        assert(f.descriptors()==64*8&&f.held.size()==64&&f.links==64*3);
        const auto calls=f.calls;assert(!s.update_selection(true,rows(LinuxProcessNetworkSampler::max_processes+1)).value&&f.calls==calls);
        for(unsigned i=0;i<64;++i)for(unsigned slot=0;slot<16;++slot)f.traffic(i,40,slot);auto w=sample(s);no_rates(w);assert(w.accounting.actors.size()==1024);
        f.tick();for(unsigned i=0;i<64;++i)for(unsigned slot=0;slot<16;++slot)f.traffic(i,40,slot);w=sample(s);
        for(const auto& a:w.accounting.actors)for(const auto& rate:a.bytes_per_second)assert(rate.value==20);
        assert(s.update_selection(true,std::span(all).last(1)).value&&f.descriptors()==8&&f.held.size()==1);f.tick();w=sample(s);no_rates(w);assert(w.accounting.actors.size()==16&&w.observed[0].buckets[0].bytes[0]==80);
        s.stop();f.clean();}
    {Fake f;LinuxSharedSelectedNetworkSampler s(f.api(),f.clock(),f.source());assert(s.start(true,std::span(r).first(1)).value);assert(s.update_selection(true,r).value);
        f.span=400'000'000;assert(!s.sample_common().value); // each batch<1s, TOTAL sweep>1s
        f.span=10;f.tick();sample(s);s.stop();f.clean();}
}
}
void scale(){
    for(unsigned n:{65u,129u,static_cast<unsigned>(LinuxProcessNetworkSampler::max_processes)}){
        Fake f;LinuxSharedSelectedNetworkSampler s(f.api(),f.clock(),f.source());auto all=rows(n);
        auto epoch=s.start(true,all);const auto batches=(n+63)/64;
        assert(epoch.value&&f.held.size()==n&&f.links==3*batches&&f.descriptors()==8*batches);
        auto w=sample(s);assert(w.observed.size()==n&&w.accounting.actors.empty());
        f.tick();for(unsigned i=0;i<n;++i)for(unsigned slot=0;slot<16;++slot)f.traffic(i,40,slot);
        w=sample(s);no_rates(w);assert(w.accounting.actors.size()==16*n);
        f.tick();for(unsigned i=0;i<n;++i)for(unsigned slot=0;slot<16;++slot)f.traffic(i,40,slot);
        w=sample(s);for(const auto& a:w.accounting.actors)for(const auto& rate:a.bytes_per_second)assert(rate.value==20);
        auto removed=s.update_selection(true,std::span(all).subspan(1));assert(removed.value&&removed.value!=epoch.value);
        w=sample(s);no_rates(w);assert(w.observed.size()==n-1&&w.accounting.actors.size()==16*(n-1));
        f.tick();for(unsigned i=0;i<n;++i)f.traffic(i,40);
        assert(s.update_selection(true,all).value);w=sample(s);no_rates(w);
        assert(w.observed.size()==n&&w.accounting.actors.size()==16*(n-1)&&w.observed[0].buckets.empty());
        f.tick();for(unsigned i=0;i<n;++i)f.traffic(i,40);
        w=sample(s);assert(w.observed[0].buckets[0].bytes[0]==40&&w.observed[1].buckets[0].bytes[0]==160);
        const auto calls=f.calls;assert(!s.update_selection(true,rows(LinuxProcessNetworkSampler::max_processes+1)).value&&f.calls==calls);
        s.stop();f.clean();
    }
    // A later batch failing must close earlier batches and every held pidfd.
    for(unsigned batch:{2u,16u})for(bool update:{false,true})for(bool throwing:{false,true}){
        Fake f;LinuxSharedSelectedNetworkSampler s(f.api(),f.clock(),f.source());auto all=rows(LinuxProcessNetworkSampler::max_processes);
        if(update)assert(s.start(true,std::span(all).first(1)).value);
        if(throwing)f.throw_batch=f.batches+batch;else f.fail_batch=f.batches+batch;
        bool threw=false;try{auto r=update?s.update_selection(true,all):s.start(true,all);assert(!r.value);}catch(const std::runtime_error&){threw=true;}
        assert(threw==throwing);f.clean();assert(!s.sample_common().value);
    }
}
int main(){if(std::endian::native!=std::endian::little)return 0;success();failures();updates();scale();}
