#include "../src/windows_gpu_devices.hpp"
#include "pcinsight/monitor.hpp"
#include <cassert>
#include <cstring>
#include <cwchar>
#include <iostream>
using namespace pcinsight;
using namespace pcinsight::detail;
namespace {
unsigned destroys,opens,closes,count;int mode;
HDEVINFO WINAPI list(const GUID*,PCWSTR,HWND,DWORD){if(mode==1){SetLastError(ERROR_ACCESS_DENIED);return INVALID_HANDLE_VALUE;}return reinterpret_cast<HDEVINFO>(1);}
BOOL WINAPI enumerate(HDEVINFO,PSP_DEVINFO_DATA,const GUID*,DWORD i,PSP_DEVICE_INTERFACE_DATA out){
    if(i>=count){SetLastError(mode==2?ERROR_ACCESS_DENIED:ERROR_NO_MORE_ITEMS);return FALSE;}
    out->Reserved=i;return TRUE;
}
BOOL WINAPI detail(HDEVINFO,PSP_DEVICE_INTERFACE_DATA iface,PSP_DEVICE_INTERFACE_DETAIL_DATA_W out,DWORD,DWORD* needed,PSP_DEVINFO_DATA node){
    *needed=offsetof(SP_DEVICE_INTERFACE_DETAIL_DATA_W,DevicePath)+8*sizeof(wchar_t);
    if(!out){SetLastError(ERROR_INSUFFICIENT_BUFFER);return FALSE;}
    std::wcscpy(out->DevicePath,L"device0");out->DevicePath[6]+=static_cast<wchar_t>(iface->Reserved);
    node->DevInst=static_cast<DWORD>(iface->Reserved+1);
    if(mode==3)std::wmemset(out->DevicePath,L'x',8);
    if(mode==13)*needed=65537;
    return TRUE;
}
BOOL WINAPI property(HDEVINFO,PSP_DEVINFO_DATA,const DEVPROPKEY* key,DEVPROPTYPE* type,PBYTE out,DWORD size,DWORD* returned,DWORD){
    if(mode==4 && key->pid==2){SetLastError(ERROR_ACCESS_DENIED);return FALSE;}
    if(key->pid==2){SYSTEMTIME utc{};utc.wYear=2025;utc.wMonth=3;utc.wDay=17;FILETIME time{};assert(SystemTimeToFileTime(&utc,&time));*type=DEVPROP_TYPE_FILETIME;*returned=sizeof(time);assert(size>=*returned);std::memcpy(out,&time,*returned);}
    else if(key->pid==21){GUID pci{0xc8ebdfb0,0xb510,0x11d0,{0x80,0xe5,0,0xa0,0xc9,0x25,0x42,0xe3}};if(mode==5)pci.Data1=0;*type=DEVPROP_TYPE_GUID;*returned=sizeof(pci);assert(size>=*returned);std::memcpy(out,&pci,*returned);}
    else {DWORD value=key->pid==23?119:(2<<16)|1;if(mode==6 && key->pid==30)value=32<<16;if(mode==7)value=0;*type=mode==8?DEVPROP_TYPE_STRING:DEVPROP_TYPE_UINT32;*returned=4;assert(size>=4);std::memcpy(out,&value,4);}
    if(mode==11)++*returned;
    if(mode==12 && key->pid==2){FILETIME invalid{0xffffffff,0xffffffff};std::memcpy(out,&invalid,sizeof(invalid));}
    if(mode==15 && key->pid==30){DWORD negative=0xffffffff;*type=DEVPROP_TYPE_INT32;std::memcpy(out,&negative,4);}
    return TRUE;
}
BOOL WINAPI destroy(HDEVINFO){++destroys;return TRUE;}
LONG APIENTRY open(GpuOpenDevice* out){++opens;out->handle=opens;out->luid={1,0};if(mode==9 && opens%2==0)out->luid.LowPart=2;if(mode==10)return static_cast<LONG>(0xc0000022);return 0;}
LONG APIENTRY close(const GpuCloseDevice* out){assert(out->handle);++closes;return 0;}
GpuDeviceApi api{list,enumerate,detail,property,destroy,open,close};
std::vector<GpuInfo> run(int scenario,unsigned n=1){mode=scenario;count=n;destroys=opens=closes=0;std::vector<GpuInfo> gpus(2);gpus[0].id="dxgi-luid:00000000:00000001";gpus[1].id="dxgi-luid:00000000:00000002";fill_windows_gpu_devices(gpus,api);assert(closes==opens);assert(destroys==(mode==1?0:1));assert(!gpus[1].driver_date.value);return gpus;}
}
int main(){
    auto g=run(0);assert(g[0].driver_date.value=="2025-03-17");assert(g[0].physical_location.value=="PCI bus 119, device 2, function 1");
    g=run(1);assert(g[0].driver_date.support==Support::permission_required);
    g=run(2);assert(!g[0].driver_date.value && g[0].physical_location.support==Support::permission_required);
    g=run(3);assert(!g[0].driver_date.value && opens==0);
    g=run(4);assert(!g[0].driver_date.value && g[0].driver_date.support==Support::permission_required && g[0].physical_location.value);
    for(int m:{5,6,8}){g=run(m);assert(g[0].driver_date.value && !g[0].physical_location.value);}
    g=run(7);assert(g[0].physical_location.value=="PCI bus 0, device 0, function 0");
    g=run(9);assert(!g[0].driver_date.value);
    g=run(10);assert(g[0].driver_date.support==Support::permission_required);
    g=run(0,2);assert(!g[0].driver_date.value); // same LUID, two different PnP nodes
    g=run(0,0);assert(!g[0].driver_date.value);
    for(int m:{11,12,13}){g=run(m);assert(!g[0].driver_date.value);}
    g=run(15);assert(g[0].driver_date.value && !g[0].physical_location.value);
    g=run(0,257);assert(!g[0].driver_date.value && opens==512);
    auto absent=api;absent.open=nullptr;fill_windows_gpu_devices(g,absent);assert(!g[0].driver_date.value);
    auto native=read_windows_gpus();fill_windows_gpu_devices(native);
    for(const auto& gpu:native){std::cout<<gpu.id<<" date="<<gpu.driver_date.value.value_or("unavailable")<<" location="<<gpu.physical_location.value.value_or("unavailable")<<'\n';if(gpu.driver_date.value)assert(gpu.driver_date.value->size()==10);}
    Monitor monitor;const auto snapshot=monitor.sample();
    for(const auto& gpu:snapshot.gpus){if(gpu.driver_date.value)assert(gpu.driver_date.value->size()==10);if(gpu.physical_location.value)assert(gpu.physical_location.value->starts_with("PCI bus "));}
    std::cout<<"GPU device binding tests passed\n";
}
