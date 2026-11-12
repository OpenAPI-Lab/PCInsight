#include "windows_gpu_devices.hpp"
#ifdef _WIN32
#include <cstddef>
#include <cstring>
#include <cstdio>
#include <map>
#include <algorithm>
namespace pcinsight::detail {
namespace {
static_assert(offsetof(GpuOpenDevice,handle)==sizeof(void*));
static_assert(offsetof(GpuOpenDevice,luid)==sizeof(void*)+4);
static_assert(sizeof(GpuCloseDevice)==4);
constexpr GUID display{0x1ca05180,0xa699,0x450a,{0x9a,0x0c,0xde,0x4f,0xbe,0x3d,0xdd,0x89}};
constexpr GUID pci{0xc8ebdfb0,0xb510,0x11d0,{0x80,0xe5,0x00,0xa0,0xc9,0x25,0x42,0xe3}};
constexpr GUID device{0xa45c254e,0xdf1c,0x4efd,{0x80,0x20,0x67,0xd1,0x46,0xa8,0x50,0xe0}};
constexpr DEVPROPKEY bus_type{device,21},bus_number{device,23},address{device,30};
constexpr DEVPROPKEY date{{0xa8b865dd,0x2e3d,0x4094,{0xad,0x97,0xe5,0x93,0xa7,0x0c,0x75,0xd6}},2};
using Text=Metric<std::string>;
Text missing(DWORD error=0){return {{},error==ERROR_ACCESS_DENIED?Support::permission_required:Support::unsupported,"PnP GPU metadata unavailable or identity not proven"};}
std::string identity(LUID luid){char text[40];std::snprintf(text,sizeof(text),"dxgi-luid:%08x:%08x",static_cast<unsigned>(luid.HighPart),static_cast<unsigned>(luid.LowPart));return text;}
struct Set {HDEVINFO value;GpuDeviceApi& api;~Set(){api.destroy(value);}};
struct Adapter {GpuOpenDevice value;GpuDeviceApi& api;~Adapter(){if(value.handle){GpuCloseDevice close{value.handle};api.close(&close);}}};
template<class T> DWORD property(GpuDeviceApi& api,HDEVINFO set,SP_DEVINFO_DATA& node,const DEVPROPKEY& key,DEVPROPTYPE expected,T& value){
    DEVPROPTYPE type{};DWORD size{};
    if(!api.property(set,&node,&key,&type,reinterpret_cast<BYTE*>(&value),sizeof(value),&size,0))return GetLastError();
    if(size!=sizeof(value) || type!=expected)return ERROR_INVALID_DATA;
    return ERROR_SUCCESS;
}
DWORD number(GpuDeviceApi& api,HDEVINFO set,SP_DEVINFO_DATA& node,const DEVPROPKEY& key,DWORD& value){
    DEVPROPTYPE type{};DWORD size{};
    if(!api.property(set,&node,&key,&type,reinterpret_cast<BYTE*>(&value),sizeof(value),&size,0))return GetLastError();
    if(size!=sizeof(value) || (type!=DEVPROP_TYPE_UINT32 && type!=DEVPROP_TYPE_INT32) || (type==DEVPROP_TYPE_INT32 && value>0x7fffffff))return ERROR_INVALID_DATA;
    return 0;
}
struct Metadata {DWORD node{};Text location,date;bool ambiguous{};};
Metadata read(GpuDeviceApi& api,HDEVINFO set,SP_DEVINFO_DATA& node){
    Metadata out{node.DevInst,missing(),missing(),false};
    FILETIME time{};DWORD error=property(api,set,node,date,DEVPROP_TYPE_FILETIME,time);
    SYSTEMTIME utc{};
    if(!error && FileTimeToSystemTime(&time,&utc) && utc.wYear<=9999){
        char text[16];std::snprintf(text,sizeof(text),"%04u-%02u-%02u",utc.wYear,utc.wMonth,utc.wDay);
        out.date={std::string(text),Support::available,"INF DriverVer date of installed driver (UTC), not installation date"};
    }else out.date=missing(error);
    GUID type{};DWORD bus{},slot{};
    error=property(api,set,node,bus_type,DEVPROP_TYPE_GUID,type);
    if(!error && IsEqualGUID(type,pci)){
        const DWORD b=number(api,set,node,bus_number,bus),a=number(api,set,node,address,slot);
        error=b==ERROR_ACCESS_DENIED || a==ERROR_ACCESS_DENIED?ERROR_ACCESS_DENIED:b?b:a;
        if(!error && (slot>>16)<=31 && (slot&65535)<=7){
            out.location={"PCI bus "+std::to_string(bus)+", device "+std::to_string(slot>>16)+", function "+std::to_string(slot&65535),Support::available,"PnP PCI location; bus number can change; PCI segment not reported"};
        }else out.location=missing(error);
    }else out.location=missing(error);
    return out;
}
}
void fill_windows_gpu_devices(std::vector<GpuInfo>& gpus,GpuDeviceApi api){
    for(auto& gpu:gpus){gpu.driver_date=missing();gpu.physical_location=missing();}
    if(gpus.empty() || !api.open || !api.close)return;
    auto fail=[&](DWORD error){for(auto& gpu:gpus){gpu.driver_date=missing(error);gpu.physical_location=missing(error);}};
    const HDEVINFO raw=api.list(&display,nullptr,nullptr,DIGCF_PRESENT|DIGCF_DEVICEINTERFACE);
    if(raw==INVALID_HANDLE_VALUE){fail(GetLastError());return;}
    Set set{raw,api};std::map<std::string,Metadata> staged;
    for(DWORD index=0;;++index){
        SP_DEVICE_INTERFACE_DATA iface{};iface.cbSize=sizeof(iface);
        if(!api.enumerate(raw,nullptr,&display,index,&iface)){
            const DWORD error=GetLastError();if(error==ERROR_NO_MORE_ITEMS)break;
            fail(error);return;
        }
        if(index>=256){fail(ERROR_INVALID_DATA);return;}
        DWORD needed{};
        if(api.detail(raw,&iface,nullptr,0,&needed,nullptr) || GetLastError()!=ERROR_INSUFFICIENT_BUFFER || needed<sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W) || needed>65536){fail(ERROR_INVALID_DATA);return;}
        // max_align_t backing keeps the native structure aligned.
        std::vector<std::max_align_t> storage((needed+sizeof(std::max_align_t)-1)/sizeof(std::max_align_t));
        auto* detail=reinterpret_cast<SP_DEVICE_INTERFACE_DETAIL_DATA_W*>(storage.data());detail->cbSize=sizeof(*detail);
        SP_DEVINFO_DATA node{};node.cbSize=sizeof(node);DWORD returned{};
        if(!api.detail(raw,&iface,detail,needed,&returned,&node)){fail(GetLastError());return;}
        constexpr auto offset=offsetof(SP_DEVICE_INTERFACE_DETAIL_DATA_W,DevicePath);
        if(returned>needed || returned<offset+sizeof(wchar_t) || (returned-offset)%sizeof(wchar_t) || node.cbSize!=sizeof(node)){fail(ERROR_INVALID_DATA);return;}
        const auto chars=(returned-offset)/sizeof(wchar_t);
        if(!detail->DevicePath[0] || std::find(detail->DevicePath,detail->DevicePath+chars,L'\0')==detail->DevicePath+chars){fail(ERROR_INVALID_DATA);return;}
        Adapter adapter{{detail->DevicePath,0,{}},api};
        const LONG status=api.open(&adapter.value);
        if(status!=0 || !adapter.value.handle){fail(static_cast<DWORD>(status)==0xc0000022?ERROR_ACCESS_DENIED:ERROR_INVALID_DATA);return;}
        const auto id=identity(adapter.value.luid);
        auto metadata=read(api,raw,node);
        Adapter checked{{detail->DevicePath,0,{}},api};
        const LONG check=api.open(&checked.value);
        if(check!=0 || !checked.value.handle || identity(checked.value.luid)!=id){fail(static_cast<DWORD>(check)==0xc0000022?ERROR_ACCESS_DENIED:ERROR_INVALID_DATA);return;}
        auto [it,inserted]=staged.emplace(id,metadata);
        if(!inserted && it->second.node!=node.DevInst)it->second.ambiguous=true;
    }
    for(auto& gpu:gpus){auto it=staged.find(gpu.id);if(it!=staged.end() && !it->second.ambiguous){gpu.driver_date=it->second.date;gpu.physical_location=it->second.location;}}
}
void fill_windows_gpu_devices(std::vector<GpuInfo>& gpus){
    struct Module {HMODULE value;~Module(){if(value)FreeLibrary(value);}} module{LoadLibraryExW(L"gdi32.dll",nullptr,LOAD_LIBRARY_SEARCH_SYSTEM32)};
    GpuDeviceApi api;
    if(module.value){
        auto open=GetProcAddress(module.value,"D3DKMTOpenAdapterFromDeviceName"),close=GetProcAddress(module.value,"D3DKMTCloseAdapter");
        static_assert(sizeof(open)==sizeof(api.open));std::memcpy(&api.open,&open,sizeof(open));std::memcpy(&api.close,&close,sizeof(close));
    }
    fill_windows_gpu_devices(gpus,api);
}
}
#endif
