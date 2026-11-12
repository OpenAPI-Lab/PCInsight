#include "linux_gpu_sysfs.hpp"
#include "linux_gpu_driver_date.hpp"
#include "linux_nvml.hpp"
#include <algorithm>
#include <charconv>
#include <limits>
#include <set>
#ifdef __linux__
#include <cerrno>
#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#endif
namespace pcinsight::detail {
namespace {
template<class T>Metric<T> missing(std::string note,Support s=Support::unsupported){return {{},s,std::move(note)};}
bool measured(const Metric<std::string>& m){return m.value && m.support==Support::available;}
bool identifier(std::string_view s){return !s.empty() && s.size()<=128 && std::all_of(s.begin(),s.end(),[](unsigned char c){return (c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='_'||c=='-'||c=='.';}) && s!="." && s!="..";}
[[maybe_unused]] bool numbered(std::string_view s,std::string_view prefix){if(!s.starts_with(prefix))return false;s.remove_prefix(prefix.size());return !s.empty() && s.size()<=10 && s.find_first_not_of("0123456789")==s.npos;}
template<class T>Metric<T> integer(const Metric<std::string>& raw,const char* source){
    if(!measured(raw))return missing<T>(std::string(source)+": "+raw.note,raw.support);
    const auto& s=*raw.value;T n{};const auto [p,e]=std::from_chars(s.data(),s.data()+s.size(),n);
    if(s.empty() || e!=std::errc{} || p!=s.data()+s.size())return missing<T>(std::string(source)+": malformed/out-of-range integer");
    return {n,Support::available,source};
}
Metric<std::string> label(const Metric<std::string>& raw,const char* source){
    if(!measured(raw))return missing<std::string>(std::string(source)+": "+raw.note,raw.support);
    if(raw.value->empty() || raw.value->size()>256 || !std::all_of(raw.value->begin(),raw.value->end(),[](unsigned char c){return c>=32&&c<=126;}))return missing<std::string>(std::string(source)+": invalid/empty text");
    return {*raw.value,Support::available,source};
}
void memory(const Metric<std::string>& total_raw,const Metric<std::string>& used_raw,const char* source,
            Metric<std::uint64_t>& total,Metric<std::uint64_t>& used,Metric<std::uint64_t>& headroom){
    total=integer<std::uint64_t>(total_raw,source);used=integer<std::uint64_t>(used_raw,source);
    headroom=missing<std::uint64_t>(std::string(source)+": total and used not both known");
    if(total.value && used.value){
        if(*used.value>*total.value){used=missing<std::uint64_t>(std::string(source)+": used exceeds total (sequential observations)");return;}
        headroom={*total.value-*used.value,Support::estimated,std::string(source)+": total-minus-used headroom, non-atomic; NOT allocatable/free-memory guarantee"};
    }
}
}
GpuInfo parse_linux_gpu_sysfs(const std::string& id,const LinuxGpuSysfsInputs& in){
    GpuInfo g;g.id=id;
    auto vendor=label(in.vendor,"DRM device/vendor"),device=label(in.device,"DRM device/device");
    if(vendor.value)g.vendor=*vendor.value;
    if(device.value)g.model=*device.value;
    g.physical_location=missing<std::string>("No proven PCI location",in.location.support==Support::permission_required?Support::permission_required:Support::unsupported);
    if(measured(in.location) && !nvml_pci_identity(*in.location.value).empty())g.physical_location=in.location;
    LinuxGpuSysfsDetails details;
    details.kernel_driver_name=label(in.driver,"sysfs driver binding name, NOT driver version");
    details.module_name=label(in.module,"sysfs driver/module binding name, NOT driver version");
    details.module_version=label(in.module_version,"bound kernel module version attribute, NOT kernel release/API version");
    g.driver_version=details.module_version;
    memory(in.vram_total,in.vram_used,"DRM mem_info_vram_* bytes",details.vram_total_bytes,details.vram_used_bytes,details.vram_headroom_bytes);
    memory(in.gtt_total,in.gtt_used,"DRM mem_info_gtt_* manager bytes",details.gtt_total_bytes,details.gtt_used_bytes,details.gtt_headroom_bytes);
    g.dedicated_memory_bytes=details.vram_total_bytes;g.memory_used_bytes=g.dedicated_memory_used_bytes=details.vram_used_bytes;
    g.memory_available_bytes=details.vram_headroom_bytes;
    g.shared_memory_bytes=missing<std::uint64_t>("No Windows-style shared GPU capacity provider; GTT is separate Linux manager accounting");
    g.shared_memory_used_bytes=missing<std::uint64_t>("No Windows-style shared GPU usage provider; GTT is NOT relabeled as shared GPU memory");
    const auto busy=integer<unsigned>(in.busy,"DRM gpu_busy_percent");
    g.utilization_percent=missing<double>(busy.note,busy.support);
    if(busy.value && *busy.value<=100){g.utilization_percent={static_cast<double>(*busy.value),Support::available,"DRM reported overall gpu_busy_percent, NOT a named 3D/copy engine"};g.engines.push_back({"overall",g.utilization_percent});}
    else if(busy.value)g.utilization_percent=missing<double>("DRM gpu_busy_percent outside 0..100");
    g.temperature_celsius=missing<double>("No unique identified GPU edge/core temperature sensor");
    std::set<std::string> ids;std::vector<Metric<double>> candidates;bool denied_candidate=false;
    bool sensors_valid=in.sensors.size()<=128;
    for(const auto& raw:in.sensors)if(!identifier(raw.id) || !ids.insert(raw.id).second)sensors_valid=false;
    if(sensors_valid)for(const auto& raw:in.sensors){
        LinuxGpuTemperatureSensor sensor; sensor.id=raw.id;sensor.label=label(raw.label,"hwmon channel label");
        const auto temperature=integer<std::int64_t>(raw.millidegrees,"hwmon millidegrees Celsius");
        sensor.celsius=missing<double>(temperature.note,temperature.support);
        if(temperature.value && *temperature.value>=-273150 && *temperature.value<=1000000)
            sensor.celsius={static_cast<double>(*temperature.value)/1000.0,Support::available,"Device-local hwmon "+raw.id+"; channel label must be considered"};
        else if(temperature.value)sensor.celsius=missing<double>("hwmon temperature outside sanity range -273.15..1000 Celsius");
        const bool named=sensor.label.value && (*sensor.label.value=="edge" || *sensor.label.value=="GPU" || *sensor.label.value=="gpu" || *sensor.label.value=="core");
        const bool amdgpu_temp1=details.kernel_driver_name.value=="amdgpu" && raw.id.ends_with(".temp1") && !raw.label.value && raw.label.support==Support::unsupported;
        if(sensor.celsius.value && (named || amdgpu_temp1))candidates.push_back(sensor.celsius);
        if((named || amdgpu_temp1) && sensor.celsius.support==Support::permission_required)denied_candidate=true;
        details.temperature_sensors.push_back(std::move(sensor));
    }
    if(candidates.size()==1 && !denied_candidate)g.temperature_celsius=candidates.front();
    else if(denied_candidate)g.temperature_celsius=missing<double>("Identified GPU temperature candidate denied; unique available sensor not established",Support::permission_required);
    g.driver_date=linux_gpu_driver_date_unavailable(LinuxGpuDriverDateProvider::drm_sysfs);
    g.vulkan_version=missing<std::string>("Vulkan PCI provider not queried");g.opencl_version=missing<std::string>("OpenCL PCI provider not queried");
    g.opengl_version=missing<std::string>("Current EGL/GLX desktop context query disabled; opt in on its owning thread");
    g.linux_sysfs_details={std::move(details),Support::available,"Per-field device-local sysfs observations; not atomic, not complete host GPU inventory"};
    return g;
}
Metric<std::string> linux_gpu_inventory_source(const Metric<std::string>& drm,Support nvml_support,const std::string& nvml_note){
    const bool has_drm=metric_is_known(drm),has_nvml=nvml_support==Support::available || nvml_support==Support::estimated;
    const std::string note="Selected source coverage only, not all host GPUs/atomic snapshot; DRM: "+drm.note+"; NVML: "+nvml_note;
    if(has_drm || has_nvml)return {has_drm?(has_nvml?"DRM/sysfs+NVML":"DRM/sysfs"):"NVML",
        has_drm && has_nvml && drm.support==Support::available && nvml_support==Support::available?Support::available:Support::estimated,note};
    return missing<std::string>(note,drm.support==Support::permission_required || nvml_support==Support::permission_required?Support::permission_required:Support::unsupported);
}
#ifdef __linux__
namespace {
struct Fd{int n{-1};~Fd(){if(n>=0)::close(n);}};
Support support(int e){return e==EACCES || e==EPERM?Support::permission_required:Support::unsupported;}
Metric<std::string> read_at(int dir,const std::string& name){
    const Fd fd{::openat(dir,name.c_str(),O_RDONLY|O_CLOEXEC|O_NOFOLLOW|O_NONBLOCK)};
    if(fd.n<0)return missing<std::string>("attribute open errno "+std::to_string(errno),support(errno));
    std::string text;char bytes[256];unsigned interrupts=0;
    for(;;){const auto n=::read(fd.n,bytes,sizeof bytes);if(n<0){const int e=errno;if(e==EINTR && ++interrupts<=3)continue;return missing<std::string>("attribute read errno "+std::to_string(e),support(e));}if(!n)break;
        if(static_cast<std::size_t>(n)>4096-text.size())return missing<std::string>("attribute exceeds 4KiB bound");
        text.append(bytes,static_cast<std::size_t>(n));}
    while(!text.empty() && (text.back()=='\n'||text.back()=='\r'||text.back()==' '||text.back()=='\t'))text.pop_back();
    return {std::move(text),Support::available,"bounded native sysfs attribute"};
}
Metric<std::string> link_at(int dir,const char* name){char text[4096];const auto n=::readlinkat(dir,name,text,sizeof text);
    if(n<0)return missing<std::string>("binding unavailable",support(errno));
    if(!n || static_cast<std::size_t>(n)==sizeof text)return missing<std::string>("binding empty/truncated");
    return {std::string(text,static_cast<std::size_t>(n)),Support::available,"sysfs binding link"};
}
Metric<std::string> binding_name(const Metric<std::string>& link){
    if(!link.value)return link;
    const auto name=std::filesystem::path(*link.value).filename().string();
    return identifier(name)?Metric<std::string>{name,Support::available,"sysfs binding basename"}:missing<std::string>("invalid binding basename");
}
bool same(int a,int b){struct stat x{},y{};return a>=0 && b>=0 && !::fstat(a,&x) && !::fstat(b,&y) && x.st_dev==y.st_dev && x.st_ino==y.st_ino;}
std::optional<std::vector<std::string>> names(int dir,const char* child,std::string_view prefix){
    const int fd=::openat(dir,child,O_RDONLY|O_DIRECTORY|O_CLOEXEC);if(fd<0)return {};
    auto* d=::fdopendir(fd);if(!d){::close(fd);return {};}
    struct Close{DIR* d;~Close(){::closedir(d);}} guard{d};std::vector<std::string> out;unsigned entries=0;
    for(;;){errno=0;auto* e=::readdir(d);if(!e){if(errno)return {};break;}if(++entries>1024)return {};const std::string n=e->d_name;if(numbered(n,prefix))out.push_back(n);}
    std::sort(out.begin(),out.end());return out;
}
}
Metric<GpuInfo> read_linux_gpu_sysfs_card(const std::filesystem::path& path,const std::function<void()>& after_read){
    const auto id=path.filename().string();if(!numbered(id,"card"))return missing<GpuInfo>("Not a DRM card basename");
    const Fd card{::open(path.c_str(),O_PATH|O_DIRECTORY|O_CLOEXEC)};
    if(card.n<0)return missing<GpuInfo>("DRM card directory unavailable",support(errno));
    const Fd dev{::openat(card.n,"device",O_PATH|O_DIRECTORY|O_CLOEXEC)};
    if(dev.n<0)return missing<GpuInfo>("DRM device directory unavailable",support(errno));
    LinuxGpuSysfsInputs in;in.vendor=read_at(dev.n,"vendor");in.device=read_at(dev.n,"device");
    std::error_code ec;const auto canonical=std::filesystem::canonical(path/"device",ec);
    if(!ec && !nvml_pci_identity(canonical.filename().string()).empty()){
        std::error_code subsystem_error;const auto subsystem=std::filesystem::canonical(path/"device/subsystem",subsystem_error);
        if(!subsystem_error && subsystem=="/sys/bus/pci")in.location={canonical.filename().string(),Support::available,"DRM PCI subsystem and canonical device BDF"};
    }
    const auto driver=link_at(dev.n,"driver");in.driver=binding_name(driver);
    const Fd driver_fd{::openat(dev.n,"driver",O_PATH|O_DIRECTORY|O_CLOEXEC)};
    const auto module=link_at(driver_fd.n,"module");in.module=binding_name(module);
    const Fd module_fd{::openat(driver_fd.n,"module",O_PATH|O_DIRECTORY|O_CLOEXEC)};
    in.module_version=read_at(module_fd.n,"version");
    in.busy=read_at(dev.n,"gpu_busy_percent");in.vram_total=read_at(dev.n,"mem_info_vram_total");in.vram_used=read_at(dev.n,"mem_info_vram_used");
    in.gtt_total=read_at(dev.n,"mem_info_gtt_total");in.gtt_used=read_at(dev.n,"mem_info_gtt_used");
    if(const auto hwmons=names(dev.n,"hwmon","hwmon"))for(const auto& hw:*hwmons){
        const Fd h{::openat(dev.n,("hwmon/"+hw).c_str(),O_PATH|O_DIRECTORY|O_CLOEXEC)};
        if(h.n<0)continue;
        const auto begin=in.sensors.size();
        for(unsigned channel=1;channel<=32 && in.sensors.size()<128;++channel){
            const auto prefix="temp"+std::to_string(channel);auto value=read_at(h.n,prefix+"_input");
            if(!value.value && value.support!=Support::permission_required)continue;
            in.sensors.push_back({hw+"."+prefix,read_at(h.n,prefix+"_label"),std::move(value)});
        }
        const Fd checked{::openat(dev.n,("hwmon/"+hw).c_str(),O_PATH|O_DIRECTORY|O_CLOEXEC)};
        if(!same(h.n,checked.n))in.sensors.resize(begin);
    }
    if(after_read)after_read();
    const Fd last_card{::open(path.c_str(),O_PATH|O_DIRECTORY|O_CLOEXEC)},last_dev{::openat(last_card.n,"device",O_PATH|O_DIRECTORY|O_CLOEXEC)};
    const auto last_path=std::filesystem::canonical(path/"device",ec);
    if(!same(card.n,last_card.n)||!same(dev.n,last_dev.n)||driver.value!=link_at(dev.n,"driver").value ||
       in.vendor.value!=read_at(dev.n,"vendor").value || in.device.value!=read_at(dev.n,"device").value ||
       (!canonical.empty() && (ec || canonical!=last_path)))return missing<GpuInfo>("DRM card/device/binding identity changed; row discarded");
    if(driver_fd.n>=0){const Fd last{::openat(dev.n,"driver",O_PATH|O_DIRECTORY|O_CLOEXEC)};
        if(!same(driver_fd.n,last.n)||module.value!=link_at(last.n,"module").value)return missing<GpuInfo>("DRM driver/module binding changed; row discarded");}
    if(module_fd.n>=0){const Fd last{::openat(driver_fd.n,"module",O_PATH|O_DIRECTORY|O_CLOEXEC)};
        if(!same(module_fd.n,last.n)||in.module_version.value!=read_at(last.n,"version").value)return missing<GpuInfo>("DRM module identity/version changed; row discarded");}
    return {parse_linux_gpu_sysfs(id,in),Support::available,"Bounded per-field sysfs GPU row with directory/binding rechecks"};
}
Metric<std::string> fill_linux_gpu_sysfs(std::vector<GpuInfo>& out,const std::filesystem::path& root){
    std::error_code ec;std::filesystem::directory_iterator it(root,ec),end;unsigned entries=0;bool partial=false;
    if(ec)return missing<std::string>("DRM directory unavailable: "+ec.message(),ec==std::errc::permission_denied?Support::permission_required:Support::unsupported);
    for(;!ec && it!=end && ++entries<=1024;it.increment(ec)){
        if(!numbered(it->path().filename().string(),"card"))continue;
        const auto row=read_linux_gpu_sysfs_card(it->path());if(row.value)out.push_back(*row.value);else partial=true;
    }
    partial=partial || static_cast<bool>(ec) || entries>1024;
    return {"DRM/sysfs",partial?Support::estimated:Support::available,partial?"DRM enumeration partial or rows rejected/bound exceeded":"DRM enumeration completed (possibly empty)"};
}
#else
Metric<GpuInfo> read_linux_gpu_sysfs_card(const std::filesystem::path&,const std::function<void()>&){return missing<GpuInfo>("Linux required");}
Metric<std::string> fill_linux_gpu_sysfs(std::vector<GpuInfo>&,const std::filesystem::path&){return missing<std::string>("Linux required");}
#endif
}
