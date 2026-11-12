#include "linux_drm_client.hpp"
#include "linux_process_stat.hpp"
#include "linux_nvml.hpp"
#include <algorithm>
#include <charconv>
#include <cmath>
#include <limits>
#include <map>
#include <set>
#ifdef __linux__
#include <cerrno>
#include <fcntl.h>
#include <filesystem>
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <unistd.h>
#endif
namespace pcinsight::detail {
namespace {
std::string_view trim(std::string_view s){
    const auto b=s.find_first_not_of(" \t\r");if(b==s.npos)return {};
    return s.substr(b,s.find_last_not_of(" \t\r")-b+1);
}
std::optional<std::uint64_t> number(std::string_view s){
    std::uint64_t n{};if(s.empty())return {};
    const auto r=std::from_chars(s.data(),s.data()+s.size(),n);
    if(r.ec!=std::errc{} || r.ptr!=s.data()+s.size())return {};
    return n;
}
bool keystr(std::string_view s){return !s.empty() && s.size()<=128 && std::all_of(s.begin(),s.end(),[](unsigned char c){return c>32 && c<=126 && c!=':';});}
std::optional<std::uint64_t> frequency_hz(std::string_view s){
    const auto split=s.find_first_of(" \t");if(split==s.npos)return {}; // Require an explicit, proven unit.
    const auto n=number(s.substr(0,split));const auto unit=trim(s.substr(split));
    const std::uint64_t scale=unit=="Hz"?1:unit=="KHz"?1000:unit=="MHz"?1000000:0;
    if(!n || !*n || !scale || *n>std::numeric_limits<std::uint64_t>::max()/scale)return {};
    return *n*scale;
}
template<class T> Metric<T> missing(std::string note,Support support=Support::unsupported){return {{},support,std::move(note)};}
bool same(const LinuxDrmClientUsage& a,const LinuxDrmClientUsage& b){return a.pid==b.pid && a.process_start_ticks==b.process_start_ticks && a.client_id==b.client_id && a.driver==b.driver && nvml_pci_identity(a.pci_address)==nvml_pci_identity(b.pci_address);}
}
std::optional<LinuxDrmClientUsage> parse_linux_drm_fdinfo(std::string_view text){
    if(text.empty() || text.size()>65536)return {};
    LinuxDrmClientUsage out;std::set<std::string> seen;std::map<std::string,std::uint64_t> busy,capacity,cycles,total,maxfreq;
    bool driver=false,pci=false,client=false;
    while(!text.empty()){
        const auto end=text.find('\n');auto line=text.substr(0,end);if(line.size()>4096)return {};
        const auto colon=line.find(':');
        if(colon!=line.npos){
            const auto key=line.substr(0,colon);auto value=trim(line.substr(colon+1));
            const bool relevant=key=="drm-driver" || key=="drm-pdev" || key=="drm-client-id" || key.starts_with("drm-engine-") || key.starts_with("drm-cycles-") || key.starts_with("drm-total-cycles-") || key.starts_with("drm-maxfreq-");
            if(relevant){
                if(!seen.insert(std::string(key)).second)return {};
                if(key=="drm-driver"){
                    if(value.empty() || value.size()>128 || !std::all_of(value.begin(),value.end(),[](unsigned char c){return c>=32 && c<=126;}))return {};
                    out.driver=value;driver=true;
                }else if(key=="drm-pdev"){
                    if(nvml_pci_identity(value).empty())return {};
                    out.pci_address=value;pci=true;
                }else if(key=="drm-client-id"){
                    auto n=number(value);if(!n)return {};out.client_id=*n;client=true;
                }else if(key.starts_with("drm-engine-capacity-")){
                    auto name=key.substr(20);auto n=number(value);if(!keystr(name) || !n || !*n)return {};
                    capacity.emplace(name,*n);
                }else if(key.starts_with("drm-total-cycles-") || key.starts_with("drm-cycles-")){
                    const bool denominator=key.starts_with("drm-total-cycles-");
                    auto name=key.substr(denominator?17:11);auto n=number(value);
                    if(!keystr(name) || !n)return {};
                    (denominator?total:cycles).emplace(name,*n);
                }else if(key.starts_with("drm-maxfreq-")){
                    auto name=key.substr(12);auto hz=frequency_hz(value);if(!keystr(name) || !hz)return {};maxfreq.emplace(name,*hz);
                }else{
                    auto name=key.substr(11);if(!keystr(name) || !value.ends_with("ns"))return {};
                    value.remove_suffix(2);if(value.empty() || (value.back()!=' ' && value.back()!='\t'))return {};
                    auto n=number(trim(value));if(!n)return {};busy.emplace(name,*n);
                }
                if(busy.size()>128 || capacity.size()>128 || cycles.size()>128 || total.size()>128 || maxfreq.size()>128)return {};
            }
        }else if(line.starts_with("drm-"))return {};
        if(end==text.npos)break;
        text.remove_prefix(end+1);
    }
    if(!driver || !pci || !client)return {};
    std::set<std::string> names;
    for(const auto& [name,n]:busy)names.insert(name);
    for(const auto& [name,n]:total)if(!cycles.contains(name) || maxfreq.contains(name))return {};
    for(const auto& [name,n]:maxfreq)if(!cycles.contains(name))return {};
    for(const auto& [name,n]:cycles){
        if(total.contains(name) || maxfreq.contains(name))names.insert(name);
        else return {};
    }
    if(names.empty() || names.size()>128)return {};
    for(const auto& [name,n]:capacity)if(!names.contains(name))return {};
    for(const auto& name:names){
        LinuxDrmEngineUsage e;e.name=name;
        if(auto i=capacity.find(name);i!=capacity.end())e.capacity=i->second;
        if(auto i=cycles.find(name);i!=cycles.end())e.busy_cycles={i->second,Support::available,"drm-cycles raw GPU units"};
        if(auto i=total.find(name);i!=total.end())e.total_cycles={i->second,Support::available,"drm-total-cycles raw matching GPU clock"};
        if(auto i=maxfreq.find(name);i!=maxfreq.end())e.max_frequency_hz={i->second,Support::available,"drm-maxfreq converted from explicit Hz/KHz/MHz"};
        if(auto i=busy.find(name);i!=busy.end())e.busy_nanoseconds=i->second;
        if(maxfreq.contains(name))e.counter_kind=LinuxDrmCounterKind::cycles_max_frequency;
        else if(!busy.contains(name))e.counter_kind=LinuxDrmCounterKind::cycles;
        e.utilization_percent=missing<double>("First client observation; sample twice for utilization");
        e.engine_class=classify_linux_drm_engine(out.driver,name);out.engines.push_back(std::move(e));
    }
    return out;
}
Metric<LinuxDrmClientUsage> read_linux_drm_client_usage(std::uint32_t pid,std::uint32_t fd,const DrmClientReadApi& api){
    if(!pid || fd>static_cast<std::uint32_t>(std::numeric_limits<int>::max()) || !api.read || !api.device_pci || !api.now)return missing<LinuxDrmClientUsage>("Invalid PID/descriptor or missing DRM reader API");
    auto fail=[](const auto& v){return missing<LinuxDrmClientUsage>(v.note,v.support);};
    const auto root="/proc/"+std::to_string(pid);const auto path=root+"/fdinfo/"+std::to_string(fd);
    const auto before=api.read(root+"/stat");if(!before.value || before.support!=Support::available)return fail(before);
    const auto process=parse_linux_process_stat(*before.value);if(!process || process->pid!=pid)return missing<LinuxDrmClientUsage>("Invalid process identity");
    const auto device=api.device_pci(pid,fd);if(!device.value || device.support!=Support::available)return fail(device);
    const auto record=api.read(path);if(!record.value || record.support!=Support::available)return fail(record);
    auto out=parse_linux_drm_fdinfo(*record.value);if(!out || nvml_pci_identity(out->pci_address)!=nvml_pci_identity(*device.value) || nvml_pci_identity(*device.value).empty())return missing<LinuxDrmClientUsage>("No valid PCI-correlated DRM client engine record");
    out->captured_at=api.now();out->pid=pid;out->descriptor=fd;out->process_start_ticks=process->start_ticks;
    const auto again=api.read(path);if(!again.value || again.support!=Support::available)return fail(again);
    const auto checked=parse_linux_drm_fdinfo(*again.value);
    if(!checked || checked->client_id!=out->client_id || checked->driver!=out->driver || nvml_pci_identity(checked->pci_address)!=nvml_pci_identity(out->pci_address))return missing<LinuxDrmClientUsage>("DRM descriptor/client identity changed during read");
    const auto last_device=api.device_pci(pid,fd);if(!last_device.value || last_device.support!=Support::available)return fail(last_device);
    const auto after=api.read(root+"/stat");if(!after.value || after.support!=Support::available)return fail(after);
    const auto last=parse_linux_process_stat(*after.value);
    if(!last || last->pid!=pid || last->start_ticks!=process->start_ticks || nvml_pci_identity(*last_device.value)!=nvml_pci_identity(*device.value))return missing<LinuxDrmClientUsage>("Process or DRM device identity changed during read");
    return {std::move(out),Support::available,"DRM client engine counters from /proc/PID/fdinfo/FD; observed client only, NOT GPU-global utilization"};
}
Metric<LinuxDrmClientUsage> update_linux_drm_client_usage(std::optional<LinuxDrmClientUsage>& previous,Metric<LinuxDrmClientUsage> result){
    if(!result.value || result.support!=Support::available){previous.reset();return result;}
    auto& current=*result.value;current.sample_interval={};auto next=current;
    for(auto& e:current.engines)e.utilization_percent=missing<double>("New client/engine or baseline reset; another sample required");
    if(previous && same(*previous,current) && current.captured_at>=previous->captured_at){
        const auto elapsed=std::chrono::duration_cast<std::chrono::nanoseconds>(current.captured_at-previous->captured_at);
        {
            if(elapsed.count()>0)
            current.sample_interval={elapsed,Support::available,"steady_clock interval between observations; not an atomic GPU interval"};
            for(std::size_t k=0;k<current.engines.size();++k){auto& e=current.engines[k];
                const auto old=std::find_if(previous->engines.begin(),previous->engines.end(),[&](const auto& p){return p.name==e.name && p.capacity==e.capacity && p.counter_kind==e.counter_kind && (e.counter_kind!=LinuxDrmCounterKind::cycles_max_frequency || p.max_frequency_hz.value==e.max_frequency_hz.value);});
                if(old==previous->engines.end() || !e.capacity)continue;
                if(e.counter_kind==LinuxDrmCounterKind::cycles_max_frequency){
                    if(!e.busy_cycles.value || !old->busy_cycles.value || !e.max_frequency_hz.value || !*e.max_frequency_hz.value)continue;
                    if(elapsed.count()<=0){next.engines[k].busy_cycles=old->busy_cycles;continue;}
                    if(*e.busy_cycles.value<*old->busy_cycles.value){next.engines[k].busy_cycles=old->busy_cycles;e.utilization_percent.note="GPU busy cycles below high-water mark; waiting for catch-up";continue;}
                    const auto delta=*e.busy_cycles.value-*old->busy_cycles.value;
                    const long double percent=100.0L*delta/static_cast<long double>(elapsed.count())*1000000000.0L/static_cast<long double>(*e.max_frequency_hz.value)/e.capacity;
                    if(!std::isfinite(percent) || percent>100.0L){e.utilization_percent.note="GPU cycles exceed maximum frequency/elapsed time/capacity; no clamped value";continue;}
                    e.utilization_percent={static_cast<double>(percent),Support::estimated,"Observed client busy cycles / steady interval / maximum frequency / capacity; frequency-normalized, NOT active time or GPU-global utilization"};
                    continue;
                }
                if(e.counter_kind==LinuxDrmCounterKind::cycles){
                    if(!e.busy_cycles.value || !e.total_cycles.value || !old->busy_cycles.value || !old->total_cycles.value)continue;
                    if(*e.total_cycles.value<*old->total_cycles.value){e.utilization_percent.note="GPU total cycles reset; new cycle epoch baseline";continue;}
                    if(*e.total_cycles.value==*old->total_cycles.value){next.engines[k].busy_cycles=old->busy_cycles;e.utilization_percent.note="GPU total cycles not advanced; retain previous cycle anchor, no measured zero";continue;}
                    if(*e.busy_cycles.value<*old->busy_cycles.value){next.engines[k].busy_cycles=old->busy_cycles;e.utilization_percent.note="GPU busy cycles below high-water mark; waiting for catch-up";continue;}
                    const auto delta=*e.busy_cycles.value-*old->busy_cycles.value;
                    const auto total_delta=*e.total_cycles.value-*old->total_cycles.value;
                    const long double percent=100.0L*delta/static_cast<long double>(total_delta)/e.capacity;
                    if(!std::isfinite(percent) || percent>100.0L){e.utilization_percent.note="GPU cycle delta exceeds total cycles/capacity; no clamped value";continue;}
                    e.utilization_percent={static_cast<double>(percent),Support::estimated,"Observed client GPU busy-cycle delta / GPU total-cycle delta / capacity; not nanosecond active time or GPU-global utilization"};
                    continue;
                }
                if(elapsed.count()<=0)continue;
                if(e.busy_nanoseconds<old->busy_nanoseconds){next.engines[k].busy_nanoseconds=old->busy_nanoseconds;e.utilization_percent.note="Counter below high-water mark; waiting for driver catch-up";continue;}
                const long double percent=100.0L*(e.busy_nanoseconds-old->busy_nanoseconds)/static_cast<long double>(elapsed.count())/e.capacity;
                if(!std::isfinite(percent) || percent>100.0L){e.utilization_percent.note="Counter delta exceeds engine capacity/elapsed time; no clamped value";continue;}
                e.utilization_percent={static_cast<double>(percent),Support::estimated,"Observed DRM client engine active time / steady interval / reported capacity; not total GPU utilization"};
            }
        }
    }
    previous=std::move(next);return result;
}
#ifdef __linux__
namespace {
Metric<std::string> read_native(const std::string& path){
    int fd=open(path.c_str(),O_RDONLY|O_CLOEXEC|O_NONBLOCK);
    if(fd<0)return missing<std::string>("Cannot read "+path,(errno==EACCES || errno==EPERM)?Support::permission_required:Support::unsupported);
    struct Close{int fd;~Close(){close(fd);}} cleanup{fd};
    std::string text;char buf[4096];
    for(;;){auto n=read(fd,buf,sizeof buf);if(n<0){if(errno==EINTR)continue;return missing<std::string>("procfs read failed",(errno==EACCES || errno==EPERM)?Support::permission_required:Support::unsupported);}if(!n)break;if(text.size()+static_cast<std::size_t>(n)>65536)return missing<std::string>("procfs record exceeds 64KiB");text.append(buf,static_cast<std::size_t>(n));}
    return {text,Support::available,"procfs"};
}
Metric<std::string> native_pci(std::uint32_t pid,std::uint32_t fd){
    const auto path="/proc/"+std::to_string(pid)+"/fd/"+std::to_string(fd);struct stat before{},after{};
    if(stat(path.c_str(),&before)!=0)return missing<std::string>("Cannot stat target descriptor",(errno==EACCES || errno==EPERM)?Support::permission_required:Support::unsupported);
    if(!S_ISCHR(before.st_mode))return missing<std::string>("Target descriptor is not a character device");
    const auto root=std::filesystem::path("/sys/dev/char")/(std::to_string(major(before.st_rdev))+":"+std::to_string(minor(before.st_rdev)));
    std::error_code ec;const auto subsystem=std::filesystem::canonical(root/"subsystem",ec);if(ec || subsystem!="/sys/class/drm")return missing<std::string>("Target descriptor is not a proven DRM device",ec==std::errc::permission_denied?Support::permission_required:Support::unsupported);
    const auto device=std::filesystem::canonical(root/"device",ec);if(ec || nvml_pci_identity(device.filename().string()).empty())return missing<std::string>("DRM descriptor has no PCI sysfs identity",ec==std::errc::permission_denied?Support::permission_required:Support::unsupported);
    if(stat(path.c_str(),&after)!=0 || before.st_dev!=after.st_dev || before.st_ino!=after.st_ino || before.st_rdev!=after.st_rdev)return missing<std::string>("Descriptor device changed during stat");
    return {device.filename().string(),Support::available,"DRM character-device/sysfs PCI identity"};
}
}
#endif
}
namespace pcinsight {
Metric<LinuxDrmClientUsage> read_linux_drm_client_usage(std::uint32_t pid,std::uint32_t fd){
#ifdef __linux__
    return detail::read_linux_drm_client_usage(pid,fd,{detail::read_native,detail::native_pci,[]{return std::chrono::steady_clock::now();}});
#else
    (void)pid;(void)fd;return {{},Support::unsupported,"Linux DRM client usage is unavailable on this platform"};
#endif
}
Metric<LinuxDrmClientUsage> LinuxDrmClientSampler::sample(std::uint32_t pid,std::uint32_t fd){return detail::update_linux_drm_client_usage(previous_,read_linux_drm_client_usage(pid,fd));}
}
