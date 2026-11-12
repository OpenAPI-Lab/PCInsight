#include "pcinsight/linux_user_network.hpp"
#include "pcinsight/monitor.hpp"
#include "../src/linux_users.hpp"
#include <cassert>
#include <iostream>
using namespace pcinsight;
Metric<LinuxProcessNetworkSnapshot> frame(){
    LinuxProcessNetworkSnapshot f;f.coverage=LinuxProcessNetworkCoverage::observed_tcp_udp_socket_namespace_buckets;
    f.observer={10,1000,50,1,2,3,4,5,6,7};f.observer_boot_token={1,2};f.selection_epoch=99;f.producer_initial_tgid=100;f.producer_start_boottime_ns=500;
    f.counter_sweep_started_ns=2'000'000'000;f.counter_sweep_finished_ns=2'000'000'100;f.previous_counter_sweep=std::array<std::uint64_t,2>{1'000'000'000,1'000'000'100};
    return {std::move(f),Support::estimated,"fixture verified F03 source"};
}
void add(auto& input,std::uint32_t pid,std::uint32_t local_uid,std::uint32_t initial_uid,bool measured=true,bool idle=false,std::uint64_t cookie=99){
    auto& f=*input.value;
    LinuxNetworkObservedProcess a{pid+1000,initial_uid,500,{pid,local_uid,50,1,2,pid+100,4,5,6,7},idle?0ULL:1ULL,idle?0ULL:1ULL};f.observed_processes.push_back(a);
    if(idle)return;
    LinuxProcessNetworkSample r;r.process=a.process;r.kernel={a.initial_tgid,a.initial_real_uid,a.leader_start_boottime_ns,cookie};r.completed_updates=1;r.payload_bytes_since_start={std::array<std::uint64_t,4>{10,20,30,40},Support::available,"counter fixture"};
    if(measured){r.nominal_interval_ns={1'000'000'000,Support::available,"interval"};for(unsigned n=0;n<4;++n)r.payload_bytes_per_second[n]={10ULL*(n+1),Support::estimated,"fixture rate"};}
    f.processes.push_back(r);
}
const auto& only(const auto& r){if(!r.value)std::cerr<<r.note<<'\n';assert(r.value&&r.value->users.size()==1);return r.value->users[0];}
int main(){
    auto input=frame();add(input,20,1000,0);add(input,21,1000,100001);add(input,22,1000,0,false);add(input,23,1000,100001,false,true);
    // Kernel-domain user sums are deliberately unrelated: never used as local UID totals.
    input.value->users.push_back({});auto result=summarize_linux_user_network(input);const auto& u=only(result);
    assert(u.real_uid==1000&&u.initial_real_uids==std::vector<std::uint32_t>({0,100001}));
    assert(u.selected_processes==4&&u.idle_processes==1&&u.observed_buckets==3);
    for(unsigned n=0;n<4;++n){assert(u.known[n]==2&&u.missing[n]==1&&u.permission_denied[n]==0&&!u.overflow[n]);assert(u.partial_payload_bytes_per_second[n].value==20ULL*(n+1));}
    assert(u.partial_send_bytes_per_second.value==80&&u.partial_receive_bytes_per_second.value==120&&u.partial_total_bytes_per_second.value==200);
    assert(result.value->observer==input.value->observer&&result.value->selection_epoch==99&&result.support==Support::estimated);
    auto replay=summarize_linux_user_network(input);assert(only(replay).partial_total_bytes_per_second.value==200); // Stateless, no cumulative sums.
    for(unsigned n=0;n<4;++n)input.value->processes[2].payload_bytes_per_second[n]={{},Support::permission_required,"denied fixture"};
    result=summarize_linux_user_network(input);assert(only(result).permission_denied[0]==1&&only(result).partial_total_bytes_per_second.value==200);
    input=frame();add(input,20,0,0,false);result=summarize_linux_user_network(input);assert(only(result).real_uid==0&&!only(result).partial_total_bytes_per_second.value);
    input=frame();add(input,20,0,0,false,true);result=summarize_linux_user_network(input);assert(only(result).idle_processes==1&&only(result).observed_buckets==0&&!only(result).partial_total_bytes_per_second.value);
    input=frame();add(input,20,0,0);for(auto& rate:input.value->processes[0].payload_bytes_per_second)rate.value=0;
    result=summarize_linux_user_network(input);assert(only(result).partial_total_bytes_per_second.value==0); // Measured zero, not unknown.
    // Separate numeric UIDs even if sessions use the same label; none are joined.
    input=frame();add(input,20,1000,0);add(input,21,2000,0);result=summarize_linux_user_network(input);
    assert(result.value->users.size()==2&&result.value->users[0].real_uid==1000&&result.value->users[1].real_uid==2000);
    std::vector<detail::LinuxLoginRecord> logins;
    for(unsigned n=0;n<3;++n){detail::LinuxLoginRecord login;login.id=std::to_string(n);login.username="same-label";LinuxSessionDetails d;d.unix_uid={n==2?2000U:1000U,Support::available,"reported only"};login.details={d,Support::available,"fixture"};logins.push_back(login);}
    const auto sessions=detail::aggregate_linux_users(logins,{});assert(sessions.size()==3);for(const auto& s:sessions)assert(!s.network_bytes_per_second.value);assert(result.value->users.size()==2);
    // Same PID across two cookies counts one actor, both bucket rates once.
    input=frame();add(input,20,1000,0);auto extra=input.value->processes[0];extra.kernel.socket_network_namespace_cookie++;
    input.value->observed_processes[0].observed_buckets=2;input.value->observed_processes[0].completed_updates=2;input.value->processes[0].completed_updates=extra.completed_updates=2;input.value->processes.push_back(extra);
    result=summarize_linux_user_network(input);assert(only(result).selected_processes==1&&only(result).observed_buckets==2&&only(result).partial_total_bytes_per_second.value==200);
    // Overflow is not a partial wrap, and doesn't destroy independent receive data.
    input=frame();add(input,20,0,0);auto& rates=input.value->processes[0].payload_bytes_per_second;
    rates[0].value=UINT64_MAX;rates[1].value=0;rates[2].value=1;rates[3].value=0;
    result=summarize_linux_user_network(input);assert(only(result).send_overflow&&only(result).total_overflow&&!only(result).partial_send_bytes_per_second.value&&!only(result).partial_total_bytes_per_second.value&&only(result).partial_receive_bytes_per_second.value==0);
    add(input,21,0,0);result=summarize_linux_user_network(input);assert(only(result).overflow[0]&&!only(result).partial_payload_bytes_per_second[0].value);
    for(unsigned mode=0;mode<15;++mode){input=frame();add(input,20,1000,0);auto& f=*input.value;auto& r=f.processes[0];
        if(mode==0)f.observed_processes.push_back(f.observed_processes[0]);
        if(mode==1)f.processes.push_back(r);
        if(mode==2)r.process.real_uid++;
        if(mode==3)r.kernel.initial_real_uid++;
        if(mode==4)r.kernel.leader_start_boottime_ns++;
        if(mode==5)r.payload_bytes_since_start.support=Support::estimated;
        if(mode==6)r.payload_bytes_per_second[0].support=Support::available;
        if(mode==7)r.nominal_interval_ns.value.reset();
        if(mode==8)r.nominal_interval_ns.value=1;
        if(mode==9)f.previous_counter_sweep.reset();
        if(mode==10)f.observed_processes[0].observed_buckets++;
        if(mode==11)f.counter_sweep_finished_ns=f.counter_sweep_started_ns-1;
        if(mode==12)f.observer.user_namespace_inode=0;
        if(mode==13)f.coverage=LinuxProcessNetworkCoverage::selected_tcp_udp_socket_operations;
        if(mode==14)r.payload_bytes_per_second[0]={{},Support::available,"invalid absence"};
        assert(!summarize_linux_user_network(input).value);
    }
    assert(!summarize_linux_user_network({{},Support::permission_required,"denied"}).value&&summarize_linux_user_network({{},Support::permission_required,"denied"}).support==Support::permission_required);
    // Maximum structural F03 capacity, deduplicated as1024 actors, not16384.
    input=frame();for(unsigned n=0;n<LinuxProcessNetworkSampler::max_processes;++n){add(input,100+n,1000,0);auto& a=input.value->observed_processes.back();a.observed_buckets=a.completed_updates=16;
        auto bucket=input.value->processes.back();input.value->processes.back().completed_updates=16;
        for(unsigned cookie=1;cookie<16;++cookie){bucket.kernel.socket_network_namespace_cookie=99+cookie;bucket.completed_updates=16;input.value->processes.push_back(bucket);}}
    result=summarize_linux_user_network(input);assert(only(result).selected_processes==1024&&only(result).observed_buckets==16384&&only(result).known[0]==16384&&only(result).partial_total_bytes_per_second.value==1'638'400);
    input.value->observed_processes.push_back(input.value->observed_processes.back());assert(!summarize_linux_user_network(input).value);
    std::cerr<<"pure observer-UID projection fixtures passed\n"<<std::flush;
#ifdef __linux__
    // Options alone cannot capture, invent user totals, or populate a session UID.
    Monitor monitor;SampleOptions options;options.include_processes=options.include_disks=options.include_networks=options.include_gpus=false;options.include_user_sessions=true;options.include_linux_process_network=true;
    const auto snapshot=monitor.sample(options);assert(!snapshot.linux_user_network.value);for(const auto& s:snapshot.user_sessions)assert(!s.network_bytes_per_second.value);
    options.include_user_sessions=false;options.include_linux_user_network=true;
    assert(!monitor.sample(options).linux_user_network.value); // Dedicated flag doesn't authorize BPF either.
#endif
    std::cout<<"unique observer-UID network: coverage/namespace domains/partial rates/overflow/unknown-zero/no session duplication passed\n";
}
