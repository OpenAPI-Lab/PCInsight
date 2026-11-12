#include "../src/windows_gpu_opencl.hpp"
#include "../src/windows_gpus.hpp"
#include "linux_opencl_fixture_api.hpp"
#include "pcinsight/monitor.hpp"
#include <iostream>
using namespace pcinsight;
using namespace pcinsight::detail;
namespace f=opencl_fixture;
namespace {
unsigned luid_reads;
ClInt PCINSIGHT_CL_CALL platforms(ClUint n,ClPlatform* out,ClUint* count){return f::platforms(n,out,count);}
ClInt PCINSIGHT_CL_CALL devices(ClPlatform platform,ClType type,ClUint n,ClDevice* out,ClUint* count){return f::devices(platform,type,n,out,count);}
ClInt PCINSIGHT_CL_CALL info(ClDevice device,ClUint key,std::size_t size,void* out,std::size_t* returned){
    if(key==cl_extensions){
        if(f::mode==20)return -5;
        return f::string(f::mode==21?"cl_khr_device_uuid_suffix":f::mode==22?"prefix_cl_khr_device_uuid":"cl_some cl_khr_device_uuid cl_other",size,out,returned);
    }
    if(key==cl_luid_valid){ClUint value=f::mode==34?0:f::mode==35?2:1;return f::scalar(value,size,out,returned);}
    if(key==cl_luid){
        ++luid_reads;struct Luid {ClUint low,high;} luid{device==f::device(0)?1u:2u,device==f::device(0)?0u:0xffffffffu};
        if(f::mode==23)luid={1,0};if(f::mode==36)return -5;
        const auto result=f::scalar(luid,size,out,returned);if(f::mode==37)*returned=7;return result;
    }
    if(key==cl_node_mask){ClUint value=f::mode==32?0:f::mode==33?3:1;return f::scalar(value,size,out,returned);}
    assert(key!=cl_pci);return f::info(device,key,size,out,returned);
}
WindowsOpenClApi api{platforms,devices,info};
std::vector<GpuInfo> targets(){std::vector<GpuInfo> g(3);g[0].id="dxgi-luid:00000000:00000001";g[1].id="dxgi-luid:ffffffff:00000002";g[2].id="dxgi-luid:00000000:00000003";for(auto& gpu:g)gpu.opencl_version={std::string("stale"),Support::available,"old"};return g;}
std::vector<GpuInfo> run(int mode){f::reset(mode);luid_reads=0;auto g=targets();fill_windows_gpu_opencl(g,api);assert(!g[2].opencl_version.value);return g;}
}
int main(){
    auto g=run(0);assert(g[0].opencl_version.value=="OpenCL 1.2 fixture-A" && g[1].opencl_version.value=="OpenCL 3.0 fixture-B");
    for(int m:{1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,28,29,30,31,32,33,34,35,36,37}){
        g=run(m);assert(!g[0].opencl_version.value);if(m==34 || m==21 || m==22 || m==19)assert(luid_reads==0);if(m==28)assert(g[1].opencl_version.value);
    }
    f::reset(0);g=targets();g.push_back(g[0]);fill_windows_gpu_opencl(g,api);assert(!g[0].opencl_version.value && !g[3].opencl_version.value && g[1].opencl_version.value);
    g=targets();auto missing=api;missing.info=nullptr;f::reset(0);fill_windows_gpu_opencl(g,missing);assert(!g[0].opencl_version.value && f::platform_calls==0);
    auto native=read_windows_gpus();fill_windows_gpu_opencl(native);
    for(const auto& gpu:native)std::cout<<gpu.model<<" | "<<gpu.id<<" | OpenCL="<<gpu.opencl_version.value.value_or("unavailable")<<" | "<<gpu.opencl_version.note<<'\n';
    // Read-only native diagnostics distinguish absent UUID support from a false
    // LUID or alternate ICDs. Never use these strings to guess an adapter match.
    auto module=LoadLibraryExW(L"OpenCL.dll",nullptr,LOAD_LIBRARY_SEARCH_SYSTEM32);
    if(module){
        WindowsOpenClApi real;
        auto symbol=[&](auto& fn,const char* name){auto proc=GetProcAddress(module,name);std::memcpy(&fn,&proc,sizeof(proc));};
        symbol(real.platforms,"clGetPlatformIDs");symbol(real.devices,"clGetDeviceIDs");symbol(real.info,"clGetDeviceInfo");
        ClUint count{};
        if(real.platforms && real.devices && real.info && real.platforms(0,nullptr,&count)==0 && count<=128){
            std::vector<ClPlatform> ps(count);if(count && real.platforms(count,ps.data(),&count)==0)for(auto p:ps){
                ClUint n{};if(real.devices(p,cl_gpu,0,nullptr,&n)!=0 || !n || n>256)continue;
                std::vector<ClDevice> ds(n);if(real.devices(p,cl_gpu,n,ds.data(),&n)!=0)continue;
                for(auto d:ds){std::size_t size{};if(real.info(d,cl_extensions,0,nullptr,&size)!=0 || !size || size>65536)continue;
                    std::string ext(size,'\0');if(real.info(d,cl_extensions,size,ext.data(),nullptr)!=0)continue;
                    const auto end=ext.find('\0');if(end==std::string::npos)continue;ext.resize(end);
                    const bool uuid=(" "+ext+" ").find(" cl_khr_device_uuid ")!=std::string::npos;
                    ClUint valid{};auto status=uuid?real.info(d,cl_luid_valid,sizeof(valid),&valid,nullptr):-30;
                    std::cout<<"Native OpenCL GPU: uuid-extension="<<uuid<<" luid-query-status="<<status<<" luid-valid="<<valid<<'\n';
                }
            }
        }
        FreeLibrary(module);
    }
    Monitor monitor;SampleOptions options;options.include_gpus=false;assert(monitor.sample(options).gpus.empty());options.include_gpus=true;
    for(const auto& gpu:monitor.sample(options).gpus)if(gpu.opencl_version.value){
        if(gpu.opencl_version.support==Support::available)assert(gpu.opencl_version.note.find("Windows DXGI LUID")!=std::string::npos);
        else {assert(gpu.opencl_version.support==Support::estimated);assert(gpu.opencl_version.note.find("D3D11-interoperable")!=std::string::npos);}
    }
    options.include_gpus=false;assert(monitor.sample(options).gpus.empty());
    std::cout<<"Windows OpenCL per-device tests passed\n";
}
