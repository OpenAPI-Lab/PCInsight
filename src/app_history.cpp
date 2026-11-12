#include "pcinsight/app_history.hpp"
namespace pcinsight {
namespace {
template<class T> Metric<T> normalized(Metric<T> m){
    if(metric_is_known(m))return m;
    const bool invalid=m.value.has_value() || (m.support!=Support::unsupported && m.support!=Support::permission_required);
    m.value.reset();
    if(m.support!=Support::permission_required)m.support=Support::unsupported;
    if(invalid)m.note+=(m.note.empty()?"":"; ")+std::string("Inconsistent value/support rejected by history view");
    if(m.note.empty())m.note="Measurement not available from source";
    return m;
}
template<class Rows> bool valid_rows(const Rows& rows){
    if(rows.size()>100000)return false;
    std::size_t text=0;
    for(const auto& r:rows){
        if(r.id.empty()||r.id.size()>1048576||r.name.size()>1048576||r.id.find('\0')!=r.id.npos||r.name.find('\0')!=r.name.npos||r.period_start.time_since_epoch()<std::chrono::system_clock::duration::zero()||r.period_end<r.period_start)return false;
        const auto bytes=r.id.size()+r.name.size();if(bytes>16*1024*1024-text)return false;text+=bytes;
    }
    return true;
}
template<class T> Metric<std::vector<AppHistorySample>> project(const Metric<std::vector<T>>& input,AppHistoryOrigin origin){
    if(!metric_is_known(input)){
        // Validate status without copying a potentially huge rejected vector.
        auto status=normalized(Metric<bool>{input.value?std::optional<bool>{true}:std::nullopt,input.support,input.note});
        return {{},status.support,std::move(status.note)};
    }
    if(!valid_rows(*input.value))return {{},Support::unsupported,"History view identity/time/text bounds invalid; no partial rows"};
    std::vector<AppHistorySample> result;result.reserve(input.value->size());
    for(const auto& r:*input.value){
        AppHistorySample out;out.id=r.id;out.name=r.name;out.period_start=r.period_start;out.period_end=r.period_end;out.origin=origin;
        out.cpu_time=normalized(r.observed_cpu_time);
        if(out.cpu_time.value&&out.cpu_time.value->count()<0)out.cpu_time={{},Support::unsupported,"Negative CPU duration rejected by history view"};
        out.network_bytes=normalized(r.network_bytes);out.notifications=normalized(r.notifications);
        result.push_back(std::move(out));
    }
    return {std::move(result),input.support,input.note};
}
}
Metric<std::vector<AppHistorySample>> app_history_view(const Metric<std::vector<LinuxAppHistoryEntry>>& input){return project(input,AppHistoryOrigin::linux_observed);}
Metric<std::vector<AppHistorySample>> app_history_view(const Metric<std::vector<WindowsAppHistoryEntry>>& input){return project(input,AppHistoryOrigin::windows_observed);}
Metric<std::vector<AppHistorySample>> app_history_view(const SystemSnapshot& snapshot){
#ifdef _WIN32
    return app_history_view(snapshot.windows_app_history);
#elif defined(__linux__)
    return app_history_view(snapshot.linux_app_history);
#else
    (void)snapshot;return {{},Support::unsupported,"No native observed app history backend on this platform"};
#endif
}
Metric<std::vector<AppHistorySample>> import_legacy_history(std::span<const LegacyAppHistoryEntry> rows){
    if(!valid_rows(rows))return {{},Support::unsupported,"Legacy history identity/time/text bounds invalid; no partial rows"};
    for(const auto& r:rows)if(r.cpu_time.count()<0)return {{},Support::unsupported,"Invalid negative legacy CPU payload; no partial rows"};
    std::vector<AppHistorySample> output;output.reserve(rows.size());
    for(const auto& r:rows){
        AppHistorySample out;out.id=r.id;out.name=r.name;out.period_start=r.period_start;out.period_end=r.period_end;out.origin=AppHistoryOrigin::legacy_unverified;
        out.cpu_time.note=out.network_bytes.note=out.notifications.note="Legacy payload has no recorded availability/provenance: raw counter not promoted, even when zero or nonzero";
        output.push_back(std::move(out));
    }
    return {std::move(output),Support::estimated,"Legacy identities/periods only; all counters unknown. No capture, no reconstruction or automatic file writes"};
}
}
