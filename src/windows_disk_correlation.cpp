#include "windows_disk_correlation.hpp"
namespace pcinsight::detail {
void WindowsDiskCorrelation::reset(){pending_.clear();epoch_=0;watermark_=0;}
DiskCorrelationBatch WindowsDiskCorrelation::sample(std::uint64_t epoch,std::int64_t watermark,
    std::span<const DiskOperation> events,const WindowsTraceIdentity& identities,bool complete){
    DiskCorrelationBatch result;
    if(!complete || identities.state()!=TraceIdentityState::ready || identities.epoch()!=epoch || identities.watermark()<watermark){reset();return result;}
    auto invalid=[&]{reset();return DiskCorrelationBatch{DiskCorrelationState::invalid,{},0,0};};
    if(!epoch || watermark<=0 || max_age_<=0 || !limit_ || limit_>100000 || events.size()>100000)return invalid();
    if(epoch_ && epoch_!=epoch)reset();
    if(watermark<=watermark_)return invalid();
    try {
        auto staged=pending_;
        auto previous=watermark_;
        for(const auto& event:events){
            if(!event.irp || event.irp==UINT64_MAX || !event.issuing_thread || event.issuing_thread==UINT32_MAX ||
               event.qpc<=0 || event.qpc<previous || event.qpc>=watermark || (!event.initiation && event.disk==UINT32_MAX))return invalid();
            previous=event.qpc;
            auto found=staged.find(event.irp);
            if(event.initiation){
                if(found!=staged.end())return invalid(); // Never overwrite an ambiguous outstanding IRP.
                if(staged.size()>=limit_)return invalid();
                staged.emplace(event.irp,Pending{event,identities.owner_at_initiation(event.issuing_thread,event.qpc)});
            }else{
                if(found==staged.end()){++result.unmatched;continue;}
                const auto& pending=found->second;
                if(pending.init.issuing_thread!=event.issuing_thread || pending.init.write!=event.write ||
                   event.qpc<=pending.init.qpc)return invalid();
                if(event.qpc-pending.init.qpc>max_age_ || !pending.owner){++result.unmatched;}
                else result.completions.push_back({*pending.owner,event.irp,pending.init.qpc,event.qpc,event.disk,event.bytes,event.write});
                staged.erase(found); // Exactly one positive pair per outstanding operation.
            }
        }
        for(auto it=staged.begin();it!=staged.end();){
            if(watermark-it->second.init.qpc>max_age_){it=staged.erase(it);++result.discarded;}
            else ++it;
        }
        pending_=std::move(staged);epoch_=epoch;watermark_=watermark;
        result.state=DiskCorrelationState::processed;return result;
    }catch(...){reset();throw;}
}
}
