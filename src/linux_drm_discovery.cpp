#include "linux_drm_discovery.hpp"
#include <algorithm>
#include <set>
#include <limits>
#include <string_view>
#ifdef __linux__
#include <cerrno>
#include <charconv>
#include <dirent.h>
#include <filesystem>
#include <sys/stat.h>
#include <sys/sysmacros.h>
#endif
namespace pcinsight::detail {
namespace {
template<class T> Metric<T> absent(std::string note,Support support=Support::unsupported){return {{},support,std::move(note)};}
bool valid_list(const DrmDirectoryEntries& v,std::uint32_t limit){
    if(v.ids.size()>limit)return false;
    std::set<std::uint32_t> ids;
    for(auto id:v.ids)if(id>static_cast<std::uint32_t>(std::numeric_limits<int>::max()) || !ids.insert(id).second)return false;
    return true;
}
}
Metric<LinuxDrmDiscovery> discover_linux_drm_clients(const LinuxDrmDiscoveryOptions& o,const DrmDiscoveryApi& api){
    if(!o.max_processes || o.max_processes>65536 || !o.max_descriptors || o.max_descriptors>262144 || !o.max_observations || o.max_observations>2048 || o.max_duration.count()<1 || o.max_duration.count()>60000 || !api.list || !api.is_drm || !api.read_client || !api.now)return absent<LinuxDrmDiscovery>("Invalid DRM discovery limits or reader API");
    LinuxDrmDiscovery out;out.started_at=api.now();auto last=out.started_at;bool bad_clock=false;
    auto expired=[&]{const auto t=api.now();if(t<last)bad_clock=true;last=t;return bad_clock || t-out.started_at>=o.max_duration;};
    auto permission=[&](Support s){if(s==Support::permission_required)++out.permission_denials;};
    const auto processes=api.list("/proc",o.max_processes);
    if(!processes.value || processes.support!=Support::available)return absent<LinuxDrmDiscovery>("Cannot enumerate visible procfs",processes.support==Support::permission_required?Support::permission_required:Support::unsupported);
    if(!valid_list(*processes.value,o.max_processes) || std::find(processes.value->ids.begin(),processes.value->ids.end(),0U)!=processes.value->ids.end())return absent<LinuxDrmDiscovery>("Invalid process directory result");
    out.limit_reached=processes.value->limit_reached;
    bool stop=false;std::size_t total_engines=0;
    for(const auto pid:processes.value->ids){
        if(expired()){out.time_limit_reached=true;break;}
        if(out.descriptors_seen>=o.max_descriptors){out.limit_reached=true;break;}
        ++out.processes_seen;
        const auto remaining=o.max_descriptors-static_cast<std::uint32_t>(out.descriptors_seen);
        const auto fds=api.list("/proc/"+std::to_string(pid)+"/fdinfo",remaining);
        if(!fds.value || fds.support!=Support::available){++out.directory_failures;permission(fds.support);continue;}
        if(!valid_list(*fds.value,remaining))return absent<LinuxDrmDiscovery>("Invalid descriptor directory result");
        out.limit_reached|=fds.value->limit_reached;
        for(const auto fd:fds.value->ids){
            if(expired()){out.time_limit_reached=true;stop=true;break;}
            ++out.descriptors_seen;
            const auto candidate=api.is_drm(pid,fd);
            if(!candidate.value || candidate.support!=Support::available){++out.unclassified_descriptors;permission(candidate.support);continue;}
            if(!*candidate.value)continue;
            ++out.drm_candidates;
            if(expired()){out.time_limit_reached=true;stop=true;break;}
            auto client=api.read_client(pid,fd);
            if(!client.value || client.support!=Support::available || client.value->pid!=pid || client.value->descriptor!=fd || client.value->engines.empty() || client.value->engines.size()>128){++out.unreadable_drm_clients;permission(client.support);continue;}
            if(out.observations.size()>=o.max_observations || total_engines+client.value->engines.size()>65536){out.limit_reached=true;stop=true;break;}
            total_engines+=client.value->engines.size();out.observations.push_back(std::move(client));
        }
        if(stop)break;
    }
    out.finished_at=api.now();if(bad_clock || out.finished_at<last)return absent<LinuxDrmDiscovery>("Non-monotonic discovery clock");
    out.scope="Visible caller PID namespace/procfs only; successful accessible PCI DRM clients; non-atomic scan, optional driver stats, no all-process or GPU-global coverage guarantee. Duplicates require PCI/client-id deduplication.";
    return {std::move(out),Support::estimated,"DRM discovery coverage is partial/observed even when no errors or limits were detected"};
}
#ifdef __linux__
namespace {
Metric<DrmDirectoryEntries> native_list(const std::string& path,std::uint32_t limit){
    DIR* dir=opendir(path.c_str());if(!dir)return absent<DrmDirectoryEntries>("procfs directory unavailable",(errno==EACCES || errno==EPERM)?Support::permission_required:Support::unsupported);
    struct Close {DIR* d;~Close(){closedir(d);}} cleanup{dir};
    DrmDirectoryEntries out;std::size_t entries=0;
    for(;;){errno=0;auto* entry=readdir(dir);if(!entry){if(errno)return absent<DrmDirectoryEntries>("procfs readdir failed",(errno==EACCES || errno==EPERM)?Support::permission_required:Support::unsupported);break;}
        if(++entries>static_cast<std::size_t>(limit)*4+256){out.limit_reached=true;break;}
        const std::string_view name=entry->d_name;std::uint32_t id{};
        auto r=std::from_chars(name.data(),name.data()+name.size(),id);
        if(r.ec!=std::errc{} || r.ptr!=name.data()+name.size() || id>static_cast<std::uint32_t>(std::numeric_limits<int>::max()))continue;
        if(out.ids.size()==limit){out.limit_reached=true;break;}out.ids.push_back(id);
    }
    std::sort(out.ids.begin(),out.ids.end());return {std::move(out),Support::available,"Observed numeric procfs directory entries"};
}
Metric<bool> native_is_drm(std::uint32_t pid,std::uint32_t fd){
    const auto path="/proc/"+std::to_string(pid)+"/fd/"+std::to_string(fd);struct stat st{};
    if(stat(path.c_str(),&st)!=0)return absent<bool>("Descriptor stat unavailable",(errno==EACCES || errno==EPERM)?Support::permission_required:Support::unsupported);
    if(!S_ISCHR(st.st_mode))return {false,Support::available,"Not a character device"};
    const auto path_sys=std::filesystem::path("/sys/dev/char")/(std::to_string(major(st.st_rdev))+":"+std::to_string(minor(st.st_rdev)))/"subsystem";
    std::error_code ec;const auto subsystem=std::filesystem::canonical(path_sys,ec);
    if(ec)return absent<bool>("Character-device subsystem unavailable",ec==std::errc::permission_denied?Support::permission_required:Support::unsupported);
    return {subsystem=="/sys/class/drm",Support::available,"Character-device sysfs subsystem; full client reader must revalidate identity"};
}
}
#endif
}
namespace pcinsight {
Metric<LinuxDrmDiscovery> discover_linux_drm_clients(const LinuxDrmDiscoveryOptions& o){
#ifdef __linux__
    return detail::discover_linux_drm_clients(o,{detail::native_list,detail::native_is_drm,read_linux_drm_client_usage,[]{return std::chrono::steady_clock::now();}});
#else
    (void)o;return {{},Support::unsupported,"Native DRM discovery requires Linux procfs/sysfs"};
#endif
}
}
