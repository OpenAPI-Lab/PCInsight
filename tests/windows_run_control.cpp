#include "../src/windows_run_control.hpp"
#include <cassert>
#include <cstring>
#include <iostream>
#include <set>
#include <map>
#include <optional>
using namespace pcinsight;
using namespace pcinsight::detail;
namespace {
std::wstring fixture;
const std::wstring value_name=L"PCInsight fixtur\x00e9";
std::string id;
enum class Fault {none,begin,query,set,erase,commit,conflict};
Fault fault=Fault::none;
std::set<HKEY> keys;std::set<HANDLE> transactions;
unsigned writes=0,removals=0,commits=0,rollbacks=0;
unsigned plain_opens=0;bool observation_race=false;
bool emulated=false;
REGSAM expected_view=0;
struct ModelValue{DWORD type;std::vector<std::uint8_t> bytes;};
struct Model {bool run=false,backup=false;std::optional<ModelValue> live,saved;};
Model model,staged;
HKEY model_user=reinterpret_cast<HKEY>(static_cast<std::uintptr_t>(100));
HKEY model_run=reinterpret_cast<HKEY>(static_cast<std::uintptr_t>(101));
HKEY model_backup=reinterpret_cast<HKEY>(static_cast<std::uintptr_t>(102));
HANDLE model_tx=reinterpret_cast<HANDLE>(static_cast<std::uintptr_t>(200));
std::wstring mapped(LPCWSTR path){
    if(std::wstring(path)==managed_run_path)return fixture+L"\\Run";
    assert(std::wstring(path)==managed_run_backup_path);return fixture+L"\\Disabled";
}
bool run_path(LPCWSTR path){const bool run=std::wstring(path)==(expected_view?managed_run_path+9:managed_run_path);if(!run)assert(std::wstring(path)==(expected_view?managed_run_backup_path+9:managed_run_backup_path));return run;}
LSTATUS WINAPI user(REGSAM access,PHKEY out){assert(!expected_view);if(emulated){*out=model_user;keys.insert(*out);return ERROR_SUCCESS;}const auto code=RegOpenCurrentUser(access,out);if(code==ERROR_SUCCESS)keys.insert(*out);return code;}
LSTATUS WINAPI open_plain(HKEY root,LPCWSTR path,DWORD options,REGSAM access,PHKEY out){
    if(root==HKEY_LOCAL_MACHINE){assert(emulated && expected_view && std::wstring(path)==L"Software" && !options && access==(KEY_READ|expected_view) && transactions.empty());*out=model_user;keys.insert(*out);return ERROR_SUCCESS;}
    assert(keys.contains(root) && options==0 && access==(KEY_QUERY_VALUE|expected_view) && transactions.empty());++plain_opens;
    if(emulated){
        if(observation_race && plain_opens==3 && model.live)model.live->bytes=std::vector<std::uint8_t>{'x',0};
        const bool run=run_path(path);
        *out=nullptr;if(!(run?model.run:model.backup))return ERROR_FILE_NOT_FOUND;
        *out=run?model_run:model_backup;keys.insert(*out);return ERROR_SUCCESS;
    }
    const auto code=RegOpenKeyExW(root,mapped(path).c_str(),options,access,out);if(code==ERROR_SUCCESS)keys.insert(*out);return code;
}
LSTATUS WINAPI open(HKEY root,LPCWSTR path,DWORD options,REGSAM access,PHKEY out,HANDLE tx,PVOID extended){
    assert(keys.contains(root) && options==0 && !extended && transactions.contains(tx));
    assert((access&(KEY_WOW64_32KEY|KEY_WOW64_64KEY))==expected_view);
    if(emulated){const bool run=run_path(path);*out=nullptr;if(!(run?staged.run:staged.backup))return ERROR_FILE_NOT_FOUND;*out=run?model_run:model_backup;keys.insert(*out);return ERROR_SUCCESS;}
    const auto code=RegOpenKeyTransactedW(root,mapped(path).c_str(),options,access,out,tx,extended);if(code==ERROR_SUCCESS)keys.insert(*out);return code;
}
LSTATUS WINAPI create(HKEY root,LPCWSTR path,DWORD reserved,LPWSTR cls,DWORD options,REGSAM access,LPSECURITY_ATTRIBUTES security,PHKEY out,LPDWORD disposition,HANDLE tx,PVOID extended){
    assert(keys.contains(root) && !reserved && !cls && options==REG_OPTION_NON_VOLATILE && !security && !extended && transactions.contains(tx));
    assert((access&(KEY_WOW64_32KEY|KEY_WOW64_64KEY))==expected_view);
    if(emulated){const bool run=run_path(path);(run?staged.run:staged.backup)=true;*out=run?model_run:model_backup;keys.insert(*out);*disposition=REG_CREATED_NEW_KEY;return ERROR_SUCCESS;}
    const auto code=RegCreateKeyTransactedW(root,mapped(path).c_str(),reserved,cls,options,access,security,out,disposition,tx,extended);if(code==ERROR_SUCCESS)keys.insert(*out);return code;
}
LSTATUS WINAPI query(HKEY key,LPCWSTR name,LPDWORD reserved,LPDWORD type,LPBYTE data,LPDWORD size){
    assert(keys.contains(key) && std::wstring(name)==value_name && !reserved && *size==65536);
    if(fault==Fault::query)return ERROR_ACCESS_DENIED;
    if(emulated){const auto& selected=transactions.empty()?model:staged;const auto& value=key==model_run?selected.live:selected.saved;if(!value)return ERROR_FILE_NOT_FOUND;*type=value->type;if(value->bytes.size()>*size){*size=static_cast<DWORD>(value->bytes.size());return ERROR_MORE_DATA;}*size=static_cast<DWORD>(value->bytes.size());std::memcpy(data,value->bytes.data(),*size);return ERROR_SUCCESS;}
    return RegQueryValueExW(key,name,reserved,type,data,size);
}
LSTATUS WINAPI set(HKEY key,LPCWSTR name,DWORD reserved,DWORD type,const BYTE* data,DWORD size){
    assert(keys.contains(key) && std::wstring(name)==value_name && !reserved);++writes;
    if(fault==Fault::set)return ERROR_ACCESS_DENIED;
    if(emulated){(key==model_run?staged.live:staged.saved)=ModelValue{type,{data,data+size}};return ERROR_SUCCESS;}
    return RegSetValueExW(key,name,reserved,type,data,size);
}
LSTATUS WINAPI erase(HKEY key,LPCWSTR name){assert(keys.contains(key) && std::wstring(name)==value_name);++removals;if(fault==Fault::erase)return ERROR_ACCESS_DENIED;if(emulated){(key==model_run?staged.live:staged.saved).reset();return ERROR_SUCCESS;}return RegDeleteValueW(key,name);}
LSTATUS WINAPI close_key(HKEY key){assert(keys.erase(key)==1);return emulated?ERROR_SUCCESS:RegCloseKey(key);}
HANDLE WINAPI begin(LPSECURITY_ATTRIBUTES security,LPGUID guid,DWORD options,DWORD level,DWORD flags,DWORD timeout,LPWSTR description){
    assert(!security && !guid && !options && !level && !flags && timeout==5000 && !description);
    if(fault==Fault::begin){SetLastError(ERROR_NOT_SUPPORTED);return INVALID_HANDLE_VALUE;}
    if(emulated){staged=model;transactions.insert(model_tx);return model_tx;}
    const auto tx=CreateTransaction(security,guid,options,level,flags,timeout,description);if(tx!=INVALID_HANDLE_VALUE)transactions.insert(tx);return tx;
}
BOOL WINAPI commit(HANDLE tx){assert(transactions.contains(tx));++commits;if(fault==Fault::commit || fault==Fault::conflict){SetLastError(fault==Fault::commit?ERROR_WRITE_FAULT:ERROR_TRANSACTIONAL_CONFLICT);return FALSE;}if(emulated){model=staged;return TRUE;}return CommitTransaction(tx);}
BOOL WINAPI rollback(HANDLE tx){assert(transactions.contains(tx));++rollbacks;if(emulated){staged={};return TRUE;}return RollbackTransaction(tx);}
BOOL WINAPI close(HANDLE tx){assert(transactions.erase(tx)==1);return emulated?TRUE:CloseHandle(tx);}
WindowsRunControlApi api(){WindowsRunControlApi a;a.user=user;a.open_plain=open_plain;a.open=open;a.create=create;a.query=query;a.set=set;a.erase=erase;a.close_key=close_key;a.begin=begin;a.commit=commit;a.rollback=rollback;a.close=close;return a;}
auto read(){auto result=pcinsight::detail::read_windows_run_registration(id,api());assert(keys.empty() && transactions.empty());return result;}
ActionResult change(const WindowsRunRegistration& expected,bool enabled){auto result=pcinsight::detail::set_windows_run_registration_enabled(expected,enabled,api());assert(keys.empty() && transactions.empty());return result;}
std::vector<std::uint8_t> bytes(const std::wstring& command,bool terminated=true){
    std::vector<std::uint8_t> result((command.size()+(terminated?1:0))*sizeof(wchar_t));std::memcpy(result.data(),command.c_str(),result.size());return result;
}
void put(const wchar_t* child,DWORD type,const std::vector<std::uint8_t>& data){
    if(emulated){const bool run=std::wstring(child)==L"Run";(run?model.run:model.backup)=true;(run?model.live:model.saved)=ModelValue{type,data};return;}
    HKEY key=nullptr;assert(RegCreateKeyExW(HKEY_CURRENT_USER,(fixture+L"\\"+child).c_str(),0,nullptr,0,KEY_QUERY_VALUE|KEY_SET_VALUE,nullptr,&key,nullptr)==ERROR_SUCCESS);
    assert(RegSetValueExW(key,value_name.c_str(),0,type,data.data(),static_cast<DWORD>(data.size()))==ERROR_SUCCESS);assert(RegCloseKey(key)==ERROR_SUCCESS);
}
void remove_value(const wchar_t* child){
    if(emulated){(std::wstring(child)==L"Run"?model.live:model.saved).reset();return;}
    HKEY key=nullptr;const auto opened=RegOpenKeyExW(HKEY_CURRENT_USER,(fixture+L"\\"+child).c_str(),0,KEY_SET_VALUE,&key);
    if(opened==ERROR_FILE_NOT_FOUND)return;assert(opened==ERROR_SUCCESS);
    const auto code=RegDeleteValueW(key,value_name.c_str());assert(code==ERROR_SUCCESS || code==ERROR_FILE_NOT_FOUND);assert(RegCloseKey(key)==ERROR_SUCCESS);
}
struct Fixture {
    Fixture(){
        fixture=L"Software\\PCInsightRunControlTest-"+std::to_wstring(GetCurrentProcessId())+L"-"+std::to_wstring(GetTickCount64());
        if(!emulated){HKEY key=nullptr;DWORD disposition=0;assert(RegCreateKeyExW(HKEY_CURRENT_USER,fixture.c_str(),0,nullptr,0,KEY_READ|KEY_WRITE,nullptr,&key,&disposition)==ERROR_SUCCESS && disposition==REG_CREATED_NEW_KEY);assert(RegCloseKey(key)==ERROR_SUCCESS);}
        const auto count=WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,value_name.data(),static_cast<int>(value_name.size()),nullptr,0,nullptr,nullptr);assert(count>0);
        std::string name(count,'\0');assert(WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,value_name.data(),static_cast<int>(value_name.size()),name.data(),count,nullptr,nullptr)==count);
        id=expected_view==KEY_WOW64_32KEY?"winrun:hklm:32:":expected_view==KEY_WOW64_64KEY?"winrun:hklm:64:":"winrun:hkcu:shared:";constexpr char digits[]="0123456789ABCDEF";for(unsigned char c:name){id+=digits[c>>4];id+=digits[c&15];}
    }
    ~Fixture(){
        if(emulated){model={};staged={};return;}
        // Only our unique, newly-created test root and two exact child keys; never Startup/Run.
        for(const auto* child:{L"Run",L"Disabled"}){remove_value(child);const auto code=RegDeleteKeyW(HKEY_CURRENT_USER,(fixture+L"\\"+child).c_str());assert(code==ERROR_SUCCESS || code==ERROR_FILE_NOT_FOUND);}
        assert(RegDeleteKeyW(HKEY_CURRENT_USER,fixture.c_str())==ERROR_SUCCESS);
    }
};
}
void suite(){
    auto state=read();if(!state.value)std::cerr<<"Initial private-registry query: "<<state.note<<'\n';assert(state.value && state.value->state==WindowsRunRegistrationState::absent && writes==0 && removals==0);
    const auto original=bytes(L"\"%PCINSIGHT_TEST%\\never.exe\" --literal & no-shell",false);put(L"Run",REG_EXPAND_SZ,original);
    state=read();assert(state.value && state.value->state==WindowsRunRegistrationState::registered && state.value->native_data==original && state.value->native_type==REG_EXPAND_SZ && state.value->command.find("%PCINSIGHT_TEST%")!=std::string::npos);
    const auto expected=*state.value;
    auto forged=expected;forged.native_data.push_back(0);auto before=writes;assert(change(forged,false).error==ActionError::stale_target && writes==before);
    for(const auto failure:{Fault::set,Fault::erase,Fault::commit,Fault::conflict}){
        fault=failure;const auto result=change(expected,false);assert(!result);if(failure==Fault::conflict)assert(result.error==ActionError::stale_target);
        fault=Fault::none;state=read();assert(state.value && state.value->state==WindowsRunRegistrationState::registered && state.value->native_data==original);
    }
    fault=Fault::query;assert(change(expected,false).error==ActionError::permission_denied);assert(!read().value);fault=Fault::none;
    fault=Fault::begin;assert(change(expected,false).error==ActionError::not_supported);fault=Fault::none;
    assert(change(expected,false));state=read();assert(state.value && state.value->state==WindowsRunRegistrationState::disabled_by_pcinsight && state.value->native_data==original);
    const auto saved=*state.value;assert(change(expected,false).error==ActionError::stale_target);
    const auto other=bytes(L"different command");put(L"Run",REG_SZ,other);state=read();assert(state.value && state.value->state==WindowsRunRegistrationState::conflict);before=writes;assert(change(saved,true).error==ActionError::stale_target && writes==before);remove_value(L"Run");
    put(L"Disabled",REG_SZ,other);assert(change(saved,true).error==ActionError::stale_target);put(L"Disabled",REG_EXPAND_SZ,original);
    for(const auto failure:{Fault::set,Fault::erase,Fault::commit}){fault=failure;assert(!change(saved,true));fault=Fault::none;state=read();assert(state.value && state.value->state==WindowsRunRegistrationState::disabled_by_pcinsight && state.value->native_data==original);}
    assert(change(saved,true));state=read();assert(state.value && state.value->state==WindowsRunRegistrationState::registered && state.value->native_data==original);
    put(L"Run",REG_SZ,bytes(std::wstring(261,L'x')));state=read();assert(state.value && change(*state.value,false).error==ActionError::not_supported);
    put(L"Run",REG_BINARY,other);assert(!read().value);
    put(L"Run",REG_SZ,bytes(std::wstring{L'x',L'\0',L'y'}));assert(!read().value);
    put(L"Run",REG_SZ,bytes(std::wstring{static_cast<wchar_t>(0xd800)}));assert(!read().value);
    put(L"Run",REG_SZ,original);state=read();assert(state.value && change(*state.value,false));state=read();assert(state.value && change(*state.value,true)); // REG_SZ also preserves raw bytes.
    for(const auto* bad:{"","winrun:hkcu:shared:","winrun:hkcu:shared:00","winrun:hkcu:shared:GG","winrun:hkcu:shared:c3a9","winrun:hkcu:shared:FF","winrun:hklm:shared:41","winfolder:user:41"}){
        assert(!pcinsight::detail::read_windows_run_registration(bad,api()).value);
    }
    auto incomplete=api();incomplete.open_plain=nullptr;assert(!pcinsight::detail::read_windows_run_registration(id,incomplete).value);
    incomplete=api();incomplete.begin=nullptr;incomplete.commit=nullptr;incomplete.rollback=nullptr;
    const auto without_ktm=pcinsight::detail::read_windows_run_registration(id,incomplete);assert(without_ktm.value && without_ktm.support==Support::estimated);
    assert(!pcinsight::detail::set_windows_run_registration_enabled(*without_ktm.value,false,incomplete));
    if(emulated){plain_opens=0;observation_race=true;const auto changing=read();assert(!changing.value && changing.note.find("changed between observations")!=std::string::npos);observation_race=false;}
    assert(!pcinsight::read_windows_run_registration("").value && !pcinsight::set_windows_run_registration_enabled({},false));
    assert(keys.empty() && transactions.empty() && rollbacks>0 && commits>0);
    std::cout<<(emulated?"Injected transaction model":"Native private registry fixture")<<": disable/restore exact bytes, conflicts, rollback after staged writes, permissions, bounds and cleanup passed; real Startup untouched\n";
}
int main(){
    for(const auto& entry:read_windows_startup())if(entry.id.starts_with("winrun:hkcu:shared:")){
        const auto observed=pcinsight::read_windows_run_registration(entry.id);
        if(observed.value)assert(observed.support==Support::estimated && observed.value->entry_id==entry.id);
        std::cout<<"Native real HKCU read-only probe: "<<(observed.value?"estimated state observed without KTM":observed.note)<<'\n';
        break; // Never call the mutation API for a real registration.
    }
    {
        Fixture own;put(L"Run",REG_SZ,bytes(L"private-test-never-execute"));const auto probe=read();assert(probe.value && probe.support==Support::estimated);
        const auto before=writes;const auto result=change(*probe.value,false);
        if(result){const auto disabled=read();assert(disabled.value && disabled.value->state==WindowsRunRegistrationState::disabled_by_pcinsight);assert(change(*disabled.value,true));remove_value(L"Run");writes=removals=commits=rollbacks=0;suite();}
        else {
            std::cout<<"Native transaction unavailable: "<<result.message<<'\n';
            assert(result.error==ActionError::not_supported && writes==before && removals==0);
            assert(read().value->native_data==probe.value->native_data);
            std::cout<<"Native read-only state works; mutation rejected without writes\n";
        }
    }
    emulated=true;writes=removals=commits=rollbacks=0;
    {Fixture own;suite();}
    for(const auto view:{KEY_WOW64_32KEY,KEY_WOW64_64KEY}){
        expected_view=view;writes=removals=commits=rollbacks=0;plain_opens=0;
        {Fixture own;suite();}
        std::cout<<"Injected HKLM "<<(view==KEY_WOW64_32KEY?32:64)<<"-bit transaction model passed with explicit view on every child open/create; no actual HKLM writes\n";
    }
}
