#include "linux_gpu_names.hpp"
#ifdef __linux__
#include "linux_nvml.hpp"
#include <algorithm>
#include <charconv>
#include <map>
#include <set>
#include <optional>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cerrno>
namespace pcinsight::detail {
namespace {
constexpr std::size_t max_bytes=16*1024*1024, max_line=4096, max_gpus=256;
std::optional<std::uint32_t> hex(std::string_view s){
    if(s.size()!=4)return {};
    std::uint32_t n{};auto r=std::from_chars(s.data(),s.data()+s.size(),n,16);
    if(r.ec!=std::errc{} || r.ptr!=s.data()+s.size())return {};
    return n;
}
std::optional<std::uint32_t> sys_id(std::string_view s){
    return s.starts_with("0x")?hex(s.substr(2)):std::nullopt;
}
struct Name { unsigned count{}; std::string value; };
using Key=std::pair<std::uint32_t,std::uint32_t>;
std::optional<std::pair<std::uint32_t,std::string>> entry(std::string_view line){
    if(line.size()<6 || line[4]!=' ')return {};
    auto id=hex(line.substr(0,4));if(!id)return {};
    line.remove_prefix(5);const auto start=line.find_first_not_of(' ');
    if(start==std::string_view::npos)return {};
    line.remove_prefix(start);
    while(!line.empty() && line.back()==' ')line.remove_suffix(1);
    if(!std::all_of(line.begin(),line.end(),[](unsigned char c){return c>=32 && c<=126;}))return {};
    return std::pair{*id,std::string(line)};
}
}
PciNamesDatabase load_linux_pci_names(){
    return load_linux_pci_names({"/usr/share/hwdata/pci.ids","/usr/share/misc/pci.ids","/usr/share/pci.ids"});
}
PciNamesDatabase load_linux_pci_names(const std::vector<std::string>& paths){
    for(const auto& path:paths){
        const int fd=open(path.c_str(),O_RDONLY|O_CLOEXEC|O_NONBLOCK);if(fd<0)continue;
        struct Close {int fd;~Close(){close(fd);}} close_fd{fd};
        struct stat st{};if(fstat(fd,&st)!=0 || !S_ISREG(st.st_mode) || st.st_size<=0 || st.st_size>static_cast<off_t>(max_bytes))continue;
        std::string data;char buffer[8192];bool ok=true;
        for(;;){const auto n=read(fd,buffer,sizeof buffer);if(n<0){if(errno==EINTR)continue;ok=false;break;}if(!n)break;
            if(data.size()+static_cast<std::size_t>(n)>max_bytes){ok=false;break;}data.append(buffer,static_cast<std::size_t>(n));}
        struct stat after{};
        if(ok && !data.empty() && fstat(fd,&after)==0 && after.st_size==st.st_size && after.st_mtim.tv_sec==st.st_mtim.tv_sec && after.st_mtim.tv_nsec==st.st_mtim.tv_nsec)return {std::move(data),path};
    }
    return {};
}
void fill_linux_gpu_names(std::vector<GpuInfo>& gpus,const PciNamesDatabase& db){
    std::map<std::uint32_t,Name> vendors;std::map<Key,Name> devices;std::vector<GpuInfo*> eligible;
    for(auto& g:gpus){
        g.pci_vendor_name={{},Support::unsupported,"No unique vendor name in local pci.ids database"};
        g.pci_device_name={{},Support::unsupported,"No unique device name in local pci.ids database"};
        // Only raw DRM PCI IDs are accepted; never reinterpret an existing driver model.
        const auto v=sys_id(g.vendor), d=sys_id(g.model);
        if(!v || !d || !g.physical_location.value || g.physical_location.support!=Support::available || nvml_pci_identity(*g.physical_location.value).empty())continue;
        g.hardware_vendor_id={*v,Support::available,"DRM device/vendor PCI ID"};
        g.hardware_device_id={*d,Support::available,"DRM device/device PCI ID"};
        if(gpus.size()<=max_gpus){vendors.try_emplace(*v);devices.try_emplace(Key{*v,*d});eligible.push_back(&g);}
    }
    if(devices.empty() || db.text.empty() || db.text.size()>max_bytes || db.source.empty())return;
    // Validate all line lengths before publishing anything; no truncated records.
    std::string_view text=db.text;std::optional<std::uint32_t> vendor;
    while(!text.empty()){
        const auto end=text.find('\n');auto line=text.substr(0,end);
        if(line.size()>max_line)return;
        if(!line.empty() && line.back()=='\r')line.remove_suffix(1);
        if(!line.empty() && line.front()!='#'){
            if(line.front()!='\t'){
                vendor.reset();
                if(auto e=entry(line)){vendor=e->first;if(auto i=vendors.find(*vendor);i!=vendors.end()){++i->second.count;i->second.value=e->second;}}
            }else if(line.size()>1 && line[1]!='\t' && vendor){
                if(auto e=entry(line.substr(1)))if(auto i=devices.find({*vendor,e->first});i!=devices.end()){++i->second.count;i->second.value=e->second;}
            }
        }
        if(end==std::string_view::npos)break;
        text.remove_prefix(end+1);
    }
    const auto note="Local PCI ID database mapping (not driver-reported retail model): "+db.source;
    for(auto* target:eligible){auto& g=*target;
        auto v=vendors.find(*g.hardware_vendor_id.value);if(v==vendors.end() || v->second.count!=1)continue;
        g.pci_vendor_name={v->second.value,Support::estimated,note};
        auto d=devices.find({*g.hardware_vendor_id.value,*g.hardware_device_id.value});
        if(d!=devices.end() && d->second.count==1)g.pci_device_name={d->second.value,Support::estimated,note};
        if(sys_id(g.vendor))g.vendor=*g.pci_vendor_name.value;
        if(sys_id(g.model) && g.pci_device_name.value)g.model=*g.pci_device_name.value;
    }
}
}
#endif
