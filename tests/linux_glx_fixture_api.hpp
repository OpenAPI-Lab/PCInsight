#pragma once
#include "../src/linux_current_opengl.hpp"
#include <cassert>
#include <cstring>
namespace glx_fixture {
using namespace pcinsight;
using namespace pcinsight::detail;
inline int mode{},displays{},contexts{},device_reads{},identities{},gl_reads{},screens{};
inline char long_version[1024],long_extensions[65536];
inline void reset(int m=0){mode=m;displays=contexts=device_reads=identities=gl_reads=screens=0;std::memset(long_version,'x',sizeof long_version);std::memset(long_extensions,'x',sizeof long_extensions);}
inline void* display(){++displays;return mode==1?nullptr:reinterpret_cast<void*>(mode==30 && displays>1?9:1);}
inline void* context(){++contexts;return mode==2?nullptr:reinterpret_cast<void*>(mode==29 && contexts>1?9:2);}
inline int query_context(void* d,void* c,int attribute,int* out){
    assert(d==reinterpret_cast<void*>(1) && c==reinterpret_cast<void*>(2) && attribute==0x800c);
    ++screens;*out=mode==5?-1:mode==34 && screens>1?1:0;return mode==4?1:0;
}
inline int query_version(void* d,int* major,int* minor){assert(d==reinterpret_cast<void*>(1));*major=1;*minor=mode==3?3:4;return mode==35?0:1;}
inline const char* extensions(void* d,int screen){assert(d==reinterpret_cast<void*>(1) && screen==0);
    if(mode==7)return "GLX_MESA_gl_interop_suffix GLX_MESA_query_renderer";
    if(mode==8)return "GLX_MESA_gl_interop GLX_MESA_query_renderer_suffix";
    if(mode==33)return long_extensions;
    return "GLX_MESA_gl_interop GLX_MESA_query_renderer GLX_other";
}
inline int is_direct(void* d,void* c){assert(d==reinterpret_cast<void*>(1) && c==reinterpret_cast<void*>(2));return mode==6?0:1;}
inline int device_info(void* d,void* c,MesaGlInteropDeviceV1* out){
    assert(d==reinterpret_cast<void*>(1) && c==reinterpret_cast<void*>(2) && out && out->version==1);
    ++device_reads;*out={1,0,3,0,0,0x1002,0x731f};
    if(mode==9 || (mode==32 && device_reads>1))return 6;
    if(mode==10)out->version=0;if(mode==11)out->version=2;
    if(mode==12)out->pci_bus=256;if(mode==13)out->pci_device=32;if(mode==14)out->pci_function=8;
    if(mode==15)out->vendor_id=0xffff;if(mode==16)out->vendor_id=0;if(mode==17)out->device_id=0xffff;if(mode==18)out->device_id=0;
    if(mode==31 && device_reads>1)out->pci_segment_group=1;
    return 0;
}
inline int renderer_integer(int attribute,unsigned* out){
    switch(attribute){
    case 0x8186:*out=mode==19?0:1;return mode==20?0:1;
    case 0x8183:*out=mode==21?0x10de:0x1002;return 1;
    case 0x8184:*out=mode==22?1:0x731f;return 1;
    case 0x818a:
        if(mode==40)return 0;
        if(mode==38){out[0]=4;return 1;}
        out[0]=mode==39 || mode==42?0:4;out[1]=mode==39?0:mode==42?2:6;return 1;
    case 0x818b:if(mode==41)return 0;out[0]=4;out[1]=5;return 1;
    default:assert(false);return 0;
    }
}
inline const unsigned char* gl_string(std::uint32_t name){assert(name==0x1f02);++gl_reads;
    return reinterpret_cast<const unsigned char*>(mode==26?"OpenGL ES 3.2 fixture":mode==27?"4.x fixture":mode==28?long_version:"4.2 (Core Profile) GLX fixture");
}
inline Metric<std::string> pci(const MesaGlInteropDeviceV1& d){assert(d.version==1 && d.vendor_id==0x1002 && d.device_id==0x731f);++identities;
    if(mode==23)return {{},Support::unsupported,"fixture no sysfs"};
    if(mode==24)return {{},Support::permission_required,"fixture denied"};
    return {std::string(mode==36 || (mode==25 && identities>1)?"1:3:0.0":"0:3:0.0"),mode==37?Support::estimated:Support::available,"fixture sysfs identity"};
}
inline CurrentGlxApi api(){return {display,context,query_context,extensions,is_direct,query_version,device_info,renderer_integer,gl_string,pci,{}};}
}
