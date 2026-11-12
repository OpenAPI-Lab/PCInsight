#include "../src/windows_nvml_memory.hpp"
#include "pcinsight/monitor.hpp"
#include <array>
#include <cassert>
#include <cstring>
#include <iostream>
#include <limits>
using namespace pcinsight;
using namespace pcinsight::detail;
namespace {
int mode,inits,stops,cuda_inits,counts,luid_reads,uuid_reads,nvml_reads,memory_reads;
int luid_calls[2],uuid_calls[2],nvml_calls[2];
int opened,closed,physical_calls[4];
unsigned long long amount=75;
constexpr const char* uuids[]{"GPU-00000000-0000-0000-0000-000000000001","GPU-00000000-0000-0000-0000-000000000002"};
int WINAPI cuda_init(unsigned flags){assert(flags==0);++cuda_inits;return mode==1?4:0;}
int __cdecl init(){++inits;return mode==2?4:mode==3?999:0;}
int __cdecl shutdown(){++stops;return 0;}
int WINAPI count(int* out){++counts;*out=mode==4?-1:mode==5?257:mode==6?0:mode==42?1:mode==7 && counts>1?3:2;return mode==8?1:0;}
int WINAPI device(int* out,int index){*out=mode==9?0:index;return mode==10?1:0;}
int WINAPI luid(char* out,unsigned* mask,int dev){
    assert(dev>=0 && dev<2);++luid_reads;const auto call=++luid_calls[dev];
    LUID id{static_cast<DWORD>(dev+1),dev==1?static_cast<LONG>(-1):0};
    if(mode==11)id={1,0};
    if(mode==12 && call>=3)++id.LowPart;
    if(mode==13 && call==2)++id.LowPart;
    if(mode==37 || mode==42)id={0,0};
    if(mode!=14)std::memcpy(out,&id,mode==15?4:8);
    *mask=mode==16?0:mode==17?3:mode==18 && call>=3?2:1;
    return mode==19?4:0;
}
int WINAPI uuid(WindowsCudaUuid* out,int dev){
    ++uuid_reads;const auto call=++uuid_calls[dev];
    if(mode!=20){std::memset(out,0,mode==21?8:16);out->bytes[15]=static_cast<unsigned char>(mode==22?1:dev+1);}
    if(mode==23 && call>=3)++out->bytes[15];
    return mode==24?1:0;
}
int __cdecl by_uuid(const char* text,NvmlDevice* out){
    assert(std::strcmp(text,uuids[0])==0 || std::strcmp(text,uuids[1])==0);
    *out=mode==25?nullptr:reinterpret_cast<void*>(static_cast<std::uintptr_t>(mode==26?1:text[39]-'0'));
    return mode==27?4:0;
}
int __cdecl nvml_uuid(NvmlDevice raw,char* out,unsigned capacity){
    assert(capacity==96);++nvml_reads;auto dev=static_cast<int>(reinterpret_cast<std::uintptr_t>(raw))-1;assert(dev>=0 && dev<2);
    auto call=++nvml_calls[dev];
    const char* text=mode==28?"MIG-00000000-0000-0000-0000-000000000001":mode==29?uuids[1-dev]:uuids[dev];
    if(mode!=30)std::strcpy(out,text);
    if(mode==31 && call>=2)out[39]='9';
    return mode==32?4:0;
}
int __cdecl memory(NvmlDevice raw,NvmlMemory* out){
    ++memory_reads;auto dev=static_cast<int>(reinterpret_cast<std::uintptr_t>(raw))-1;
    if(mode!=33)*out={100,amount,100-amount};
    if(mode==34)out->free=101;
    if(mode==35)out->used=101;
    if(mode==36)out->free=1;
    if(mode==38)*out={9007199254741993ULL,9007199254741992ULL,1};
    if(mode==39 && dev==0)return 4;
    if(mode==40)out->total=std::numeric_limits<unsigned long long>::max();
    if(mode==50)*out={0,0,0};
    return mode==41?999:0;
}
LONG APIENTRY open(GpuOpenLuid* in){++opened;in->handle=in->luid.LowPart?in->luid.LowPart:3;if(mode==47)++in->luid.LowPart;return mode==48?static_cast<LONG>(0xc0000022):0;}
LONG APIENTRY close(const GpuCloseDevice*){++closed;return 0;}
LONG APIENTRY physical(GpuAdapterQuery* in){assert(in->type==30 && in->size==4 && in->handle<4);auto call=++physical_calls[in->handle];*static_cast<UINT*>(in->data)=mode==43?0:mode==44 || (mode==45 && call>=2)?2:1;if(mode==46)++in->size;return mode==49?static_cast<LONG>(0xc0000022):0;}
WindowsNvmlMemoryApi api{cuda_init,count,device,luid,uuid,init,shutdown,by_uuid,nvml_uuid,memory,{open,physical,close}};
std::vector<GpuInfo> targets(){
    std::vector<GpuInfo> g(3);g[0].id="dxgi-luid:00000000:00000001";g[1].id="dxgi-luid:ffffffff:00000002";g[2].id="dxgi-luid:00000000:00000003";
    for(auto& gpu:g){gpu.hardware_vendor_id={0x10deU,Support::available,"DXGI"};gpu.memory_available_bytes={999,Support::available,"stale"};gpu.dedicated_memory_bytes={200,Support::available,"DXGI capacity"};gpu.dedicated_memory_used_bytes={125,Support::available,"PDH"};gpu.shared_memory_used_bytes={50,Support::available,"PDH"};}
    return g;
}
void reset(int scenario){mode=scenario;opened=closed=0;std::fill_n(physical_calls,4,0);inits=stops=cuda_inits=counts=luid_reads=uuid_reads=nvml_reads=memory_reads=0;std::fill_n(luid_calls,2,0);std::fill_n(uuid_calls,2,0);std::fill_n(nvml_calls,2,0);amount=75;}
std::vector<GpuInfo> run(int scenario){reset(scenario);auto g=targets();fill_windows_nvml_memory(g,api);assert(opened==closed);assert(stops==(inits && scenario!=2 && scenario!=3?1:0));assert(!g[2].memory_available_bytes.value);for(const auto& gpu:g)assert(gpu.dedicated_memory_bytes.value==200 && gpu.dedicated_memory_used_bytes.value==125 && gpu.shared_memory_used_bytes.value==50);return g;}
}
int main(){
    auto g=run(0);assert(g[0].memory_available_bytes.value==75 && g[1].memory_available_bytes.value==75);assert(g[0].memory_available_bytes.support==Support::available && memory_reads==2 && stops==1);
    for(int scenario:{1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,40,41,43,44,45,46,47,48,49}){
        g=run(scenario);assert(!g[0].memory_available_bytes.value && !g[1].memory_available_bytes.value);
        if(scenario==2 || scenario==27 || scenario==32 || scenario==48 || scenario==49)assert(g[0].memory_available_bytes.support==Support::permission_required);
        if(scenario==1)assert(g[0].memory_available_bytes.support==Support::unsupported && inits==0);
    }
    g=run(38);assert(g[0].memory_available_bytes.value==9007199254741992ULL);
    g=run(50);assert(g[0].memory_available_bytes.value==0 && g[1].memory_available_bytes.value==0);
    g=run(39);assert(!g[0].memory_available_bytes.value && g[0].memory_available_bytes.support==Support::permission_required && g[1].memory_available_bytes.value==75);
    reset(0);g=targets();fill_windows_nvml_memory(g,api);amount=0;fill_windows_nvml_memory(g,api);assert(g[0].memory_available_bytes.value==0);amount=100;fill_windows_nvml_memory(g,api);assert(g[0].memory_available_bytes.value==100);mode=41;fill_windows_nvml_memory(g,api);assert(!g[0].memory_available_bytes.value);mode=0;fill_windows_nvml_memory(g,api);assert(g[0].memory_available_bytes.value==100 && stops==5);
    reset(0);g=targets();g.push_back(g[0]);fill_windows_nvml_memory(g,api);assert(!g[0].memory_available_bytes.value && !g[3].memory_available_bytes.value && g[1].memory_available_bytes.value==75);
    reset(0);g=targets();g.push_back(g[1]);g.back().id="dxgi-luid:FFFFFFFF:00000002";g.back().hardware_vendor_id={0x1002U,Support::available,"AMD"};fill_windows_nvml_memory(g,api);assert(!g[1].memory_available_bytes.value && g.back().memory_available_bytes.value==999 && g[0].memory_available_bytes.value==75);
    reset(0);g=targets();g[0].id="invalid";fill_windows_nvml_memory(g,api);assert(!g[0].memory_available_bytes.value && g[1].memory_available_bytes.value==75);
    reset(37);g=targets();g.resize(1);g[0].id="dxgi-luid:00000000:00000000"; // two source devices claim LUID0: ambiguous, not absent inferred
    fill_windows_nvml_memory(g,api);assert(!g[0].memory_available_bytes.value);
    reset(42);g=targets();g.resize(1);g[0].id="dxgi-luid:00000000:00000000";fill_windows_nvml_memory(g,api);assert(g[0].memory_available_bytes.value==75 && opened==closed);
    reset(0);g=targets();g.resize(257);fill_windows_nvml_memory(g,api);assert(!cuda_inits);
    reset(0);g=targets();for(auto& gpu:g)gpu.hardware_vendor_id.support=Support::unsupported;fill_windows_nvml_memory(g,api);assert(!cuda_inits && g[0].memory_available_bytes.value==999);
    for(unsigned missing=0;missing<13;++missing){reset(0);auto partial=api;switch(missing){case 0:partial.cuda_init=nullptr;break;case 1:partial.count=nullptr;break;case 2:partial.device=nullptr;break;case 3:partial.luid=nullptr;break;case 4:partial.uuid=nullptr;break;case 5:partial.init=nullptr;break;case 6:partial.shutdown=nullptr;break;case 7:partial.by_uuid=nullptr;break;case 8:partial.nvml_uuid=nullptr;break;case 9:partial.memory=nullptr;break;case 10:partial.topology.open=nullptr;break;case 11:partial.topology.query=nullptr;break;case 12:partial.topology.close=nullptr;break;}g=targets();fill_windows_nvml_memory(g,partial);assert(!cuda_inits && !inits && !g[0].memory_available_bytes.value);}
    auto native=read_windows_gpus();fill_windows_nvml_memory(native);
    for(const auto& gpu:native)std::cout<<gpu.model<<" | NVML free="<<(gpu.memory_available_bytes.value?std::to_string(*gpu.memory_available_bytes.value):"unavailable")<<" | "<<gpu.memory_available_bytes.note<<'\n';
    auto unavailable=targets();unavailable.resize(1);unavailable[0].id="not-an-adapter";fill_windows_nvml_memory(unavailable);assert(!unavailable[0].memory_available_bytes.value);
    std::cout<<"Native optional loader/invalid-target probe: "<<unavailable[0].memory_available_bytes.note<<'\n';
    Monitor monitor;SampleOptions options;options.include_gpus=false;assert(monitor.sample(options).gpus.empty());options.include_gpus=true;
    for(const auto& gpu:monitor.sample(options).gpus)if(gpu.memory_available_bytes.value)assert(gpu.memory_available_bytes.note.find("NVML v1 unallocated framebuffer")!=std::string::npos);
    options.include_gpus=false;assert(monitor.sample(options).gpus.empty());
    std::cout<<"Windows NVML framebuffer free tests passed (native NVIDIA positive requires NVIDIA hardware)\n";
}
