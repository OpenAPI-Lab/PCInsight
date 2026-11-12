#include "windows_gpu_temperature.hpp"
#ifdef _WIN32
#include <charconv>
#include <map>
#include <cstring>
#include <cstddef>
#include <limits>
namespace pcinsight::detail {
static_assert(sizeof(GpuOpenLuid)==12 && offsetof(GpuOpenLuid,handle)==8);
static_assert(sizeof(GpuAdapterPerfData)==64 && alignof(GpuAdapterPerfData)==8);
static_assert(offsetof(GpuAdapterPerfData,memory_frequency)==8 && offsetof(GpuAdapterPerfData,temperature)==56 && offsetof(GpuAdapterPerfData,power_state)==60);
#ifdef _WIN64
static_assert(sizeof(GpuAdapterQuery)==24 && offsetof(GpuAdapterQuery,data)==8 && offsetof(GpuAdapterQuery,size)==16);
#endif
namespace {
Metric<double> missing(const char* reason,LONG status=0){return {{},static_cast<ULONG>(status)==0xc0000022?Support::permission_required:Support::unsupported,reason};}
std::optional<LUID> parse(std::string_view id){
    if(id.size()!=27 || !id.starts_with("dxgi-luid:") || id[18]!=':')return {};
    ULONG high{},low{};auto a=std::from_chars(id.data()+10,id.data()+18,high,16),b=std::from_chars(id.data()+19,id.data()+27,low,16);
    if(a.ec!=std::errc{} || a.ptr!=id.data()+18 || b.ec!=std::errc{} || b.ptr!=id.data()+27)return {};
    LUID luid{};luid.LowPart=low;std::memcpy(&luid.HighPart,&high,sizeof(high));return luid;
}
std::uint64_t key(LUID luid){return (static_cast<std::uint64_t>(static_cast<ULONG>(luid.HighPart))<<32)|luid.LowPart;}
struct Adapter {GpuOpenLuid value;GpuTemperatureApi api;~Adapter(){if(value.handle){GpuCloseDevice close{value.handle};api.close(&close);}}};
template<class T> LONG query(GpuTemperatureApi api,UINT handle,UINT type,T& data){
    GpuAdapterQuery q{handle,type,&data,sizeof(data)};const auto status=api.query(&q);
    if(status!=0)return status;
    if(q.handle!=handle || q.type!=type || q.data!=&data || q.size!=sizeof(data))return static_cast<LONG>(0xc000000d);
    return 0;
}
}
void fill_windows_gpu_temperature(std::vector<GpuInfo>& gpus,GpuTemperatureApi api){
    for(auto& gpu:gpus)gpu.temperature_celsius=missing("Native KMT GPU main-sensor temperature unavailable");
    if(gpus.size()>256 || !api.open || !api.query || !api.close)return;
    std::map<std::uint64_t,unsigned> counts;
    for(const auto& gpu:gpus)if(auto luid=parse(gpu.id))++counts[key(*luid)];
    for(auto& gpu:gpus){
        const auto luid=parse(gpu.id);if(!luid || counts[key(*luid)]!=1)continue;
        Adapter adapter{{*luid,0},api};auto status=api.open(&adapter.value);
        if(status!=0 || !adapter.value.handle || key(adapter.value.luid)!=key(*luid)){gpu.temperature_celsius=missing("KMT open by exact DXGI LUID failed or returned invalid identity",status);continue;}
        UINT physical_count{};status=query(api,adapter.value.handle,30,physical_count); // PHYSICALADAPTERCOUNT
        if(status!=0 || physical_count!=1){gpu.temperature_celsius=missing("KMT physical count unavailable or linked adapters ambiguous",status);continue;}
        GpuAdapterPerfData data{};data.temperature=std::numeric_limits<ULONG>::max();
        status=query(api,adapter.value.handle,62,data); // ADAPTERPERFDATA
        if(status!=0){gpu.temperature_celsius=missing("KMT adapter performance temperature query failed",status);continue;}
        if(data.physical_index!=0 || data.temperature>2550){gpu.temperature_celsius=missing("KMT temperature output absent/malformed or outside validated 0..255 C range");continue;}
        UINT checked_count{};status=query(api,adapter.value.handle,30,checked_count);
        if(status!=0 || checked_count!=1){gpu.temperature_celsius=missing("KMT physical adapter count changed or revalidation failed",status);continue;}
        gpu.temperature_celsius={data.temperature/10.0,Support::available,"KMT main temperature sensor reported for exact DXGI LUID, single physical adapter; not hotspot/VRAM temperature"};
    }
}
void fill_windows_gpu_temperature(std::vector<GpuInfo>& gpus){
    if(gpus.empty())return;
    if constexpr(sizeof(void*)!=8){for(auto& gpu:gpus)gpu.temperature_celsius=missing("Native KMT temperature provider currently supports 64-bit callers only");return;}
    struct Module {HMODULE handle;~Module(){if(handle)FreeLibrary(handle);}} module{LoadLibraryExW(L"gdi32.dll",nullptr,LOAD_LIBRARY_SEARCH_SYSTEM32)};
    GpuTemperatureApi api;
    if(!module.handle){const auto error=GetLastError();fill_windows_gpu_temperature(gpus,api);if(error==ERROR_ACCESS_DENIED)for(auto& gpu:gpus)gpu.temperature_celsius.support=Support::permission_required;return;}
    auto symbol=[&](auto& fn,const char* name){const auto proc=GetProcAddress(module.handle,name);static_assert(sizeof(fn)==sizeof(proc));std::memcpy(&fn,&proc,sizeof(proc));};
    symbol(api.open,"D3DKMTOpenAdapterFromLuid");symbol(api.query,"D3DKMTQueryAdapterInfo");symbol(api.close,"D3DKMTCloseAdapter");
    fill_windows_gpu_temperature(gpus,api);
}
}
#endif
