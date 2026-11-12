#include "windows_trace_identity.hpp"
#include <algorithm>
namespace pcinsight::detail {
namespace {
bool valid_id(std::uint32_t id){return id && id!=0xffffffffU;}
std::int64_t end_at(std::int64_t end,std::int64_t watermark){return end?end:watermark;}
bool valid_span(std::int64_t start,std::int64_t end,std::int64_t watermark){
    return start>0 && start<=watermark && (!end || (end>start && end<=watermark));
}
}
void WindowsTraceIdentity::reset(){
    threads_.clear();processes_.clear();size_=0;epoch_=0;watermark_=0;
    state_=TraceIdentityState::unavailable;
}
TraceIdentityState WindowsTraceIdentity::replace(std::uint64_t epoch,std::int64_t watermark,
    std::span<const TraceProcessLifetime> processes,std::span<const TraceThreadLifetime> threads,
    bool complete,std::size_t limit){
    const auto old_epoch=epoch_;const auto old_watermark=watermark_;
    reset(); // Even validation/allocation failures must not leave stale answers.
    if(!complete)return state_;
    state_=TraceIdentityState::invalid;
    if(!epoch || watermark<=0 || !limit || limit>100000 || processes.size()>limit ||
       threads.size()>limit-processes.size() || (epoch==old_epoch && watermark<old_watermark))return state_;
    decltype(processes_) new_processes;
    std::map<std::uint32_t,std::vector<TraceProcessLifetime>> by_pid;
    for(const auto& p:processes){
        if(!valid_id(p.pid) || !valid_span(p.start,p.end,watermark) ||
           !new_processes.emplace(std::make_pair(p.pid,p.start),p).second)return state_;
        by_pid[p.pid].push_back(p);
    }
    for(auto& [pid,lives]:by_pid){
        std::sort(lives.begin(),lives.end(),[](const auto& a,const auto& b){return a.start<b.start;});
        for(std::size_t i=1;i<lives.size();++i)
            if(lives[i].start<end_at(lives[i-1].end,watermark))return state_;
    }
    decltype(threads_) new_threads;
    for(const auto& t:threads){
        const auto process=new_processes.find({t.pid,t.process_start});
        if(!valid_id(t.tid) || !valid_id(t.pid) || !valid_span(t.start,t.end,watermark) ||
           process==new_processes.end())return state_;
        const auto& p=process->second;
        if(t.start<p.start || end_at(t.end,watermark)>end_at(p.end,watermark))return state_;
        new_threads[t.tid].push_back(t);
    }
    for(auto& [tid,lives]:new_threads){
        std::sort(lives.begin(),lives.end(),[](const auto& a,const auto& b){return a.start<b.start;});
        for(std::size_t i=1;i<lives.size();++i)
            if(lives[i].start==lives[i-1].start || lives[i].start<end_at(lives[i-1].end,watermark))return state_;
    }
    processes_=std::move(new_processes);threads_=std::move(new_threads);
    epoch_=epoch;watermark_=watermark;size_=processes.size()+threads.size();
    state_=TraceIdentityState::ready;return state_;
}
std::optional<TraceOwner> WindowsTraceIdentity::owner_at_initiation(std::uint32_t tid,std::int64_t issued) const {
    return owner_for_interval(tid,issued,issued);
}
std::optional<TraceOwner> WindowsTraceIdentity::owner_for_interval(std::uint32_t tid,
    std::int64_t issued,std::int64_t completed) const {
    if(state_!=TraceIdentityState::ready || !valid_id(tid) || issued<=0 || completed<issued || completed>=watermark_)return {};
    const auto found=threads_.find(tid);if(found==threads_.end())return {};
    const auto& lives=found->second;
    auto thread=std::upper_bound(lives.begin(),lives.end(),issued,
        [](std::int64_t qpc,const auto& t){return qpc<t.start;});
    if(thread==lives.begin())return {};
    --thread;
    if(issued<=thread->start || completed>=end_at(thread->end,watermark_))return {};
    const auto process=processes_.find({thread->pid,thread->process_start});
    if(process==processes_.end() || issued<=process->second.start || completed>=end_at(process->second.end,watermark_))return {};
    return TraceOwner{epoch_,thread->pid,tid,thread->process_start,thread->start};
}
}
