#include "windows_nvml_memory.hpp"
#include "windows_nvml_loader.hpp"
#ifdef _WIN32
#include <algorithm>
#include <array>
#include <charconv>
#include <cstring>
#include <limits>
#include <map>
#include <set>
namespace pcinsight::detail {
static_assert(sizeof(WindowsCudaUuid)==16 && sizeof(NvmlMemory)==24);
namespace {
bool nvidia(const GpuInfo& gpu){return gpu.hardware_vendor_id.support==Support::available && gpu.hardware_vendor_id.value==0x10deU;}
Metric<std::uint64_t> missing(const char* note,int error=0){return {{},error==4?Support::permission_required:Support::unsupported,std::string(note)+(error?"; status="+std::to_string(error):"")};}
std::optional<std::uint64_t> key(std::string_view id){
    if(id.size()!=27 || !id.starts_with("dxgi-luid:") || id[18]!=':')return {};
    std::uint32_t high{},low{};
    const auto a=std::from_chars(id.data()+10,id.data()+18,high,16),b=std::from_chars(id.data()+19,id.data()+27,low,16);
    if(a.ec!=std::errc{} || a.ptr!=id.data()+18 || b.ec!=std::errc{} || b.ptr!=id.data()+27)return {};
    return (static_cast<std::uint64_t>(high)<<32)|low;
}
struct Identity {std::uint64_t luid;unsigned mask;WindowsCudaUuid uuid;};
bool same(const Identity& a,const Identity& b){return a.luid==b.luid && a.mask==b.mask && std::memcmp(a.uuid.bytes,b.uuid.bytes,16)==0;}
bool identity(WindowsNvmlMemoryApi api,int device,Identity& out,unsigned char seed){
    std::array<char,8> bytes;std::memset(bytes.data(),seed,bytes.size());
    out.mask=0;std::memset(out.uuid.bytes,seed,16);
    if(api.luid(bytes.data(),&out.mask,device)!=0 || !out.mask || (out.mask&(out.mask-1)) || api.uuid(&out.uuid,device)!=0)return false;
    LUID luid{};std::memcpy(&luid,bytes.data(),8);
    out.luid=(static_cast<std::uint64_t>(static_cast<ULONG>(luid.HighPart))<<32)|luid.LowPart;
    return true;
}
std::string uuid_text(const WindowsCudaUuid& uuid){
    constexpr char hex[]="0123456789abcdef";std::string out="GPU-";
    for(unsigned i=0;i<16;++i){if(i==4 || i==6 || i==8 || i==10)out+='-';out+=hex[uuid.bytes[i]>>4];out+=hex[uuid.bytes[i]&15];}
    return out;
}
int uuid_status(WindowsNvmlMemoryApi api,NvmlDevice device,const std::string& expected){
    std::array<char,96> text;text.fill(static_cast<char>(0x7f));
    const auto status=api.nvml_uuid(device,text.data(),static_cast<unsigned>(text.size()));if(status!=0)return status;
    const auto end=std::find(text.begin(),text.end(),'\0');if(end==text.end() || end-text.begin()!=40)return 2;
    for(auto& c:text)if(c>='A' && c<='F')c=static_cast<char>(c-'A'+'a');
    return std::equal(text.begin(),end,expected.begin(),expected.end())?0:2; // rejects MIG-/truncation/suffix
}
struct Session {WindowsNvmlMemoryApi api;~Session(){api.shutdown();}};
struct Adapter {GpuOpenLuid value;GpuTemperatureApi api;~Adapter(){if(value.handle){GpuCloseDevice close{value.handle};api.close(&close);}}};
LONG single_physical(GpuTemperatureApi api,UINT handle){
    UINT count{};GpuAdapterQuery q{handle,30,&count,sizeof(count)}; // PHYSICALADAPTERCOUNT
    const auto status=api.query(&q);if(status!=0)return status;
    return q.handle==handle && q.type==30 && q.data==&count && q.size==sizeof(count) && count==1?0:static_cast<LONG>(0xc000000d);
}
int topology_error(LONG status){return static_cast<ULONG>(status)==0xc0000022?4:0;}
struct Module {HMODULE handle;~Module(){if(handle)FreeLibrary(handle);}};
template<class T> void symbol(HMODULE module,T& fn,const char* name){auto raw=GetProcAddress(module,name);static_assert(sizeof(raw)==sizeof(fn));std::memcpy(&fn,&raw,sizeof(fn));}
}
void fill_windows_nvml_memory(std::vector<GpuInfo>& gpus,WindowsNvmlMemoryApi api){
    std::map<std::uint64_t,unsigned> targets;
    for(auto& gpu:gpus){
        if(auto id=key(gpu.id))++targets[*id]; // duplicate non-NVIDIA rows are ambiguous too
        if(nvidia(gpu))gpu.memory_available_bytes=missing("NVML framebuffer free unavailable: exact CUDA LUID/UUID association required");
    }
    if(gpus.size()>256 || std::none_of(gpus.begin(),gpus.end(),nvidia))return;
    auto fail=[&](const char* note,int error=0){for(auto& gpu:gpus)if(nvidia(gpu))gpu.memory_available_bytes=missing(note,error);};
    if(!api.cuda_init || !api.count || !api.device || !api.luid || !api.uuid || !api.init || !api.shutdown || !api.by_uuid || !api.nvml_uuid || !api.memory || !api.topology.open || !api.topology.query || !api.topology.close)return;
    if(api.cuda_init(0)!=0){fail("CUDA driver initialization failed (not mapped to NVML permission codes)");return;}
    const int error=api.init();if(error!=0){fail("NVML initialization failed",error);return;}
    Session session{api};
    int count=-1;if(api.count(&count)!=0 || count<0 || count>256){fail("CUDA device count unavailable or invalid");return;}
    struct Source {int device;Identity id;std::string uuid;};
    std::vector<Source> sources;std::set<int> handles;std::map<std::uint64_t,unsigned> luids;std::map<std::string,unsigned> uuids;
    for(int index=0;index<count;++index){
        int device=-1;Identity a{},b{};
        if(api.device(&device,index)!=0 || device<0 || !handles.insert(device).second ||
           !identity(api,device,a,0xa5) || !identity(api,device,b,0x5a) || !same(a,b)){
            fail("CUDA enumeration/identity incomplete, malformed, or unstable");return;
        }
        auto uuid=uuid_text(a.uuid);++luids[a.luid];++uuids[uuid];sources.push_back({device,a,std::move(uuid)});
    }
    std::map<std::uint64_t,Metric<std::uint64_t>> staged;std::set<NvmlDevice> seen;
    for(const auto& source:sources){
        if(targets[source.id.luid]!=1 || luids[source.id.luid]!=1 || uuids[source.uuid]!=1)continue;
        const auto target=std::find_if(gpus.begin(),gpus.end(),[&](const auto& gpu){return nvidia(gpu) && key(gpu.id)==source.id.luid;});
        if(target==gpus.end())continue;
        LUID luid{};luid.LowPart=static_cast<ULONG>(source.id.luid);const auto high=static_cast<ULONG>(source.id.luid>>32);std::memcpy(&luid.HighPart,&high,sizeof(high));
        Adapter adapter{{luid,0},api.topology};
        const auto opened=api.topology.open(&adapter.value);
        if(opened!=0 || !adapter.value.handle || std::memcmp(&adapter.value.luid,&luid,sizeof(luid))!=0){staged[source.id.luid]=missing("KMT exact LUID open failed/invalid identity",topology_error(opened));continue;}
        auto physical=single_physical(api.topology,adapter.value.handle);
        if(physical!=0){staged[source.id.luid]=missing("KMT single physical adapter not proven; linked/unknown topology",topology_error(physical));continue;}
        NvmlDevice device{};const auto status=api.by_uuid(source.uuid.c_str(),&device);
        if(status!=0 || !device){staged[source.id.luid]=missing("NVML UUID lookup failed or returned null",status);continue;}
        if(!seen.insert(device).second){fail("NVML device handle aliases distinct CUDA UUIDs");return;}
        auto uuid_error=uuid_status(api,device,source.uuid);
        if(uuid_error!=0){staged[source.id.luid]=missing("NVML UUID roundtrip failed/mismatch/unsupported root GPU identity",uuid_error);continue;}
        constexpr auto absent=std::numeric_limits<unsigned long long>::max();
        NvmlMemory memory{absent,absent,absent};const auto result=api.memory(device,&memory);
        if(result!=0){staged[source.id.luid]=missing("NVML v1 framebuffer memory query failed",result);continue;}
        Identity checked{};
        if(!identity(api,source.device,checked,0x3c) || !same(source.id,checked)){
            fail("CUDA LUID/UUID changed during memory query");return;
        }
        uuid_error=uuid_status(api,device,source.uuid);if(uuid_error!=0){fail("NVML UUID changed/revalidation failed during memory query",uuid_error);return;}
        physical=single_physical(api.topology,adapter.value.handle);
        if(physical!=0){staged[source.id.luid]=missing("KMT single physical adapter changed/revalidation failed",topology_error(physical));continue;}
        if(memory.total==absent || memory.free==absent || memory.used==absent || memory.free>memory.total || memory.used>memory.total || memory.free!=memory.total-memory.used){
            staged[source.id.luid]=missing("NVML v1 framebuffer memory absent or inconsistent");continue;
        }
        staged[source.id.luid]={static_cast<std::uint64_t>(memory.free),Support::available,
            "NVML v1 unallocated framebuffer bytes, exact CUDA LUID/UUID and root NVML UUID, KMT single physical adapter; WDDM-managed, not guaranteed allocatable nor process budget; independent of DXGI/PDH"};
    }
    int checked=-1;if(api.count(&checked)!=0 || checked!=count){fail("CUDA device count changed/revalidation failed");return;}
    for(auto& gpu:gpus)if(nvidia(gpu))if(auto id=key(gpu.id)){auto found=staged.find(*id);if(found!=staged.end())gpu.memory_available_bytes=std::move(found->second);}
}
void fill_windows_nvml_memory(std::vector<GpuInfo>& gpus){
    if(std::none_of(gpus.begin(),gpus.end(),nvidia))return;
    if constexpr(sizeof(void*)!=8){for(auto& gpu:gpus)if(nvidia(gpu))gpu.memory_available_bytes=missing("Native Windows NVML memory provider currently supports 64-bit callers only");return;}
    // CUDA/KMT remain System32-only; NVML also supports the fixed legacy
    // ProgramFilesX64/NVIDIA Corporation/NVSMI location, never cwd or PATH.
    Module cuda{LoadLibraryExW(L"nvcuda.dll",nullptr,LOAD_LIBRARY_SEARCH_SYSTEM32)};
    const DWORD cuda_error=cuda.handle?0:GetLastError();
    const auto library=cuda.handle?load_windows_nvml_library():WindowsNvmlLibrary{nullptr,cuda_error,"NVML load skipped because System32 CUDA driver is unavailable"};
    Module nvml{library.handle};
    const DWORD nvml_error=library.error;
    Module gdi{cuda.handle && nvml.handle?LoadLibraryExW(L"gdi32.dll",nullptr,LOAD_LIBRARY_SEARCH_SYSTEM32):nullptr};
    const DWORD gdi_error=gdi.handle?0:GetLastError();
    WindowsNvmlMemoryApi api;
    if(cuda.handle && nvml.handle && gdi.handle){
        symbol(cuda.handle,api.cuda_init,"cuInit");symbol(cuda.handle,api.count,"cuDeviceGetCount");symbol(cuda.handle,api.device,"cuDeviceGet");
        symbol(cuda.handle,api.luid,"cuDeviceGetLuid");symbol(cuda.handle,api.uuid,"cuDeviceGetUuid_v2");
        symbol(nvml.handle,api.init,"nvmlInit_v2");symbol(nvml.handle,api.shutdown,"nvmlShutdown");
        symbol(nvml.handle,api.by_uuid,"nvmlDeviceGetHandleByUUID");symbol(nvml.handle,api.nvml_uuid,"nvmlDeviceGetUUID");symbol(nvml.handle,api.memory,"nvmlDeviceGetMemoryInfo");
        symbol(gdi.handle,api.topology.open,"D3DKMTOpenAdapterFromLuid");symbol(gdi.handle,api.topology.query,"D3DKMTQueryAdapterInfo");symbol(gdi.handle,api.topology.close,"D3DKMTCloseAdapter");
    }
    fill_windows_nvml_memory(gpus,api);
    const auto provenance=std::string("; ")+library.note+"; NVML loader Win32="+std::to_string(library.error);
    for(auto& gpu:gpus)if(nvidia(gpu))gpu.memory_available_bytes.note+=provenance;
    if(!cuda.handle || !nvml.handle || !gdi.handle)for(auto& gpu:gpus)if(nvidia(gpu)){
        gpu.memory_available_bytes=missing("NVIDIA/KMT driver libraries unavailable; no install/PATH fallback");
        gpu.memory_available_bytes.note+=provenance;
        if(cuda_error==ERROR_ACCESS_DENIED || nvml_error==ERROR_ACCESS_DENIED || gdi_error==ERROR_ACCESS_DENIED)gpu.memory_available_bytes.support=Support::permission_required;
    }
}
}
#endif
