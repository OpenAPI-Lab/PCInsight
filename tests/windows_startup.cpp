#include "../src/windows_startup.hpp"
#include "pcinsight/monitor.hpp"
#include <cassert>
#include <cstring>
#include <iostream>
#include <set>
#include <system_error>
using namespace pcinsight;
using namespace pcinsight::detail;
namespace {
int mode=0,opened=0,closed=0,attempts=0;int tokens[3];
void start(int value){assert(opened==closed);mode=value;opened=closed=attempts=0;}
void WINAPI info(LPSYSTEM_INFO out){out->wProcessorArchitecture=mode==17?PROCESSOR_ARCHITECTURE_ARM64:mode==18?PROCESSOR_ARCHITECTURE_INTEL:PROCESSOR_ARCHITECTURE_AMD64;}
LSTATUS WINAPI open(HKEY hive,LPCWSTR path,DWORD options,REGSAM access,PHKEY out){
    assert(std::wstring(path)==L"Software\\Microsoft\\Windows\\CurrentVersion\\Run" && !options);++attempts;
    const int scope=hive==HKEY_CURRENT_USER?0:(access&KEY_WOW64_64KEY)?1:2;
    assert(hive==HKEY_CURRENT_USER || hive==HKEY_LOCAL_MACHINE);
    assert(access==(KEY_QUERY_VALUE|(scope==0 || mode==18?0:scope==1?KEY_WOW64_64KEY:KEY_WOW64_32KEY)));
    if(mode==1)return ERROR_ACCESS_DENIED;
    if(mode==2)return ERROR_FILE_NOT_FOUND;
    if(mode==3 && scope==2)return ERROR_ACCESS_DENIED;
    *out=reinterpret_cast<HKEY>(&tokens[scope]);++opened;return ERROR_SUCCESS;
}
LSTATUS WINAPI close(HKEY key){assert(key==reinterpret_cast<HKEY>(&tokens[0]) || key==reinterpret_cast<HKEY>(&tokens[1]) || key==reinterpret_cast<HKEY>(&tokens[2]));++closed;return ERROR_SUCCESS;}
LSTATUS WINAPI enumerate(HKEY key,DWORD index,LPWSTR name,LPDWORD chars,LPDWORD reserved,LPDWORD type,LPBYTE data,LPDWORD bytes){
    assert(!reserved && *chars==16384 && *bytes==65536);const bool machine32=key==reinterpret_cast<HKEY>(&tokens[2]);
    if(mode==4)return ERROR_ACCESS_DENIED;
    if(mode==5)return ERROR_MORE_DATA;
    if(mode==6){*chars=16384;*bytes=0;return ERROR_SUCCESS;}
    if(mode==7){*chars=1;*bytes=65538;return ERROR_SUCCESS;}
    if(mode!=15 && mode!=16 && index==4)return ERROR_NO_MORE_ITEMS;
    if(mode==15 && index==10001)return ERROR_NO_MORE_ITEMS;
    std::wstring label=L"App/\u00e9:%";std::wstring command=L"\"C:\\Program Files\\App\\app.exe\" --literal & echo NEVER";
    *type=machine32?REG_EXPAND_SZ:REG_SZ;
    if(machine32)command=L"%ProgramFiles%\\App\\app.exe --unchanged";
    if(index==1 && mode!=15 && mode!=16){*type=REG_DWORD;label=L"unsupported-type";}
    if(index==2 && mode!=15 && mode!=16)label.clear();
    if(index==3 && mode!=15 && mode!=16){command.clear();label=L"empty-command";}
    if(mode==8)command=std::wstring(L"a\0b",3);
    if(mode==9)command=std::wstring(1,static_cast<wchar_t>(0xd800));
    if(mode==10)label=std::wstring(L"a\0b",3);
    if(mode==11 && index==1){*type=REG_SZ;label=L"App/\u00e9:%";} // Repeated value name, detected even if identical.
    if(mode==15 || mode==16)label=std::to_wstring(index);
    if(mode==16)command.assign(32767,L'x');
    *chars=static_cast<DWORD>(label.size());std::memcpy(name,label.c_str(),(label.size()+1)*sizeof(wchar_t));
    *bytes=static_cast<DWORD>((command.size()+1)*sizeof(wchar_t));std::memcpy(data,command.c_str(),*bytes);
    if(mode==12)*bytes=1;
    if(mode==13)*bytes-=sizeof(wchar_t); // Safe bounded acceptance without terminator.
    if(mode==14 && index>0)return ERROR_NO_MORE_ITEMS;
    return ERROR_SUCCESS;
}
WindowsStartupApi api(){return {open,enumerate,close,info};}
}
int main(){
    start(0);auto rows=read_windows_startup(api());
    assert(rows.size()==3 && opened==closed && opened==3 && attempts==3);std::set<std::string> ids;
    for(const auto& row:rows){assert(ids.insert(row.id).second && row.id.starts_with("winrun:") && row.name=="App/\xc3\xa9:%" && row.publisher.empty() && row.status==StartupStatus::unknown && row.impact==StartupImpact::not_measured && !row.xdg_standard_eligible.value && !row.observed_startup_impact.value);assert(row.source.find("registration only")!=std::string::npos);}
    assert(rows[0].id.find("4170702FC3A93A25")!=std::string::npos); // Unambiguous hex UTF-8 value name.
    for(const auto& row:rows)if(row.id.starts_with("winrun:hklm:32:"))assert(row.command=="%ProgramFiles%\\App\\app.exe --unchanged" && row.source.find("REG_EXPAND_SZ")!=std::string::npos);
    for(int m:{1,3,4,5,6,7,8,9,10,11,12,15,16,17}){
        start(m);bool failed=false;try{(void)read_windows_startup(api());}catch(const std::system_error& e){failed=true;assert(e.code().value()!=0);}
        assert(failed && opened==closed);
    }
    start(2);assert(read_windows_startup(api()).empty() && opened==0 && attempts==3);
    start(13);assert(read_windows_startup(api()).size()==3 && opened==closed);
    start(14);assert(read_windows_startup(api()).size()==3 && opened==closed);
    start(18);assert(read_windows_startup(api()).size()==2 && opened==closed && attempts==2);
    start(0);auto incomplete=api();incomplete.enumerate=nullptr;bool failed=false;
    try{(void)read_windows_startup(incomplete);}catch(const std::system_error&){failed=true;}assert(failed && attempts==0);
    // Native, read-only: do not create/delete/change actual Run entries.
    const auto native=read_windows_startup();ids.clear();for(const auto& row:native)assert(ids.insert(row.id).second && row.status==StartupStatus::unknown && !row.command.empty());
    Monitor monitor;SampleOptions options;options.include_processes=false;options.include_gpus=false;options.include_disks=false;options.include_networks=false;
    assert(monitor.sample(options).startup_entries.empty());options.include_startup_entries=true;
    const auto snapshot=monitor.sample(options);assert(snapshot.processes.empty());
    for(const auto& row:snapshot.startup_entries)assert((row.id.starts_with("winrun:") || row.id.starts_with("winfolder:")) && row.status==StartupStatus::unknown && row.impact==StartupImpact::not_measured);
    options.include_startup_entries=false;assert(monitor.sample(options).startup_entries.empty());
    std::cout<<"Run registry inventory, views, bounds, errors, native reader and Monitor opt-in passed\n";
}
