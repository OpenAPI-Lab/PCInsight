#pragma once
#include "../src/linux_current_opengl.hpp"
#include <cassert>
#include <cstring>
namespace egl_fixture {
using namespace pcinsight;
using namespace pcinsight::detail;
inline int mode,display_calls,context_calls,identity_calls,gl_calls;
inline const char* version_override{};
inline char long_version[1024],long_extensions[65536];
inline void reset(int scenario){mode=scenario;version_override=nullptr;display_calls=context_calls=identity_calls=gl_calls=0;std::memset(long_version,'x',sizeof(long_version));std::memset(long_extensions,'x',sizeof(long_extensions));}
inline void* display(){++display_calls;return mode==1?nullptr:reinterpret_cast<void*>(mode==14 && display_calls>1?9:1);}
inline void* context(){++context_calls;return mode==2?nullptr:reinterpret_cast<void*>(mode==15 && context_calls>1?9:2);}
inline EglBool query_context(void* d,void* c,EglInt attribute,EglInt* out){assert(d==reinterpret_cast<void*>(1) && c==reinterpret_cast<void*>(2) && attribute==0x3097);*out=mode==3?0x30a0:0x30a2;return mode==4?0:1;}
inline const char* query_string(void* d,EglInt name){assert(!d && name==0x3055);if(mode==5)return "EGL_EXT_device_query_suffix";if(mode==6)return nullptr;if(mode==21)return long_extensions;return "EGL_EXT_device_query EGL_other";}
inline EglBool query_display(void* d,EglInt name,std::intptr_t* out){assert(d==reinterpret_cast<void*>(1) && name==0x322c);*out=mode==8?0:3;return mode==7?0:1;}
inline const char* query_device(void* d,EglInt name){assert(d==reinterpret_cast<void*>(3));
    if(name==0x3055){if(mode==9)return "EGL_MESA_device_software EGL_EXT_device_drm";if(mode==10)return "EGL_EXT_device_drm_suffix";return mode==20?"EGL_EXT_device_drm":"EGL_EXT_device_drm_render_node EGL_EXT_device_drm";}
    assert(name==0x3377 || name==0x3233);if(mode==11)return nullptr;if(name==0x3377 && mode==19)return nullptr;
    return name==0x3377?"/dev/dri/renderD128":"/dev/dri/card0";
}
inline const unsigned char* gl_string(std::uint32_t name){assert(name==0x1f02);++gl_calls;const char* value=version_override?version_override:mode==12?"OpenGL ES 3.2 fixture":mode==13?"4.x fixture":mode==22?long_version:mode==23?nullptr:mode==24?"4.6 bad\ntext":"4.6 (Core Profile) fixture";return reinterpret_cast<const unsigned char*>(value);}
inline Metric<std::string> identity(const char* path){assert(std::strcmp(path,"/dev/dri/renderD128")==0 || std::strcmp(path,"/dev/dri/card0")==0);++identity_calls;
    if(mode==16)return {{},Support::permission_required,"fixture denied"};if(mode==17)return {{},Support::unsupported,"fixture no PCI"};
    return {std::string(mode==18 && identity_calls>1?"1:3:0.0":"0:3:0.0"),Support::available,"fixture stat/sysfs identity"};
}
inline CurrentEglApi api(){return {display,context,query_context,query_string,query_display,query_device,gl_string,identity,{}};}
}
