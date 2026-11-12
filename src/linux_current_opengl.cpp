#include "linux_current_opengl.hpp"
#ifdef __linux__
#include "linux_nvml.hpp"
#include "linux_gpu_opencl.hpp"
#include <charconv>
#include <cerrno>
#include <filesystem>
#include <map>
#include <algorithm>
#include <cstring>
#include <dlfcn.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>
namespace pcinsight::detail {
namespace {
std::optional<std::string> text(const char* value,std::size_t limit){
    if(!value)return {};
    const auto size=strnlen(value,limit);if(size==limit)return {};
    std::string out(value,size);
    if(!std::all_of(out.begin(),out.end(),[](unsigned char c){return c>=32 && c<=126;}))return {};
    return out;
}
Metric<std::string> unavailable(const char* why,Support support=Support::unsupported){return {{},support,why};}
Metric<std::string> pci(const char* raw){
    const auto path=text(raw,4096);if(!path || !path->starts_with("/dev/dri/"))return unavailable("EGL DRM node path unavailable or malformed");
    auto name=std::string_view(*path).substr(9);
    if(name.starts_with("renderD"))name.remove_prefix(7);else if(name.starts_with("card"))name.remove_prefix(4);else return unavailable("Not a DRM primary/render node path");
    if(name.empty() || name.find_first_not_of("0123456789")!=std::string_view::npos)return unavailable("Malformed DRM node basename");
    struct stat before{},after{};
    if(stat(path->c_str(),&before)!=0)return unavailable("DRM node stat failed",errno==EACCES?Support::permission_required:Support::unsupported);
    if(!S_ISCHR(before.st_mode))return unavailable("DRM node is not a character device");
    const auto base=std::filesystem::path("/sys/dev/char")/(std::to_string(major(before.st_rdev))+":"+std::to_string(minor(before.st_rdev)));
    std::error_code ec;const auto subsystem=std::filesystem::canonical(base/"subsystem",ec);
    if(ec || subsystem!=std::filesystem::path("/sys/class/drm"))return unavailable("Character device is not a proven DRM node");
    const auto device=std::filesystem::canonical(base/"device",ec);if(ec)return unavailable("DRM PCI sysfs path unavailable");
    const auto key=nvml_pci_identity(device.filename().string());if(key.empty())return unavailable("DRM device has no valid PCI BDF");
    if(stat(path->c_str(),&after)!=0 || before.st_rdev!=after.st_rdev || before.st_dev!=after.st_dev || before.st_ino!=after.st_ino)return unavailable("DRM node changed during identity lookup");
    return {key,Support::available,"DRM char-device/sysfs PCI identity"};
}
bool version(std::string_view value){
    // GL_VERSION permits major.minor or major.minor.release, followed by
    // space-separated vendor information. Preserve the original full string.
    value=value.substr(0,value.find(' '));
    unsigned count=0;
    while(!value.empty()){
        const auto dot=value.find('.');const auto component=value.substr(0,dot);
        unsigned number{};
        const auto result=std::from_chars(component.data(),component.data()+component.size(),number);
        if(component.empty() || result.ec!=std::errc{} || result.ptr!=component.data()+component.size() || (!count && !number))return false;
        if(++count>3)return false;
        if(dot==std::string_view::npos)return count>=2;
        value.remove_prefix(dot+1);
    }
    return false;
}
template<class T> void symbol(void* module,T& fn,const char* name){void* address=dlsym(module,name);static_assert(sizeof(address)==sizeof(fn));std::memcpy(&fn,&address,sizeof(address));}
}
std::optional<std::string> bounded_gl_text(const char* value,std::size_t limit){return text(value,limit);}
bool valid_desktop_gl_version(std::string_view value){return version(value);}
CurrentEglApi load_linux_current_egl(){
    CurrentEglApi api;api.pci_identity=pci;
    void* module=dlopen("libEGL.so.1",RTLD_NOW|RTLD_LOCAL);if(!module)return api;
    api.library=std::shared_ptr<void>(module,[](void* p){dlclose(p);});
    symbol(module,api.display,"eglGetCurrentDisplay");symbol(module,api.context,"eglGetCurrentContext");
    symbol(module,api.query_context,"eglQueryContext");symbol(module,api.query_string,"eglQueryString");
    EglProc (*get_proc)(const char*){};symbol(module,get_proc,"eglGetProcAddress");
    if(get_proc){
        auto extension=[&](auto& fn,const char* name){auto address=get_proc(name);static_assert(sizeof(fn)==sizeof(address));std::memcpy(&fn,&address,sizeof(address));};
        extension(api.query_display,"eglQueryDisplayAttribEXT");extension(api.query_device,"eglQueryDeviceStringEXT");extension(api.gl_string,"glGetString");
    }
    return api;
}
void fill_linux_current_opengl(std::vector<GpuInfo>& gpus,const CurrentEglApi& api){
    for(auto& gpu:gpus){gpu.opengl_version=unavailable("No proven matching desktop OpenGL context current on calling thread (EGL/DRM/PCI)");gpu.linux_opengl_details={{},Support::unsupported,"No proven current EGL context"};}
    if(gpus.empty() || !api.display || !api.context || !api.query_context || !api.query_string || !api.query_display || !api.query_device || !api.gl_string || !api.pci_identity)return;
    auto display=api.display();auto context=api.context();if(!display || !context)return;
    EglInt client{};if(api.query_context(display,context,0x3097,&client)!=1 || client!=0x30a2)return; // desktop GL, not ES
    const auto extensions=text(api.query_string(nullptr,0x3055),65536);
    if(!extensions || (!has_opencl_extension(*extensions,"EGL_EXT_device_query") && !has_opencl_extension(*extensions,"EGL_EXT_device_base")))return;
    std::intptr_t handle{};if(api.query_display(display,0x322c,&handle)!=1 || !handle)return;
    auto device=reinterpret_cast<void*>(handle);
    const auto device_extensions=text(api.query_device(device,0x3055),65536);if(!device_extensions)return;
    if(has_opencl_extension(*device_extensions,"EGL_MESA_device_software"))return;
    std::optional<std::string> node;
    if(has_opencl_extension(*device_extensions,"EGL_EXT_device_drm_render_node"))node=text(api.query_device(device,0x3377),4096);
    if(!node && has_opencl_extension(*device_extensions,"EGL_EXT_device_drm"))node=text(api.query_device(device,0x3233),4096);
    if(!node)return;
    const auto identity=api.pci_identity(node->c_str());
    if(!identity.value || identity.support!=Support::available){if(identity.support==Support::permission_required)for(auto& gpu:gpus)gpu.opengl_version.support=Support::permission_required;return;}
    const auto reported=text(reinterpret_cast<const char*>(api.gl_string(0x1f02)),1024);if(!reported || !version(*reported))return;
    const auto checked=api.pci_identity(node->c_str());
    if(checked.value!=identity.value || checked.support!=Support::available || api.display()!=display || api.context()!=context)return;
    GpuInfo* match=nullptr;unsigned count=0;
    for(auto& gpu:gpus)if(gpu.physical_location.value && gpu.physical_location.support==Support::available && nvml_pci_identity(*gpu.physical_location.value)==*identity.value){match=&gpu;++count;}
    if(count!=1)return;
    match->opengl_version={*reported,Support::available,"GL_VERSION of current caller-thread desktop EGL context, matched by DRM/sysfs PCI; context version, NOT maximum GPU capability or hardware-acceleration certification"};
    LinuxOpenGlDetails details;details.context_api="EGL";details.provider="EGL_EXT_device_query/DRM/sysfs";details.context_version=match->opengl_version;
    details.reported_max_core_profile_version=unavailable("EGL device query does not report profile maximum");
    details.reported_max_compatibility_profile_version=details.reported_max_core_profile_version;
    details.core_profile_supported={{},Support::unsupported,"Profile capability not queried"};details.compatibility_profile_supported=details.core_profile_supported;
    match->linux_opengl_details={std::move(details),Support::available,"Current EGL context attributed by DRM/sysfs PCI"};
}
void fill_linux_current_opengl(std::vector<GpuInfo>& gpus,const CurrentEglApi& egl,const CurrentGlxApi& glx){
    const bool has_egl=egl.context && egl.context(),has_glx=glx.context && glx.context();
    if(has_egl && has_glx){for(auto& gpu:gpus){gpu.opengl_version=unavailable("Both EGL and GLX report current contexts; ambiguous GL dispatch, no attribution");gpu.linux_opengl_details={{},Support::unsupported,"Ambiguous current context APIs"};}return;}
    if(has_glx)fill_linux_current_glx(gpus,glx);else fill_linux_current_opengl(gpus,egl);
}
void fill_linux_current_opengl(std::vector<GpuInfo>& gpus){if(!gpus.empty())fill_linux_current_opengl(gpus,load_linux_current_egl(),load_linux_current_glx());}
}
#endif
