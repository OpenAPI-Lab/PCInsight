// Read-only SDK example. Never attaches BPF, connects a notification observer,
// writes a history file, or invokes SystemActions.
#include <pcinsight/app_history.hpp>
#include <pcinsight/linux_notifications.hpp>
#include <pcinsight/monitor.hpp>
#include "detail/metric_text.hpp"
#include <iostream>
#include <stdexcept>
#include <thread>
#ifdef __linux__
#include <unistd.h>
#endif
using namespace pcinsight;
using namespace std::chrono;
namespace {
using pcinsight_example::field;
template<class T> void shown(std::ostream& out,const std::string& key,const char* unit,T value){
    field(out,key,unit,Metric<T>{std::move(value),Support::available,"Reported metadata, not verified application identity"});
}
template<class T> void count(std::ostream& out,const std::string& key,const Metric<T>& report,std::size_t n){
    Metric<std::uint64_t> result{{},report.support,report.note};
    if(metric_is_known(report))result.value=n;
    field(out,key,"rows",result);
}
void period(std::ostream& out,const std::string& key,system_clock::time_point begin,system_clock::time_point end){
    shown(out,key+".start","ms-since-epoch-display",duration_cast<milliseconds>(begin.time_since_epoch()).count());
    shown(out,key+".end","ms-since-epoch-display",duration_cast<milliseconds>(end.time_since_epoch()).count());
}
void history(std::ostream& out,const std::string& source,const Metric<std::vector<AppHistorySample>>& view){
    count(out,source+".rows",view,view.value?view.value->size():0);
    if(!metric_is_known(view))return;
    for(std::size_t i=0;i<view.value->size();++i){
        const auto& row=(*view.value)[i];const auto key=source+"["+std::to_string(i)+"]";
        shown(out,key+".id","observed-id",row.id);shown(out,key+".name","label",row.name);
        period(out,key,row.period_start,row.period_end);
        field(out,key+".cpu_time","ns",row.cpu_time);field(out,key+".network","B-payload",row.network_bytes);
        field(out,key+".notifications","count",row.notifications);
    }
}
void notifications(std::ostream& out,const Metric<LinuxNotificationReport>& report){
    count(out,"notify.rows",report,report.value?report.value->apps.size():0);
    if(!metric_is_known(report))return;
    const auto& r=*report.value;
    period(out,"notify",r.period_start,r.period_end);
    shown(out,"notify.server","connection",r.server_unique_name);
    shown(out,"notify.running","bool",r.running);
    shown(out,"notify.pending","calls",r.pending_calls);
    shown(out,"notify.rejected","calls",r.rejected_calls);
    shown(out,"notify.expired_unresolved","calls",r.expired_unresolved_calls);
    shown(out,"notify.no_reply_unconfirmed","calls",r.unconfirmed_no_reply_calls);
    shown(out,"notify.poll_budget_reached","bool",r.poll_budget_reached);
    for(std::size_t i=0;i<r.apps.size();++i){
        const auto& a=r.apps[i];const auto key="notify["+std::to_string(i)+"]";
        shown(out,key+".sender","connection",a.sender_unique_name);
        field(out,key+".declared_desktop_entry","untrusted-hint",a.declared_desktop_entry);
        shown(out,key+".accepted_notify_calls","calls",a.accepted_notify_calls);
        shown(out,key+".replacement_calls","calls",a.replacement_calls);
    }
}
void check(bool ok){if(!ok)throw std::runtime_error("App history example contract failed");}
int self_test(){
    LinuxAppHistoryEntry cpu;cpu.id="/fixture/same";cpu.name="same";
    cpu.period_start=system_clock::time_point(seconds(1));cpu.period_end=cpu.period_start+seconds(2);
    cpu.observed_cpu_time={nanoseconds(0),Support::estimated,{}};
    auto network=cpu;network.observed_cpu_time={};network.network_bytes={42,Support::estimated,{}};
    Metric<std::vector<LinuxAppHistoryEntry>> cpu_report{{{cpu}},Support::estimated,{}};
    Metric<std::vector<LinuxAppHistoryEntry>> net_report{{{network}},Support::estimated,{}};
    LinuxNotificationReport n;n.period_start=cpu.period_start;n.period_end=cpu.period_end;
    n.apps.push_back({":1.9",{cpu.id,Support::estimated,"untrusted"},3,1});
    LegacyAppHistoryEntry old;old.id=cpu.id;old.name=cpu.name;old.period_start=cpu.period_start;old.period_end=cpu.period_end;old.network_bytes=99;
    std::ostringstream out;
    history(out,"cpu",app_history_view(cpu_report));history(out,"network",app_history_view(net_report));
    notifications(out,{n,Support::estimated,{}});history(out,"legacy",import_legacy_history(std::span(&old,1)));
    const auto lines=out.str();
    for(const auto* expected:{"cpu[0].cpu_time\tns\testimated\t0", "cpu[0].network\tB-payload\tunsupported\tunknown",
        "network[0].cpu_time\tns\tunsupported\tunknown", "network[0].network\tB-payload\testimated\t42",
        "notify[0].accepted_notify_calls\tcalls\tavailable\t3", "notify[0].replacement_calls\tcalls\tavailable\t1",
        "legacy[0].network\tB-payload\tunsupported\tunknown"})check(lines.find(expected)!=lines.npos);
    std::ostringstream denied;notifications(denied,{n,Support::permission_required,"not authorized"});
    check(denied.str().find("notify.rows\trows\tpermission_required\tunknown")!=std::string::npos);
    check(denied.str().find("notify[0]")==std::string::npos);
    return 0;
}
}
int main(int argc,char** argv){
    if(argc==2&&std::string(argv[1])=="--self-test")return self_test();
    if(argc!=2||std::string(argv[1])!="--sample"){std::cout<<"--sample: two Linux CPU history observations; --self-test: pure fixtures. No extra capture or storage.\n";return 0;}
#ifdef __linux__
    alarm(20);Monitor monitor;SampleOptions options;
    options.include_app_history=true;options.include_processes=false;
    options.include_disks=options.include_networks=options.include_gpus=false;
    options.include_linux_process_descriptions=false;
    monitor.sample(options);std::this_thread::sleep_for(milliseconds(150));
    auto sample=monitor.sample(options);
    history(std::cout,"cpu",app_history_view(sample.linux_app_history));
    history(std::cout,"network",app_history_view(sample.linux_network_app_history));
    notifications(std::cout,{{},Support::unsupported,"Observer not started"});
    return 0;
#else
    std::cerr<<"--sample is Linux-only.\n";return 2;
#endif
}
