#include "windows_volumes.hpp"
#ifdef _WIN32
#include <array>
#include <cstring>
#include <cwctype>
#include <limits>
#include <optional>
#include <set>
namespace pcinsight::detail {
namespace {
constexpr std::size_t name_limit=32768;
DWORD last_error() { const auto error=GetLastError(); return error?error:ERROR_INVALID_DATA; }
struct Handle {
    HANDLE value; VolumeApi& api;
    ~Handle() { if(value!=INVALID_HANDLE_VALUE) api.close(value); }
};
struct Enumeration {
    HANDLE value; VolumeApi& api;
    ~Enumeration() { if(value!=INVALID_HANDLE_VALUE) api.finish(value); }
};
std::optional<std::wstring> text(const wchar_t* pointer,std::size_t maximum=name_limit) {
    if(!pointer) return std::nullopt;
    std::size_t n=0;
    while(n<maximum && pointer[n]) ++n;
    if(!n || n==maximum) return std::nullopt;
    return std::wstring(pointer,n);
}
std::wstring canonical(std::wstring value) {
    for(auto& ch:value) ch=static_cast<wchar_t>(std::towlower(ch));
    return value;
}
std::optional<std::wstring> volume_name(const std::wstring& path,VolumeApi& api) {
    std::array<wchar_t,name_limit> root{},name{};
    if(!api.root(path.c_str(),root.data(),static_cast<DWORD>(root.size())) || !text(root.data())) return std::nullopt;
    if(!api.name(root.data(),name.data(),static_cast<DWORD>(name.size()))) return std::nullopt;
    auto result=text(name.data());
    if(!result) return std::nullopt;
    return canonical(*result);
}
struct Extents {
    std::set<std::uint32_t> disks;
    DWORD error=ERROR_SUCCESS;
    std::optional<std::uint64_t> ntfs_capacity{};
};
Extents extents(std::wstring volume,VolumeApi& api) {
    if(volume.empty() || volume.back()!=L'\\') return {{},ERROR_INVALID_DATA};
    volume.pop_back();
    Handle handle{api.open(volume.c_str(),0,FILE_SHARE_READ|FILE_SHARE_WRITE,nullptr,OPEN_EXISTING,0,nullptr),api};
    if(handle.value==INVALID_HANDLE_VALUE) return {{},last_error()};
    std::vector<unsigned char> buffer(65536); DWORD returned=0;
    if(!api.query(handle.value,IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS,nullptr,0,buffer.data(),
                  static_cast<DWORD>(buffer.size()),&returned,nullptr)) return {{},last_error()};
    constexpr auto offset=offsetof(VOLUME_DISK_EXTENTS,Extents);
    if(returned<offset || returned>buffer.size()) return {{},ERROR_INVALID_DATA};
    DWORD count=0; std::memcpy(&count,buffer.data(),sizeof count);
    if(!count || count>2048 || count>(returned-offset)/sizeof(DISK_EXTENT)) return {{},ERROR_INVALID_DATA};
    Extents result;
    for(DWORD i=0;i<count;++i) {
        DISK_EXTENT entry{}; std::memcpy(&entry,buffer.data()+offset+i*sizeof entry,sizeof entry);
        if(entry.DiskNumber==0xffffffffu || entry.StartingOffset.QuadPart<0 || entry.ExtentLength.QuadPart<=0)
            return {{},ERROR_INVALID_DATA};
        result.disks.insert(entry.DiskNumber);
    }
    NTFS_VOLUME_DATA_BUFFER ntfs{}; returned=0;
    if(api.query(handle.value,FSCTL_GET_NTFS_VOLUME_DATA,nullptr,0,&ntfs,sizeof ntfs,&returned,nullptr) &&
        returned==sizeof ntfs && ntfs.TotalClusters.QuadPart>0 && ntfs.BytesPerCluster &&
        static_cast<std::uint64_t>(ntfs.TotalClusters.QuadPart)<=std::numeric_limits<std::uint64_t>::max()/ntfs.BytesPerCluster)
        result.ntfs_capacity=static_cast<std::uint64_t>(ntfs.TotalClusters.QuadPart)*ntfs.BytesPerCluster;
    return result;
}
struct Pages { std::vector<std::wstring> paths; bool failed=false; };
BOOL CALLBACK page_callback(LPVOID opaque,PENUM_PAGE_FILE_INFORMATION info,LPCWSTR filename) {
    auto& state=*static_cast<Pages*>(opaque);
    try {
        auto path=text(filename);
        if(!info || !path || state.paths.size()>=1024) { state.failed=true; return FALSE; }
        // Normalize the DOS-device prefix sometimes returned by PSAPI.
        if(path->rfind(L"\\??\\",0)==0) *path=L"\\\\?\\"+path->substr(4);
        state.paths.push_back(std::move(*path)); return TRUE;
    } catch(...) { state.failed=true; return FALSE; } // Never unwind through a Win32 callback.
}
Metric<bool> flag(bool value,const char* note) { return {value,Support::available,note}; }
Metric<bool> unknown(const char* note,DWORD error=ERROR_INVALID_DATA) {
    return {std::nullopt,error==ERROR_ACCESS_DENIED?Support::permission_required:Support::unsupported,
        std::string(note)+"; Win32 "+std::to_string(error)};
}
}
void fill_windows_volumes(std::vector<DiskInfo>& disks,const std::map<std::uint32_t,std::string>& numbers,VolumeApi api) {
    std::map<std::string,std::uint32_t> reverse;
    for(const auto& [number,id]:numbers) reverse.emplace(id,number);
    std::map<std::wstring,Extents> cache;
    auto resolve=[&](const std::wstring& name)->const Extents& {
        const auto key=canonical(name);
        auto it=cache.find(key);
        if(it==cache.end()) it=cache.emplace(key,extents(name,api)).first;
        return it->second;
    };
    // Independent system/page-file queries: failures cannot fabricate false flags.
    std::set<std::uint32_t> system_disks,page_disks;
    DWORD system_error=ERROR_INVALID_DATA,page_error=ERROR_INVALID_DATA;
    std::array<wchar_t,name_limit> windows{};
    const auto length=api.windows(windows.data(),static_cast<UINT>(windows.size()));
    if(length && length<windows.size()) {
        auto path=text(windows.data());
        auto name=path?volume_name(*path,api):std::nullopt;
        if(name) { const auto& mapped=resolve(*name); system_disks=mapped.disks; system_error=mapped.error; }
        else system_error=last_error();
    } else system_error=last_error();
    Pages pages;
    const bool enumerated=api.pages(page_callback,&pages)!=FALSE;
    page_error=enumerated&&!pages.failed?ERROR_SUCCESS:(pages.failed?ERROR_INVALID_DATA:last_error());
    for(const auto& path:pages.paths) {
        auto name=volume_name(path,api);
        if(!name) { page_error=GetLastError(); if(!page_error)page_error=ERROR_INVALID_DATA; continue; }
        const auto& mapped=resolve(*name);
        if(mapped.error) { page_error=mapped.error; continue; }
        page_disks.insert(mapped.disks.begin(),mapped.disks.end());
    }
    std::map<std::uint32_t,std::uint64_t> capacities;
    std::set<std::uint32_t> approximate;
    std::map<std::uint32_t,DWORD> unavailable;
    DWORD inventory_error=ERROR_SUCCESS;
    std::array<wchar_t,1024> volume{};
    Enumeration enumeration{api.first(volume.data(),static_cast<DWORD>(volume.size())),api};
    if(enumeration.value==INVALID_HANDLE_VALUE) {
        inventory_error=last_error(); if(inventory_error==ERROR_NO_MORE_FILES) inventory_error=ERROR_SUCCESS;
    } else {
        std::set<std::wstring> seen; unsigned count=0;
        for(;;) {
            auto name=text(volume.data(),volume.size());
            if(!name || ++count>4096 || !seen.insert(canonical(*name)).second) { inventory_error=ERROR_INVALID_DATA; break; }
            const auto& mapped=resolve(*name);
            if(mapped.error) { inventory_error=mapped.error; break; }
            if(mapped.disks.size()!=1) {
                for(const auto number:mapped.disks) unavailable[number]=ERROR_NOT_SUPPORTED;
            } else {
                const auto number=*mapped.disks.begin(); ULARGE_INTEGER total{};
                bool known=true;
                if(mapped.ntfs_capacity) total.QuadPart=*mapped.ntfs_capacity;
                else {
                    approximate.insert(number);
                    known=api.space(name->c_str(),nullptr,&total,nullptr)!=FALSE;
                }
                if(!known) unavailable[number]=last_error();
                else if(!total.QuadPart || total.QuadPart>std::numeric_limits<std::uint64_t>::max()-capacities[number])
                    unavailable[number]=ERROR_INVALID_DATA;
                else capacities[number]+=total.QuadPart;
            }
            volume={};
            if(!api.next(enumeration.value,volume.data(),static_cast<DWORD>(volume.size()))) {
                const auto error=GetLastError();
                if(error!=ERROR_NO_MORE_FILES) inventory_error=error;
                break;
            }
        }
    }
    for(auto& disk:disks) {
        auto it=reverse.find(disk.id);
        if(it==reverse.end()) {
            disk.system_disk=disk.contains_page_file=unknown("No unique physical device number");
            disk.formatted_capacity_bytes={std::nullopt,Support::unsupported,"No unique physical device number"}; continue;
        }
        const auto number=it->second;
        disk.system_disk=system_error?unknown("Windows volume mapping unavailable",system_error):
            flag(system_disks.contains(number),"Contains volume holding Windows directory; not EFI boot-partition classification");
        disk.contains_page_file=page_disks.contains(number)?flag(true,"Contains an enumerated installed page file"):
            (page_error?unknown("Page-file mapping incomplete",page_error):flag(false,"Complete installed page-file enumeration did not reference this disk"));
        const auto error=inventory_error?inventory_error:(unavailable.contains(number)?unavailable[number]:ERROR_SUCCESS);
        if(error) disk.formatted_capacity_bytes={std::nullopt,error==ERROR_ACCESS_DENIED?Support::permission_required:Support::unsupported,
            "Volume capacity mapping incomplete, inaccessible or spanning; Win32 "+std::to_string(error)};
        else {
            const auto value=capacities[number];
            if(disk.capacity_bytes.value && value>*disk.capacity_bytes.value)
                disk.formatted_capacity_bytes={std::nullopt,Support::unsupported,"Mapped filesystem capacity exceeds device size"};
            else if(approximate.contains(number)) disk.formatted_capacity_bytes={value,Support::estimated,
                "Sum of single-disk filesystem capacities; caller-visible fallback may be quota-adjusted, not partition allocation"};
            else disk.formatted_capacity_bytes={value,Support::available,
                "Sum of NTFS filesystem cluster capacities, quota-independent; zero if no mapped volumes, not partition allocation"};
        }
    }
}
}
#endif
