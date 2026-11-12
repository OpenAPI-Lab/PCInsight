#include "windows_startup_file_properties.hpp"
#ifdef _WIN32
#include "windows_shortcut.hpp"
#include <algorithm>
#include <system_error>
namespace pcinsight::detail {
namespace {
StartupPropertiesResult fail(ActionError code,const char* note){return {{},code,note};}
StartupPropertiesResult error(DWORD code,const char* stage){
    return {{},code==ERROR_ACCESS_DENIED?ActionError::permission_denied:
        (code==ERROR_FILE_NOT_FOUND || code==ERROR_PATH_NOT_FOUND)?ActionError::not_found:ActionError::system_error,
        std::string(stage)+"; Win32 error="+std::to_string(code)};
}
std::string hex(const std::string& text){
    constexpr char digits[]="0123456789ABCDEF";std::string out;out.reserve(text.size()*2);
    for(unsigned char c:text){out+=digits[c>>4];out+=digits[c&15];}return out;
}
bool extension(std::string name,const char* suffix){
    for(auto& c:name)if(c>='A' && c<='Z')c=static_cast<char>(c-'A'+'a');
    return name.ends_with(suffix);
}
std::uint64_t joined(DWORD hi,DWORD lo){return (std::uint64_t(hi)<<32)|lo;}
bool same(const BY_HANDLE_FILE_INFORMATION& a,const BY_HANDLE_FILE_INFORMATION& b){
    return a.dwFileAttributes==b.dwFileAttributes && a.dwVolumeSerialNumber==b.dwVolumeSerialNumber &&
        a.nFileIndexHigh==b.nFileIndexHigh && a.nFileIndexLow==b.nFileIndexLow &&
        a.nFileSizeHigh==b.nFileSizeHigh && a.nFileSizeLow==b.nFileSizeLow &&
        CompareFileTime(&a.ftCreationTime,&b.ftCreationTime)==0 && CompareFileTime(&a.ftLastWriteTime,&b.ftLastWriteTime)==0;
}
bool valid_id(const std::string& id){
    std::size_t start=0;
    if(id.starts_with("winfolder:user:"))start=15;
    else if(id.starts_with("winfolder:common:"))start=17;
    else return false;
    if(id.size()<=start || id.size()>262160 || (id.size()-start)%2)return false;
    return std::all_of(id.begin()+start,id.end(),[](char c){return (c>='0' && c<='9') || (c>='A' && c<='F');});
}
StartupEntry select(const std::string& id,const std::vector<StartupEntry>& rows){
    if(rows.size()>20000)throw std::length_error("Startup folder properties inventory exceeds 20000 rows");
    StartupEntry found;bool matched=false;
    for(const auto& row:rows)if(row.id==id){
        if(matched)throw std::invalid_argument("Duplicate Startup folder ID");
        matched=true;found=row;
    }
    return found;
}
}
StartupPropertiesResult get_windows_startup_file_properties(const std::string& id,const StartupFolderInventory& inventory,WindowsStartupFilePropertiesApi api){
    if(!valid_id(id) || id.find('\0')!=id.npos)return fail(ActionError::invalid_argument,"Invalid/excessive Windows Startup folder ID");
    if(!inventory || !api.open || !api.close || !api.type || !api.info || !api.read)return fail(ActionError::not_supported,"Incomplete Startup file properties provider");
    try{
        auto selected=select(id,inventory());
        if(selected.id.empty())return fail(ActionError::not_found,"Startup file ID is not present in caller's current known-folder inventory");
        const auto prefix=id.starts_with("winfolder:user:")?"winfolder:user:":"winfolder:common:";
        if(selected.source.empty() || selected.source.size()>131068 || id!=prefix+hex(selected.source) || selected.name.empty() || selected.name.size()>32768 || selected.name.find_first_of("/\\:")!=selected.name.npos || selected.source.find('\0')!=selected.source.npos)
            return fail(ActionError::invalid_argument,"Inconsistent Startup file inventory identity");
        const auto count=MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,selected.source.data(),static_cast<int>(selected.source.size()),nullptr,0);
        if(count<=0 || count>32765)return fail(ActionError::invalid_argument,"Invalid/excessive Startup file UTF-8 path");
        std::wstring path(count,L'\0');
        if(MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,selected.source.data(),static_cast<int>(selected.source.size()),path.data(),count)!=count)return error(GetLastError(),"Startup file path conversion failed");
        struct File{HANDLE h;decltype(&CloseHandle) close;~File(){if(h && h!=INVALID_HANDLE_VALUE)close(h);}};
        File file{api.open(path.c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_FLAG_OPEN_REPARSE_POINT,nullptr),api.close};
        if(file.h==INVALID_HANDLE_VALUE)return error(GetLastError(),"Startup properties file open failed");
        if(!file.h)return fail(ActionError::system_error,"Startup properties open returned null handle");
        if(api.type(file.h)!=FILE_TYPE_DISK)return fail(ActionError::not_supported,"Startup properties requires a disk file");
        BY_HANDLE_FILE_INFORMATION before{},after{};
        if(!api.info(file.h,&before))return error(GetLastError(),"Startup properties file information failed");
        if(before.dwFileAttributes&(FILE_ATTRIBUTE_DIRECTORY|FILE_ATTRIBUTE_REPARSE_POINT|FILE_ATTRIBUTE_DEVICE))return fail(ActionError::stale_target,"Startup file is now a directory/reparse/device entry; reload inventory");
        StartupProperties result;
        // Copy inventory identity only, not optional metadata from another handle/sample.
        auto& row=result.entry;row.id=selected.id;row.name=selected.name;row.source=selected.source;
        row.impact_scope="windows-startup-folder-registration-only";
        const auto size=joined(before.nFileSizeHigh,before.nFileSizeLow);
        result.source_file_size_bytes={size,Support::available,"Opened Startup source file logical size, not target executable or allocated bytes"};
        result.fields={{"Windows Startup file","path",row.source},
            {"Windows Startup file","scope",id.starts_with("winfolder:user:")?"user":"common"},
            {"Windows Startup file","size_bytes",std::to_string(size)},
            {"Windows Startup file","attributes_raw",std::to_string(before.dwFileAttributes)},
            {"Windows Startup file","creation_filetime_100ns",std::to_string(joined(before.ftCreationTime.dwHighDateTime,before.ftCreationTime.dwLowDateTime))},
            {"Windows Startup file","last_write_filetime_100ns",std::to_string(joined(before.ftLastWriteTime.dwHighDateTime,before.ftLastWriteTime.dwLowDateTime))}};
        if(extension(row.name,".lnk") || extension(row.name,".url")){
            if(size>1048576)row.windows_shortcut.note="Startup source metadata exceeds 1 MiB; file properties still available";
            else{
                std::vector<std::uint8_t> bytes(static_cast<std::size_t>(size));std::size_t read=0;
                unsigned attempts=0;
                while(read<bytes.size()){
                    if(++attempts>256)return fail(ActionError::system_error,"Startup source metadata exceeded 256 read calls");
                    DWORD got=0;const auto request=static_cast<DWORD>(std::min<std::size_t>(65536,bytes.size()-read));
                    if(!api.read(file.h,bytes.data()+read,request,&got,nullptr))return error(GetLastError(),"Startup source metadata read failed");
                    if(!got || got>request)return fail(ActionError::stale_target,"Startup source metadata shortened/invalid read");
                    read+=got;
                }
                row.windows_shortcut=extension(row.name,".url")?parse_windows_internet_shortcut(bytes):parse_windows_shortcut(bytes);
            }
        }
        if(!api.info(file.h,&after))return error(GetLastError(),"Startup source revalidation failed");
        if(!same(before,after))return fail(ActionError::stale_target,"Startup source metadata changed during query");
        const auto current=select(id,inventory());
        if(current.id.empty() || current.source!=row.source || current.name!=row.name)return fail(ActionError::stale_target,"Startup entry disappeared/changed known-folder context");
        File check{api.open(path.c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_FLAG_OPEN_REPARSE_POINT,nullptr),api.close};
        if(!check.h || check.h==INVALID_HANDLE_VALUE)return fail(ActionError::stale_target,"Startup source path could not be revalidated");
        if(api.type(check.h)!=FILE_TYPE_DISK || !api.info(check.h,&after) || !same(before,after))return fail(ActionError::stale_target,"Startup source path metadata/identity changed");
        return {std::move(result),ActionError::none,"Startup source file properties observed; stored shortcut metadata only, not resolved launch identity, approval or atomic snapshot"};
    }catch(const std::system_error& e){return error(static_cast<DWORD>(e.code().value()),"Startup folder inventory failed");}
    catch(const std::invalid_argument&){return fail(ActionError::invalid_argument,"Invalid/duplicate Startup folder inventory");}
    catch(const std::length_error&){return fail(ActionError::invalid_argument,"Startup folder inventory exceeds bounds");}
}
StartupPropertiesResult get_windows_startup_file_properties(const std::string& id){
    return get_windows_startup_file_properties(id,[]{return read_windows_startup_folders();});
}
}
#endif
