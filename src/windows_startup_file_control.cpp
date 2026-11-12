#include "windows_startup_file_control.hpp"
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <winternl.h>
#include <shlobj.h>
#include <algorithm>
#include <cstddef>
#include <cstring>
#include <cwchar>
#include <memory>
#include <map>
#include <utility>
namespace pcinsight::detail {
namespace {
constexpr wchar_t backup_name[]=L"PCInsight.Disabled.v1";
// Documented ntifs.h ABI; the user-mode Windows SDK's winternl.h omits these
// declarations (MinGW supplies them). No WDK installation or import lib needed.
enum class RenameInformationClass : int { rename = 10 };
struct RenameInformation {
    BOOLEAN ReplaceIfExists;
    HANDLE RootDirectory;
    ULONG FileNameLength;
    WCHAR FileName[1];
};
using SetInformationFile = NTSTATUS (NTAPI*)(HANDLE, PIO_STATUS_BLOCK, PVOID, ULONG, RenameInformationClass);
static_assert(sizeof(RenameInformationClass) == 4 && sizeof(BOOLEAN) == 1 && sizeof(ULONG) == 4);
static_assert(offsetof(RenameInformation, RootDirectory) == (sizeof(HANDLE) == 8 ? 8 : 4));
static_assert(offsetof(RenameInformation, FileNameLength) == (sizeof(HANDLE) == 8 ? 16 : 8));
static_assert(offsetof(RenameInformation, FileName) == (sizeof(HANDLE) == 8 ? 20 : 12));
static_assert(sizeof(RenameInformation) == (sizeof(HANDLE) == 8 ? 24 : 16));
#ifdef __MINGW32__
// Cross-check against an independently supplied declaration when available.
static_assert(sizeof(RenameInformation) == sizeof(FILE_RENAME_INFORMATION));
static_assert(offsetof(RenameInformation, RootDirectory) == offsetof(FILE_RENAME_INFORMATION, RootDirectory));
static_assert(offsetof(RenameInformation, FileNameLength) == offsetof(FILE_RENAME_INFORMATION, FileNameLength));
static_assert(offsetof(RenameInformation, FileName) == offsetof(FILE_RENAME_INFORMATION, FileName));
static_assert(static_cast<int>(RenameInformationClass::rename) == FileRenameInformation);
#endif
struct Failure { ActionError error; std::string note; };
[[noreturn]] void fail(ActionError error,const char* note){throw Failure{error,note};}
[[noreturn]] void native(DWORD code,const char* stage){
    const auto error=code==ERROR_ACCESS_DENIED || code==ERROR_PRIVILEGE_NOT_HELD?ActionError::permission_denied:
        code==ERROR_FILE_NOT_FOUND || code==ERROR_PATH_NOT_FOUND?ActionError::not_found:
        code==ERROR_ALREADY_EXISTS || code==ERROR_FILE_EXISTS || code==ERROR_SHARING_VIOLATION || code==ERROR_DELETE_PENDING?ActionError::stale_target:
        code==ERROR_NOT_SUPPORTED || code==ERROR_INVALID_FUNCTION || code==ERROR_CALL_NOT_IMPLEMENTED?ActionError::not_supported:ActionError::system_error;
    throw Failure{error,std::string(stage)+"; Win32 error="+std::to_string(code)};
}
struct Handle {
    HANDLE value{};
    ~Handle(){if(value && value!=INVALID_HANDLE_VALUE)CloseHandle(value);}
    Handle()=default;
    Handle(const Handle&)=delete;
    Handle& operator=(const Handle&)=delete;
    Handle(Handle&& other) noexcept:value(std::exchange(other.value,nullptr)){}
    Handle& operator=(Handle&& other) noexcept{if(this!=&other){if(value && value!=INVALID_HANDLE_VALUE)CloseHandle(value);value=std::exchange(other.value,nullptr);}return *this;}
};
struct NativeApi {
    decltype(&NtCreateFile) create{};
    SetInformationFile set{};
    decltype(&RtlNtStatusToDosError) error{};
    template<class Function> static Function load(HMODULE module,const char* name){
        const auto address=GetProcAddress(module,name);Function result{};
        static_assert(sizeof(result)==sizeof(address));std::memcpy(&result,&address,sizeof(result));return result;
    }
    NativeApi(){
        const auto module=GetModuleHandleW(L"ntdll.dll");
        if(!module)fail(ActionError::not_supported,"Native filesystem module unavailable");
        create=load<decltype(create)>(module,"NtCreateFile");
        set=load<decltype(set)>(module,"NtSetInformationFile");
        error=load<decltype(error)>(module,"RtlNtStatusToDosError");
        if(!create || !set || !error)fail(ActionError::not_supported,"Relative-handle filesystem API unavailable; no path-based fallback");
    }
};
std::uint64_t joined(DWORD high,DWORD low){return (std::uint64_t(high)<<32)|low;}
WindowsStartupFileIdentity identify(HANDLE file,bool directory){
    BY_HANDLE_FILE_INFORMATION info{};
    if(GetFileType(file)!=FILE_TYPE_DISK)fail(ActionError::not_supported,"Startup control requires a disk filesystem object");
    if(!GetFileInformationByHandle(file,&info))native(GetLastError(),"Startup object information failed");
    if(bool(info.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)!=directory ||
       info.dwFileAttributes&(FILE_ATTRIBUTE_REPARSE_POINT|FILE_ATTRIBUTE_DEVICE))
        fail(ActionError::not_supported,"Startup control rejects reparse/device objects and mismatched file types");
    if(!info.nFileIndexHigh && !info.nFileIndexLow)fail(ActionError::not_supported,"Filesystem did not provide a nonzero object index; checked control unavailable");
    return {info.dwVolumeSerialNumber,joined(info.nFileIndexHigh,info.nFileIndexLow),
        joined(info.ftCreationTime.dwHighDateTime,info.ftCreationTime.dwLowDateTime),
        joined(info.ftLastWriteTime.dwHighDateTime,info.ftLastWriteTime.dwLowDateTime),joined(info.nFileSizeHigh,info.nFileSizeLow)};
}
bool same_directory(const WindowsStartupFileIdentity& a,const WindowsStartupFileIdentity& b){
    // Directory timestamps/size legitimately change when files are moved.
    return a.volume==b.volume && a.file_index==b.file_index && a.creation_time_100ns==b.creation_time_100ns;
}
std::wstring root_path(const WindowsStartupRoot& resolver){
    if(!resolver)fail(ActionError::invalid_argument,"Missing Startup root resolver");
    auto root=resolver();
    if(root.empty() || root.size()>32000 || root.find(L'\0')!=root.npos)fail(ActionError::invalid_argument,"Invalid Startup root size/NUL");
    auto path=std::wstring_view(root);if(path.starts_with(L"\\\\?\\"))path.remove_prefix(4);
    if(path.size()<3 || !((path[0]>=L'A' && path[0]<=L'Z') || (path[0]>=L'a' && path[0]<=L'z')) || path[1]!=L':' || path[2]!=L'\\' ||
       path.find(L':',2)!=path.npos || path.find_first_of(L"*?/")!=path.npos)
        fail(ActionError::not_supported,"Startup control supports literal local drive paths only, not UNC/device/stream paths");
    if(GetDriveTypeW(std::wstring(path.substr(0,3)).c_str())==DRIVE_REMOTE)fail(ActionError::not_supported,"Startup control does not support mapped network drives");
    if(root.back()!=L'\\')root+=L'\\';
    return root;
}
std::string_view scope_prefix(WindowsStartupFileScope scope){
    switch(scope){case WindowsStartupFileScope::current_user:return "winfolder:user:";case WindowsStartupFileScope::common:return "winfolder:common:";}
    fail(ActionError::invalid_argument,"Invalid Startup file scope");
}
WindowsStartupFileScope scope_from_id(const std::string& id){
    if(id.starts_with("winfolder:user:"))return WindowsStartupFileScope::current_user;
    if(id.starts_with("winfolder:common:"))return WindowsStartupFileScope::common;
    fail(ActionError::invalid_argument,"Startup file ID must identify user or common scope");
}
std::wstring name_from_id(const std::string& id,const std::wstring& root,WindowsStartupFileScope scope){
    const auto prefix=scope_prefix(scope);
    if(!id.starts_with(prefix))fail(ActionError::not_supported,"Startup ID scope differs from the selected known folder");
    const auto hex=std::string_view(id).substr(prefix.size());
    if(hex.empty() || hex.size()%2 || hex.size()>131068 || id.find('\0')!=id.npos)fail(ActionError::invalid_argument,"Invalid Startup file ID size/NUL");
    const auto digit=[](char c){return c>='0' && c<='9'?c-'0':c>='A' && c<='F'?c-'A'+10:-1;};
    std::string bytes;bytes.reserve(hex.size()/2);
    for(std::size_t i=0;i<hex.size();i+=2){auto a=digit(hex[i]),b=digit(hex[i+1]);if(a<0 || b<0)fail(ActionError::invalid_argument,"Startup ID is not canonical uppercase hex");bytes+=static_cast<char>(a*16+b);}
    if(bytes.find('\0')!=bytes.npos)fail(ActionError::invalid_argument,"Startup path contains NUL");
    const auto count=MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,bytes.data(),static_cast<int>(bytes.size()),nullptr,0);
    if(count<=0)fail(ActionError::invalid_argument,"Invalid Startup path UTF-8");
    std::wstring path(count,L'\0');
    if(MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,bytes.data(),static_cast<int>(bytes.size()),path.data(),count)!=count)native(GetLastError(),"Startup path conversion failed");
    if(!path.starts_with(root))fail(ActionError::stale_target,"Startup ID no longer matches current known-folder root");
    auto name=path.substr(root.size());
    if(name.empty() || name.size()>255 || name==L"." || name==L".." || name.find_first_of(L"\\/:*?")!=name.npos || name.back()==L'.' || name.back()==L' ')
        fail(ActionError::invalid_argument,"Startup entry must be a direct literal filename");
    if(CompareStringOrdinal(name.data(),static_cast<int>(name.size()),L"desktop.ini",11,TRUE)==CSTR_EQUAL ||
       CompareStringOrdinal(name.data(),static_cast<int>(name.size()),backup_name,static_cast<int>(std::size(backup_name)-1),TRUE)==CSTR_EQUAL)
        fail(ActionError::not_supported,"Startup metadata/backup directory is not an application registration");
    return name;
}
Handle child(HANDLE parent,const std::wstring& name,bool directory,bool mutation,bool create,const NativeApi& api,bool enumerate=false){
    UNICODE_STRING text{};text.Length=static_cast<USHORT>(name.size()*sizeof(wchar_t));text.MaximumLength=text.Length;text.Buffer=const_cast<PWSTR>(name.data());
    OBJECT_ATTRIBUTES attributes{};attributes.Length=sizeof(attributes);attributes.RootDirectory=parent;attributes.ObjectName=&text;
    // Do not force OBJ_CASE_INSENSITIVE: preserve exact names on case-sensitive directories.
    IO_STATUS_BLOCK io{};Handle result;
    const ACCESS_MASK access=FILE_READ_ATTRIBUTES|SYNCHRONIZE|(mutation && !directory?DELETE:0)| (directory?FILE_TRAVERSE:0)|(enumerate?FILE_LIST_DIRECTORY:0);
    const auto status=api.create(&result.value,access,&attributes,&io,nullptr,FILE_ATTRIBUTE_NORMAL,
        directory?(FILE_SHARE_READ|FILE_SHARE_WRITE):(mutation?FILE_SHARE_READ:(FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE)),create?3:1, // FILE_OPEN_IF / FILE_OPEN
        (directory?0x1:0x40)|0x20|0x200000,nullptr,0); // DIRECTORY/NON_DIRECTORY, SYNCHRONOUS_IO_NONALERT, OPEN_REPARSE_POINT
    if(status<0){const auto error=api.error(status);if(!create && (error==ERROR_FILE_NOT_FOUND || error==ERROR_PATH_NOT_FOUND))return {};native(error,"Relative Startup object open failed");}
    if(!result.value || result.value==INVALID_HANDLE_VALUE)fail(ActionError::system_error,"Relative Startup open returned no handle");
    (void)identify(result.value,directory);return result;
}
WindowsStartupFileRegistration operate(const std::string& id,const WindowsStartupFileRegistration* expected,bool registered,const WindowsStartupRoot& resolver,WindowsStartupFileScope scope){
    const auto root=root_path(resolver),name=name_from_id(id,root,scope);NativeApi api;
    Handle directory;directory.value=CreateFileW(root.c_str(),FILE_READ_ATTRIBUTES|FILE_TRAVERSE|SYNCHRONIZE,
        FILE_SHARE_READ|FILE_SHARE_WRITE,nullptr,OPEN_EXISTING,FILE_FLAG_BACKUP_SEMANTICS|FILE_FLAG_OPEN_REPARSE_POINT,nullptr);
    if(directory.value==INVALID_HANDLE_VALUE)native(GetLastError(),"Startup directory open failed");
    const auto root_identity=identify(directory.value,true);
    auto saved_dir=child(directory.value,backup_name,true,expected!=nullptr,false,api);
    auto live=child(directory.value,name,false,expected!=nullptr,false,api);
    auto saved=saved_dir.value?child(saved_dir.value,name,false,expected!=nullptr,false,api):Handle{};
    WindowsStartupFileRegistration result;result.entry_id=id;result.directory_identity=root_identity;
    using State=WindowsStartupFileState;
    result.state=live.value?(saved.value?State::conflict:State::registered):(saved.value?State::retained_by_pcinsight:State::absent);
    if(result.state==State::registered || result.state==State::retained_by_pcinsight)result.file_identity=identify(live.value?live.value:saved.value,false);
    if(!expected)return result;
    const auto required=registered?State::retained_by_pcinsight:State::registered;
    if(expected->state!=required || result.state!=required || !same_directory(expected->directory_identity,root_identity) || !expected->file_identity || expected->file_identity!=result.file_identity)
        fail(ActionError::stale_target,"Startup file/directory identity or registration changed; reload, no overwrite");
    if(!saved_dir.value)saved_dir=child(directory.value,backup_name,true,true,true,api);
    const auto backup_identity=identify(saved_dir.value,true);
    if(backup_identity.volume!=root_identity.volume)fail(ActionError::not_supported,"Startup backup is not on the same volume");
    // No path resolution at commit: both the source file and destination directory
    // are pinned handles. The single rename rejects any existing destination.
    const auto length=static_cast<ULONG>(name.size()*sizeof(wchar_t));
    const auto size=sizeof(RenameInformation)+length;
    auto storage=std::make_unique<std::byte[]>(size);auto* rename=reinterpret_cast<RenameInformation*>(storage.get());
    rename->ReplaceIfExists=FALSE;rename->RootDirectory=registered?directory.value:saved_dir.value;rename->FileNameLength=length;
    std::memcpy(rename->FileName,name.data(),length);IO_STATUS_BLOCK io{};
    const auto status=api.set(registered?saved.value:live.value,&io,rename,static_cast<ULONG>(size),RenameInformationClass::rename);
    if(status<0)native(api.error(status),"Startup no-replace handle rename failed; requery before retry");
    result.state=registered?State::registered:State::retained_by_pcinsight;return result;
}
std::string utf8_text(const std::wstring& path){
    const auto count=WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,path.data(),static_cast<int>(path.size()),nullptr,0,nullptr,nullptr);
    if(count<=0)fail(ActionError::invalid_argument,"Backup name/path has invalid UTF-16");
    std::string bytes(count,'\0');
    if(WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,path.data(),static_cast<int>(path.size()),bytes.data(),count,nullptr,nullptr)!=count)native(GetLastError(),"Backup ID conversion failed");
    return bytes;
}
std::string file_id(const std::wstring& path,WindowsStartupFileScope scope){
    const auto bytes=utf8_text(path);std::string id(scope_prefix(scope));constexpr char hex[]="0123456789ABCDEF";
    for(unsigned char c:bytes){id+=hex[c>>4];id+=hex[c&15];}return id;
}
using BackupEntries=std::map<std::wstring,WindowsStartupFileIdentity>;
BackupEntries scan_backups(HANDLE directory,std::uint64_t volume,decltype(&GetFileInformationByHandleEx) query){
    BackupEntries result;std::size_t visited=0;
    std::vector<std::byte> buffer(65536);
    constexpr auto header=offsetof(FILE_ID_BOTH_DIR_INFO,FileName);
    for(std::size_t call=0;;++call){
        if(call>10000)fail(ActionError::invalid_argument,"Backup listing exceeds 10001 enumeration calls");
        std::fill(buffer.begin(),buffer.end(),std::byte{});
        if(!query(directory,call?FileIdBothDirectoryInfo:FileIdBothDirectoryRestartInfo,buffer.data(),static_cast<DWORD>(buffer.size()))){
            const auto code=GetLastError();if(code==ERROR_NO_MORE_FILES)break;
            native(code,"Backup enumeration failed; no partial result");
        }
        std::size_t offset=0;
        for(;;){
            if(++visited>10000)fail(ActionError::invalid_argument,"Backup listing exceeds 10000 entries including ignored objects");
            const auto remaining=buffer.size()-offset;
            if(remaining<header)fail(ActionError::invalid_argument,"Truncated backup directory record");
            FILE_ID_BOTH_DIR_INFO record{};std::memcpy(&record,buffer.data()+offset,header);
            const auto size=record.FileNameLength;
            if(!size || size%sizeof(wchar_t) || size>510 || size>remaining-header)
                fail(ActionError::invalid_argument,"Invalid backup filename byte count");
            if(record.NextEntryOffset && (record.NextEntryOffset%8 || record.NextEntryOffset<header+size || record.NextEntryOffset>remaining-header))
                fail(ActionError::invalid_argument,"Invalid backup directory record offset");
            std::wstring name(size/sizeof(wchar_t),L'\0');std::memcpy(name.data(),buffer.data()+offset+header,size);
            if(name.find(L'\0')!=name.npos || name.find_first_of(L"\\/:*?")!=name.npos)fail(ActionError::invalid_argument,"Unsafe backup filename");
            (void)utf8_text(name); // Validate UTF-16 also for ignored entries.
            if(name!=L"." && name!=L".." && !(record.FileAttributes&(FILE_ATTRIBUTE_DIRECTORY|FILE_ATTRIBUTE_REPARSE_POINT|FILE_ATTRIBUTE_DEVICE)) &&
               CompareStringOrdinal(name.data(),static_cast<int>(name.size()),L"desktop.ini",11,TRUE)!=CSTR_EQUAL){
                if(record.EndOfFile.QuadPart<0 || !record.FileId.QuadPart)fail(ActionError::not_supported,"Backup enumeration lacks usable size/object identity");
                WindowsStartupFileIdentity identity{volume,static_cast<std::uint64_t>(record.FileId.QuadPart),static_cast<std::uint64_t>(record.CreationTime.QuadPart),
                    static_cast<std::uint64_t>(record.LastWriteTime.QuadPart),static_cast<std::uint64_t>(record.EndOfFile.QuadPart)};
                if(!result.emplace(std::move(name),identity).second)fail(ActionError::stale_target,"Duplicate backup entry during enumeration");
            }
            if(!record.NextEntryOffset)break;
            offset+=record.NextEntryOffset;
        }
    }
    return result;
}
std::vector<WindowsStartupFileBackup> list_backups(const WindowsStartupRoot& resolver,decltype(&GetFileInformationByHandleEx) query,WindowsStartupFileScope scope){
    (void)scope_prefix(scope);
    if(!query)fail(ActionError::not_supported,"Backup enumeration API unavailable");
    const auto root=root_path(resolver);NativeApi api;Handle directory;
    directory.value=CreateFileW(root.c_str(),FILE_READ_ATTRIBUTES|FILE_TRAVERSE|SYNCHRONIZE,FILE_SHARE_READ|FILE_SHARE_WRITE,
        nullptr,OPEN_EXISTING,FILE_FLAG_BACKUP_SEMANTICS|FILE_FLAG_OPEN_REPARSE_POINT,nullptr);
    if(directory.value==INVALID_HANDLE_VALUE)native(GetLastError(),"Backup listing Startup root open failed");
    const auto root_identity=identify(directory.value,true);
    auto saved_dir=child(directory.value,backup_name,true,false,false,api,true);
    std::vector<WindowsStartupFileBackup> result;
    if(saved_dir.value){
        const auto backup_identity=identify(saved_dir.value,true);
        if(backup_identity.volume!=root_identity.volume)fail(ActionError::not_supported,"Backup listing crosses volumes");
        const auto entries=scan_backups(saved_dir.value,root_identity.volume,query);std::size_t payload=0;
        for(const auto& [name,identity]:entries){
            auto id=file_id(root+name,scope);auto label=utf8_text(name);(void)name_from_id(id,root,scope);
            const auto cost=id.size()+label.size();
            if(cost>16*1024*1024-payload)fail(ActionError::invalid_argument,"Backup listing text exceeds 16 MiB");
            payload+=cost;
            auto saved=child(saved_dir.value,name,false,false,false,api);
            if(!saved.value || identify(saved.value,false)!=identity)fail(ActionError::stale_target,"Backup changed since enumeration");
            auto live=child(directory.value,name,false,false,false,api);
            WindowsStartupFileRegistration row;row.entry_id=std::move(id);row.directory_identity=root_identity;
            row.state=live.value?WindowsStartupFileState::conflict:WindowsStartupFileState::retained_by_pcinsight;
            if(!live.value)row.file_identity=identity;
            result.push_back({std::move(label),std::move(row)});
        }
        if(scan_backups(saved_dir.value,root_identity.volume,query)!=entries)fail(ActionError::stale_target,"Backup listing changed between passes");
        // Re-observe live conflicts too, without implying atomicity across names.
        for(const auto& item:result){const auto& row=item.registration;
            auto live=child(directory.value,name_from_id(row.entry_id,root,scope),false,false,false,api);
            if(bool(live.value)!=(row.state==WindowsStartupFileState::conflict))fail(ActionError::stale_target,"Live/backup conflict changed during listing");
        }
    }else{
        auto check=child(directory.value,backup_name,true,false,false,api);
        if(check.value)fail(ActionError::stale_target,"Backup directory appeared during listing");
    }
    if(root_path(resolver)!=root)fail(ActionError::stale_target,"Known-folder context changed during backup listing");
    Handle check;check.value=CreateFileW(root.c_str(),FILE_READ_ATTRIBUTES|FILE_TRAVERSE|SYNCHRONIZE,FILE_SHARE_READ|FILE_SHARE_WRITE,
        nullptr,OPEN_EXISTING,FILE_FLAG_BACKUP_SEMANTICS|FILE_FLAG_OPEN_REPARSE_POINT,nullptr);
    if(check.value==INVALID_HANDLE_VALUE)native(GetLastError(),"Backup listing root revalidation failed");
    if(!same_directory(root_identity,identify(check.value,true)))fail(ActionError::stale_target,"Startup root identity changed during backup listing");
    return result;
}
}
Metric<std::vector<WindowsStartupFileBackup>> list_windows_startup_file_backups(const WindowsStartupRoot& root,decltype(&GetFileInformationByHandleEx) query,WindowsStartupFileScope scope){
    try{return {list_backups(root,query,scope),Support::estimated,"Bounded direct regular backup files in the selected user/common scope, matching enumeration/identity/conflict observations; NOT atomic, Windows approval or authenticated authorship; ignored directories/reparse/device/desktop.ini, no writes"};}
    catch(const Failure& e){return {{},e.error==ActionError::permission_denied?Support::permission_required:Support::unsupported,e.note};}
}
Metric<WindowsStartupFileRegistration> read_windows_startup_file_registration(const std::string& id,const WindowsStartupRoot& root,WindowsStartupFileScope scope){
    try{return {operate(id,nullptr,false,root,scope),Support::estimated,"Observed Startup/direct backup file state in selected user/common scope; not atomic, Windows approval, authenticated ownership or launch eligibility"};}
    catch(const Failure& e){return {{},e.error==ActionError::permission_denied?Support::permission_required:Support::unsupported,e.note};}
}
ActionResult set_windows_startup_file_registered(const WindowsStartupFileRegistration& observed,bool registered,const WindowsStartupRoot& root,WindowsStartupFileScope scope){
    try{operate(observed.entry_id,&observed,registered,root,scope);return {ActionError::none,registered?"Startup source file restored without overwrite; selected scope unchanged, common scope can affect all users; Windows approval unchanged":"Startup source file retained in PCInsight.Disabled.v1 of selected scope; common scope can affect all users; running processes and other startup sources unchanged"};}
    catch(const Failure& e){return {e.error,e.note};}
}
std::wstring startup_root(WindowsStartupFileScope scope){
    (void)scope_prefix(scope);
    PWSTR raw=nullptr;const auto status=SHGetKnownFolderPath(scope==WindowsStartupFileScope::current_user?FOLDERID_Startup:FOLDERID_CommonStartup,KF_FLAG_DONT_VERIFY,nullptr,&raw);
    struct Memory{PWSTR p;~Memory(){if(p)CoTaskMemFree(p);}} memory{raw};
    if(FAILED(status))throw Failure{status==E_ACCESSDENIED?ActionError::permission_denied:ActionError::system_error,"Startup known-folder query failed; HRESULT="+std::to_string(static_cast<unsigned long>(status))};
    if(!raw)fail(ActionError::system_error,"Startup known-folder path missing");
    const auto size=wcsnlen(raw,32768);if(size>=32768)fail(ActionError::invalid_argument,"Startup root exceeds path bound");return {raw,size};
}
std::wstring current_startup_root(){return startup_root(WindowsStartupFileScope::current_user);}
std::wstring common_startup_root(){return startup_root(WindowsStartupFileScope::common);}
Metric<WindowsStartupFileRegistration> read_scoped(const std::string& id){
    try{const auto scope=scope_from_id(id);return read_windows_startup_file_registration(id,scope==WindowsStartupFileScope::common?common_startup_root:current_startup_root,scope);}
    catch(const Failure& e){return {{},Support::unsupported,e.note};}
}
ActionResult set_scoped(const WindowsStartupFileRegistration& observed,bool registered){
    try{const auto scope=scope_from_id(observed.entry_id);return set_windows_startup_file_registered(observed,registered,scope==WindowsStartupFileScope::common?common_startup_root:current_startup_root,scope);}
    catch(const Failure& e){return {e.error,e.note};}
}
}
#endif
namespace pcinsight {
Metric<std::vector<WindowsStartupFileBackup>> list_windows_startup_file_backups(){
#ifdef _WIN32
    return detail::list_windows_startup_file_backups(detail::current_startup_root);
#else
    return {{},Support::unsupported,"Startup file backup listing requires Windows"};
#endif
}
Metric<WindowsStartupFileRegistration> read_windows_startup_file_registration(const std::string& id){
#ifdef _WIN32
    return detail::read_scoped(id);
#else
    (void)id;return {{},Support::unsupported,"Startup file registration control requires Windows"};
#endif
}
ActionResult set_windows_startup_file_registered(const WindowsStartupFileRegistration& observed,bool registered){
#ifdef _WIN32
    return detail::set_scoped(observed,registered);
#else
    (void)observed;(void)registered;return {ActionError::not_supported,"Startup file registration control requires Windows"};
#endif
}
Metric<std::vector<WindowsStartupFileBackup>> list_windows_common_startup_file_backups(){
#ifdef _WIN32
    return detail::list_windows_startup_file_backups(detail::common_startup_root,&GetFileInformationByHandleEx,detail::WindowsStartupFileScope::common);
#else
    return {{},Support::unsupported,"Common Startup file backup listing requires Windows"};
#endif
}
}
