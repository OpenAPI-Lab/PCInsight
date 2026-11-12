#include "windows_disk_media.hpp"
#ifdef _WIN32
#include <wbemidl.h>
#include <charconv>
#include <limits>
#include <set>
#include <algorithm>
namespace pcinsight::detail {
namespace {
template<class T> struct Com {
    T* value=nullptr;
    ~Com() { if(value) value->Release(); }
    Com()=default; Com(const Com&)=delete;
};
struct Apartment {
    HRESULT status=CoInitializeEx(nullptr,COINIT_MULTITHREADED);
    ~Apartment() { if(SUCCEEDED(status)) CoUninitialize(); }
};
struct Bstr {
    BSTR value;
    explicit Bstr(const wchar_t* text) : value(SysAllocString(text)) {}
    ~Bstr() { SysFreeString(value); }
    Bstr(const Bstr&)=delete;
};
struct Value {
    VARIANT value{};
    ~Value() { VariantClear(&value); }
};
DiskMediaInventory failed(const char* operation,HRESULT hr) {
    return {{},hr==E_ACCESSDENIED||static_cast<DWORD>(hr)==WBEM_E_ACCESS_DENIED ? Support::permission_required:Support::unsupported,
        std::string(operation)+"; HRESULT="+std::to_string(static_cast<unsigned long>(hr))};
}
std::optional<std::string> ascii(BSTR value) {
    if(!value) return std::string{};
    const auto length=SysStringLen(value);
    if(length>4096) return std::nullopt;
    std::string result;
    for(UINT i=0;i<length;++i) {
        if(!value[i] || value[i]>127) return std::nullopt;
        result.push_back(static_cast<char>(value[i]));
    }
    const auto first=result.find_first_not_of(' ');
    if(first==std::string::npos) return std::string{};
    return result.substr(first,result.find_last_not_of(' ')-first+1);
}
std::optional<std::uint64_t> integer(const VARIANT& value) {
    switch(value.vt) {
        case VT_UI1: return value.bVal;
        case VT_UI2: return value.uiVal;
        case VT_UI4: return value.ulVal;
        case VT_UI8: return value.ullVal;
        case VT_I2: if(value.iVal>=0) return static_cast<std::uint64_t>(value.iVal); break;
        case VT_I4: if(value.lVal>=0) return static_cast<std::uint64_t>(value.lVal); break;
        case VT_I8: if(value.llVal>=0) return static_cast<std::uint64_t>(value.llVal); break;
        case VT_BSTR: {
            auto text=ascii(value.bstrVal); if(!text||text->empty()) break;
            std::uint64_t number{};
            const auto parsed=std::from_chars(text->data(),text->data()+text->size(),number);
            if(parsed.ec==std::errc{}&&parsed.ptr==text->data()+text->size()) return number;
            break;
        }
    }
    return std::nullopt;
}
std::string media_name(unsigned type) {
    switch(type) { case 3:return "HDD"; case 4:return "SSD"; case 5:return "SCM"; default:return {}; }
}
}
std::optional<DiskMediaRow> decode_windows_media_row(const VARIANT& serial,const VARIANT& size,
                                                   const VARIANT& bus,const VARIANT& media) {
    auto s=serial.vt==VT_BSTR?ascii(serial.bstrVal):
        (serial.vt==VT_NULL?std::optional<std::string>(std::string{}):std::nullopt);
    auto capacity=integer(size),b=integer(bus),m=integer(media);
    if(!s||!capacity||!b||!m||*b>std::numeric_limits<unsigned>::max()||*m>65535) return std::nullopt;
    return DiskMediaRow{{*s,*capacity,static_cast<unsigned>(*b)},static_cast<unsigned>(*m)};
}
DiskMediaInventory read_windows_disk_media() {
    Apartment apartment;
    // Reuse an existing STA owned by the caller, without uninitializing it.
    if(FAILED(apartment.status)&&apartment.status!=RPC_E_CHANGED_MODE) return failed("CoInitializeEx disk media",apartment.status);
    Com<IWbemLocator> locator; Com<IWbemServices> services; Com<IEnumWbemClassObject> enumeration;
    auto hr=CoCreateInstance(CLSID_WbemLocator,nullptr,CLSCTX_INPROC_SERVER,IID_IWbemLocator,
                             reinterpret_cast<void**>(&locator.value));
    if(FAILED(hr)) return failed("Create WMI locator",hr);
    Bstr space(L"ROOT\\Microsoft\\Windows\\Storage"),language(L"WQL"),
        query(L"SELECT SerialNumber, Size, BusType, MediaType FROM MSFT_PhysicalDisk");
    if(!space.value||!language.value||!query.value) return failed("Allocate WMI strings",E_OUTOFMEMORY);
    hr=locator.value->ConnectServer(space.value,nullptr,nullptr,nullptr,WBEM_FLAG_CONNECT_USE_MAX_WAIT,nullptr,nullptr,&services.value);
    if(FAILED(hr)) return failed("Connect local storage WMI",hr);
    // Proxy-local security only: do not change host-wide COM security defaults.
    hr=CoSetProxyBlanket(services.value,RPC_C_AUTHN_WINNT,RPC_C_AUTHZ_NONE,nullptr,
                        RPC_C_AUTHN_LEVEL_PKT_PRIVACY,RPC_C_IMP_LEVEL_IMPERSONATE,nullptr,EOAC_NONE);
    if(FAILED(hr)) return failed("Storage proxy security",hr);
    hr=services.value->ExecQuery(language.value,query.value,WBEM_FLAG_FORWARD_ONLY|WBEM_FLAG_RETURN_IMMEDIATELY,
                                nullptr,&enumeration.value);
    if(FAILED(hr)) return failed("Query physical media",hr);
    hr=CoSetProxyBlanket(enumeration.value,RPC_C_AUTHN_WINNT,RPC_C_AUTHZ_NONE,nullptr,
                        RPC_C_AUTHN_LEVEL_PKT_PRIVACY,RPC_C_IMP_LEVEL_IMPERSONATE,nullptr,EOAC_NONE);
    if(FAILED(hr)) return failed("Storage enumerator security",hr);
    DiskMediaInventory result; result.support=Support::available;
    result.note="MSFT_PhysicalDisk.MediaType matched by unique serial/size/bus; OS/provider-reported, not independently verified";
    const auto began=std::chrono::steady_clock::now();
    for(unsigned i=0;i<4096;++i) {
        if(std::chrono::steady_clock::now()-began>std::chrono::seconds(5)) return failed("Storage enumeration deadline",WBEM_S_TIMEDOUT);
        Com<IWbemClassObject> object; ULONG count=0;
        hr=enumeration.value->Next(1000,1,&object.value,&count);
        if(FAILED(hr)) return failed("Storage enumeration",hr);
        if(hr==WBEM_S_FALSE&&!count) return result;
        if(hr==WBEM_S_TIMEDOUT&&!count) continue;
        if(count!=1 || !object.value) return failed("Incomplete storage enumeration",hr);
        Value serial,size,bus,media;
        const wchar_t* names[]{L"SerialNumber",L"Size",L"BusType",L"MediaType"};
        VARIANT* properties[]{&serial.value,&size.value,&bus.value,&media.value};
        for(unsigned property=0;property<4;++property) {
            hr=object.value->Get(names[property],0,properties[property],nullptr,nullptr);
            if(FAILED(hr)) return failed("Storage row property",hr);
        }
        auto row=decode_windows_media_row(serial.value,size.value,bus.value,media.value);
        if(!row)
            return failed("Invalid storage row types",static_cast<HRESULT>(WBEM_E_INVALID_OBJECT));
        result.rows.push_back(std::move(*row));
    }
    return failed("Storage row limit",static_cast<HRESULT>(WBEM_E_QUOTA_VIOLATION));
}
void apply_windows_disk_media(std::vector<DiskInfo>& disks,const std::map<std::string,DiskHardwareIdentity>& native,
                             const DiskMediaInventory& inventory) {
    for(auto& disk:disks) {
        if(disk.media_type=="HDD"||disk.media_type=="SSD"||disk.media_type=="SCM") disk.media_type.clear();
        disk.reported_media_type={std::nullopt,inventory.support==Support::permission_required?Support::permission_required:Support::unsupported,
            inventory.support==Support::available?"No unique serial/size/bus match or medium unspecified":inventory.note};
        if(inventory.support!=Support::available) continue;
        const auto identity=native.find(disk.id);
        if(identity==native.end()||identity->second.serial.empty()||!identity->second.capacity||!identity->second.bus) continue;
        unsigned native_matches=0,provider_matches=0,type=0;
        for(const auto& [id,key]:native) { (void)id; if(key==identity->second) ++native_matches; }
        for(const auto& row:inventory.rows) if(row.identity==identity->second) { ++provider_matches; type=row.media_type; }
        auto label=media_name(type);
        if(native_matches==1&&provider_matches==1&&!label.empty()) {
            disk.reported_media_type={label,Support::available,inventory.note}; disk.media_type=label;
        }
    }
}
void WindowsDiskMedia::fill(std::vector<DiskInfo>& disks,const std::map<std::string,DiskHardwareIdentity>& identities,
                          std::chrono::steady_clock::time_point now) {
    if(std::none_of(identities.begin(),identities.end(),[](const auto& entry) {
        return !entry.second.serial.empty()&&entry.second.capacity&&entry.second.bus;
    })) { reset(); apply_windows_disk_media(disks,identities,{}); return; }
    if(!primed_||identities!=identities_||now<captured_||now-captured_>=std::chrono::seconds(30)) {
        auto inventory=provider_(); // Commit only after provider finishes.
        inventory_=std::move(inventory); identities_=identities; captured_=now; primed_=true;
    }
    apply_windows_disk_media(disks,identities,inventory_);
}
}
#endif
