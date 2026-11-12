#include "pcinsight/app_history.hpp"
#include "pcinsight/history_store.hpp"
#include "pcinsight/monitor.hpp"
#include <cassert>
#include <iostream>
#include <limits>
#include <type_traits>
using namespace pcinsight;
using namespace std::chrono_literals;
static_assert(std::is_same_v<LegacyAppHistoryEntry,AppHistoryEntry>);
static_assert(!std::is_convertible_v<AppHistorySample,LegacyAppHistoryEntry>);
static_assert(!std::is_convertible_v<LegacyAppHistoryEntry,AppHistorySample>);
static_assert(std::is_same_v<decltype(AppHistorySample{}.cpu_time),Metric<std::chrono::nanoseconds>>);
int main(){
    Metric<std::uint64_t> value;assert(!metric_is_known(value));
    value={0,Support::available,"measured zero"};assert(metric_is_known(value));
    value.support=Support::estimated;assert(metric_is_known(value));
    value.support=Support::unsupported;assert(!metric_is_known(value));
    value.support=Support::permission_required;assert(!metric_is_known(value));
    value.value.reset();value.support=Support::available;assert(!metric_is_known(value));
    Metric<bool> measured_false{false,Support::available,"measured false"};assert(metric_is_known(measured_false));
    AppHistorySample empty;assert(!metric_is_known(empty.cpu_time)&&!metric_is_known(empty.network_bytes)&&!metric_is_known(empty.notifications));
    LinuxAppHistoryEntry row;row.id="/fixture";row.name="fixture";row.period_start=std::chrono::system_clock::time_point{}+1h;row.period_end=row.period_start+1s;
    row.observed_cpu_time={0ns,Support::estimated,"CPU zero measured"};row.network_bytes.note="not collected";
    row.notifications={{},Support::permission_required,"denied fixture"};
    Metric<std::vector<LinuxAppHistoryEntry>> linux_input{{std::vector{row}},Support::estimated,"capture scope"};
    auto result=app_history_view(linux_input);assert(metric_is_known(result)&&result.note=="capture scope"&&result.value->size()==1);
    auto out=result.value->front();assert(out.id==row.id&&out.name==row.name&&out.period_start==row.period_start&&out.period_end==row.period_end&&out.origin==AppHistoryOrigin::linux_observed);
    assert(metric_is_known(out.cpu_time)&&out.cpu_time.value==0ns&&out.cpu_time.note=="CPU zero measured");
    assert(!out.network_bytes.value&&out.network_bytes.note=="not collected");assert(!out.notifications.value&&out.notifications.support==Support::permission_required&&out.notifications.note=="denied fixture");
    assert(!linux_input.value->front().network_bytes.value); // projection does not mutate source
    row.network_bytes={0,Support::available,"network measured0"};row.notifications={0,Support::estimated,"notifications measured0"};
    row.observed_cpu_time={9ns,Support::available,"CPU measured"};linux_input.value=std::vector{row};
    result=app_history_view(linux_input);assert(result.value->front().network_bytes.value==0&&metric_is_known(result.value->front().notifications));
    auto maximum=linux_input;maximum.value->front().network_bytes.value=std::numeric_limits<std::uint64_t>::max();
    assert(app_history_view(maximum).value->front().network_bytes.value==std::numeric_limits<std::uint64_t>::max());
    for(const auto support:{Support::unsupported,Support::permission_required,static_cast<Support>(99)}){
        auto bad=linux_input;bad.value->front().network_bytes.support=support;result=app_history_view(bad);
        assert(result.value&&!result.value->front().network_bytes.value&&!result.value->front().network_bytes.note.empty());
    }
    auto bad=linux_input;bad.value->front().observed_cpu_time={-1ns,Support::available,"negative"};assert(!app_history_view(bad).value->front().cpu_time.value);
    bad=linux_input;bad.value->front().network_bytes={{},Support::available,"inconsistent"};result=app_history_view(bad);assert(!result.value->front().network_bytes.value&&result.value->front().network_bytes.support==Support::unsupported);
    for(const auto support:{Support::unsupported,Support::permission_required,static_cast<Support>(99)}){
        bad=linux_input;bad.support=support;result=app_history_view(bad);assert(!result.value);
        if(support==Support::permission_required)assert(result.support==Support::permission_required);
    }
    bad=linux_input;bad.value.reset();bad.support=Support::available;assert(!app_history_view(bad).value&&app_history_view(bad).support==Support::unsupported);
    bad.value=std::vector<LinuxAppHistoryEntry>{};assert(app_history_view(bad).value->empty()); // known empty != unavailable
    for(int mode=0;mode<6;++mode){
        bad=linux_input;auto& r=bad.value->front();
        if(mode==0)r.id.clear();if(mode==1)r.id=std::string("a\0b",3);if(mode==2)r.name=std::string(1048577,'x');
        if(mode==3)r.period_end=r.period_start-1s;if(mode==4)r.period_start=std::chrono::system_clock::time_point{}-1s;
        if(mode==5)bad.value->resize(100001,row);
        assert(!app_history_view(bad).value);
    }
    bad=linux_input;auto huge=row;huge.id=std::string(1048576,'x');bad.value=std::vector(17,huge);assert(!app_history_view(bad).value);
    WindowsAppHistoryEntry windows_row;windows_row.id="C:\\fixture.exe";windows_row.name="fixture";windows_row.period_start=row.period_start;windows_row.period_end=row.period_end;
    windows_row.observed_cpu_time={1ns,Support::estimated,"Windows fixture"};
    Metric<std::vector<WindowsAppHistoryEntry>> windows_input{std::vector{windows_row},Support::estimated,"Windows capture"};
    result=app_history_view(windows_input);assert(result.value->front().origin==AppHistoryOrigin::windows_observed&&result.value->front().cpu_time.value==1ns&&!result.value->front().network_bytes.value);
    LegacyAppHistoryEntry raw;raw.id="/legacy";raw.name="legacy";raw.cpu_time=7ns;raw.network_bytes=42;raw.notifications=0;raw.period_start=row.period_start;raw.period_end=row.period_end;
    result=import_legacy_history(std::span(&raw,1));assert(result.value&&result.value->front().origin==AppHistoryOrigin::legacy_unverified);
    assert(!result.value->front().cpu_time.value&&!result.value->front().network_bytes.value&&!result.value->front().notifications.value);
    assert(raw.cpu_time==7ns&&raw.network_bytes==42&&raw.notifications==0);raw.cpu_time=0ns;raw.network_bytes=0;
    result=import_legacy_history(std::span(&raw,1));assert(!result.value->front().cpu_time.value&&!result.value->front().network_bytes.value);
    raw.cpu_time=-1ns;assert(!import_legacy_history(std::span(&raw,1)).value);assert(import_legacy_history({}).value->empty());
    // Native routing uses only the platform report, not ambiguous legacy payload.
    SystemSnapshot fixture;fixture.linux_app_history=linux_input;fixture.windows_app_history=windows_input;fixture.app_history.push_back(raw);
#ifdef _WIN32
    assert(app_history_view(fixture).value->front().origin==AppHistoryOrigin::windows_observed);
#elif defined(__linux__)
    assert(app_history_view(fixture).value->front().origin==AppHistoryOrigin::linux_observed);
#else
    assert(!app_history_view(fixture).value);
#endif
    SampleOptions options;options.include_app_history=true;options.include_processes=false;options.include_linux_process_descriptions=false;
    options.include_disks=false;options.include_networks=false;options.include_gpus=false;options.include_user_sessions=false;options.include_services=false;options.include_startup_entries=false;
    Monitor monitor;auto snapshot=monitor.sample(options);result=app_history_view(snapshot);assert(snapshot.app_history.empty());
#if defined(__linux__) || defined(_WIN32)
    assert(result.value);for(const auto& r:*result.value){assert(!r.network_bytes.value&&!r.notifications.value);if(metric_is_known(r.cpu_time))assert(r.cpu_time.value->count()>=0);}
#endif
    options.include_app_history=false;snapshot=monitor.sample(options);assert(!app_history_view(snapshot).value);
    std::cout<<"App history API: known zero vs unknown, per-field support/notes, portable platform projections, bounds/no partial rows, unverified legacy import and native opt-in routing passed\n";
}
