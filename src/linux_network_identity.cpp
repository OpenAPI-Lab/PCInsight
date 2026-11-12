#include "linux_network_identity.hpp"
#include <algorithm>
#include <array>
#include <charconv>
#include <limits>
#ifdef __linux__
#include <fcntl.h>
#include <linux/if_arp.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cerrno>
#endif
namespace pcinsight::detail {
namespace {
template<class T> Metric<T> missing(std::string note,Support s=Support::unsupported){return {{},s,std::move(note)};}
std::string_view trim(std::string_view s){while(!s.empty()&&(s.back()=='\n'||s.back()=='\r'||s.back()==' '||s.back()=='\t'))s.remove_suffix(1);return s;}
std::optional<std::uint32_t> number(std::string_view s,int base,std::size_t digits=0){
    s=trim(s);if(s.empty()||(digits&&s.size()!=digits))return {};
    std::uint32_t value{};const auto [end,error]=std::from_chars(s.data(),s.data()+s.size(),value,base);
    if(error!=std::errc{}||end!=s.data()+s.size())return {};
    return value;
}
bool label(std::string_view s){return !s.empty()&&s.size()<=4096&&std::all_of(s.begin(),s.end(),[](unsigned char c){return c>=32&&c<127;});}
std::optional<std::pair<std::uint32_t,std::string_view>> pci_entry(std::string_view line){
    if(line.size()<6||line[4]!=' ')return {};
    const auto id=number(line.substr(0,4),16,4);if(!id)return {};
    line.remove_prefix(5);const auto pos=line.find_first_not_of(' ');if(pos==line.npos)return {};
    line=trim(line.substr(pos));if(!label(line))return {};
    return std::pair{*id,line};
}
}
void apply_linux_network_pci_names(LinuxNetworkDetails& d,const PciNamesDatabase& db){
    d.vendor=missing<std::string>("No unique vendor label in optional local pci.ids");
    d.model=missing<std::string>("No unique device label in optional local pci.ids");
    const auto supported=[](const auto& m){return m.value&&(m.support==Support::available||m.support==Support::estimated);};
    if(d.bus.value!="pci"||!supported(d.bus)||!supported(d.hardware_vendor_id)||!supported(d.hardware_device_id)||
       *d.hardware_vendor_id.value>65535||*d.hardware_device_id.value>65535||
       db.text.empty()||db.text.size()>16*1024*1024||db.text.find('\0')!=db.text.npos||!label(db.source))return;
    std::string_view text=db.text;std::optional<std::uint32_t> vendor;
    unsigned vc=0,dc=0;std::string vn,dn;
    while(!text.empty()){
        const auto end=text.find('\n');auto line=text.substr(0,end);if(line.size()>4096)return;
        if(!line.empty()&&line.back()=='\r')line.remove_suffix(1);
        if(!line.empty()&&line.front()!='#'){
            if(line.front()!='\t'){
                vendor.reset();if(auto e=pci_entry(line)){vendor=e->first;if(*vendor==*d.hardware_vendor_id.value){++vc;vn=e->second;}}
            }else if(line.size()>1&&line[1]!='\t'&&vendor==d.hardware_vendor_id.value){
                if(auto e=pci_entry(line.substr(1));e&&e->first==*d.hardware_device_id.value){++dc;dn=e->second;}
            }
        }
        if(end==text.npos)break;
        text.remove_prefix(end+1);
    }
    const auto note="Optional local pci.ids label, not authenticated retail identity: "+db.source;
    if(vc==1){d.vendor={vn,Support::estimated,note};if(dc==1)d.model={dn,Support::estimated,note};}
}
#ifdef __linux__
namespace {
struct Fd{int n;explicit Fd(int fd):n(fd){}~Fd(){if(n>=0)::close(n);}Fd(const Fd&)=delete;};
Support access(int error){return error==EACCES||error==EPERM?Support::permission_required:Support::unsupported;}
Metric<std::string> attr(int dir,const char* name){
    Fd fd(::openat(dir,name,O_RDONLY|O_CLOEXEC|O_NONBLOCK|O_NOFOLLOW));
    if(fd.n<0){const auto error=errno;return missing<std::string>(std::string("Cannot open sysfs ")+name,access(error));}
    struct stat s{};if(::fstat(fd.n,&s)||!S_ISREG(s.st_mode))return missing<std::string>("Sysfs attribute not a regular file");
    std::string result;std::array<char,512> buffer{};
    while(true){const auto n=::read(fd.n,buffer.data(),buffer.size());
        if(n<0){if(errno==EINTR)continue;const auto error=errno;return missing<std::string>("Sysfs attribute read failed",access(error));}
        if(!n)break;
        if(static_cast<std::size_t>(n)>4096-result.size())return missing<std::string>("Sysfs attribute exceeds4096B");
        result.append(buffer.data(),static_cast<std::size_t>(n));}
    if(result.find('\0')!=result.npos)return missing<std::string>("NUL in sysfs text");
    result=std::string(trim(result));return {std::move(result),Support::available,"Bounded sysfs attribute"};
}
Metric<std::string> link_name(int dir,const char* name){
    std::array<char,4096> buffer{};const auto n=::readlinkat(dir,name,buffer.data(),buffer.size());
    if(n<0){const auto error=errno;return missing<std::string>(std::string("Cannot read sysfs link ")+name,access(error));}
    if(static_cast<std::size_t>(n)>=buffer.size())return missing<std::string>("Oversized sysfs link");
    const std::string_view path(buffer.data(),static_cast<std::size_t>(n));const auto slash=path.find_last_of('/');const auto base=path.substr(slash==path.npos?0:slash+1);
    if(base.empty()||base.size()>128||base.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_.-")!=base.npos||base=="."||base=="..")return missing<std::string>("Invalid sysfs link basename");
    return {std::string(base),Support::available,"Kernel sysfs link basename, not manufacturer/model"};
}
bool usb_type(int fd,std::string_view type){
    const auto event=attr(fd,"uevent");if(!event.value)return false;
    std::string_view s=*event.value;unsigned count=0;bool match=false;
    while(!s.empty()){const auto e=s.find('\n');const auto line=s.substr(0,e);if(line.starts_with("DEVTYPE=")){++count;match=line.substr(8)==type;}if(e==s.npos)break;s.remove_prefix(e+1);}
    return count==1&&match;
}
Metric<std::uint32_t> id_attr(int fd,const char* file,bool prefix){
    const auto raw=attr(fd,file);if(!raw.value)return missing<std::uint32_t>(raw.note,raw.support);
    std::string_view s=*raw.value;
    if(prefix){if(!s.starts_with("0x"))return missing<std::uint32_t>("PCI ID missing 0x prefix");s.remove_prefix(2);}
    const auto value=number(s,16,4);if(!value)return missing<std::uint32_t>("Invalid16-bit sysfs hardware ID");
    return {*value,Support::available,std::string("Kernel bus-specific ")+file+" ID"};
}
Metric<std::string> usb_label(int fd,const char* file){
    auto value=attr(fd,file);if(!value.value)return value;
    if(!label(*value.value))return missing<std::string>("USB string empty, non-ASCII/control or too long");
    value.support=Support::estimated;value.note=std::string("USB device-reported ")+file+" string, not authenticated manufacturer/retail identity";return value;
}
bool same_dir(int parent,const char* name,int fd){struct stat a{},b{};return !::fstat(fd,&a)&&!::fstatat(parent,name,&b,0)&&a.st_dev==b.st_dev&&a.st_ino==b.st_ino;}
}
#endif
Metric<LinuxNetworkDetails> read_linux_network_identity(const std::string& name,std::uint64_t expected,const PciNamesDatabase& db,const std::string& root,const std::string& virtual_root){
#ifdef __linux__
    const auto bad=[](std::string note,Support support=Support::unsupported){return missing<LinuxNetworkDetails>(std::move(note),support);};
    if(name.empty()||name.size()>15||name=="."||name==".."||name.find('/')!=name.npos||name.find('\0')!=name.npos||!expected||expected>std::numeric_limits<std::uint32_t>::max()||root.empty()||root.size()>65536||root.find('\0')!=root.npos||virtual_root.empty()||virtual_root.size()>65536||virtual_root.find('\0')!=virtual_root.npos)return bad("Invalid network metadata identity/path");
    Fd base(::open(root.c_str(),O_RDONLY|O_DIRECTORY|O_CLOEXEC));if(base.n<0){const auto error=errno;return bad("Cannot open network sysfs root",access(error));}
    Fd net(::openat(base.n,name.c_str(),O_RDONLY|O_DIRECTORY|O_CLOEXEC));if(net.n<0){const auto error=errno;return bad("Cannot pin network sysfs directory",access(error));}
    const auto index=attr(net.n,"ifindex"),type=attr(net.n,"type");
    if(!index.value||!type.value)return bad("Network ifindex/type unavailable",index.support==Support::permission_required||type.support==Support::permission_required?Support::permission_required:Support::unsupported);
    if(number(*index.value,10)!=expected)return bad("Network ifindex changed or invalid");
    const auto kind=number(*type.value,10);if(!kind||*kind>65535)return bad("Invalid kernel ARPHRD type");
    LinuxNetworkDetails d;d.interface_index=static_cast<std::uint32_t>(expected);d.link_layer_type=*kind;
    d.vendor=missing<std::string>("No supported manufacturer source for this device");d.model=missing<std::string>("No supported model source for this device");
    d.connection_type=missing<std::string>("No named mapping for this ARPHRD type; numeric type retained");
    d.bus=missing<std::string>("No device subsystem exposed");d.driver=missing<std::string>("No device driver exposed");
    d.hardware_vendor_id=missing<std::uint32_t>("No PCI/USB vendor ID available");d.hardware_device_id=missing<std::uint32_t>("No PCI/USB product ID available");
    struct stat wireless{};const int wi=::fstatat(net.n,"wireless",&wireless,0);const int we=wi?errno:0;
    struct stat phy{};const int ph=::fstatat(net.n,"phy80211",&phy,0);const int pe=ph?errno:0;
    if((!wi&&S_ISDIR(wireless.st_mode))||(!ph&&S_ISDIR(phy.st_mode)))d.wireless={true,Support::available,"Kernel wireless/phy80211 directory marker; not connection state"};
    else if(we==ENOENT&&pe==ENOENT)d.wireless={false,Support::available,"No kernel wireless/phy80211 marker observed"};
    else d.wireless={{},access(we)==Support::permission_required||access(pe)==Support::permission_required?Support::permission_required:Support::unsupported,"Wireless marker unavailable/invalid"};
    Fd vr(::open(virtual_root.c_str(),O_RDONLY|O_DIRECTORY|O_CLOEXEC));
    if(vr.n>=0){d.virtual_device={same_dir(net.n,"..",vr.n),Support::available,"Interface parent compared with sysfs virtual/net directory; not proof of physical hardware otherwise"};}
    else d.virtual_device=missing<bool>("Virtual sysfs root unavailable",access(errno));
    if(*kind==772)d.connection_type={"Loopback",Support::available,"Kernel ARPHRD_LOOPBACK, not interface-name heuristic"};
    else if(*kind==1)d.connection_type={d.wireless.value==true?"Wi-Fi":(d.virtual_device.value==true?"Virtual Ethernet link-layer":"Ethernet link-layer"),Support::estimated,"ARPHRD_ETHER with kernel wireless/virtual evidence; does not certify wired medium, link state or negotiated speed"};
    else if(*kind==512)d.connection_type={"PPP",Support::available,"Kernel ARPHRD_PPP"};
    else if(*kind==ARPHRD_CAN)d.connection_type={"CAN",Support::available,"Kernel ARPHRD_CAN; not link state"};
    else if(*kind==ARPHRD_TUNNEL||*kind==ARPHRD_TUNNEL6||*kind==ARPHRD_SIT)
        d.connection_type={"IP tunnel link-layer",Support::available,"Kernel ARPHRD_TUNNEL/TUNNEL6/SIT; not the underlying physical transport"};
    else if(*kind==ARPHRD_IPGRE||*kind==ARPHRD_IP6GRE)
        d.connection_type={"GRE tunnel link-layer",Support::available,"Kernel ARPHRD_IPGRE/IP6GRE; not the underlying physical transport"};
    else if(*kind==32)d.connection_type={"InfiniBand",Support::available,"Kernel ARPHRD_INFINIBAND"};
    else if(*kind==801)d.connection_type={"IEEE 802.11",Support::available,"Kernel ARPHRD_IEEE80211; not connection state"};
    else if(*kind==802||*kind==803)d.connection_type={"IEEE 802.11 monitor",Support::available,"Kernel ARPHRD_IEEE80211_PRISM/RADIOTAP; not an associated Wi-Fi connection"};
    Fd device(::openat(net.n,"device",O_RDONLY|O_DIRECTORY|O_CLOEXEC));
    if(device.n>=0){
        d.bus=link_name(device.n,"subsystem");d.driver=link_name(device.n,"driver");
        if(d.bus.value=="pci"){
            d.hardware_vendor_id=id_attr(device.n,"vendor",true);d.hardware_device_id=id_attr(device.n,"device",true);
            apply_linux_network_pci_names(d,db);
        }else if(d.bus.value=="usb"){
            Fd parent(::openat(device.n,"..",O_RDONLY|O_DIRECTORY|O_CLOEXEC));
            int usb=device.n;
            if(usb_type(device.n,"usb_interface")&&parent.n>=0&&link_name(parent.n,"subsystem").value=="usb"&&usb_type(parent.n,"usb_device"))usb=parent.n;
            if(usb_type(usb,"usb_device")){
                d.hardware_vendor_id=id_attr(usb,"idVendor",false);d.hardware_device_id=id_attr(usb,"idProduct",false);
                d.vendor=usb_label(usb,"manufacturer");d.model=usb_label(usb,"product");
            }
        }
        if(!same_dir(net.n,"device",device.n)||link_name(device.n,"subsystem").value!=d.bus.value)return bad("Network device identity/subsystem changed during read");
    }else{const auto error=errno;if(access(error)==Support::permission_required){
        d.bus=d.driver=d.vendor=d.model=missing<std::string>("Device metadata directory denied",Support::permission_required);
        d.hardware_vendor_id=d.hardware_device_id=missing<std::uint32_t>("Device metadata directory denied",Support::permission_required);
    }}
    const auto after=attr(net.n,"ifindex"),type_after=attr(net.n,"type");
    if(!after.value||!type_after.value||*after.value!=*index.value||*type_after.value!=*type.value||!same_dir(base.n,name.c_str(),net.n))return bad("Network directory/ifindex/type changed during metadata read");
    return {std::move(d),Support::estimated,"Verified endpoint sysfs identity; metadata fields may be absent. No atomic hotplug/driver generation or universal retail identification"};
#else
    (void)name;(void)expected;(void)db;(void)root;(void)virtual_root;return missing<LinuxNetworkDetails>("Linux network metadata unsupported on this platform");
#endif
}
}
