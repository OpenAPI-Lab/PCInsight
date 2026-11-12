#include "../src/windows_disks.hpp"
#include "pcinsight/monitor.hpp"
#include <cassert>
#include <cstring>
#include <cwchar>
#include <iostream>
#include <system_error>
using namespace pcinsight::detail;
namespace {
DWORD list_error, enumerate_error, open_error, geometry_error;
bool broken_path, invalid_descriptor, invalid_size, invalid_geometry;
unsigned destroyed, closed;
HDEVINFO WINAPI devices(const GUID* guid,PCWSTR enumerator,HWND parent,DWORD flags) {
    assert(guid->Data1==0x53f56307 && !enumerator && !parent && flags==(DIGCF_PRESENT|DIGCF_DEVICEINTERFACE));
    if(list_error) { SetLastError(list_error); return INVALID_HANDLE_VALUE; }
    return reinterpret_cast<HDEVINFO>(1);
}
BOOL WINAPI enumerate(HDEVINFO h,PSP_DEVINFO_DATA dev,const GUID*,DWORD i,PSP_DEVICE_INTERFACE_DATA out) {
    assert(h==reinterpret_cast<HDEVINFO>(1) && !dev && out->cbSize==sizeof *out);
    if(i) { SetLastError(enumerate_error ? enumerate_error : ERROR_NO_MORE_ITEMS); return FALSE; }
    return TRUE;
}
BOOL WINAPI detail(HDEVINFO,PSP_DEVICE_INTERFACE_DATA,PSP_DEVICE_INTERFACE_DETAIL_DATA_W out,DWORD size,PDWORD required,PSP_DEVINFO_DATA dev) {
    assert(!dev);
    constexpr wchar_t path[]=L"\\\\?\\mock-disk";
    const DWORD length=offsetof(SP_DEVICE_INTERFACE_DETAIL_DATA_W,DevicePath)+sizeof path;
    *required=length;
    if(!out) { assert(!size); if(invalid_size)*required=100000; SetLastError(ERROR_INSUFFICIENT_BUFFER); return FALSE; }
    assert(out->cbSize==sizeof *out && size>=length);
    std::memcpy(out->DevicePath,path,sizeof path);
    if(broken_path) std::wmemset(out->DevicePath,L'x',sizeof path/sizeof(wchar_t));
    return TRUE;
}
BOOL WINAPI destroy(HDEVINFO h) { assert(h==reinterpret_cast<HDEVINFO>(1)); ++destroyed; return TRUE; }
HANDLE WINAPI open(LPCWSTR path,DWORD access,DWORD share,LPSECURITY_ATTRIBUTES attrs,DWORD disposition,DWORD flags,HANDLE template_file) {
    assert(std::wstring(path)==L"\\\\?\\mock-disk" && !access && share==(FILE_SHARE_READ|FILE_SHARE_WRITE));
    assert(!attrs && disposition==OPEN_EXISTING && !flags && !template_file);
    if(open_error) { SetLastError(open_error); return INVALID_HANDLE_VALUE; }
    return reinterpret_cast<HANDLE>(2);
}
BOOL WINAPI close(HANDLE h) { assert(h==reinterpret_cast<HANDLE>(2)); ++closed; return TRUE; }
BOOL WINAPI query(HANDLE h,DWORD code,LPVOID in,DWORD input_size,LPVOID out,DWORD capacity,LPDWORD returned,LPOVERLAPPED overlapped) {
    assert(h==reinterpret_cast<HANDLE>(2) && !overlapped);
    if(code==IOCTL_STORAGE_GET_DEVICE_NUMBER) {
        assert(!in && !input_size && capacity==sizeof(STORAGE_DEVICE_NUMBER));
        STORAGE_DEVICE_NUMBER number{FILE_DEVICE_DISK,3,0};
        std::memcpy(out,&number,sizeof number); *returned=sizeof number; return TRUE;
    }
    if(code==IOCTL_DISK_GET_DRIVE_GEOMETRY_EX) {
        assert(!in && !input_size && capacity>=sizeof(DISK_GEOMETRY_EX));
        if(geometry_error) { SetLastError(geometry_error); return FALSE; }
        DISK_GEOMETRY_EX geometry{}; geometry.DiskSize.QuadPart=invalid_geometry?-1:500000000000LL;
        std::memcpy(out,&geometry,sizeof geometry); *returned=sizeof geometry; return TRUE;
    }
    assert(code==IOCTL_STORAGE_QUERY_PROPERTY && input_size==sizeof(STORAGE_PROPERTY_QUERY));
    auto& request=*static_cast<STORAGE_PROPERTY_QUERY*>(in); assert(request.QueryType==PropertyStandardQuery);
    if(request.PropertyId==StorageDeviceSeekPenaltyProperty) {
        assert(capacity==sizeof(DEVICE_SEEK_PENALTY_DESCRIPTOR));
        DEVICE_SEEK_PENALTY_DESCRIPTOR seek{}; seek.Version=seek.Size=sizeof seek; seek.IncursSeekPenalty=FALSE;
        std::memcpy(out,&seek,sizeof seek); *returned=sizeof seek; return TRUE;
    }
    assert(request.PropertyId==StorageDeviceProperty);
    constexpr DWORD bytes=128;
    if(capacity==sizeof(STORAGE_DESCRIPTOR_HEADER)) {
        STORAGE_DESCRIPTOR_HEADER header{sizeof(STORAGE_DEVICE_DESCRIPTOR),bytes};
        std::memcpy(out,&header,sizeof header); *returned=sizeof header; return TRUE;
    }
    assert(capacity==bytes); std::memset(out,0,capacity);
    STORAGE_DEVICE_DESCRIPTOR descriptor{}; descriptor.Version=sizeof descriptor; descriptor.Size=bytes;
    descriptor.BusType=BusTypeNvme; descriptor.VendorIdOffset=64; descriptor.ProductIdOffset=80;
    descriptor.SerialNumberOffset=100;
    if(invalid_descriptor) descriptor.ProductIdOffset=500;
    std::memcpy(out,&descriptor,sizeof descriptor);
    std::memcpy(static_cast<char*>(out)+64,"ACME ",6);
    std::memcpy(static_cast<char*>(out)+80,"Model X ",9);
    std::memcpy(static_cast<char*>(out)+100," SERIAL-1 ",11);
    *returned=bytes; return TRUE;
}
DiskApi api{devices,enumerate,detail,destroy,open,close,query};
void reset() { list_error=enumerate_error=open_error=geometry_error=0; broken_path=invalid_descriptor=invalid_size=invalid_geometry=false; destroyed=closed=0; }
void fails(DWORD error) {
    try { (void)read_windows_disks(api); assert(false); }
    catch(const std::system_error& e) { assert(e.code().value()==static_cast<int>(error)); }
}
}
int main() {
    reset(); auto result=read_windows_disks(api);
    assert(result.size()==1 && result[0].model=="ACME Model X" && result[0].bus_type=="NVMe");
    assert(result[0].media_type=="no-seek-penalty" && result[0].capacity_bytes.value==500000000000ULL);
    assert(!result[0].formatted_capacity_bytes.value && !result[0].system_disk.value && !result[0].active_percent.value);
    assert(destroyed==1 && closed==1);
    reset(); std::map<std::uint32_t,std::string> numbers;
    result=read_windows_disks(api,&numbers); assert(numbers.at(3)==result[0].id && numbers.size()==1);
    reset(); std::map<std::string,DiskHardwareIdentity> hardware;
    result=read_windows_disks(api,&numbers,&hardware);
    const auto& identity=hardware.at(result[0].id);
    assert(identity.serial=="SERIAL-1" && identity.capacity==500000000000ULL && identity.bus==BusTypeNvme);
    reset(); list_error=ERROR_ACCESS_DENIED; fails(list_error); assert(!destroyed && !closed);
    reset(); enumerate_error=ERROR_ACCESS_DENIED; fails(enumerate_error); assert(destroyed==1 && closed==1);
    reset(); broken_path=true; fails(ERROR_INVALID_DATA); assert(destroyed==1 && !closed);
    reset(); invalid_size=true; fails(ERROR_INVALID_DATA); assert(destroyed==1 && !closed);
    reset(); open_error=ERROR_ACCESS_DENIED; result=read_windows_disks(api);
    assert(!result[0].capacity_bytes.value && result[0].capacity_bytes.support==pcinsight::Support::permission_required);
    assert(result[0].model.empty() && destroyed==1 && !closed);
    reset(); geometry_error=ERROR_ACCESS_DENIED; result=read_windows_disks(api);
    assert(result[0].model=="ACME Model X" && !result[0].capacity_bytes.value);
    reset(); invalid_descriptor=true; result=read_windows_disks(api); assert(result[0].model=="ACME");
    reset(); invalid_geometry=true; result=read_windows_disks(api); assert(!result[0].capacity_bytes.value);
    pcinsight::Monitor monitor; pcinsight::SampleOptions options;
    options.include_networks=false; options.include_processes=false; options.include_gpus=false;
    auto native=monitor.sample(options);
    for(const auto& disk:native.disks) { assert(!disk.id.empty()); if(disk.capacity_bytes.value) assert(*disk.capacity_bytes.value>0); }
    options.include_disks=false; assert(monitor.sample(options).disks.empty());
    std::cout<<"Native disk interfaces: "<<native.disks.size()<<'\n';
    for(const auto& disk:native.disks) std::cout<<disk.model<<" / "<<disk.bus_type<<" / "<<disk.capacity_bytes.value.value_or(0)<<" bytes\n";
}
