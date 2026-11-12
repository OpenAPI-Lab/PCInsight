#include "../src/linux_disk_identity.hpp"
#include "pcinsight/monitor.hpp"
#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#ifdef __linux__
#include <cstdlib>
#include <sys/stat.h>
#endif
using namespace pcinsight;
using namespace pcinsight::detail;
Metric<std::string> present(std::string value){return {std::move(value),Support::available,"fixture"};}
std::string page(unsigned code,std::size_t length){std::string s(length,'\0');s[1]=static_cast<char>(code);s[2]=static_cast<char>((length-4)>>8);s[3]=static_cast<char>(length-4);return s;}
void word(std::string& s,unsigned index,unsigned value){s[60+index*2]=static_cast<char>(value);s[61+index*2]=static_cast<char>(value>>8);}
std::string ata(unsigned rate=1){auto s=page(0x89,572);s[56]=static_cast<char>(0xec);word(s,80,0x100);word(s,76,0xe);word(s,217,rate);return s;}
std::string b1(unsigned rate){auto s=page(0xb1,64);s[4]=static_cast<char>(rate>>8);s[5]=static_cast<char>(rate);return s;}
std::string port(unsigned protocol){auto s=page(0x83,12);s[4]=static_cast<char>((protocol<<4)|1);s[5]=static_cast<char>(0x94);s[7]=4;s[10]=1;s[11]=2;return s;}
int main(){
    LinuxDiskIdentityInputs in;in.subsystem=present("scsi");in.ata_vpd=present(ata());
    auto d=parse_linux_disk_identity(in);
    assert(d.device_protocol.value=="SATA"&&d.media_type.value=="SSD"&&d.media_type.support==Support::estimated);
    assert(!d.transport.value&&!d.nominal_rotation_rpm.value);
    in.ata_vpd=present(ata(7200));d=parse_linux_disk_identity(in);
    assert(d.media_type.value=="HDD"&&d.nominal_rotation_rpm.value==7200);
    auto s=ata();word(s,76,0xffff);in.ata_vpd=present(s);assert(parse_linux_disk_identity(in).device_protocol.value=="ATA");
    word(s,76,0xf);in.ata_vpd=present(s);assert(parse_linux_disk_identity(in).device_protocol.value=="ATA");
    in.ata_vpd=present(ata());in.identification_vpd=present(port(6));d=parse_linux_disk_identity(in);
    assert(d.device_protocol.value=="SATA"&&d.transport.value=="SAS"); // different layers, not conflicting evidence
    in.ata_vpd={};in.characteristics_vpd=present(b1(10000));d=parse_linux_disk_identity(in);
    assert(d.media_type.value=="HDD"&&d.nominal_rotation_rpm.value==10000&&d.transport.value=="SAS");
    in.characteristics_vpd=present(b1(1));d=parse_linux_disk_identity(in);
    assert(d.media_type.value=="non-rotating"&&!d.nominal_rotation_rpm.value); // not invented SSD or 0 RPM
    for(unsigned rate:{0u,2u,0x400u,0xffffu}){in.characteristics_vpd=present(b1(rate));d=parse_linux_disk_identity(in);assert(!d.media_type.value&&!d.nominal_rotation_rpm.value);}
    for(unsigned rate:{0x401u,0xfffeu}){in.characteristics_vpd=present(b1(rate));assert(parse_linux_disk_identity(in).nominal_rotation_rpm.value==rate);}
    in.ata_vpd=present(ata());in.characteristics_vpd=present(b1(7200));d=parse_linux_disk_identity(in);
    assert(!d.media_type.value&&!d.nominal_rotation_rpm.value&&d.media_type.note.find("Conflicting")!=std::string::npos);
    in.characteristics_vpd=present(b1(1));assert(parse_linux_disk_identity(in).media_type.value=="SSD");
    in.characteristics_vpd={};
    for(unsigned which=0;which<5;++which){
        s=b1(7200);
        if(which==0)s.pop_back();
        if(which==1)s[1]=0;
        if(which==2)s[0]=0x20;
        if(which==3)s=page(0xb1,63);
        if(which==4)s=page(0xb1,65540);
        in.ata_vpd={};in.characteristics_vpd=present(s);assert(!parse_linux_disk_identity(in).media_type.value);
    }
    in.characteristics_vpd={};
    for(unsigned which=0;which<8;++which){
        s=ata();
        if(which==0)s.pop_back();if(which==1)s.push_back('\0');if(which==2)s[1]=0;
        if(which==3)s[56]=static_cast<char>(0xa1);if(which==4)word(s,0,0x8000);
        if(which==5)word(s,80,0x20);if(which==6)word(s,80,0xffff);
        if(which==7){s[570]=static_cast<char>(0xa5);s[571]=0;unsigned sum=0;for(std::size_t i=60;i<s.size();++i)sum+=static_cast<unsigned char>(s[i]);s[571]=static_cast<char>(1-(sum&255));}
        in.ata_vpd=present(s);d=parse_linux_disk_identity(in);assert(d.device_protocol.value=="SCSI"&&!d.media_type.value);
    }
    s=ata();s[570]=static_cast<char>(0xa5);s[571]=0;unsigned sum=0;for(std::size_t i=60;i<s.size();++i)sum+=static_cast<unsigned char>(s[i]);s[571]=static_cast<char>(0-(sum&255));
    in.ata_vpd=present(s);assert(parse_linux_disk_identity(in).media_type.value=="SSD");
    in.ata_vpd={};
    in.ata_vpd={{},Support::permission_required,"fixture denied"};
    d=parse_linux_disk_identity(in);
    assert(d.media_type.support==Support::permission_required&&d.nominal_rotation_rpm.support==Support::permission_required);
    assert(!d.media_type.value&&!d.nominal_rotation_rpm.value);
    assert(d.nominal_rotation_rpm.note.find("permission")!=std::string::npos);
    in.characteristics_vpd=present(b1(7200));d=parse_linux_disk_identity(in);
    assert(d.media_type.value=="HDD"&&d.nominal_rotation_rpm.value==7200); // independent valid report survives
    in.characteristics_vpd={};
    in.ata_vpd={ata(),Support::unsupported,"inconsistent fixture: ignore stored value"};
    assert(!parse_linux_disk_identity(in).media_type.value);in.ata_vpd={};
    for(unsigned which=0;which<7;++which){
        s=port(6);
        if(which==0)s[5]=0x14; // PIV absent
        if(which==1)s[5]=static_cast<char>(0x84); // logical unit, not port
        if(which==2)s[7]=5;if(which==3)s[7]=0;if(which==4)s[6]=1;
        if(which==5){s+=port(5).substr(4);s[3]=16;} // conflicting protocols
        if(which==6){s+='x';s[3]=9;} // truncated trailing descriptor after a valid one
        in.identification_vpd=present(s);assert(!parse_linux_disk_identity(in).transport.value);
    }
    for(const auto& pair:std::vector<std::pair<unsigned,std::string>>{{0,"Fibre Channel"},{1,"Parallel SCSI"},{3,"IEEE 1394"},{4,"SRP"},{5,"iSCSI"},{6,"SAS"},{8,"ATA"}}){in.identification_vpd=present(port(pair.first));assert(parse_linux_disk_identity(in).transport.value==pair.second);}
    in.subsystem=present("nvme");
    for(const auto& t:{"pcie","tcp","rdma","fc","loop"}){in.nvme_transport=present(std::string(t)+"\n");d=parse_linux_disk_identity(in);assert(d.device_protocol.value=="NVMe"&&d.transport.value==t&&!d.media_type.value);}
    for(const auto& t:std::vector<std::string>{"","PCIe"," pcie","pcie x",std::string("pcie\0",5),std::string(129,'x')}){in.nvme_transport=present(t);assert(!parse_linux_disk_identity(in).transport.value);}
    in.nvme_transport={{},Support::permission_required,"fixture denied"};assert(parse_linux_disk_identity(in).transport.support==Support::permission_required);
    in.subsystem=present("virtio");d=parse_linux_disk_identity(in);assert(!d.device_protocol.value&&!d.media_type.value&&!d.transport.value);
    // Other-layer VPD/transport metadata must not manufacture a backing medium.
    for(const auto& sub:{"virtio","usb","block","pci"}){
        in={};in.subsystem=present(sub);in.ata_vpd=present(ata());in.characteristics_vpd=present(b1(7200));
        in.identification_vpd=present(port(6));in.nvme_transport=present("pcie");
        d=parse_linux_disk_identity(in);
        assert(d.kernel_subsystem.value==sub&&!d.device_protocol.value&&!d.transport.value&&!d.media_type.value&&!d.nominal_rotation_rpm.value);
    }
    in={};in.subsystem=present("nvme");in.nvme_transport=present("tcp");
    in.ata_vpd=present(ata());in.characteristics_vpd=present(b1(7200));d=parse_linux_disk_identity(in);
    assert(d.device_protocol.value=="NVMe"&&d.transport.value=="tcp"&&!d.media_type.value&&!d.nominal_rotation_rpm.value);
    assert(d.media_type.note.find("not medium evidence")!=std::string::npos);
    for(const auto& sub:std::vector<std::string>{"","../scsi","scsi\n",std::string("scsi\0",5),std::string(129,'x')}){in.subsystem=present(sub);d=parse_linux_disk_identity(in);assert(!d.kernel_subsystem.value&&!d.media_type.value);}
    DiskInfo disk;disk.id="nvme-fake";disk.bus_type="scsi";disk.media_type="non-rotational";
    in={};in.subsystem=present("scsi");in.ata_vpd=present(ata());
    apply_linux_disk_identity(disk,{parse_linux_disk_identity(in),Support::available,"fixture"});assert(disk.reported_media_type.value=="SSD");
    assert(disk.bus_type=="scsi"&&disk.media_type=="non-rotational"); // legacy hint not overwritten
    in={};in.subsystem=present("nvme");in.nvme_transport=present("pcie");
    apply_linux_disk_identity(disk,{parse_linux_disk_identity(in),Support::available,"fixture NVMe without medium"});
    assert(disk.linux_details.value&&disk.linux_details.value->device_protocol.value=="NVMe"&&!disk.reported_media_type.value);
    assert(!disk.reported_media_type.note.empty()&&disk.media_type=="non-rotational"); // no stale SSD or promotion of queue hint
    apply_linux_disk_identity(disk,{{},Support::permission_required,"metadata denied"});
    assert(!disk.linux_details.value&&!disk.reported_media_type.value&&disk.reported_media_type.support==Support::permission_required);
    apply_linux_disk_identity(disk,{{},Support::unsupported,"gone"});assert(!disk.linux_details.value&&!disk.reported_media_type.value);
#ifdef __linux__
    namespace fs=std::filesystem;
    char pattern[]="/tmp/pcinsight-disk-identity-XXXXXX";const auto made=::mkdtemp(pattern);assert(made);
    struct Cleanup{fs::path root;~Cleanup(){std::error_code e;fs::remove_all(root,e);}} cleanup{made};
    const auto root=cleanup.root,dev=root/"arbitrary-name"/"device";fs::create_directories(dev);
    auto write=[](const fs::path& path,const std::string& content){std::ofstream f(path,std::ios::binary);f.write(content.data(),content.size());f.close();assert(f);};
    write(dev.parent_path()/"dev","8:16\n");write(dev.parent_path()/"diskseq","42\n");
    fs::create_symlink("../../bus/scsi",dev/"subsystem");write(dev/"vpd_pg89",ata());write(dev/"vpd_pgb1",b1(1));write(dev/"vpd_pg83",port(6));
    auto result=read_linux_disk_identity("arbitrary-name",root.string());assert(result.value&&result.value->media_type.value=="SSD"&&result.value->transport.value=="SAS");
    for(unsigned change=0;change<6;++change){
        const auto mutate=[&]{
            if(change==0)write(dev.parent_path()/"dev","8:17\n");
            if(change==1)write(dev.parent_path()/"diskseq","43\n");
            if(change==2)write(dev/"vpd_pg89",ata(7200));
            if(change==3){fs::remove(dev/"subsystem");fs::create_symlink("../../bus/virtio",dev/"subsystem");}
            if(change==4){fs::rename(dev,root/"old-device");fs::create_directories(dev);}
            if(change==5){fs::rename(dev.parent_path(),root/"old-disk");fs::create_directories(dev.parent_path());}
        };
        result=read_linux_disk_identity("arbitrary-name",root.string(),mutate);assert(!result.value);
        if(change==4){fs::remove(dev);fs::rename(root/"old-device",dev);}
        if(change==5){fs::remove(dev.parent_path());fs::rename(root/"old-disk",dev.parent_path());}
        write(dev.parent_path()/"dev","8:16\n");write(dev.parent_path()/"diskseq","42\n");write(dev/"vpd_pg89",ata());
        if(change==3){fs::remove(dev/"subsystem");fs::create_symlink("../../bus/scsi",dev/"subsystem");}
    }
    write(dev/"vpd_pg89",ata(7200));write(dev/"vpd_pgb1",b1(7200));result=read_linux_disk_identity("arbitrary-name",root.string());assert(result.value&&result.value->nominal_rotation_rpm.value==7200);
    fs::remove(dev/"vpd_pg89");assert(::mkfifo((dev/"vpd_pg89").c_str(),0600)==0);
    result=read_linux_disk_identity("arbitrary-name",root.string());assert(result.value&&result.value->media_type.value=="HDD"); // B1 remains independent, FIFO never read
    write(dev/"vpd_pgb1",std::string(65540,'x'));result=read_linux_disk_identity("arbitrary-name",root.string());assert(result.value&&!result.value->media_type.value);
    fs::remove(dev/"vpd_pg89");write(root/"other-ata",ata());fs::create_symlink(root/"other-ata",dev/"vpd_pg89");
    write(dev/"vpd_pgb1",b1(7200));result=read_linux_disk_identity("arbitrary-name",root.string());
    assert(result.value&&result.value->device_protocol.value=="SCSI"&&result.value->media_type.value=="HDD"); // attribute symlink is not followed
    fs::remove(dev/"subsystem");fs::create_symlink("../../bus/nvme",dev/"subsystem");write(dev/"transport","tcp\n");
    result=read_linux_disk_identity("arbitrary-name",root.string());assert(result.value&&result.value->device_protocol.value=="NVMe"&&result.value->transport.value=="tcp"&&!result.value->media_type.value);
    fs::remove(dev.parent_path()/"diskseq");assert(read_linux_disk_identity("arbitrary-name",root.string()).value);
    write(dev.parent_path()/"dev","bad");result=read_linux_disk_identity("arbitrary-name",root.string());assert(!result.value&&result.support==Support::unsupported);
    write(dev.parent_path()/"dev","8:16");write(dev.parent_path()/"diskseq","-1");assert(!read_linux_disk_identity("arbitrary-name",root.string()).value);
    for(const auto& name:{"../escape",".","a/b","nvme-fake"})assert(!read_linux_disk_identity(name,root.string()).value);
    SampleOptions options;options.include_processes=false;options.include_networks=false;options.include_gpus=false;
    Monitor monitor;const auto native=monitor.sample(options);assert(!native.disks.empty());unsigned pinned=0,classified=0;
    for(const auto& row:native.disks){
        if(row.linux_details.value){++pinned;const auto& details=*row.linux_details.value;assert(row.reported_media_type.value==details.media_type.value);if(details.media_type.value)++classified;}
        else assert(!row.reported_media_type.value);
    }
    std::cout<<"Native Linux disk identity: "<<pinned<<" pinned rows, "<<classified<<" medium reports (virtual host need not expose physical medium)\n";
#else
    assert(!read_linux_disk_identity("anything").value);
#endif
    std::cout<<"Disk identity: ATA/SATA, SCSI target-port protocols, NVMe transports, rotation reports, conflicts, bounds, malformed inputs and clearing passed\n";
}
