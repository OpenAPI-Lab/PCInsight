#include "pcinsight/linux_drm.hpp"
#include "linux_drm_client.hpp"
#include "linux_nvml.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
#include <set>
#include <tuple>
namespace pcinsight {
namespace {
bool valid_name(const std::string& s,bool spaces=false){return !s.empty() && s.size()<=128 && std::all_of(s.begin(),s.end(),[&](unsigned char c){return c>=(spaces?32:33) && c<=126 && c!=':';});}
bool valid(const LinuxDrmClientUsage& v){
    if(!v.pid || v.descriptor>static_cast<std::uint32_t>(std::numeric_limits<int>::max()) || !valid_name(v.driver,true) || (v.pci_address.size()!=12 && v.pci_address.size()!=16) || detail::nvml_pci_identity(v.pci_address).empty() || v.engines.empty() || v.engines.size()>128)return false;
    std::set<std::string> names;
    for(const auto& e:v.engines){
        if(!valid_name(e.name) || !e.capacity || !names.insert(e.name).second)return false;
        if(e.counter_kind==LinuxDrmCounterKind::cycles){if(!e.busy_cycles.value || !e.total_cycles.value || e.busy_cycles.support!=Support::available || e.total_cycles.support!=Support::available)return false;}
        else if(e.counter_kind==LinuxDrmCounterKind::cycles_max_frequency){if(!e.busy_cycles.value || !e.max_frequency_hz.value || !*e.max_frequency_hz.value || e.busy_cycles.support!=Support::available || e.max_frequency_hz.support!=Support::available || e.total_cycles.value)return false;}
        else if(e.counter_kind!=LinuxDrmCounterKind::nanoseconds)return false;
    }
    return true;
}
bool compatible(const LinuxDrmClientUsage& older,const LinuxDrmClientUsage& newer){
    if(older.driver!=newer.driver || older.engines.size()!=newer.engines.size())return false;
    // Copies are normalized by engine name before deduplication.
    for(std::size_t k=0;k<older.engines.size();++k){const auto& a=older.engines[k];const auto& b=newer.engines[k];
        if(a.name!=b.name || a.capacity!=b.capacity || a.counter_kind!=b.counter_kind)return false;
        if(a.counter_kind==LinuxDrmCounterKind::cycles){
            if(*a.busy_cycles.value>*b.busy_cycles.value || *a.total_cycles.value>*b.total_cycles.value ||
                (older.captured_at==newer.captured_at && (a.busy_cycles.value!=b.busy_cycles.value || a.total_cycles.value!=b.total_cycles.value)))return false;
        }else if(a.counter_kind==LinuxDrmCounterKind::cycles_max_frequency){
            if(a.max_frequency_hz.value!=b.max_frequency_hz.value || *a.busy_cycles.value>*b.busy_cycles.value || (older.captured_at==newer.captured_at && a.busy_cycles.value!=b.busy_cycles.value))return false;
        }else if(a.busy_nanoseconds>b.busy_nanoseconds || (older.captured_at==newer.captured_at && a.busy_nanoseconds!=b.busy_nanoseconds))return false;
    }
    return true;
}
}
Metric<std::vector<LinuxDrmGpuAggregate>> LinuxDrmGpuSampler::sample(const std::vector<Metric<LinuxDrmClientUsage>>& input){
    auto fail=[&](std::string note,Support support=Support::unsupported)->Metric<std::vector<LinuxDrmGpuAggregate>>{reset();return {{},support,std::move(note)};};
    if(input.size()>2048)return fail("DRM batch exceeds 2048 observations; no truncated aggregate");
    using Key=std::pair<std::string,std::uint64_t>;
    std::map<Key,std::vector<LinuxDrmClientUsage>> copies;
    std::map<std::string,std::string> drivers;
    std::map<std::string,std::uint64_t> record_counts;
    std::size_t engine_count=0;
    for(const auto& r:input){
        if(!r.value || r.support!=Support::available)return fail("Unusable input observation; aggregate omitted rather than treating it as zero",r.support==Support::permission_required?Support::permission_required:Support::unsupported);
        const auto& v=*r.value;
        if(!valid(v))return fail("Malformed DRM client identity/engine input");
        engine_count+=v.engines.size();if(engine_count>65536)return fail("DRM batch exceeds 65536 engine records");
        const auto pci=detail::nvml_pci_identity(v.pci_address);
        const auto [driver,inserted]=drivers.emplace(pci,v.driver);if(!inserted && driver->second!=v.driver)return fail("Conflicting drivers for one PCI device");
        // Ignore caller-derived percentages/notes; retain only bounded raw inputs.
        LinuxDrmClientUsage copy;
        copy.pid=v.pid;copy.descriptor=v.descriptor;copy.process_start_ticks=v.process_start_ticks;
        copy.client_id=v.client_id;copy.driver=v.driver;copy.pci_address=v.pci_address;copy.captured_at=v.captured_at;
        copy.engines.reserve(v.engines.size());
        for(const auto& e:v.engines){
            LinuxDrmEngineUsage raw;raw.name=e.name;raw.busy_nanoseconds=e.busy_nanoseconds;raw.capacity=e.capacity;raw.counter_kind=e.counter_kind;
            if(e.busy_cycles.value)raw.busy_cycles={e.busy_cycles.value,e.busy_cycles.support,"Raw GPU busy cycles"};
            if(e.total_cycles.value)raw.total_cycles={e.total_cycles.value,e.total_cycles.support,"Raw GPU total cycles"};
            if(e.max_frequency_hz.value)raw.max_frequency_hz={e.max_frequency_hz.value,e.max_frequency_hz.support,"Raw maximum frequency in Hz"};
            copy.engines.push_back(std::move(raw));
        }
        std::sort(copy.engines.begin(),copy.engines.end(),[](const auto& a,const auto& b){return a.name<b.name;});
        copies[{pci,v.client_id}].push_back(std::move(copy));++record_counts[pci];
    }
    std::map<Key,LinuxDrmClientUsage> selected;
    for(auto& [key,rows]:copies){
        std::sort(rows.begin(),rows.end(),[](const auto& a,const auto& b){
            return std::tie(a.captured_at,a.pid,a.process_start_ticks,a.descriptor)<std::tie(b.captured_at,b.pid,b.process_start_ticks,b.descriptor);
        });
        for(std::size_t k=1;k<rows.size();++k)if(!compatible(rows[k-1],rows[k]))return fail("Conflicting shared DRM client observations; no double-counted or guessed aggregate");
        selected.emplace(key,std::move(rows.back()));
    }
    std::map<Key,LinuxDrmClientUsage> next;
    std::map<std::string,LinuxDrmGpuAggregate> groups;
    struct Sum {LinuxDrmEngineAggregate value;long double total{};bool consistent{true};};
    std::map<std::pair<std::string,std::string>,Sum> sums;
    for(const auto& [key,raw]:selected){
        std::optional<LinuxDrmClientUsage> previous;
        if(auto old=previous_.find(key);old!=previous_.end())previous=old->second;
        auto sampled=detail::update_linux_drm_client_usage(previous,{raw,Support::available,"explicit DRM batch"});
        next.emplace(key,std::move(*previous));
        const auto& v=*sampled.value;
        auto [it,inserted]=groups.try_emplace(key.first);auto& g=it->second;
        if(inserted){g.pci_address=v.pci_address;g.driver=v.driver;g.input_records=record_counts.at(key.first);g.earliest_observation=g.latest_observation=v.captured_at;}
        ++g.unique_clients;g.earliest_observation=std::min(g.earliest_observation,v.captured_at);g.latest_observation=std::max(g.latest_observation,v.captured_at);
        for(const auto& e:v.engines){
            auto [sum_it,fresh]=sums.try_emplace({key.first,e.name});auto& s=sum_it->second;auto& a=s.value;
            if(fresh){a.name=e.name;a.capacity={e.capacity,Support::available,"Capacity reported by selected DRM clients"};a.engine_class=classify_linux_drm_engine(v.driver,e.name);a.counter_kind={e.counter_kind,Support::available,"Counter basis for selected reporting clients"};if(e.counter_kind==LinuxDrmCounterKind::cycles_max_frequency)a.max_frequency_hz=e.max_frequency_hz;}
            else if(a.capacity.value!=e.capacity || a.counter_kind.value!=e.counter_kind || (e.counter_kind==LinuxDrmCounterKind::cycles_max_frequency && a.max_frequency_hz.value!=e.max_frequency_hz.value))s.consistent=false;
            ++a.observed_clients;
            if(e.utilization_percent.value && e.utilization_percent.support==Support::estimated){
                ++a.measured_clients;s.total+=*e.utilization_percent.value;
                if(!v.sample_interval.value)continue;
                const auto interval=*v.sample_interval.value;
                if(!a.shortest_interval.value || interval<*a.shortest_interval.value)a.shortest_interval={interval,Support::available,"Shortest selected client interval; intervals need not align"};
                if(!a.longest_interval.value || interval>*a.longest_interval.value)a.longest_interval={interval,Support::available,"Longest selected client interval; intervals need not align"};
            }
        }
    }
    for(auto& [key,s]:sums){auto& a=s.value;
        auto& value=a.counter_kind.value==LinuxDrmCounterKind::cycles?a.summed_client_cycle_percent:a.counter_kind.value==LinuxDrmCounterKind::cycles_max_frequency?a.summed_client_frequency_percent:a.summed_client_active_percent;
        if(!s.consistent){a.capacity={{},Support::unsupported,"Conflicting capacities, frequencies or counter kinds across clients"};a.counter_kind={{},Support::unsupported,"No common counter basis"};a.max_frequency_hz={{},Support::unsupported,"No common normalization"};a.summed_client_active_percent.note=a.summed_client_cycle_percent.note=a.summed_client_frequency_percent.note="No sum of differently normalized engine classes or counter bases";}
        else if(a.measured_clients!=a.observed_clients)value.note="Not all selected reporting clients have a valid delta; no partial sum";
        else if(!std::isfinite(s.total) || s.total>std::numeric_limits<double>::max())value.note="Aggregate arithmetic outside representable range";
        else value={static_cast<double>(s.total),Support::estimated,"Sum of selected deduplicated client rates using reported counter basis; may exceed 100, NOT GPU-global utilization or coverage certification"};
        groups.at(key.first).engines.push_back(std::move(a));
    }
    std::vector<LinuxDrmGpuAggregate> result;result.reserve(groups.size());for(auto& [pci,g]:groups)result.push_back(std::move(g));
    previous_=std::move(next);
    return {std::move(result),Support::estimated,"Caller-selected DRM observations only; no all-process discovery or GPU-global coverage guarantee"};
}
}
