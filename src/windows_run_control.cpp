#include "windows_run_control.hpp"
#ifdef _WIN32
#include <algorithm>
#include <cstring>
#include <system_error>
#include <map>
#include <set>
#include <string_view>
namespace pcinsight::detail {
namespace {
struct Failure {ActionError error;std::string message;};
[[noreturn]] void fail(ActionError code,const char* message){throw Failure{code,message};}
[[noreturn]] void native(DWORD code,const char* stage){
    const auto error=(code==ERROR_ACCESS_DENIED || code==ERROR_PRIVILEGE_NOT_HELD)?ActionError::permission_denied:
        (code==ERROR_TRANSACTIONAL_CONFLICT || code==ERROR_TRANSACTION_NOT_ACTIVE || code==ERROR_MORE_DATA)?ActionError::stale_target:
        (code==ERROR_NOT_SUPPORTED || code==ERROR_CALL_NOT_IMPLEMENTED || code==ERROR_TRANSACTIONS_UNSUPPORTED_REMOTE || code==ERROR_RM_NOT_ACTIVE)?ActionError::not_supported:
        (code==ERROR_FILE_NOT_FOUND || code==ERROR_PATH_NOT_FOUND)?ActionError::not_found:ActionError::system_error;
    throw Failure{error,std::string(stage)+"; Win32 error="+std::to_string(code)};
}
struct Scope {std::string_view prefix;REGSAM view{};bool machine{};};
constexpr Scope user_scope{"winrun:hkcu:shared:",0,false};
Scope machine_scope(WindowsMachineRunView view){
    switch(view){
    case WindowsMachineRunView::registry32:return {"winrun:hklm:32:",KEY_WOW64_32KEY,true};
    case WindowsMachineRunView::registry64:return {"winrun:hklm:64:",KEY_WOW64_64KEY,true};
    }
    fail(ActionError::invalid_argument,"Invalid machine Run registry view");
}
Scope scope_from_id(const std::string& id){
    if(id.empty() || id.size()>131128 || id.find('\0')!=id.npos)fail(ActionError::invalid_argument,"Invalid/excessive managed Run ID");
    if(id.starts_with(user_scope.prefix))return user_scope;
    if(id.starts_with("winrun:hklm:32:"))return machine_scope(WindowsMachineRunView::registry32);
    if(id.starts_with("winrun:hklm:64:"))return machine_scope(WindowsMachineRunView::registry64);
    fail(ActionError::not_supported,"Run ID must identify HKCU/shared or HKLM/32/64");
}
void validate_scope(Scope scope,const WindowsRunControlApi& api){
    if(!api.native_info)fail(ActionError::not_supported,"Native Run architecture API missing");
    SYSTEM_INFO info{};api.native_info(&info);
    if(info.wProcessorArchitecture!=PROCESSOR_ARCHITECTURE_AMD64 && info.wProcessorArchitecture!=PROCESSOR_ARCHITECTURE_INTEL)
        fail(ActionError::not_supported,"Managed Run architecture unsupported");
    if(scope.view==KEY_WOW64_64KEY && info.wProcessorArchitecture!=PROCESSOR_ARCHITECTURE_AMD64)
        fail(ActionError::not_supported,"64-bit machine Run view unavailable on this native architecture; no 32-bit fallback");
}
LSTATUS open_root(Scope scope,REGSAM access,PHKEY out,const WindowsRunControlApi& api){
    if(!scope.machine)return api.user(access,out);
    // Pin a real read-only Software handle, not an empty predefined-key alias.
    // Child operations request their own rights/view; no writable hive root or elevation.
    return api.open_plain(HKEY_LOCAL_MACHINE,L"Software",0,KEY_READ|scope.view,out);
}
const wchar_t* relative_path(Scope scope,const wchar_t* path){return scope.machine?path+9:path;} // Software is already pinned for HKLM.
std::wstring name_from_id(const std::string& id,Scope scope){
    if(id.empty() || id.size()>131128 || id.find('\0')!=id.npos)fail(ActionError::invalid_argument,"Invalid/excessive managed Run ID");
    const auto encoded=std::string_view(id).substr(scope.prefix.size());
    if(encoded.empty() || encoded.size()%2 || encoded.size()>131064)fail(ActionError::invalid_argument,"Invalid managed Run ID size");
    const auto digit=[](char c){return c>='0' && c<='9'?c-'0':c>='A' && c<='F'?c-'A'+10:-1;};
    std::string name;name.reserve(encoded.size()/2);
    for(std::size_t i=0;i<encoded.size();i+=2){const auto hi=digit(encoded[i]),lo=digit(encoded[i+1]);if(hi<0 || lo<0)fail(ActionError::invalid_argument,"Run ID must be canonical uppercase hex");name+=static_cast<char>(hi*16+lo);}
    if(name.find('\0')!=name.npos)fail(ActionError::invalid_argument,"Run value name contains NUL");
    const auto count=MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,name.data(),static_cast<int>(name.size()),nullptr,0);
    if(count<=0 || count>16383)fail(ActionError::invalid_argument,"Invalid/excessive Run name UTF-8");
    std::wstring out(count,L'\0');if(MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,name.data(),static_cast<int>(name.size()),out.data(),count)!=count)native(GetLastError(),"Run name conversion failed");return out;
}
struct Value {bool present=false;DWORD type=0;std::vector<std::uint8_t> bytes;bool operator==(const Value&) const=default;};
Value query(HKEY key,const std::wstring& name,const WindowsRunControlApi& api){
    if(!key)return {};
    Value value;value.bytes.resize(65536);DWORD size=65536;
    const auto code=api.query(key,name.c_str(),nullptr,&value.type,value.bytes.data(),&size);
    if(code==ERROR_FILE_NOT_FOUND)return {};
    if(code!=ERROR_SUCCESS)native(code,"Managed Run query failed");
    if(size>65536)fail(ActionError::invalid_argument,"Run value exceeds 64 KiB");
    value.present=true;value.bytes.resize(size);return value;
}
std::string command(const Value& value,bool action){
    if(value.type!=REG_SZ && value.type!=REG_EXPAND_SZ)fail(ActionError::not_supported,"Managed Run requires REG_SZ/REG_EXPAND_SZ");
    if(value.bytes.empty() || value.bytes.size()%sizeof(wchar_t))fail(ActionError::invalid_argument,"Invalid Run text byte count");
    std::wstring text(value.bytes.size()/sizeof(wchar_t),L'\0');std::memcpy(text.data(),value.bytes.data(),value.bytes.size());
    if(text.back()==L'\0')text.pop_back();
    if(text.empty() || text.find(L'\0')!=text.npos)fail(ActionError::invalid_argument,"Empty Run command or embedded NUL");
    if(action && text.size()>260)fail(ActionError::not_supported,"Managed action exceeds documented 260-character Run command bound");
    const auto count=WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,text.data(),static_cast<int>(text.size()),nullptr,0,nullptr,nullptr);
    if(count<=0)fail(ActionError::invalid_argument,"Run command has invalid UTF-16");
    std::string out(count,'\0');if(WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,text.data(),static_cast<int>(text.size()),out.data(),count,nullptr,nullptr)!=count)native(GetLastError(),"Run command conversion failed");return out;
}
WindowsRunRegistration observed(const std::string& id,const Value& live,const Value& saved,bool action){
    WindowsRunRegistration result;result.entry_id=id;
    using State=WindowsRunRegistrationState;
    result.state=live.present?(saved.present?State::conflict:State::registered):(saved.present?State::disabled_by_pcinsight:State::absent);
    const auto& selected=live.present?live:saved;
    if(result.state==State::registered || result.state==State::disabled_by_pcinsight){result.command=command(selected,action);result.native_type=selected.type;result.native_data=selected.bytes;}
    return result;
}
struct NameLess {
    bool operator()(const std::wstring& a,const std::wstring& b) const {
        const auto compared=CompareStringOrdinal(a.data(),static_cast<int>(a.size()),b.data(),static_cast<int>(b.size()),TRUE);
        if(!compared)native(GetLastError(),"Run value name comparison failed");
        return compared==CSTR_LESS_THAN;
    }
};
std::string utf8_name(const std::wstring& name){
    if(name.empty() || name.find(L'\0')!=name.npos)fail(ActionError::invalid_argument,"Empty/embedded-NUL Run name");
    const auto count=WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,name.data(),static_cast<int>(name.size()),nullptr,0,nullptr,nullptr);
    if(count<=0)fail(ActionError::invalid_argument,"Run inventory name has invalid UTF-16");
    std::string out(count,'\0');if(WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,name.data(),static_cast<int>(name.size()),out.data(),count,nullptr,nullptr)!=count)native(GetLastError(),"Run inventory name conversion failed");return out;
}
std::vector<WindowsRunRegistration> list_observed(const WindowsRunControlApi& api,Scope scope){
    if((!scope.machine && !api.user) || !api.open_plain || !api.enumerate || !api.close_key || !api.native_info)fail(ActionError::not_supported,"Incomplete Run listing API");
    validate_scope(scope,api);
    struct Key{HKEY h{};const WindowsRunControlApi& api;~Key(){if(h)api.close_key(h);}};
    Key user{nullptr,api};const auto code=open_root(scope,KEY_READ,&user.h,api);
    if(code!=ERROR_SUCCESS)native(code,"Run listing selected hive open failed");
    if(!user.h)fail(ActionError::system_error,"Run listing selected hive handle missing");
    using Values=std::map<std::wstring,Value,NameLess>;
    std::size_t input=0;
    auto scan=[&](const wchar_t* path,bool backup){
        Values values;Key key{nullptr,api};const auto opened=api.open_plain(user.h,relative_path(scope,path),0,KEY_QUERY_VALUE|scope.view,&key.h);
        if(opened==ERROR_FILE_NOT_FOUND || opened==ERROR_PATH_NOT_FOUND)return values;
        if(opened!=ERROR_SUCCESS)native(opened,"Run listing key open failed");
        if(!key.h)fail(ActionError::system_error,"Run listing key handle missing");
        std::vector<wchar_t> name(16384);std::vector<std::uint8_t> bytes(65536);std::set<std::wstring,NameLess> names;
        for(DWORD index=0;;++index){
            DWORD chars=16384,size=65536,type=0;const auto status=api.enumerate(key.h,index,name.data(),&chars,nullptr,&type,bytes.data(),&size);
            if(status==ERROR_NO_MORE_ITEMS)break;
            if(status!=ERROR_SUCCESS)native(status,"Run listing enumeration failed; no partial result");
            if(index>=10000 || chars>=16384 || size>65536)fail(ActionError::invalid_argument,"Run listing enumeration exceeds field/row bounds");
            const auto cost=chars*sizeof(wchar_t)+size;
            if(cost>16*1024*1024-input)fail(ActionError::invalid_argument,"Run listing four-pass raw input exceeds 16 MiB");
            input+=cost;
            std::wstring label(name.data(),chars);
            if(label.empty()){if(backup)fail(ActionError::invalid_argument,"Unnamed PCInsight backup is invalid");continue;}
            (void)utf8_name(label);
            if(!names.insert(label).second)fail(ActionError::stale_target,"Duplicate/case-equivalent Run value during enumeration");
            if(type!=REG_SZ && type!=REG_EXPAND_SZ){if(backup)fail(ActionError::not_supported,"PCInsight backup has unsupported value type");continue;}
            Value value{true,type,{bytes.begin(),bytes.begin()+size}};
            const bool empty=size==0 || (size==sizeof(wchar_t) && bytes[0]==0 && bytes[1]==0);
            if(empty && !backup)continue;
            (void)command(value,false); // No silently discarded malformed textual commands/backups.
            values.emplace(std::move(label),std::move(value));
        }
        return values;
    };
    const auto live=scan(managed_run_path,false),saved=scan(managed_run_backup_path,true);
    const auto final_live=scan(managed_run_path,false),final_saved=scan(managed_run_backup_path,true);
    if(live!=final_live || saved!=final_saved)fail(ActionError::stale_target,"Run/backup listing changed between observations");
    std::set<std::wstring,NameLess> names;for(const auto& [name,value]:live)names.insert(name);for(const auto& [name,value]:saved)names.insert(name);
    std::vector<WindowsRunRegistration> result;std::size_t output=0;constexpr char hex[]="0123456789ABCDEF";
    for(const auto& name:names){
        const auto encoded=utf8_name(name);std::string id(scope.prefix);
        for(unsigned char c:encoded){id+=hex[c>>4];id+=hex[c&15];}
        const auto l=live.find(name),s=saved.find(name);
        auto row=observed(id,l==live.end()?Value{}:l->second,s==saved.end()?Value{}:s->second,false);
        const auto cost=row.entry_id.size()+row.command.size()+row.native_data.size();
        if(cost>16*1024*1024-output)fail(ActionError::invalid_argument,"Run listing output exceeds 16 MiB");
        output+=cost;
        result.push_back(std::move(row));
    }
    return result;
}
WindowsRunRegistration read_observed(const std::string& id,const WindowsRunControlApi& api){
    const auto scope=scope_from_id(id);const auto name=name_from_id(id,scope);
    if((!scope.machine && !api.user) || !api.open_plain || !api.query || !api.close_key || !api.native_info)fail(ActionError::not_supported,"Incomplete read-only Run API");
    validate_scope(scope,api);
    struct Key{HKEY h{};const WindowsRunControlApi& api;~Key(){if(h)api.close_key(h);}};
    Key user{nullptr,api};auto code=open_root(scope,KEY_READ,&user.h,api);
    if(code!=ERROR_SUCCESS)native(code,"Read-only selected hive open failed");
    if(!user.h)fail(ActionError::system_error,"Read-only selected hive handle missing");
    auto read=[&](const wchar_t* path){
        Key key{nullptr,api};const auto opened=api.open_plain(user.h,relative_path(scope,path),0,KEY_QUERY_VALUE|scope.view,&key.h);
        if(opened==ERROR_FILE_NOT_FOUND || opened==ERROR_PATH_NOT_FOUND)return Value{};
        if(opened!=ERROR_SUCCESS)native(opened,"Read-only Run key open failed");
        if(!key.h)fail(ActionError::system_error,"Read-only Run key handle missing");
        return query(key.h,name,api);
    };
    const auto live=read(managed_run_path),saved=read(managed_run_backup_path);
    const auto final_live=read(managed_run_path),final_saved=read(managed_run_backup_path);
    const auto same=[](const Value& a,const Value& b){return a.present==b.present && (!a.present || (a.type==b.type && a.bytes==b.bytes));};
    if(!same(live,final_live) || !same(saved,final_saved))fail(ActionError::stale_target,"Run/backup changed between observations; reload, no atomic snapshot");
    return observed(id,live,saved,false);
}
WindowsRunRegistration perform(const std::string& id,const WindowsRunRegistration* expected,bool enabled,WindowsRunControlApi api){
    const auto scope=scope_from_id(id);const auto name=name_from_id(id,scope);
    if((!scope.machine && !api.user) || (scope.machine && !api.open_plain) || !api.open || !api.create || !api.query || !api.set || !api.erase || !api.close_key || !api.begin || !api.commit || !api.rollback || !api.close || !api.native_info)fail(ActionError::not_supported,"Incomplete managed Run transaction API");
    validate_scope(scope,api);
    struct Key {HKEY h{};const WindowsRunControlApi& api;~Key(){if(h)api.close_key(h);}};
    Key user{nullptr,api};const auto access=KEY_QUERY_VALUE|(expected?KEY_SET_VALUE:0)|scope.view;
    const auto opened=open_root(scope,expected?(KEY_READ|KEY_WRITE):KEY_READ,&user.h,api);
    if(opened!=ERROR_SUCCESS)native(opened,"Selected hive open failed");
    if(!user.h)fail(ActionError::system_error,"Selected hive handle missing");
    struct Transaction {HANDLE h;const WindowsRunControlApi& api;bool done=false;~Transaction(){if(h && h!=INVALID_HANDLE_VALUE){if(!done)api.rollback(h);api.close(h);}}};
    Transaction transaction{api.begin(nullptr,nullptr,0,0,0,5000,nullptr),api};
    if(!transaction.h || transaction.h==INVALID_HANDLE_VALUE)native(GetLastError(),"Managed Run transaction unavailable");
    Key run{nullptr,api},backup{nullptr,api};
    auto open=[&](const wchar_t* path,Key& key){const auto code=api.open(user.h,relative_path(scope,path),0,access,&key.h,transaction.h,nullptr);if(code!=ERROR_SUCCESS && code!=ERROR_FILE_NOT_FOUND && code!=ERROR_PATH_NOT_FOUND)native(code,"Managed Run transacted key open failed");if(code==ERROR_SUCCESS && !key.h)fail(ActionError::system_error,"Transacted key handle missing");};
    open(managed_run_path,run);open(managed_run_backup_path,backup);
    const auto live=query(run.h,name,api),saved=query(backup.h,name,api);
    auto result=observed(id,live,saved,expected!=nullptr);
    using State=WindowsRunRegistrationState;
    const auto& selected=live.present?live:saved;
    if(!expected)return result; // Read-only transaction rolled back/closed; no key creation.
    const auto required=enabled?State::disabled_by_pcinsight:State::registered;
    if(expected->state!=required || result.state!=required || expected->command!=result.command || expected->native_type!=result.native_type || expected->native_data!=result.native_data)fail(ActionError::stale_target,"Managed Run state/content differs; reload, never overwrite a conflict");
    Key& destination=enabled?run:backup;Key& source=enabled?backup:run;
    if(!destination.h){DWORD disposition=0;const auto code=api.create(user.h,relative_path(scope,enabled?managed_run_path:managed_run_backup_path),0,nullptr,REG_OPTION_NON_VOLATILE,access,nullptr,&destination.h,&disposition,transaction.h,nullptr);if(code!=ERROR_SUCCESS)native(code,"Managed Run destination create failed");if(!destination.h)fail(ActionError::system_error,"Managed Run created key handle missing");}
    // No overwriting even when another participant creates a destination value.
    if(query(destination.h,name,api).present)fail(ActionError::stale_target,"Managed Run destination already exists");
    auto code=api.set(destination.h,name.c_str(),0,selected.type,selected.bytes.data(),static_cast<DWORD>(selected.bytes.size()));
    if(code!=ERROR_SUCCESS)native(code,"Managed Run destination write failed");
    code=api.erase(source.h,name.c_str());if(code!=ERROR_SUCCESS)native(code,"Managed Run source removal failed");
    if(!api.commit(transaction.h))native(GetLastError(),"Managed Run commit failed; outcome unconfirmed, requery before retry");
    transaction.done=true;result.state=enabled?State::registered:State::disabled_by_pcinsight;return result;
}
}
Metric<WindowsRunRegistration> read_windows_run_registration(const std::string& id,WindowsRunControlApi api){
    try{return {read_observed(id,api),Support::estimated,"Two matching read-only Run/backup observations, NOT atomic; no KTM required. Not authenticated authorship, Windows approval, login eligibility or launch success"};}
    catch(const Failure& e){return {{},e.error==ActionError::permission_denied?Support::permission_required:Support::unsupported,e.message};}
}
Metric<std::vector<WindowsRunRegistration>> list_windows_run_registrations(WindowsRunControlApi api){
    try{return {list_observed(api,user_scope),Support::estimated,"Two matching bounded HKCU Run/PCInsight-backup enumerations, NOT atomic or Windows approval; no KTM or mutation"};}
    catch(const Failure& e){return {{},e.error==ActionError::permission_denied?Support::permission_required:Support::unsupported,e.message};}
}
Metric<std::vector<WindowsRunRegistration>> list_windows_machine_run_registrations(WindowsMachineRunView view,WindowsRunControlApi api){
    try{return {list_observed(api,machine_scope(view)),Support::estimated,"Two matching bounded HKLM Run/PCInsight-backup enumerations in the explicit 32/64-bit view; NOT atomic, Windows approval or authenticated authorship; no KTM or mutation"};}
    catch(const Failure& e){return {{},e.error==ActionError::permission_denied?Support::permission_required:Support::unsupported,e.message};}
}
ActionResult set_windows_run_registration_enabled(const WindowsRunRegistration& expected,bool enabled,WindowsRunControlApi api){
    try{perform(expected.entry_id,&expected,enabled,api);return {ActionError::none,enabled?"Run registration restored in selected hive/view; HKLM can affect all users; Windows approval unchanged, not guaranteed to launch":"Run registration moved to PCInsight backup in selected hive/view; HKLM can affect all users; other startup sources/running processes unchanged"};}
    catch(const Failure& e){return {e.error,e.message};}
}
}
#endif
namespace pcinsight {
Metric<std::vector<WindowsRunRegistration>> list_windows_machine_run_registrations(WindowsMachineRunView view){
#ifdef _WIN32
    return detail::list_windows_machine_run_registrations(view,{});
#else
    (void)view;return {{},Support::unsupported,"Machine Run registration listing requires Windows"};
#endif
}
Metric<std::vector<WindowsRunRegistration>> list_windows_run_registrations(){
#ifdef _WIN32
    return detail::list_windows_run_registrations({});
#else
    return {{},Support::unsupported,"Managed Run registration listing requires Windows"};
#endif
}
Metric<WindowsRunRegistration> read_windows_run_registration(const std::string& id){
#ifdef _WIN32
    return detail::read_windows_run_registration(id,{});
#else
    (void)id;return {{},Support::unsupported,"Managed Run registration requires Windows"};
#endif
}
ActionResult set_windows_run_registration_enabled(const WindowsRunRegistration& expected,bool enabled){
#ifdef _WIN32
    return detail::set_windows_run_registration_enabled(expected,enabled,{});
#else
    (void)expected;(void)enabled;return {ActionError::not_supported,"Managed Run registration requires Windows"};
#endif
}
}
