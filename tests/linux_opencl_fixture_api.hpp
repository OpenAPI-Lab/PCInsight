#pragma once
#include "../src/linux_gpu_opencl.hpp"
#include <cassert>
#include <cstring>
#include <algorithm>
namespace opencl_fixture {
using namespace pcinsight::detail;
inline int mode=0,platform_calls=0,device_calls=0,pci_calls=0;
inline void reset(int scenario){mode=scenario;platform_calls=device_calls=pci_calls=0;}
inline ClPlatform platform(unsigned i){return reinterpret_cast<void*>(static_cast<std::uintptr_t>(10+i));}
inline ClDevice device(unsigned i){return reinterpret_cast<void*>(static_cast<std::uintptr_t>(100+i));}
inline ClInt platforms(ClUint capacity,ClPlatform* out,ClUint* count){
    ++platform_calls;if(mode==1)return -1001;if(mode==2)return -5;
    *count=mode==3?129:2;
    if(out){assert(capacity==2);out[0]=platform(0);out[1]=platform(1);
        if(mode==4)out[0]=nullptr;if(mode==5)out[1]=out[0];if(mode==6)*count=3;}
    return 0;
}
inline ClInt devices(ClPlatform p,ClType type,ClUint capacity,ClDevice* out,ClUint* count){
    ++device_calls;assert(type==cl_gpu);if(mode==7)return -1;if(mode==8)return -5;
    *count=mode==9?257:1;unsigned index=p==platform(0)?0:1;
    if(out){assert(capacity==1);out[0]=device(index);if(mode==10)out[0]=nullptr;if(mode==11)out[0]=device(0);if(mode==12)*count=2;}
    return 0;
}
template<class T> inline ClInt scalar(const T& value,std::size_t size,void* out,std::size_t* returned){
    *returned=sizeof(value);if(out){if(size<sizeof(value))return -30;std::memcpy(out,&value,sizeof(value));if(mode==13)--*returned;}return 0;
}
inline ClInt string(const char* value,std::size_t size,void* out,std::size_t* returned){
    *returned=std::strlen(value)+1;if(!out){if(mode==14)*returned=65537;return 0;}
    if(size<*returned)return -30;std::memcpy(out,value,*returned);
    if(mode==15)static_cast<char*>(out)[*returned-1]='x';if(mode==16)++*returned;
    if(mode==17)static_cast<char*>(out)[0]='\n';return 0;
}
inline ClInt info(ClDevice d,ClUint key,std::size_t size,void* out,std::size_t* returned){
    const unsigned index=d==device(0)?0:1;
    if(key==cl_platform){auto p=platform(index);if(mode==18)p=nullptr;return scalar(p,size,out,returned);}
    if(key==cl_type){ClType type=mode==19?2:cl_gpu;return scalar(type,size,out,returned);}
    if(key==cl_extensions){if(mode==20)return -5;return string(mode==21?"cl_khr_pci_bus_info_suffix":mode==22?"prefix_cl_khr_pci_bus_info":"cl_some_extension cl_khr_pci_bus_info cl_other",size,out,returned);}
    if(key==cl_pci){++pci_calls;ClPci pci{mode==23?0:index,3,0,0};
        if(mode==24)pci.bus=256;if(mode==25)pci.device=32;if(mode==26)pci.function=8;if(mode==27)return -5;return scalar(pci,size,out,returned);}
    if(key==cl_version){if(mode==28 && index==0)return -5;
        return string(mode==29?"OpenCL C 3.0 fixture":mode==30?"OpenCL 0.0 fixture":mode==31?"OpenCL 3.x fixture":index==0?"OpenCL 1.2 fixture-A":"OpenCL 3.0 fixture-B",size,out,returned);}
    assert(false);return -30;
}
}
