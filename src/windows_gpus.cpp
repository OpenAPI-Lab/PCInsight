#include "windows_gpus.hpp"
#include "windows_gpu_d3d12.hpp"
#include "windows_gpu_budget.hpp"
#ifdef _WIN32
#include <cwchar>
#include <iomanip>
#include <set>
#include <sstream>
#include <stdexcept>
#include <system_error>
namespace pcinsight::detail {
namespace {
HRESULT create(void** out) {
    IDXGIFactory1* factory=nullptr;
    const auto result=CreateDXGIFactory1(__uuidof(IDXGIFactory1),reinterpret_cast<void**>(&factory));
    *out=factory;return result;
}
HRESULT enumerate(void* factory,UINT index,void** out) {
    IDXGIAdapter1* adapter=nullptr;
    const auto result=static_cast<IDXGIFactory1*>(factory)->EnumAdapters1(index,&adapter);
    *out=adapter;return result;
}
HRESULT describe(void* adapter,DXGI_ADAPTER_DESC1* out) {return static_cast<IDXGIAdapter1*>(adapter)->GetDesc1(out);}
HRESULT driver(void* adapter,LARGE_INTEGER* out) {return static_cast<IDXGIAdapter1*>(adapter)->CheckInterfaceSupport(__uuidof(IDXGIDevice),out);}
void release_factory(void* factory) {static_cast<IDXGIFactory1*>(factory)->Release();}
void release_adapter(void* adapter) {static_cast<IDXGIAdapter1*>(adapter)->Release();}
struct Reference {
    void* handle{};void (*release)(void*);
    ~Reference(){if(handle)release(handle);}
    Reference(const Reference&)=delete;
    Reference(void (*operation)(void*)):release(operation){}
};
[[noreturn]] void failure(const char* message,HRESULT code) {throw std::system_error(static_cast<int>(code),std::system_category(),message);}
std::string description(const wchar_t (&text)[128]) {
    const auto end=std::wmemchr(text,L'\0',128);
    if(!end)failure("unterminated DXGI description",E_UNEXPECTED);
    const auto length=static_cast<int>(end-text);
    if(!length)return {};
    const auto size=WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,text,length,nullptr,0,nullptr,nullptr);
    if(!size)failure("invalid DXGI description Unicode",E_UNEXPECTED);
    std::string result(size,'\0');
    if(!WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,text,length,result.data(),size,nullptr,nullptr))
        failure("DXGI description conversion failed",E_UNEXPECTED);
    return result;
}
std::string vendor(UINT id) {
    switch(id){case 0:return {};case 0x1002:return "AMD";case 0x10de:return "NVIDIA";case 0x8086:return "Intel";default:break;}
    std::ostringstream text;text<<(id<=0xffff?"PCI vendor 0x":"ACPI vendor 0x")<<std::hex<<id;return text.str();
}
std::string identity(const LUID& luid) {
    std::ostringstream text;text<<"dxgi-luid:"<<std::hex<<std::setfill('0')
        <<std::setw(8)<<static_cast<std::uint32_t>(luid.HighPart)<<':'<<std::setw(8)<<luid.LowPart;
    return text.str();
}
}
GpuInventoryApi native_gpu_inventory_api() {return {create,enumerate,describe,driver,release_factory,release_adapter,probe_windows_gpu_d3d12,fill_windows_gpu_budget};}
std::vector<GpuInfo> read_windows_gpus(GpuInventoryApi api) {
    if(!api.create || !api.enumerate || !api.describe || !api.driver_version || !api.release_factory || !api.release_adapter)
        throw std::invalid_argument("incomplete GPU inventory API");
    Reference factory(api.release_factory);const auto opened=api.create(&factory.handle);
    if(opened!=S_OK)failure("CreateDXGIFactory1",opened);
    if(!factory.handle)failure("DXGI success with null factory",E_UNEXPECTED);
    std::vector<GpuInfo> result;std::set<std::string> identities;
    for(UINT index=0;index<=256;++index) {
        Reference adapter(api.release_adapter);const auto status=api.enumerate(factory.handle,index,&adapter.handle);
        if(status==DXGI_ERROR_NOT_FOUND)return result;
        if(status!=S_OK)failure("DXGI adapter enumeration",status);
        if(!adapter.handle)failure("DXGI success with null adapter",E_UNEXPECTED);
        if(index==256)failure("DXGI adapter count limit",HRESULT_FROM_WIN32(ERROR_MORE_DATA));
        DXGI_ADAPTER_DESC1 desc{};const auto described=api.describe(adapter.handle,&desc);
        if(described!=S_OK)failure("DXGI adapter description",described);
        const auto id=identity(desc.AdapterLuid);
        if(!identities.insert(id).second)failure("duplicate DXGI adapter LUID",E_UNEXPECTED);
        if(desc.Flags&DXGI_ADAPTER_FLAG_SOFTWARE)continue;
        GpuInfo gpu;gpu.id=id;gpu.vendor=vendor(desc.VendorId);gpu.model=description(desc.Description);
        if(api.d3d12_level)gpu.direct3d12_feature_level=api.d3d12_level(adapter.handle);
        if(api.process_memory)api.process_memory(adapter.handle,gpu);
        gpu.hardware_vendor_id={desc.VendorId,Support::available,"DXGI PCI/ACPI vendor ID; zero can mean unspecified"};
        gpu.hardware_device_id={desc.DeviceId,Support::available,"DXGI PCI/ACPI device ID; not PCI bus/device/function"};
        gpu.dedicated_memory_bytes={static_cast<std::uint64_t>(desc.DedicatedVideoMemory),Support::available,"DXGI static dedicated video capacity, not bytes currently used"};
        gpu.shared_memory_bytes={static_cast<std::uint64_t>(desc.SharedSystemMemory),Support::available,"DXGI maximum shared system memory capacity, not current usage/free memory"};
        gpu.dedicated_system_memory_bytes={static_cast<std::uint64_t>(desc.DedicatedSystemMemory),Support::available,"DXGI dedicated system memory reserved at boot; separate from dedicated video memory"};
        LARGE_INTEGER version{};const auto checked=api.driver_version(adapter.handle,&version);
        if(checked==S_OK) {
            const auto bits=static_cast<std::uint64_t>(version.QuadPart);
            gpu.driver_version={std::to_string((bits>>48)&0xffff)+'.'+std::to_string((bits>>32)&0xffff)+'.'+
                std::to_string((bits>>16)&0xffff)+'.'+std::to_string(bits&0xffff),Support::available,"DXGI IDXGIDevice user-mode driver version; not marketing version or date"};
        } else gpu.driver_version={std::nullopt,(checked==E_ACCESSDENIED || checked==HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED))?Support::permission_required:Support::unsupported,"DXGI user-mode driver version query failed/unsupported"};
        gpu.memory_used_bytes.note=gpu.memory_available_bytes.note="GPU-global live memory accounting not provided by static DXGI inventory";
        gpu.utilization_percent.note="GPU engine utilization provider pending";
        gpu.temperature_celsius.note="DXGI does not expose GPU temperature";
        gpu.physical_location.note="LUID is not a physical PCI location; location provider pending";
        gpu.driver_date.note="DXGI user-mode version query does not provide driver date";
        gpu.vulkan_version.note=gpu.opencl_version.note=gpu.opengl_version.note="graphics runtime provider not queried by DXGI inventory";
        result.push_back(std::move(gpu));
    }
    failure("DXGI adapter enumeration did not terminate",E_UNEXPECTED);
}
}
#endif
