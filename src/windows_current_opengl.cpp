#include "windows_current_opengl.hpp"
#ifdef _WIN32
#include <array>
#include <charconv>
#include <cstdio>
#include <cstring>
#include <optional>
#include <set>
#include <string_view>
namespace pcinsight::detail {
namespace {
constexpr GLenum num_extensions=0x821d, device_luid=0x9599;
using Stringi = const GLubyte* (APIENTRY*)(GLenum, GLuint);
using ByteQuery = void (APIENTRY*)(GLenum, GLubyte*);
template<class T> T converted(PROC proc) {
    const auto address=reinterpret_cast<std::uintptr_t>(proc);
    if(address<=3 || address==~std::uintptr_t{})return {};
    T out{};static_assert(sizeof(out)==sizeof(proc));std::memcpy(&out,&proc,sizeof(out));return out;
}
std::optional<std::string> text(const GLubyte* raw,std::size_t limit) {
    if(!raw)return {};
    std::string out;
    for(std::size_t i=0;i<limit;++i){
        const auto ch=raw[i];if(!ch)return out;
        if(ch<32 || ch>126)return {};
        out+=static_cast<char>(ch);
    }
    return {};
}
std::optional<unsigned> major_version(std::string_view value) {
    value=value.substr(0,value.find(' '));unsigned major=0,count=0;
    while(!value.empty()){
        const auto dot=value.find('.');const auto part=value.substr(0,dot);unsigned n=0;
        const auto [end,error]=std::from_chars(part.data(),part.data()+part.size(),n);
        if(part.empty() || error!=std::errc{} || end!=part.data()+part.size() || n>65535 || (!count && !n))return {};
        if(!count)major=n;
        if(++count>3)return {};
        if(dot==value.npos)return count>=2?std::optional{major}:std::nullopt;
        value.remove_prefix(dot+1);
    }
    return {};
}
std::optional<std::set<std::string>> extensions(const CurrentWglApi& api,unsigned major) {
    std::set<std::string> result;
    if(major>=3){
        const auto get=converted<Stringi>(api.proc("glGetStringi"));if(!get)return {};
        GLint count=-1;api.integer(num_extensions,&count);if(count<0 || count>4096)return {};
        std::size_t bytes=0;
        for(GLint i=0;i<count;++i){
            const auto name=text(get(GL_EXTENSIONS,static_cast<GLuint>(i)),256);
            if(!name || name->empty() || name->find(' ')!=name->npos || name->size()>131072-bytes || !result.insert(*name).second)return {};
            bytes+=name->size();
        }
    }else{
        const auto names=text(api.string(GL_EXTENSIONS),65536);if(!names)return {};
        std::string_view remaining=*names;
        while(!remaining.empty()){
            const auto first=remaining.find_first_not_of(' ');if(first==remaining.npos)break;remaining.remove_prefix(first);
            const auto end=remaining.find(' ');const auto name=remaining.substr(0,end);
            if(name.size()>255 || result.size()>=4096 || !result.emplace(name).second)return {};
            if(end==remaining.npos)break;
            remaining.remove_prefix(end+1);
        }
    }
    return result;
}
std::string identity(const std::array<GLubyte,8>& bytes) {
    LUID luid{};static_assert(sizeof(luid)==8);std::memcpy(&luid,bytes.data(),8);
    char id[40];std::snprintf(id,sizeof(id),"dxgi-luid:%08x:%08x",static_cast<unsigned>(luid.HighPart),static_cast<unsigned>(luid.LowPart));return id;
}
}
CurrentWglApi load_windows_current_wgl() {
    CurrentWglApi api;
    const auto module=LoadLibraryExW(L"opengl32.dll",nullptr,LOAD_LIBRARY_SEARCH_SYSTEM32);
    if(!module){if(GetLastError()==ERROR_ACCESS_DENIED)api.failure_support=Support::permission_required;return api;}
    api.library={module,[](void* handle){FreeLibrary(static_cast<HMODULE>(handle));}};
    api.context=converted<decltype(api.context)>(GetProcAddress(module,"wglGetCurrentContext"));
    api.dc=converted<decltype(api.dc)>(GetProcAddress(module,"wglGetCurrentDC"));
    api.proc=converted<decltype(api.proc)>(GetProcAddress(module,"wglGetProcAddress"));
    api.string=converted<decltype(api.string)>(GetProcAddress(module,"glGetString"));
    api.integer=converted<decltype(api.integer)>(GetProcAddress(module,"glGetIntegerv"));
    return api;
}
void fill_windows_current_opengl(std::vector<GpuInfo>& gpus,const CurrentWglApi& api) {
    for(auto& gpu:gpus)gpu.opengl_version={{},api.failure_support,"No proven matching current desktop WGL context on the calling thread"};
    const auto absent=[&](const char* why){for(auto& gpu:gpus)gpu.opengl_version={{},Support::unsupported,why};};
    if(gpus.empty() || !api.context || !api.dc || !api.proc || !api.string || !api.integer)return;
    const auto context=api.context();const auto dc=api.dc();if(!context || !dc)return;
    const auto unchanged=[&]{return api.context()==context && api.dc()==dc;};
    const auto version=text(api.string(GL_VERSION),1024);if(!version){absent("Missing/malformed bounded WGL GL_VERSION");return;}
    const auto major=major_version(*version);if(!major || !unchanged()){absent("Not a desktop GL version or WGL context changed");return;} // no ES/name/vendor fallback
    const auto exts=extensions(api,*major);if(!exts || !unchanged()){absent("WGL extension inventory missing/malformed or context changed");return;}
    const bool memory=exts->contains("GL_EXT_memory_object") && exts->contains("GL_EXT_memory_object_win32");
    const bool semaphore=exts->contains("GL_EXT_semaphore") && exts->contains("GL_EXT_semaphore_win32");
    if(!memory && !semaphore){absent("WGL EXT memory/semaphore Win32 LUID query contract absent");return;}
    const auto get=converted<ByteQuery>(api.proc("glGetUnsignedBytevEXT"));if(!get || !unchanged()){absent("WGL LUID extension function missing or context changed");return;}
    // GpuInfo identifies a DXGI ADAPTER. EXT's LUID maps directly to that
    // adapter. No physical-node claim: querying DEVICE_NODE_MASK_EXT is neither
    // needed for this context version nor a supported operation on every driver.
    std::array<GLubyte,8> first{},second{};
    // Distinct initial bytes detect success-without-output/partial writes.
    // Zero is an opaque LUID value, not itself evidence of query failure.
    first.fill(static_cast<GLubyte>(0xa5));second.fill(static_cast<GLubyte>(0x5a));
    get(device_luid,first.data());
    if(!unchanged()){absent("WGL context changed during LUID query");return;}
    get(device_luid,second.data());
    const auto again=text(api.string(GL_VERSION),1024);
    if(first!=second || again!=version || !unchanged()){absent("WGL context/DC/LUID/version changed during observation");return;}
    const auto id=identity(first);GpuInfo* match=nullptr;unsigned count=0;
    for(auto& gpu:gpus)if(gpu.id==id){match=&gpu;++count;}
    if(count!=1){absent("WGL device LUID does not match exactly one DXGI inventory row");return;}
    match->opengl_version={*version,Support::available,
        "GL_VERSION of current caller-thread WGL context, EXT Win32 LUID matched to one DXGI ADAPTER row; context version, NOT physical-node/maximum GPU capability or hardware-acceleration proof"};
}
void fill_windows_current_opengl(std::vector<GpuInfo>& gpus) {
    if(!gpus.empty())fill_windows_current_opengl(gpus,load_windows_current_wgl());
}
}
#endif
