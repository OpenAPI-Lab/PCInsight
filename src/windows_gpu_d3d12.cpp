#include "windows_gpu_d3d12.hpp"
#ifdef _WIN32
#include <cstring>
namespace pcinsight::detail {
namespace {
Metric<std::string> absent(HRESULT result){return {{},result==E_ACCESSDENIED || result==HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED)?Support::permission_required:Support::unsupported,"Direct3D 12 feature-level validation unavailable/failed; not an installed DirectX version or proof of absent hardware support"};}
}
Metric<std::string> probe_windows_gpu_d3d12(void* adapter,PFN_D3D12_CREATE_DEVICE create){
    if(!adapter || !create)return absent(E_NOINTERFACE); // never select default adapter
    struct Level {D3D_FEATURE_LEVEL value;const char* text;};
    constexpr Level levels[]={{D3D_FEATURE_LEVEL_12_2,"12_2"},{D3D_FEATURE_LEVEL_12_1,"12_1"},{D3D_FEATURE_LEVEL_12_0,"12_0"},{D3D_FEATURE_LEVEL_11_1,"11_1"},{D3D_FEATURE_LEVEL_11_0,"11_0"}};
    bool unrecognized=false;
    for(const auto& level:levels){
        const HRESULT status=create(static_cast<IUnknown*>(static_cast<IDXGIAdapter1*>(adapter)),level.value,__uuidof(ID3D12Device),nullptr);
        if(status==S_FALSE)return {std::string(level.text),unrecognized?Support::estimated:Support::available,
            unrecognized?"Highest validated D3D12 candidate; higher candidate unrecognized by runtime (lower bound), not installed DirectX version":"Highest D3D12 level validated among known 11_0..12_2 candidates; not installed DirectX version"};
        if(status==DXGI_ERROR_UNSUPPORTED)continue;
        if(status==E_INVALIDARG){unrecognized=true;continue;}
        // Do not mask permission/device-removed/transient failures with a lower level.
        return absent(status);
    }
    return absent(DXGI_ERROR_UNSUPPORTED);
}
Metric<std::string> probe_windows_gpu_d3d12(void* adapter){
    struct Module {HMODULE value;~Module(){if(value)FreeLibrary(value);}} module{LoadLibraryExW(L"d3d12.dll",nullptr,LOAD_LIBRARY_SEARCH_SYSTEM32)};
    if(!module.value)return absent(HRESULT_FROM_WIN32(GetLastError()));
    const auto proc=GetProcAddress(module.value,"D3D12CreateDevice");
    PFN_D3D12_CREATE_DEVICE create{};static_assert(sizeof(proc)==sizeof(create));std::memcpy(&create,&proc,sizeof(create));
    return probe_windows_gpu_d3d12(adapter,create);
}
}
#endif
