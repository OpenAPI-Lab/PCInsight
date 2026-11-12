#include "linux_disk_identity.hpp"
#include "linux_disk_rates.hpp"
#include <array>
#include <charconv>
#ifdef __linux__
#include <cerrno>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#endif
namespace pcinsight::detail {
namespace {
template<class T> Metric<T> missing(std::string note,Support support=Support::unsupported){return {{},support,std::move(note)};}
bool known(const Metric<std::string>& m){return metric_is_known(m);}
unsigned byte(std::string_view s,std::size_t i){return static_cast<unsigned char>(s[i]);}
unsigned be16(std::string_view s,std::size_t i){return byte(s,i)*256+byte(s,i+1);}
unsigned le16(std::string_view s,std::size_t i){return byte(s,i)+byte(s,i+1)*256;}
bool page(std::string_view s,unsigned code,std::size_t minimum){
    return s.size()>=minimum&&s.size()<=65539&&(byte(s,0)==0||byte(s,0)==0x14)&&byte(s,1)==code&&be16(s,2)+4==s.size();
}
std::string_view trim(std::string_view s){while(!s.empty()&&(s.back()=='\n'||s.back()=='\r'||s.back()==' '||s.back()=='\t'))s.remove_suffix(1);return s;}
bool token(std::string_view s){return !s.empty()&&s.size()<=128&&s.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_-")==s.npos;}
Metric<std::string> absent_source(const Metric<std::string>& source,const char* note){
    return missing<std::string>(note,source.support==Support::permission_required?Support::permission_required:Support::unsupported);
}
std::optional<unsigned> rotation(unsigned value){if(value==1||(value>=0x401&&value<=0xfffe))return value;return {};}
Metric<std::string> port_transport(const Metric<std::string>& input){
    auto absent=absent_source(input,"No unambiguous supported target-port protocol in SCSI VPD83");
    if(!known(input)||!page(*input.value,0x83,4))return absent;
    std::string_view s=*input.value;std::optional<unsigned> protocol;
    for(std::size_t i=4;i<s.size();){
        if(s.size()-i<4)return absent;
        const auto length=byte(s,i+3),flags=byte(s,i+1),codeset=byte(s,i)&15;
        if(length>s.size()-i-4||!length||byte(s,i+2)!=0||(flags&0x40))return absent;
        // Only PIV target-port designators with defined binary NAA/EUI/relative-port layouts.
        if((flags&0xb0)==0x90){
            const auto type=flags&15;
            bool valid_designator=codeset==1&&((type==2&&(length==8||length==12||length==16))||
                (type==3&&((length==8&&((byte(s,i+4)>>4)==2||(byte(s,i+4)>>4)==3||(byte(s,i+4)>>4)==5))||(length==16&&(byte(s,i+4)>>4)==6)))||
                (type==4&&length==4&&byte(s,i+4)==0&&byte(s,i+5)==0));
            if(valid_designator){const auto p=byte(s,i)>>4;if(protocol&&*protocol!=p)return absent;protocol=p;}
        }
        i+=4+length;
    }
    if(!protocol)return absent;
    const char* label=nullptr;
    switch(*protocol){case 0:label="Fibre Channel";break;case 1:label="Parallel SCSI";break;case 3:label="IEEE 1394";break;case 4:label="SRP";break;case 5:label="iSCSI";break;case 6:label="SAS";break;case 8:label="ATA";break;default:return absent;}
    return {std::string(label),Support::estimated,"Device-reported SCSI VPD83 PIV target-port protocol; not host wiring or physical backing-medium proof"};
}
}
LinuxDiskDetails parse_linux_disk_identity(const LinuxDiskIdentityInputs& input){
    LinuxDiskDetails d;
    d.kernel_subsystem=absent_source(input.subsystem,"Kernel device subsystem unavailable/invalid");
    d.device_protocol=missing<std::string>("No supported device-protocol evidence");
    d.transport=missing<std::string>("No supported target/controller transport evidence");
    d.media_type=missing<std::string>("No supported physical-medium report; queue/rotational and NVMe protocol are not medium evidence");
    d.nominal_rotation_rpm=missing<std::uint32_t>("No valid nominal rotation rate reported; non-rotating is not zero RPM measured");
    if(!known(input.subsystem)||!token(*input.subsystem.value))return d;
    d.kernel_subsystem={*input.subsystem.value,Support::available,"Kernel device/subsystem link basename"};
    if(*input.subsystem.value=="nvme"){
        d.device_protocol={"NVMe",Support::available,"Kernel device subsystem; does not establish SSD or a local PCIe connection"};
        if(known(input.nvme_transport)){
            const auto t=trim(*input.nvme_transport.value);
            if(input.nvme_transport.value->size()<=128&&(t=="pcie"||t=="tcp"||t=="rdma"||t=="fc"||t=="loop"))
                d.transport={std::string(t),Support::available,"NVMe controller transport attribute; not physical medium"};
        }else d.transport=absent_source(input.nvme_transport,"NVMe controller transport unavailable");
        return d;
    }
    if(*input.subsystem.value!="scsi")return d;
    d.device_protocol={"SCSI",Support::available,"Kernel SCSI subsystem; does not imply SAS, SATA or a physical disk"};
    d.transport=port_transport(input.identification_vpd);
    std::optional<unsigned> ata_rate,b1_rate;bool ata=false;
    if(known(input.ata_vpd)&&page(*input.ata_vpd.value,0x89,572)){
        const std::string_view s=*input.ata_vpd.value;
        // SAT ATA Information has a fixed 512-byte IDENTIFY payload at offset60.
        if(s.size()==572&&byte(s,56)==0xec&&(le16(s,60)&0x8000)==0){
            const auto version=le16(s,60+80*2),cap=le16(s,60+76*2);
            bool checksum=true;
            if(byte(s,60+510)==0xa5){unsigned sum=0;for(std::size_t i=60;i<572;++i)sum+=byte(s,i);checksum=(sum&255)==0;}
            if(checksum&&version!=0&&!(version&0x8000)&&(version&0x7f80)){
                ata=true;ata_rate=rotation(le16(s,60+217*2));
                const bool sata=cap!=0&&cap!=0xffff&&!(cap&1)&&(cap&0x0e);
                d.device_protocol={sata?"SATA":"ATA",Support::estimated,sata?
                    "SAT VPD89 ATA IDENTIFY word76 advertises SATA link capability; bridge/host attachment may differ":
                    "SAT VPD89 IDENTIFY DEVICE, ATA7+; no positive SATA capability; not proof of PATA"};
            }
        }
    }
    if(known(input.characteristics_vpd)&&page(*input.characteristics_vpd.value,0xb1,64))
        b1_rate=rotation(be16(*input.characteristics_vpd.value,4));
    if(ata_rate&&b1_rate&&*ata_rate!=*b1_rate){
        d.media_type=missing<std::string>("Conflicting ATA IDENTIFY and SCSI VPD B1 rotation reports");
        d.nominal_rotation_rpm=missing<std::uint32_t>(d.media_type.note);return d;
    }
    const auto rate=ata_rate?ata_rate:b1_rate;
    if(rate){
        const auto source=ata_rate?"SAT VPD89 ATA IDENTIFY word217":"SCSI VPD B1 nominal medium rotation rate";
        if(*rate==1){
            // SCSI's non-rotating report alone is not a technology-specific SSD report.
            if(ata&&ata_rate)d.media_type={"SSD",Support::estimated,std::string(source)+"=1 (ATA solid-state classification); device/bridge-reported, not authenticated physical backing"};
            else d.media_type={"non-rotating",Support::estimated,std::string(source)+"=1; does not distinguish SSD, RAM or emulated backing"};
        }else{
            d.media_type={"HDD",Support::estimated,std::string(source)+" reports a rotating medium; device/bridge-reported, not authenticated physical backing"};
            d.nominal_rotation_rpm={static_cast<std::uint32_t>(*rate),Support::estimated,std::string(source)+"; nominal RPM, not live speed"};
        }
    }else if(input.ata_vpd.support==Support::permission_required||input.characteristics_vpd.support==Support::permission_required){
        d.media_type=missing<std::string>("Medium identification page requires permission; no valid medium report",Support::permission_required);
        d.nominal_rotation_rpm=missing<std::uint32_t>("Rotation identification page requires permission; no valid nominal RPM report",Support::permission_required);
    }
    return d;
}
#ifdef __linux__
namespace {
struct Fd{int n;explicit Fd(int value):n(value){}~Fd(){if(n>=0)::close(n);}Fd(const Fd&)=delete;};
Support access_support(int e){return e==EACCES||e==EPERM?Support::permission_required:Support::unsupported;}
Metric<std::string> attr(int dir,const char* name,std::size_t limit){
    Fd f(::openat(dir,name,O_RDONLY|O_CLOEXEC|O_NONBLOCK|O_NOFOLLOW));
    if(f.n<0)return missing<std::string>(std::string("Cannot open sysfs ")+name,access_support(errno));
    struct stat st{};if(::fstat(f.n,&st)||!S_ISREG(st.st_mode))return missing<std::string>("Attribute is not a regular file");
    std::array<char,1024> buffer{};std::string value;
    while(true){const auto n=::read(f.n,buffer.data(),buffer.size());
        if(n<0){if(errno==EINTR)continue;return missing<std::string>("Sysfs attribute read failed",access_support(errno));}
        if(n==0)break;
        if(static_cast<std::size_t>(n)>limit-value.size())return missing<std::string>("Oversized sysfs attribute");
        value.append(buffer.data(),static_cast<std::size_t>(n));}
    return {std::move(value),Support::available,std::string("Bounded sysfs ")+name};
}
Metric<std::string> subsystem(int dir){
    std::array<char,4096> b{};const auto n=::readlinkat(dir,"subsystem",b.data(),b.size());
    if(n<0)return missing<std::string>("Cannot read device/subsystem",access_support(errno));
    if(static_cast<std::size_t>(n)>=b.size())return missing<std::string>("Oversized subsystem link");
    const std::string_view s(b.data(),static_cast<std::size_t>(n));const auto slash=s.find_last_of('/');const auto base=s.substr(slash==s.npos?0:slash+1);
    if(!token(base))return missing<std::string>("Invalid subsystem link");
    return {std::string(base),Support::available,"Device subsystem link"};
}
bool same_dir(int parent,const char* name,int dir){struct stat a{},b{};return !::fstat(dir,&a)&&!::fstatat(parent,name,&b,0)&&a.st_dev==b.st_dev&&a.st_ino==b.st_ino;}
bool same(const Metric<std::string>& a,const Metric<std::string>& b){return a.value==b.value&&a.support==b.support;}
}
#endif
Metric<LinuxDiskDetails> read_linux_disk_identity(const std::string& name,const std::string& root,const std::function<void()>& after_capture){
#ifdef __linux__
    const auto bad=[](const char* note,Support s=Support::unsupported){return missing<LinuxDiskDetails>(note,s==Support::permission_required?s:Support::unsupported);};
    if(!token(name)||name=="."||name==".."||root.empty()||root.size()>65536||root.find('\0')!=root.npos)return bad("Invalid disk identity/path");
    Fd base(::open(root.c_str(),O_RDONLY|O_DIRECTORY|O_CLOEXEC));if(base.n<0)return bad("Cannot open block sysfs root",access_support(errno));
    Fd disk(::openat(base.n,name.c_str(),O_RDONLY|O_DIRECTORY|O_CLOEXEC));if(disk.n<0)return bad("Cannot pin block sysfs directory",access_support(errno));
    const auto dev=attr(disk.n,"dev",128),seq=attr(disk.n,"diskseq",128);
    if(!known(dev)||!valid_linux_disk_device(std::string(trim(*dev.value))))return bad("Block major:minor unavailable/invalid",dev.support);
    if(known(seq)){
        const auto s=trim(*seq.value);std::uint64_t v{};const auto [end,error]=std::from_chars(s.data(),s.data()+s.size(),v);
        if(s.empty()||error!=std::errc{}||end!=s.data()+s.size())return bad("Invalid block diskseq");
    }
    Fd device(::openat(disk.n,"device",O_RDONLY|O_DIRECTORY|O_CLOEXEC));
    if(device.n<0)return bad("No accessible device directory for disk metadata",access_support(errno));
    const auto capture=[&]{LinuxDiskIdentityInputs in;in.subsystem=subsystem(device.n);
        if(in.subsystem.value=="scsi"){
            in.ata_vpd=attr(device.n,"vpd_pg89",572);in.characteristics_vpd=attr(device.n,"vpd_pgb1",65539);in.identification_vpd=attr(device.n,"vpd_pg83",65539);
        }else if(in.subsystem.value=="nvme")in.nvme_transport=attr(device.n,"transport",128);
        return in;};
    const auto before=capture();const auto details=parse_linux_disk_identity(before);
    if(after_capture)after_capture();
    const auto after=capture();
    if(!same_dir(base.n,name.c_str(),disk.n)||!same_dir(disk.n,"device",device.n)||
       !same(dev,attr(disk.n,"dev",128))||!same(seq,attr(disk.n,"diskseq",128))||
       !same(before.subsystem,after.subsystem)||!same(before.ata_vpd,after.ata_vpd)||!same(before.characteristics_vpd,after.characteristics_vpd)||
       !same(before.identification_vpd,after.identification_vpd)||!same(before.nvme_transport,after.nvme_transport))return bad("Disk identity or metadata changed during read");
    return {details,Support::available,"Pinned block/device directories, major:minor/diskseq and repeated metadata checked; not an atomic hardware snapshot"};
#else
    (void)name;(void)root;(void)after_capture;return missing<LinuxDiskDetails>("Linux disk metadata is unavailable on this backend");
#endif
}
void apply_linux_disk_identity(DiskInfo& disk,Metric<LinuxDiskDetails> details){
    disk.linux_details=std::move(details);
    disk.reported_media_type=metric_is_known(disk.linux_details)?disk.linux_details.value->media_type:
        missing<std::string>(disk.linux_details.note,disk.linux_details.support==Support::permission_required?Support::permission_required:Support::unsupported);
}
}
