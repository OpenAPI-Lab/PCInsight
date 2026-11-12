#include "../src/windows_amd_runtime_memory.hpp"
#include "linux_opencl_fixture_api.hpp"
#include "pcinsight/monitor.hpp"
#include <array>
#include <iostream>
#include <limits>
using namespace pcinsight;
using namespace pcinsight::detail;
namespace f=opencl_fixture;
namespace {
int mode,factory;unsigned creates,closed,adapters_opened,adapters_closed,reads;unsigned driver_reads[2],vendor_reads[2];
struct Adapter {unsigned index;} adapters[2]{{0},{1}};
HRESULT create(void** out){++creates;*out=&factory;return mode==15?E_ACCESSDENIED:S_OK;}
HRESULT enumerate(void*,UINT i,void** out){*out=nullptr;if(i>=2)return DXGI_ERROR_NOT_FOUND;*out=&adapters[i];++adapters_opened;return S_OK;}
HRESULT describe(void* value,DXGI_ADAPTER_DESC1* out){*out={};out->AdapterLuid.LowPart=static_cast<Adapter*>(value)->index+1;if(mode==16 && creates>1)++out->AdapterLuid.LowPart;return S_OK;}
void close_factory(void*){++closed;}
void close_adapter(void*){++adapters_closed;}
GpuInventoryApi inventory{create,enumerate,describe,nullptr,close_factory,close_adapter};
ClInt PCINSIGHT_CL_CALL platforms(ClUint n,ClPlatform* out,ClUint* count){return f::platforms(n,out,count);}
ClInt PCINSIGHT_CL_CALL devices(ClPlatform p,ClType type,ClUint n,ClDevice* out,ClUint* count){return f::devices(p,type,n,out,count);}
ClInt PCINSIGHT_CL_CALL info(ClDevice device,ClUint key,std::size_t size,void* out,std::size_t* returned){
    auto index=device==f::device(0)?0u:1u;
    if(key==cl_extensions)return f::string(mode==1?"cl_khr_d3d11_sharing cl_amd_device_attribute_query_suffix":"cl_khr_d3d11_sharing cl_amd_device_attribute_query",size,out,returned);
    if(key==0x1001){++vendor_reads[index];ClUint vendor=mode==2 || (mode==3 && vendor_reads[index]>1)?0x8086U:0x1002U;return f::scalar(vendor,size,out,returned);}
    if(key==0x102d){++driver_reads[index];if(mode==4)return -5;return f::string(mode==5 && driver_reads[index]>2?"changed":"fixture-driver",size,out,returned);}
    if(key==0x4039){
        ++reads;assert(size==2*sizeof(std::size_t) && out && returned);
        std::array<std::size_t,2> values{100+index,50};
        if(mode==6)values={0,0};if(mode==7)values={100,101};if(mode==8)values[0]=std::numeric_limits<std::size_t>::max();
        if(mode==9)values={std::numeric_limits<std::uint64_t>::max()/1024+1,0};
        if(mode==10)values={9007199254740993ULL,1};
        *returned=mode==11?sizeof(std::size_t):sizeof(values);
        if(mode!=12)std::memcpy(out,values.data(),mode==13?sizeof(std::size_t):sizeof(values));
        return mode==14 || (mode==17 && index==0)?-5:0;
    }
    return f::info(device,key,size,out,returned);
}
WindowsOpenClApi api{platforms,devices,info};
ClInt PCINSIGHT_CL_CALL query(ClPlatform p,ClUint source,void* value,ClUint selection,ClUint capacity,ClDevice* out,ClUint* count){
    assert(source==0x401a && selection==0x401c);auto index=p==f::platform(0)?0u:1u;
    if(static_cast<Adapter*>(value)->index!=index)return -1;
    *count=1;if(out){assert(capacity==1);out[0]=f::device(mode==18 && creates>1?1-index:index);}return 0;
}
void* PCINSIGHT_CL_CALL resolve(ClPlatform,const char* name){assert(std::strcmp(name,"clGetDeviceIDsFromD3D11KHR")==0);auto fn=query;void* raw{};std::memcpy(&raw,&fn,sizeof(raw));return raw;}
std::vector<GpuInfo> targets(){std::vector<GpuInfo> out(2);for(unsigned i=0;i<2;++i){out[i].id="dxgi-luid:00000000:0000000"+std::to_string(i+1);out[i].hardware_vendor_id={0x1002U,Support::available,"DXGI"};out[i].memory_available_bytes={987,Support::available,"global untouched"};}return out;}
void reset(int scenario){mode=scenario;creates=closed=adapters_opened=adapters_closed=reads=0;std::fill_n(driver_reads,2,0);std::fill_n(vendor_reads,2,0);f::reset(0);}
WindowsAmdRuntimeMemorySnapshot run(int scenario){reset(scenario);auto g=targets();auto out=read_windows_amd_runtime_memory(g,api,resolve,inventory);assert(creates==closed && adapters_closed==adapters_opened);assert(g[0].memory_available_bytes.value==987 && g[1].memory_available_bytes.value==987);return out;}
}
int main(){
    auto s=run(0);assert(s.support==Support::estimated && s.gpus.size()==2);assert(s.gpus[0].reported_free_bytes.value==102400 && s.gpus[1].reported_free_bytes.value==103424 && s.gpus[0].reported_largest_block_bytes.value==51200);assert(s.gpus[0].reported_free_bytes.support==Support::estimated && s.gpus[0].opencl_driver_version.value=="fixture-driver");
    for(int scenario:{1,2,3,4,5,7,8,9,11,12,13,14,15,16,18}){s=run(scenario);assert(!s.gpus[0].reported_free_bytes.value && !s.gpus[0].reported_largest_block_bytes.value);if(scenario==1 || scenario==2 || scenario==4)assert(reads==0);}
    s=run(6);assert(s.gpus[0].reported_free_bytes.value==0 && s.gpus[0].reported_largest_block_bytes.value==0);
    s=run(10);assert(s.gpus[0].reported_free_bytes.value==9007199254740993ULL*1024);
    s=run(17);assert(!s.gpus[0].reported_free_bytes.value && s.gpus[1].reported_free_bytes.value==103424);
    reset(0);auto g=targets();g[1].hardware_vendor_id={0x10deU,Support::available,"NVIDIA"};s=read_windows_amd_runtime_memory(g,api,resolve,inventory);assert(s.gpus.size()==1 && s.gpus[0].reported_free_bytes.value==102400);
    reset(0);g=targets();for(auto& gpu:g)gpu.hardware_vendor_id.support=Support::unsupported;s=read_windows_amd_runtime_memory(g,api,resolve,inventory);assert(s.gpus.empty() && !creates && s.support==Support::available);
    reset(0);g=targets();g.push_back(g[0]);s=read_windows_amd_runtime_memory(g,api,resolve,inventory);assert(s.gpus.size()==3 && !s.gpus[0].reported_free_bytes.value);
    reset(0);g=targets();auto missing=api;missing.info=nullptr;s=read_windows_amd_runtime_memory(g,missing,resolve,inventory);assert(!s.gpus[0].reported_free_bytes.value && !reads);
    reset(0);s=read_windows_amd_runtime_memory(g,api,nullptr,inventory);assert(!s.gpus[0].reported_free_bytes.value && !reads);
    const auto native=query_experimental_windows_amd_runtime_memory();
    for(const auto& row:native.gpus){
        std::cout<<row.adapter_id<<" | EXPERIMENTAL runtime free="<<(row.reported_free_bytes.value?std::to_string(*row.reported_free_bytes.value):"unknown")<<" largest="<<(row.reported_largest_block_bytes.value?std::to_string(*row.reported_largest_block_bytes.value):"unknown")<<" driver="<<row.opencl_driver_version.value.value_or("unknown")<<'\n';
        if(row.reported_free_bytes.value){assert(row.reported_free_bytes.support==Support::estimated && row.reported_largest_block_bytes.value && *row.reported_largest_block_bytes.value<=*row.reported_free_bytes.value);}
    }
    Monitor monitor;SampleOptions options;options.include_gpus=true;
    for(const auto& gpu:monitor.sample(options).gpus)if(gpu.hardware_vendor_id.value==0x1002U)assert(!gpu.memory_available_bytes.value); // explicit query never changes Monitor's contract
    std::cout<<"Experimental AMD runtime memory tests passed\n";
}
