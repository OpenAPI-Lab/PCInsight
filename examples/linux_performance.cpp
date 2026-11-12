// Public SDK only. Read-only example; no SystemActions, capture, or helper setup.
#include <pcinsight/monitor.hpp>
#include <pcinsight/linux_network_addresses.hpp>
#include "detail/metric_text.hpp"
#include <cmath>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <thread>
#include <type_traits>
#ifdef __linux__
#include <unistd.h>
#endif
using namespace pcinsight;
namespace {
using pcinsight_example::field;
template<class Outer,class Inner>auto nested(const Metric<Outer>& parent,Inner Outer::*member){
    if(!metric_is_known(parent))return Inner{{},parent.support,"Parent unavailable: "+parent.note};
    auto result=(*parent.value).*member;
    if(parent.support==Support::estimated&&result.support==Support::available)result.support=Support::estimated;
    return result;
}
template<class Outer,class T>Metric<T> component(const Metric<Outer>& parent,T Outer::*member){
    if(!metric_is_known(parent))return {{},parent.support,"Parent unavailable: "+parent.note};
    return {(*parent.value).*member,parent.support,parent.note};
}
void label(std::ostream& out,const std::string& key,const std::string& value){
    field(out,key,"label",Metric<std::string>{value.empty()?std::optional<std::string>{}:value,
        value.empty()?Support::unsupported:Support::estimated,"Legacy provider label, not hardware attestation"});
}
void render_addresses(std::ostream& out,const Metric<std::vector<LinuxInterfaceAddressList>>& report){
    Metric<std::uint64_t> count{{},report.support,report.note};
    if(metric_is_known(report))count.value=report.value->size();
    field(out,"address_query.interface_rows","count",count);
    if(!metric_is_known(report))return;
    for(std::size_t i=0;i<report.value->size();++i){
        const auto& row=(*report.value)[i];const auto p="address_query["+std::to_string(i)+"].";
        field(out,p+"ifindex","index",Metric<std::uint32_t>{row.interface_index,report.support,report.note});
        for(std::size_t j=0;j<row.ipv4.size();++j)
            field(out,p+"ipv4["+std::to_string(j)+"]","address",Metric<std::string>{row.ipv4[j],report.support,report.note});
        for(std::size_t j=0;j<row.ipv6.size();++j)
            field(out,p+"ipv6["+std::to_string(j)+"]","address",Metric<std::string>{row.ipv6[j],report.support,report.note});
    }
}
void render(std::ostream& out,const SystemSnapshot& s){
#define F(path,unit) field(out,#path,unit,s.path)
    F(cpu.reported_vendor,"label");F(cpu.reported_model,"label");
    F(cpu.utilization_percent,"percent");F(cpu.current_ghz,"GHz");F(cpu.base_ghz,"GHz");
    F(cpu.sockets,"count");F(cpu.physical_cores,"count");F(cpu.logical_processors,"count");
    F(cpu.virtualization_enabled,"bool");F(cpu.linux_kvm_api_version,"version");
    F(cpu.l1_bytes,"B");F(cpu.l2_bytes,"B");F(cpu.l3_bytes,"B");
    F(cpu.processes,"count");F(cpu.threads,"count");F(cpu.handles,"count");F(cpu.uptime,"s");
    F(memory.reported_total_bytes,"B");F(memory.utilization_percent,"percent");
    F(memory.in_use_bytes,"B");F(memory.compressed_bytes,"B");F(memory.available_bytes,"B");
    F(memory.committed_bytes,"B");F(memory.commit_limit_bytes,"B");F(memory.cached_bytes,"B");
    F(memory.paged_pool_bytes,"B");F(memory.nonpaged_pool_bytes,"B");
    F(memory.speed_mts,"MT/s");F(memory.slots_used,"count");F(memory.slots_total,"count");
    F(memory.type,"label");F(memory.form_factor,"label");F(memory.hardware_reserved_bytes,"B");
    F(memory.linux_details.firmware_installed_bytes,"B");F(memory.linux_details.installed_minus_usable_bytes,"B");
    F(memory.linux_details.free_bytes,"B");F(memory.linux_details.buffers_bytes,"B");
    F(memory.linux_details.page_cache_bytes,"B");F(memory.linux_details.anonymous_bytes,"B");
    F(memory.linux_details.shared_bytes,"B");F(memory.linux_details.slab_reclaimable_bytes,"B");
    F(memory.linux_details.slab_unreclaimable_bytes,"B");F(memory.linux_details.dirty_bytes,"B");
    F(memory.linux_details.writeback_bytes,"B");F(memory.linux_details.zswap_pool_bytes,"B");F(memory.linux_details.zswap_original_bytes,"B");
#undef F
    const auto& composition=s.memory.linux_details.composition;
    field(out,"memory.composition.unavailable_estimate","B",component(composition,&LinuxMemoryComposition::unavailable_estimate_bytes));
    field(out,"memory.composition.reclaimable_estimate","B",component(composition,&LinuxMemoryComposition::reclaimable_estimate_bytes));
    field(out,"memory.composition.free","B",component(composition,&LinuxMemoryComposition::free_bytes));
    const auto& zram=s.memory.linux_details.zram_devices;
    Metric<std::uint64_t> zram_count{{},zram.support,zram.note};
    if(metric_is_known(zram))zram_count.value=zram.value->size();
    field(out,"memory.zram_devices","record-count",zram_count);
    if(metric_is_known(zram))for(std::size_t i=0;i<zram.value->size();++i){const auto& d=(*zram.value)[i];const auto p="zram["+std::to_string(i)+"].";
        label(out,p+"id",d.id);field(out,p+"original","B",component(d.memory,&LinuxZramMemory::original_data_bytes));
        field(out,p+"compressed","B",component(d.memory,&LinuxZramMemory::compressed_data_bytes));field(out,p+"allocated","B",component(d.memory,&LinuxZramMemory::allocated_memory_bytes));}
    for(std::size_t i=0;i<s.disks.size();++i){const auto& d=s.disks[i];const auto p="disk["+std::to_string(i)+"].";
        label(out,p+"id",d.id);label(out,p+"model",d.model);
#define D(member,unit) field(out,p+#member,unit,d.member)
        D(active_percent,"percent");D(transfer_bytes_per_second,"B/s");D(read_bytes_per_second,"B/s");D(write_bytes_per_second,"B/s");
        D(average_response_ms,"ms");D(capacity_bytes,"B");D(formatted_capacity_bytes,"B");D(system_disk,"bool");D(contains_page_file,"bool");
        D(reported_media_type,"label");D(rotational,"bool");
#undef D
        field(out,p+"protocol","label",nested(d.linux_details,&LinuxDiskDetails::device_protocol));
        field(out,p+"transport","label",nested(d.linux_details,&LinuxDiskDetails::transport));
    }
    for(std::size_t i=0;i<s.networks.size();++i){const auto& n=s.networks[i];const auto p="network["+std::to_string(i)+"].";
        label(out,p+"id",n.id);label(out,p+"name",n.name);
        field(out,p+"send","B/s",n.send_bytes_per_second);field(out,p+"receive","B/s",n.receive_bytes_per_second);
        field(out,p+"vendor","label",nested(n.linux_details,&LinuxNetworkDetails::vendor));
        field(out,p+"model","label",nested(n.linux_details,&LinuxNetworkDetails::model));
        field(out,p+"connection_type","label",nested(n.linux_details,&LinuxNetworkDetails::connection_type));
        if(n.ipv4.empty())field(out,p+"ipv4","address",Metric<std::string>{{},Support::unsupported,"No IPv4 addresses reported; absence and collection failure are indistinguishable"});
        if(n.ipv6.empty())field(out,p+"ipv6","address",Metric<std::string>{{},Support::unsupported,"No IPv6 addresses reported; absence and collection failure are indistinguishable"});
        for(std::size_t a=0;a<n.ipv4.size();++a)label(out,p+"ipv4["+std::to_string(a)+"]",n.ipv4[a]);
        for(std::size_t a=0;a<n.ipv6.size();++a)label(out,p+"ipv6["+std::to_string(a)+"]",n.ipv6[a]);
    }
    for(std::size_t i=0;i<s.gpus.size();++i){const auto& g=s.gpus[i];const auto p="gpu["+std::to_string(i)+"].";
        label(out,p+"id",g.id);label(out,p+"vendor",g.vendor);label(out,p+"model",g.model);
#define G(member,unit) field(out,p+#member,unit,g.member)
        G(utilization_percent,"percent");G(temperature_celsius,"C");G(memory_used_bytes,"B");G(memory_available_bytes,"B");
        G(dedicated_memory_bytes,"B");G(dedicated_memory_used_bytes,"B");G(shared_memory_bytes,"B");G(shared_memory_used_bytes,"B");
        G(driver_version,"version");G(driver_date,"date");G(physical_location,"PCI");G(vulkan_version,"version");G(opengl_version,"version");G(opencl_version,"version");
#undef G
        field(out,p+"gtt_total","B",nested(g.linux_sysfs_details,&LinuxGpuSysfsDetails::gtt_total_bytes));
        field(out,p+"gtt_used","B",nested(g.linux_sysfs_details,&LinuxGpuSysfsDetails::gtt_used_bytes));
        for(std::size_t e=0;e<g.engines.size();++e){const auto ep=p+"engine["+std::to_string(e)+"].";label(out,ep+"name",g.engines[e].name);field(out,ep+"utilization","percent",g.engines[e].utilization_percent);}
    }
    field(out,"gpu.inventory_source","scope",s.linux_gpu_inventory_source);
}
void check(bool condition){if(!condition)throw std::runtime_error("Performance example contract failed");}
int self_test(){
    std::ostringstream address_text;
    render_addresses(address_text,{});
    check(address_text.str().find("address_query.interface_rows\tcount\tunsupported\tunknown")!=std::string::npos);
    address_text.str("");
    render_addresses(address_text,{std::vector<LinuxInterfaceAddressList>{},Support::available,"complete empty"});
    check(address_text.str().find("address_query.interface_rows\tcount\tavailable\t0")!=std::string::npos);
    address_text.str("");
    render_addresses(address_text,{std::vector<LinuxInterfaceAddressList>{{7,{"192.0.2.1"},{"2001:db8::1"}}},Support::available,"complete"});
    check(address_text.str().find("address_query[0].ifindex\tindex\tavailable\t7")!=std::string::npos);
    check(address_text.str().find("address_query[0].ipv6[0]\taddress\tavailable\t\"2001:db8::1\"")!=std::string::npos);
    std::ostringstream text;
    field(text,"absent","B",Metric<std::uint64_t>{});
    field(text,"zero","B",Metric<std::uint64_t>{0,Support::available,{}});
    field(text,"false","bool",Metric<bool>{false,Support::available,{}});
    field(text,"estimate","percent",Metric<double>{12.5,Support::estimated,{}});
    field(text,"denied","B",Metric<std::uint64_t>{9,Support::permission_required,"blocked\nline"});
    field(text,"nan","percent",Metric<double>{std::numeric_limits<double>::quiet_NaN(),Support::available,{}});
    const auto lines=text.str();check(lines.find("absent\tB\tunsupported\tunknown")!=lines.npos);
    check(lines.find("zero\tB\tavailable\t0")!=lines.npos);check(lines.find("false\tbool\tavailable\tfalse")!=lines.npos);
    check(lines.find("estimate\tpercent\testimated\t12.5")!=lines.npos);check(lines.find("denied\tB\tpermission_required\tunknown")!=lines.npos);
    check(lines.find("nan\tpercent\tavailable\tunknown")!=lines.npos);check(lines.find("blocked\\\\x0aline")!=lines.npos);
    Metric<LinuxNetworkDetails> parent;parent.value.emplace();parent.value->vendor={"hidden",Support::available,{}};
    check(!nested(parent,&LinuxNetworkDetails::vendor).value);
    SystemSnapshot s;s.disks.resize(1);s.networks.resize(1);s.gpus.resize(1);s.gpus[0].engines.resize(3);
    s.networks[0].ipv4={"192.0.2.1","192.0.2.2"};
    std::ostringstream all;render(all,s);check(all.str().find("gpu[0].engine[2].utilization")!=std::string::npos);
    check(all.str().find("memory.paged_pool_bytes\tB\tunsupported\tunknown")!=std::string::npos);
    check(all.str().find("cpu.base_ghz\tGHz\tunsupported\tunknown")!=std::string::npos);
    check(all.str().find("memory.zram_devices\trecord-count\tunsupported\tunknown")!=std::string::npos);
    check(all.str().find("network[0].ipv4[0]\tlabel\testimated\t\"192.0.2.1\"")!=std::string::npos);
    check(all.str().find("network[0].ipv4[1]\tlabel\testimated\t\"192.0.2.2\"")!=std::string::npos);
    check(all.str().find("network[0].ipv4\taddress") == std::string::npos);
    check(all.str().find("network[0].ipv6\taddress\tunsupported\tunknown")!=std::string::npos);
    s.memory.linux_details.zram_devices={std::vector<LinuxZramDevice>{},Support::available,"known empty scope"};
    std::ostringstream empty;render(empty,s);check(empty.str().find("memory.zram_devices\trecord-count\tavailable\t0")!=std::string::npos);
    return 0;
}
}
int main(int argc,char** argv){
    if(argc==2&&std::string(argv[1])=="--self-test")return self_test();
    if(argc!=2||std::string(argv[1])!="--sample"){std::cout<<"Use --sample for two Linux read-only observations, or --self-test. No actions/capture.\n";return 0;}
#ifdef __linux__
    alarm(20);Monitor monitor;SampleOptions options;options.include_processes=false;
    monitor.sample(options);std::this_thread::sleep_for(std::chrono::milliseconds(150));
    render(std::cout,monitor.sample(options));
    // Separate query, not an atomic join to Monitor's adapter inventory.
    render_addresses(std::cout,query_linux_network_addresses());return 0;
#else
    std::cerr<<"--sample is Linux-only; portable contract test remains available.\n";return 2;
#endif
}
