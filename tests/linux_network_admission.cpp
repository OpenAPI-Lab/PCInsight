#include "../src/linux_network_admission.hpp"
#include <cassert>
#include <algorithm>
#include <map>
#include <stdexcept>
#include <utility>
using namespace pcinsight;using namespace pcinsight::detail;
namespace {
struct Fake {
    int calls=0,fail=-1,throw_at=-1,nextfd=100;unsigned starts=0,updates=0,samples=0,stops=0,contexts=0,drift=0,clocks=0,stale=0;
    std::uint64_t caller=7,time=1'000'000'000,epoch=0;bool active=false,bad_sample=false;
    NetworkSelectionContext context{{1,2},{7,42,100,1,2,3,4,5,6,7}};
    std::vector<std::uint32_t> pids{8,9};std::map<int,std::uint32_t> fds;std::map<std::uint32_t,std::uint64_t> starts_at;
    LinuxNetworkInventoryState state=LinuxNetworkInventoryState::verified;
    std::uint32_t error_pid=0;
    static Fake& f(void* p){return *static_cast<Fake*>(p);}
    bool fault(){++calls;if(calls==throw_at)throw std::runtime_error("injected");return calls==fail;}
    Metric<NetworkSelectionContext> observe(){++contexts;if(fault())return {{},Support::permission_required,"context denied"};auto c=context;if(contexts==drift)++c.observer.user_namespace_inode;return {c,Support::available,{}};}
    LinuxNetworkSelfIdentity identity(std::uint32_t pid){return {pid,0,starts_at.contains(pid)?starts_at.at(pid):200+pid,1,2,100+pid,4,9,6,10};}
    NetworkInventoryApi inventory(){return {this,
        [](void* p){return f(p).observe();},
        [](void* p)->Metric<std::uint64_t>{auto& s=f(p);if(s.fault())return {{},Support::permission_required,"clock"};if(++s.clocks==s.stale)s.time+=6'000'000'000;const auto t=s.time;s.time+=10;return {t,Support::available,{}};},
        [](void* p,const NetworkSelectionContext&)->Metric<std::vector<std::uint32_t>>{auto& s=f(p);if(s.fault())return {{},Support::permission_required,"enum"};return {s.pids,Support::available,{}};},
        [](void* p,std::uint32_t pid)->NetworkInventoryObservation{auto& s=f(p);if(s.fault())return {LinuxNetworkInventoryState::permission_denied,{{},Support::permission_required,"inspect denied"}};
            const auto state=(!s.error_pid||pid==s.error_pid)?s.state:LinuxNetworkInventoryState::verified;
            if(state!=LinuxNetworkInventoryState::verified)return {state,{{},state==LinuxNetworkInventoryState::permission_denied?Support::permission_required:Support::unsupported,"row unavailable"}};
            return {state,{s.identity(pid),Support::available,{}}};}};}
    NetworkSelectionSourceApi source(){return {this,
        [](void* p) noexcept{return f(p).caller;},
        [](void* p){return f(p).observe();},
        [](void* p,std::uint32_t pid)->NetworkSessionFd{auto& s=f(p);if(s.fault())return {-1,13};const int fd=s.nextfd++;s.fds[fd]=pid;return {fd,0};},
        [](void* p,int fd,std::uint32_t pid,std::uint64_t start)->Metric<LinuxNetworkSelfIdentity>{auto& s=f(p);assert(s.fds.at(fd)==pid);if(s.fault())return {{},Support::permission_required,"pidfd denied"};auto i=s.identity(pid);if(i.start_ticks!=start)return {{},Support::unsupported,"start changed"};return {i,Support::available,{}};},
        [](void* p,int fd) noexcept{assert(f(p).fds.erase(fd)==1);}};}
    NetworkAdmissionCaptureApi capture(){return {this,
        [](void* p,std::span<const LinuxProcessNetworkSelection> rows)->Metric<std::uint64_t>{auto& s=f(p);assert(!s.active&&!rows.empty());++s.starts;if(s.fault())return {{},Support::permission_required,"start"};s.active=true;return {++s.epoch,Support::available,{}};},
        [](void* p,std::span<const LinuxProcessNetworkSelection> rows)->Metric<std::uint64_t>{auto& s=f(p);assert(s.active&&!rows.empty());++s.updates;if(s.fault())return {{},Support::permission_required,"update"};return {++s.epoch,Support::available,{}};},
        [](void* p)->Metric<LinuxProcessNetworkSnapshot>{auto& s=f(p);assert(s.active);++s.samples;if(s.fault())return {{},Support::permission_required,"sample"};LinuxProcessNetworkSnapshot r;r.coverage=LinuxProcessNetworkCoverage::observed_tcp_udp_socket_namespace_buckets;r.selection_epoch=s.epoch+(s.bad_sample?1:0);return {r,Support::estimated,{}};},
        [](void* p) noexcept{auto& s=f(p);++s.stops;s.active=false;}};}
    void clean(){assert(fds.empty()&&!active);}
};
void success(){Fake f;LinuxNetworkAdmission a(f.inventory(),f.source(),f.capture());assert(!a.refresh(false).value&&f.calls==0);assert(!a.sample().value);
    auto first=a.refresh(true);assert(first.value&&first.value->capture_active&&first.value->selection_changed&&first.value->visible_processes==2&&f.starts==1&&f.fds.size()==2);
    assert(a.sample().value);const auto old=first.value->selection_epoch;const auto calls=f.calls;assert(!a.refresh(false).value&&f.calls==calls);
    auto same=a.refresh(true);assert(same.value&&!same.value->selection_changed&&same.value->selection_epoch==old&&f.updates==0&&f.starts==1&&f.samples==1);
    std::reverse(f.pids.begin(),f.pids.end());same=a.refresh(true);assert(same.value&&!same.value->selection_changed&&f.updates==0);
    f.pids={9};auto changed=a.refresh(true);assert(changed.value&&changed.value->selection_changed&&changed.value->selection_epoch!=old&&f.updates==1&&f.fds.size()==1);
    f.pids={8,9};assert(a.refresh(true).value&&f.updates==2&&f.fds.size()==2);
    f.starts_at[8]=999;assert(a.refresh(true).value&&f.updates==3&&f.fds.size()==2);
    f.pids.clear();auto empty=a.refresh(true);assert(empty.value&&!empty.value->capture_active&&empty.value->selection_changed&&!empty.value->selection_epoch&&empty.value->visible_processes==0);f.clean();
    assert(!a.sample().value);empty=a.refresh(true);assert(empty.value&&!empty.value->selection_changed);f.clean();
    f.pids={8};assert(a.refresh(true).value&&f.starts==2);a.stop();f.clean();assert(!a.sample().value);
}
void failures(){
    for(bool updating:{false,true}){
        int operations=0;{Fake f;LinuxNetworkAdmission a(f.inventory(),f.source(),f.capture());if(updating){assert(a.refresh(true).value);f.pids={9,10};}const auto start=f.calls;assert(a.refresh(true).value);operations=f.calls-start;a.stop();f.clean();}
        for(bool throwing:{false,true})for(int at=1;at<=operations;++at){Fake f;LinuxNetworkAdmission a(f.inventory(),f.source(),f.capture());if(updating){assert(a.refresh(true).value);f.pids={9,10};}
            if(throwing)f.throw_at=f.calls+at;else f.fail=f.calls+at;assert(!a.refresh(true).value);f.clean();assert(!a.sample().value);}
    }
    for(auto state:{LinuxNetworkInventoryState::permission_denied,LinuxNetworkInventoryState::disappeared,LinuxNetworkInventoryState::unavailable}){Fake f;LinuxNetworkAdmission a(f.inventory(),f.source(),f.capture());assert(a.refresh(true).value);f.state=state;
        auto r=a.refresh(true);assert(!r.value&&r.support==(state==LinuxNetworkInventoryState::permission_denied?Support::permission_required:Support::unsupported));f.clean();}
    for(unsigned fault=0;fault<6;++fault){Fake f;LinuxNetworkAdmission a(f.inventory(),f.source(),f.capture());assert(a.refresh(true).value);
        if(fault==0){f.pids.clear();for(unsigned i=1;i<=LinuxProcessNetworkSampler::max_processes+1;++i)f.pids.push_back(i);}if(fault==1)++f.context.boot[0];if(fault==2)++f.context.observer.user_namespace_inode;
        if(fault==3)f.caller=8;if(fault==4)f.drift=f.contexts+3;if(fault==5)f.stale=f.clocks+6;
        assert(!a.refresh(true).value);f.clean();}
    {Fake f;LinuxNetworkAdmission a(f.inventory(),f.source(),f.capture());f.pids.clear();const auto cap=LinuxProcessNetworkSampler::max_processes;for(unsigned i=1;i<=cap;++i)f.pids.push_back(i);assert(a.refresh(true).value->visible_processes==cap&&f.fds.size()==cap);a.stop();f.clean();}
    for(unsigned fault=0;fault<4;++fault){Fake f;LinuxNetworkAdmission a(f.inventory(),f.source(),f.capture());assert(a.refresh(true).value);
        if(fault==0)f.fail=f.calls+1;if(fault==1)f.throw_at=f.calls+1;if(fault==2)f.bad_sample=true;if(fault==3)f.caller=8;
        assert(!a.sample().value);f.clean();assert(!a.sample().value);}
    LinuxVisibleProcessNetworkSampler public_sampler;assert(!public_sampler.refresh_visible(false).value&&!public_sampler.sample().value);
    auto moved=std::move(public_sampler);assert(!public_sampler.sample().value&&!moved.sample().value);moved.stop();
}
void partial(){Fake f;LinuxNetworkAdmission a(f.inventory(),f.source(),f.capture());assert(a.refresh(true).value);assert(a.sample().value);
    const auto epoch=f.epoch;f.state=LinuxNetworkInventoryState::permission_denied;f.error_pid=9;
    auto changed=a.refresh(true,LinuxNetworkAdmissionPolicy::verified_only);assert(changed.value&&changed.value->capture_active&&!changed.value->complete_encountered_set&&changed.value->visible_processes==2&&changed.value->verified_processes==1&&changed.value->excluded.size()==1&&changed.value->excluded[0].pid==9&&f.fds.size()==1&&f.epoch!=epoch);
    auto w=a.sample_with_admission();assert(w.value&&w.support==Support::estimated&&w.value->admission.excluded[0].state==LinuxNetworkInventoryState::permission_denied&&w.value->admission.selection_epoch==w.value->network.selection_epoch);
    const auto held=f.epoch;f.state=LinuxNetworkInventoryState::disappeared;changed=a.refresh(true,LinuxNetworkAdmissionPolicy::verified_only);assert(changed.value&&!changed.value->selection_changed&&f.epoch==held&&changed.value->excluded[0].state==LinuxNetworkInventoryState::disappeared);
    assert(a.sample().note.find("PARTIAL")!=std::string::npos);assert(!a.refresh(false,LinuxNetworkAdmissionPolicy::strict_all).value&&f.active);
    assert(!a.refresh(true).value);f.clean(); // default never inherits partial policy
    f.error_pid=0;f.state=LinuxNetworkInventoryState::unavailable;changed=a.refresh(true,LinuxNetworkAdmissionPolicy::verified_only);assert(changed.value&&!changed.value->capture_active&&changed.value->visible_processes==2&&changed.value->verified_processes==0&&!changed.value->complete_encountered_set&&changed.value->excluded.size()==2);f.clean();assert(!a.sample_with_admission().value);
    f.state=LinuxNetworkInventoryState::verified;assert(a.refresh(true).value->complete_encountered_set);
    assert(!a.refresh(true,static_cast<LinuxNetworkAdmissionPolicy>(99)).value);f.clean();
    // A late runtime failure is NOT a reason to retry/filter a verified actor.
    f.state=LinuxNetworkInventoryState::permission_denied;f.error_pid=9;f.fail=f.calls+1;assert(!a.refresh(true,LinuxNetworkAdmissionPolicy::verified_only).value);f.clean();
}
}
int main(){success();failures();partial();}
