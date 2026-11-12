#include "../src/windows_startup_properties.hpp"
#include <cassert>
#include <algorithm>
#include <cstring>
#include <iostream>
using namespace pcinsight;
using namespace pcinsight::detail;
namespace {
unsigned opens=0,queries=0,closes=0;
WORD architecture=PROCESSOR_ARCHITECTURE_AMD64;
HKEY expected_hive=HKEY_CURRENT_USER;
REGSAM expected_view=0;
LSTATUS open_error=ERROR_SUCCESS,query_error=ERROR_SUCCESS;
bool null_handle=false,change=false,remove_second=false,odd=false,oversized=false;
DWORD value_type=REG_EXPAND_SZ;
std::wstring expected_name=L"Ab\x00e9";
std::vector<wchar_t> value;
HKEY token=reinterpret_cast<HKEY>(static_cast<std::uintptr_t>(321));
void reset(){
    opens=queries=closes=0;architecture=PROCESSOR_ARCHITECTURE_AMD64;
    expected_hive=HKEY_CURRENT_USER;expected_view=0;
    open_error=query_error=ERROR_SUCCESS;null_handle=change=remove_second=odd=oversized=false;
    value_type=REG_EXPAND_SZ;expected_name=L"Ab\x00e9";
    const std::wstring command=L"\"%APP%\\Never.exe\" --literal & no-shell";
    value.assign(command.begin(),command.end());value.push_back(L'\0');
}
void WINAPI native_info(LPSYSTEM_INFO out){*out={};out->wProcessorArchitecture=architecture;}
LSTATUS WINAPI open(HKEY hive,LPCWSTR key,DWORD options,REGSAM access,PHKEY out){
    ++opens;assert(hive==expected_hive && std::wstring(key)==L"Software\\Microsoft\\Windows\\CurrentVersion\\Run");
    assert(options==0 && access==(KEY_QUERY_VALUE|expected_view));
    *out=null_handle?nullptr:token;return open_error;
}
LSTATUS WINAPI close(HKEY key){++closes;assert(key==token);return ERROR_SUCCESS;}
LSTATUS WINAPI query(HKEY key,LPCWSTR name,LPDWORD reserved,LPDWORD type,LPBYTE data,LPDWORD bytes){
    ++queries;assert(key==token && std::wstring(name)==expected_name && !reserved && *bytes==65536 && data);
    if(queries==2 && remove_second)return ERROR_FILE_NOT_FOUND;
    if(query_error!=ERROR_SUCCESS)return query_error;
    *type=value_type;*bytes=static_cast<DWORD>(value.size()*sizeof(wchar_t));
    if(odd){*bytes=3;return ERROR_SUCCESS;}
    if(oversized){*bytes=65538;return ERROR_SUCCESS;}
    assert(*bytes<=65536);std::memcpy(data,value.data(),*bytes);
    if(queries==2 && change && *bytes)reinterpret_cast<wchar_t*>(data)[0]^=1;
    return ERROR_SUCCESS;
}
const std::string id="winrun:hkcu:shared:4162C3A9";
WindowsStartupPropertiesApi api(){WindowsStartupPropertiesApi a;a.open=open;a.query=query;a.close=close;a.native_info=native_info;return a;}
StartupPropertiesResult read(){return get_windows_startup_properties(id,api());}
}
int main(){
    reset();auto result=read();assert(result && opens==1 && queries==2 && closes==1);
    const auto& properties=*result.value;
    assert(properties.entry.id==id && properties.entry.name=="Ab\xc3\xa9");
    assert(properties.entry.command=="\"%APP%\\Never.exe\" --literal & no-shell" && properties.entry.status==StartupStatus::unknown && properties.entry.impact==StartupImpact::not_measured);
    assert(properties.entry.publisher.empty() && !properties.entry.windows_signature.value && !properties.entry.windows_shortcut.value);
    assert(!properties.source_file_size_bytes.value && properties.source_file_size_bytes.support==Support::unsupported && properties.fields.size()==6);
    assert(properties.fields[4].raw_value=="REG_EXPAND_SZ" && properties.fields[5].raw_value==properties.entry.command);
    reset();value.pop_back();value_type=REG_SZ;result=read();assert(result && result.value->fields[4].raw_value=="REG_SZ" && queries==2 && closes==1); // Bounded, non-terminated REG_SZ is valid.
    reset();value={L'x',L'\0',L'y',L'\0'};assert(read().error==ActionError::invalid_argument && closes==1);
    reset();value={static_cast<wchar_t>(0xd800),L'\0'};assert(read().error==ActionError::invalid_argument && closes==1);
    reset();value={L'\0'};assert(read().error==ActionError::not_found && closes==1);
    reset();value.assign(32768,L'x');assert(read() && closes==1); // Exact 64 KiB data bound, no terminator.
    reset();odd=true;assert(read().error==ActionError::invalid_argument && closes==1);
    reset();oversized=true;assert(read().error==ActionError::invalid_argument && closes==1);
    reset();value_type=REG_BINARY;assert(read().error==ActionError::not_supported && queries==1 && closes==1);
    reset();change=true;assert(read().error==ActionError::stale_target && closes==1);
    reset();remove_second=true;assert(read().error==ActionError::stale_target && closes==1);
    for(const auto error:{ERROR_ACCESS_DENIED,ERROR_FILE_NOT_FOUND,ERROR_MORE_DATA,ERROR_INVALID_DATA}){
        reset();query_error=error;const auto r=read();assert(!r && closes==1);
        assert(r.error==(error==ERROR_ACCESS_DENIED?ActionError::permission_denied:error==ERROR_FILE_NOT_FOUND?ActionError::not_found:error==ERROR_MORE_DATA?ActionError::stale_target:ActionError::system_error));
    }
    reset();open_error=ERROR_ACCESS_DENIED;assert(read().error==ActionError::permission_denied && queries==0 && closes==1); // Even an unusual non-null failure handle is released.
    reset();null_handle=true;assert(read().error==ActionError::system_error && queries==0 && closes==0);
    for(const auto* bad:{"winrun:hkcu:shared:","winrun:hkcu:shared:4","winrun:hkcu:shared:GG","winrun:hkcu:shared:00","winrun:hkcu:shared:FF","winrun:hkcu:shared:c3a9","winrun:hklm:shared:41","winrun:hkcu:64:41"}){
        reset();assert(get_windows_startup_properties(bad,api()).error==ActionError::invalid_argument && opens==0);
    }
    reset();assert(get_windows_startup_properties(std::string(131129,'x'),api()).error==ActionError::invalid_argument && opens==0);
    reset();assert(get_windows_startup_properties("winfolder:user:fixture",api()).error==ActionError::invalid_argument && opens==0);
    reset();auto incomplete=api();incomplete.query=nullptr;assert(get_windows_startup_properties(id,incomplete).error==ActionError::not_supported && opens==0);
    reset();architecture=PROCESSOR_ARCHITECTURE_ARM64;assert(read().error==ActionError::not_supported && opens==0);
    for(const auto* view:{"64","32"}){
        reset();expected_hive=HKEY_LOCAL_MACHINE;expected_view=std::string(view)=="64"?KEY_WOW64_64KEY:KEY_WOW64_32KEY;
        assert(get_windows_startup_properties("winrun:hklm:"+std::string(view)+":4162C3A9",api()) && closes==1);
    }
    reset();architecture=PROCESSOR_ARCHITECTURE_INTEL;assert(get_windows_startup_properties("winrun:hklm:64:4162C3A9",api()).error==ActionError::not_supported && opens==0);
    reset();architecture=PROCESSOR_ARCHITECTURE_INTEL;expected_hive=HKEY_LOCAL_MACHINE;expected_view=0;assert(get_windows_startup_properties("winrun:hklm:32:4162C3A9",api()) && closes==1);
    assert(get_startup_properties("").error==ActionError::invalid_argument);
    assert(get_startup_properties(std::string("winrun:\0x",9)).error==ActionError::invalid_argument);
    unsigned observed=0;
    for(const auto& row:read_windows_startup()){
        const auto native=get_startup_properties(row.id);
        if(!native){assert(native.error==ActionError::stale_target || native.error==ActionError::not_found || native.error==ActionError::permission_denied);continue;}
        assert(native.value->entry.id==row.id && native.value->entry.name==row.name && !native.value->entry.command.empty() && !native.value->source_file_size_bytes.value);
        ++observed;if(observed==3)break;
    }
    std::cout<<"Windows Run Properties: injected views, raw strings, malformed IDs/data, races and cleanup passed; native registrations observed="<<observed<<'\n';
}
