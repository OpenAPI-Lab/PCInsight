#include "windows_startup_folders.hpp"
#ifdef _WIN32
#include <algorithm>
#include <cwchar>
#include <set>
#include <string_view>
#include <system_error>
namespace pcinsight::detail {
namespace {
[[noreturn]] void fail(DWORD error,const char* stage){throw std::system_error(static_cast<int>(error?error:ERROR_INVALID_DATA),std::system_category(),stage);}
struct PathMemory{PWSTR value;decltype(&CoTaskMemFree) free;~PathMemory(){if(value)free(value);}};
struct Search{HANDLE value;decltype(&FindClose) close;~Search(){if(value && value!=INVALID_HANDLE_VALUE)close(value);}};
std::string utf8(std::wstring_view value){
    const auto n=WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,value.data(),static_cast<int>(value.size()),nullptr,0,nullptr,nullptr);
    if(!n)fail(GetLastError(),"Startup folder invalid UTF-16");
    std::string out(n,'\0');if(WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,value.data(),static_cast<int>(value.size()),out.data(),n,nullptr,nullptr)!=n)fail(GetLastError(),"Startup folder UTF-8 conversion failed");return out;
}
std::string hex(const std::string& value){
    constexpr char digits[]="0123456789ABCDEF";std::string out;out.reserve(value.size()*2);
    for(unsigned char c:value){out.push_back(digits[c>>4]);out.push_back(digits[c&15]);}return out;
}
bool regular_root(std::wstring_view folder){
    // An extended Win32 prefix is not itself evidence of a filesystem path:
    // GLOBALROOT, pipes and other object-manager namespaces also use it.
    const bool extended=folder.starts_with(L"\\\\?\\");
    if(extended)folder.remove_prefix(4);
    const bool extended_unc=extended && folder.size()>=4 &&
        (folder[0]==L'U' || folder[0]==L'u') &&
        (folder[1]==L'N' || folder[1]==L'n') &&
        (folder[2]==L'C' || folder[2]==L'c') && folder[3]==L'\\';
    const auto slash=[](wchar_t c){return c==L'\\' || c==L'/';};
    const bool drive=folder.size()>=3 &&
        ((folder[0]>=L'A' && folder[0]<=L'Z') || (folder[0]>=L'a' && folder[0]<=L'z')) &&
        folder[1]==L':' && slash(folder[2]);
    if(drive)return folder.find(L':',2)==folder.npos;
    if(extended_unc)folder.remove_prefix(4);
    else if(!extended && folder.starts_with(L"\\\\"))folder.remove_prefix(2);
    else return false;
    if(folder.find(L':')!=folder.npos)return false;
    const auto server_end=folder.find_first_of(L"\\/");
    if(server_end==folder.npos || server_end==0)return false;
    const auto server=folder.substr(0,server_end);
    folder.remove_prefix(server_end+1);
    const auto share=folder.substr(0,folder.find_first_of(L"\\/"));
    return server!=L"." && server!=L".." && !share.empty() && share!=L"." && share!=L"..";
}
}
std::vector<StartupEntry> read_windows_startup_folders(WindowsStartupFoldersApi api){
    if(!api.path || !api.free || !api.first || !api.next || !api.close)fail(ERROR_NOT_SUPPORTED,"Incomplete Startup folder API");
    struct Scope{const KNOWNFOLDERID* id;const char* name;};
    const Scope scopes[]{{&FOLDERID_Startup,"user"},{&FOLDERID_CommonStartup,"common"}};
    std::vector<StartupEntry> result;std::size_t payload=0;
    for(const auto& scope:scopes){
        PWSTR raw=nullptr;const auto status=api.path(*scope.id,KF_FLAG_DONT_VERIFY,nullptr,&raw);PathMemory memory{raw,api.free};
        if(FAILED(status))fail(static_cast<DWORD>(status),"Startup known folder query failed; no partial inventory");
        if(!raw)fail(ERROR_INVALID_DATA,"Startup known folder has no path");
        const auto length=wcsnlen(raw,32768);
        if(!length || length>=32766)fail(ERROR_INVALID_DATA,"Startup known folder path exceeds bound");
        std::wstring folder(raw,length);
        if(!regular_root(folder))fail(ERROR_INVALID_DATA,"Startup known folder requires an absolute drive or server/share path, not a device namespace");
        // Extended path prefix is allowed, but wildcard characters elsewhere are not.
        if(folder.find_first_of(L"*?",folder.starts_with(L"\\\\?\\")?4:0)!=folder.npos)fail(ERROR_INVALID_DATA,"Startup known folder path has wildcard");
        (void)utf8(folder); // Validate even when the directory is empty/missing.
        if(folder.back()!=L'\\' && folder.back()!=L'/')folder+=L'\\';
        WIN32_FIND_DATAW entry{};const auto search=api.first((folder+L"*").c_str(),&entry);
        const auto open_error=search==INVALID_HANDLE_VALUE?GetLastError():ERROR_SUCCESS;Search cleanup{search,api.close};
        if(search==INVALID_HANDLE_VALUE){if(open_error==ERROR_FILE_NOT_FOUND || open_error==ERROR_PATH_NOT_FOUND)continue;fail(open_error,"Startup folder enumeration open failed");}
        if(!search)fail(ERROR_INVALID_HANDLE,"Startup folder enumeration has no handle");
        std::set<std::wstring> names;std::size_t visited=0;
        for(;;){
            if(++visited>10000)fail(ERROR_INVALID_DATA,"Startup folder exceeds 10000 entries");
            const auto end=std::find(std::begin(entry.cFileName),std::end(entry.cFileName),L'\0');
            if(end==std::end(entry.cFileName) || end==std::begin(entry.cFileName))fail(ERROR_INVALID_DATA,"Startup folder entry has invalid name");
            const std::wstring name(entry.cFileName,end);
            if(name.find_first_of(L"/\\:")!=name.npos || !names.insert(name).second)fail(ERROR_INVALID_DATA,"Startup folder duplicate/unsafe entry name");
            const auto label=utf8(name);
            const bool metadata=CompareStringOrdinal(name.c_str(),static_cast<int>(name.size()),L"desktop.ini",11,TRUE)==CSTR_EQUAL;
            if(name!=L"." && name!=L".." && !(entry.dwFileAttributes&(FILE_ATTRIBUTE_DIRECTORY|FILE_ATTRIBUTE_REPARSE_POINT|FILE_ATTRIBUTE_DEVICE)) && !metadata){
                StartupEntry row;row.name=label;row.source=utf8(folder+name);
                row.id="winfolder:"+std::string(scope.name)+":"+hex(row.source);
                // Raw folder inventory, NOT resolved launch command or effective approval.
                row.status=StartupStatus::unknown;row.impact=StartupImpact::not_measured;
                row.impact_scope="windows-startup-folder-registration-only";
                const auto cost=row.name.size()+row.source.size()+row.id.size()+row.impact_scope.size();
                if(cost>16*1024*1024-payload)fail(ERROR_INVALID_DATA,"Startup folder payload exceeds 16 MiB");
                payload+=cost;result.push_back(std::move(row));
            }
            if(!api.next(search,&entry)){const auto error=GetLastError();if(error==ERROR_NO_MORE_FILES)break;fail(error,"Startup folder enumeration interrupted; no partial inventory");}
        }
    }
    std::sort(result.begin(),result.end(),[](const auto& a,const auto& b){return a.id<b.id;});return result;
}
}
#endif
