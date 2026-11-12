#include "linux_current_opengl.hpp"
#ifdef __linux__
#include "linux_nvml.hpp"
#include "linux_gpu_opencl.hpp"
#include <algorithm>
#include <array>
#include <cerrno>
#include <charconv>
#include <cstdio>
#include <cstring>
#include <dlfcn.h>
#include <fcntl.h>
#include <limits>
#include <sys/stat.h>
#include <unistd.h>

namespace pcinsight::detail {
namespace {
Metric<std::string> absent(const char* note,Support support=Support::unsupported){return {{},support,note};}
bool valid_device(const MesaGlInteropDeviceV1& d){
    return d.version==1 && d.pci_segment_group!=std::numeric_limits<std::uint32_t>::max() &&
        d.pci_bus<=255 && d.pci_device<=31 && d.pci_function<=7 &&
        d.vendor_id>0 && d.vendor_id<0xffff && d.device_id>0 && d.device_id<0xffff;
}
std::string key(const MesaGlInteropDeviceV1& d){
    return std::to_string(d.pci_segment_group)+":"+std::to_string(d.pci_bus)+":"+
        std::to_string(d.pci_device)+"."+std::to_string(d.pci_function);
}
struct Fd{int n;~Fd(){if(n>=0)::close(n);}};
struct HexResult{std::optional<unsigned> value;int error{};};
HexResult read_hex(int dir,const char* name){
    const Fd fd{::openat(dir,name,O_RDONLY|O_CLOEXEC|O_NOFOLLOW|O_NONBLOCK)};
    if(fd.n<0)return {{},errno};
    std::array<char,64> bytes{};const auto n=::read(fd.n,bytes.data(),bytes.size());
    if(n<0)return {{},errno};
    if(!n || static_cast<std::size_t>(n)>=bytes.size())return {{},EOVERFLOW};
    std::string_view s(bytes.data(),static_cast<std::size_t>(n));
    while(!s.empty() && (s.back()=='\n' || s.back()=='\r' || s.back()==' '))s.remove_suffix(1);
    if(s.starts_with("0x"))s.remove_prefix(2);
    unsigned value{};const auto [end,ec]=std::from_chars(s.data(),s.data()+s.size(),value,16);
    return ec==std::errc{} && end==s.data()+s.size()?HexResult{value,0}:HexResult{{},EINVAL};
}
Metric<std::string> pci_impl(const MesaGlInteropDeviceV1& d,const std::string& root,const std::function<void()>& after_read){
    if(!valid_device(d))return absent("GLX interop PCI identity invalid");
    char bdf[32];std::snprintf(bdf,sizeof bdf,"%08x:%02x:%02x.%x",d.pci_segment_group,d.pci_bus,d.pci_device,d.pci_function);
    // sysfs uses four domain digits for ordinary PCI domains, eight for larger.
    std::string name=bdf;if(d.pci_segment_group<=0xffff)name.erase(0,4);
    const auto path=root+"/"+name;
    const Fd dir{::open(path.c_str(),O_PATH|O_DIRECTORY|O_CLOEXEC)};
    if(dir.n<0)return absent("GLX PCI sysfs directory unavailable",errno==EACCES || errno==EPERM?Support::permission_required:Support::unsupported);
    struct stat first{},last{};
    if(::fstat(dir.n,&first))return absent("GLX PCI directory stat failed");
    const auto vendor=read_hex(dir.n,"vendor"),device=read_hex(dir.n,"device");
    if(vendor.error==EPERM || vendor.error==EACCES || device.error==EPERM || device.error==EACCES)
        return absent("GLX PCI attributes denied",Support::permission_required);
    if(!vendor.value || !device.value || *vendor.value!=d.vendor_id || *device.value!=d.device_id)return absent("GLX PCI vendor/device not corroborated by sysfs");
    if(after_read)after_read();
    const Fd checked{::open(path.c_str(),O_PATH|O_DIRECTORY|O_CLOEXEC)};
    if(checked.n<0 || ::fstat(checked.n,&last) || first.st_dev!=last.st_dev || first.st_ino!=last.st_ino)
        return absent("GLX PCI sysfs directory changed during lookup");
    if(read_hex(checked.n,"vendor").value!=vendor.value || read_hex(checked.n,"device").value!=device.value)
        return absent("GLX PCI attributes changed during lookup");
    return {key(d),Support::available,"Mesa interop BDF and PCI vendor/device corroborated by pinned sysfs directory"};
}
Metric<std::string> pci(const MesaGlInteropDeviceV1& d){return pci_impl(d,"/sys/bus/pci/devices",{});}
template<class T>void symbol(void* module,T& fn,const char* name){void* p=dlsym(module,name);static_assert(sizeof p==sizeof fn);std::memcpy(&fn,&p,sizeof p);}
MesaGlInteropDeviceV1 request(){
    MesaGlInteropDeviceV1 out;out.pci_segment_group=out.pci_bus=out.pci_device=out.pci_function=out.vendor_id=out.device_id=std::numeric_limits<std::uint32_t>::max();return out;
}
bool renderer_scalar(const CurrentGlxApi& api,int attr,unsigned expected){
    unsigned value=std::numeric_limits<unsigned>::max();return api.renderer_integer(attr,&value)==1 && value==expected;
}
void profile(const CurrentGlxApi& api,int attr,Metric<bool>& supported,Metric<std::string>& maximum){
    supported={{},Support::unsupported,"GLX profile capability query unavailable/invalid"};
    maximum=absent("GLX profile capability query unavailable/invalid");
    unsigned values[2]{std::numeric_limits<unsigned>::max(),std::numeric_limits<unsigned>::max()};
    if(api.renderer_integer(attr,values)!=1)return;
    if(!values[0] && !values[1]){
        supported={false,Support::available,"Renderer reports this profile unsupported (0,0)"};
        maximum=absent("Renderer reports this profile unsupported (0,0)");return;
    }
    // Do not invent a future-version upper bound, but reject malformed partial
    // writes/sentinels and minor components unsuitable for an API version.
    if(!values[0] || values[0]==std::numeric_limits<unsigned>::max() || values[1]==std::numeric_limits<unsigned>::max())return;
    supported={true,Support::available,"Renderer-reported profile support"};
    maximum={std::to_string(values[0])+"."+std::to_string(values[1]),Support::available,
        "GLX_MESA_query_renderer reported profile maximum, NOT context creation tested or universal GPU maximum"};
}
}
Metric<std::string> read_linux_glx_pci(const MesaGlInteropDeviceV1& d,const std::string& root,const std::function<void()>& after_read){return pci_impl(d,root,after_read);}
CurrentGlxApi load_linux_current_glx(){
    CurrentGlxApi api;api.pci_identity=pci;
    void* lib=dlopen("libGLX.so.0",RTLD_NOW|RTLD_LOCAL);
    if(!lib)lib=dlopen("libGL.so.1",RTLD_NOW|RTLD_LOCAL);
    if(!lib)return api;
    api.library=std::shared_ptr<void>(lib,[](void* p){dlclose(p);});
    symbol(lib,api.context,"glXGetCurrentContext");symbol(lib,api.display,"glXGetCurrentDisplay");
    symbol(lib,api.query_context,"glXQueryContext");symbol(lib,api.query_version,"glXQueryVersion");
    symbol(lib,api.extensions,"glXQueryExtensionsString");symbol(lib,api.is_direct,"glXIsDirect");
    EglProc (*get_proc)(const unsigned char*){};symbol(lib,get_proc,"glXGetProcAddressARB");
    if(!get_proc)symbol(lib,get_proc,"glXGetProcAddress");
    if(get_proc){
        auto proc=[&](auto& fn,const char* name){const auto p=get_proc(reinterpret_cast<const unsigned char*>(name));static_assert(sizeof p==sizeof fn);std::memcpy(&fn,&p,sizeof p);};
        // GLX-dispatched name, NOT a direct call into an arbitrary Mesa vendor
        // DSO with a context belonging to a different GLVND vendor.
        proc(api.device_info,"glXGLInteropQueryDeviceInfoMESA");
        proc(api.renderer_integer,"glXQueryCurrentRendererIntegerMESA");
        proc(api.gl_string,"glGetString");
    }
    return api;
}
void fill_linux_current_glx(std::vector<GpuInfo>& gpus,const CurrentGlxApi& api){
    for(auto& gpu:gpus){gpu.opengl_version=absent("No proven matching current direct GLX/Mesa PCI context");gpu.linux_opengl_details={{},Support::unsupported,"No proven matching current GLX context"};}
    if(gpus.empty() || !api.display || !api.context || !api.query_context || !api.extensions || !api.is_direct ||
       !api.query_version || !api.device_info || !api.renderer_integer || !api.gl_string || !api.pci_identity)return;
    auto* display=api.display();auto* context=api.context();if(!display || !context)return;
    int major=0,minor=0,screen=-1;
    if(api.query_version(display,&major,&minor)!=1 || major<1 || minor<0 || (major==1 && minor<4) ||
       api.query_context(display,context,0x800c,&screen)!=0 || screen<0 || api.is_direct(display,context)!=1)return;
    const auto ext=bounded_gl_text(api.extensions(display,screen),65536);
    if(!ext || !has_opencl_extension(*ext,"GLX_MESA_gl_interop") || !has_opencl_extension(*ext,"GLX_MESA_query_renderer"))return;
    auto before=request();if(api.device_info(display,context,&before)!=0 || !valid_device(before))return;
    if(!renderer_scalar(api,0x8186,1) || !renderer_scalar(api,0x8183,before.vendor_id) || !renderer_scalar(api,0x8184,before.device_id))return;
    const auto first=api.pci_identity(before);
    if(!first.value || first.support!=Support::available || *first.value!=key(before)){
        if(first.support==Support::permission_required)for(auto& gpu:gpus)gpu.opengl_version.support=Support::permission_required;
        return;
    }
    const auto version=bounded_gl_text(reinterpret_cast<const char*>(api.gl_string(0x1f02)),1024);
    if(!version || !valid_desktop_gl_version(*version))return;
    LinuxOpenGlDetails details;details.context_api="GLX";details.provider="GLX_MESA_gl_interop/GLX_MESA_query_renderer/sysfs";
    details.context_version={*version,Support::available,"GL_VERSION of current caller-thread direct GLX context; NOT renderer maximum"};
    profile(api,0x818a,details.core_profile_supported,details.reported_max_core_profile_version);
    profile(api,0x818b,details.compatibility_profile_supported,details.reported_max_compatibility_profile_version);
    auto after=request();if(api.device_info(display,context,&after)!=0 || after!=before)return;
    if(!renderer_scalar(api,0x8186,1) || !renderer_scalar(api,0x8183,after.vendor_id) || !renderer_scalar(api,0x8184,after.device_id))return;
    const auto last=api.pci_identity(after);int last_screen=-1;
    if(last.value!=first.value || last.support!=Support::available || api.display()!=display || api.context()!=context ||
       api.query_context(display,context,0x800c,&last_screen)!=0 || last_screen!=screen)return;
    GpuInfo* match=nullptr;unsigned matches=0;
    for(auto& gpu:gpus)if(gpu.physical_location.value && gpu.physical_location.support==Support::available &&
        nvml_pci_identity(*gpu.physical_location.value)==*first.value){
        // Extra IDs, when available, must agree too. BDF remains the identity;
        // vendor/device IDs alone are never used to choose among identical GPUs.
        if(gpu.hardware_vendor_id.value && (gpu.hardware_vendor_id.support!=Support::available || *gpu.hardware_vendor_id.value!=before.vendor_id))return;
        if(gpu.hardware_device_id.value && (gpu.hardware_device_id.support!=Support::available || *gpu.hardware_device_id.value!=before.device_id))return;
        match=&gpu;++matches;
    }
    if(matches!=1)return;
    match->opengl_version=details.context_version;
    match->linux_opengl_details={std::move(details),Support::available,"GLX context/version and reported profile capabilities matched by Mesa interop PCI identity plus sysfs"};
}
}
#endif
