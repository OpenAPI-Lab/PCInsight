#include "linux_glx_fixture_api.hpp"
extern "C" {
void* glXGetCurrentDisplay(){return glx_fixture::display();}
void* glXGetCurrentContext(){return glx_fixture::context();}
int glXQueryContext(void* d,void* c,int a,int* out){return glx_fixture::query_context(d,c,a,out);}
int glXQueryVersion(void* d,int* major,int* minor){return glx_fixture::query_version(d,major,minor);}
int glXIsDirect(void* d,void* c){return glx_fixture::is_direct(d,c);}
const char* glXQueryExtensionsString(void* d,int screen){return glx_fixture::extensions(d,screen);}
pcinsight::detail::EglProc glXGetProcAddressARB(const unsigned char* raw){
    const char* name=reinterpret_cast<const char*>(raw);pcinsight::detail::EglProc out{};
    auto convert=[&](auto fn){static_assert(sizeof fn==sizeof out);std::memcpy(&out,&fn,sizeof out);};
    if(!std::strcmp(name,"glXGLInteropQueryDeviceInfoMESA"))convert(glx_fixture::device_info);
    else if(!std::strcmp(name,"glXQueryCurrentRendererIntegerMESA"))convert(glx_fixture::renderer_integer);
    else if(!std::strcmp(name,"glGetString"))convert(glx_fixture::gl_string);
    return out;
}
}
