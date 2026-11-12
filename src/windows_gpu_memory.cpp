#include "windows_gpu_memory.hpp"
#ifdef _WIN32
#include <pdhmsg.h>
#include <cstring>
#include <cwchar>
#include <map>
#include <optional>
#include <string_view>
namespace pcinsight::detail {
namespace {
using Bytes=Metric<std::uint64_t>;
Bytes missing(const char* message,PDH_STATUS code=PDH_INVALID_DATA) {
    return {std::nullopt,code==ERROR_ACCESS_DENIED || static_cast<DWORD>(code)==PDH_ACCESS_DENIED?Support::permission_required:Support::unsupported,
        std::string(message)+"; PDH status="+std::to_string(static_cast<unsigned long>(code))};
}
struct Instance {std::string id;};
std::optional<Instance> instance(std::wstring_view name) {
    if(name.size()<33 || name.substr(0,7)!=L"luid_0x" || name.substr(15,3)!=L"_0x" || name.substr(26,6)!=L"_phys_")return {};
    std::string id="dxgi-luid:";
    for(unsigned start:{7u,18u}) {
        if(start==18)id+=':';
        for(unsigned i=start;i<start+8;++i){auto c=name[i];
            if(c>=L'A' && c<=L'F')c+=L'a'-L'A';
            if(!((c>=L'0' && c<=L'9') || (c>=L'a' && c<=L'f')))return {};
            id+=static_cast<char>(c);
        }
    }
    if(name.size()>42)return {};
    std::uint64_t physical=0;
    for(std::size_t i=32;i<name.size();++i){if(name[i]<L'0' || name[i]>L'9')return {};
        physical=physical*10+static_cast<unsigned>(name[i]-L'0');if(physical>0xffffffffULL)return {};}
    return Instance{std::move(id)}; // do not sum physical nodes under a logical LUID
}
struct Values {std::map<std::string,Bytes> values;Bytes error;bool failed{};};
Values read_values(GpuMemoryPdhApi& api,PDH_HCOUNTER counter) {
    auto fail=[](const char* message,PDH_STATUS code){return Values{{},missing(message,code),true};};
    DWORD size=0,count=0;
    constexpr DWORD format=PDH_FMT_LARGE|PDH_FMT_NOSCALE|PDH_FMT_NOCAP100;
    auto rc=api.format(counter,format,&size,&count,nullptr);
    if(static_cast<DWORD>(rc)!=PDH_MORE_DATA)return fail("GPU memory array size unavailable",rc==ERROR_SUCCESS?static_cast<PDH_STATUS>(PDH_CSTATUS_NO_INSTANCE):rc);
    if(size<sizeof(PDH_FMT_COUNTERVALUE_ITEM_W) || size>4*1024*1024)return fail("invalid GPU memory array size",PDH_INVALID_DATA);
    std::vector<unsigned char> buffer(size);
    rc=api.format(counter,format,&size,&count,reinterpret_cast<PPDH_FMT_COUNTERVALUE_ITEM_W>(buffer.data()));
    if(rc!=ERROR_SUCCESS)return fail("GPU memory array read failed",rc);
    if(size>buffer.size() || count>16384 || count>size/sizeof(PDH_FMT_COUNTERVALUE_ITEM_W))return fail("invalid GPU memory array count",PDH_INVALID_DATA);
    Values result{{},missing("GPU adapter memory instance absent",PDH_CSTATUS_NO_INSTANCE),false};
    const auto base=reinterpret_cast<std::uintptr_t>(buffer.data());
    for(DWORD i=0;i<count;++i) {
        PDH_FMT_COUNTERVALUE_ITEM_W item{};std::memcpy(&item,buffer.data()+i*sizeof item,sizeof item);
        const auto pointer=reinterpret_cast<std::uintptr_t>(item.szName);
        if(pointer<base+count*sizeof item || pointer>=base+size || (pointer-base)%alignof(wchar_t))return fail("invalid GPU instance pointer",PDH_INVALID_DATA);
        const auto capacity=(size-(pointer-base))/sizeof(wchar_t);
        const auto end=std::wmemchr(item.szName,L'\0',capacity);
        if(!end || end-item.szName>128)return fail("invalid/unterminated GPU instance name",PDH_INVALID_DATA);
        const std::wstring_view name(item.szName,static_cast<std::size_t>(end-item.szName));
        if(name==L"_Total")continue;
        const auto parsed=instance(name);
        if(!parsed){if(name.starts_with(L"luid_"))return fail("unrecognized GPU LUID/physical node instance",PDH_INVALID_DATA);continue;}
        if(result.values.contains(parsed->id)) {
            result.values[parsed->id]=missing("multiple/duplicate physical instances for logical GPU; not summed");continue;
        }
        if(item.FmtValue.CStatus!=PDH_CSTATUS_VALID_DATA && item.FmtValue.CStatus!=PDH_CSTATUS_NEW_DATA)
            result.values.emplace(parsed->id,missing("GPU memory item status invalid",static_cast<PDH_STATUS>(item.FmtValue.CStatus)));
        else if(item.FmtValue.largeValue<0)result.values.emplace(parsed->id,missing("negative GPU memory usage"));
        else result.values.emplace(parsed->id,Bytes{static_cast<std::uint64_t>(item.FmtValue.largeValue),Support::available,
            "PDH GPU Adapter Memory usage across processes, single physical instance; not caller-process usage"});
    }
    return result;
}
}
void WindowsGpuMemory::reset(Slot& slot) {if(slot.query)api_.close(slot.query);slot={};}
void WindowsGpuMemory::reset() {for(auto& slot:slots_)reset(slot);identities_.clear();}
void WindowsGpuMemory::fill(std::vector<GpuInfo>& gpus) {
    const auto initial=missing("GPU memory requires a fresh query and two collections");
    for(auto& gpu:gpus){gpu.dedicated_memory_used_bytes=gpu.shared_memory_used_bytes=gpu.memory_used_bytes=initial;
        gpu.memory_available_bytes={std::nullopt,Support::unsupported,"DXGI capacity minus PDH usage is not guaranteed allocatable/free GPU memory"};}
    std::set<std::string> ids;
    for(const auto& gpu:gpus)if(gpu.id.empty() || !ids.insert(gpu.id).second){reset();return;}
    if(ids.empty()){reset();return;}
    if(ids!=identities_){reset();identities_=std::move(ids);}
    constexpr const wchar_t* paths[]{L"\\GPU Adapter Memory(*)\\Dedicated Usage",L"\\GPU Adapter Memory(*)\\Shared Usage"};
    for(unsigned index=0;index<2;++index) {
        auto& slot=slots_[index];Values values{{},initial,false};
        auto sample=[&] {
            if(!slot.query) {
                auto rc=api_.open(nullptr,0,&slot.query);
                if(rc!=ERROR_SUCCESS || !slot.query){values.error=missing("GPU memory query open failed",rc==ERROR_SUCCESS?static_cast<PDH_STATUS>(PDH_INVALID_HANDLE):rc);reset(slot);return;}
                rc=api_.add(slot.query,paths[index],0,&slot.counter);
                if(rc!=ERROR_SUCCESS || !slot.counter){values.error=missing("GPU memory counter unavailable",rc==ERROR_SUCCESS?static_cast<PDH_STATUS>(PDH_INVALID_HANDLE):rc);reset(slot);return;}
            }
            const auto rc=api_.collect(slot.query);
            if(rc!=ERROR_SUCCESS){values.error=missing("GPU memory collection failed",rc);reset(slot);return;}
            if(!slot.primed){slot.primed=true;return;}
            values=read_values(api_,slot.counter);
            if(values.failed)reset(slot);
        };
        sample();
        for(auto& gpu:gpus) {
            const auto found=values.values.find(gpu.id);
            const auto& value=found==values.values.end()?values.error:found->second;
            if(index==0){gpu.dedicated_memory_used_bytes=value;gpu.memory_used_bytes=value;
                if(value.value)gpu.memory_used_bytes.note="Windows dedicated GPU memory usage only; shared usage is reported separately";}
            else gpu.shared_memory_used_bytes=value;
        }
    }
}
}
#endif
