#include "../src/windows_network_identity.hpp"
#include "../src/windows_network.hpp"
#include <cassert>
#include <cstring>
#include <iostream>
#include <objbase.h>
using namespace pcinsight;
using namespace pcinsight::detail;
namespace {
int mode{}, row_calls{}, value_calls{}, keys{}, closed{}, sets{}, destroyed{};
const GUID identity{0x12345678,0x1234,0x1234,{1,2,3,4,5,6,7,8}};
HDEVINFO set_handle(){return reinterpret_cast<HDEVINFO>(1);}
HKEY key_handle(){return reinterpret_cast<HKEY>(2);}
NETIO_STATUS WINAPI row(PMIB_IF_ROW2 out) {
    assert(out->InterfaceLuid.Value==42); ++row_calls;
    if(mode==1){return ERROR_ACCESS_DENIED;}
    if(mode==20&&row_calls==2)return ERROR_NOT_FOUND;
    out->InterfaceGuid=identity; out->Type=IF_TYPE_ETHERNET_CSMACD;
    if(mode==12 && row_calls==2) ++out->Type;
    if(mode==13 && row_calls==2) ++out->InterfaceGuid.Data1;
    return NO_ERROR;
}
HDEVINFO WINAPI list(const GUID* guid,PCWSTR name,HWND window,DWORD flags){
    assert(guid && guid->Data1==0x4d36e972 && !name && !window && flags==DIGCF_PRESENT);
    if(mode==2){SetLastError(ERROR_ACCESS_DENIED);return INVALID_HANDLE_VALUE;}
    ++sets;return set_handle();
}
BOOL WINAPI enumerate(HDEVINFO h,DWORD index,PSP_DEVINFO_DATA node){
    assert(h==set_handle()&&node->cbSize==sizeof(*node));
    if(mode==3&&index==1){SetLastError(ERROR_ACCESS_DENIED);return FALSE;}
    if(mode!=21&&index>=(mode==4?2u:1u)){SetLastError(ERROR_NO_MORE_ITEMS);return FALSE;}
    node->DevInst=index+1;if(mode==22)node->cbSize=0;return TRUE;
}
HKEY WINAPI key(HDEVINFO h,PSP_DEVINFO_DATA,DWORD scope,DWORD profile,DWORD kind,REGSAM access){
    assert(h==set_handle()&&scope==DICS_FLAG_GLOBAL&&!profile&&kind==DIREG_DRV&&access==KEY_QUERY_VALUE);
    if(mode==5){SetLastError(ERROR_ACCESS_DENIED);return reinterpret_cast<HKEY>(INVALID_HANDLE_VALUE);}
    ++keys;return key_handle();
}
LSTATUS WINAPI value(HKEY h,LPCWSTR name,LPDWORD reserved,LPDWORD type,LPBYTE data,LPDWORD size){
    assert(h==key_handle()&&std::wstring(name)==L"NetCfgInstanceId"&&!reserved&&*size==80);
    ++value_calls;
    if(mode==6)return ERROR_FILE_NOT_FOUND;
    if(mode==7)return ERROR_MORE_DATA;
    if(mode==16)return ERROR_ACCESS_DENIED;
    wchar_t text[40]{};GUID guid=identity;
    if(mode==8||mode==21)++guid.Data1;
    if(mode==9&&keys>1)++guid.Data1;
    if(mode==19&&value_calls==3)++guid.Data1;
    StringFromGUID2(guid,text,40);
    if(mode==10)text[1]=L'Z';
    std::memcpy(data,text,78);*size=78;*type=mode==11?REG_BINARY:REG_SZ;return ERROR_SUCCESS;
}
BOOL WINAPI property(HDEVINFO h,PSP_DEVINFO_DATA,DWORD property,PDWORD type,PBYTE data,DWORD size,PDWORD needed){
    assert(h==set_handle()&&(property==SPDRP_MFG||property==SPDRP_DEVICEDESC)&&size==4096);
    if(mode==14&&property==SPDRP_MFG){SetLastError(ERROR_ACCESS_DENIED);return FALSE;}
    std::wstring text=property==SPDRP_MFG?L"Maker \u00e9":L"Device";
    if(mode==15)text.clear();
    if(mode==17)text=std::wstring(1,static_cast<wchar_t>(0xd800));
    *type=REG_SZ;*needed=static_cast<DWORD>((text.size()+1)*sizeof(wchar_t));
    if(mode==18)*type=REG_MULTI_SZ;
    std::memcpy(data,text.c_str(),*needed);
    if(mode==23){std::memset(data,1,size);*needed=size;}
    return TRUE;
}
LSTATUS WINAPI close(HKEY h){assert(h==key_handle());++closed;return ERROR_SUCCESS;}
BOOL WINAPI destroy(HDEVINFO h){assert(h==set_handle());++destroyed;return TRUE;}
Metric<WindowsNetworkIdentity> run(int next=0,const std::string& id="luid:42"){
    mode=next; row_calls=value_calls=keys=closed=sets=destroyed=0;
    auto out=query_windows_network_identity_impl(id,{row,list,enumerate,key,value,property,close,destroy});
    assert(keys==closed&&sets==destroyed);return out;
}
}
int main(){
    auto r=run();assert(metric_is_known(r)&&r.value->adapter_id=="luid:42");
    assert(r.value->manufacturer.value=="Maker \xc3\xa9"&&r.value->device_description.value=="Device");
    assert(keys==2&&row_calls==2);
    for(const auto& id:{std::string{},std::string("luid:0"),std::string("luid:042"),std::string("LUID:42"),
        std::string("luid:-1"),std::string("luid:18446744073709551616"),std::string("luid:42x"),std::string("luid:42\0",8)}){
        assert(!run(0,id).value&&!row_calls&&!sets);
    }
    for(int m:{1,2,3,4,5,7,9,10,11,12,13,16,19,20,21,22})assert(!run(m).value);
    for(int m:{1,2,3,5,16})assert(run(m).support==Support::permission_required);
    r=run(6);assert(r.value&&!r.value->manufacturer.value);
    r=run(8);assert(r.value&&!r.value->manufacturer.value); // valid GUID, no match
    r=run(14);assert(r.value&&r.value->manufacturer.support==Support::permission_required&&r.value->device_description.value);
    r=run(15);assert(r.value->manufacturer.value==""&&metric_is_known(r.value->manufacturer));
    for(int m:{17,18,23}){r=run(m);assert(r.value&&!r.value->manufacturer.value);}
    WindowsNetwork inventory;
    std::size_t observed{},known{},ethernet{};
    for(const auto& adapter:inventory.sample()){
        ++observed; const auto native=query_windows_network_identity(adapter.id);
        if(native.value&&metric_is_known(native.value->manufacturer)&&!native.value->manufacturer.value->empty()){
            ++known;if(adapter.connection_type=="Ethernet")++ethernet;
            if(known<=4)std::cout<<"Native PnP "<<adapter.connection_type<<" manufacturer="<<*native.value->manufacturer.value
                <<" device="<<native.value->device_description.value.value_or("unavailable")<<'\n';
        }else if(observed<=3)std::cout<<"Native identity note="<<native.note<<'\n';
    }
    assert(observed&&known&&ethernet);
    std::cout<<"Native network identity: interfaces="<<observed<<" known_manufacturers="<<known<<" Ethernet="<<ethernet<<'\n';
}
