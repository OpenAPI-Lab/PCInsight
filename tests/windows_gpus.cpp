#include "../src/windows_gpus.hpp"
#include "pcinsight/monitor.hpp"
#include <cassert>
#include <cwchar>
#include <iostream>
#include <stdexcept>
#include <system_error>
using namespace pcinsight;
using namespace pcinsight::detail;
namespace {
struct Adapter {DXGI_ADAPTER_DESC1 desc{};HRESULT describe_status{S_OK},driver_status{S_OK};LARGE_INTEGER version{};} adapters[3];
int factory;unsigned factory_releases,adapter_releases,enumerations,driver_calls,count;
HRESULT create_status,enum_status;bool null_factory,null_adapter,failure_pointer,end_pointer,limit;
HRESULT create(void** out){*out=null_factory?nullptr:&factory;return create_status;}
HRESULT enumerate(void* handle,UINT index,void** out){
    assert(handle==&factory);++enumerations;
    if(enum_status!=S_OK){*out=failure_pointer?adapters:nullptr;return enum_status;}
    if(!limit && index>=count){*out=end_pointer?adapters:nullptr;return DXGI_ERROR_NOT_FOUND;}
    auto& adapter=adapters[index%3];if(limit)adapter.desc.AdapterLuid.LowPart=index;
    *out=null_adapter?nullptr:&adapter;return S_OK;
}
HRESULT describe(void* handle,DXGI_ADAPTER_DESC1* out){auto& adapter=*static_cast<Adapter*>(handle);*out=adapter.desc;return adapter.describe_status;}
HRESULT version(void* handle,LARGE_INTEGER* out){++driver_calls;auto& adapter=*static_cast<Adapter*>(handle);*out=adapter.version;return adapter.driver_status;}
void release_factory(void* handle){assert(handle==&factory);++factory_releases;}
void release_adapter(void* handle){assert(handle==&adapters[0] || handle==&adapters[1] || handle==&adapters[2]);++adapter_releases;}
Metric<std::string> capability(void* handle){
    assert(handle==&adapters[0] || handle==&adapters[1]); // never software/default
    if(handle==&adapters[0])return {std::string("12_2"),Support::available,"fixture"};
    return {{},Support::permission_required,"fixture denial"};
}
void process_budget(void* handle,GpuInfo& gpu){
    assert(handle==&adapters[0] || handle==&adapters[1]);
    if(handle==&adapters[0]){assert(gpu.id=="dxgi-luid:00000000:00000001");gpu.local_process_memory.budget_bytes={111,Support::available,"fixture"};}
    else {assert(gpu.id=="dxgi-luid:00000000:00000002");gpu.local_process_memory.budget_bytes={222,Support::available,"fixture"};}
}
GpuInventoryApi api{create,enumerate,describe,version,release_factory,release_adapter};
void reset(){
    factory_releases=adapter_releases=enumerations=driver_calls=0;count=3;create_status=enum_status=S_OK;
    null_factory=null_adapter=failure_pointer=end_pointer=limit=false;
    for(unsigned i=0;i<3;++i){adapters[i]={};auto& d=adapters[i].desc;d.AdapterLuid.LowPart=i+1;
        d.VendorId=i==0?0x1002:i==1?0x8086:0x1414;d.DeviceId=100+i;
        d.DedicatedVideoMemory=i==0?std::size_t{8}*1024*1024*1024:0;
        d.DedicatedSystemMemory=1024;d.SharedSystemMemory=std::size_t{16}*1024*1024*1024;
        std::wcscpy(d.Description,i==0?L"Discrete \u00e9 GPU":i==1?L"Integrated GPU":L"Software GPU");
        if(i==2)d.Flags=DXGI_ADAPTER_FLAG_SOFTWARE;
        adapters[i].version.QuadPart=static_cast<LONGLONG>((std::uint64_t{31}<<48)|(std::uint64_t{2}<<32)|(std::uint64_t{3}<<16)|4);
    }
}
void fails(HRESULT expected){
    try{(void)read_windows_gpus(api);assert(false);}
    catch(const std::system_error& error){assert(error.code().value()==static_cast<int>(expected));}
}
}
int main(){
    reset();auto result=read_windows_gpus(api);
    assert(!result[0].direct3d12_feature_level.value); // optional provider absent
    assert(result.size()==2 && factory_releases==1 && adapter_releases==3 && enumerations==4 && driver_calls==2);
    assert(result[0].id=="dxgi-luid:00000000:00000001" && result[0].vendor=="AMD" && result[0].model=="Discrete \xc3\xa9 GPU");
    assert(result[0].dedicated_memory_bytes.value==std::uint64_t{8}*1024*1024*1024);
    assert(result[0].shared_memory_bytes.value==std::uint64_t{16}*1024*1024*1024);
    assert(result[0].dedicated_system_memory_bytes.value==1024 && result[0].hardware_vendor_id.value==0x1002 && result[0].hardware_device_id.value==100);
    assert(result[0].driver_version.value=="31.2.3.4" && result[0].driver_version.support==Support::available);
    assert(result[1].dedicated_memory_bytes.value==0 && result[1].dedicated_memory_bytes.support==Support::available); // UMA zero is a reported value
    for(const auto& gpu:result){assert(gpu.engines.empty() && !gpu.utilization_percent.value && !gpu.memory_used_bytes.value && !gpu.memory_available_bytes.value);
        assert(!gpu.temperature_celsius.value && !gpu.physical_location.value && !gpu.driver_date.value && !gpu.vulkan_version.value);}
    reset();auto capabilities=api;capabilities.d3d12_level=capability;result=read_windows_gpus(capabilities);
    assert(result.size()==2 && result[0].direct3d12_feature_level.value=="12_2");
    assert(result[1].direct3d12_feature_level.support==Support::permission_required && !result[1].direct3d12_feature_level.value);
    assert(factory_releases==1 && adapter_releases==3);
    reset();auto budgets=api;budgets.process_memory=process_budget;result=read_windows_gpus(budgets);
    assert(result[0].local_process_memory.budget_bytes.value==111 && result[1].local_process_memory.budget_bytes.value==222);
    reset();adapters[0].driver_status=E_ACCESSDENIED;adapters[1].driver_status=DXGI_ERROR_UNSUPPORTED;result=read_windows_gpus(api);
    assert(!result[0].driver_version.value && result[0].driver_version.support==Support::permission_required);
    assert(!result[1].driver_version.value && result[1].driver_version.support==Support::unsupported);
    assert(factory_releases==1 && adapter_releases==3);
    reset();adapters[0].version.QuadPart=0;result=read_windows_gpus(api);assert(result[0].driver_version.value=="0.0.0.0");
    reset();adapters[0].desc.AdapterLuid.HighPart=-1;adapters[0].desc.VendorId=0x1234;result=read_windows_gpus(api);
    assert(result[0].id=="dxgi-luid:ffffffff:00000001" && result[0].vendor=="PCI vendor 0x1234");
    reset();adapters[0].desc.VendorId=0x12345678;result=read_windows_gpus(api);assert(result[0].vendor=="ACPI vendor 0x12345678");
    reset();count=0;assert(read_windows_gpus(api).empty() && factory_releases==1 && adapter_releases==0);
    reset();count=1;adapters[0].desc.Flags=DXGI_ADAPTER_FLAG_SOFTWARE;assert(read_windows_gpus(api).empty() && driver_calls==0 && adapter_releases==1);
    reset();create_status=E_ACCESSDENIED;fails(E_ACCESSDENIED);assert(factory_releases==1 && adapter_releases==0);
    reset();null_factory=true;fails(E_UNEXPECTED);assert(factory_releases==0);
    reset();enum_status=DXGI_ERROR_DEVICE_REMOVED;failure_pointer=true;fails(enum_status);assert(factory_releases==1 && adapter_releases==1);
    reset();null_adapter=true;fails(E_UNEXPECTED);assert(factory_releases==1 && adapter_releases==0);
    reset();adapters[1].describe_status=E_FAIL;fails(E_FAIL);assert(factory_releases==1 && adapter_releases==2);
    reset();adapters[1].desc.AdapterLuid=adapters[0].desc.AdapterLuid;fails(E_UNEXPECTED);assert(factory_releases==1 && adapter_releases==2);
    reset();std::wmemset(adapters[0].desc.Description,L'a',128);fails(E_UNEXPECTED);assert(factory_releases==1 && adapter_releases==1);
    reset();adapters[0].desc.Description[0]=0xd800;adapters[0].desc.Description[1]=0;fails(E_UNEXPECTED);assert(factory_releases==1 && adapter_releases==1);
    reset();count=0;end_pointer=true;assert(read_windows_gpus(api).empty() && adapter_releases==1 && factory_releases==1);
    reset();limit=true;for(auto& adapter:adapters)adapter.desc.Flags=0;
    fails(HRESULT_FROM_WIN32(ERROR_MORE_DATA));assert(factory_releases==1 && adapter_releases==257);
    reset();auto incomplete=api;incomplete.describe=nullptr;
    try{(void)read_windows_gpus(incomplete);assert(false);}catch(const std::invalid_argument&){}assert(factory_releases==0);
    // Real local DXGI enumeration: no graphics device, benchmark or settings mutation.
    const auto native=read_windows_gpus();
    std::cout<<"Native non-software DXGI adapters: "<<native.size()<<'\n';
    for(const auto& gpu:native){
        assert(gpu.id.rfind("dxgi-luid:",0)==0 && gpu.dedicated_memory_bytes.value && gpu.shared_memory_bytes.value);
        assert(gpu.hardware_vendor_id.value && gpu.hardware_device_id.value && gpu.dedicated_system_memory_bytes.value);
        assert(!gpu.memory_used_bytes.value && !gpu.memory_available_bytes.value && !gpu.utilization_percent.value);
        std::cout<<gpu.model<<" | "<<gpu.vendor<<" | dedicated="<<*gpu.dedicated_memory_bytes.value
            <<" shared-capacity="<<*gpu.shared_memory_bytes.value<<" driver="<<gpu.driver_version.value.value_or("unknown")<<'\n';
    }
    Monitor monitor;SampleOptions options;options.include_disks=options.include_networks=options.include_processes=options.include_gpus=false;
    assert(monitor.sample(options).gpus.empty());options.include_gpus=true;const auto snapshot=monitor.sample(options);
    for(const auto& gpu:snapshot.gpus)assert(gpu.dedicated_memory_bytes.value && gpu.shared_memory_bytes.value && !gpu.memory_used_bytes.value);
    options.include_gpus=false;assert(monitor.sample(options).gpus.empty());
}
