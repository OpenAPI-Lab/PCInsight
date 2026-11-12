#include "../src/linux_network_multi.hpp"
#include <bit>
#include <cassert>
#include <stdexcept>
using namespace pcinsight;using namespace pcinsight::detail;
namespace {
struct Fake {
    std::array<bool,2048> live{};std::array<unsigned,2048> entries{},owner{};
    std::array<unsigned,64> held{};std::array<int,64> joins{},clocks{};
    std::array<std::array<std::uint64_t,private_network_discovery_entries>,64> values{};
    std::array<std::uint64_t,64> time{1'000'000'000,1'000'005'000};
    std::array<PrivatePidfdNetworkJoin,64> identity{{
        {{999,2000,800},{{1,2},{7,42,100,1,2,3,4,5,6,7},{8,43,200,1,2,8,4,9,6,10}}},
        {{1000,2000,801},{{1,2},{7,42,100,1,2,3,4,5,6,7},{9,43,201,1,2,9,4,9,6,10}}}}};
    Fake(){for(unsigned i=2;i<64;++i){identity[i]=identity[0];identity[i].kernel={999+i,2000,800+i};identity[i].local.process.local_pid=8+i;
        identity[i].local.process.start_ticks=200+i;identity[i].local.process.process_inode=8+i;time[i]=1'000'005'000;}}
    int next=128,calls=0,current=0,deny=-1,throw_at=-1,bad_capture=-1;
    bool throwing=false;std::uint64_t process=7;
    std::uint64_t span=10;Support clock_support=Support::available;bool throw_clock=false;
    int drift_fd=-1,drift_at=-1;
    int throw_capture=-1;Support capture_support=Support::available;
    bool bucket_maps=false;unsigned next_bucket=0;
    static Fake& f(void* p){return *static_cast<Fake*>(p);}
    int fd(unsigned count=0){assert(next<2048);live[next]=true;entries[next]=count;owner[next]=static_cast<unsigned>(current);return next++;}
    void clean(){for(auto x:held)assert(!x);for(bool x:live)assert(!x);}
    NetworkSessionApi capture(){return {this,
        [](void* p)->Metric<LinuxNetworkBtfLayout>{++f(p).calls;LinuxNetworkBtfLayout l;l.inet_send_function_id=10;l.inet6_send_function_id=11;l.receive_function_id=12;l.sock_protocol_width=1;
            l.socket_namespace={LinuxSocketNamespaceBtfLayout{64,128},Support::available,{}};return {l,Support::available,{}};},
        [](void* p,unsigned n)->NetworkSessionFd{auto& s=f(p);++s.calls;if(n==9||n==private_network_discovery_entries){if(s.bucket_maps){assert(s.next_bucket<2);s.current=static_cast<int>(s.next_bucket++);}s.values[s.current]={};}return {s.fd(n),0};},
        [](void* p,std::span<const NetworkProbeInstruction>,std::uint32_t,bool)->NetworkSessionFd{auto& s=f(p);++s.calls;return {s.fd(),0};},
        [](void*,int)->NetworkSessionRun{return {};},
        [](void* p,int fd,std::uint32_t key)->Metric<std::uint64_t>{auto& s=f(p);++s.calls;assert(s.live[fd]);
            if(s.entries[fd]==3){const std::array<std::uint64_t,3> producer{123,1000,900};return {producer.at(key),Support::available,{}};}
            if(static_cast<int>(s.owner[fd])==s.bad_capture)return {{},Support::permission_required,"capture denied"};
            if(static_cast<int>(s.owner[fd])==s.throw_capture){s.throw_capture=-1;throw std::runtime_error("capture");}
            return {s.values[s.owner[fd]].at(key),s.capture_support,{}};},
        [](void* p,int)->NetworkSessionFd{auto& s=f(p);++s.calls;return {s.fd(),0};},
        [](void* p,int fd) noexcept {auto& s=f(p);assert(s.live[fd]);s.live[fd]=false;}};}
    NetworkSamplerApi clock(){return {this,nullptr,
        [](void* p)->Metric<std::uint64_t>{auto& s=f(p);if(s.throw_clock){s.throw_clock=false;throw std::runtime_error("clock");}
            const int i=s.current;return {s.time[i]+((s.clocks[i]++%2)?s.span:0),s.clock_support,{}};},
        [](void* p) noexcept ->std::uint64_t{return f(p).process;},nullptr};}
    SelectedNetworkSourceApi target(){return {this,
        [](void* p,int fd)->NetworkSessionFd{auto& s=f(p);++s.calls;const auto i=static_cast<unsigned>(fd-55);assert(i<64);++s.held[i];return {static_cast<int>(i),0};},
        [](void* p,int fd) noexcept {auto& s=f(p);assert(fd>=0&&fd<64&&s.held[fd]);--s.held[fd];},
        [](void* p,int fd,std::uint32_t pid,std::uint64_t start)->Metric<PrivatePidfdNetworkJoin>{auto& s=f(p);assert(fd>=0&&fd<64&&s.held[fd]&&pid==8U+fd&&start==200U+fd);
            s.current=fd;++s.calls;++s.joins[fd];
            if(s.throwing&&fd==s.throw_at){s.throwing=false;throw std::runtime_error("join");}
            if(fd==s.deny)return {{},Support::permission_required,"join denied"};auto result=s.identity[fd];
            if(fd==s.drift_fd&&s.joins[fd]==s.drift_at)++result.local.process.user_namespace_inode;
            return {result,Support::available,{}};}};}
    void tick(){time[0]+=2'000'000'000;for(unsigned i=1;i<64;++i)time[i]+=3'000'000'000;clocks={};}
};
const std::array<PrivateNetworkSelection,2> selection{{{55,8,200,12},{56,9,201,12}}};
PrivateMultiNetworkWindow sample(LinuxMultiNetworkSampler& s){auto r=s.sample();assert(r.value);return *r.value;}
void baseline(const PrivateMultiNetworkWindow& w){for(const auto& a:w.actors)assert(!a.window.accounting.actors[0].interval_ns.value);}
PrivateCommonNetworkWindow common(LinuxMultiNetworkSampler& s){auto r=s.sample_common();assert(r.value);return *r.value;}
void baseline(const PrivateCommonNetworkWindow& w){assert(!w.previous_read_bracket);for(const auto& a:w.accounting.actors)assert(!a.interval_ns.value);}
const std::array<PrivateNetworkSelection,2> observed_selection{{{55,8,200,0},{56,9,201,0}}};
void bucket(Fake& f,unsigned actor,unsigned slot,std::uint64_t cookie,std::uint64_t bytes){const auto base=9+slot*6;
    f.values[actor][base]=cookie;for(unsigned lane=1;lane<5;++lane)f.values[actor][base+lane]=bytes*(lane);f.values[actor][base+5]=1;++f.values[actor][8];}
void test_capacity(){
    std::vector<PrivateNetworkSelection> rows;for(unsigned i=0;i<64;++i)rows.push_back({55+static_cast<int>(i),8+i,200+i,0});
    {Fake f;LinuxMultiNetworkSampler s(f.capture(),f.clock(),f.target(),false,true);assert(s.start(true,rows).value);
        auto empty=common(s);assert(empty.observed.size()==64&&empty.accounting.actors.empty());
        for(unsigned i=0;i<64;++i)bucket(f,i,0,12,10+i);f.tick();auto first=common(s);assert(first.accounting.actors.size()==64);for(const auto& a:first.accounting.actors)assert(!a.interval_ns.value);
        for(unsigned i=0;i<64;++i)bucket(f,i,0,12,2*(10+i));f.tick();auto w=common(s);
        for(unsigned lane=0;lane<4;++lane){std::uint64_t expected=0;for(unsigned i=0;i<64;++i){const auto rate=(10+i)*(lane+1)/3;
                assert(w.accounting.actors[i].bytes_per_second[lane].value==rate);expected+=rate;}
            assert(w.accounting.users[0].known[lane]==64&&w.accounting.users[0].missing[lane]==0&&w.accounting.users[0].bytes_per_second[lane].value==expected);}
        f.bad_capture=63;f.tick();assert(!s.sample_common().value);f.bad_capture=-1;f.tick();baseline(common(s));
        auto reversed=rows;std::reverse(reversed.begin(),reversed.end());const auto allocated=f.next;
        assert(s.update_selection(true,reversed).value&&f.next==allocated);w=common(s);baseline(w);assert(w.observed.front().identity.kernel_tgid==1062);
        assert(s.update_selection(true,std::span(rows).first(63)).value&&f.held[63]==0);baseline(common(s));
        assert(s.update_selection(true,rows).value);w=common(s);baseline(w);assert(w.observed.size()==64&&w.observed.back().buckets.empty()&&w.accounting.actors.size()==63);
        s.stop();f.clean();}
    for(bool throwing:{false,true}){Fake f;LinuxMultiNetworkSampler s(f.capture(),f.clock(),f.target(),false,true);
        if(throwing){f.throwing=true;f.throw_at=63;}else f.deny=63;bool threw=false;
        try{assert(!s.start(true,rows).value);}catch(const std::runtime_error&){threw=true;}assert(threw==throwing);f.clean();}
}
void test_observed(){
    {Fake f;LinuxMultiNetworkSampler s(f.capture(),f.clock(),f.target(),false,true);
        assert(!s.start(false,observed_selection).value&&!s.start(true,selection).value&&!s.start(true,{}).value);
        auto bad=observed_selection;bad[1]=bad[0];assert(!s.start(true,bad).value);
        assert(!s.sample().value&&!s.sample_common().value&&f.calls==0);f.clean();
        LinuxMultiNetworkSampler conflict(f.capture(),f.clock(),f.target(),true,true);assert(!conflict.start(true,observed_selection).value&&f.calls==0);}
    {Fake f;LinuxMultiNetworkSampler s(f.capture(),f.clock(),f.target(),false,true);
        const auto start=s.start(true,observed_selection);assert(start.value&&!s.sample().value);
        auto w=common(s);baseline(w);assert(w.associations.empty()&&w.observed.size()==2&&w.accounting.actors.empty()&&w.accounting.users.empty());
        assert(w.observed[0].local.process.local_pid==8&&w.observed[1].local.process.local_pid==9&&w.accounting.scope.coverage==NetworkAccountingCoverage::selected_observed_actor_set);
        f.tick();bucket(f,0,0,12,300);bucket(f,1,0,12,600);w=common(s);assert(w.accounting.actors.size()==2);
        for(const auto& a:w.accounting.actors)assert(!a.interval_ns.value); // Empty registry is not a zero baseline.
        f.tick();bucket(f,0,0,12,600);bucket(f,1,0,12,1200);bucket(f,1,1,13,90);w=common(s);
        assert(w.accounting.actors.size()==3&&w.observed[1].buckets.size()==2);
        for(unsigned lane=0;lane<4;++lane){assert(w.accounting.actors[0].bytes_per_second[lane].value==100*(lane+1));
            assert(w.accounting.actors[1].bytes_per_second[lane].value==200*(lane+1)&&!w.accounting.actors[2].bytes_per_second[lane].value);
            assert(w.accounting.users[0].known[lane]==2&&w.accounting.users[0].missing[lane]==1&&w.accounting.users[0].bytes_per_second[lane].value==300*(lane+1));}
        f.tick();w=common(s);for(unsigned lane=0;lane<4;++lane)assert(w.accounting.users[0].known[lane]==3&&w.accounting.users[0].bytes_per_second[lane].value==0);
        const auto calls=f.calls;assert(s.update_selection(false,observed_selection).support==Support::permission_required);
        assert(!s.update_selection(true,selection).value&&f.calls==calls);
        const auto allocated=f.next;auto reversed=observed_selection;std::swap(reversed[0],reversed[1]);
        const auto updated=s.update_selection(true,reversed);assert(updated.value&&updated.value!=start.value&&f.next==allocated);
        w=common(s);baseline(w);assert(w.observed[0].identity.kernel_tgid==1000&&w.observed[0].buckets.size()==2);
        assert(s.update_selection(true,std::span(observed_selection).last(1)).value&&!f.held[0]&&f.held[1]);baseline(common(s));
        assert(s.update_selection(true,observed_selection).value);w=common(s);baseline(w);
        assert(w.observed[0].buckets.empty()&&w.observed[1].buckets.size()==2&&w.accounting.actors.size()==2); // Re-added actor excludes gap.
        const auto epoch=w.accounting.scope.producer_epoch;s.stop();f.clean();assert(s.start(true,observed_selection).value);w=common(s);
        assert(w.accounting.actors.empty()&&w.accounting.scope.producer_epoch!=epoch);f.process=9;const auto before=f.calls;
        assert(!s.sample_common().value&&f.calls==before);f.clean();}
    for(unsigned fault=0;fault<20;++fault){Fake f;{
        LinuxMultiNetworkSampler s(f.capture(),f.clock(),f.target(),false,true);assert(s.start(true,observed_selection).value);
        bucket(f,0,0,12,300);bucket(f,1,0,12,600);baseline(common(s));f.tick();
        if(fault==0)f.deny=1;if(fault==1){f.throw_at=1;f.throwing=true;}if(fault==2)f.bad_capture=1;
        if(fault==3){f.drift_fd=1;f.drift_at=f.joins[1]+2;}if(fault==4)f.clock_support=Support::estimated;
        if(fault==5)f.span=1'000'000'001;if(fault==6)f.time[1]=0;if(fault==7)f.time[1]=1;if(fault==8)f.throw_clock=true;
        if(fault==9)f.values[1][4]=1;if(fault==10)++f.identity[1].local.process.user_namespace_inode;
        if(fault==11)++f.identity[1].kernel.kernel_real_uid;if(fault==12)f.values[1][9]=13;
        if(fault==13)f.values[1][10]=599;if(fault==14)f.values[1][14]=0;if(fault==15)f.values[1]={};
        if(fault==16)f.values[1][7]=1;if(fault==17)f.values[1][6]=1;if(fault==18)f.throw_capture=1;
        if(fault==19)f.capture_support=Support::estimated;
        bool thrown=false;try{assert(!s.sample_common().value);}catch(const std::runtime_error&){thrown=true;}assert(thrown==(fault==1||fault==8||fault==18));
        f.deny=f.bad_capture=f.drift_fd=-1;f.clock_support=f.capture_support=Support::available;f.span=10;f.time={10'000'000'000,10'000'005'000};f.clocks={};
        f.values[1]={};bucket(f,1,0,12,600);f.identity[1].kernel.kernel_real_uid=2000;
        auto w=common(s);baseline(w);assert(w.accounting.actors.size()==2);f.tick();w=common(s);
        for(const auto& a:w.accounting.actors)assert(a.bytes_per_second[0].value==0); // No first-actor hidden commit/old interval.
    }f.clean();}
    for(unsigned fault=0;fault<5;++fault){Fake f;{
        if(fault==0)f.deny=1;if(fault==1){f.throwing=true;f.throw_at=1;}if(fault==2)f.identity[1].kernel=f.identity[0].kernel;
        if(fault==3)++f.identity[1].local.observer.user_namespace_inode;if(fault==4)++f.identity[1].local.boot[0];
        LinuxMultiNetworkSampler s(f.capture(),f.clock(),f.target(),false,true);bool thrown=false;
        try{assert(!s.start(true,observed_selection).value);}catch(const std::runtime_error&){thrown=true;}assert(thrown==(fault==1));f.clean();
    }f.clean();}
    {Fake f;LinuxMultiNetworkSampler s(f.capture(),f.clock(),f.target(),false,true);assert(s.start(true,observed_selection).value);
        f.identity[1].kernel.kernel_real_uid=2001;assert(!s.sample_common().value);s.stop();f.clean(); // Actor UID cannot be relabelled after start.
        assert(s.start(true,observed_selection).value);for(unsigned actor=0;actor<2;++actor)for(unsigned slot=0;slot<16;++slot)bucket(f,actor,slot,100+slot,0);
        auto w=common(s);baseline(w);assert(w.accounting.actors.size()==32&&w.observed.size()==2&&w.accounting.users.size()==2);
        f.tick();w=common(s);for(const auto& u:w.accounting.users)assert(u.known[0]==16&&u.bytes_per_second[0].value==0);
        f.time[1]+=LinuxNetworkAccounting::max_interval_ns+1;f.clocks={};w=common(s);for(const auto& a:w.accounting.actors)assert(!a.interval_ns.value);
        NetworkAccountingInput input{w.accounting.scope,f.time[1]+100,0,true,{}};LinuxNetworkAccounting reducer;
        for(std::uint32_t a=0;a<16;++a)for(std::uint64_t b=1;b<=16;++b)input.rows.push_back({{100+a,2000,800+a,b},{},true,false});
        assert(reducer.sample({input,Support::available,{}}).value); // Full16*16 supported, not a16-row ceiling.
        input.rows.push_back({{100,2000,800,17},{},true,false});assert(!reducer.sample({input,Support::available,{}}).value);
        input.rows.clear();for(std::uint32_t a=0;a<=max_selected_network_actors;++a)input.rows.push_back({{100+a,2000,800+a,1},{},true,false});
        assert(!reducer.sample({input,Support::available,{}}).value);
        input.rows={{{100,2000,800,1},{},true,false},{{100,2001,800,2},{},true,false}};assert(!reducer.sample({input,Support::available,{}}).value);
        s.stop();f.clean();}
    // Runtime addition failure in observed mode must close retained actors too.
    {Fake f;LinuxMultiNetworkSampler s(f.capture(),f.clock(),f.target(),false,true);
        assert(s.start(true,std::span(observed_selection).first(1)).value);baseline(common(s));f.deny=1;
        assert(!s.update_selection(true,observed_selection).value);f.clean();f.deny=-1;
        assert(!s.update_selection(true,observed_selection).value&&!s.sample_common().value);}
}
void test_buckets(){
    const std::array<PrivateNetworkSelection,2> buckets{{selection[0],{55,8,200,13}}};
    {Fake f;f.bucket_maps=true;LinuxMultiNetworkSampler s(f.capture(),f.clock(),f.target(),true);
        auto invalid=buckets;invalid[1].socket_cookie=12;assert(!s.start(true,invalid).value&&f.calls==0);
        invalid=buckets;++invalid[1].start_ticks;assert(!s.start(true,invalid).value&&f.calls==0);
        assert(!s.start(false,buckets).value&&f.calls==0);const auto start=s.start(true,buckets);assert(start.value&&f.held[0]==2);
        auto first=common(s);baseline(first);assert(first.accounting.scope.coverage==NetworkAccountingCoverage::selected_namespace_bucket_set);
        assert(first.associations[0].local==first.associations[1].local);
        f.tick();f.values[0]={100,200,300,400,0,0,0,0,4};f.values[1]={200,400,600,800,0,0,0,0,4};
        auto measured=common(s);assert(measured.accounting.actors.size()==2&&measured.accounting.users[0].observed_buckets==2);
        for(unsigned lane=0;lane<4;++lane){assert(measured.accounting.actors[0].bytes_per_second[lane].value==50*(lane+1));
            assert(measured.accounting.actors[1].bytes_per_second[lane].value==100*(lane+1));assert(measured.accounting.users[0].bytes_per_second[lane].value==150*(lane+1));}
        auto reversed=buckets;std::swap(reversed[0],reversed[1]);const auto allocated=f.next;
        const auto reordered=s.update_selection(true,reversed);assert(reordered.value&&reordered.value!=start.value&&f.next==allocated);
        baseline(common(s));assert(s.update_selection(true,std::span(buckets).first(1)).value&&f.held[0]==1);baseline(common(s));
        f.deny=0;assert(!s.sample_common().value);f.deny=-1;f.tick();baseline(common(s));
        s.stop();f.clean();
    }
    // A kernel identity alias across DIFFERENT local processes is not a new bucket.
    {Fake f;f.identity[1].kernel=f.identity[0].kernel;LinuxMultiNetworkSampler s(f.capture(),f.clock(),f.target(),true);
        auto invalid=selection;invalid[1].socket_cookie=13;assert(!s.start(true,invalid).value);f.clean();}
    // Runtime failure in any bucket invalidates ALL intervals, including siblings.
    {Fake f;f.bucket_maps=true;LinuxMultiNetworkSampler s(f.capture(),f.clock(),f.target(),true);assert(s.start(true,buckets).value);
        baseline(common(s));f.tick();f.bad_capture=1;assert(!s.sample_common().value);f.bad_capture=-1;f.tick();baseline(common(s));s.stop();f.clean();}
}
void test_update(){
    const auto only_a=std::span(selection).first(1),only_b=std::span(selection).last(1);
    {Fake f;LinuxMultiNetworkSampler s(f.capture(),f.clock(),f.target());
        assert(!s.update_selection(true,selection).value&&f.calls==0);
        const auto first=s.start(true,only_a);assert(first.value);baseline(common(s));
        f.tick();f.values[0][0]=42;assert(common(s).accounting.actors[0].bytes_per_second[0].value);
        const auto calls=f.calls;assert(s.update_selection(false,selection).support==Support::permission_required);
        assert(!s.update_selection(true,{}).value);auto bad=selection;bad[1].pid=8;assert(!s.update_selection(true,bad).value);
        bad=selection;bad[1].pidfd=-1;assert(!s.update_selection(true,bad).value);
        bad=selection;bad[1].start_ticks=0;assert(!s.update_selection(true,bad).value);
        bad=selection;bad[1].socket_cookie=0;assert(!s.update_selection(true,bad).value);
        std::vector<PrivateNetworkSelection> many(max_selected_network_actors+1,selection[0]);assert(!s.update_selection(true,many).value);
        assert(f.calls==calls&&f.held[0]&&!f.held[1]);
        const auto added=s.update_selection(true,selection);assert(added.value&&added.value!=first.value&&f.held[0]&&f.held[1]);
        auto w=common(s);baseline(w);assert(w.accounting.scope.producer_epoch==*added.value);
        assert(w.accounting.actors[0].bytes.value->at(0)==42&&w.accounting.actors[1].bytes.value->at(0)==0);
        f.tick();w=common(s);for(const auto& a:w.accounting.actors)assert(a.bytes_per_second[0].value==0);
        const auto fd=f.next;const auto removed=s.update_selection(true,only_b);assert(removed.value&&removed.value!=added.value);
        assert(!f.held[0]&&f.held[1]&&f.next==fd);w=common(s);baseline(w);assert(w.accounting.actors.size()==1&&w.accounting.actors[0].actor.tgid==1000);
        assert(s.update_selection(true,selection).value&&f.held[0]&&f.held[1]);baseline(common(s));
        auto reversed=selection;std::swap(reversed[0],reversed[1]);const auto held_fd=f.next;
        const auto reordered=s.update_selection(true,reversed);assert(reordered.value&&f.next==held_fd);
        w=common(s);baseline(w);assert(w.associations[0].actor.tgid==1000&&w.associations[1].actor.tgid==999); // Reducer rows are key-sorted.
        // Even an identical explicit update changes epoch and discards intervals.
        const auto identical=s.update_selection(true,reversed);assert(identical.value&&identical.value!=reordered.value&&f.next==held_fd);
        baseline(sample(s));f.tick();baseline(common(s));
        // Cookie replacement closes the old duplicate before holding the new one.
        auto changed=selection;changed[0].socket_cookie=13;f.values[0][0]=91;
        assert(s.update_selection(true,changed).value&&f.next>held_fd);
        w=common(s);baseline(w);assert(w.accounting.actors[0].actor.socket_network_namespace_cookie==13&&w.accounting.actors[0].bytes.value->at(0)==0);
        s.stop();f.clean();assert(!s.update_selection(true,selection).value);
    }
    // Add/revalidate failures close retained, added and removed resources; no old selection survives.
    for(unsigned fault=0;fault<7;++fault){Fake f;{LinuxMultiNetworkSampler s(f.capture(),f.clock(),f.target());
        assert(s.start(true,only_a).value);baseline(common(s));
        if(fault==0)f.deny=1;if(fault==1){f.throw_at=1;f.throwing=true;}if(fault==2)f.deny=0;
        if(fault==3){f.identity[1].kernel.kernel_tgid=999;f.identity[1].kernel.leader_start_boottime_ns=800;}
        if(fault==4)++f.identity[1].local.boot[0];if(fault==5)++f.identity[1].local.observer.user_namespace_inode;
        if(fault==6)f.process=10;
        bool thrown=false;try{assert(!s.update_selection(true,selection).value);}catch(const std::runtime_error&){thrown=true;}
        assert(thrown==(fault==1));f.clean();assert(!s.sample_common().value);
        f.deny=-1;f.process=7; // An update cannot silently restart after a failed one.
        assert(!s.update_selection(true,only_a).value);assert(s.start(true,only_a).value);baseline(common(s));
    }f.clean();}
    // A retired, inaccessible process must not prevent removing it.
    {Fake f;LinuxMultiNetworkSampler s(f.capture(),f.clock(),f.target());assert(s.start(true,selection).value);
        f.deny=0;assert(s.update_selection(true,only_b).value);baseline(common(s));s.stop();f.clean();}
}
void test_common(){
    // Common sweep: same nominal interval for all rows, grouped kernel UID sum.
    {Fake f;LinuxMultiNetworkSampler s(f.capture(),f.clock(),f.target());assert(s.start(true,selection).value);
        auto first=common(s);baseline(first);assert(first.associations.size()==2&&first.accounting.scope.coverage==NetworkAccountingCoverage::selected_actor_set);
        f.tick();f.values[0]={300,600,900,1200,0,0,0,0,4};f.values[1]={600,1200,1800,2400,0,0,0,0,4};
        auto next=common(s);const std::array expected_bracket{first.read_started_ns,first.read_finished_ns};assert(next.previous_read_bracket==expected_bracket);
        assert(next.read_finished_ns-next.read_started_ns==10);
        for(unsigned i=0;i<2;++i){const auto& a=next.accounting.actors[i];assert(a.interval_ns.value==3'000'000'000ULL);
            for(unsigned lane=0;lane<4;++lane)assert(a.bytes_per_second[lane].value==(i+1)*100*(lane+1));}
        assert(next.accounting.users.size()==1&&next.accounting.users[0].real_uid==2000);
        for(unsigned lane=0;lane<4;++lane){const auto& u=next.accounting.users[0];assert(u.bytes_per_second[lane].value==300*(lane+1));assert(u.known[lane]==2&&u.missing[lane]==0);}
        f.tick();baseline(sample(s));f.tick();baseline(common(s)); // No baseline bridge between modes.
        f.tick();next=common(s);assert(next.accounting.actors[0].bytes_per_second[0].value==0);
        const auto calls=f.calls;f.process=9;assert(!s.sample_common().value&&f.calls==calls);f.clean();
    }
    for(unsigned fault=0;fault<12;++fault){Fake f;{LinuxMultiNetworkSampler s(f.capture(),f.clock(),f.target());assert(s.start(true,selection).value);baseline(common(s));f.tick();
        if(fault==0)f.deny=1;if(fault==1){f.throw_at=1;f.throwing=true;}if(fault==2)f.bad_capture=1;
        if(fault==3){f.drift_fd=1;f.drift_at=f.joins[1]+2;} // AFTER reading both counters.
        if(fault==4)f.clock_support=Support::estimated;if(fault==5)f.span=1'000'000'001;
        if(fault==6)f.time[1]=0;if(fault==7)f.time[1]=1;if(fault==8)f.throw_clock=true;
        if(fault==9)f.values[1][4]=1;if(fault==10)++f.identity[1].local.process.user_namespace_inode;
        if(fault==11)++f.identity[1].kernel.kernel_real_uid;
        bool thrown=false;try{assert(!s.sample_common().value);}catch(const std::runtime_error&){thrown=true;}assert(thrown==(fault==1||fault==8));
        f.deny=f.bad_capture=f.drift_fd=-1;f.clock_support=Support::available;f.span=10;f.time={10'000'000'000,10'000'005'000};f.clocks={};f.values={};f.identity[1].kernel.kernel_real_uid=2000;
        baseline(common(s));f.tick();const auto recovered=common(s);for(const auto& a:recovered.accounting.actors)assert(a.bytes_per_second[0].value==0);
    }f.clean();}
    {Fake f;f.identity[1].kernel.kernel_real_uid=3000;LinuxMultiNetworkSampler s(f.capture(),f.clock(),f.target());assert(s.start(true,selection).value);
        baseline(common(s));f.tick();auto w=common(s);assert(w.accounting.users.size()==2);
        for(const auto& u:w.accounting.users)assert(u.known[0]==1&&u.bytes_per_second[0].value==0);
        f.time[1]+=LinuxNetworkAccounting::max_interval_ns+1;f.clocks={};w=common(s);for(const auto& a:w.accounting.actors)assert(!a.interval_ns.value);
    }
    {Fake f;LinuxMultiNetworkSampler s(f.capture(),f.clock(),f.target());assert(s.start(true,selection).value);baseline(common(s));
        f.time[1]+=1'000'000'000;f.clocks={};for(auto& values:f.values)for(unsigned lane=0;lane<4;++lane)values[lane]=UINT64_MAX;
        const auto w=common(s);for(unsigned lane=0;lane<4;++lane){const auto& u=w.accounting.users[0];
            assert(u.overflow[lane]&&!u.bytes_per_second[lane].value&&u.known[lane]==2&&u.missing[lane]==0);}
    }
    {Fake f;f.span=LinuxPrivateNetworkSampler::max_read_span_ns;LinuxMultiNetworkSampler s(f.capture(),f.clock(),f.target());assert(s.start(true,selection).value);
        baseline(common(s));f.tick();assert(common(s).accounting.actors[0].interval_ns.value==3'000'000'000ULL);}
}
}
int main(){
    {Fake f;LinuxMultiNetworkSampler s(f.capture(),f.clock(),f.target());assert(!s.start(false,selection).value);assert(!s.start(true,{}).value);
        auto bad=selection;bad[1].pid=8;assert(!s.start(true,bad).value);bad=selection;bad[1].socket_cookie=0;assert(!s.start(true,bad).value);
        std::vector<PrivateNetworkSelection> many(max_selected_network_actors+1,selection[0]);assert(!s.start(true,many).value);assert(!s.sample().value);assert(f.calls==0);f.clean();}
    if(std::endian::native!=std::endian::little)return 0;
    test_common();
    test_capacity();
    test_observed();
    test_buckets();
    test_update();
    for(unsigned fault=0;fault<5;++fault){Fake f;{
        if(fault==0)f.deny=1;if(fault==1){f.throwing=true;f.throw_at=1;}
        if(fault==2){f.identity[1].kernel.kernel_tgid=999;f.identity[1].kernel.leader_start_boottime_ns=800;}
        if(fault==3)++f.identity[1].local.observer.user_namespace_inode;if(fault==4)++f.identity[1].local.boot[0];
        LinuxMultiNetworkSampler s(f.capture(),f.clock(),f.target());bool thrown=false;try{assert(!s.start(true,selection).value);}catch(const std::runtime_error&){thrown=true;}
        assert(thrown==(fault==1));f.clean();
    }f.clean();}
    {Fake f;{
        LinuxMultiNetworkSampler s(f.capture(),f.clock(),f.target());auto started=s.start(true,selection);assert(started.value);const auto epoch=*started.value;
        assert(!s.start(true,selection).value);auto first=sample(s);baseline(first);assert(first.actors.size()==2&&first.selection_epoch==epoch);
        f.tick();f.values[0]={100,200,300,400,0,0,0,0,4};f.values[1]={300,600,900,1200,0,0,0,0,4};auto w=sample(s);
        for(unsigned i=0;i<2;++i){const auto& a=w.actors[i].window.accounting.actors[0];assert(a.actor.tgid==999+i&&a.actor.real_uid==2000);
            assert(a.interval_ns.value==(i?3'000'000'000ULL:2'000'000'000ULL));
            for(unsigned lane=0;lane<4;++lane)assert(a.bytes_per_second[lane].value==(i?100:50)*(lane+1));}
        // Same UID does not create an invented common-window cross-actor sum.
        s.stop();s.stop();f.clean();assert(s.start(true,selection).value);first=sample(s);baseline(first);assert(first.selection_epoch!=epoch);
        const auto calls=f.calls;f.process=9;assert(!s.sample().value&&f.calls==calls);f.clean();
    }f.clean();}
    // Second actor fails AFTER the first actor has already advanced its reducer.
    for(unsigned fault=0;fault<4;++fault){Fake f;{
        LinuxMultiNetworkSampler s(f.capture(),f.clock(),f.target());assert(s.start(true,selection).value);baseline(sample(s));f.tick();
        if(fault==0)f.deny=1;if(fault==1){f.throwing=true;f.throw_at=1;}if(fault==2)f.bad_capture=1;
        if(fault==3)++f.identity[1].local.process.user_namespace_inode;
        bool thrown=false;try{assert(!s.sample().value);}catch(const std::runtime_error&){thrown=true;}assert(thrown==(fault==1));
        f.deny=f.bad_capture=-1;f.tick();auto recovered=sample(s);baseline(recovered); // BOTH baselines discarded, no hidden first-actor commit.
        f.tick();auto measured=sample(s);for(const auto& a:measured.actors)assert(a.window.accounting.actors[0].bytes_per_second[0].value==0);
    }f.clean();}
}
