#include "../src/windows_run_control.hpp"
#include <cassert>
#include <cstring>
#include <iostream>
#include <map>
using namespace pcinsight;
using namespace pcinsight::detail;
namespace {
struct KeyInfo{REGSAM view;bool root;};
std::map<HKEY,KeyInfo> keys;std::wstring fixture;unsigned opens=0,writes=0,removals=0;
WORD architecture=PROCESSOR_ARCHITECTURE_AMD64;
enum class Fault{none,root_denied,root_privilege,child_denied,enum_denied,missing};Fault fault{};
void WINAPI info(LPSYSTEM_INFO out){*out={};out->wProcessorArchitecture=architecture;}
std::wstring branch(REGSAM view){return fixture+(view==KEY_WOW64_32KEY?L"\\View32":L"\\View64");}
std::wstring child_path(LPCWSTR path){if(std::wstring(path)==managed_run_path+9)return L"Run";assert(std::wstring(path)==managed_run_backup_path+9);return L"Disabled";}
REGSAM view_of(REGSAM access){const auto view=access&(KEY_WOW64_32KEY|KEY_WOW64_64KEY);assert(view==KEY_WOW64_32KEY || view==KEY_WOW64_64KEY);return view;}
LSTATUS WINAPI open_plain(HKEY root,LPCWSTR path,DWORD options,REGSAM access,PHKEY out){
    ++opens;assert(!options);*out=nullptr;const auto view=view_of(access);LSTATUS status;
    if(root==HKEY_LOCAL_MACHINE){
        assert(std::wstring(path)==L"Software" && access==(KEY_READ|view));
        if(fault==Fault::root_denied)return ERROR_ACCESS_DENIED;
        if(fault==Fault::root_privilege)return ERROR_PRIVILEGE_NOT_HELD;
        status=RegOpenKeyExW(HKEY_CURRENT_USER,branch(view).c_str(),0,KEY_READ,out);
        if(status==ERROR_SUCCESS)assert(keys.emplace(*out,KeyInfo{view,true}).second);
    }else{
        assert(keys.contains(root) && keys.at(root).root && keys.at(root).view==view && access==(KEY_QUERY_VALUE|view));
        if(fault==Fault::child_denied)return ERROR_ACCESS_DENIED;
        if(fault==Fault::missing)return ERROR_FILE_NOT_FOUND;
        status=RegOpenKeyExW(root,child_path(path).c_str(),0,access,out);
        if(status==ERROR_SUCCESS)assert(keys.emplace(*out,KeyInfo{view,false}).second);
    }
    return status;
}
LSTATUS WINAPI tx_open(HKEY root,LPCWSTR path,DWORD options,REGSAM access,PHKEY out,HANDLE tx,PVOID extended){
    assert(keys.contains(root) && keys.at(root).root && view_of(access)==keys.at(root).view);
    const auto status=RegOpenKeyTransactedW(root,child_path(path).c_str(),options,access,out,tx,extended);
    if(status==ERROR_SUCCESS)assert(keys.emplace(*out,KeyInfo{keys.at(root).view,false}).second);return status;
}
LSTATUS WINAPI tx_create(HKEY root,LPCWSTR path,DWORD reserved,LPWSTR cls,DWORD options,REGSAM access,LPSECURITY_ATTRIBUTES security,PHKEY out,LPDWORD disposition,HANDLE tx,PVOID extended){
    assert(keys.contains(root) && keys.at(root).root && view_of(access)==keys.at(root).view);
    const auto status=RegCreateKeyTransactedW(root,child_path(path).c_str(),reserved,cls,options,access,security,out,disposition,tx,extended);
    if(status==ERROR_SUCCESS)assert(keys.emplace(*out,KeyInfo{keys.at(root).view,false}).second);return status;
}
LSTATUS WINAPI enumerate(HKEY key,DWORD index,LPWSTR name,LPDWORD chars,LPDWORD reserved,LPDWORD type,LPBYTE data,LPDWORD size){
    assert(keys.contains(key) && !keys.at(key).root);if(fault==Fault::enum_denied && index==1)return ERROR_ACCESS_DENIED;
    return RegEnumValueW(key,index,name,chars,reserved,type,data,size);
}
LSTATUS WINAPI query(HKEY key,LPCWSTR name,LPDWORD reserved,LPDWORD type,LPBYTE data,LPDWORD size){assert(keys.contains(key) && !keys.at(key).root);return RegQueryValueExW(key,name,reserved,type,data,size);}
LSTATUS WINAPI set(HKEY key,LPCWSTR name,DWORD reserved,DWORD type,const BYTE* data,DWORD size){assert(keys.contains(key) && !keys.at(key).root);++writes;return RegSetValueExW(key,name,reserved,type,data,size);}
LSTATUS WINAPI erase(HKEY key,LPCWSTR name){assert(keys.contains(key) && !keys.at(key).root);++removals;return RegDeleteValueW(key,name);}
LSTATUS WINAPI close(HKEY key){assert(keys.erase(key)==1);return RegCloseKey(key);}
WindowsRunControlApi api(bool mutation=false){
    WindowsRunControlApi result;result.user=nullptr;result.open_plain=open_plain;result.enumerate=enumerate;result.query=query;result.close_key=close;result.native_info=info;
    result.open=tx_open;result.create=tx_create;result.set=set;result.erase=erase;
    if(!mutation){result.begin=nullptr;result.commit=nullptr;result.rollback=nullptr;}
    return result;
}
std::string prefix(WindowsMachineRunView view){return view==WindowsMachineRunView::registry32?"winrun:hklm:32:":"winrun:hklm:64:";}
auto list(WindowsMachineRunView view){auto result=pcinsight::detail::list_windows_machine_run_registrations(view,api());assert(keys.empty());return result;}
struct Fixture{
    Fixture(){
        fixture=L"Software\\PCInsightMachineRunTest-"+std::to_wstring(GetCurrentProcessId())+L"-"+std::to_wstring(GetTickCount64());
        HKEY root{};DWORD disposition{};assert(RegCreateKeyExW(HKEY_CURRENT_USER,fixture.c_str(),0,nullptr,0,KEY_READ|KEY_WRITE,nullptr,&root,&disposition)==ERROR_SUCCESS && disposition==REG_CREATED_NEW_KEY);assert(RegCloseKey(root)==ERROR_SUCCESS);
        for(const auto view:{KEY_WOW64_32KEY,KEY_WOW64_64KEY})for(const auto* child:{L"Run",L"Disabled"}){
            HKEY key{};assert(RegCreateKeyExW(HKEY_CURRENT_USER,(branch(view)+L"\\"+child).c_str(),0,nullptr,0,KEY_SET_VALUE,nullptr,&key,nullptr)==ERROR_SUCCESS);
            const std::wstring value=std::wstring(child)==L"Run"?(view==KEY_WOW64_32KEY?L"private-32-never-execute":L"private-64-never-execute"):L"%PRIVATE%\\backup";
            const auto* label=std::wstring(child)==L"Run"?L"Same":L"Backup";
            assert(RegSetValueExW(key,label,0,REG_EXPAND_SZ,reinterpret_cast<const BYTE*>(value.c_str()),static_cast<DWORD>((value.size()+1)*sizeof(wchar_t)))==ERROR_SUCCESS);assert(RegCloseKey(key)==ERROR_SUCCESS);
        }
    }
    ~Fixture(){
        assert(keys.empty());
        for(const auto view:{KEY_WOW64_32KEY,KEY_WOW64_64KEY}){
            for(const auto* child:{L"Run",L"Disabled"}){
                HKEY key{};const auto path=branch(view)+L"\\"+child;assert(RegOpenKeyExW(HKEY_CURRENT_USER,path.c_str(),0,KEY_SET_VALUE,&key)==ERROR_SUCCESS);
                for(const auto* name:{L"Same",L"Backup"}){const auto status=RegDeleteValueW(key,name);assert(status==ERROR_SUCCESS || status==ERROR_FILE_NOT_FOUND);}assert(RegCloseKey(key)==ERROR_SUCCESS);assert(RegDeleteKeyW(HKEY_CURRENT_USER,path.c_str())==ERROR_SUCCESS);
            }
            assert(RegDeleteKeyW(HKEY_CURRENT_USER,branch(view).c_str())==ERROR_SUCCESS);
        }
        assert(RegDeleteKeyW(HKEY_CURRENT_USER,fixture.c_str())==ERROR_SUCCESS);
    }
};
}
int main(){
    Fixture own;
    for(const auto view:{WindowsMachineRunView::registry32,WindowsMachineRunView::registry64}){
        writes=removals=0;
        auto rows=list(view);assert(rows.value && rows.support==Support::estimated && rows.value->size()==2 && writes==0 && removals==0);
        const auto id=prefix(view)+"53616D65";
        auto observed=pcinsight::detail::read_windows_run_registration(id,api());assert(observed.value && keys.empty() && observed.value->state==WindowsRunRegistrationState::registered);
        assert(observed.value->command==(view==WindowsMachineRunView::registry32?"private-32-never-execute":"private-64-never-execute"));
        for(const auto& row:*rows.value)assert(row.entry_id.starts_with(prefix(view)));
        const auto blocked=pcinsight::detail::set_windows_run_registration_enabled(*observed.value,false,api());assert(blocked.error==ActionError::not_supported && keys.empty() && writes==0);
        const auto native=pcinsight::detail::set_windows_run_registration_enabled(*observed.value,false,api(true));assert(keys.empty());
        if(native){auto saved=pcinsight::detail::read_windows_run_registration(id,api());assert(saved.value && saved.value->state==WindowsRunRegistrationState::disabled_by_pcinsight);assert(pcinsight::detail::set_windows_run_registration_enabled(*saved.value,true,api(true)));}
        else{std::cout<<"Private redirected HKLM view "<<prefix(view)<<" transaction unavailable: "<<native.message<<'\n';assert(native.error==ActionError::not_supported && writes==0 && removals==0);}
        assert(pcinsight::detail::read_windows_run_registration(id,api()).value->native_data==observed.value->native_data && keys.empty());
        for(auto f:{Fault::root_denied,Fault::root_privilege,Fault::child_denied,Fault::enum_denied}){fault=f;const auto denied=list(view);assert(!denied.value && denied.support==Support::permission_required);}
        fault=Fault::missing;assert(list(view).value->empty());fault=Fault::none;
        const auto real=pcinsight::list_windows_machine_run_registrations(view);
        if(real.value){for(const auto& row:*real.value)assert(row.entry_id.starts_with(prefix(view)));std::cout<<"Real HKLM view "<<prefix(view)<<" read-only rows="<<real.value->size()<<'\n';
            if(!real.value->empty()){const auto single=pcinsight::read_windows_run_registration(real.value->front().entry_id);if(single.value)assert(single.support==Support::estimated && single.value->entry_id==real.value->front().entry_id);std::cout<<"Real HKLM individual read: "<<(single.value?"observed without KTM":single.note)<<'\n';}
        }
        else std::cout<<"Real HKLM read-only unavailable: "<<real.note<<'\n';
    }
    opens=0;assert(!list(static_cast<WindowsMachineRunView>(99)).value && opens==0);
    architecture=PROCESSOR_ARCHITECTURE_INTEL;assert(!list(WindowsMachineRunView::registry64).value && opens==0);assert(list(WindowsMachineRunView::registry32).value);
    opens=0;architecture=PROCESSOR_ARCHITECTURE_ARM64;assert(!list(WindowsMachineRunView::registry32).value && opens==0);
    std::cout<<"Machine Run: isolated views, raw bytes, backup discovery, no-KTM reads, permissions, architecture and handle cleanup passed; actual HKLM untouched\n";
}
