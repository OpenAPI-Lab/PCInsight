#include "windows_startup_properties.hpp"
#include "windows_startup_file_properties.hpp"
#ifdef _WIN32
#include <algorithm>
#include <string_view>
namespace pcinsight::detail {
namespace {
StartupPropertiesResult fail(ActionError error,const char* message){return {{},error,message};}
StartupPropertiesResult native_error(LSTATUS code,const char* stage){
    auto error=code==ERROR_ACCESS_DENIED?ActionError::permission_denied:
        (code==ERROR_FILE_NOT_FOUND || code==ERROR_PATH_NOT_FOUND)?ActionError::not_found:
        code==ERROR_MORE_DATA?ActionError::stale_target:ActionError::system_error;
    return {{},error,std::string(stage)+"; Win32 error="+std::to_string(code)};
}
bool decode_name(std::string_view encoded,std::string& name,std::wstring& wide){
    if(encoded.empty() || encoded.size()%2 || encoded.size()>131064)return false;
    auto digit=[](char c)->int {return c>='0' && c<='9'?c-'0':c>='A' && c<='F'?c-'A'+10:-1;};
    name.reserve(encoded.size()/2);
    for(std::size_t i=0;i<encoded.size();i+=2){const auto hi=digit(encoded[i]),lo=digit(encoded[i+1]);if(hi<0 || lo<0)return false;name.push_back(static_cast<char>(hi*16+lo));}
    if(name.find('\0')!=name.npos)return false;
    const auto count=MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,name.data(),static_cast<int>(name.size()),nullptr,0);
    if(count<=0 || count>16383)return false;
    wide.resize(count);return MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,name.data(),static_cast<int>(name.size()),wide.data(),count)==count;
}
bool utf8(std::wstring_view value,std::string& out){
    if(value.empty() || value.find(L'\0')!=value.npos)return false;
    const auto count=WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,value.data(),static_cast<int>(value.size()),nullptr,0,nullptr,nullptr);
    if(count<=0)return false;
    out.resize(count);return WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,value.data(),static_cast<int>(value.size()),out.data(),count,nullptr,nullptr)==count;
}
}
StartupPropertiesResult get_windows_startup_properties(const std::string& id,WindowsStartupPropertiesApi api){
    if(id.starts_with("winfolder:"))return get_windows_startup_file_properties(id);
    if(id.empty() || id.size()>131128 || id.find('\0')!=id.npos)return fail(ActionError::invalid_argument,"Invalid/excessive Windows Startup ID");
    if(!id.starts_with("winrun:"))return fail(ActionError::not_supported,"Properties currently supports Windows Run registration IDs only");
    struct Scope{const char* prefix;HKEY hive;REGSAM view;const char* label;const char* hive_name;const char* view_name;};
    const Scope scopes[]{
        {"winrun:hkcu:shared:",HKEY_CURRENT_USER,0,"hkcu:shared","HKEY_CURRENT_USER","shared"},
        {"winrun:hklm:64:",HKEY_LOCAL_MACHINE,KEY_WOW64_64KEY,"hklm:64","HKEY_LOCAL_MACHINE","64"},
        {"winrun:hklm:32:",HKEY_LOCAL_MACHINE,KEY_WOW64_32KEY,"hklm:32","HKEY_LOCAL_MACHINE","32"}};
    const Scope* scope=nullptr;
    for(const auto& value:scopes)if(id.starts_with(value.prefix)){scope=&value;break;}
    if(!scope)return fail(ActionError::invalid_argument,"Unrecognized Windows Run scope");
    std::string name;std::wstring wide;
    if(!decode_name(std::string_view(id).substr(std::char_traits<char>::length(scope->prefix)),name,wide))return fail(ActionError::invalid_argument,"Run ID name is not canonical uppercase hex of valid UTF-8");
    if(!api.open || !api.query || !api.close || !api.native_info)return fail(ActionError::not_supported,"Incomplete Run properties API");
    SYSTEM_INFO info{};api.native_info(&info);
    if(info.wProcessorArchitecture!=PROCESSOR_ARCHITECTURE_AMD64 && info.wProcessorArchitecture!=PROCESSOR_ARCHITECTURE_INTEL)return fail(ActionError::not_supported,"Run registry views unsupported on this native architecture");
    if(info.wProcessorArchitecture==PROCESSOR_ARCHITECTURE_INTEL && scope->view==KEY_WOW64_64KEY)return fail(ActionError::not_supported,"No 64-bit Run view on native x86");
    const REGSAM view=info.wProcessorArchitecture==PROCESSOR_ARCHITECTURE_INTEL?0:scope->view;
    constexpr wchar_t key_path[]=L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";
    HKEY raw=nullptr;const auto opened=api.open(scope->hive,key_path,0,KEY_QUERY_VALUE|view,&raw);
    struct Key{HKEY key;decltype(&RegCloseKey) close;~Key(){if(key)close(key);}} cleanup{raw,api.close};
    if(opened!=ERROR_SUCCESS)return native_error(opened,"Run properties key open failed");
    if(!raw)return fail(ActionError::system_error,"Run properties key has no handle");
    std::vector<wchar_t> first(32768),second(32768);DWORD bytes=65536,type=0;
    auto code=api.query(raw,wide.c_str(),nullptr,&type,reinterpret_cast<LPBYTE>(first.data()),&bytes);
    if(code!=ERROR_SUCCESS)return native_error(code,"Run properties value query failed");
    if(bytes>65536 || bytes%sizeof(wchar_t))return fail(ActionError::invalid_argument,"Run value has invalid/excessive byte size");
    if(type!=REG_SZ && type!=REG_EXPAND_SZ)return fail(ActionError::not_supported,"Run value is not REG_SZ or REG_EXPAND_SZ");
    DWORD final_bytes=65536,final_type=0;
    code=api.query(raw,wide.c_str(),nullptr,&final_type,reinterpret_cast<LPBYTE>(second.data()),&final_bytes);
    if(code==ERROR_FILE_NOT_FOUND || code==ERROR_PATH_NOT_FOUND || code==ERROR_MORE_DATA)return fail(ActionError::stale_target,"Run value disappeared/grew during properties query");
    if(code!=ERROR_SUCCESS)return native_error(code,"Run properties revalidation failed");
    if(final_bytes!=bytes || final_type!=type || !std::equal(first.begin(),first.begin()+bytes/sizeof(wchar_t),second.begin()))return fail(ActionError::stale_target,"Run value changed during properties query");
    std::wstring_view command(first.data(),bytes/sizeof(wchar_t));
    if(!command.empty() && command.back()==L'\0')command.remove_suffix(1);
    if(command.empty())return fail(ActionError::not_found,"Run value is empty; not an application registration");
    std::string text;if(!utf8(command,text))return fail(ActionError::invalid_argument,"Run value is not bounded valid UTF-16 text");
    StartupProperties result;auto& row=result.entry;
    row.id=id;row.name=name;row.command=std::move(text);
    row.source="registry:"+std::string(scope->label)+"/Software/Microsoft/Windows/CurrentVersion/Run; "+(type==REG_SZ?"REG_SZ":"REG_EXPAND_SZ (unexpanded)")+"; registration only, approval unknown";
    result.source_file_size_bytes.note="Registry value source; no source file size applies (legacy file_size_bytes is Linux-only)";
    result.fields={{"Windows Run","hive",scope->hive_name},{"Windows Run","view",scope->view_name},
        {"Windows Run","key","Software\\Microsoft\\Windows\\CurrentVersion\\Run"},{"Windows Run","value_name",name},
        {"Windows Run","value_type",type==REG_SZ?"REG_SZ":"REG_EXPAND_SZ"},{"Windows Run","command_unexpanded",row.command}};
    return {std::move(result),ActionError::none,"Run registration observed twice unchanged; not approval, execution identity or an atomic snapshot"};
}
}
#endif
