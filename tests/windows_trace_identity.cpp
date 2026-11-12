#include "../src/windows_trace_identity.hpp"
#include <algorithm>
#include <cassert>
#include <iostream>
#include <limits>
using namespace pcinsight::detail;
namespace {
const std::vector<TraceProcessLifetime> processes{{10,100,250},{10,300,0},{20,100,0}};
const std::vector<TraceThreadLifetime> threads{{7,10,100,110,240},{7,10,300,310,0},{8,20,100,120,0}};
void good(WindowsTraceIdentity& index){assert(index.replace(1,500,processes,threads,true)==TraceIdentityState::ready);}
void invalid(std::vector<TraceProcessLifetime> p,std::vector<TraceThreadLifetime> t,
    std::uint64_t epoch=1,std::int64_t watermark=500,std::size_t limit=100000){
    WindowsTraceIdentity index;good(index);assert(index.owner_at_initiation(7,150));
    assert(index.replace(epoch,watermark,p,t,true,limit)==TraceIdentityState::invalid);
    assert(index.size()==0 && !index.owner_at_initiation(7,150));
    good(index);assert(index.owner_at_initiation(7,150));
}
std::optional<TraceOwner> oracle(std::uint64_t epoch,std::int64_t watermark,
    const std::vector<TraceProcessLifetime>& p,const std::vector<TraceThreadLifetime>& t,
    std::uint32_t tid,std::int64_t begin,std::int64_t end){
    if(begin<=0 || end<begin || end>=watermark)return {};
    for(const auto& thread:t){
        if(thread.tid!=tid || begin<=thread.start || end>=(thread.end?thread.end:watermark))continue;
        for(const auto& process:p)
            if(process.pid==thread.pid && process.start==thread.process_start &&
               begin>process.start && end<(process.end?process.end:watermark))
                return TraceOwner{epoch,thread.pid,tid,process.start,thread.start};
    }
    return {};
}
}
int main(){
    WindowsTraceIdentity index;
    assert(index.state()==TraceIdentityState::unavailable && !index.owner_at_initiation(7,150));
    good(index);assert(index.size()==6);
    const TraceOwner old{1,10,7,100,110},reused{1,10,7,300,310},other{1,20,8,100,120};
    assert(index.owner_at_initiation(7,150)==old);
    assert(index.owner_at_initiation(7,350)==reused);
    assert(index.owner_at_initiation(8,350)==other);
    for(auto qpc:{-1,0,100,110,240,250,280,300,310,500,501})assert(!index.owner_at_initiation(7,qpc));
    for(auto tid:{0U,9U,0xffffffffU})assert(!index.owner_at_initiation(tid,150));
    assert(index.owner_for_interval(7,150,239)==old);
    assert(!index.owner_for_interval(7,150,240));
    assert(!index.owner_for_interval(7,150,350)); // A late completion MUST NOT bind to reused.
    assert(!index.owner_for_interval(7,350,349));
    // At initiation150 the old identity can be retained by a future IRP tracker;
    // querying at completion350 instead would select reused, a different owner.
    assert(index.owner_at_initiation(7,150)!=index.owner_at_initiation(7,350));
    auto p=processes;auto t=threads;
    std::reverse(p.begin(),p.end());std::reverse(t.begin(),t.end());
    assert(index.replace(1,500,p,t,true)==TraceIdentityState::ready && index.owner_at_initiation(7,150)==old);
    assert(index.replace(1,500,p,t,false)==TraceIdentityState::unavailable && !index.owner_at_initiation(7,150) && index.size()==0);
    good(index);index.reset();assert(!index.owner_at_initiation(7,150) && !index.size());
    // Epoch is part of identity, never a Windows FILETIME/action token.
    assert(index.replace(2,500,processes,threads,true)==TraceIdentityState::ready);
    assert(index.owner_at_initiation(7,150)->epoch==2 && *index.owner_at_initiation(7,150)!=old);
    invalid(processes,threads,0);invalid(processes,threads,1,0);
    invalid(processes,threads,1,-1);invalid(processes,threads,1,499); // Same-epoch watermark regression.
    invalid(processes,threads,1,500,0);invalid(processes,threads,1,500,100001);
    invalid(processes,threads,1,500,5);
    for(int mode=0;mode<8;++mode){p=processes;
        switch(mode){case 0:p[0].pid=0;break;case 1:p[0].pid=0xffffffffU;break;
            case 2:p[0].start=0;break;case 3:p[0].end=p[0].start;break;
            case 4:p[0].end=501;break;case 5:p[0].end=350;break;
            case 6:p.push_back(p[0]);break;case 7:p[1].start=501;break;}
        invalid(p,threads);
    }
    for(int mode=0;mode<11;++mode){t=threads;
        switch(mode){case 0:t[0].tid=0;break;case 1:t[0].tid=0xffffffffU;break;
            case 2:t[0].pid=11;break;case 3:t[0].process_start=101;break;
            case 4:t[0].start=99;break;case 5:t[0].end=251;break;
            case 6:t[0].end=0;break;case 7:t[0].end=t[0].start;break;
            case 8:t.push_back(t[0]);break;case 9:t[1]={7,20,100,200,0};break;
            case 10:t[0].end=-1;break;}
        invalid(processes,t);
    }
    // Coincident exit/start timestamps are accepted as records, not resolved
    // at the ambiguous boundary. Independent different TIDs may overlap.
    p={{10,100,200},{10,200,0}};t={{7,10,100,110,200},{7,10,200,200,0}};
    assert(index.replace(3,500,p,t,true)==TraceIdentityState::ready);
    assert(!index.owner_at_initiation(7,200) && index.owner_at_initiation(7,199)->process_start==100 && index.owner_at_initiation(7,201)->process_start==200);
    assert(index.replace(4,500,{}, {},true)==TraceIdentityState::ready && !index.owner_at_initiation(7,201));
    const auto maximum=std::numeric_limits<std::int64_t>::max();
    p={{10,1,0}};t={{7,10,1,2,0}};
    assert(index.replace(5,maximum,p,t,true)==TraceIdentityState::ready);
    assert(index.owner_at_initiation(7,maximum-1) && !index.owner_at_initiation(7,maximum));
    // Independent linear oracle vs binary-search index across many generations.
    p.clear();t.clear();
    for(std::uint32_t pid=1;pid<=100;++pid){
        p.push_back({pid,100,300});p.push_back({pid,400,700});
        t.push_back({pid+1000,pid,100,110,290});t.push_back({pid+1000,pid,400,410,690});
    }
    assert(index.replace(6,800,p,t,true)==TraceIdentityState::ready);
    std::uint32_t seed=13;
    for(unsigned i=0;i<20000;++i){seed=seed*1664525U+1013904223U;
        const auto tid=1000+seed%103;const auto begin=static_cast<std::int64_t>((seed>>8)%850);
        const auto end=begin+static_cast<std::int64_t>((seed>>20)%100);
        assert(index.owner_for_interval(tid,begin,end)==oracle(6,800,p,t,tid,begin,end));
    }
    // Hard total-record limit, exact and exceeded; no unbounded state growth.
    p.clear();t.clear();p.reserve(50000);t.reserve(50000);
    for(std::uint32_t id=1;id<=50000;++id){p.push_back({id,1,0});t.push_back({id,id,1,2,0});}
    assert(index.replace(7,10,p,t,true)==TraceIdentityState::ready && index.size()==100000);
    t.push_back({60000,1,1,2,0});
    assert(index.replace(7,10,p,t,true)==TraceIdentityState::invalid && !index.size());
    std::cout<<"Temporal trace identity: PID/TID reuse, boundaries, coverage, limits and20000-case oracle passed\n";
}
