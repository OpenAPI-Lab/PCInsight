// Read-only public SDK example. Inventory never selects a control backend.
#include <pcinsight/monitor.hpp>
#include <pcinsight/linux_services.hpp>
#include "detail/metric_text.hpp"
#include <iostream>
#include <stdexcept>
#ifdef __linux__
#include <unistd.h>
#endif
using namespace pcinsight;
namespace {
using pcinsight_example::field;
void label(std::ostream& out,const std::string& key,const std::string& value){
    field(out,key,"label",value.empty()?Metric<std::string>{}:Metric<std::string>{value,Support::available,"Reported label, not authority to act"});
}
template<class T> Metric<T> member(const Metric<LinuxServiceDetails>& details,Metric<T> LinuxServiceDetails::*p){
    if(!metric_is_known(details))return {{},details.support,details.note};
    auto result=(*details.value).*p;
    if(details.support==Support::estimated&&result.support==Support::available)result.support=Support::estimated;
    return result;
}
void row(std::ostream& out,const ServiceInfo& service,const std::string& key){
    const auto& d=service.linux_details;
    label(out,key+".id",service.id);
    if(metric_is_known(d)){label(out,key+".provider",d.value->provider);label(out,key+".group_kind",d.value->group_kind);}
#define S(name,unit) field(out,key+"." #name,unit,member(d,&LinuxServiceDetails::name))
    S(name,"label");S(description,"text");S(status,"reported-state");S(main_pid,"reported-PID");
    S(slice,"systemd-slice");S(load_state,"state");S(active_state,"state");S(sub_state,"state");S(unit_file_state,"installation-state");
#undef S
    // No raw supervisor PID, install flag, configured name or marker becomes a control target.
}
void render(std::ostream& out,const SystemSnapshot& s){
    field(out,"services.source","provider",s.linux_service_inventory_source);
    Metric<std::uint64_t> rows{{},s.linux_service_inventory_source.support,s.linux_service_inventory_source.note};
    if(metric_is_known(s.linux_service_inventory_source))rows.value=s.services.size();
    field(out,"services.rows","records",rows);
    if(!metric_is_known(s.linux_service_inventory_source))return;
    for(std::size_t i=0;i<s.services.size();++i)row(out,s.services[i],"service["+std::to_string(i)+"]");
}
void check(bool ok){if(!ok)throw std::runtime_error("Services example contract failed");}
int self_test(){
    SystemSnapshot s;s.linux_service_inventory_source={"fixture",Support::estimated,"partial scope"};
    ServiceInfo a;a.id="first";a.pid=999;a.description="LEGACY_MUST_NOT_LEAK";
    LinuxServiceDetails d;d.provider="systemd-system-manager";d.group_kind="systemd-slice";
    d.name={"example.service",Support::available,{}};d.main_pid={0,Support::available,"no main PID reported"};
    d.description={"",Support::available,{}};d.slice={"",Support::available,{}};
    d.unit_file_state={"enabled",Support::available,{}};a.linux_details={d,Support::available,{}};
    ServiceInfo b;b.id="marked";b.pid=998;b.linux_details.value.emplace();
    b.linux_details.value->provider="openrc-markers";
    b.linux_details.value->status={"marked-started",Support::estimated,"not process liveness"};
    b.linux_details.support=Support::estimated;s.services={a,b};
    std::ostringstream out;render(out,s);const auto text=out.str();
    for(const auto* value:{"services.rows\trecords\testimated\t2","service[0].main_pid\treported-PID\tavailable\t0",
        "service[0].description\ttext\tavailable\t\"\"","service[0].slice\tsystemd-slice\tavailable\t\"\"",
        "service[0].unit_file_state\tinstallation-state\tavailable\t\"enabled\"","service[0].status\treported-state\tunsupported\tunknown",
        "service[1].status\treported-state\testimated\t\"marked-started\"","service[1].main_pid\treported-PID\tunsupported\tunknown"})check(text.find(value)!=text.npos);
    for(const auto* bad:{"LEGACY_MUST_NOT_LEAK","999","998"})check(text.find(bad)==text.npos);
    s.linux_service_inventory_source.support=Support::permission_required;
    std::ostringstream denied;render(denied,s);check(denied.str().find("service[0]")==std::string::npos);
    s.services.clear();s.linux_service_inventory_source.support=Support::available;
    std::ostringstream empty;render(empty,s);check(empty.str().find("services.rows\trecords\tavailable\t0")!=std::string::npos);
    return 0;
}
}
int main(int argc,char** argv){
    if(argc==2&&std::string(argv[1])=="--self-test")return self_test();
#ifdef __linux__
    if(argc==3&&std::string(argv[1])=="--details"){
        alarm(20);const auto result=open_linux_service({LinuxServiceScope::system,argv[2]});
        if(!metric_is_known(result)){std::cerr<<pcinsight_example::escaped(result.note)<<'\n';return 1;}
        row(std::cout,result.value->service,"details");return 0;
    }
    if(argc==2&&std::string(argv[1])=="--sample"){
        alarm(20);Monitor monitor;SampleOptions options;options.include_services=true;options.include_processes=false;
        options.include_disks=options.include_networks=options.include_gpus=false;
        options.linux_service_selection.provider=LinuxServiceProvider::systemd_system;
        render(std::cout,monitor.sample(options));return 0;
    }
#endif
    std::cout<<"--sample: explicit systemd system inventory; --details UNIT.service: read-only; --self-test: fixtures. No control.\n";
    return argc==1?0:2;
}
