#include "../src/windows_disk_correlation.hpp"
#include <cassert>
#include <iostream>
#include <limits>
using namespace pcinsight::detail;
namespace {
WindowsTraceIdentity identities(std::uint64_t epoch=1,std::int64_t watermark=1000){
    WindowsTraceIdentity result;
    const TraceProcessLifetime p[]={{10,10,200},{10,300,0},{20,10,0}};
    const TraceThreadLifetime t[]={{7,10,10,20,190},{7,10,300,310,0},{8,20,10,20,0}};
    assert(result.replace(epoch,watermark,p,t,true)==TraceIdentityState::ready);return result;
}
DiskOperation init(std::uint64_t irp=123,std::int64_t qpc=100,std::uint32_t tid=7,bool write=false){return {irp,tid,qpc,true,write,0,0};}
DiskOperation done(std::uint64_t irp=123,std::int64_t qpc=150,std::uint32_t tid=7,bool write=false,std::uint32_t bytes=4096){return {irp,tid,qpc,false,write,0,bytes};}
DiskCorrelationBatch batch(WindowsDiskCorrelation& c,std::int64_t w,std::initializer_list<DiskOperation> e,const WindowsTraceIdentity& id,bool complete=true,std::uint64_t epoch=1){
    return c.sample(epoch,w,std::span<const DiskOperation>(e.begin(),e.size()),id,complete);
}
void seed(WindowsDiskCorrelation& c,const WindowsTraceIdentity& id){assert(batch(c,120,{init()},id).state==DiskCorrelationState::processed && c.pending()==1);}
}
int main(){
    auto id=identities();WindowsDiskCorrelation c(500);
    auto r=batch(c,180,{init(),done()},id);
    assert(r.state==DiskCorrelationState::processed && r.completions.size()==1 && !c.pending() && !r.unmatched);
    const auto& row=r.completions.front();assert((row.owner==TraceOwner{1,10,7,10,20}) && row.bytes==4096 && row.issued==100 && row.completed==150 && row.irp==123);
    c.reset();seed(c,id);
    // Original process/thread have exited and TID/PID now belong to generation300.
    // Completion350 keeps the identity captured at initiation100, NOT lookup350.
    r=batch(c,400,{done(123,350)},id);
    assert(r.completions.size()==1 && r.completions[0].owner.process_start==10);
    assert(id.owner_at_initiation(7,350)->process_start==300 && !c.pending());
    r=batch(c,500,{init(123,410),done(123,450)},id);
    assert(r.completions.size()==1 && r.completions[0].owner.process_start==300);
    c.reset();r=batch(c,420,{init(123,410)},id);
    r=batch(c,500,{done(123,150)},id); // Old duplicate arrives after key was reused.
    assert(r.state==DiskCorrelationState::invalid && r.completions.empty() && !c.pending());
    c.reset();r=batch(c,180,{done()},id);assert(r.unmatched==1 && r.completions.empty());
    c.reset();r=batch(c,180,{init(),done(),done(123,151)},id);assert(r.completions.size()==1 && r.unmatched==1);
    c.reset();r=batch(c,180,{init(123,100,999),done(123,150,999)},id);assert(r.completions.empty() && r.unmatched==1);
    c.reset();r=batch(c,180,{init(123,20),done(123,150)},id);assert(r.completions.empty() && r.unmatched==1); // Ambiguous birth boundary.
    c.reset();r=batch(c,180,{init(),done(123,150,7,false,0)},id);assert(r.completions.size()==1 && r.completions[0].bytes==0);
    c.reset();r=batch(c,180,{init(),done(123,150,7,false,UINT32_MAX)},id);assert(r.completions[0].bytes==UINT32_MAX);
    for(int mode=0;mode<11;++mode){
        c.reset();seed(c,id);auto event=done();
        switch(mode){case 0:event.irp=0;break;case 1:event.irp=UINT64_MAX;break;case 2:event.issuing_thread=0;break;
            case 3:event.issuing_thread=UINT32_MAX;break;case 4:event.issuing_thread=8;break;case 5:event.write=true;break;
            case 6:event.qpc=0;break;case 7:event.qpc=119;break;case 8:event.qpc=180;break;
            case 9:event.disk=UINT32_MAX;break;case 10:event=init(123,130);break;}
        r=batch(c,180,{event},id);assert(r.state==DiskCorrelationState::invalid && r.completions.empty() && !c.pending());
    }
    c.reset();r=batch(c,180,{init(),done(123,100)},id);assert(r.state==DiskCorrelationState::invalid);
    c.reset();r=batch(c,180,{init(),done(),init(456,140,8)},id);assert(r.state==DiskCorrelationState::invalid && r.completions.empty());
    // Failure after a valid pair invalidates the whole batch; never publish prefix.
    c.reset();r=batch(c,180,{init(),done(),init(456,160,8),init(456,170,8)},id);
    assert(r.state==DiskCorrelationState::invalid && r.completions.empty() && !c.pending());
    c.reset();seed(c,id);r=batch(c,180,{},id,false);assert(r.state==DiskCorrelationState::unavailable && !c.pending());
    r=batch(c,200,{done(123,190)},id);assert(r.completions.empty() && r.unmatched==1);
    c.reset();seed(c,id);auto unknown=WindowsTraceIdentity{};
    r=batch(c,180,{},unknown);assert(r.state==DiskCorrelationState::unavailable && !c.pending());
    c.reset();seed(c,id);r=batch(c,180,{},id,true,2);assert(r.state==DiskCorrelationState::unavailable && !c.pending());
    c.reset();seed(c,id);auto ahead=identities(2);
    r=batch(c,180,{done()},ahead,true,2);assert(r.state==DiskCorrelationState::processed && r.completions.empty() && r.unmatched==1);
    c.reset();seed(c,id);r=batch(c,1001,{},id);assert(r.state==DiskCorrelationState::unavailable && !c.pending());
    c.reset();seed(c,id);r=batch(c,120,{},id);assert(r.state==DiskCorrelationState::invalid && !c.pending());
    c.reset();seed(c,id);r=batch(c,119,{},id);assert(r.state==DiskCorrelationState::invalid && !c.pending());
    // Empty batches are not zero-activity measurements; they can expire pending state.
    WindowsDiskCorrelation short_lived(50);r=batch(short_lived,120,{init()},id);
    r=batch(short_lived,151,{},id);assert(r.discarded==1 && !short_lived.pending());
    r=batch(short_lived,180,{done(123,160)},id);assert(r.unmatched==1 && r.completions.empty());
    WindowsDiskCorrelation age50(50);r=batch(age50,200,{init(),done(123,151)},id);assert(r.unmatched==1 && r.completions.empty());
    WindowsDiskCorrelation age_exact(50);r=batch(age_exact,200,{init(),done()},id);assert(r.completions.size()==1);
    // Do not expire by final watermark BEFORE processing an earlier valid completion.
    WindowsDiskCorrelation within_batch(60);r=batch(within_batch,900,{init(),done()},id);assert(r.completions.size()==1);
    for(auto age:{std::int64_t(0),std::int64_t(-1)}){WindowsDiskCorrelation bad(age);assert(batch(bad,180,{},id).state==DiskCorrelationState::invalid);}
    for(auto limit:{std::size_t(0),std::size_t(100001)}){WindowsDiskCorrelation bad(500,limit);assert(batch(bad,180,{},id).state==DiskCorrelationState::invalid);}
    WindowsDiskCorrelation limited(500,1);r=batch(limited,180,{init(),init(456,110,8)},id);assert(r.state==DiskCorrelationState::invalid && !limited.pending());
    r=batch(limited,180,{init(),done(),init(456,160,8),done(456,170,8)},id);assert(r.completions.size()==2 && !limited.pending());
    // A completion exactly at a new batch's inclusive lower boundary is accepted.
    c.reset();seed(c,id);r=batch(c,180,{done(123,120)},id);assert(r.completions.size()==1);
    // Saturated ticks without signed duration overflow.
    const auto maximum=std::numeric_limits<std::int64_t>::max();
    auto extreme_identity=identities(9,maximum);WindowsDiskCorrelation large(maximum);
    const DiskOperation extreme[]={init(1,maximum-3),done(1,maximum-2)};
    r=large.sample(9,maximum,extreme,extreme_identity,true);assert(r.completions.size()==1 && r.completions[0].owner.epoch==9);
    // Deterministic stream, multiple IRP reuses and interleaved issuing threads.
    c.reset();std::vector<DiskOperation> stream;std::uint64_t expected_bytes=0;
    auto long_id=identities(1,50000);
    for(unsigned i=0;i<10000;++i){const auto stamp=1000+static_cast<std::int64_t>(i)*3;
        const auto bytes=static_cast<std::uint32_t>(i*17);const bool write=bool(i&1);const auto tid=i%2?7U:8U;
        stream.push_back(init(i%13+1,stamp,tid,write));stream.push_back(done(i%13+1,stamp+1,tid,write,bytes));expected_bytes+=bytes;
    }
    WindowsDiskCorrelation stress(50000);r=stress.sample(1,40000,stream,long_id,true);
    assert(r.state==DiskCorrelationState::processed && r.completions.size()==10000 && !r.unmatched && !stress.pending());
    std::uint64_t sum=0;for(const auto& item:r.completions){sum+=item.bytes;assert(item.owner.pid==(item.owner.tid==7?10U:20U));}
    assert(sum==expected_bytes);
    // Explicit pending/input hard limits (no source activation).
    std::vector<DiskOperation> full;full.reserve(100001);
    for(std::uint64_t i=1;i<=100000;++i)full.push_back(init(i,1000));
    WindowsDiskCorrelation bounded(50000);r=bounded.sample(1,2000,full,long_id,true);
    assert(r.state==DiskCorrelationState::processed && bounded.pending()==100000);
    full.push_back(init(100001,1001));r=bounded.sample(1,3000,full,long_id,true);
    assert(r.state==DiskCorrelationState::invalid && !bounded.pending());
    std::cout<<"Disk IRP correlation: initiation identity, reuse, loss/reset, bounded state and10000 pairs passed\n";
}
