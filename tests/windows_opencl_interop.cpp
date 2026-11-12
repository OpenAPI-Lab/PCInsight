#include "../src/windows_opencl_interop.hpp"
#include "linux_opencl_fixture_api.hpp"
#include <iostream>
using namespace pcinsight;
using namespace pcinsight::detail;
namespace f=opencl_fixture;
namespace {
int mode,factory;unsigned factory_closed,adapter_closed,queries;
struct Adapter {unsigned index,calls;} adapters[2];
HRESULT create(void** out){*out=mode==16?nullptr:&factory;return mode==15?E_ACCESSDENIED:S_OK;}
HRESULT enumerate(void*,UINT index,void** out){*out=nullptr;if(index>=2)return DXGI_ERROR_NOT_FOUND;*out=&adapters[index];if(mode==18)*out=nullptr;if(mode==10 && index==1)return E_FAIL;return S_OK;}
HRESULT describe(void* raw,DXGI_ADAPTER_DESC1* out){auto& a=*static_cast<Adapter*>(raw);*out={};out->AdapterLuid.LowPart=a.index+1;if(mode==17)out->AdapterLuid.LowPart=1;if(mode==11 && a.calls++)out->AdapterLuid.LowPart=3;if(mode==12 && a.index==1)out->Flags=DXGI_ADAPTER_FLAG_SOFTWARE;return S_OK;}
void close_factory(void* raw){assert(raw==&factory);++factory_closed;}
void close_adapter(void* raw){assert(raw==&adapters[0] || raw==&adapters[1]);++adapter_closed;}
GpuInventoryApi inventory{create,enumerate,describe,nullptr,close_factory,close_adapter};
ClInt PCINSIGHT_CL_CALL platforms(ClUint n,ClPlatform* out,ClUint* count){return f::platforms(n,out,count);}
ClInt PCINSIGHT_CL_CALL devices(ClPlatform p,ClType type,ClUint n,ClDevice* out,ClUint* count){return f::devices(p,type,n,out,count);}
ClInt PCINSIGHT_CL_CALL info(ClDevice device,ClUint key,std::size_t size,void* out,std::size_t* returned){
    if(key==cl_extensions){if(mode==19)return -5;return f::string(mode==8?"cl_khr_d3d11_sharing_suffix":"cl_khr_d3d11_sharing",size,out,returned);}
    if(key==cl_version && device==f::device(0)){if(mode==13)return -5;if(mode==21)return f::string("not a version",size,out,returned);}
    return f::info(device,key,size,out,returned);
}
WindowsOpenClApi api{platforms,devices,info};
ClInt PCINSIGHT_CL_CALL query(ClPlatform p,ClUint source,void* object,ClUint selection,ClUint capacity,ClDevice* out,ClUint* count){
    ++queries;assert(source==0x401a && selection==0x401c);
    auto* adapter=static_cast<Adapter*>(object);assert(adapter==&adapters[0] || adapter==&adapters[1]);
    const unsigned index=p==f::platform(0)?0:1;
    bool match=adapter->index==index;
    if(mode==2 && index==0)match=true;
    if(mode==3)match=adapter->index==0;
    if(!match)return -1;
    *count=mode==6?257:mode==5?2:1;
    if(out){assert(capacity==*count);std::fill(out,out+capacity,f::device(index));if(mode==4)out[0]=f::device(99);if(mode==7)++*count;}
    return 0;
}
void* PCINSIGHT_CL_CALL resolve(ClPlatform p,const char* name){assert(p && std::strcmp(name,"clGetDeviceIDsFromD3D11KHR")==0);if(mode==9)return nullptr;auto function=query;void* out{};static_assert(sizeof(out)==sizeof(function));std::memcpy(&out,&function,sizeof(out));return out;}
std::vector<GpuInfo> targets(){std::vector<GpuInfo> g(2);g[0].id="dxgi-luid:00000000:00000001";g[1].id="dxgi-luid:00000000:00000002";return g;}
std::vector<GpuInfo> run(int scenario){mode=scenario;f::reset(0);factory_closed=adapter_closed=queries=0;adapters[0]={0,0};adapters[1]={1,0};auto g=targets();fill_windows_opencl_interop(g,api,resolve,inventory);return g;}
}
int main(){
    auto g=run(0);assert(g[0].opencl_version.value=="OpenCL 1.2 fixture-A" && g[1].opencl_version.value=="OpenCL 3.0 fixture-B");assert(g[0].opencl_version.support==Support::estimated && factory_closed==1 && adapter_closed==2);
    for(int m:{2,3,4,5,6,7,8,9,10,11,12,15,16,17,18,19}){g=run(m);assert(!g[0].opencl_version.value && !g[1].opencl_version.value);if(m==10)assert(factory_closed==1 && adapter_closed==2);if(m==15)assert(factory_closed==1);if(m==16)assert(factory_closed==0);}
    for(int m:{13,21}){g=run(m);assert(!g[0].opencl_version.value && g[1].opencl_version.value);}
    run(0);g=targets();g[0].opencl_version={std::string("proven LUID version"),Support::available,"exact identity"};fill_windows_opencl_interop(g,api,resolve,inventory);assert(g[0].opencl_version.value=="proven LUID version" && g[0].opencl_version.support==Support::available && g[1].opencl_version.value);
    g=targets();g.push_back(g[0]);fill_windows_opencl_interop(g,api,resolve,inventory);assert(!g[0].opencl_version.value);
    g=targets();g[1].id="dxgi-luid:00000000:00000003";fill_windows_opencl_interop(g,api,resolve,inventory);assert(!g[0].opencl_version.value);
    g=targets();fill_windows_opencl_interop(g,api,nullptr,inventory);assert(!g[0].opencl_version.value);
    g=targets();for(auto& gpu:g)gpu.opencl_version={std::string("exact"),Support::available,"existing"};queries=0;fill_windows_opencl_interop(g,api,resolve,inventory);assert(queries==0);
    auto native=read_windows_gpus();fill_windows_gpu_opencl(native);
    for(const auto& gpu:native)std::cout<<gpu.model<<" | OpenCL="<<gpu.opencl_version.value.value_or("unavailable")<<" | support="<<static_cast<int>(gpu.opencl_version.support)<<" | "<<gpu.opencl_version.note<<'\n';
    std::cout<<"D3D11/OpenCL association tests passed\n";
}
