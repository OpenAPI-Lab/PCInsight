#include "pcinsight/startup_overlay.hpp"
#include <map>
#include <set>
#include <tuple>
#include <type_traits>
namespace pcinsight {
namespace {
template<class T> bool known(const Metric<T>& m){return m.value && (m.support==Support::available || m.support==Support::estimated);}
template<class T> bool bounded(const Metric<T>& m){return m.note.size()<=4096;}
bool id_valid(const std::string& id){return !id.empty() && id.size()<=32768 && id.find('\0')==id.npos;}
struct Patch {
    std::size_t index;Metric<StartupImpact> impact;
    Metric<std::chrono::nanoseconds> cpu;Metric<std::uint64_t> disk;
    Metric<std::chrono::steady_clock::duration> interval;std::string scope;
};
}
StartupImpactOverlayResult apply_startup_impact_observations(SystemSnapshot& snapshot,
    const std::vector<StartupImpactObservation>& observations,const std::string& context) {
    auto fail=[](ActionError error,const char* message){return StartupImpactOverlayResult{error,0,0,message};};
    if(context.empty() || context.size()>1024 || context.find('\0')!=context.npos ||
        observations.size()>4096 || snapshot.startup_entries.size()>100000)
        return fail(ActionError::invalid_argument,"invalid/excessive overlay context or count");
    std::size_t payload=0;
    auto add=[&](std::size_t bytes){if(bytes>8*1024*1024-payload)return false;payload+=bytes;return true;};
    std::map<std::string,std::size_t> entries;
    for(std::size_t i=0;i<snapshot.startup_entries.size();++i){
        const auto& id=snapshot.startup_entries[i].id;
        if(!id_valid(id) || !add(id.size()) || !entries.emplace(id,i).second)
            return fail(ActionError::invalid_argument,"invalid/duplicate overlay startup ID or payload limit");
    }
    std::set<std::tuple<std::uint32_t,std::uint64_t>> identities;
    std::map<std::size_t,std::vector<const StartupImpactObservation*>> grouped;
    StartupImpactOverlayResult result;
    for(const auto& row:observations) {
        if(!id_valid(row.startup_entry_id) || !row.pid || !row.linux_start_ticks || !row.sample_count ||
            row.native_context.empty() || row.native_context.size()>1024 || row.native_context.find('\0')!=row.native_context.npos ||
            !add(row.startup_entry_id.size()+row.native_context.size()) ||
            !identities.emplace(row.pid,row.linux_start_ticks).second)
            return fail(ActionError::invalid_argument,"invalid/duplicate overlay process observation");
        if(row.native_context!=context)return fail(ActionError::stale_target,"overlay observations belong to another native context");
        if(row.window_complete && (!row.closed || row.sampling_gap || !row.measurement))
            return fail(ActionError::invalid_argument,"inconsistent overlay window completion");
        if(row.measurement) {
            const auto& m=*row.measurement;
            if(m.startup_entry_id!=row.startup_entry_id || m.pid!=row.pid || m.linux_start_ticks!=row.linux_start_ticks ||
                row.sample_count<2 || m.observation_interval<=std::chrono::steady_clock::duration::zero() ||
                (m.cpu_time.value && m.cpu_time.value->count()<0) || !bounded(m.cpu_time) || !bounded(m.disk_bytes) || !bounded(m.impact) ||
                !add(m.startup_entry_id.size()+m.cpu_time.note.size()+m.disk_bytes.note.size()+m.impact.note.size()) ||
                (m.impact.value && *m.impact.value!=StartupImpact::low && *m.impact.value!=StartupImpact::medium && *m.impact.value!=StartupImpact::high))
                return fail(ActionError::invalid_argument,"inconsistent/excessive overlay measurement");
            if(known(m.impact) && ((!known(m.cpu_time) && !known(m.disk_bytes)) ||
                (*m.impact.value!=StartupImpact::high && (!known(m.cpu_time) || !known(m.disk_bytes)))))
                return fail(ActionError::invalid_argument,"overlay classification lacks supporting counters");
            if(known(m.impact) && *m.impact.value!=StartupImpact::low &&
                !(known(m.cpu_time) && m.cpu_time.value->count()>0) && !(known(m.disk_bytes) && *m.disk_bytes.value>0))
                return fail(ActionError::invalid_argument,"non-low overlay classification lacks positive resource evidence");
        }
        const auto match=entries.find(row.startup_entry_id);
        if(match==entries.end()){++result.not_present_observations;continue;}
        grouped[match->second].push_back(&row);
    }
    std::vector<Patch> patches;patches.reserve(grouped.size());
    for(const auto& [index,rows]:grouped) {
        Patch patch{};patch.index=index;
        if(rows.size()==1) {
            const auto& row=*rows.front();
            patch.scope="caller-associated single process and observed interval; automatic startup origin NOT certified";
            if(row.measurement) {
                const auto& m=*row.measurement;
                patch.cpu=m.cpu_time;patch.disk=m.disk_bytes;
                if(known(patch.cpu))patch.cpu.support=Support::estimated;
                else patch.cpu.value.reset();
                if(known(patch.disk))patch.disk.support=Support::estimated;
                else patch.disk.value.reset();
                patch.interval={m.observation_interval,Support::estimated,"actual sampled interval, not whole boot or initialization duration"};
                if(known(m.impact) && (*m.impact.value==StartupImpact::high || (row.closed && row.window_complete && !row.sampling_gap)))
                    patch.impact={*m.impact.value,Support::estimated,"observed process impact only; confidence/scope required in GUI"};
                else patch.impact={std::nullopt,(row.last_error==ActionError::permission_denied || m.impact.support==Support::permission_required)?Support::permission_required:Support::unsupported,
                    "incomplete/unknown observation cannot certify low or medium"};
            } else {
                patch.impact={std::nullopt,row.last_error==ActionError::permission_denied?Support::permission_required:Support::unsupported,"no observed delta yet"};
                patch.cpu.note=patch.disk.note=patch.interval.note="no observed delta yet";
            }
        } else {
            patch.scope="multiple caller-associated processes; totals NOT summed; automatic startup origin NOT certified";
            bool high=false,permission=false;
            for(const auto* row:rows) {
                permission|=row->last_error==ActionError::permission_denied;
                if(row->measurement) {
                    const auto& m=*row->measurement;
                    high|=known(m.impact) && *m.impact.value==StartupImpact::high;
                    permission|=m.impact.support==Support::permission_required;
                }
            }
            patch.impact=high?Metric<StartupImpact>{StartupImpact::high,Support::estimated,"high observed lower bound from at least one associated process"}:
                Metric<StartupImpact>{std::nullopt,permission?Support::permission_required:Support::unsupported,"multiple process totals require explicit aggregation policy; low/medium unknown"};
            patch.cpu.note=patch.disk.note=patch.interval.note="multiple process resources/windows not aggregated";
        }
        patches.push_back(std::move(patch));
    }
    // All allocations/validation above. Commit uses noexcept moves of standard
    // metadata fields; malformed batches never partially overwrite the snapshot.
    static_assert(std::is_nothrow_move_assignable_v<Metric<StartupImpact>>);
    static_assert(std::is_nothrow_move_assignable_v<Metric<std::chrono::nanoseconds>>);
    static_assert(std::is_nothrow_move_assignable_v<Metric<std::uint64_t>>);
    static_assert(std::is_nothrow_move_assignable_v<Metric<std::chrono::steady_clock::duration>>);
    for(auto& patch:patches){
        auto& entry=snapshot.startup_entries[patch.index];
        entry.impact=patch.impact.value.value_or(StartupImpact::not_measured);
        entry.observed_startup_impact=std::move(patch.impact);
        entry.observed_startup_cpu_time=std::move(patch.cpu);entry.observed_startup_disk_bytes=std::move(patch.disk);
        entry.observed_startup_interval=std::move(patch.interval);entry.impact_scope=std::move(patch.scope);
    }
    result.applied_entries=patches.size();result.message="explicit observed-impact overlay applied; no automatic startup proof";
    return result;
}
}
