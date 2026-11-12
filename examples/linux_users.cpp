// Public SDK only: no logout, disconnect, account opener, or BPF setup.
#include <pcinsight/monitor.hpp>
#include "detail/metric_text.hpp"
#include <iostream>
#include <stdexcept>
#include <thread>
#ifdef __linux__
#include <unistd.h>
#endif
using namespace pcinsight;
namespace {
using pcinsight_example::field;
void label(std::ostream& out,const std::string& key,const std::string& value){
    field(out,key,"label",value.empty()?Metric<std::string>{}:Metric<std::string>{value,Support::available,"Reported metadata; not an authenticated identity"});
}
template<class T> void scalar(std::ostream& out,const std::string& key,const char* unit,T value){
    field(out,key,unit,Metric<T>{value,Support::available,"Reported metadata in this scope"});
}
void session(std::ostream& out,const UserSession& s,const std::string& k){
    label(out,k+".id",s.id);label(out,k+".user",s.username);label(out,k+".resource_scope",s.resource_scope);
    label(out,k+".reported_status",s.status);
    field(out,k+".cpu","percent-visible-capacity",s.cpu_percent);
    field(out,k+".memory","B-RSS",s.memory_bytes);
    field(out,k+".disconnect_available","bool",s.disconnect_available);
    field(out,k+".logout_available","bool",s.logout_available); // Never print the token.
    const auto& native=s.linux_details;
    if(metric_is_known(native)){
        label(out,k+".provider",native.value->provider);
        field(out,k+".native_state","state",native.value->state);
        field(out,k+".active","bool",native.value->active);field(out,k+".remote","bool",native.value->remote);
        field(out,k+".reported_uid","UID",native.value->unix_uid);
    }else field(out,k+".native_state","state",Metric<std::string>{{},native.support,native.note});
    const auto& io=s.linux_io_details;
    if(!metric_is_known(io)){field(out,k+".disk","B/s-accounted-storage",Metric<std::uint64_t>{{},io.support,io.note});return;}
    label(out,k+".disk_scope",io.value->scope);
    field(out,k+".disk_uid","UID",io.value->real_uid);
    field(out,k+".disk","B/s-accounted-storage",io.value->disk_bytes_per_second);
    field(out,k+".disk_matching_processes","count",io.value->matching_processes);
    const auto& c=io.value->disk_coverage;
    if(metric_is_known(c)){
        scalar(out,k+".disk_known","count",c.value->known);scalar(out,k+".disk_missing","count",c.value->missing);
        scalar(out,k+".disk_permission_denied","count",c.value->permission_denied);
        scalar(out,k+".disk_identity_unverified","count",c.value->identity_unverified);
        scalar(out,k+".disk_overflow","bool",c.value->overflow);
    }
}
void render(std::ostream& out,const SystemSnapshot& s){
    field(out,"sessions.source","provider",s.linux_session_inventory_source);
    for(std::size_t i=0;i<s.user_sessions.size();++i)session(out,s.user_sessions[i],"session["+std::to_string(i)+"]");
    const auto& n=s.linux_user_network;
    Metric<std::uint64_t> rows{{},n.support,n.note};if(metric_is_known(n))rows.value=n.value->users.size();
    field(out,"user_network.rows","records",rows);
    if(!metric_is_known(n))return;
    const auto& r=*n.value;
    scalar(out,"user_network.epoch","identifier",r.selection_epoch);
    scalar(out,"user_network.boot[0]","identifier",r.observer_boot_token[0]);scalar(out,"user_network.boot[1]","identifier",r.observer_boot_token[1]);
    scalar(out,"user_network.userns_device","identifier",r.observer.user_namespace_device);
    scalar(out,"user_network.userns_inode","identifier",r.observer.user_namespace_inode);
    scalar(out,"user_network.producer_tgid","identifier",r.producer_initial_tgid);
    scalar(out,"user_network.producer_start","ns-boottime",r.producer_start_boottime_ns);
    for(std::size_t i=0;i<r.users.size();++i){const auto& u=r.users[i];const auto k="user_network["+std::to_string(i)+"]";
        scalar(out,k+".uid","observer-UID",u.real_uid);
        scalar(out,k+".selected","count",u.selected_processes);scalar(out,k+".idle","count",u.idle_processes);scalar(out,k+".buckets","count",u.observed_buckets);
        field(out,k+".send","B/s-partial-payload",u.partial_send_bytes_per_second);
        field(out,k+".receive","B/s-partial-payload",u.partial_receive_bytes_per_second);
        field(out,k+".total","B/s-partial-payload",u.partial_total_bytes_per_second);
        constexpr const char* lanes[]{"tcp_send","tcp_receive","udp_send","udp_receive"};
        for(std::size_t j=0;j<4;++j){const auto lane=k+"."+lanes[j];
            scalar(out,lane+".known","count",u.known[j]);scalar(out,lane+".missing","count",u.missing[j]);
            scalar(out,lane+".permission_denied","count",u.permission_denied[j]);scalar(out,lane+".overflow","bool",u.overflow[j]);
        }
    } // No UID/name join to sessions, no sum across session rows.
}
void check(bool ok){if(!ok)throw std::runtime_error("Users example contract failed");}
int self_test(){
    SystemSnapshot s;UserSession u;u.id="first";u.username="same";u.logout_token="MUST_NOT_LOG";
    u.memory_bytes={0,Support::estimated,{}};u.disconnect_available={false,Support::available,{}};
    u.disk_bytes_per_second={999,Support::available,"legacy not authoritative"};
    u.linux_io_details.support=Support::permission_required;
    s.user_sessions.push_back(u);u.id="second";s.user_sessions.push_back(u);
    LinuxUserNetworkReport report;report.users.resize(1);report.users.front().real_uid=0;
    report.users.front().partial_total_bytes_per_second={7,Support::estimated,{}};
    s.linux_user_network={report,Support::estimated,{}};
    std::ostringstream out;render(out,s);const auto text=out.str();
    for(const auto* expected:{"session[0].memory\tB-RSS\testimated\t0","session[1].memory\tB-RSS\testimated\t0",
        "session[0].disconnect_available\tbool\tavailable\tfalse","session[0].disk\tB/s-accounted-storage\tpermission_required\tunknown",
        "user_network.rows\trecords\testimated\t1","user_network[0].uid\tobserver-UID\tavailable\t0",
        "user_network[0].total\tB/s-partial-payload\testimated\t7"})check(text.find(expected)!=text.npos);
    for(const auto* rejected:{"MUST_NOT_LOG","999","user_network[1]","session[0].network"})check(text.find(rejected)==text.npos);
    s.linux_user_network.support=Support::permission_required;std::ostringstream denied;render(denied,s);
    check(denied.str().find("user_network[0]")==std::string::npos);return 0;
}
}
int main(int argc,char** argv){
    if(argc==2&&std::string(argv[1])=="--self-test")return self_test();
#ifdef __linux__
    if(argc==2&&std::string(argv[1])=="--sample"){
        alarm(20);Monitor monitor;SampleOptions options;options.include_user_sessions=true;options.include_processes=false;
        options.include_disks=options.include_networks=options.include_gpus=false;options.include_linux_process_descriptions=false;
        monitor.sample(options);std::this_thread::sleep_for(std::chrono::milliseconds(150));render(std::cout,monitor.sample(options));return 0;
    }
#endif
    std::cout<<"--sample: Linux read-only sessions; --self-test: fixtures. No actions or capture.\n";return argc==1?0:2;
}
