#include "../src/windows_gpu_budget.hpp"
#include "pcinsight/monitor.hpp"
#include <cassert>
#include <iostream>
#include <limits>
#include <stdexcept>
using namespace pcinsight;
using namespace pcinsight::detail;
namespace {
int adapter,version3;unsigned opened,released,queries;int mode;
HRESULT open(void* raw,void** out){assert(raw==&adapter);++opened;*out=mode==1?nullptr:&version3;return mode==2?E_ACCESSDENIED:mode==3?E_NOINTERFACE:S_OK;}
HRESULT query(void* raw,UINT node,DXGI_MEMORY_SEGMENT_GROUP group,DXGI_QUERY_VIDEO_MEMORY_INFO* out){
    assert(raw==&version3 && node==0);++queries;
    if(mode==4 && group==DXGI_MEMORY_SEGMENT_GROUP_LOCAL)return E_ACCESSDENIED;
    if(mode==5 && group==DXGI_MEMORY_SEGMENT_GROUP_NON_LOCAL)return E_FAIL;
    if(mode==6)throw std::runtime_error("fixture");
    if(group==DXGI_MEMORY_SEGMENT_GROUP_LOCAL)*out={1000,250,400,50};else *out={2000,3000,600,70};
    if(mode==7)*out={};
    if(mode==8)*out={std::numeric_limits<UINT64>::max(),1,0,std::numeric_limits<UINT64>::max()};
    return S_OK;
}
void release(void* raw){assert(raw==&version3);++released;}
GpuBudgetApi api{open,query,release};
GpuInfo run(int scenario){mode=scenario;opened=released=queries=0;GpuInfo gpu;gpu.memory_available_bytes={123,Support::estimated,"untouched fixture global"};fill_windows_gpu_budget(&adapter,gpu,api);assert(gpu.memory_available_bytes.value==123);assert(released==(mode==1?0:1));return gpu;}
}
int main(){
    auto gpu=run(0);auto& a=gpu.local_process_memory;auto& b=gpu.non_local_process_memory;
    assert(a.process_id.value==GetCurrentProcessId() && a.node_index.value==0 && a.budget_bytes.value==1000 && a.current_usage_bytes.value==250);
    assert(a.available_for_reservation_bytes.value==400 && a.current_reservation_bytes.value==50 && a.budget_headroom_bytes.value==750);
    assert(b.current_usage_bytes.value==3000 && b.budget_bytes.value==2000 && b.budget_headroom_bytes.value==0 && queries==2);
    for(int m:{1,2,3}){gpu=run(m);assert(!gpu.local_process_memory.budget_bytes.value && queries==0);if(m==2)assert(gpu.local_process_memory.budget_bytes.support==Support::permission_required);}
    gpu=run(4);assert(!gpu.local_process_memory.budget_bytes.value && gpu.non_local_process_memory.budget_bytes.value);
    gpu=run(5);assert(gpu.local_process_memory.budget_bytes.value && !gpu.non_local_process_memory.budget_bytes.value);
    gpu=run(7);assert(gpu.local_process_memory.budget_bytes.value==0 && gpu.local_process_memory.budget_headroom_bytes.value==0);
    gpu=run(8);assert(gpu.local_process_memory.budget_headroom_bytes.value==std::numeric_limits<UINT64>::max()-1);
    mode=4;fill_windows_gpu_budget(&adapter,gpu,api);assert(!gpu.local_process_memory.budget_bytes.value); // stale reset
    mode=6;released=0;try{fill_windows_gpu_budget(&adapter,gpu,api);assert(false);}catch(const std::runtime_error&){}assert(released==1);
    auto missing=api;missing.query=nullptr;opened=0;fill_windows_gpu_budget(&adapter,gpu,missing);assert(!gpu.local_process_memory.budget_bytes.value && opened==0);
    fill_windows_gpu_budget(nullptr,gpu,api);assert(!gpu.local_process_memory.budget_bytes.value);
    for(const auto& g:read_windows_gpus()){
        const auto& local=g.local_process_memory;assert(local.process_id.value==GetCurrentProcessId() && !g.memory_available_bytes.value);
        std::cout<<g.model<<" | caller="<<*local.process_id.value<<" node=0 | local budget="<<local.budget_bytes.value.value_or(0)<<" usage="<<local.current_usage_bytes.value.value_or(0)<<" headroom="<<local.budget_headroom_bytes.value.value_or(0)<<'\n';
    }
    Monitor monitor;SampleOptions options;options.include_gpus=false;assert(monitor.sample(options).gpus.empty());options.include_gpus=true;
    for(const auto& g:monitor.sample(options).gpus){
        assert(g.local_process_memory.process_id.value==GetCurrentProcessId());
        if(g.memory_available_bytes.value)assert(g.hardware_vendor_id.support==Support::available && g.hardware_vendor_id.value==0x10deU && g.memory_available_bytes.support==Support::available && g.memory_available_bytes.note.starts_with("NVML v1 unallocated framebuffer bytes"));
    }
    std::cout<<"GPU caller process budget tests passed\n";
}
