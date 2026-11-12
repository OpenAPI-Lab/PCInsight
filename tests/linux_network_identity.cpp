#include "../src/linux_network_identity.hpp"
#include "pcinsight/monitor.hpp"
#include <cassert>
#include <iostream>
#ifdef __linux__
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sys/stat.h>
#endif
using namespace pcinsight;
using namespace pcinsight::detail;
int main(){
    const PciNamesDatabase db{"1234  Fixture Vendor\n\t5678  Fixture NIC\n\t\t0001 0002  Subsystem not model\n4321  Other\n\t5678  Other device\n","fixture pci.ids"};
    LinuxNetworkDetails d;d.bus={"pci",Support::available,"fixture"};d.hardware_vendor_id={0x1234,Support::available,"fixture"};d.hardware_device_id={0x5678,Support::available,"fixture"};
    apply_linux_network_pci_names(d,db);assert(d.vendor.value=="Fixture Vendor"&&d.model.value=="Fixture NIC"&&d.model.support==Support::estimated);
    for(const auto& text:std::vector<std::string>{db.text+"1234  Duplicate\n",db.text+"1234  Fixture Vendor\n\t5678  Duplicate\n","1234  Fixture Vendor\n\t5678  First\n\t5678  Duplicate\n",std::string(4097,'x'),db.text+std::string("\0",1),std::string(16*1024*1024+1,'x')}){
        apply_linux_network_pci_names(d,{text,"fixture"});assert(!d.model.value);
    }
    apply_linux_network_pci_names(d,{"1234  Fixture Vendor\nC 00  Class\n\t5678  Not a device\n","fixture"});assert(!d.model.value&&d.vendor.value);
    apply_linux_network_pci_names(d,{});assert(!d.vendor.value&&!d.model.value);
    d.hardware_vendor_id.support=Support::unsupported;apply_linux_network_pci_names(d,db);assert(!d.vendor.value&&!d.model.value);d.hardware_vendor_id.support=Support::available;
    d.bus.value="usb";apply_linux_network_pci_names(d,db);assert(!d.vendor.value&&!d.model.value);
#ifdef __linux__
    namespace fs=std::filesystem;
    char pattern[]="/tmp/pcinsight-net-id-XXXXXX";const auto* made=::mkdtemp(pattern);assert(made);
    struct Cleanup{fs::path root;~Cleanup(){std::error_code e;fs::remove_all(root,e);}} cleanup{made};const auto root=cleanup.root;
    const auto net=root/"class/net",virt=root/"devices/virtual/net";fs::create_directories(net);fs::create_directories(virt);
    auto write=[](const fs::path& p,const std::string& text){std::ofstream f(p,std::ios::binary);f<<text;f.close();assert(f);};
    auto iface=[&](std::string name,unsigned index,unsigned type,bool virtual_device=false){const auto p=(virtual_device?virt:root/"devices/pci/net")/name;fs::create_directories(p);write(p/"ifindex",std::to_string(index)+"\n");write(p/"type",std::to_string(type)+"\n");fs::create_directory_symlink(p,net/name);return p;};
    auto read=[&](std::string name,unsigned index){return read_linux_network_identity(name,index,db,net.string(),virt.string());};
    const auto renamed=iface("renamed0",1,772,true);auto result=read("renamed0",1);
    assert(result.value&&result.value->connection_type.value=="Loopback"&&result.value->virtual_device.value==true&&!result.value->vendor.value&&!result.value->model.value);
    const auto fake_lo=iface("lo",2,1,true);result=read("lo",2);assert(result.value&&result.value->connection_type.value=="Virtual Ethernet link-layer");
    const auto wifi=iface("eth9",3,1);fs::create_directory(wifi/"wireless");result=read("eth9",3);assert(result.value&&result.value->connection_type.value=="Wi-Fi");
    const auto ppp=iface("eth8",4,512);result=read("eth8",4);assert(result.value&&result.value->connection_type.value=="PPP");
    for(const auto type:{280u,768u,769u,776u,778u,823u}) {
        const auto name="eth-fixture"+std::to_string(type);
        iface(name,type,type);
        const auto link=read(name,type);
        assert(link.value&&link.value->link_layer_type==type);
        const auto* expected=type==280?"CAN":(type==778||type==823?"GRE tunnel link-layer":"IP tunnel link-layer");
        assert(link.value->connection_type.value==expected&&link.value->connection_type.support==Support::available);
    }
    const auto other=iface("wifi2",5,65534);result=read("wifi2",5);assert(result.value&&!result.value->connection_type.value&&result.value->link_layer_type==65534);
    iface("mon0",9,803);assert(read("mon0",9).value->connection_type.value=="IEEE 802.11 monitor");
    const auto pci=iface("net0",6,1);const auto hardware=root/"hardware/pci0";fs::create_directories(hardware);
    fs::create_directory_symlink(hardware,pci/"device");fs::create_symlink("/sys/bus/pci",hardware/"subsystem");fs::create_symlink("/sys/bus/pci/drivers/fixture",hardware/"driver");
    write(hardware/"vendor","0x1234\n");write(hardware/"device","0x5678\n");
    result=read("net0",6);assert(result.value&&result.value->model.value=="Fixture NIC"&&result.value->vendor.value=="Fixture Vendor"&&result.value->driver.value=="fixture"&&result.value->bus.value=="pci");
    write(hardware/"device","0xNOPE\n");result=read("net0",6);assert(result.value&&!result.value->model.value&&!result.value->hardware_device_id.value);
    fs::remove(hardware/"device");assert(::mkfifo((hardware/"device").c_str(),0600)==0);assert(read("net0",6).value&&!read("net0",6).value->model.value);fs::remove(hardware/"device");
    fs::create_symlink("vendor",hardware/"device");assert(!read("net0",6).value->hardware_device_id.value);
    write(pci/"ifindex","7\n");assert(!read("net0",6).value);write(pci/"ifindex","6\n");
    write(pci/"type","bad\n");assert(!read("net0",6).value);write(pci/"type","1\n");
    const auto usb=iface("usb0",8,1);const auto usb_parent=root/"hardware/usb0",usb_if=usb_parent/"interface";fs::create_directories(usb_if);
    fs::create_directory_symlink(usb_if,usb/"device");fs::create_symlink("/sys/bus/usb",usb_if/"subsystem");fs::create_symlink("/sys/bus/usb",usb_parent/"subsystem");
    write(usb_if/"uevent","DEVTYPE=usb_interface\n");write(usb_parent/"uevent","DEVTYPE=usb_device\n");
    write(usb_parent/"idVendor","abcd\n");write(usb_parent/"idProduct","1234\n");write(usb_parent/"manufacturer","USB Maker\n");write(usb_parent/"product","USB Adapter\n");
    result=read("usb0",8);assert(result.value&&result.value->vendor.value=="USB Maker"&&result.value->model.value=="USB Adapter"&&result.value->hardware_vendor_id.value==0xabcd&&result.value->hardware_device_id.value==0x1234);
    write(usb_parent/"product","bad\ncontrol\n");assert(!read("usb0",8).value->model.value);
    write(usb_parent/"manufacturer",std::string(4097,'x'));assert(!read("usb0",8).value->vendor.value);
    write(usb_parent/"uevent","DEVTYPE=usb_device\nDEVTYPE=usb_device\n");assert(!read("usb0",8).value->vendor.value);
    assert(!read("../net0",6).value&&!read("net0",0).value&&!read("missing",1).value&&!read(std::string("net0\0",5),6).value);
    SampleOptions options;options.include_processes=false;options.include_app_history=false;options.include_user_sessions=false;options.include_startup_entries=false;options.include_services=false;options.include_disks=false;options.include_gpus=false;
    Monitor monitor;const auto snapshot=monitor.sample(options);assert(!snapshot.networks.empty());unsigned checked=0;
    for(const auto& n:snapshot.networks){assert(n.linux_details.value);const auto& meta=*n.linux_details.value;
        assert(n.model==meta.model.value.value_or("")&&n.connection_type==meta.connection_type.value.value_or(""));
        assert(meta.interface_index>0);
        if(meta.link_layer_type==772)assert(meta.connection_type.value=="Loopback"&&!meta.vendor.value&&!meta.model.value);
        std::cout<<"Native network "<<n.name<<" ifindex="<<meta.interface_index<<" ARPHRD="<<meta.link_layer_type<<" type="<<n.connection_type<<" bus="<<meta.bus.value.value_or("unknown")<<" model="<<n.model<<'\n';++checked;}
    options.include_networks=false;assert(monitor.sample(options).networks.empty());assert(checked>0);
#else
    assert(!read_linux_network_identity("lo",1,db).value);
#endif
    std::cout<<"Network identity: source labels, PCI duplicate/missing/bounds, virtual/Wi-Fi/loopback without name inference, USB parent, invalid identity and unavailable metadata passed\n";
}
