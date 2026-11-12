// Public SDK only. No actions, launches, signaling, priority writes or BPF setup.
#include <pcinsight/monitor.hpp>
#include <pcinsight/actions.hpp>
#include "detail/metric_text.hpp"
#include <array>
#include <iostream>
#include <stdexcept>
#include <thread>
#include <type_traits>
#ifdef __linux__
#include <unistd.h>
#endif
using namespace pcinsight;
namespace {
using pcinsight_example::field;
// Compile the action signatures, NEVER invoke them in this example.
static_assert(std::is_invocable_r_v<ActionResult,decltype(&SystemActions::terminate_process_checked),SystemActions&,std::uint32_t,std::uint64_t,bool>);
static_assert(std::is_invocable_r_v<ActionResult,decltype(&SystemActions::launch_process),SystemActions&,const LaunchRequest&>);
static_assert(std::is_invocable_r_v<ActionResult,decltype(&SystemActions::set_efficiency_mode_checked),SystemActions&,std::uint32_t,std::uint64_t,bool>);
template<class T>void reported(std::ostream& out,const std::string& key,T value,const char* unit="identifier"){
    field(out,key,unit,Metric<T>{value,Support::available,"Reported snapshot scalar; NOT an authorization or durable identity"});
}
void render_process(std::ostream& out,const ProcessInfo& p,std::size_t index){
    const auto key="process["+std::to_string(index)+"].";
    reported(out,key+"pid",p.pid,"PID");reported(out,key+"parent_pid",p.parent_pid,"PID");reported(out,key+"thread_count",p.thread_count,"count");
#define P(member,unit) field(out,key+#member,unit,p.member)
    P(linux_start_ticks,"boot-ticks");P(linux_details.name,"label");P(linux_details.status,"state");
    P(linux_details.real_uid,"observer-UID");P(linux_details.username,"label");P(linux_details.architecture,"ELF");
    P(linux_details.executable,"path");P(linux_details.description,"label");P(linux_details.description_source,"source");
    P(cpu_percent,"percent-visible-CPU-capacity");P(memory_bytes,"B-RSS");P(disk_bytes_per_second,"B/s-accounted-storage");
    P(linux_details.disk_rate_real_uid,"observer-UID");P(network_bytes_per_second,"B/s-legacy");
    P(efficiency_mode_available,"bool");P(efficiency_mode_requested,"bool");
#undef P
}
void identity(std::ostream& out,const std::string& key,const LinuxNetworkProcIdentity& p){
#define I(member) reported(out,key+#member,p.member)
    I(pid);I(real_uid);I(procfs_device);I(procfs_inode);I(process_inode);
    I(pid_namespace_device);I(pid_namespace_inode);I(user_namespace_device);I(user_namespace_inode);
#undef I
    reported(out,key+"start_ticks",p.start_ticks,"boot-ticks");
}
void render_network(std::ostream& out,const Metric<LinuxProcessNetworkSnapshot>& report){
    Metric<std::uint64_t> rows{{},report.support,report.note};
    if(metric_is_known(report))rows.value=report.value->processes.size();
    field(out,"selected_network.bucket_rows","record-count",rows);
    if(!metric_is_known(report))return;
    const auto& r=*report.value;
    const char* coverage=r.coverage==LinuxProcessNetworkCoverage::selected_tcp_udp_socket_operations?"selected-single-namespace":
        r.coverage==LinuxProcessNetworkCoverage::declared_tcp_udp_socket_namespace_buckets?"declared-namespace-buckets":
        r.coverage==LinuxProcessNetworkCoverage::observed_tcp_udp_socket_namespace_buckets?"observed-namespace-buckets":"unrecognized-coverage";
    field(out,"selected_network.coverage","scope",Metric<std::string>{coverage,report.support,report.note});
    reported(out,"selected_network.epoch",r.selection_epoch);
    reported(out,"selected_network.boot_token[0]",r.observer_boot_token[0]);reported(out,"selected_network.boot_token[1]",r.observer_boot_token[1]);
    reported(out,"selected_network.producer_initial_tgid",r.producer_initial_tgid);reported(out,"selected_network.producer_start_boottime_ns",r.producer_start_boottime_ns,"ns-boottime");
    reported(out,"selected_network.sweep_started_ns",r.counter_sweep_started_ns,"ns-monotonic");reported(out,"selected_network.sweep_finished_ns",r.counter_sweep_finished_ns,"ns-monotonic");
    reported(out,"selected_network.observed_actor_rows",std::uint64_t(r.observed_processes.size()),"record-count");
    identity(out,"selected_network.observer.",r.observer);
    const std::array<const char*,4> lanes={"tcp_send","tcp_receive","udp_send","udp_receive"};
    for(std::size_t i=0;i<r.processes.size();++i){const auto& b=r.processes[i];const auto key="selected_network.bucket["+std::to_string(i)+"].";
        identity(out,key+"process.",b.process);reported(out,key+"kernel.initial_tgid",b.kernel.initial_tgid);
        reported(out,key+"kernel.initial_real_uid",b.kernel.initial_real_uid);reported(out,key+"kernel.leader_start_boottime_ns",b.kernel.leader_start_boottime_ns,"ns-boottime");
        reported(out,key+"kernel.socket_network_namespace_cookie",b.kernel.socket_network_namespace_cookie);
        field(out,key+"nominal_interval_ns","ns",b.nominal_interval_ns);
        for(std::size_t lane=0;lane<lanes.size();++lane){
            Metric<std::uint64_t> count{{},b.payload_bytes_since_start.support,b.payload_bytes_since_start.note};
            if(metric_is_known(b.payload_bytes_since_start))count.value=(*b.payload_bytes_since_start.value)[lane];
            field(out,key+lanes[lane]+".observed_payload","B-payload",count);
            field(out,key+lanes[lane]+".rate","B/s-payload-partial",b.payload_bytes_per_second[lane]);
        }
    }
    // No joins or sums into ProcessInfo.network_bytes_per_second.
}
void check(bool value){if(!value)throw std::runtime_error("Process consumer contract failed");}
int self_test(){
    ProcessInfo p;p.pid=42;p.name="DO_NOT_USE_LEGACY_FALLBACK";
    p.linux_start_ticks={0,Support::available,"valid observation, unusable checked-action token"};
    p.linux_details.real_uid={0,Support::available,{}};p.cpu_percent={0,Support::estimated,{}};
    p.memory_bytes={512,Support::permission_required,"denied"};p.linux_details.username={{},Support::unsupported,"No NSS entry"};
    std::ostringstream row;render_process(row,p,0);const auto text=row.str();
    check(text.find("DO_NOT_USE_LEGACY_FALLBACK")==text.npos);
    check(text.find("linux_details.name\tlabel\tunsupported\tunknown")!=text.npos);
    check(text.find("linux_details.real_uid\tobserver-UID\tavailable\t0")!=text.npos);
    check(text.find("linux_start_ticks\tboot-ticks\tavailable\t0")!=text.npos);
    check(text.find("cpu_percent\tpercent-visible-CPU-capacity\testimated\t0")!=text.npos);
    check(text.find("memory_bytes\tB-RSS\tpermission_required\tunknown")!=text.npos);
    check(text.find("linux_details.username\tlabel\tunsupported\tunknown")!=text.npos);
    Metric<LinuxProcessNetworkSnapshot> capture;capture.value.emplace();capture.value->processes.resize(2);
    capture.value->processes[0].payload_bytes_per_second[0]={123,Support::available,{}};
    std::ostringstream denied;render_network(denied,capture);check(denied.str().find("bucket[0]")==std::string::npos);
    capture.support=Support::estimated;auto& first=capture.value->processes[0];auto& second=capture.value->processes[1];
    first.process.pid=second.process.pid=42;first.process.start_ticks=10;second.process.start_ticks=11;
    first.payload_bytes_since_start={std::array<std::uint64_t,4>{0,0,0,0},Support::estimated,{}};
    first.payload_bytes_per_second[0]={3,Support::estimated,{}};second.payload_bytes_per_second[0]={7,Support::estimated,{}};
    std::ostringstream buckets;render_network(buckets,capture);const auto output=buckets.str();
    check(output.find("bucket[0].process.start_ticks\tboot-ticks\tavailable\t10")!=output.npos);
    check(output.find("bucket[1].process.start_ticks\tboot-ticks\tavailable\t11")!=output.npos);
    check(output.find("bucket[0].tcp_receive.observed_payload\tB-payload\testimated\t0")!=output.npos);
    check(output.find("bucket[0].tcp_receive.rate\tB/s-payload-partial\tunsupported\tunknown")!=output.npos);
    check(text.find("network_bytes_per_second\tB/s-legacy\tunsupported\tunknown")!=text.npos);
    capture.value->processes.clear();capture.value->observed_processes.resize(1);
    std::ostringstream idle;render_network(idle,capture);check(idle.str().find("bucket_rows\trecord-count\testimated\t0")!=std::string::npos);
    check(idle.str().find("bucket[0]")==std::string::npos);return 0;
}
}
int main(int argc,char** argv){
    if(argc==2&&std::string(argv[1])=="--self-test")return self_test();
    if(argc!=2||std::string(argv[1])!="--sample"){std::cout<<"--self-test: pure fixtures; --sample: Linux read-only inventory, render SELF only. No actions/capture.\n";return 0;}
#ifdef __linux__
    alarm(20);Monitor monitor;SampleOptions options;options.include_disks=false;options.include_networks=false;options.include_gpus=false;
    options.include_linux_process_descriptions=false; // Avoid desktop scan in native smoke.
    monitor.sample(options);std::this_thread::sleep_for(std::chrono::milliseconds(150));const auto sample=monitor.sample(options);
    for(std::size_t i=0;i<sample.processes.size();++i)if(sample.processes[i].pid==std::uint32_t(getpid())){render_process(std::cout,sample.processes[i],i);render_network(std::cout,sample.linux_process_network);return 0;}
    std::cerr<<"SELF not observed; no fabricated process row.\n";return 1;
#else
    std::cerr<<"--sample is Linux-only.\n";return 2;
#endif
}
