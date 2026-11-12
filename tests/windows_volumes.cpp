#include "../src/windows_volumes.hpp"
#include "pcinsight/monitor.hpp"
#include <cassert>
#include <cstring>
#include <cwchar>
#include <cwctype>
#include <iostream>
using namespace pcinsight;
using namespace pcinsight::detail;
namespace {
const wchar_t* volA=L"\\\\?\\Volume{a}\\";
const wchar_t* volB=L"\\\\?\\Volume{b}\\";
DWORD first_error,next_error,page_error,space_error,extent_error,root_error;
bool spanning,bad_count,duplicate,no_pages,exact_fs;
unsigned search_closed,handles_closed,enumerated;
HANDLE WINAPI first(LPWSTR buffer,DWORD size) {
    assert(size>=100);
    if(first_error) { SetLastError(first_error); return INVALID_HANDLE_VALUE; }
    std::wcscpy(buffer,volA); enumerated=1; return reinterpret_cast<HANDLE>(1);
}
BOOL WINAPI next(HANDLE h,LPWSTR buffer,DWORD size) {
    assert(h==reinterpret_cast<HANDLE>(1)&&size>=100);
    if(next_error) { SetLastError(next_error); return FALSE; }
    if(enumerated++==1) { std::wcscpy(buffer,duplicate?volA:volB); return TRUE; }
    SetLastError(ERROR_NO_MORE_FILES); return FALSE;
}
BOOL WINAPI finish(HANDLE h) { assert(h==reinterpret_cast<HANDLE>(1)); ++search_closed; return TRUE; }
BOOL WINAPI space(LPCWSTR name,PULARGE_INTEGER free,PULARGE_INTEGER total,PULARGE_INTEGER raw_free) {
    assert(!free&&!raw_free&&total);
    if(space_error) { SetLastError(space_error); return FALSE; }
    total->QuadPart=std::wstring(name)==volA?400:200; return TRUE;
}
UINT WINAPI windows(LPWSTR out,UINT size) { assert(size>=100); std::wcscpy(out,L"C:\\Windows"); return 10; }
BOOL WINAPI root(LPCWSTR path,LPWSTR out,DWORD size) {
    assert(size>=100);
    if(root_error) { SetLastError(root_error); return FALSE; }
    const auto p=std::wstring(path); assert(p==L"C:\\Windows"||p==L"C:\\pagefile.sys"||p==L"\\\\?\\C:\\pagefile.sys");
    std::wcscpy(out,L"C:\\"); return TRUE;
}
BOOL WINAPI name(LPCWSTR path,LPWSTR out,DWORD size) {
    assert(std::wstring(path)==L"C:\\"&&size>=100); std::wcscpy(out,volA); return TRUE;
}
BOOL WINAPI pages(PENUM_PAGE_FILE_CALLBACKW callback,LPVOID context) {
    if(page_error) { SetLastError(page_error); return FALSE; }
    if(!no_pages) { ENUM_PAGE_FILE_INFORMATION info{}; info.cb=sizeof info;
        assert(callback(context,&info,L"\\??\\C:\\pagefile.sys")); }
    return TRUE;
}
HANDLE WINAPI open(LPCWSTR path,DWORD access,DWORD share,LPSECURITY_ATTRIBUTES attrs,DWORD mode,DWORD flags,HANDLE template_file) {
    assert(!access&&share==(FILE_SHARE_READ|FILE_SHARE_WRITE)&&!attrs&&mode==OPEN_EXISTING&&!flags&&!template_file);
    auto p=std::wstring(path); for(auto& ch:p) ch=static_cast<wchar_t>(std::towlower(ch));
    assert(p==L"\\\\?\\volume{a}"||p==L"\\\\?\\volume{b}");
    return reinterpret_cast<HANDLE>(static_cast<std::uintptr_t>(p==L"\\\\?\\volume{a}"?2u:3u));
}
BOOL WINAPI close(HANDLE h) { assert(h==reinterpret_cast<HANDLE>(2)||h==reinterpret_cast<HANDLE>(3)); ++handles_closed; return TRUE; }
BOOL WINAPI query(HANDLE h,DWORD code,LPVOID input,DWORD input_size,LPVOID out,DWORD size,LPDWORD returned,LPOVERLAPPED ov) {
    if(code==FSCTL_GET_NTFS_VOLUME_DATA) {
        assert(!input&&!input_size&&!ov&&size==sizeof(NTFS_VOLUME_DATA_BUFFER));
        if(!exact_fs) { SetLastError(ERROR_INVALID_FUNCTION); return FALSE; }
        NTFS_VOLUME_DATA_BUFFER ntfs{}; ntfs.TotalClusters.QuadPart=1; ntfs.BytesPerCluster=512;
        std::memcpy(out,&ntfs,sizeof ntfs); *returned=sizeof ntfs; return TRUE;
    }
    assert(code==IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS&&!input&&!input_size&&!ov);
    if(extent_error) { SetLastError(extent_error); return FALSE; }
    const DWORD count=h==reinterpret_cast<HANDLE>(2)&&spanning?2:1;
    const DWORD bytes=offsetof(VOLUME_DISK_EXTENTS,Extents)+count*sizeof(DISK_EXTENT);
    assert(size>=bytes); std::memset(out,0,bytes);
    const DWORD reported=bad_count?9999:count; std::memcpy(out,&reported,sizeof reported);
    for(DWORD i=0;i<count;++i) { DISK_EXTENT extent{};
        extent.DiskNumber=h==reinterpret_cast<HANDLE>(2)?1+i:2; extent.ExtentLength.QuadPart=4096;
        std::memcpy(static_cast<char*>(out)+offsetof(VOLUME_DISK_EXTENTS,Extents)+i*sizeof extent,&extent,sizeof extent); }
    *returned=bytes; return TRUE;
}
VolumeApi api{first,next,finish,space,windows,root,name,pages,open,close,query};
void reset() { first_error=next_error=page_error=space_error=extent_error=root_error=0;
    spanning=bad_count=duplicate=no_pages=exact_fs=false; search_closed=handles_closed=enumerated=0; }
std::vector<DiskInfo> run() {
    std::vector<DiskInfo> result(3);
    for(unsigned i=0;i<3;++i) { result[i].id="disk"+std::to_string(i+1); result[i].capacity_bytes={800,Support::available,{}}; }
    fill_windows_volumes(result,{{1,"disk1"},{2,"disk2"},{3,"disk3"}},api); return result;
}
}
int main() {
    reset(); auto d=run();
    assert(d[0].formatted_capacity_bytes.value==400&&d[1].formatted_capacity_bytes.value==200&&d[2].formatted_capacity_bytes.value==0);
    assert(d[0].formatted_capacity_bytes.support==Support::estimated);
    assert(d[0].system_disk.value==true&&d[1].system_disk.value==false);
    assert(d[0].contains_page_file.value==true&&d[1].contains_page_file.value==false);
    assert(search_closed==1&&handles_closed==2); // Mapping cache avoids repeated handle queries.
    reset(); exact_fs=true; space_error=ERROR_ACCESS_DENIED; d=run();
    assert(d[0].formatted_capacity_bytes.value==512&&d[0].formatted_capacity_bytes.support==Support::available);
    reset(); spanning=true; d=run();
    assert(!d[0].formatted_capacity_bytes.value&&!d[1].formatted_capacity_bytes.value);
    assert(d[0].system_disk.value==true&&d[1].system_disk.value==true);
    assert(d[1].contains_page_file.value==true);
    reset(); page_error=ERROR_ACCESS_DENIED; d=run();
    assert(!d[0].contains_page_file.value&&d[0].contains_page_file.support==Support::permission_required);
    assert(d[0].system_disk.value==true&&d[0].formatted_capacity_bytes.value==400);
    reset(); no_pages=true; d=run(); assert(d[0].contains_page_file.value==false);
    reset(); root_error=ERROR_ACCESS_DENIED; d=run(); assert(!d[0].system_disk.value&&!d[0].contains_page_file.value);
    reset(); first_error=ERROR_ACCESS_DENIED; d=run();
    assert(!d[0].formatted_capacity_bytes.value&&d[0].system_disk.value==true&&search_closed==0);
    reset(); next_error=ERROR_ACCESS_DENIED; d=run(); assert(!d[0].formatted_capacity_bytes.value&&search_closed==1);
    reset(); space_error=ERROR_ACCESS_DENIED; d=run();
    assert(!d[0].formatted_capacity_bytes.value&&d[0].formatted_capacity_bytes.support==Support::permission_required);
    reset(); bad_count=true; d=run(); assert(!d[0].system_disk.value&&!d[0].formatted_capacity_bytes.value);
    reset(); extent_error=ERROR_ACCESS_DENIED; d=run(); assert(!d[0].contains_page_file.value&&!d[1].formatted_capacity_bytes.value);
    reset(); duplicate=true; d=run(); assert(!d[0].formatted_capacity_bytes.value);
    Monitor monitor; SampleOptions options; options.include_processes=false; options.include_networks=false; options.include_gpus=false;
    const auto native=monitor.sample(options); unsigned capacities=0,system=0,page=0;
    for(const auto& disk:native.disks) {
        if(disk.formatted_capacity_bytes.value) { ++capacities;
            if(disk.capacity_bytes.value) assert(*disk.formatted_capacity_bytes.value<=*disk.capacity_bytes.value); }
        if(disk.system_disk.value.value_or(false)) ++system;
        if(disk.contains_page_file.value.value_or(false)) ++page;
    }
    std::cout<<"Native mapped filesystem capacities: "<<capacities<<'/'<<native.disks.size()<<"; system="<<system<<"; pagefile="<<page<<'\n';
}
