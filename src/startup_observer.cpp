#include "pcinsight/startup_observer.hpp"
#include <limits>
#include <mutex>
#include <stdexcept>
namespace pcinsight {
namespace {
constexpr std::size_t count_limit=4096,payload_limit=8*1024*1024;
bool valid(const StartupProcessSample& s) {
    return static_cast<unsigned>(s.cpu_counter)<=static_cast<unsigned>(StartupCpuCounter::linux_scheduler_runtime)&&s.pid && s.linux_start_ticks && !s.native_context.empty() && s.native_context.size()<=1024 &&
        s.native_context.find('\0')==s.native_context.npos && s.cumulative_cpu_time.note.size()<=4096 &&
        s.cumulative_disk_bytes.note.size()<=4096 && (!s.cumulative_cpu_time.value || s.cumulative_cpu_time.value->count()>=0);
}
std::size_t bytes(const StartupProcessSample& s) {return s.native_context.size()+s.cumulative_cpu_time.note.size()+s.cumulative_disk_bytes.note.size();}
template<class T> bool known(const Metric<T>& m){return m.value && (m.support==Support::available || m.support==Support::estimated);}
}
struct StartupImpactObserver::Impl {
    struct Record {
        std::string id;StartupProcessSample baseline,last;std::size_t samples{1};
        bool closed{},gap{};ActionError error{ActionError::none};bool partial_final{};
        std::size_t payload() const {return id.size()+bytes(baseline)+bytes(last);}
    };
    std::chrono::steady_clock::duration window;
    StartupImpactPolicy policy;StartupImpactSampleReader reader;
    mutable std::mutex mutex;std::vector<Record> records;std::size_t payload{};bool closed{};
    Impl(std::chrono::milliseconds duration,StartupImpactPolicy p,StartupImpactSampleReader r)
        :policy(p),reader(std::move(r)) {
        if(duration.count()<=0 || duration>std::chrono::hours(1) || !reader ||
            policy.medium_cpu.count()<=0 || policy.high_cpu<policy.medium_cpu ||
            !policy.medium_disk_bytes || policy.high_disk_bytes<policy.medium_disk_bytes)
            throw std::invalid_argument("invalid startup observation window/policy/reader");
        window=duration; // convert only AFTER rejecting excessive millisecond counts
    }
    bool complete(const Record& r) const {
        return r.last.sampled_at>r.baseline.sampled_at &&
            r.last.sampled_at-r.baseline.sampled_at>=window && !r.gap;
    }
    void fail(Record& r,ActionError error) {
        r.error=error;r.gap=true;
        if(error==ActionError::not_found || error==ActionError::stale_target)r.closed=true;
    }
    StartupImpactPollResult poll(std::optional<std::chrono::steady_clock::time_point> now) {
        std::lock_guard lock(mutex);StartupImpactPollResult result;
        if(closed)return result;
        for(auto& r:records)if(!r.closed) {
            ++result.attempted;
            try {
                const auto sample=reader(r.baseline.pid,r.baseline.linux_start_ticks);
                if(!sample){fail(r,sample.error==ActionError::none?ActionError::system_error:sample.error);++result.failed;}
                else {
                    const auto updated=update(r,*sample.value);
                    if(updated)++result.updated;
                    else {fail(r,updated.error);++result.failed;}
                }
            } catch(...) {fail(r,ActionError::system_error);++result.failed;}
            // Baseline + window is overflow-checked by register_process. Compare
            // timepoints instead of subtracting arbitrary injected timestamps.
            // Native clock is read AFTER I/O, including exceptions, so a slow
            // failing read cannot keep its record open past the window.
            const auto completed_at=now?*now:std::chrono::steady_clock::now();
            if(completed_at>=r.baseline.sampled_at+window)r.closed=true;
        }
        return result;
    }
    ActionResult update(Record& r,const StartupProcessSample& s) {
        if(r.closed)return {ActionError::invalid_argument,"observation already closed"};
        if(r.samples==std::numeric_limits<std::size_t>::max())return {ActionError::invalid_argument,"startup sample counter exhausted"};
        if(!valid(s))return {ActionError::invalid_argument,"invalid/excessive startup sample"};
        auto checked=measure_startup_impact(r.id,r.last,s,policy);
        if(!checked) {if(checked.error==ActionError::stale_target)fail(r,checked.error);return {checked.error,checked.message};}
        auto total=measure_startup_impact(r.id,r.baseline,s,policy);
        if(!total)return {total.error,total.message}; // includes interval overflow
        const auto next=bytes(s),old=bytes(r.last);
        if(next>payload_limit-(payload-old))return {ActionError::invalid_argument,"startup observation payload limit"};
        // Validate cumulative monotonicity at every step, not just endpoints.
        const auto reset=[](const auto& a,const auto& b){return a.value && b.value && *b.value<*a.value;};
        const bool bad=reset(r.last.cumulative_cpu_time,s.cumulative_cpu_time) || reset(r.last.cumulative_disk_bytes,s.cumulative_disk_bytes);
        auto replacement=s; // copy before commit; throwing reader/copy cannot corrupt retained sample
        r.last=std::move(replacement);payload=payload-old+next;++r.samples;
        r.error=ActionError::none;r.gap|=bad || !known(s.cumulative_cpu_time) || !known(s.cumulative_disk_bytes);
        if(s.sampled_at-r.baseline.sampled_at>=window)r.closed=true;
        return {ActionError::none,"startup process observation updated; association remains caller-supplied"};
    }
};
StartupImpactObserver::StartupImpactObserver(std::chrono::milliseconds window,StartupImpactPolicy policy,StartupImpactSampleReader reader)
    :impl_(std::make_unique<Impl>(window,policy,std::move(reader))) {}
StartupImpactObserver::~StartupImpactObserver()=default;
ActionResult StartupImpactObserver::register_process(const std::string& id,const StartupProcessSample& s) {
    std::lock_guard lock(impl_->mutex);
    if(impl_->closed)return {ActionError::invalid_argument,"startup observation run is closed"};
    if(id.empty() || id.size()>32768 || id.find('\0')!=id.npos || !valid(s) ||
        s.sampled_at.time_since_epoch().count()>std::numeric_limits<std::chrono::steady_clock::duration::rep>::max()-impl_->window.count())
        return {ActionError::invalid_argument,"invalid startup association/baseline"};
    for(const auto& r:impl_->records)if(r.baseline.pid==s.pid && r.baseline.linux_start_ticks==s.linux_start_ticks && r.baseline.native_context==s.native_context)
        return {r.id==id?ActionError::invalid_argument:ActionError::stale_target,"process identity already attributed; no double count"};
    const auto added=id.size()+2*bytes(s);
    if(impl_->records.size()>=count_limit || added>payload_limit-impl_->payload)
        return {ActionError::invalid_argument,"startup observation count/payload limit"};
    impl_->records.push_back({id,s,s});impl_->payload+=added;
    impl_->records.back().gap=!known(s.cumulative_cpu_time) || !known(s.cumulative_disk_bytes);
    return {ActionError::none,"startup process baseline registered explicitly"};
}
ActionResult StartupImpactObserver::observe_sample(const StartupProcessSample& s) {
    std::lock_guard lock(impl_->mutex);
    if(impl_->closed)return {ActionError::invalid_argument,"startup observation run is closed"};
    for(auto& r:impl_->records)if(r.baseline.pid==s.pid && r.baseline.linux_start_ticks==s.linux_start_ticks && r.baseline.native_context==s.native_context)
        return impl_->update(r,s);
    return {ActionError::stale_target,"sample identity not registered; no attribution"};
}
StartupImpactPollResult StartupImpactObserver::poll() {
    return impl_->poll(std::nullopt);
}
ActionResult StartupImpactObserver::finish_process(const StartupProcessSample& s) {
    std::lock_guard lock(impl_->mutex);
    if(impl_->closed)return {ActionError::invalid_argument,"startup observation run is closed"};
    for(auto& r:impl_->records)if(r.baseline.pid==s.pid && r.baseline.linux_start_ticks==s.linux_start_ticks && r.baseline.native_context==s.native_context) {
        auto result=impl_->update(r,s);
        if(result)r.closed=true;
        return result;
    }
    return {ActionError::stale_target,"final sample identity not registered; no attribution"};
}
StartupImpactPollResult StartupImpactObserver::poll(std::chrono::steady_clock::time_point now) {
    return impl_->poll(now);
}
ActionResult StartupImpactObserver::finish_partial_process(const StartupProcessSample& s) {
    std::lock_guard lock(impl_->mutex);
    if(impl_->closed)return {ActionError::invalid_argument,"startup observation run is closed"};
    for(auto& r:impl_->records)if(r.baseline.pid==s.pid && r.baseline.linux_start_ticks==s.linux_start_ticks && r.baseline.native_context==s.native_context) {
        auto result=impl_->update(r,s);
        if(result){r.gap=true;r.closed=true;r.partial_final=true;}
        return result;
    }
    return {ActionError::stale_target,"partial final sample identity not registered; no attribution"};
}
void StartupImpactObserver::close() {
    std::lock_guard lock(impl_->mutex);impl_->closed=true;
    for(auto& r:impl_->records)r.closed=true;
}
std::vector<StartupImpactObservation> StartupImpactObserver::observations() const {
    std::lock_guard lock(impl_->mutex);std::vector<StartupImpactObservation> out;out.reserve(impl_->records.size());
    for(const auto& r:impl_->records) {
        StartupImpactObservation row{r.id,r.baseline.pid,r.baseline.linux_start_ticks,std::nullopt,r.samples,r.closed,impl_->complete(r),r.gap,r.error};
        row.native_context=r.baseline.native_context;
        if(r.samples>1) {
            auto measured=measure_startup_impact(r.id,r.baseline,r.last,impl_->policy);
            if(measured) {
                row.measurement=std::move(measured.value);
                if(r.partial_final){
                    row.measurement->cpu_time.note+="; partial final: "+r.last.cumulative_cpu_time.note.substr(0,1024);
                    row.measurement->disk_bytes.note+="; partial final: "+r.last.cumulative_disk_bytes.note.substr(0,1024);
                }
                if(!row.window_complete && row.measurement->impact.value!=StartupImpact::high)
                    row.measurement->impact={std::nullopt,(r.error==ActionError::permission_denied || row.measurement->impact.support==Support::permission_required)?Support::permission_required:Support::unsupported,
                        "observation window incomplete/gapped; low/medium not certified"};
                else if(!row.window_complete) row.measurement->impact.note="high observed lower bound despite incomplete/gapped window";
            }
        }
        out.push_back(std::move(row));
    }
    return out;
}
}
