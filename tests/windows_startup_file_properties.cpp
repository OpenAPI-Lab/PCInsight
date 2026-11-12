#include "../src/windows_startup_file_properties.hpp"
#include <cassert>
#include <cstring>
#include <iostream>
#include <system_error>
using namespace pcinsight;
using namespace pcinsight::detail;
namespace {
StartupEntry row;std::string content;BY_HANDLE_FILE_INFORMATION metadata{};
unsigned opens=0,closes=0,infos=0,reads=0,inventories=0;std::size_t cursor=0;
bool deny=false,change=false,replace=false,vanish=false,short_read=false,fail_read=false,fail_info=false,slow_read=false;
DWORD file_type=FILE_TYPE_DISK;
HANDLE first=reinterpret_cast<HANDLE>(static_cast<std::uintptr_t>(100));
HANDLE second=reinterpret_cast<HANDLE>(static_cast<std::uintptr_t>(200));
std::string hex(const std::string& text){std::string out;constexpr char d[]="0123456789ABCDEF";for(unsigned char c:text){out+=d[c>>4];out+=d[c&15];}return out;}
StartupEntry make_row(std::string path="C:\\Startup\\fixture.url",std::string name="fixture.url"){
    StartupEntry e;e.source=std::move(path);e.name=std::move(name);e.id="winfolder:user:"+hex(e.source);return e;
}
void reset(){
    row=make_row();content="[InternetShortcut]\r\nURL=custom-test:never-open\r\n";
    metadata={};metadata.dwFileAttributes=FILE_ATTRIBUTE_ARCHIVE;metadata.nFileSizeLow=static_cast<DWORD>(content.size());metadata.dwVolumeSerialNumber=1;metadata.nFileIndexLow=10;
    opens=closes=infos=reads=inventories=0;cursor=0;deny=change=replace=vanish=short_read=fail_read=fail_info=slow_read=false;file_type=FILE_TYPE_DISK;
}
HANDLE WINAPI open(LPCWSTR path,DWORD access,DWORD share,LPSECURITY_ATTRIBUTES security,DWORD disposition,DWORD flags,HANDLE template_file){
    ++opens;assert(std::wstring(path)==L"C:\\Startup\\fixture.url");assert(access==GENERIC_READ && share==FILE_SHARE_READ && !security && disposition==OPEN_EXISTING && flags==FILE_FLAG_OPEN_REPARSE_POINT && !template_file);
    if(deny){SetLastError(ERROR_ACCESS_DENIED);return INVALID_HANDLE_VALUE;}
    return opens==1?first:second;
}
BOOL WINAPI close(HANDLE handle){assert(handle==first || handle==second);++closes;return TRUE;}
DWORD WINAPI type(HANDLE handle){assert(handle==first || handle==second);return file_type;}
BOOL WINAPI info(HANDLE handle,LPBY_HANDLE_FILE_INFORMATION out){
    ++infos;if(fail_info){SetLastError(ERROR_ACCESS_DENIED);return FALSE;}
    *out=metadata;if(change && infos==2)++out->nFileSizeLow;
    if(replace && handle==second)++out->nFileIndexLow;
    return TRUE;
}
BOOL WINAPI read(HANDLE handle,LPVOID buffer,DWORD requested,LPDWORD got,LPOVERLAPPED overlapped){
    ++reads;assert(handle==first && !overlapped && requested<=65536);
    if(fail_read){SetLastError(ERROR_ACCESS_DENIED);return FALSE;}
    if(short_read){*got=0;return TRUE;}
    *got=static_cast<DWORD>(std::min<std::size_t>(requested,content.size()-cursor));
    if(slow_read && *got>1)*got=1;
    std::memcpy(buffer,content.data()+cursor,*got);cursor+=*got;return TRUE;
}
std::vector<StartupEntry> inventory(){++inventories;return vanish && inventories==2?std::vector<StartupEntry>{}:std::vector<StartupEntry>{row};}
WindowsStartupFilePropertiesApi api(){WindowsStartupFilePropertiesApi a;a.open=open;a.close=close;a.type=type;a.info=info;a.read=read;return a;}
StartupPropertiesResult query(){return get_windows_startup_file_properties(row.id,inventory,api());}
std::string utf8(const std::wstring& value){
    const auto count=WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,value.data(),static_cast<int>(value.size()),nullptr,0,nullptr,nullptr);assert(count>0);
    std::string out(count,'\0');assert(WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,value.data(),static_cast<int>(value.size()),out.data(),count,nullptr,nullptr)==count);return out;
}
void native_fixture(){
    wchar_t folder[32768]{},temp[32768]{};assert(GetTempPathW(32768,folder)>0);assert(GetTempFileNameW(folder,L"pci",0,temp));
    std::wstring path=std::wstring(temp)+L".url";assert(MoveFileW(temp,path.c_str()));
    struct Remove{const std::wstring& path;~Remove(){assert(DeleteFileW(path.c_str()));}} remove{path}; // Exact own fixture only, never recursive.
    const auto file=CreateFileW(path.c_str(),GENERIC_WRITE,0,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);assert(file!=INVALID_HANDLE_VALUE);
    const std::string text="[InternetShortcut]\nURL=test-fixture:never-open\n";DWORD written=0;
    assert(WriteFile(file,text.data(),static_cast<DWORD>(text.size()),&written,nullptr) && written==text.size());assert(CloseHandle(file));
    const auto fixture=make_row(utf8(path),utf8(path.substr(path.find_last_of(L'\\')+1)));
    const auto result=get_windows_startup_file_properties(fixture.id,[&]{return std::vector<StartupEntry>{fixture};});
    assert(result && result.value->source_file_size_bytes.value==text.size());
    assert(result.value->entry.windows_shortcut.value && result.value->entry.windows_shortcut.value->stored_url.value=="test-fixture:never-open");
    std::cout<<"Native private URL file: size and stored metadata read without launching destination\n";
}
}
int main(){
    reset();auto result=query();assert(result && opens==2 && closes==2 && infos==3 && inventories==2 && reads==1);
    assert(result.value->source_file_size_bytes.value==content.size() && result.value->fields.size()==6);
    assert(result.value->entry.windows_shortcut.value && result.value->entry.windows_shortcut.value->stored_url.value=="custom-test:never-open");
    assert(result.value->entry.command.empty() && result.value->entry.status==StartupStatus::unknown && result.value->entry.impact==StartupImpact::not_measured && result.value->entry.publisher.empty());
    reset();row.name="fixture.exe";result=query();assert(result && reads==0 && !result.value->entry.windows_shortcut.value);
    reset();content="malformed";metadata.nFileSizeLow=static_cast<DWORD>(content.size());result=query();assert(result && !result.value->entry.windows_shortcut.value && result.value->source_file_size_bytes.value==content.size());
    reset();metadata.nFileSizeLow=1048577;result=query();assert(result && reads==0 && !result.value->entry.windows_shortcut.value);
    reset();row.name="fixture.exe";metadata.nFileSizeHigh=1;metadata.nFileSizeLow=0;result=query();assert(result && result.value->source_file_size_bytes.value==4294967296ULL && reads==0);
    reset();deny=true;assert(query().error==ActionError::permission_denied && closes==0);
    reset();fail_info=true;assert(query().error==ActionError::permission_denied && closes==1);
    reset();fail_read=true;assert(query().error==ActionError::permission_denied && closes==1);
    reset();short_read=true;assert(query().error==ActionError::stale_target && closes==1);
    reset();change=true;assert(query().error==ActionError::stale_target && closes==1);
    reset();replace=true;assert(query().error==ActionError::stale_target && closes==2);
    reset();vanish=true;assert(query().error==ActionError::stale_target && closes==1);
    reset();file_type=FILE_TYPE_PIPE;assert(query().error==ActionError::not_supported && closes==1 && reads==0);
    for(auto attribute:{FILE_ATTRIBUTE_DIRECTORY,FILE_ATTRIBUTE_REPARSE_POINT,FILE_ATTRIBUTE_DEVICE}){reset();metadata.dwFileAttributes=attribute;assert(query().error==ActionError::stale_target && reads==0 && closes==1);}
    reset();content.assign(300,'x');metadata.nFileSizeLow=300;slow_read=true;assert(query().error==ActionError::system_error && reads==256 && closes==1);
    reset();const auto none=[] {return std::vector<StartupEntry>{};};assert(get_windows_startup_file_properties(row.id,none,api()).error==ActionError::not_found && opens==0);
    reset();assert(get_windows_startup_file_properties(row.id,[]{return std::vector<StartupEntry>{row,row};},api()).error==ActionError::invalid_argument && opens==0);
    reset();assert(get_windows_startup_file_properties(row.id,[]{throw std::system_error(ERROR_ACCESS_DENIED,std::system_category());return std::vector<StartupEntry>{};},api()).error==ActionError::permission_denied && opens==0);
    reset();const auto original_id=row.id;row.source="C:\\other.url";assert(get_windows_startup_file_properties(original_id,inventory,api()).error==ActionError::invalid_argument && opens==0);
    for(const auto* id:{"winfolder:user:fixture","winfolder:other:41","winfolder:user:4","winfolder:user:ff","winfolder:common:"}){reset();assert(get_windows_startup_file_properties(id,inventory,api()).error==ActionError::invalid_argument && inventories==0 && opens==0);}
    native_fixture();
    unsigned observed=0;
    for(const auto& entry:read_windows_startup_folders()){
        const auto properties=get_startup_properties(entry.id);
        if(properties){assert(properties.value->source_file_size_bytes.value && properties.value->entry.command.empty());++observed;}
        else assert(properties.error==ActionError::permission_denied || properties.error==ActionError::stale_target || properties.error==ActionError::not_found);
        if(observed==3)break;
    }
    std::cout<<"Startup file Properties: bounds, same-handle metadata, errors, revalidation and cleanup passed; native folder entries observed="<<observed<<'\n';
}
