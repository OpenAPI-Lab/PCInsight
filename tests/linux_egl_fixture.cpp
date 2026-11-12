#include "linux_egl_fixture_api.hpp"
extern "C" {
void* eglGetCurrentDisplay(){return egl_fixture::display();}
void* eglGetCurrentContext(){return egl_fixture::context();}
pcinsight::detail::EglBool eglQueryContext(void* d,void* c,pcinsight::detail::EglInt key,pcinsight::detail::EglInt* out){return egl_fixture::query_context(d,c,key,out);}
const char* eglQueryString(void* d,pcinsight::detail::EglInt key){return egl_fixture::query_string(d,key);}
pcinsight::detail::EglProc eglGetProcAddress(const char* name){
    pcinsight::detail::EglProc out{};
    auto convert=[&](auto fn){static_assert(sizeof(fn)==sizeof(out));std::memcpy(&out,&fn,sizeof(fn));};
    if(std::strcmp(name,"eglQueryDisplayAttribEXT")==0)convert(egl_fixture::query_display);
    else if(std::strcmp(name,"eglQueryDeviceStringEXT")==0)convert(egl_fixture::query_device);
    else if(std::strcmp(name,"glGetString")==0)convert(egl_fixture::gl_string);
    return out;
}
}
