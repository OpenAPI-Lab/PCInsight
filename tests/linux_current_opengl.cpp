#include "linux_egl_fixture_api.hpp"
#include "pcinsight/monitor.hpp"
#include <iostream>
namespace f=egl_fixture;
using namespace pcinsight;
using namespace pcinsight::detail;
std::vector<GpuInfo> targets(){std::vector<GpuInfo> g(2);g[0].physical_location={std::string("0000:03:00.0"),Support::available,"DRM"};g[1].physical_location={std::string("0001:03:00.0"),Support::available,"other domain"};for(auto& gpu:g)gpu.opengl_version={std::string("stale"),Support::available,"old"};return g;}
int main(int argc,char**){
    if(argc>1){auto api=load_linux_current_egl();assert(api.library);api.pci_identity=f::identity;f::reset(0);auto g=targets();fill_linux_current_opengl(g,api);assert(g[0].opengl_version.value=="4.6 (Core Profile) fixture" && !g[1].opengl_version.value);std::cout<<"EGL dynamic loader fixture passed\n";return 0;}
    f::reset(0);auto g=targets();fill_linux_current_opengl(g,f::api());assert(g[0].opengl_version.value=="4.6 (Core Profile) fixture" && !g[1].opengl_version.value && f::identity_calls==2);
    for(const char* value:{"4.6", "4.6.0", "4.6.0 NVIDIA fixture", "3.3.12 Vendor", "4.6 (Core Profile) Mesa fixture"}){
        f::reset(0);f::version_override=value;g=targets();fill_linux_current_opengl(g,f::api());
        assert(g[0].opengl_version.value==value && !g[1].opengl_version.value);
    }
    for(const char* value:{"", "4", "4.", "4..0", "4.6.", "4.6.x", "4.6.0.1", "4.6.0suffix", "4.6suffix", "-4.6", "+4.6", "0.6", " 4.6", "4.-6", "4.+6", "4.6.-1", "4.6.+1", "4294967296.6", "4.4294967296", "4.6.4294967296", "OpenGL ES 3.2 Vendor"}){
        f::reset(0);f::version_override=value;g=targets();fill_linux_current_opengl(g,f::api());
        assert(!g[0].opengl_version.value && !g[1].opengl_version.value);
    }
    for(int m=1;m<=24;++m){f::reset(m);g=targets();fill_linux_current_opengl(g,f::api());if(m==19 || m==20)assert(g[0].opengl_version.value);else assert(!g[0].opengl_version.value);assert(!g[1].opengl_version.value);if(m==1 || m==2 || m==3 || m==9)assert(f::gl_calls==0);if(m==16)assert(g[0].opengl_version.support==Support::permission_required);}
    f::reset(0);g=targets();g.push_back(g[0]);fill_linux_current_opengl(g,f::api());assert(!g[0].opengl_version.value && !g[2].opengl_version.value);
    f::reset(0);g=targets();g[0].physical_location.support=Support::estimated;fill_linux_current_opengl(g,f::api());assert(!g[0].opengl_version.value);
    g=targets();auto missing=f::api();missing.query_device=nullptr;f::reset(0);fill_linux_current_opengl(g,missing);assert(!g[0].opengl_version.value && f::display_calls==0);
    auto real=load_linux_current_egl();g=targets();fill_linux_current_opengl(g,real);
    for(const auto& gpu:g)std::cout<<*gpu.physical_location.value<<" | GL="<<gpu.opengl_version.value.value_or("unavailable")<<'\n';
    SampleOptions options;assert(!options.include_current_opengl_context);options.include_current_opengl_context=true;options.include_gpus=false;Monitor monitor;assert(monitor.sample(options).gpus.empty());options.include_gpus=true;
    for(const auto& gpu:monitor.sample(options).gpus)if(gpu.opengl_version.value)assert(gpu.opengl_version.note.starts_with("GL_VERSION of current caller-thread"));
    std::cout<<"Current EGL OpenGL context tests passed\n";
}
