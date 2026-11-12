#include "../src/windows_startup_folders.hpp"
#include "pcinsight/monitor.hpp"
#include <cassert>
#include <cstring>
#include <iostream>
#include <set>
#include <system_error>
using namespace pcinsight;
using namespace pcinsight::detail;
namespace {
int mode=0,allocated=0,freed=0,opened=0,closed=0,path_calls=0;unsigned index=0;int token;
void start(int m){assert(allocated==freed && opened==closed);mode=m;allocated=freed=opened=closed=path_calls=0;index=0;}
HRESULT WINAPI path(REFKNOWNFOLDERID id,DWORD flags,HANDLE user,PWSTR* out){
    assert(flags==KF_FLAG_DONT_VERIFY && !user);++path_calls;
    const bool common=IsEqualGUID(id,FOLDERID_CommonStartup);assert(common || IsEqualGUID(id,FOLDERID_Startup));
    if(mode==1 || (mode==14 && common))return E_ACCESSDENIED;
    if(mode==2)return S_OK;
    std::wstring value=common?L"C:\\Fixture\\Common":L"C:\\Fixture\\User";
    if(mode==3)value=L"relative";
    if(mode==4)value=L"C:\\Bad*Folder";
    if(mode==5)value.assign(32768,L'x');
    if(mode==6)value=L"C:\\Bad"+std::wstring(1,static_cast<wchar_t>(0xd800));
    if(mode==16)value=L"C:\\"+std::wstring(32000,L'x');
    if(mode==17)value=L"\\\\?\\C:\\Fixture";
    if(mode==18)value=L"\\\\.\\PhysicalDrive0";
    if(mode==20)value=L"\\\\?\\GLOBALROOT\\Device\\HarddiskVolume1\\Startup";
    if(mode==21)value=L"\\\\?\\pipe\\Startup";
    if(mode==22)value=L"\\\\server";
    if(mode==23)value=L"C:\\Fixture:stream";
    if(mode==24)value=L"\\\\server\\share\\Startup";
    if(mode==25)value=L"\\\\?\\UNC\\server\\share\\Startup";
    if(mode==26)value=L"\\\\?\\unc\\server\\share\\Startup";
    if(mode==27)value=L"\\\\?\\UNC\\server";
    if(mode==28)value=L"\\\\?\\Volume{00000000-0000-0000-0000-000000000000}\\Startup";
    if(mode==29)value=L"C:Startup";
    if(mode==30)value=L"\\\\server\\";
    if(mode==31)value=L"\\\\?\\C:\\Fixture:stream";
    const auto bytes=(value.size()+1)*sizeof(wchar_t);*out=static_cast<PWSTR>(CoTaskMemAlloc(bytes));assert(*out);std::memcpy(*out,value.c_str(),bytes);++allocated;
    if(mode==19)return E_ACCESSDENIED; // Allocated even on failure must be freed.
    return S_OK;
}
void WINAPI release(void* p){assert(p);++freed;CoTaskMemFree(p);}
void record(LPWIN32_FIND_DATAW out,unsigned k){
    *out={};std::wstring name;
    if(k==0){name=L"App \u00e9.lnk";out->dwFileAttributes=FILE_ATTRIBUTE_ARCHIVE;}
    if(k==1){name=L"script.cmd";out->dwFileAttributes=FILE_ATTRIBUTE_HIDDEN;}
    if(k==2){name=L"DESKTOP.INI";out->dwFileAttributes=FILE_ATTRIBUTE_SYSTEM;}
    if(k==3){name=L"subdir";out->dwFileAttributes=FILE_ATTRIBUTE_DIRECTORY;}
    if(k==4){name=L"junction.lnk";out->dwFileAttributes=FILE_ATTRIBUTE_REPARSE_POINT;}
    if(k==5){name=L".";out->dwFileAttributes=FILE_ATTRIBUTE_DIRECTORY;}
    if(mode==10 && k==1)name=L"App \u00e9.lnk";
    if(mode==11)name=L"..\\escape.lnk";
    if(mode==12)name=std::wstring(1,static_cast<wchar_t>(0xd800));
    if(mode==15 || mode==16)name=std::to_wstring(k)+L".lnk";
    assert(name.size()<MAX_PATH);std::memcpy(out->cFileName,name.c_str(),(name.size()+1)*sizeof(wchar_t));
    if(mode==13)std::fill(std::begin(out->cFileName),std::end(out->cFileName),L'x');
}
HANDLE WINAPI first(LPCWSTR wildcard,LPWIN32_FIND_DATAW out){
    assert(std::wstring(wildcard).ends_with(L"\\*"));
    if(mode==7){SetLastError(ERROR_FILE_NOT_FOUND);return INVALID_HANDLE_VALUE;}
    if(mode==8){SetLastError(ERROR_ACCESS_DENIED);return INVALID_HANDLE_VALUE;}
    ++opened;index=0;record(out,0);return &token;
}
BOOL WINAPI next(HANDLE h,LPWIN32_FIND_DATAW out){
    assert(h==&token);
    if(mode==9){SetLastError(ERROR_ACCESS_DENIED);return FALSE;}
    ++index;
    if(index==(mode==15?10001U:mode==16?1000U:6U)){SetLastError(ERROR_NO_MORE_FILES);return FALSE;}
    record(out,index);return TRUE;
}
BOOL WINAPI close(HANDLE h){assert(h==&token);++closed;return TRUE;}
WindowsStartupFoldersApi api(){return {path,release,first,next,close};}
}
int main(){
    start(0);auto rows=read_windows_startup_folders(api());assert(rows.size()==4 && allocated==freed && opened==closed && opened==2);std::set<std::string> ids;
    for(const auto& row:rows){assert(ids.insert(row.id).second && row.id.starts_with("winfolder:") && row.source.starts_with("C:\\Fixture\\") && row.command.empty() && row.publisher.empty() && row.status==StartupStatus::unknown && row.impact==StartupImpact::not_measured && !row.observed_startup_impact.value && row.impact_scope=="windows-startup-folder-registration-only");assert(row.name=="App \xc3\xa9.lnk" || row.name=="script.cmd");}
    // Keep hidden regular files and equal labels in distinct folders, not reparse targets/directories/metadata.
    for(int m:{1,2,3,4,5,6,8,9,10,11,12,13,14,15,16,18,19,20,21,22,23,27,28,29,30,31}){
        start(m);bool failed=false;try{(void)read_windows_startup_folders(api());}catch(const std::system_error& e){failed=true;assert(e.code().value()!=0);}assert(failed && allocated==freed && opened==closed);
        if(m>=20)assert(opened==0 && path_calls==1); // Reject before enumerating any device/unsafe path.
    }
    start(7);assert(read_windows_startup_folders(api()).empty() && allocated==freed && opened==0 && path_calls==2);
    for(int m:{17,24,25,26}){start(m);assert(read_windows_startup_folders(api()).size()==4 && allocated==freed && opened==closed);}
    start(0);auto incomplete=api();incomplete.next=nullptr;bool failed=false;
    try{(void)read_windows_startup_folders(incomplete);}catch(const std::system_error&){failed=true;}assert(failed && path_calls==0);
    // Native read-only; do not create/modify/delete actual Startup files or load shortcut targets.
    const auto native=read_windows_startup_folders();ids.clear();for(const auto& row:native)assert(ids.insert(row.id).second && row.id.starts_with("winfolder:") && row.command.empty());
    Monitor monitor;SampleOptions options;options.include_processes=false;options.include_gpus=false;options.include_disks=false;options.include_networks=false;
    assert(monitor.sample(options).startup_entries.empty());options.include_startup_entries=true;
    const auto snapshot=monitor.sample(options);assert(snapshot.processes.empty());ids.clear();
    for(const auto& row:snapshot.startup_entries){assert(ids.insert(row.id).second && (row.id.starts_with("winrun:") || row.id.starts_with("winfolder:")));if(row.id.starts_with("winfolder:"))assert(row.command.empty() && !row.source.empty());}
    options.include_startup_entries=false;assert(monitor.sample(options).startup_entries.empty());
    std::cout<<"Startup known folders: inventory, bounds, cleanup, native reader and Monitor merge passed\n";
}
