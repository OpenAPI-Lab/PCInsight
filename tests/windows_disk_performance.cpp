#include "../src/windows_disk_performance.hpp"
#include "pcinsight/monitor.hpp"
#include <pdhmsg.h>
#include <cassert>
#include <cstring>
#include <cwchar>
#include <iostream>
#include <limits>
#include <thread>
using namespace pcinsight;
using namespace pcinsight::detail;
namespace {
PDH_STATUS open_error,add_error,collect_error,format_error;
unsigned opened,closed,added,collected;
DWORD cstatus;
int malformed;
bool duplicate;
double data[4];
PDH_STATUS WINAPI open(LPCWSTR source,DWORD_PTR user,PDH_HQUERY* query) {
    assert(!source && !user); ++opened;
    if(open_error) return open_error;
    added=0;
    *query=reinterpret_cast<PDH_HQUERY>(1); return ERROR_SUCCESS;
}
PDH_STATUS WINAPI add(PDH_HQUERY query,LPCWSTR path,DWORD_PTR user,PDH_HCOUNTER* counter) {
    assert(query==reinterpret_cast<PDH_HQUERY>(1) && !user && std::wstring(path).find(L"\\PhysicalDisk(*)\\")==0);
    if(add_error) return add_error;
    *counter=reinterpret_cast<PDH_HCOUNTER>(std::uintptr_t(++added)); return ERROR_SUCCESS;
}
PDH_STATUS WINAPI collect(PDH_HQUERY query) { assert(query==reinterpret_cast<PDH_HQUERY>(1)); ++collected; return collect_error; }
PDH_STATUS WINAPI close(PDH_HQUERY query) { assert(query==reinterpret_cast<PDH_HQUERY>(1)); ++closed; return ERROR_SUCCESS; }
PDH_STATUS WINAPI format(PDH_HCOUNTER counter,DWORD flags,LPDWORD size,LPDWORD count,PPDH_FMT_COUNTERVALUE_ITEM_W output) {
    const auto index=reinterpret_cast<std::uintptr_t>(counter)-1; assert(index<4);
    assert(flags==(PDH_FMT_DOUBLE|PDH_FMT_NOCAP100));
    if(format_error) return format_error;
    constexpr wchar_t name[]=L"0 C:"; constexpr wchar_t total[]=L"_Total";
    constexpr DWORD needed=2*sizeof(PDH_FMT_COUNTERVALUE_ITEM_W)+sizeof name+sizeof total;
    *count=2;
    if(!output) { assert(!*size); *size=needed; return PDH_MORE_DATA; }
    assert(*size>=needed); *size=needed;
    auto strings=reinterpret_cast<wchar_t*>(reinterpret_cast<unsigned char*>(output)+2*sizeof(*output));
    std::memcpy(strings,name,sizeof name);
    auto second=reinterpret_cast<wchar_t*>(reinterpret_cast<unsigned char*>(strings)+sizeof name);
    std::memcpy(second,duplicate?name:total,duplicate?sizeof name:sizeof total);
    output[0].szName=strings; output[0].FmtValue.CStatus=cstatus; output[0].FmtValue.doubleValue=data[index];
    output[1].szName=second; output[1].FmtValue.CStatus=PDH_CSTATUS_VALID_DATA; output[1].FmtValue.doubleValue=9999;
    if(malformed==1) *count=10000;
    if(malformed==2) output[0].szName=reinterpret_cast<wchar_t*>(std::uintptr_t(1));
    if(malformed==3) { std::memset(strings,'x',sizeof name+sizeof total); }
    return ERROR_SUCCESS;
}
DiskPdhApi api{open,add,collect,format,close};
void reset() {
    open_error=add_error=collect_error=format_error=0; opened=closed=added=collected=0;
    cstatus=PDH_CSTATUS_VALID_DATA; malformed=0; duplicate=false;
    data[0]=100.75; data[1]=200.25; data[2]=80; data[3]=0.002;
}
std::vector<DiskInfo> disks() { DiskInfo d; d.id="disk A"; return {d}; }
const std::map<std::uint32_t,std::string> mapping{{0,"disk A"}};
}
int main() {
    reset(); {
        WindowsDiskPerformance provider(api); auto result=disks();
        provider.fill(result,mapping); assert(!result[0].read_bytes_per_second.value && added==4);
        provider.fill(result,mapping);
        assert(result[0].read_bytes_per_second.value==100 && result[0].write_bytes_per_second.value==200);
        assert(result[0].transfer_bytes_per_second.value==300 && result[0].active_percent.value==20);
        assert(result[0].average_response_ms.value==2);
        data[2]=150; provider.fill(result,mapping); assert(!result[0].active_percent.value);
        data[0]=std::numeric_limits<double>::quiet_NaN(); provider.fill(result,mapping);
        assert(!result[0].read_bytes_per_second.value && !result[0].transfer_bytes_per_second.value);
        data[0]=-1; provider.fill(result,mapping); assert(!result[0].read_bytes_per_second.value);
        data[0]=std::numeric_limits<double>::max(); provider.fill(result,mapping);
        assert(!result[0].read_bytes_per_second.value);
        data[0]=0; data[1]=0; data[2]=100; data[3]=0; provider.fill(result,mapping);
        assert(result[0].read_bytes_per_second.value==0 && result[0].active_percent.value==0);
        cstatus=PDH_CSTATUS_INVALID_DATA; provider.fill(result,mapping); assert(!result[0].average_response_ms.value);
        cstatus=PDH_CSTATUS_VALID_DATA; duplicate=true; provider.fill(result,mapping); assert(!result[0].read_bytes_per_second.value);
        duplicate=false;
        for(malformed=1;malformed<=3;++malformed) { provider.fill(result,mapping); assert(!result[0].read_bytes_per_second.value); }
        malformed=0; format_error=PDH_ACCESS_DENIED; provider.fill(result,mapping);
        assert(result[0].read_bytes_per_second.support==Support::permission_required);
        format_error=0; provider.fill(result,mapping); assert(result[0].read_bytes_per_second.value==0);
        provider.fill(result,{}); assert(!result[0].read_bytes_per_second.value && closed==1);
    }
    for(int failure=0;failure<3;++failure) {
        reset(); WindowsDiskPerformance provider(api); auto result=disks();
        if(failure==0) open_error=ERROR_ACCESS_DENIED;
        if(failure==1) add_error=PDH_CSTATUS_NO_COUNTER;
        if(failure==2) collect_error=PDH_ACCESS_DENIED;
        provider.fill(result,mapping); assert(!result[0].active_percent.value);
        assert(closed==(failure==0?0u:1u));
    }
    reset(); {
        WindowsDiskPerformance provider(api); auto result=disks(); provider.fill(result,mapping);
        provider.fill(result,mapping); assert(result[0].active_percent.value);
        result[0].id="replacement";
        provider.fill(result,{{0,"replacement"}}); assert(!result[0].active_percent.value && closed==1);
        // Counter handle mock numbers restart with each new query.
    }
    reset(); {
        WindowsDiskPerformance provider(api); auto result=disks(); provider.fill(result,mapping);
        collect_error=PDH_ACCESS_DENIED; provider.fill(result,mapping); assert(closed==1);
        collect_error=0; added=0; provider.fill(result,mapping); assert(!result[0].active_percent.value);
        provider.fill(result,mapping); assert(result[0].active_percent.value);
    }
    // Native integration: only reads existing counters and inventory.
    Monitor monitor; SampleOptions options; options.include_processes=false; options.include_networks=false; options.include_gpus=false;
    const auto first=monitor.sample(options);
    for(const auto& disk:first.disks) assert(!disk.read_bytes_per_second.value);
    std::this_thread::sleep_for(std::chrono::milliseconds(250));
    const auto next=monitor.sample(options); unsigned available=0,writes=0,active=0,latency=0;
    for(const auto& disk:next.disks) {
        if(disk.read_bytes_per_second.value) ++available;
        if(disk.write_bytes_per_second.value) ++writes;
        if(disk.active_percent.value) ++active;
        else std::cout<<"Native active-time unavailable: "<<disk.active_percent.note<<'\n';
        if(disk.average_response_ms.value) ++latency;
        if(disk.active_percent.value) assert(*disk.active_percent.value>=0 && *disk.active_percent.value<=100);
        if(disk.average_response_ms.value) assert(*disk.average_response_ms.value>=0);
    }
    options.include_disks=false; assert(monitor.sample(options).disks.empty());
    options.include_disks=true;
    for(const auto& disk:monitor.sample(options).disks) assert(!disk.read_bytes_per_second.value);
    std::cout<<"Native disks with read rates: "<<available<<'/'<<next.disks.size()
        <<"; write="<<writes<<"; active="<<active<<"; latency="<<latency<<'\n';
}
