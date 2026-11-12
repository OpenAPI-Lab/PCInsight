#include "windows_startup.hpp"
#include "windows_startup_folders.hpp"
#ifdef _WIN32
#include <algorithm>
#include <set>
#include <string_view>
#include <system_error>
namespace pcinsight::detail {
namespace {
constexpr wchar_t path[]=L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";
[[noreturn]] void fail(LSTATUS error,const char* stage){throw std::system_error(error?error:ERROR_INVALID_DATA,std::system_category(),stage);}
struct Key {HKEY value;decltype(&RegCloseKey) close;~Key(){if(value)close(value);}};
std::string utf8(std::wstring_view text){
    if(text.find(L'\0')!=text.npos)fail(ERROR_INVALID_DATA,"Run entry has embedded NUL");
    if(text.empty())return {};
    const auto n=WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,text.data(),static_cast<int>(text.size()),nullptr,0,nullptr,nullptr);
    if(!n)fail(GetLastError(),"Run entry has invalid UTF-16");
    std::string out(n,'\0');
    if(WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,text.data(),static_cast<int>(text.size()),out.data(),n,nullptr,nullptr)!=n)fail(GetLastError(),"Run entry UTF-8 conversion failed");
    return out;
}
std::string hex(const std::string& value){
    constexpr char digits[]="0123456789ABCDEF";std::string out;out.reserve(value.size()*2);
    for(unsigned char c:value){out.push_back(digits[c>>4]);out.push_back(digits[c&15]);}return out;
}
}
std::vector<StartupEntry> read_windows_startup(WindowsStartupApi api){
    if(!api.open || !api.enumerate || !api.close || !api.native_info)fail(ERROR_NOT_SUPPORTED,"Incomplete Run registry API");
    SYSTEM_INFO info{};api.native_info(&info);
    // ARM64 has architecture-dependent alternate views; do not claim full coverage there.
    if(info.wProcessorArchitecture!=PROCESSOR_ARCHITECTURE_AMD64 && info.wProcessorArchitecture!=PROCESSOR_ARCHITECTURE_INTEL)fail(ERROR_NOT_SUPPORTED,"Run registry views unsupported on this native architecture");
    struct Scope {HKEY hive;REGSAM view;const char* id;};
    std::vector<Scope> scopes{{HKEY_CURRENT_USER,0,"hkcu:shared"}};
    if(info.wProcessorArchitecture==PROCESSOR_ARCHITECTURE_AMD64){scopes.push_back({HKEY_LOCAL_MACHINE,KEY_WOW64_64KEY,"hklm:64"});scopes.push_back({HKEY_LOCAL_MACHINE,KEY_WOW64_32KEY,"hklm:32"});}
    else scopes.push_back({HKEY_LOCAL_MACHINE,0,"hklm:32"});
    std::vector<StartupEntry> result;std::size_t payload=0,input_bytes=0;
    for(const auto& scope:scopes){
        HKEY raw=nullptr;const auto opened=api.open(scope.hive,path,0,KEY_QUERY_VALUE|scope.view,&raw);Key cleanup{raw,api.close};
        if(opened==ERROR_FILE_NOT_FOUND || opened==ERROR_PATH_NOT_FOUND)continue;
        if(opened!=ERROR_SUCCESS)fail(opened,"Run registry key open failed; no partial inventory");
        if(!raw)fail(ERROR_INVALID_HANDLE,"Run registry key missing handle");
        std::vector<wchar_t> name(16384),data(32768);std::set<std::wstring> names;
        for(DWORD index=0;;++index){
            DWORD chars=static_cast<DWORD>(name.size()),bytes=static_cast<DWORD>(data.size()*sizeof(wchar_t)),type=0;
            const auto error=api.enumerate(raw,index,name.data(),&chars,nullptr,&type,reinterpret_cast<LPBYTE>(data.data()),&bytes);
            if(error==ERROR_NO_MORE_ITEMS)break;
            if(error!=ERROR_SUCCESS)fail(error,"Run registry enumeration failed or exceeded field bound; no partial inventory");
            if(index>=10000 || chars>=name.size() || bytes>data.size()*sizeof(wchar_t))fail(ERROR_INVALID_DATA,"Run registry inventory exceeds bounds");
            const auto input_cost=chars*sizeof(wchar_t)+bytes;
            if(input_cost>16*1024*1024-input_bytes)fail(ERROR_INVALID_DATA,"Run registry raw text exceeds 16 MiB");
            input_bytes+=input_cost;
            const std::wstring value_name(name.data(),chars);
            if(!names.insert(value_name).second)fail(ERROR_INVALID_DATA,"Run registry changed/duplicate value name");
            if(type!=REG_SZ && type!=REG_EXPAND_SZ)continue;
            if(bytes%sizeof(wchar_t))fail(ERROR_INVALID_DATA,"Run registry invalid text byte length");
            std::wstring_view command(data.data(),bytes/sizeof(wchar_t));
            if(!command.empty() && command.back()==L'\0')command.remove_suffix(1);
            auto label=utf8(value_name),text=utf8(command);
            if(label.empty() || text.empty())continue;
            StartupEntry row;row.name=std::move(label);row.command=std::move(text);
            row.id="winrun:"+std::string(scope.id)+":"+hex(row.name);
            row.source="registry:"+std::string(scope.id)+"/Software/Microsoft/Windows/CurrentVersion/Run; "+(type==REG_SZ?"REG_SZ":"REG_EXPAND_SZ (unexpanded)")+"; registration only, approval unknown";
            row.status=StartupStatus::unknown;row.impact=StartupImpact::not_measured;
            const auto cost=row.id.size()+row.name.size()+row.command.size()+row.source.size();
            if(cost>16*1024*1024-payload)fail(ERROR_INVALID_DATA,"Run registry payload exceeds 16 MiB");
            payload+=cost;result.push_back(std::move(row));
        }
    }
    std::sort(result.begin(),result.end(),[](const auto& a,const auto& b){return a.id<b.id;});return result;
}
std::vector<StartupEntry> read_windows_startup_inventory(){
    auto rows=read_windows_startup();auto folders=read_windows_startup_folders();
    rows.reserve(rows.size()+folders.size());for(auto& row:folders)rows.push_back(std::move(row));
    std::sort(rows.begin(),rows.end(),[](const auto& a,const auto& b){return a.id<b.id;});return rows;
}
}
#endif
