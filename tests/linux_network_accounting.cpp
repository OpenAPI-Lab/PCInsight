#include "../src/linux_network_accounting.hpp"
#include <cassert>
#include <limits>
using namespace pcinsight;
using namespace pcinsight::detail;
constexpr std::uint64_t second=1'000'000'000;
constexpr auto maximum=std::numeric_limits<std::uint64_t>::max();
NetworkAccountingInput fixture(){
    return {{{1,2},3,4,5},second,0,true,{
        {{10,1000,100,7},{100,200,300,400},true,false},
        {{11,1000,200,7},{400,300,200,100},true,false},
        {{12,1001,300,8},{0,0,0,0},true,false}}};
}
Metric<NetworkAccountingInput> known(NetworkAccountingInput value){return {std::move(value),Support::available,{}};}
NetworkAccountingOutput sample(LinuxNetworkAccounting& tracker,NetworkAccountingInput input){
    auto out=tracker.sample(known(std::move(input)));assert(metric_is_known(out));return *out.value;
}
void no_rates(const NetworkAccountingOutput& out){for(const auto& row:out.actors){
    assert(!row.interval_ns.value);for(const auto& value:row.bytes_per_second)assert(!value.value);
}}
int main(){
    {auto buckets=fixture();buckets.rows.resize(2);auto& s=buckets.scope;
        s.pid_domain=s.user_domain=NetworkIdentityDomain::kernel_initial;s.pid_namespace=s.user_namespace=0;
        s.coverage=NetworkAccountingCoverage::selected_namespace_bucket_set;s.producer_tgid=99;s.producer_start_boottime_ns=900;
        buckets.rows[1].actor=buckets.rows[0].actor;buckets.rows[1].actor.socket_network_namespace_cookie=8;
        LinuxNetworkAccounting group;no_rates(sample(group,buckets));buckets.observed_at_ns+=second;
        for(auto& row:buckets.rows)for(auto& value:row.bytes)value+=10;
        const auto rates=sample(group,buckets);assert(rates.actors.size()==2&&rates.users.size()==1&&rates.users[0].observed_buckets==2);
        for(unsigned lane=0;lane<4;++lane)assert(rates.users[0].bytes_per_second[lane].value==20);
        for(unsigned fault=0;fault<5;++fault){auto bad=buckets;
            if(fault==0)bad.scope.coverage=NetworkAccountingCoverage::selected_actor_set;
            if(fault==1)bad.rows[1].actor.socket_network_namespace_cookie=bad.rows[0].actor.socket_network_namespace_cookie;
            if(fault==2)++bad.rows[1].actor.real_uid;if(fault==3)bad.rows[0].actor.tgid=0;
            if(fault==4)bad.rows.resize(max_selected_network_actors+1,bad.rows[0]);LinuxNetworkAccounting checked;assert(!checked.sample(known(bad)).value);}
    }
    for(unsigned fault=0;fault<9;++fault){
        auto self=fixture();self.rows.resize(1);auto& s=self.scope;
        s.pid_domain=s.user_domain=NetworkIdentityDomain::kernel_initial;s.pid_namespace=s.user_namespace=0;
        s.coverage=NetworkAccountingCoverage::self_single_socket_namespace;s.producer_tgid=10;s.producer_start_boottime_ns=100;
        if(fault==0)s.pid_namespace=1;if(fault==1)s.user_namespace=1;if(fault==2)s.producer_tgid=11;
        if(fault==3)s.producer_start_boottime_ns=101;if(fault==4)self.rows.clear();
        if(fault==5)self.rows.push_back(self.rows[0]);if(fault==6)s.pid_domain=static_cast<NetworkIdentityDomain>(99);
        if(fault==7)s.coverage=static_cast<NetworkAccountingCoverage>(99);
        LinuxNetworkAccounting checked;auto result=checked.sample(known(self));assert(result.value.has_value()==(fault==8));
    }
    LinuxNetworkAccounting tracker;
    auto input=fixture();auto initial=sample(tracker,input);no_rates(initial);
    for(const auto& user:initial.users)for(std::size_t i=0;i<4;++i){
        assert(!user.bytes_per_second[i].value&&user.known[i]==0);
        assert(user.missing[i]==user.observed_buckets);
    }
    input.observed_at_ns+=2*second;
    input.rows[0].bytes={120,240,360,480};input.rows[1].bytes={440,360,280,200};
    auto out=sample(tracker,input);
    assert(out.actors.size()==3&&out.users.size()==2);
    for(std::size_t i=0;i<4;++i){
        assert(*out.actors[0].bytes_per_second[i].value==(i+1)*10);
        assert(*out.actors[1].bytes_per_second[i].value==(i+2)*10);
        assert(*out.actors[2].bytes_per_second[i].value==0); // Known unchanged, not absence.
        assert(*out.users[0].bytes_per_second[i].value==(2*i+3)*10);
        assert(out.users[0].known[i]==2&&out.users[0].missing[i]==0);
    }
    assert(*out.actors[0].interval_ns.value==2*second);
    assert(out.users[0].bytes_per_second[0].support==Support::estimated);
    // UID at event time, PID generation, socket net namespace and collector
    // identity are part of the bucket; none may reuse another bucket's baseline.
    for(int field=0;field<4;++field){
        tracker.reset();input=fixture();sample(tracker,input);input.observed_at_ns+=second;
        auto& key=input.rows[0].actor;
        if(field==0)++key.tgid;if(field==1)++key.real_uid;
        if(field==2)++key.leader_start_boottime_ns;if(field==3)++key.socket_network_namespace_cookie;
        out=sample(tracker,input);
        for(const auto& row:out.actors)if(row.actor==key)assert(!row.bytes_per_second[0].value);
    }
    for(int field=0;field<5;++field){
        tracker.reset();input=fixture();sample(tracker,input);input.observed_at_ns+=second;
        if(field==0)++input.scope.boot[0];if(field==1)++input.scope.boot[1];
        if(field==2)++input.scope.producer_epoch;if(field==3)++input.scope.pid_namespace;
        if(field==4)++input.scope.user_namespace;no_rates(sample(tracker,input));
    }
    // Loss invalidates the crossing interval; a later loss-free interval works.
    tracker.reset();input=fixture();sample(tracker,input);
    input.observed_at_ns+=second;++input.lost_updates;no_rates(sample(tracker,input));
    input.observed_at_ns+=second;out=sample(tracker,input);
    assert(out.actors[0].bytes.value&&out.actors[0].bytes.support==Support::estimated);
    assert(out.actors[0].bytes_per_second[0].value==0);
    // Bad row never turns into zero, and cannot seed the following interval.
    for(int bad=0;bad<2;++bad){
        tracker.reset();input=fixture();sample(tracker,input);input.observed_at_ns+=second;
        input.rows[0].coherent=bad!=0;input.rows[0].saturated=bad!=0;
        out=sample(tracker,input);assert(!out.actors[0].bytes.value);
        assert(out.users[0].known[0]==1&&out.users[0].missing[0]==1);
        input.observed_at_ns+=second;input.rows[0].coherent=true;input.rows[0].saturated=false;
        out=sample(tracker,input);assert(!out.actors[0].bytes_per_second[0].value);
        input.observed_at_ns+=second;out=sample(tracker,input);assert(out.actors[0].bytes_per_second[0].value==0);
    }
    tracker.reset();input=fixture();sample(tracker,input);input.observed_at_ns+=second;
    --input.rows[0].bytes[3];out=sample(tracker,input);
    for(const auto& r:out.actors[0].bytes_per_second)assert(!r.value);
    // Removal means no baseline if the same actor reappears later.
    tracker.reset();input=fixture();sample(tracker,input);auto saved=input.rows[0];
    input.rows.erase(input.rows.begin());input.observed_at_ns+=second;sample(tracker,input);
    input.rows.push_back(saved);input.observed_at_ns+=second;out=sample(tracker,input);
    assert(!out.actors[0].bytes_per_second[0].value);
    // Each malformed snapshot clears state, including contradictory support.
    for(int bad=0;bad<10;++bad){
        tracker.reset();input=fixture();sample(tracker,input);input.observed_at_ns+=second;
        auto invalid=known(input);
        if(bad==0)invalid.value->enumeration_complete=false;
        if(bad==1)invalid.value->rows.push_back(invalid.value->rows[0]);
        if(bad==2)invalid.value->scope.producer_epoch=0;
        if(bad==3)invalid.value->rows[0].actor.tgid=0;
        if(bad==4)invalid.value->rows[0].actor.socket_network_namespace_cookie=0;
        if(bad==5)invalid.value->observed_at_ns=second;
        if(bad==6)invalid.support=Support::permission_required;
        if(bad==7)invalid.support=Support::estimated;
        if(bad==8)invalid.value.reset();
        if(bad==9)invalid.value->rows.resize(LinuxNetworkAccounting::max_rows+1);
        assert(!tracker.sample(invalid).value);no_rates(sample(tracker,input));
    }
    tracker.reset();input=fixture();input.lost_updates=1;sample(tracker,input);
    ++input.observed_at_ns;input.lost_updates=0;assert(!tracker.sample(known(input)).value);
    tracker.reset();input=fixture();sample(tracker,input);
    input.observed_at_ns+=LinuxNetworkAccounting::max_interval_ns+1;no_rates(sample(tracker,input));
    input.observed_at_ns+=second;out=sample(tracker,input);assert(out.actors[0].bytes_per_second[0].value==0);
    // Exact floor and overflow at full uint64 range, no float rounding.
    for(const auto ns:std::array<std::uint64_t,5>{1,999'999'999,second,second+1,60*second}){
        tracker.reset();input=fixture();input.rows.resize(1);input.rows[0].bytes={0,0,0,0};sample(tracker,input);
        input.observed_at_ns+=ns;input.rows[0].bytes={maximum,0,1,60};out=sample(tracker,input);
        if(ns<second)assert(!out.actors[0].bytes_per_second[0].value);
        else assert(out.actors[0].bytes_per_second[0].value);
        if(ns==second)assert(*out.actors[0].bytes_per_second[0].value==maximum);
        if(ns==second+1)assert(*out.actors[0].bytes_per_second[0].value==18'446'744'055'262'807'559ULL);
        assert(*out.actors[0].bytes_per_second[2].value==second/ns);
        assert(*out.actors[0].bytes_per_second[3].value==60*second/ns);
    }
    tracker.reset();input=fixture();input.rows.resize(2);for(auto& row:input.rows)row.bytes={0,0,0,0};
    sample(tracker,input);input.observed_at_ns+=second;
    for(auto& row:input.rows)row.bytes={maximum,0,0,0};out=sample(tracker,input);
    assert(out.users[0].overflow[0]&&!out.users[0].bytes_per_second[0].value);
    assert(out.users[0].known[0]==2&&out.users[0].missing[0]==0);
    assert(out.users[0].bytes_per_second[1].value==0);
    // Random deterministic portable oracle where multiplication cannot overflow.
    std::uint64_t rng=17;
    for(unsigned iteration=0;iteration<10000;++iteration){
        rng=rng*6364136223846793005ULL+1;
        const auto delta=rng%(maximum/second);
        rng=rng*6364136223846793005ULL+1;
        const auto ns=1+rng%LinuxNetworkAccounting::max_interval_ns;
        tracker.reset();input=fixture();input.rows.resize(1);input.rows[0].bytes={0,0,0,0};
        sample(tracker,input);input.observed_at_ns+=ns;input.rows[0].bytes[0]=delta;
        out=sample(tracker,input);assert(*out.actors[0].bytes_per_second[0].value==delta*second/ns);
    }
    // Selected coverage is a single actor bucket, not SELF or global enumeration.
    tracker.reset();input=fixture();input.rows.resize(1);input.scope.pid_domain=input.scope.user_domain=NetworkIdentityDomain::kernel_initial;
    input.scope.pid_namespace=input.scope.user_namespace=0;input.scope.coverage=NetworkAccountingCoverage::selected_single_socket_namespace;
    input.scope.producer_tgid=123;input.scope.producer_start_boottime_ns=900;
    out=sample(tracker,input);assert(out.scope.coverage==NetworkAccountingCoverage::selected_single_socket_namespace);
    auto selected=input;selected.rows.clear();assert(!tracker.sample(known(selected)).value);
    selected=input;selected.rows.push_back(selected.rows[0]);assert(!tracker.sample(known(selected)).value);
    selected=input;selected.rows[0].actor.leader_start_boottime_ns=0;assert(!tracker.sample(known(selected)).value);
    selected=input;selected.rows[0].actor.tgid=2147483648U;assert(!tracker.sample(known(selected)).value);
    selected=input;selected.scope.coverage=NetworkAccountingCoverage::selected_actor_set;assert(tracker.sample(known(selected)).value);
    selected.rows.clear();assert(!tracker.sample(known(selected)).value);
    selected=input;selected.scope.coverage=NetworkAccountingCoverage::selected_actor_set;
    auto alias=selected.rows[0];++alias.actor.real_uid;++alias.actor.socket_network_namespace_cookie;selected.rows.push_back(alias);
    assert(!tracker.sample(known(selected)).value); // Same kernel process is not another actor just because UID/cookie differ.
    selected.rows.resize(max_selected_network_actors+1);assert(!tracker.sample(known(selected)).value);
    // Empty successful enumeration is not an absent snapshot or synthetic UID0.
    input=fixture();
    input.observed_at_ns+=second;input.rows.clear();out=sample(tracker,input);
    assert(out.actors.empty()&&out.users.empty());
}
