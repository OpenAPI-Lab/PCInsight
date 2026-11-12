#include <pcinsight/monitor.hpp>
#include <pcinsight/sdk_headers.hpp>

#include <cmath>
#include <iostream>
#include <sstream>
#include <string_view>
#include <type_traits>
#ifdef __linux__
#include <unistd.h>
#endif

using namespace pcinsight;
namespace {
const char* support_text(Support support) {
    switch(support) {
    case Support::available: return "available";
    case Support::estimated: return "estimated";
    case Support::permission_required: return "permission_required";
    case Support::unsupported: return "unsupported";
    }
    return "invalid_support";
}
template<class T> void observation(std::ostream& out,const std::string& key,const Metric<T>& value) {
    bool known=metric_is_known(value);
    if constexpr(std::is_floating_point_v<T>)if(known&&!std::isfinite(*value.value))known=false;
    out<<key<<'\t'<<support_text(value.support)<<'\t';
    if(!known)out<<"unknown";
    else if constexpr(std::is_same_v<T,std::string>)out<<"known-text-not-exported";
    else out<<std::boolalpha<<*value.value;
    out<<'\n'; // Do not emit note, paths, names, or raw textual properties.
}
template<class Outer,class T> Metric<T> nested(const Metric<Outer>& parent,Metric<T> Outer::*member) {
    if(!metric_is_known(parent))return {{},parent.support,{}};
    auto result=(*parent.value).*member;
    if(parent.support==Support::estimated&&result.support==Support::available)result.support=Support::estimated;
    return result;
}
void report(std::ostream& out,const SystemSnapshot& s) {
    out<<"pcinsight-linux-hardware-evidence-v1\n";
#define M(key,path) observation(out,key,s.path)
    M("n02.cpu.vendor",cpu.reported_vendor); M("n02.cpu.model",cpu.reported_model);
    M("n02.cpu.sockets",cpu.sockets); M("n02.cpu.cores",cpu.physical_cores); M("n02.cpu.logical",cpu.logical_processors);
    M("n02.cpu.current_ghz",cpu.current_ghz); M("n02.cpu.base_ghz",cpu.base_ghz);
    M("n02.cpu.l1_bytes",cpu.l1_bytes); M("n02.cpu.l2_bytes",cpu.l2_bytes); M("n02.cpu.l3_bytes",cpu.l3_bytes);
    M("n02.cpu.kvm_api",cpu.linux_kvm_api_version); M("n02.cpu.firmware_virtualization",cpu.virtualization_enabled);
    M("n02.ram.visible_bytes",memory.reported_total_bytes); M("n02.ram.firmware_bytes",memory.linux_details.firmware_installed_bytes);
    M("n02.ram.type",memory.type); M("n02.ram.speed_mts",memory.speed_mts); M("n02.ram.slots_used",memory.slots_used);
    M("n02.ram.slots_total",memory.slots_total); M("n02.ram.form_factor",memory.form_factor);
    M("g07.inventory_source",linux_gpu_inventory_source);
#undef M
    out<<"n02.disk.observed_rows\tmetadata\t"<<s.disks.size()<<'\n';
    for(std::size_t i=0;i<s.disks.size();++i){const auto& d=s.disks[i];const auto k="n02.disk["+std::to_string(i)+"].";
        observation(out,k+"capacity_bytes",d.capacity_bytes);observation(out,k+"formatted_bytes",d.formatted_capacity_bytes);
        observation(out,k+"system",d.system_disk);observation(out,k+"swap",d.contains_page_file);
        observation(out,k+"media",d.reported_media_type);observation(out,k+"transport",nested(d.linux_details,&LinuxDiskDetails::transport));
    }
    out<<"n02.network.observed_rows\tmetadata\t"<<s.networks.size()<<'\n';
    for(std::size_t i=0;i<s.networks.size();++i){const auto& n=s.networks[i];const auto k="n02.network["+std::to_string(i)+"].";
        observation(out,k+"vendor_id",nested(n.linux_details,&LinuxNetworkDetails::hardware_vendor_id));
        observation(out,k+"device_id",nested(n.linux_details,&LinuxNetworkDetails::hardware_device_id));
        observation(out,k+"virtual",nested(n.linux_details,&LinuxNetworkDetails::virtual_device));
        observation(out,k+"wireless",nested(n.linux_details,&LinuxNetworkDetails::wireless));
        observation(out,k+"connection_type",nested(n.linux_details,&LinuxNetworkDetails::connection_type));
    }
    out<<"g07.gpu.observed_rows\tmetadata\t"<<s.gpus.size()<<'\n';
    for(std::size_t i=0;i<s.gpus.size();++i){const auto& g=s.gpus[i];const auto k="g07.gpu["+std::to_string(i)+"].";
        observation(out,k+"vendor_id",g.hardware_vendor_id);observation(out,k+"device_id",g.hardware_device_id);
        observation(out,k+"driver",nested(g.linux_sysfs_details,&LinuxGpuSysfsDetails::kernel_driver_name));
        observation(out,k+"driver_version",g.driver_version);observation(out,k+"vulkan",g.vulkan_version);
        observation(out,k+"opencl",g.opencl_version);observation(out,k+"opengl",g.opengl_version);
        observation(out,k+"location",g.physical_location);observation(out,k+"temperature_c",g.temperature_celsius);
        observation(out,k+"utilization_pct",g.utilization_percent);observation(out,k+"dedicated_bytes",g.dedicated_memory_bytes);
        observation(out,k+"used_bytes",g.memory_used_bytes);observation(out,k+"available_bytes",g.memory_available_bytes);
        out<<k<<"engine_observed_rows\tmetadata\t"<<g.engines.size()<<'\n';
        for(std::size_t j=0;j<g.engines.size();++j)observation(out,k+"engine["+std::to_string(j)+"].utilization_pct",g.engines[j].utilization_percent);
    }
}
int self_test() {
    SystemSnapshot s; s.cpu.linux_kvm_api_version={12,Support::available,"NOTE_PRIVATE"};
    s.networks.emplace_back().ipv4.push_back("IP_PRIVATE");
    auto& n=s.networks.back();n.name="IF_PRIVATE";n.linux_details.value.emplace();
    n.linux_details.support=Support::permission_required;
    n.linux_details.value->hardware_vendor_id={999,Support::available,{}};
    s.gpus.emplace_back().physical_location={"LOCATION_PRIVATE",Support::available,{}};
    s.gpus.back().engines.resize(3);s.gpus.back().engines[2].utilization_percent={0,Support::estimated,{}};
    std::ostringstream out;report(out,s);const auto text=out.str();
    for(const auto* expected:{"n02.cpu.kvm_api\tavailable\t12","n02.cpu.firmware_virtualization\tunsupported\tunknown",
        "n02.network[0].vendor_id\tpermission_required\tunknown","g07.gpu[0].location\tavailable\tknown-text-not-exported",
        "g07.gpu[0].engine[2].utilization_pct\testimated\t0"})if(text.find(expected)==text.npos)return 1;
    return text.find("_PRIVATE")==text.npos && text.find("999")==text.npos ? 0 : 1;
}
}
int main(int argc,char** argv) {
    require_matching_sdk_headers();
    if(argc==2&&std::string_view(argv[1])=="--self-test")return self_test();
#ifdef __linux__
    if(argc==2&&std::string_view(argv[1])=="--sample"){
        alarm(20);Monitor monitor;SampleOptions options;
        options.include_processes=false;options.include_linux_process_descriptions=false;
        options.include_disks=options.include_networks=options.include_gpus=true;
        report(std::cout,monitor.sample(options));return 0;
    }
#endif
    std::cerr<<"Usage: linux-hardware-evidence --sample or --self-test\n";return 2;
}
