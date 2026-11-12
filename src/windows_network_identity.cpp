#include "windows_network_identity.hpp"
#ifdef _WIN32
#include <algorithm>
#include <charconv>
#include <cstring>
#include <objbase.h>
namespace pcinsight::detail {
namespace {
constexpr GUID network_class{0x4d36e972,0xe325,0x11ce,{0xbf,0xc1,0x08,0x00,0x2b,0xe1,0x03,0x18}};
template<class T> Metric<T> missing(const char* operation, DWORD code = ERROR_NOT_FOUND) {
    return {std::nullopt, code == ERROR_ACCESS_DENIED ? Support::permission_required : Support::unsupported,
        std::string(operation) + "; Win32=" + std::to_string(code)};
}
struct Set { HDEVINFO value; NetworkIdentityApi& api; ~Set() { api.destroy(value); } };
struct Key { HKEY value; NetworkIdentityApi& api; ~Key() { if (value != INVALID_HANDLE_VALUE) api.close(value); } };
// A GUID association is read from the driver's existing software key; no key
// is created and no driver-supplied name/path is used to open a registry key.
DWORD instance_guid(HKEY key, GUID& result, NetworkIdentityApi& api) {
    wchar_t text[40]; std::fill(std::begin(text),std::end(text),static_cast<wchar_t>(0xffff));
    DWORD size = sizeof text, type{};
    const auto status = api.value(key,L"NetCfgInstanceId",nullptr,&type,reinterpret_cast<BYTE*>(text),&size);
    if (status != ERROR_SUCCESS) return status;
    if (type != REG_SZ || size != 39 * sizeof(wchar_t) || text[38] || text[0] != L'{' || text[37] != L'}') return ERROR_INVALID_DATA;
    for (unsigned i=1; i<37; ++i) {
        if (i==9 || i==14 || i==19 || i==24) { if(text[i]!=L'-') return ERROR_INVALID_DATA; }
        else if (!((text[i]>=L'0'&&text[i]<=L'9')||(text[i]>=L'a'&&text[i]<=L'f')||(text[i]>=L'A'&&text[i]<=L'F'))) return ERROR_INVALID_DATA;
    }
    return SUCCEEDED(IIDFromString(text,&result)) ? ERROR_SUCCESS : ERROR_INVALID_DATA;
}
Metric<std::string> property(HDEVINFO set, SP_DEVINFO_DATA& node, DWORD requested, NetworkIdentityApi& api) {
    wchar_t text[2048]; std::fill(std::begin(text),std::end(text),static_cast<wchar_t>(0xffff));
    DWORD size{}, type{};
    if (!api.property(set,&node,requested,&type,reinterpret_cast<BYTE*>(text),sizeof text,&size))
        return missing<std::string>("PnP property",GetLastError());
    if (type != REG_SZ || size < sizeof(wchar_t) || size > sizeof text || size % sizeof(wchar_t))
        return missing<std::string>("invalid PnP property size/type",ERROR_INVALID_DATA);
    const auto count = size / sizeof(wchar_t);
    if (text[count-1] || std::find(text,text+count-1,L'\0')!=text+count-1)
        return missing<std::string>("invalid PnP string termination",ERROR_INVALID_DATA);
    if (count==1) return {std::string{},Support::available,"PnP/INF-reported empty string"};
    const auto bytes = WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,text,static_cast<int>(count-1),nullptr,0,nullptr,nullptr);
    if (!bytes) return missing<std::string>("invalid PnP UTF-16",ERROR_INVALID_DATA);
    std::string result(bytes,'\0');
    if (!WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,text,static_cast<int>(count-1),result.data(),bytes,nullptr,nullptr))
        return missing<std::string>("PnP UTF-8 conversion",GetLastError());
    return {std::move(result),Support::available,"PnP/INF-reported, not independently verified hardware identity"};
}
}
Metric<WindowsNetworkIdentity> query_windows_network_identity_impl(const std::string& id, NetworkIdentityApi api) {
    std::uint64_t luid{};
    if (!id.starts_with("luid:") || id.size() <= 5 || id.size() > 25)
        return missing<WindowsNetworkIdentity>("invalid adapter id",ERROR_INVALID_PARAMETER);
    const auto parsed = std::from_chars(id.data()+5,id.data()+id.size(),luid);
    if (parsed.ec!=std::errc{} || parsed.ptr!=id.data()+id.size() || !luid || id[5]=='0')
        return missing<WindowsNetworkIdentity>("invalid adapter LUID",ERROR_INVALID_PARAMETER);
    MIB_IF_ROW2 before{}; before.InterfaceLuid.Value=luid;
    auto error = api.row(&before);
    if (error) return missing<WindowsNetworkIdentity>("GetIfEntry2",error);
    const GUID zero{};
    if (before.InterfaceLuid.Value!=luid || IsEqualGUID(before.InterfaceGuid,zero))
        return missing<WindowsNetworkIdentity>("invalid interface identity",ERROR_INVALID_DATA);
    wchar_t guid_text[40]{};
    if (StringFromGUID2(before.InterfaceGuid,guid_text,40)!=39)
        return missing<WindowsNetworkIdentity>("format GUID",ERROR_INVALID_DATA);
    WindowsNetworkIdentity out;
    out.adapter_id=id;
    // StringFromGUID2 produces only the ASCII GUID grammar; preserve it without
    // an implicit wchar_t-to-char range conversion.
    for (unsigned i=0; i<38; ++i) out.interface_guid.push_back(static_cast<char>(guid_text[i]));
    out.manufacturer=out.device_description=missing<std::string>("no unique PnP device match");
    const auto raw=api.list(&network_class,nullptr,nullptr,DIGCF_PRESENT);
    if (raw==INVALID_HANDLE_VALUE) return missing<WindowsNetworkIdentity>("SetupDiGetClassDevs",GetLastError());
    Set set{raw,api};
    unsigned matches{};
    for (DWORD index=0;;++index) {
        SP_DEVINFO_DATA node{}; node.cbSize=sizeof node;
        if (!api.enumerate(raw,index,&node)) {
            error=GetLastError();
            if (error==ERROR_NO_MORE_ITEMS) break;
            return missing<WindowsNetworkIdentity>("SetupDiEnumDeviceInfo",error);
        }
        if(index>=4096 || node.cbSize!=sizeof node)
            return missing<WindowsNetworkIdentity>("PnP inventory limit/record",ERROR_INVALID_DATA);
        Key key{api.key(raw,&node,DICS_FLAG_GLOBAL,0,DIREG_DRV,KEY_QUERY_VALUE),api};
        if (key.value==INVALID_HANDLE_VALUE) {
            error=GetLastError();
            if(error==ERROR_FILE_NOT_FOUND || error==ERROR_KEY_DOES_NOT_EXIST) continue;
            return missing<WindowsNetworkIdentity>("SetupDiOpenDevRegKey",error);
        }
        GUID association{};
        error=instance_guid(key.value,association,api);
        if(error==ERROR_FILE_NOT_FOUND) continue; // Device without an interface association.
        if(error) return missing<WindowsNetworkIdentity>("NetCfgInstanceId",error);
        if(!IsEqualGUID(association,before.InterfaceGuid)) continue;
        if(++matches>1) return missing<WindowsNetworkIdentity>("ambiguous PnP GUID association",ERROR_DUP_NAME);
        out.manufacturer=property(raw,node,SPDRP_MFG,api);
        out.device_description=property(raw,node,SPDRP_DEVICEDESC,api);
        GUID after{};
        // Reopen through the same PnP node as well as re-read the pinned key.
        Key checked{api.key(raw,&node,DICS_FLAG_GLOBAL,0,DIREG_DRV,KEY_QUERY_VALUE),api};
        if(checked.value==INVALID_HANDLE_VALUE) return missing<WindowsNetworkIdentity>("reopen PnP key",GetLastError());
        error=instance_guid(checked.value,after,api);
        if(error || !IsEqualGUID(association,after))
            return missing<WindowsNetworkIdentity>("PnP identity changed",error?error:ERROR_INVALID_DATA);
        GUID pinned{}; error=instance_guid(key.value,pinned,api);
        if(error || !IsEqualGUID(pinned,association))
            return missing<WindowsNetworkIdentity>("pinned PnP identity changed",error?error:ERROR_INVALID_DATA);
    }
    MIB_IF_ROW2 after{}; after.InterfaceLuid.Value=luid;
    error=api.row(&after);
    if(error) return missing<WindowsNetworkIdentity>("revalidate GetIfEntry2",error);
    if(after.InterfaceLuid.Value!=luid || !IsEqualGUID(before.InterfaceGuid,after.InterfaceGuid) || after.Type!=before.Type)
        return missing<WindowsNetworkIdentity>("interface identity changed",ERROR_INVALID_DATA);
    return {std::move(out),Support::available,"PnP association observed before/after; not an atomic snapshot or action token"};
}
}
#endif
namespace pcinsight {
Metric<WindowsNetworkIdentity> query_windows_network_identity(const std::string& id) {
#ifdef _WIN32
    return detail::query_windows_network_identity_impl(id);
#else
    (void)id;
    return {std::nullopt,Support::unsupported,"Windows IP Helper/PnP is unavailable on this platform"};
#endif
}
}
