#include "pcinsight/startup_birth_impact.hpp"
#include <limits>
namespace pcinsight {
namespace {
bool text(const std::string& s,std::size_t limit,bool empty=false){return (empty||!s.empty())&&s.size()<=limit&&s.find('\0')==s.npos;}
template<class T> bool shape(const Metric<T>& m){return text(m.note,4096,true)&&static_cast<unsigned>(m.support)<=static_cast<unsigned>(Support::estimated)&&
    (m.value?(m.support==Support::available||m.support==Support::estimated):(m.support==Support::unsupported||m.support==Support::permission_required));}
template<class T> Metric<T> absent(const char* note,Support support=Support::unsupported){return {{},support,note};}
}
StartupBirthImpactResult measure_startup_birth_impact(const StartupBirthObserverReport& report,
    const StartupBirthProcessKey& key,std::chrono::nanoseconds window,const StartupImpactPolicy& policy){
    const auto invalid=[](const char* note){return StartupBirthImpactResult{{},ActionError::invalid_argument,note};};
    if(!text(key.capture_id,128)||!(key.boot[0]||key.boot[1])||!key.kernel_tgid||key.kernel_tgid>2147483647U||!key.kernel_start_boottime_ns||
       !text(report.capture_id,128)||report.observations.size()>64||report.quality_flags&~127ULL||report.publishing>64||
       static_cast<unsigned>(report.state)>static_cast<unsigned>(StartupBirthObserverState::failed)||report.state==StartupBirthObserverState::idle||
       window.count()<=0||window>std::chrono::hours(1)||policy.medium_cpu.count()<=0||policy.high_cpu<policy.medium_cpu||
       !policy.medium_disk_bytes||policy.high_disk_bytes<policy.medium_disk_bytes)
        return invalid("Invalid birth impact key/report/window/policy; no fabricated legacy identity or clock");
    if(key.capture_id!=report.capture_id||key.boot!=report.boot)
        return {{},ActionError::stale_target,"Exact capture/boot child key required"};
    const StartupBirthObservation* row=nullptr;
    for(const auto& item:report.observations)if(item.key==key){
        if(row)return invalid("Duplicate retained child key; no ambiguous impact projection");
        row=&item;
    }
    if(!row)return {{},ActionError::stale_target,"Exact retained child key not found; no PID-only matching"};
    if(!row->birth_monotonic_ns||!shape(row->startup_entry_id)||!shape(row->cpu_time)||!shape(row->read_bytes)||!shape(row->write_bytes)||
       (row->cpu_time.value&&row->cpu_time.value->count()<0)||
       (row->final_monotonic_ns&&(!report.final_resources_enabled||*row->final_monotonic_ns<row->birth_monotonic_ns))||
       (!row->final_monotonic_ns&&(row->cpu_time.value||row->read_bytes.value||row->write_bytes.value)))
        return invalid("Malformed birth/binding/release/counters; no impact projection");
    if(!row->startup_entry_id.value)return {{},ActionError::not_found,"Explicit retained startup-entry binding required; candidates are not bindings"};
    if(!text(*row->startup_entry_id.value,32768))return invalid("Invalid explicit startup-entry binding");
    const auto span=static_cast<std::uint64_t>(window.count());
    if(row->birth_monotonic_ns>std::numeric_limits<std::uint64_t>::max()-span)return invalid("Birth-relative deadline overflow");
    StartupBirthImpactMeasurement m;m.key=key;
    m.startup_entry_id={*row->startup_entry_id.value,Support::estimated,"Caller-declared retained binding; not executable/app/automatic-startup cause proof"};
    m.begin_monotonic_ns=row->birth_monotonic_ns;m.deadline_monotonic_ns=m.begin_monotonic_ns+span;
    m.release_monotonic_ns=row->final_monotonic_ns;
    m.automatic_startup_origin=absent<bool>("Birth/caller binding/resource thresholds do not prove automatic-startup origin");
    m.impact=absent<StartupImpact>("Incomplete release-observed coverage cannot certify low/medium startup impact");
    m.cpu_time=absent<std::chrono::nanoseconds>("No release within the birth-relative window; CPU is not clipped or prorated");
    m.disk_bytes=absent<std::uint64_t>("No release within the birth-relative window; disk is not clipped or prorated");
    if(row->final_monotonic_ns&&*row->final_monotonic_ns<=m.deadline_monotonic_ns){
        m.released_within_window=true;
        if(row->cpu_time.value)m.cpu_time={*row->cpu_time.value,Support::estimated,"Release-observed group scheduler CPU lower bound within birth-relative window; no waited-child CPU/post-switch finality guarantee"};
        else m.cpu_time=absent<std::chrono::nanoseconds>("Release-observed CPU unavailable; not zero",row->cpu_time.support);
        if(row->read_bytes.value&&row->write_bytes.value){
            if(*row->write_bytes.value<=std::numeric_limits<std::uint64_t>::max()-*row->read_bytes.value)
                m.disk_bytes={*row->read_bytes.value+*row->write_bytes.value,Support::estimated,"Observed group-accounted read+write bytes within window; includes waited-child IO, never sum overlapping process trees"};
            else m.disk_bytes=absent<std::uint64_t>("Read+write uint64 overflow; no wrapped/saturated disk value");
        }else m.disk_bytes=absent<std::uint64_t>("Read and/or write unavailable; no partial sum or zero",row->read_bytes.support==Support::permission_required||row->write_bytes.support==Support::permission_required?Support::permission_required:Support::unsupported);
        // Same strict high thresholds as measure_startup_impact. A high observed
        // lower bound survives an unknown other resource; never infer low/medium.
        if((m.cpu_time.value&&*m.cpu_time.value>policy.high_cpu)||(m.disk_bytes.value&&*m.disk_bytes.value>policy.high_disk_bytes))
            m.impact={StartupImpact::high,Support::estimated,"High observed resource lower bound only; declared association, not complete startup coverage"};
        else if(m.cpu_time.support==Support::permission_required||m.disk_bytes.support==Support::permission_required)
            m.impact.support=Support::permission_required;
    }
    return {std::move(m),ActionError::none,"Single retained child projection; no legacy-clock conversion, tree aggregation, persistence or automatic attribution"};
}
}
