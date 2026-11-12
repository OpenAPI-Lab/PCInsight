#include "../src/windows_gpu_memory.hpp"
#include "../src/windows_gpus.hpp"
#include "pcinsight/monitor.hpp"
#include <pdhmsg.h>
#include <cassert>
#include <cstring>
#include <iostream>
#include <limits>
#include <thread>
using namespace pcinsight;
using namespace pcinsight::detail;
using namespace std::chrono_literals;
namespace {
struct Row {std::wstring name;LONGLONG bytes;DWORD status{PDH_CSTATUS_VALID_DATA};};
struct Query {unsigned kind{};bool alive{};} queries[128];
unsigned opened,closed,formatted;
PDH_STATUS open_error,add_error[2],collect_error[2],format_error[2];
bool null_query,null_counter,bad_pointer,bad_count,bad_size,no_terminator;
std::vector<Row> rows[2];
PDH_STATUS WINAPI open(LPCWSTR source,DWORD_PTR user,PDH_HQUERY* out){
    assert(!source && !user && opened<128);auto& q=queries[opened++];q={0,true};
    *out=null_query?nullptr:reinterpret_cast<PDH_HQUERY>(&q);if(null_query)q.alive=false;return open_error;
}
PDH_STATUS WINAPI add(PDH_HQUERY handle,LPCWSTR path,DWORD_PTR user,PDH_HCOUNTER* out){
    assert(!user);auto& q=*reinterpret_cast<Query*>(handle);assert(q.alive);
    const std::wstring value=path;
    assert(value==L"\\GPU Adapter Memory(*)\\Dedicated Usage" || value==L"\\GPU Adapter Memory(*)\\Shared Usage");
    q.kind=value.ends_with(L"Shared Usage")?1:0;*out=null_counter?nullptr:reinterpret_cast<PDH_HCOUNTER>(&q);return add_error[q.kind];
}
PDH_STATUS WINAPI collect(PDH_HQUERY handle){auto& q=*reinterpret_cast<Query*>(handle);assert(q.alive);return collect_error[q.kind];}
PDH_STATUS WINAPI close(PDH_HQUERY handle){auto& q=*reinterpret_cast<Query*>(handle);assert(q.alive);q.alive=false;++closed;return ERROR_SUCCESS;}
PDH_STATUS WINAPI format(PDH_HCOUNTER handle,DWORD flags,LPDWORD size,LPDWORD count,PPDH_FMT_COUNTERVALUE_ITEM_W output){
    assert(flags==(PDH_FMT_LARGE|PDH_FMT_NOSCALE|PDH_FMT_NOCAP100));auto& q=*reinterpret_cast<Query*>(handle);assert(q.alive);++formatted;
    if(format_error[q.kind])return format_error[q.kind];
    const auto& input=rows[q.kind];DWORD required=static_cast<DWORD>(input.size()*sizeof(PDH_FMT_COUNTERVALUE_ITEM_W));
    for(const auto& row:input)required+=static_cast<DWORD>((row.name.size()+1)*sizeof(wchar_t));
    if(!output){*size=bad_size?5*1024*1024:required;*count=static_cast<DWORD>(input.size());return PDH_MORE_DATA;}
    assert(*size>=required);*size=required;*count=bad_count?16385:static_cast<DWORD>(input.size());
    auto* cursor=reinterpret_cast<unsigned char*>(output)+input.size()*sizeof(PDH_FMT_COUNTERVALUE_ITEM_W);
    for(std::size_t i=0;i<input.size();++i){PDH_FMT_COUNTERVALUE_ITEM_W item{};item.szName=reinterpret_cast<wchar_t*>(cursor);
        std::memcpy(cursor,input[i].name.c_str(),(input[i].name.size()+1)*sizeof(wchar_t));
        if(no_terminator)for(std::size_t j=0;j<input[i].name.size()+1;++j)item.szName[j]=L'x';
        if(bad_pointer)item.szName=reinterpret_cast<wchar_t*>(output);
        item.FmtValue.CStatus=input[i].status;item.FmtValue.largeValue=input[i].bytes;
        std::memcpy(reinterpret_cast<unsigned char*>(output)+i*sizeof item,&item,sizeof item);
        cursor+=(input[i].name.size()+1)*sizeof(wchar_t);
    }
    return ERROR_SUCCESS;
}
GpuMemoryPdhApi api{open,add,collect,format,close};
std::vector<GpuInfo> gpus(){GpuInfo gpu;gpu.id="dxgi-luid:abcdef01:00000002";gpu.dedicated_memory_bytes={1,Support::available,"capacity intentionally incomparable"};return {gpu};}
void reset(){
    for(const auto& q:queries)assert(!q.alive);
    opened=closed=formatted=0;open_error=0;for(unsigned i=0;i<2;++i)add_error[i]=collect_error[i]=format_error[i]=0;
    null_query=null_counter=bad_pointer=bad_count=bad_size=no_terminator=false;
    rows[0]={{L"luid_0xABCDEF01_0x00000002_phys_0",1234567890123LL},{L"_Total",999}};
    rows[1]={{L"luid_0xabcdef01_0x00000002_phys_0",0,PDH_CSTATUS_NEW_DATA}};
}
std::vector<GpuInfo> read(){auto gpu=gpus();{WindowsGpuMemory memory(api);memory.fill(gpu);assert(!gpu[0].memory_used_bytes.value);memory.fill(gpu);}assert(opened==closed || null_query);return gpu;}
}
int main(){
    reset();auto result=read();assert(result[0].dedicated_memory_used_bytes.value==1234567890123ULL && result[0].shared_memory_used_bytes.value==0);
    assert(result[0].memory_used_bytes.value==result[0].dedicated_memory_used_bytes.value && !result[0].memory_available_bytes.value);
    assert(result[0].dedicated_memory_bytes.value==1 && result[0].dedicated_memory_used_bytes.support==Support::available);
    reset();rows[0][0].bytes=std::numeric_limits<LONGLONG>::max();result=read();assert(result[0].memory_used_bytes.value==static_cast<std::uint64_t>(std::numeric_limits<LONGLONG>::max()));
    reset();add_error[1]=PDH_CSTATUS_NO_COUNTER;result=read();assert(result[0].memory_used_bytes.value && !result[0].shared_memory_used_bytes.value);
    reset();open_error=ERROR_ACCESS_DENIED;result=read();assert(!result[0].memory_used_bytes.value && result[0].memory_used_bytes.support==Support::permission_required);
    reset();null_query=true;result=read();assert(!result[0].memory_used_bytes.value);
    reset();null_counter=true;result=read();assert(!result[0].memory_used_bytes.value);
    reset();rows[0][0].status=PDH_ACCESS_DENIED;result=read();assert(!result[0].memory_used_bytes.value && result[0].memory_used_bytes.support==Support::permission_required && result[0].shared_memory_used_bytes.value==0);
    reset();rows[0][0].bytes=-1;result=read();assert(!result[0].memory_used_bytes.value);
    reset();rows[0].push_back({L"luid_0xabcdef01_0x00000002_phys_1",100});result=read();assert(!result[0].memory_used_bytes.value);
    reset();rows[0].push_back(rows[0][0]);result=read();assert(!result[0].memory_used_bytes.value);
    reset();rows[0][0].name=L"pid_10_luid_0xabcdef01_0x00000002_phys_0";result=read();assert(!result[0].memory_used_bytes.value); // never attribute per-process counters
    reset();rows[0][0].name=L"luid_0xabcdef01_0x00000002_phys_0#1";result=read();assert(!result[0].memory_used_bytes.value);
    reset();rows[0][0].name=L"luid_0xabcdef01_0x00000002_phys_4294967296";result=read();assert(!result[0].memory_used_bytes.value);
    reset();rows[0][0].name=L"luid_0x00000000_0x00000002_phys_0";result=read();assert(!result[0].memory_used_bytes.value);
    for(unsigned fault=0;fault<4;++fault){reset();bad_pointer=fault==0;bad_count=fault==1;bad_size=fault==2;no_terminator=fault==3;
        result=read();assert(!result[0].memory_used_bytes.value && !result[0].shared_memory_used_bytes.value);}
    reset();format_error[0]=PDH_MORE_DATA;result=read();assert(!result[0].memory_used_bytes.value && result[0].shared_memory_used_bytes.value==0);
    reset();{
        WindowsGpuMemory memory(api);auto gpu=gpus();memory.fill(gpu);memory.fill(gpu);assert(gpu[0].memory_used_bytes.value);
        rows[0][0].bytes=99;rows[1][0].bytes=33;memory.fill(gpu);
        assert(gpu[0].memory_used_bytes.value==99 && gpu[0].shared_memory_used_bytes.value==33);
        rows[1][0].bytes=0;
        collect_error[0]=ERROR_ACCESS_DENIED;memory.fill(gpu);assert(!gpu[0].memory_used_bytes.value && gpu[0].shared_memory_used_bytes.value==0);
        collect_error[0]=0;memory.fill(gpu);assert(!gpu[0].memory_used_bytes.value);memory.fill(gpu);assert(gpu[0].memory_used_bytes.value);
        gpu[0].id="dxgi-luid:00000000:00000003";memory.fill(gpu);assert(!gpu[0].memory_used_bytes.value); // topology resets both queries
        gpu.clear();memory.fill(gpu);assert(opened==closed);
    }
    reset();{WindowsGpuMemory memory(api);auto gpu=gpus();gpu.push_back(gpu.front());memory.fill(gpu);assert(opened==0 && !gpu[0].memory_used_bytes.value);}
    const auto native_gpus=read_windows_gpus();auto native=native_gpus;
    {WindowsGpuMemory memory;memory.fill(native);std::this_thread::sleep_for(100ms);memory.fill(native);
        for(const auto& gpu:native)std::cout<<gpu.model<<" dedicated-used="<<(gpu.dedicated_memory_used_bytes.value?std::to_string(*gpu.dedicated_memory_used_bytes.value):gpu.dedicated_memory_used_bytes.note)
            <<" shared-used="<<(gpu.shared_memory_used_bytes.value?std::to_string(*gpu.shared_memory_used_bytes.value):gpu.shared_memory_used_bytes.note)<<'\n';}
    Monitor monitor;SampleOptions options;options.include_disks=options.include_networks=options.include_processes=false;
    monitor.sample(options);std::this_thread::sleep_for(50ms);const auto snapshot=monitor.sample(options);
    for(const auto& gpu:snapshot.gpus){
        // PDH never invents free bytes; Monitor may now add the independent
        // exact-identity NVIDIA provider after this PDH stage.
        if(gpu.memory_available_bytes.value)assert(gpu.hardware_vendor_id.support==Support::available && gpu.hardware_vendor_id.value==0x10deU && gpu.memory_available_bytes.support==Support::available && gpu.memory_available_bytes.note.starts_with("NVML v1 unallocated framebuffer bytes"));
        assert(gpu.memory_used_bytes.value==gpu.dedicated_memory_used_bytes.value);
    }
    options.include_gpus=false;assert(monitor.sample(options).gpus.empty());options.include_gpus=true;
    for(const auto& gpu:monitor.sample(options).gpus)assert(!gpu.dedicated_memory_used_bytes.value && !gpu.shared_memory_used_bytes.value);
}
