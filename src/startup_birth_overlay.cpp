#include "pcinsight/startup_birth_overlay.hpp"
#include "startup_birth_archive.hpp"
#include <map>
#include <type_traits>
namespace pcinsight {
namespace {
bool id_valid(const std::string& id){return !id.empty()&&id.size()<=32768&&id.find('\0')==id.npos;}
struct Patch {
    std::size_t index{};
    Metric<StartupImpact> impact;
    Metric<std::chrono::nanoseconds> cpu;
    Metric<std::uint64_t> disk;
    Metric<std::chrono::steady_clock::duration> interval;
    std::string scope;
};
}
StartupBirthOverlayResult apply_startup_birth_impact(SystemSnapshot& snapshot,const StartupBirthArchive& archive,const StartupBirthOverlayContext& context){
    const auto fail=[](ActionError e,const char* note){return StartupBirthOverlayResult{e,0,0,0,note};};
    if(!context.accept_declared_associations)return fail(ActionError::permission_denied,"Explicit acceptance of historical caller-declared associations required");
    if(context.capture_id.empty()||context.capture_id.size()>128||context.capture_id.find('\0')!=context.capture_id.npos||!(context.boot[0]||context.boot[1])||!context.pid_namespace_device||!context.pid_namespace_inode||snapshot.startup_entries.size()>100000)
        return fail(ActionError::invalid_argument,"Invalid/excessive accepted birth context or startup catalog");
    const auto& capture=archive.capture;
    if(context.capture_id!=capture.capture_id||context.boot!=capture.boot||context.pid_namespace_device!=capture.pid_namespace_device||context.pid_namespace_inode!=capture.pid_namespace_inode)
        return fail(ActionError::stale_target,"Accepted capture/boot/PID namespace does not match historical birth source");
    if(!detail::validate_startup_birth_archive(archive).empty())return fail(ActionError::invalid_argument,"Invalid historical birth archive; no partial overlay");
    std::map<std::string,std::size_t> entries;std::size_t bytes=0;
    for(std::size_t n=0;n<snapshot.startup_entries.size();++n){const auto& id=snapshot.startup_entries[n].id;
        if(!id_valid(id)||id.size()>8*1024*1024-bytes||!entries.emplace(id,n).second)return fail(ActionError::invalid_argument,"Invalid/duplicate startup entry ID or catalog payload limit");
        bytes+=id.size();
    }
    std::map<std::size_t,std::vector<StartupBirthImpactMeasurement>> groups;
    StartupBirthOverlayResult result;
    for(const auto& row:capture.observations){
        if(!row.startup_entry_id.value){++result.unbound_observations;continue;}
        const auto target=entries.find(*row.startup_entry_id.value);
        if(target==entries.end()){++result.not_present_observations;continue;}
        auto measured=measure_startup_birth_impact(capture,row.key,archive.window,archive.policy);
        if(!measured)return fail(ActionError::invalid_argument,"Invalid bound birth row; no partial overlay");
        groups[target->second].push_back(std::move(*measured.value));
    }
    std::vector<Patch> patches;patches.reserve(groups.size());
    for(const auto& [index,rows]:groups){Patch patch;patch.index=index;
        patch.scope="HISTORICAL birth capture="+capture.capture_id+"; boot="+std::to_string(capture.boot[0])+":"+std::to_string(capture.boot[1])+"; PIDns="+std::to_string(capture.pid_namespace_device)+":"+std::to_string(capture.pid_namespace_inode)+"; caller-declared association only; automatic startup origin/app identity NOT certified; per-child birth-relative window, NOT current usage/global startup phase; source="+
            std::string(capture.state==StartupBirthObserverState::failed?"failed":"closed")+"; quality_flags="+std::to_string(capture.quality_flags);
        patch.interval={{},Support::unsupported,"Kernel birth-relative window is not a legacy steady-clock sampled interval"};
        if(rows.size()==1){patch.cpu=rows[0].cpu_time;patch.disk=rows[0].disk_bytes;patch.impact=rows[0].impact;}
        else {bool high=false,permission=false;for(const auto& m:rows){high|=m.impact.value==StartupImpact::high;permission|=m.impact.support==Support::permission_required;}
            patch.cpu.note=patch.disk.note="Multiple historical groups; overlapping group accounting is NOT summed";
            patch.impact=high?Metric<StartupImpact>{StartupImpact::high,Support::estimated,"High observed lower bound in at least one declared group; totals not aggregated"}:
                Metric<StartupImpact>{{},permission?Support::permission_required:Support::unsupported,"Multiple partial groups cannot certify low/medium; no aggregation"};
            patch.scope+="; MULTIPLE groups, totals NOT summed";
        }
        patches.push_back(std::move(patch));
    }
    result.applied_entries=patches.size();result.message="Explicit historical birth overlay; declared IDs only, not verified current app/cause or complete startup coverage";
    static_assert(std::is_nothrow_move_assignable_v<Metric<StartupImpact>>&&std::is_nothrow_move_assignable_v<Metric<std::chrono::nanoseconds>>&&std::is_nothrow_move_assignable_v<Metric<std::uint64_t>>&&std::is_nothrow_move_assignable_v<Metric<std::chrono::steady_clock::duration>>&&std::is_nothrow_move_assignable_v<std::string>);
    // All validation and allocations completed before this noexcept commit.
    for(auto& patch:patches){auto& e=snapshot.startup_entries[patch.index];e.impact=patch.impact.value.value_or(StartupImpact::not_measured);
        e.observed_startup_impact=std::move(patch.impact);e.observed_startup_cpu_time=std::move(patch.cpu);e.observed_startup_disk_bytes=std::move(patch.disk);e.observed_startup_interval=std::move(patch.interval);e.impact_scope=std::move(patch.scope);}
    return result;
}
}
