#include "windows_gpu_engines.hpp"
#ifdef _WIN32
#include <pdhmsg.h>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <cwchar>
#include <map>
#include <optional>
#include <tuple>
namespace pcinsight::detail {
namespace {
Metric<double> missing(const char* message,PDH_STATUS code=PDH_INVALID_DATA) {
    return {std::nullopt,code==ERROR_ACCESS_DENIED || static_cast<DWORD>(code)==PDH_ACCESS_DENIED?Support::permission_required:Support::unsupported,
        std::string(message)+"; PDH status="+std::to_string(static_cast<unsigned long>(code))};
}
struct Instance {std::string luid,type;std::uint32_t pid{},physical{},engine{};};
std::optional<Instance> parse(std::wstring_view text) {
    std::size_t pos=0;
    auto literal=[&](std::wstring_view value){if(text.substr(pos,value.size())!=value)return false;pos+=value.size();return true;};
    auto number=[&](std::uint32_t& out){const auto start=pos;std::uint64_t value=0;
        while(pos<text.size() && text[pos]>=L'0' && text[pos]<=L'9'){
            value=value*10+static_cast<unsigned>(text[pos++]-L'0');if(value>0xffffffffULL || pos-start>10)return false;}
        out=static_cast<std::uint32_t>(value);return pos>start;
    };
    Instance result;result.luid="dxgi-luid:";
    if(!literal(L"pid_") || !number(result.pid) || !literal(L"_luid_0x"))return {};
    for(unsigned part=0;part<2;++part) {
        if(part){if(!literal(L"_0x"))return {};result.luid+=':';}
        if(text.size()-pos<8)return {};
        for(unsigned i=0;i<8;++i){auto c=text[pos++];if(c>=L'A' && c<=L'F')c+=L'a'-L'A';
            if(!((c>=L'0' && c<=L'9') || (c>=L'a' && c<=L'f')))return {};
            result.luid+=static_cast<char>(c);}
    }
    if(!literal(L"_phys_") || !number(result.physical) || !literal(L"_eng_") || !number(result.engine) || !literal(L"_engtype_"))return {};
    if(pos==text.size() || text.size()-pos>64)return {};
    while(pos<text.size()){const auto c=text[pos++];if(c<32 || c>126 || c==L'#')return {};result.type+=static_cast<char>(c);}
    return result;
}
using EngineKey=std::pair<std::uint32_t,std::uint32_t>;
struct Group {std::string type;double total{};Metric<double> metric{0.0,Support::estimated,"sum of observed per-process utilization on this engine; coverage estimated"};};
using Groups=std::map<std::string,std::map<EngineKey,Group>>;
struct Result {Groups groups;Metric<double> error;bool failed{};};
Result read(GpuMemoryPdhApi& api,PDH_HCOUNTER counter,const std::set<std::string>& ids) {
    auto fail=[](const char* message,PDH_STATUS code=PDH_INVALID_DATA){return Result{{},missing(message,code),true};};
    DWORD bytes=0,count=0;constexpr DWORD flags=PDH_FMT_DOUBLE|PDH_FMT_NOSCALE|PDH_FMT_NOCAP100;
    auto rc=api.format(counter,flags,&bytes,&count,nullptr);
    if(static_cast<DWORD>(rc)!=PDH_MORE_DATA)return fail("GPU engine array size unavailable",rc);
    if(bytes<sizeof(PDH_FMT_COUNTERVALUE_ITEM_W) || bytes>8*1024*1024)return fail("invalid GPU engine array size");
    std::vector<unsigned char> buffer(bytes);
    rc=api.format(counter,flags,&bytes,&count,reinterpret_cast<PPDH_FMT_COUNTERVALUE_ITEM_W>(buffer.data()));
    if(rc!=ERROR_SUCCESS)return fail("GPU engine array read failed",rc);
    if(bytes>buffer.size() || count>65536 || count>bytes/sizeof(PDH_FMT_COUNTERVALUE_ITEM_W))return fail("invalid GPU engine item count");
    Result result{{},missing("no observed GPU engine instances",PDH_CSTATUS_NO_INSTANCE),false};
    std::set<std::tuple<std::string,std::uint32_t,std::uint32_t,std::uint32_t>> seen;
    const auto base=reinterpret_cast<std::uintptr_t>(buffer.data());std::size_t engines=0;
    constexpr double tolerance=0.000001; // numerical roundoff only, not arbitrary workload clamping
    for(DWORD i=0;i<count;++i) {
        PDH_FMT_COUNTERVALUE_ITEM_W item{};std::memcpy(&item,buffer.data()+i*sizeof item,sizeof item);
        const auto pointer=reinterpret_cast<std::uintptr_t>(item.szName);
        if(pointer<base+count*sizeof item || pointer>=base+bytes || (pointer-base)%alignof(wchar_t))return fail("invalid GPU engine name pointer");
        const auto capacity=std::min<std::size_t>((bytes-(pointer-base))/sizeof(wchar_t),257);
        const auto end=std::wmemchr(item.szName,L'\0',capacity);
        if(!end)return fail("GPU engine name too long/unterminated");
        const std::wstring_view name(item.szName,static_cast<std::size_t>(end-item.szName));
        if(name==L"_Total")continue;
        const auto instance=parse(name);
        if(!instance)return fail("unsupported/ambiguous GPU engine instance grammar");
        if(!ids.contains(instance->luid))continue;
        auto& groups=result.groups[instance->luid];
        auto [it,inserted]=groups.try_emplace({instance->physical,instance->engine});auto& group=it->second;
        auto poison=[&](Metric<double> error){if(group.metric.value || error.support==Support::permission_required)group.metric=std::move(error);};
        if(inserted){if(++engines>4096)return fail("GPU engine count limit");group.type=instance->type;}
        if(group.type!=instance->type){group.type.clear();poison(missing("inconsistent reported types for the same GPU engine"));}
        if(!seen.emplace(instance->luid,instance->physical,instance->engine,instance->pid).second)
            poison(missing("duplicate PID/physical/engine instance; not double counted"));
        if(item.FmtValue.CStatus!=PDH_CSTATUS_VALID_DATA && item.FmtValue.CStatus!=PDH_CSTATUS_NEW_DATA) {
            const auto error=missing("GPU engine item status invalid",static_cast<PDH_STATUS>(item.FmtValue.CStatus));
            poison(error);
        } else if(!std::isfinite(item.FmtValue.doubleValue) || item.FmtValue.doubleValue<0 || item.FmtValue.doubleValue>100+tolerance)
            poison(missing("GPU process-engine utilization outside 0..100"));
        else if(group.metric.value) {
            group.total+=item.FmtValue.doubleValue; // retain unrounded sum, do not repeatedly discard small overruns
            if(group.total>100+tolerance)group.metric=missing("GPU engine sum above 100; inconsistent/overlapping observations");
            else group.metric.value=std::min(100.0,group.total);
        }
    }
    return result;
}
}
void WindowsGpuEngines::reset() {if(query_)api_.close(query_);query_=nullptr;counter_=nullptr;primed_=false;identities_.clear();}
void WindowsGpuEngines::fill(std::vector<GpuInfo>& gpus) {
    const auto initial=missing("GPU engines require two fresh counter collections");
    auto clear=[&](const Metric<double>& error){for(auto& gpu:gpus){gpu.engines.clear();gpu.utilization_percent=error;}};
    clear(initial);std::set<std::string> ids;
    for(const auto& gpu:gpus)if(gpu.id.empty() || !ids.insert(gpu.id).second){reset();return;}
    if(ids.empty()){reset();return;}
    if(ids!=identities_){reset();identities_=std::move(ids);}
    if(!query_) {
        auto rc=api_.open(nullptr,0,&query_);
        if(rc!=ERROR_SUCCESS || !query_){clear(missing("GPU engine query open failed",rc==ERROR_SUCCESS?static_cast<PDH_STATUS>(PDH_INVALID_HANDLE):rc));reset();return;}
        rc=api_.add(query_,L"\\GPU Engine(*)\\Utilization Percentage",0,&counter_);
        if(rc!=ERROR_SUCCESS || !counter_){clear(missing("GPU engine counter unavailable",rc==ERROR_SUCCESS?static_cast<PDH_STATUS>(PDH_INVALID_HANDLE):rc));reset();return;}
    }
    const auto rc=api_.collect(query_);
    if(rc!=ERROR_SUCCESS){clear(missing("GPU engine collection failed",rc));reset();return;}
    if(!primed_){primed_=true;return;}
    const auto values=read(api_,counter_,identities_);
    if(values.failed){clear(values.error);reset();return;}
    for(auto& gpu:gpus) {
        const auto found=values.groups.find(gpu.id);
        if(found==values.groups.end()){gpu.utilization_percent=values.error;continue;}
        bool complete=true,permission=false;double maximum=0;
        for(const auto& [key,group]:found->second) {
            GpuEngine engine;engine.id="phys:"+std::to_string(key.first)+"/eng:"+std::to_string(key.second);engine.type=group.type;
            engine.name=(group.type.empty()?"Unknown":group.type)+" ["+engine.id+"]";engine.utilization_percent=group.metric;
            gpu.engines.push_back(std::move(engine));
            if(group.metric.value)maximum=std::max(maximum,*group.metric.value);
            else complete=false;
            permission|=group.metric.support==Support::permission_required;
        }
        gpu.utilization_percent=complete?Metric<double>{maximum,Support::estimated,"maximum of observed engine utilizations, not their sum/average; coverage estimated"}:
            Metric<double>{std::nullopt,permission?Support::permission_required:Support::unsupported,"at least one observed engine is unknown; overall utilization not certified"};
    }
}
}
#endif
