#include "../src/linux_gpu_names.hpp"
#include "../src/linux_nvml.hpp"
#include "pcinsight/monitor.hpp"
#include <cassert>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <sys/stat.h>
#include <unistd.h>
using namespace pcinsight;
using namespace pcinsight::detail;
std::vector<GpuInfo> targets(){
    std::vector<GpuInfo> g(4);
    for(auto& v:g){v.vendor="0x1002";v.model="0x744c";v.physical_location={std::string("0000:03:00.0"),Support::available,"DRM"};}
    g[1].model="0x9999";g[2].vendor="0x10de";g[2].model="0x1234";g[3].physical_location.value="0001:03:00.0";return g;
}
PciNamesDatabase db(){return {"# Synthetic database, not a hardware certification\n1002  AMD fixture\n\t744c  Navi fixture\n\t\t1002 0001  Board must not replace device\n\t9999  Other fixture\n10de  NVIDIA fixture\n\t1234  GPU fixture\nC 03  Display controller\n\t00  VGA\nS 1002\n\t744c  Not a device name\nZ future extension\n\t1234  Not a GPU\n","fixture:pci.ids"};}
int main(){
    auto g=targets();fill_linux_gpu_names(g,db());
    assert(g[0].vendor=="AMD fixture" && g[0].model=="Navi fixture");
    assert(g[0].hardware_vendor_id.value==0x1002 && g[0].hardware_device_id.value==0x744c);
    assert(g[0].pci_device_name.support==Support::estimated && g[0].pci_device_name.note.find("fixture:pci.ids")!=std::string::npos);
    assert(g[1].model=="Other fixture" && g[2].model=="GPU fixture" && g[3].model=="Navi fixture");
    auto test=[&](std::string text){auto v=targets();fill_linux_gpu_names(v,{std::move(text),"fixture"});return v;};
    for(const auto& text:{"", "# only comments\n", "1003  Wrong vendor\n\t744c  Wrong GPU\n", "1002  AMD\n\t744d  Other\n", "1002  AMD\nC 03  Display\n\t744c  Wrong\n", "1002  AMD\nS 1002\n\t744c  Wrong\n", "1002  AMD\nZ future\n\t744c  Wrong\n", "1002  AMD\nzzzz malformed\n\t744c  Wrong\n", "1002  AMD\n 744c  Wrong indent\n", "1002  AMD\n\t744cX Bad separator\n", "1002  AMD\n\t+44c  Invalid hex\n", "1002  AMD\n\t744c  \n"})assert(!test(text)[0].pci_device_name.value);
    assert(test("1002  AMD\r\n\t744C  GPU\r\n")[0].model=="GPU");
    assert(test("1002  AMD\n\t744c  GPU")[0].model=="GPU");
    assert(!test("1002  AMD\n\t744c  First\n\t744c  Second\n")[0].pci_device_name.value);
    assert(!test("1002  AMD\n\t744c  First\n1002  AMD\n\t744c  First\n")[0].pci_vendor_name.value);
    assert(!test("1002  AMD\n\t744c  GPU\n"+std::string(4097,'x'))[0].pci_device_name.value);
    assert(!test(std::string(16*1024*1024+1,'x'))[0].pci_device_name.value);
    const char nul_name[]="1002  AMD\n\t744c  G\0PU";
    assert(!test(std::string(nul_name,sizeof(nul_name)-1))[0].pci_device_name.value);
    assert(!test("1002  AMD\n\t744c  Bad\tName")[0].pci_device_name.value);
    assert(!test("1002  AMD\n\t744c  Bad\x01Name")[0].pci_device_name.value);
    g=targets();fill_linux_gpu_names(g,{db().text,{}});assert(!g[0].pci_device_name.value && g[0].model=="0x744c");
    g=targets();g[0].physical_location.support=Support::estimated;g[1].physical_location.value.reset();g[2].physical_location.value="platform-device";g[3].model="Driver model";fill_linux_gpu_names(g,db());for(const auto& v:g)assert(!v.pci_device_name.value);
    g=targets();g[0].vendor="0x10020";g[1].model="0x74zz";g[2].vendor="+1002";g[3].model="0x1";fill_linux_gpu_names(g,db());for(const auto& v:g)assert(!v.hardware_device_id.value);
    g=targets();g.resize(257,g[0]);fill_linux_gpu_names(g,db());assert(!g[0].pci_device_name.value && g[0].hardware_device_id.value==0x744c);
    // Optional names must not break NVML failure propagation or its authoritative model.
    g=targets();fill_linux_gpu_names(g,db());NvmlSnapshot failure;failure.support=Support::permission_required;failure.note="denied";merge_linux_nvml(g,failure);assert(g[2].temperature_celsius.support==Support::permission_required);
    NvmlSnapshot n;n.support=Support::available;GpuInfo nv;nv.vendor="NVIDIA";nv.model="Driver model";nv.physical_location={std::string("0000:03:00.0"),Support::available,"NVML"};n.gpus.push_back(nv);
    g={targets()[2]};fill_linux_gpu_names(g,db());merge_linux_nvml(g,n);assert(g.size()==1 && g[0].model=="Driver model" && g[0].pci_device_name.value=="GPU fixture");
    char temp[]="/tmp/pcinsight-pci-names-XXXXXX";auto* created=mkdtemp(temp);assert(created);
    struct Cleanup {std::filesystem::path path;~Cleanup(){std::error_code ec;std::filesystem::remove_all(path,ec);}} cleanup{created};
    const auto root=cleanup.path;const auto file=(root/"pci.ids").string(),fifo=(root/"fifo").string(),large=(root/"large").string();
    {std::ofstream out(file);out<<db().text;assert(out.good());}
    assert(mkfifo(fifo.c_str(),0600)==0);
    auto loaded=load_linux_pci_names({(root/"missing").string(),root.string(),fifo,file});assert(loaded.text==db().text && loaded.source==file);
    assert(load_linux_pci_names({fifo,root.string()}).text.empty());
    {std::ofstream out(large);out.seekp(16*1024*1024);out.put('x');assert(out.good());}
    assert(load_linux_pci_names({large}).text.empty());
    if(geteuid()!=0){assert(chmod(file.c_str(),0000)==0);assert(load_linux_pci_names({file}).text.empty());assert(chmod(file.c_str(),0600)==0);}
    assert(load_linux_pci_names({}).text.empty());
    const auto native=load_linux_pci_names();std::cout<<"Native PCI database: "<<(native.source.empty()?"unavailable":native.source)<<" bytes="<<native.text.size()<<'\n';
    SampleOptions options;options.include_gpus=true;Monitor monitor;for(const auto& v:monitor.sample(options).gpus)if(v.pci_device_name.value)assert(v.pci_device_name.support==Support::estimated);
    std::cout<<"PCI GPU names tests passed\n";
}
