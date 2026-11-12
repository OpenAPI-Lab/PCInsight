#include "../src/windows_current_opengl.hpp"
#include "../src/windows_gpus.hpp"
#include "pcinsight/monitor.hpp"
#include <algorithm>
#include <cassert>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>
using namespace pcinsight;
using namespace pcinsight::detail;
namespace {
unsigned mode=0,bytes_calls=0,version_calls=0,extension_calls=0;
std::string version="4.6.0 fixture",long_text(1024,'x');
HGLRC WINAPI context(){return reinterpret_cast<HGLRC>(std::uintptr_t(mode==28?0:(mode==9 && bytes_calls)?64:16));}
HDC WINAPI dc(){return reinterpret_cast<HDC>(std::uintptr_t(mode==29?0:(mode==10 && bytes_calls)?64:32));}
const GLubyte* APIENTRY string(GLenum name){
    if(name==GL_VERSION){++version_calls;if(mode==11 && version_calls>1)return reinterpret_cast<const GLubyte*>("4.5 changed");if(mode==12)return nullptr;if(mode==13)return reinterpret_cast<const GLubyte*>(long_text.c_str());return reinterpret_cast<const GLubyte*>(version.c_str());}
    assert(name==GL_EXTENSIONS);++extension_calls;
    if(mode==2)return reinterpret_cast<const GLubyte*>("GL_EXT_memory_object GL_EXT_memory_object_win32_suffix");
    if(mode==16)return reinterpret_cast<const GLubyte*>("GL_EXT_memory_object GL_EXT_memory_object");
    if(mode==17)return reinterpret_cast<const GLubyte*>("bad\nname");
    if(mode==32)return nullptr;
    return reinterpret_cast<const GLubyte*>(mode==21?"GL_EXT_semaphore GL_EXT_semaphore_win32":"GL_EXT_memory_object GL_EXT_memory_object_win32");
}
void APIENTRY integer(GLenum name,GLint* out){
    if(name==0x821d){*out=mode==14?-1:mode==15?4097:mode==30?0:2;return;}
    assert(false && "No node-mask or other GL state query is needed for adapter attribution");
}
const GLubyte* APIENTRY stringi(GLenum name,GLuint i){
    assert(name==GL_EXTENSIONS && i<2);++extension_calls;
    const char* result=i?"GL_EXT_memory_object_win32":"GL_EXT_memory_object";
    if(mode==2 && i)result="GL_EXT_memory_object_win32_suffix";
    if(mode==3 && !i)result="GL_EXT_unrelated";
    if(mode==16)result="GL_EXT_memory_object";
    if(mode==17)result="bad\nname";
    if(mode==18)result="bad name";
    if(mode==21)result=i?"GL_EXT_semaphore_win32":"GL_EXT_semaphore";
    if(mode==31)return reinterpret_cast<const GLubyte*>(long_text.c_str());
    if(mode==32)return nullptr;
    return reinterpret_cast<const GLubyte*>(result);
}
void APIENTRY bytes(GLenum name,GLubyte* out){
    assert(name==0x9599);++bytes_calls;
    if(mode==33)return;
    LUID id{};id.LowPart=mode==4?0:mode==7 && bytes_calls>1?7:42;id.HighPart=mode==23?-1:0;
    std::memcpy(out,&id,mode==34?4:8);
}
template<class T> PROC proc(T fn){PROC result{};static_assert(sizeof(fn)==sizeof(result));std::memcpy(&result,&fn,sizeof(fn));return result;}
PROC WINAPI resolve(LPCSTR name){
    if(std::strcmp(name,"glGetStringi")==0)return mode==19?nullptr:proc(stringi);
    assert(std::strcmp(name,"glGetUnsignedBytevEXT")==0);
    if(mode==1)return nullptr;
    if(mode>=24 && mode<=27)return reinterpret_cast<PROC>(mode==27?~std::uintptr_t{}:mode-23);
    return proc(bytes);
}
CurrentWglApi api(){CurrentWglApi a;a.context=context;a.dc=dc;a.proc=resolve;a.string=string;a.integer=integer;return a;}
std::vector<GpuInfo> targets(){std::vector<GpuInfo> g(2);g[0].id="dxgi-luid:00000000:0000002a";g[1].id="dxgi-luid:00000000:0000002b";for(auto& row:g){row.opengl_version={"stale",Support::available,"old"};row.memory_used_bytes={7,Support::estimated,"unrelated"};}return g;}
auto run(unsigned m){mode=m;bytes_calls=version_calls=extension_calls=0;auto g=targets();fill_windows_current_opengl(g,api());assert(!g[1].opengl_version.value && g[0].memory_used_bytes.value==7);return g;}
struct NativeContext {
    HGLRC previous=wglGetCurrentContext(),gl{};
    HDC previous_dc=wglGetCurrentDC(),device{};
    HWND window{};ATOM atom{};
    HINSTANCE instance=GetModuleHandleW(nullptr);
    std::wstring name=L"PCInsightWgl-"+std::to_wstring(GetCurrentProcessId())+L"-"+std::to_wstring(GetTickCount64());
    bool open(){
        WNDCLASSW cls{};cls.style=CS_OWNDC;cls.lpfnWndProc=DefWindowProcW;cls.hInstance=instance;cls.lpszClassName=name.c_str();
        atom=RegisterClassW(&cls);if(!atom)return false;
        window=CreateWindowExW(0,name.c_str(),L"Private test",WS_OVERLAPPED,0,0,1,1,nullptr,nullptr,instance,nullptr);if(!window)return false;
        device=GetDC(window);if(!device)return false;
        PIXELFORMATDESCRIPTOR format{};format.nSize=sizeof(format);format.nVersion=1;format.dwFlags=PFD_DRAW_TO_WINDOW|PFD_SUPPORT_OPENGL|PFD_DOUBLEBUFFER;format.iPixelType=PFD_TYPE_RGBA;format.cColorBits=24;
        const auto index=ChoosePixelFormat(device,&format);if(!index || !SetPixelFormat(device,index,&format))return false;
        gl=wglCreateContext(device);return gl && wglMakeCurrent(device,gl);
    }
    ~NativeContext(){
        if(gl){assert(wglMakeCurrent(previous_dc,previous));assert(wglDeleteContext(gl));}
        if(device)assert(ReleaseDC(window,device));
        if(window)assert(DestroyWindow(window));
        if(atom)assert(UnregisterClassW(name.c_str(),instance));
    }
};
}
int main(){
    auto g=run(0);assert(g[0].opengl_version.value==version && g[0].opengl_version.support==Support::available && bytes_calls==2);
    for(unsigned m=1;m<=34;++m){g=run(m);if(m==5 || m==6 || m==8 || m==20 || m==21 || m==22)assert(g[0].opengl_version.value);else assert(!g[0].opengl_version.value);if(m==28 || m==29)assert(!version_calls && !extension_calls && !bytes_calls);}
    mode=0;version="2.1 fixture";assert(run(0)[0].opengl_version.value);
    for(unsigned m:{2U,16U,17U,32U})assert(!run(m)[0].opengl_version.value);
    version="4.6.0 fixture";
    for(const char* bad:{"","OpenGL ES 3.2","0.1","4","4.","4..6","4.6.0.1","4.6bad","4.6\n","65536.1"}){version=bad;assert(!run(0)[0].opengl_version.value);}
    version="4.6.0 fixture";mode=0;bytes_calls=version_calls=0;g=targets();g.push_back(g[0]);fill_windows_current_opengl(g,api());for(const auto& row:g)assert(!row.opengl_version.value);
    mode=23;bytes_calls=version_calls=0;g=targets();g[0].id="dxgi-luid:ffffffff:0000002a";fill_windows_current_opengl(g,api());assert(g[0].opengl_version.value==version && !g[1].opengl_version.value);mode=0;
    mode=4;bytes_calls=version_calls=0;g=targets();g[0].id="dxgi-luid:00000000:00000000";fill_windows_current_opengl(g,api());assert(g[0].opengl_version.value==version);mode=0; // zero is not missing output
    for(unsigned missing=0;missing<5;++missing){auto a=api();if(missing==0)a.context=nullptr;if(missing==1)a.dc=nullptr;if(missing==2)a.proc=nullptr;if(missing==3)a.string=nullptr;if(missing==4)a.integer=nullptr;g=targets();fill_windows_current_opengl(g,a);for(const auto& row:g)assert(!row.opengl_version.value);}
    auto denied=CurrentWglApi{};denied.failure_support=Support::permission_required;g=targets();fill_windows_current_opengl(g,denied);assert(!g[0].opengl_version.value && g[0].opengl_version.support==Support::permission_required);
    const auto prior=wglGetCurrentContext();const auto prior_dc=wglGetCurrentDC();
    if(!prior){g=read_windows_gpus();fill_windows_current_opengl(g);for(const auto& row:g)assert(!row.opengl_version.value);}
    {
        NativeContext native;
        if(native.open()){
            for(const auto field:{GL_VERSION,GL_RENDERER}){
                const auto* raw=glGetString(field);std::string value;
                if(raw)for(unsigned n=0;n<256 && raw[n];++n)value+=static_cast<char>(raw[n]);
                std::cout<<"Private context "<<(field==GL_VERSION?"GL_VERSION":"GL_RENDERER")<<"="<<value<<'\n';
            }
            auto own=read_windows_gpus();
            for(unsigned n=0;n<16 && glGetError()!=GL_NO_ERROR;++n){} // own private context only, never in the library
            assert(glGetError()==GL_NO_ERROR);
            glEnable(0xffffffffU); // seed a private error; collection must not consume it
            fill_windows_current_opengl(own);
            assert(glGetError()==GL_INVALID_ENUM && glGetError()==GL_NO_ERROR);
            // Diagnostic in OUR context only. This is intentionally NOT queried
            // by the library: attribution/version is adapter-level, not a node.
            GLint reported_mask=-1;glGetIntegerv(0x959a,&reported_mask);
            const auto mask_error=glGetError();
            std::cout<<"Private context node_mask="<<static_cast<std::uint32_t>(reported_mask)<<" GL_error="<<mask_error<<'\n';
            using RawBytes=void (APIENTRY*)(GLenum,GLubyte*);
            const auto raw_proc=wglGetProcAddress("glGetUnsignedBytevEXT");RawBytes raw_bytes{};
            static_assert(sizeof(raw_proc)==sizeof(raw_bytes));std::memcpy(&raw_bytes,&raw_proc,sizeof(raw_bytes));
            const auto raw_address=reinterpret_cast<std::uintptr_t>(raw_proc);
            if(raw_address>3 && raw_address!=~std::uintptr_t{}){LUID reported{};raw_bytes(0x9599,reinterpret_cast<GLubyte*>(&reported));std::cout<<"Private context LUID="<<reported.HighPart<<':'<<reported.LowPart<<" GL_error="<<glGetError()<<'\n';}
            fill_windows_current_opengl(own);assert(glGetError()==GL_NO_ERROR); // no seeded error hiding an added error
            assert(wglGetCurrentContext()==native.gl && wglGetCurrentDC()==native.device);
            unsigned matched=0;
            for(const auto& row:own)if(row.opengl_version.value){++matched;std::cout<<"Native WGL matched "<<row.id<<" | "<<*row.opengl_version.value<<'\n';}
            else std::cout<<"Native WGL unknown "<<row.id<<" | "<<row.opengl_version.note<<'\n';
            assert(matched<=1);
            SampleOptions options;options.include_processes=options.include_disks=options.include_networks=false;options.include_current_opengl_context=true;
            Monitor monitor;const auto snapshot=monitor.sample(options);unsigned published=0;
            for(const auto& row:snapshot.gpus)if(row.opengl_version.value){++published;const auto it=std::find_if(own.begin(),own.end(),[&](const auto& old){return old.id==row.id;});assert(it!=own.end() && it->opengl_version.value==row.opengl_version.value);}
            assert(published==matched && wglGetCurrentContext()==native.gl && wglGetCurrentDC()==native.device && glGetError()==GL_NO_ERROR);
            options.include_current_opengl_context=false;for(const auto& row:monitor.sample(options).gpus)assert(!row.opengl_version.value);
            options.include_gpus=false;options.include_current_opengl_context=true;assert(monitor.sample(options).gpus.empty());
            if(!matched)std::cout<<"Native WGL context exists, but no unique EXT Win32 LUID/DXGI match; no adapter version fabricated\n";
        }else std::cout<<"Native private WGL context unavailable on this host; synthetic checks only\n";
    }
    assert(wglGetCurrentContext()==prior && wglGetCurrentDC()==prior_dc);
    std::cout<<"WGL checks: adapter identity, duplicate rows, extension contracts, bounds, context changes, no node-mask query, errors not consumed, opt-in gating and cleanup passed\n";
}
