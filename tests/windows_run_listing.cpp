#include "../src/windows_run_control.hpp"
#include <cassert>
#include <cstring>
#include <iostream>
#include <set>
using namespace pcinsight;
using namespace pcinsight::detail;
namespace {
struct Item{std::wstring name;DWORD type;std::vector<std::uint8_t> bytes;};
std::vector<Item> live,saved;std::set<HKEY> keys;
bool native_mode=false,race=false,oversized_name=false,too_many=false;
LSTATUS enum_error=ERROR_SUCCESS;unsigned opens=0;std::wstring fixture;
HKEY root_token=reinterpret_cast<HKEY>(static_cast<std::uintptr_t>(101));
HKEY run_token=reinterpret_cast<HKEY>(static_cast<std::uintptr_t>(102));
HKEY backup_token=reinterpret_cast<HKEY>(static_cast<std::uintptr_t>(103));
std::vector<std::uint8_t> bytes(const std::wstring& text){std::vector<std::uint8_t> out((text.size()+1)*2);std::memcpy(out.data(),text.c_str(),out.size());return out;}
void reset(){assert(keys.empty());live={{L"Active",REG_SZ,bytes(L"never-execute-a")},{L"Same",REG_SZ,bytes(L"never-execute-live")}};saved={{L"Backup\x00e9",REG_EXPAND_SZ,bytes(L"%TEST%\\never.exe")},{L"sAME",REG_SZ,bytes(L"never-execute-saved")}};opens=0;race=oversized_name=too_many=false;enum_error=ERROR_SUCCESS;}
std::wstring mapped(LPCWSTR path){if(std::wstring(path)==managed_run_path)return fixture+L"\\Run";assert(std::wstring(path)==managed_run_backup_path);return fixture+L"\\Disabled";}
LSTATUS WINAPI user(REGSAM access,PHKEY out){assert(access==KEY_READ);if(native_mode){const auto code=RegOpenCurrentUser(access,out);if(code==ERROR_SUCCESS)keys.insert(*out);return code;}*out=root_token;keys.insert(*out);return ERROR_SUCCESS;}
LSTATUS WINAPI open(HKEY root,LPCWSTR path,DWORD options,REGSAM access,PHKEY out){
    assert(keys.contains(root) && !options && access==KEY_QUERY_VALUE);++opens;
    if(native_mode){const auto code=RegOpenKeyExW(root,mapped(path).c_str(),options,access,out);if(code==ERROR_SUCCESS)keys.insert(*out);return code;}
    const bool run=std::wstring(path)==managed_run_path;if(!run)assert(std::wstring(path)==managed_run_backup_path);*out=run?run_token:backup_token;keys.insert(*out);return ERROR_SUCCESS;
}
LSTATUS WINAPI enumerate(HKEY key,DWORD index,LPWSTR name,LPDWORD chars,LPDWORD reserved,LPDWORD type,LPBYTE data,LPDWORD size){
    assert(keys.contains(key) && *chars==16384 && !reserved && *size==65536);
    if(native_mode)return RegEnumValueW(key,index,name,chars,reserved,type,data,size);
    if(enum_error!=ERROR_SUCCESS && index==1)return enum_error;
    if(oversized_name){*chars=16384;return ERROR_SUCCESS;}
    const auto& items=key==run_token?live:saved;Item generated;
    const Item* item=nullptr;
    if(too_many && key==run_token){if(index>10000)return ERROR_NO_MORE_ITEMS;generated={L"Row"+std::to_wstring(index),REG_SZ,bytes(L"x")};item=&generated;}
    else {if(index>=items.size())return ERROR_NO_MORE_ITEMS;item=&items[index];}
    auto label=item->name;if(race && opens==3 && index==0)label[0]=L'a';
    assert(label.size()<*chars && item->bytes.size()<=*size);
    *chars=static_cast<DWORD>(label.size());std::copy(label.begin(),label.end(),name);
    *size=static_cast<DWORD>(item->bytes.size());*type=item->type;std::memcpy(data,item->bytes.data(),*size);return ERROR_SUCCESS;
}
LSTATUS WINAPI close(HKEY key){assert(keys.erase(key)==1);return native_mode?RegCloseKey(key):ERROR_SUCCESS;}
WindowsRunControlApi api(){WindowsRunControlApi a;a.user=user;a.open_plain=open;a.enumerate=enumerate;a.close_key=close;a.begin=nullptr;a.commit=nullptr;a.rollback=nullptr;a.set=nullptr;a.erase=nullptr;a.create=nullptr;return a;}
auto list(){auto result=pcinsight::detail::list_windows_run_registrations(api());assert(keys.empty());return result;}
std::string id(const std::string& text){std::string out="winrun:hkcu:shared:";constexpr char digits[]="0123456789ABCDEF";for(unsigned char c:text){out+=digits[c>>4];out+=digits[c&15];}return out;}
void check(const Metric<std::vector<WindowsRunRegistration>>& result){
    assert(result.value && result.support==Support::estimated && result.value->size()==3);
    unsigned registered=0,disabled=0,conflicts=0;
    for(const auto& row:*result.value){
        if(row.state==WindowsRunRegistrationState::registered){++registered;assert(row.entry_id==id("Active") && row.command=="never-execute-a");}
        if(row.state==WindowsRunRegistrationState::disabled_by_pcinsight){++disabled;assert(row.entry_id==id("Backup\xc3\xa9") && row.native_type==REG_EXPAND_SZ && row.command=="%TEST%\\never.exe");}
        if(row.state==WindowsRunRegistrationState::conflict){++conflicts;assert(row.entry_id==id("Same") && row.command.empty() && row.native_data.empty());}
    }
    assert(registered==1 && disabled==1 && conflicts==1);
}
void native_fixture(){
    reset();native_mode=true;fixture=L"Software\\PCInsightRunListingTest-"+std::to_wstring(GetCurrentProcessId())+L"-"+std::to_wstring(GetTickCount64());
    HKEY parent=nullptr;DWORD disposition=0;assert(RegCreateKeyExW(HKEY_CURRENT_USER,fixture.c_str(),0,nullptr,0,KEY_READ|KEY_WRITE,nullptr,&parent,&disposition)==ERROR_SUCCESS && disposition==REG_CREATED_NEW_KEY);assert(RegCloseKey(parent)==ERROR_SUCCESS);
    for(const auto* child:{L"Run",L"Disabled"}){
        HKEY key=nullptr;assert(RegCreateKeyExW(HKEY_CURRENT_USER,(fixture+L"\\"+child).c_str(),0,nullptr,0,KEY_SET_VALUE,nullptr,&key,nullptr)==ERROR_SUCCESS);
        for(const auto& item:(std::wstring(child)==L"Run"?live:saved))assert(RegSetValueExW(key,item.name.c_str(),0,item.type,item.bytes.data(),static_cast<DWORD>(item.bytes.size()))==ERROR_SUCCESS);
        assert(RegCloseKey(key)==ERROR_SUCCESS);
    }
    check(list()); // Registry-backed backup-only entry survives a new provider invocation.
    check(list());
    for(const auto* child:{L"Run",L"Disabled"}){
        HKEY key=nullptr;assert(RegOpenKeyExW(HKEY_CURRENT_USER,(fixture+L"\\"+child).c_str(),0,KEY_SET_VALUE,&key)==ERROR_SUCCESS);
        for(const auto& item:(std::wstring(child)==L"Run"?live:saved))assert(RegDeleteValueW(key,item.name.c_str())==ERROR_SUCCESS);
        assert(RegCloseKey(key)==ERROR_SUCCESS);assert(RegDeleteKeyW(HKEY_CURRENT_USER,(fixture+L"\\"+child).c_str())==ERROR_SUCCESS);
    }
    assert(RegDeleteKeyW(HKEY_CURRENT_USER,fixture.c_str())==ERROR_SUCCESS);native_mode=false;
    std::cout<<"Native private registry listing: active, backup-only Unicode, case-insensitive conflict and repeat invocation passed; fixture removed exactly\n";
}
}
int main(){
    reset();check(list());
    reset();race=true;auto result=list();assert(!result.value && result.note.find("changed between")!=std::string::npos);
    reset();live.push_back({L"active",REG_SZ,bytes(L"duplicate")});result=list();assert(!result.value && result.note.find("Duplicate/case-equivalent")!=std::string::npos);
    reset();enum_error=ERROR_ACCESS_DENIED;result=list();assert(!result.value && result.support==Support::permission_required);
    reset();enum_error=ERROR_MORE_DATA;assert(!list().value);
    reset();oversized_name=true;assert(!list().value);
    reset();too_many=true;result=list();assert(!result.value && result.note.find("bounds")!=std::string::npos);
    reset();saved={{L"bad",REG_BINARY,bytes(L"x")}};assert(!list().value);
    reset();live.push_back({L"Ignored",REG_BINARY,{1,2,3}});check(list());
    reset();live.push_back({L"",REG_SZ,bytes(L"ignored")});live.push_back({L"Empty",REG_SZ,bytes(L"")});check(list());
    reset();saved.push_back({L"",REG_SZ,bytes(L"x")});assert(!list().value);
    reset();live[0].name=std::wstring(1,static_cast<wchar_t>(0xd800));assert(!list().value);
    reset();live[0].bytes={1};assert(!list().value);
    reset();saved[0].bytes=bytes(L"");assert(!list().value);
    reset();live.clear();saved.clear();result=list();assert(result.value && result.value->empty() && result.support==Support::estimated);
    reset();live.clear();saved.clear();for(unsigned i=0;i<128;++i)live.push_back({L"Large"+std::to_wstring(i),REG_SZ,bytes(std::wstring(32767,L'x'))});result=list();assert(!result.value && result.note.find("16 MiB")!=std::string::npos);
    reset();auto incomplete=api();incomplete.enumerate=nullptr;assert(!pcinsight::detail::list_windows_run_registrations(incomplete).value && keys.empty());
    native_fixture();
    const auto real=pcinsight::list_windows_run_registrations();if(real.value){assert(real.support==Support::estimated);for(const auto& row:*real.value)assert(row.entry_id.starts_with("winrun:hkcu:shared:"));std::cout<<"Real HKCU listing read-only: "<<real.value->size()<<" registrations/backups observed\n";}else std::cout<<"Real HKCU listing unavailable: "<<real.note<<'\n';
    std::cout<<"Listing bounds, no-KTM provider, errors, races and field validation passed\n";
}
