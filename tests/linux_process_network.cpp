#include "../src/linux_process_network.hpp"
#include <cassert>
#include <type_traits>
#include <utility>
using namespace pcinsight;using namespace pcinsight::detail;
namespace {
PrivateCommonNetworkWindow fixture(bool baseline=false){
    NetworkAccountingScope scope;scope.boot={1,2};scope.producer_epoch=4;
    scope.pid_domain=scope.user_domain=NetworkIdentityDomain::kernel_initial;scope.coverage=NetworkAccountingCoverage::selected_actor_set;
    scope.producer_tgid=123;scope.producer_start_boottime_ns=900;
    NetworkAccountingInput input{scope,1'000'000'000,0,true,{{{999,2000,800,12},{},true,false},{{1000,2000,801,12},{},true,false}}};
    LinuxNetworkAccounting reducer;auto result=reducer.sample({input,Support::available,{}});assert(result.value);
    if(!baseline){input.observed_at_ns=2'000'000'000;input.rows[0].bytes={100,200,300,400};input.rows[1].bytes={200,400,600,800};
        result=reducer.sample({input,Support::available,{}});assert(result.value);}
    PrivateCommonNetworkWindow frame;frame.accounting=*result.value;frame.read_finished_ns=input.observed_at_ns;frame.read_started_ns=input.observed_at_ns-10;
    if(!baseline)frame.previous_read_bracket=std::array<std::uint64_t,2>{999'999'990,1'000'000'000};
    for(int i=1;i>=0;--i){ // Deliberately opposite to accounting actor order.
        LinuxNetworkSelfIdentity observer{7,42,100,1,2,3,4,5,6,7};
        LinuxNetworkSelfIdentity process{static_cast<std::uint32_t>(8+i),43,static_cast<std::uint64_t>(200+i),1,2,static_cast<std::uint64_t>(8+i),4,9,6,10};
        frame.associations.push_back({input.rows[i].actor,{{1,2},observer,process},{input.rows[i].bytes,8,4}});
    }return frame;
}
PrivateCommonNetworkWindow observed_fixture(bool empty=false){
    auto f=fixture();f.accounting.scope.coverage=NetworkAccountingCoverage::selected_observed_actor_set;
    for(const auto& a:f.associations)f.observed.push_back({{a.actor.tgid,a.actor.real_uid,a.actor.leader_start_boottime_ns},a.local,
        {{a.actor.socket_network_namespace_cookie,a.quality.bytes,a.quality.excluded_positive_receives}},a.quality.completed_updates});
    f.associations.clear();
    if(empty){f.accounting.actors.clear();f.accounting.users.clear();for(auto& a:f.observed){a.buckets.clear();a.completed_updates=0;}}
    return f;
}
void test_observed(){
    auto f=observed_fixture();auto r=publish_linux_process_network(f);assert(r.value&&r.support==Support::estimated);
    assert(r.value->coverage==LinuxProcessNetworkCoverage::observed_tcp_udp_socket_namespace_buckets&&r.value->observed_processes.size()==2);
    assert(r.value->observed_processes[0].process.pid==9&&r.value->processes[0].process.pid==8); // Match keys, not position.
    for(const auto& a:r.value->observed_processes)assert(a.initial_real_uid==2000&&a.process.real_uid==43&&a.observed_buckets==1&&a.completed_updates==4);
    auto empty=publish_linux_process_network(observed_fixture(true));assert(empty.value&&empty.value->processes.empty()&&empty.value->users.empty());
    assert(empty.value->observed_processes.size()==2&&empty.value->observed_processes[0].observed_buckets==0&&empty.value->observed_processes[0].completed_updates==0);
    auto mixed=observed_fixture();mixed.accounting.actors.erase(mixed.accounting.actors.begin());mixed.observed[1].buckets.clear();mixed.observed[1].completed_updates=0;
    auto& u=mixed.accounting.users[0];u.observed_buckets=1;u.known.fill(1);for(unsigned lane=0;lane<4;++lane)u.bytes_per_second[lane].value=200*(lane+1);
    auto partial=publish_linux_process_network(mixed);assert(partial.value&&partial.value->processes.size()==1&&partial.value->observed_processes[1].observed_buckets==0);
    // A new bucket can be present while no matching rate baseline exists.
    for(auto& a:mixed.accounting.actors){a.interval_ns={{},Support::unsupported,"new bucket"};for(auto& rate:a.bytes_per_second)rate={{},Support::unsupported,"new bucket"};}
    u.known.fill(0);u.missing.fill(1);for(auto& rate:u.bytes_per_second)rate={{},Support::unsupported,"no known buckets"};
    partial=publish_linux_process_network(mixed);assert(partial.value&&!partial.value->processes[0].nominal_interval_ns.value);
    for(unsigned fault=0;fault<23;++fault){auto bad=f;
        switch(fault){
        case 0:bad.observed.clear();break;case 1:bad.observed.push_back(bad.observed[0]);break;
        case 2:bad.observed[0].identity=bad.observed[1].identity;break;case 3:bad.observed[0].local.process=bad.observed[1].local.process;break;
        case 4:++bad.observed[0].local.observer.local_real_uid;break;case 5:++bad.observed[0].local.boot[0];break;
        case 6:bad.observed[0].identity.kernel_tgid=0;break;case 7:bad.observed[0].identity.leader_start_boottime_ns=0;break;
        case 8:++bad.observed[0].local.process.procfs_inode;break;case 9:bad.observed[0].completed_updates=0;break;
        case 10:bad.observed[0].buckets[0].socket_namespace_cookie=0;break;
        case 11:bad.observed[0].buckets.push_back(bad.observed[0].buckets[0]);break;
        case 12:bad.observed[0].buckets.resize(17,bad.observed[0].buckets[0]);bad.observed[0].completed_updates=17;break;
        case 13:bad.observed[0].buckets[0].bytes={};bad.observed[0].buckets[0].excluded_positive_receives=0;break;
        case 14:++bad.observed[0].buckets[0].bytes[0];break;case 15:bad.observed[0].buckets.clear();break;
        case 16:bad.associations=fixture().associations;break;case 17:bad.accounting.actors.pop_back();break;
        case 18:bad.accounting.actors[0]=bad.accounting.actors[1];break;case 19:++bad.accounting.users[0].known[0];break;
        case 20:bad.accounting.scope.coverage=NetworkAccountingCoverage::selected_actor_set;break;
        case 21:bad.observed.resize(LinuxProcessNetworkSampler::max_processes+1,bad.observed[0]);break;case 22:bad.accounting.actors[0].bytes.support=Support::estimated;break;
        }assert(!publish_linux_process_network(bad).value);
    }
    // Multiple cookies repeat one actor's update sequence, NOT a per-cookie sum.
    auto multi=observed_fixture();auto extra=multi.accounting.actors[0];extra.actor.socket_network_namespace_cookie=13;
    multi.accounting.actors.push_back(extra);multi.observed[1].buckets.push_back({13,*extra.bytes.value,8});
    auto& user=multi.accounting.users[0];++user.observed_buckets;
    for(unsigned lane=0;lane<4;++lane){++user.known[lane];*user.bytes_per_second[lane].value+=*extra.bytes_per_second[lane].value;}
    auto many=publish_linux_process_network(multi);assert(many.value&&many.value->processes.size()==3&&many.value->observed_processes[1].observed_buckets==2);
    assert(many.value->processes[0].completed_updates==4&&many.value->processes[2].completed_updates==4);
    // A full max_processes x16 snapshot is valid. Namespace slots per actor stay16.
    auto full=observed_fixture(true);const auto proto=full.observed[0];full.observed.clear();full.accounting.actors.clear();
    NetworkAccountingInput input{full.accounting.scope,full.read_finished_ns,0,true,{}};
    for(std::uint32_t i=0;i<LinuxProcessNetworkSampler::max_processes;++i){auto actor=proto;actor.identity.kernel_tgid=100+i;actor.identity.leader_start_boottime_ns=800+i;
        actor.local.process.local_pid=10+i;actor.local.process.start_ticks=200+i;actor.local.process.process_inode=20+i;actor.completed_updates=16;
        for(std::uint64_t cookie=1;cookie<=16;++cookie){actor.buckets.push_back({cookie,{},1});input.rows.push_back({{actor.identity.kernel_tgid,2000,800+i,cookie},{},true,false});}
        full.observed.push_back(actor);}
    LinuxNetworkAccounting reducer;auto accounting=reducer.sample({input,Support::available,{}});assert(accounting.value);full.accounting=*accounting.value;
    auto maximum=publish_linux_process_network(full);assert(maximum.value&&maximum.value->processes.size()==LinuxProcessNetworkSampler::max_observed_buckets&&maximum.value->observed_processes.size()==LinuxProcessNetworkSampler::max_processes);
    for(const auto& row:maximum.value->processes)assert(row.payload_bytes_since_start.value&&!row.payload_bytes_per_second[0].value);
    LinuxProcessNetworkSampler selected(LinuxProcessNetworkSelectionMode::observed_namespace_buckets);assert(!selected.sample().value);
    assert(selected.start(false,{}).support==Support::permission_required&&!selected.start(true,{}).value);selected.stop();
}
}
int main(){
    test_observed();
    static_assert(!std::is_copy_constructible_v<LinuxProcessNetworkSampler>);
    static_assert(std::is_nothrow_move_constructible_v<LinuxProcessNetworkSampler>);
    static_assert(std::is_nothrow_move_assignable_v<LinuxProcessNetworkSampler>);
    {LinuxProcessNetworkSampler s;assert(!s.sample().value);auto denied=s.start(false,{});assert(!denied.value&&denied.support==Support::permission_required);
        const LinuxProcessNetworkSelection invalid{-1,8,200,12};assert(!s.start(true,std::span(&invalid,1)).value);
        auto moved=std::move(s);assert(!s.sample().value);assert(!s.start(false,{}).value);s.stop();moved.stop();moved.stop();
        s=std::move(moved);assert(!s.sample().value);
    }
    auto frame=fixture();auto result=publish_linux_process_network(frame);assert(result.value&&result.support==Support::estimated);
    {auto bucket=fixture();bucket.accounting.scope.coverage=NetworkAccountingCoverage::selected_namespace_bucket_set;
        bucket.accounting.actors[1].actor=bucket.accounting.actors[0].actor;bucket.accounting.actors[1].actor.socket_network_namespace_cookie=13;
        bucket.associations[0].actor=bucket.accounting.actors[1].actor;bucket.associations[0].local=bucket.associations[1].local;
        auto published=publish_linux_process_network(bucket);assert(published.value&&published.value->coverage==LinuxProcessNetworkCoverage::declared_tcp_udp_socket_namespace_buckets);
        assert(published.value->processes.size()==2&&published.value->processes[0].process==published.value->processes[1].process);
        assert(published.value->users[0].partial_payload_bytes_per_second[0].value==300&&published.value->users[0].observed_buckets==2);
        for(unsigned fault=0;fault<6;++fault){auto bad=bucket;
            if(fault==0)bad.accounting.scope.coverage=NetworkAccountingCoverage::selected_actor_set;
            if(fault==1)bad.associations[0].actor.socket_network_namespace_cookie=12;
            if(fault==2)++bad.associations[0].actor.real_uid;
            if(fault==3)++bad.associations[0].actor.leader_start_boottime_ns;
            if(fault==4)++bad.associations[0].local.process.user_namespace_inode;
            if(fault==5)++bad.associations[0].local.process.local_pid;
            assert(!publish_linux_process_network(bad).value);
        }
        LinuxProcessNetworkSampler selected(LinuxProcessNetworkSelectionMode::declared_namespace_buckets);assert(!selected.sample().value);
        assert(selected.start(false,{}).support==Support::permission_required);assert(!selected.start(true,{}).value);selected.stop();
        LinuxProcessNetworkSampler invalid(static_cast<LinuxProcessNetworkSelectionMode>(99));const LinuxProcessNetworkSelection row{0,8,200,12};assert(!invalid.start(true,std::span(&row,1)).value);
    }
    const auto& out=*result.value;assert(out.coverage==LinuxProcessNetworkCoverage::selected_tcp_udp_socket_operations);
    assert(out.selection_epoch==4&&out.producer_initial_tgid==123&&out.observer.pid==7&&out.observer.real_uid==42);
    assert(out.processes.size()==2&&out.users.size()==1&&out.users[0].initial_real_uid==2000);
    for(unsigned i=0;i<2;++i){const auto& p=out.processes[i];assert(p.kernel.initial_tgid==999+i&&p.process.pid==8+i&&p.process.real_uid==43);
        assert(p.kernel.initial_real_uid==2000&&p.process.user_namespace_inode==10&&out.observer.user_namespace_inode==7);
        assert(p.nominal_interval_ns.value==1'000'000'000&&p.excluded_positive_receives==8&&p.completed_updates==4);
        for(unsigned lane=0;lane<4;++lane){assert(p.payload_bytes_per_second[lane].value==(i+1)*100*(lane+1));assert(p.payload_bytes_per_second[lane].support==Support::estimated);}}
    for(unsigned lane=0;lane<4;++lane){assert(out.users[0].partial_payload_bytes_per_second[lane].value==300*(lane+1));assert(out.users[0].known[lane]==2&&out.users[0].missing[lane]==0);}
    auto first=publish_linux_process_network(fixture(true));assert(first.value&&!first.value->previous_counter_sweep);
    for(const auto& p:first.value->processes){assert(p.payload_bytes_since_start.value&&(*p.payload_bytes_since_start.value)[0]==0&&!p.nominal_interval_ns.value);
        assert(!p.payload_bytes_per_second[0].value&&!p.payload_bytes_per_second[0].note.empty());}
    assert(!first.value->users[0].partial_payload_bytes_per_second[0].value&&first.value->users[0].missing[0]==2);
    for(unsigned fault=0;fault<34;++fault){auto bad=frame;auto& a=bad.accounting;
        switch(fault){
        case 0:a.scope.coverage=NetworkAccountingCoverage::declared_buckets;break;
        case 1:a.scope.pid_domain=NetworkIdentityDomain::namespace_inode;break;case 2:a.scope.user_namespace=1;break;
        case 3:a.scope.boot={};break;case 4:a.scope.producer_epoch=0;break;case 5:a.scope.producer_tgid=0;break;
        case 6:a.scope.producer_start_boottime_ns=0;break;case 7:a.lost_updates=1;break;case 8:bad.read_started_ns=0;break;
        case 9:bad.read_started_ns=bad.read_finished_ns+1;break;case 10:bad.read_started_ns=1;break;
        case 11:++a.observed_at_ns;break;case 12:bad.previous_read_bracket.reset();break;case 13:(*bad.previous_read_bracket)[0]=0;break;
        case 14:(*bad.previous_read_bracket)[1]=bad.read_finished_ns;break;case 15:bad.associations.pop_back();break;
        case 16:bad.associations[0]=bad.associations[1];break;case 17:++bad.associations[0].quality.bytes[0];break;
        case 18:++bad.associations[0].local.observer.local_real_uid;break;case 19:++bad.associations[0].local.process.procfs_inode;break;
        case 20:bad.associations[0].local.process.local_pid=bad.associations[1].local.process.local_pid;break;
        case 21:bad.associations[0].actor.tgid=0;break;case 22:a.actors[0]=a.actors[1];break;
        case 23:a.actors[0].bytes.support=Support::estimated;break;case 24:a.actors[0].bytes.value.reset();break;
        case 25:a.actors[0].bytes_per_second[0].support=Support::available;break;case 26:a.actors[0].interval_ns.value=123;break;
        case 27:a.users[0].real_uid=43;break;case 28:++a.users[0].known[0];break;case 29:++a.users[0].missing[0];break;
        case 30:a.users[0].overflow[0]=true;break;case 31:++*a.users[0].bytes_per_second[0].value;break;
        case 32:a.users[0].bytes_per_second[0].support=Support::available;break;case 33:a.users.push_back(a.users[0]);break;
        }assert(!publish_linux_process_network(bad).value);
    }
    auto zero_uid=fixture();for(auto& p:zero_uid.associations)p.actor.real_uid=0;for(auto& p:zero_uid.accounting.actors)p.actor.real_uid=0;
    zero_uid.accounting.users[0].real_uid=0;assert(publish_linux_process_network(zero_uid).value);
    auto overflow=frame;for(auto& p:overflow.accounting.actors){p.bytes.value->fill(UINT64_MAX);for(auto& rate:p.bytes_per_second)rate.value=UINT64_MAX;}
    for(auto& association:overflow.associations)association.quality.bytes.fill(UINT64_MAX);
    for(unsigned i=0;i<4;++i){auto& u=overflow.accounting.users[0];u.overflow[i]=true;u.bytes_per_second[i]={{},Support::unsupported,"UID rate sum overflow"};}
    auto projected=publish_linux_process_network(overflow);assert(projected.value&&projected.value->users[0].overflow[0]&&!projected.value->users[0].partial_payload_bytes_per_second[0].value);
}
