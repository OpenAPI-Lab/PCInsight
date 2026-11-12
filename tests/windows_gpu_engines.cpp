#include "../src/windows_gpu_engines.hpp"
#include "../src/windows_gpus.hpp"
#include "pcinsight/monitor.hpp"
#include <pdhmsg.h>
#include <cassert>
#include <cmath>
#include <cstring>
#include <iostream>
#include <limits>
#include <thread>
using namespace pcinsight;
using namespace pcinsight::detail;
using namespace std::chrono_literals;
namespace {
struct Row {std::wstring name;double percent;DWORD status{PDH_CSTATUS_VALID_DATA};};
unsigned opens,closes,formats;int token;bool alive,null_query,null_counter,bad_pointer,bad_count,bad_size,unterminated;
PDH_STATUS open_error,add_error,collect_error,format_error;
std::vector<Row> rows;
PDH_STATUS WINAPI open(LPCWSTR source,DWORD_PTR user,PDH_HQUERY* out){assert(!source && !user && !alive);++opens;alive=!null_query;*out=null_query?nullptr:reinterpret_cast<PDH_HQUERY>(&token);return open_error;}
PDH_STATUS WINAPI add(PDH_HQUERY query,LPCWSTR path,DWORD_PTR user,PDH_HCOUNTER* out){assert(alive && query==reinterpret_cast<PDH_HQUERY>(&token) && !user && std::wstring(path)==L"\\GPU Engine(*)\\Utilization Percentage");*out=null_counter?nullptr:reinterpret_cast<PDH_HCOUNTER>(&token);return add_error;}
PDH_STATUS WINAPI collect(PDH_HQUERY query){assert(alive && query==reinterpret_cast<PDH_HQUERY>(&token));return collect_error;}
PDH_STATUS WINAPI close(PDH_HQUERY query){assert(alive && query==reinterpret_cast<PDH_HQUERY>(&token));alive=false;++closes;return ERROR_SUCCESS;}
PDH_STATUS WINAPI format(PDH_HCOUNTER counter,DWORD flags,LPDWORD size,LPDWORD count,PPDH_FMT_COUNTERVALUE_ITEM_W out){
    assert(alive && counter==reinterpret_cast<PDH_HCOUNTER>(&token) && flags==(PDH_FMT_DOUBLE|PDH_FMT_NOSCALE|PDH_FMT_NOCAP100));++formats;
    if(format_error)return format_error;
    DWORD required=static_cast<DWORD>(rows.size()*sizeof(PDH_FMT_COUNTERVALUE_ITEM_W));
    for(const auto& row:rows)required+=static_cast<DWORD>((row.name.size()+1)*sizeof(wchar_t));
    if(!out){*size=bad_size?9*1024*1024:required;*count=static_cast<DWORD>(rows.size());return PDH_MORE_DATA;}
    assert(*size>=required);*size=required;*count=bad_count?65537:static_cast<DWORD>(rows.size());
    auto* cursor=reinterpret_cast<unsigned char*>(out)+rows.size()*sizeof(PDH_FMT_COUNTERVALUE_ITEM_W);
    for(std::size_t i=0;i<rows.size();++i){PDH_FMT_COUNTERVALUE_ITEM_W item{};item.szName=reinterpret_cast<wchar_t*>(cursor);
        std::memcpy(cursor,rows[i].name.c_str(),(rows[i].name.size()+1)*sizeof(wchar_t));
        if(unterminated)std::wmemset(item.szName,L'x',rows[i].name.size()+1);
        if(bad_pointer)item.szName=reinterpret_cast<wchar_t*>(out);
        item.FmtValue.CStatus=rows[i].status;item.FmtValue.doubleValue=rows[i].percent;
        std::memcpy(reinterpret_cast<unsigned char*>(out)+i*sizeof item,&item,sizeof item);cursor+=(rows[i].name.size()+1)*sizeof(wchar_t);
    }
    return ERROR_SUCCESS;
}
GpuMemoryPdhApi api{open,add,collect,format,close};
std::wstring instance(unsigned pid,unsigned engine,const std::wstring& type=L"3D",unsigned physical=0){return L"pid_"+std::to_wstring(pid)+L"_luid_0xABCDEF01_0x00000002_phys_"+std::to_wstring(physical)+L"_eng_"+std::to_wstring(engine)+L"_engtype_"+type;}
std::vector<GpuInfo> gpus(){GpuInfo gpu;gpu.id="dxgi-luid:abcdef01:00000002";return {gpu};}
void reset(){assert(!alive);opens=closes=formats=0;null_query=null_counter=bad_pointer=bad_count=bad_size=unterminated=false;open_error=add_error=collect_error=format_error=0;
    rows={{instance(10,0),20},{instance(11,0),30},{instance(10,1,L"Copy"),15},{instance(12,2,L"Compute_0"),0},{L"_Total",99}};}
std::vector<GpuInfo> read(){auto gpu=gpus();{WindowsGpuEngines engines(api);engines.fill(gpu);assert(gpu[0].engines.empty() && !gpu[0].utilization_percent.value);engines.fill(gpu);}assert(!alive && (opens==closes || null_query));return gpu;}
}
int main(){
    reset();auto result=read();assert(result[0].engines.size()==3 && result[0].utilization_percent.value==50 && result[0].utilization_percent.support==Support::estimated);
    assert(result[0].engines[0].id=="phys:0/eng:0" && result[0].engines[0].type=="3D" && result[0].engines[0].utilization_percent.value==50);
    assert(result[0].engines[1].type=="Copy" && result[0].engines[2].utilization_percent.value==0);
    reset();rows.push_back({instance(10,3,L"Compute_1"),60});rows.push_back({instance(10,0,L"3D",1),70});result=read();assert(result[0].engines.size()==5 && result[0].utilization_percent.value==70);
    reset();rows[0].percent=100;rows[1].percent=0.0000005;result=read();assert(result[0].engines[0].utilization_percent.value==100);
    reset();rows[0].percent=100;rows[1].percent=0;for(unsigned i=20;i<40;++i)rows.push_back({instance(i,0),0.0000001});result=read();assert(!result[0].engines[0].utilization_percent.value);
    reset();rows[0].percent=80;rows[1].percent=30;result=read();assert(!result[0].engines[0].utilization_percent.value && !result[0].utilization_percent.value);
    reset();rows.push_back(rows[0]);result=read();assert(!result[0].engines[0].utilization_percent.value && result[0].engines[1].utilization_percent.value==15);
    reset();rows[1].name=instance(11,0,L"Compute");result=read();assert(result[0].engines[0].type.empty() && !result[0].utilization_percent.value);
    for(double value:{-1.0,101.0,std::numeric_limits<double>::quiet_NaN(),std::numeric_limits<double>::infinity()}){reset();rows[0].percent=value;result=read();assert(!result[0].utilization_percent.value);}
    reset();rows[0].status=PDH_ACCESS_DENIED;rows.push_back(rows[0]);rows[1].percent=-1;result=read();assert(result[0].engines[0].utilization_percent.support==Support::permission_required && result[0].utilization_percent.support==Support::permission_required);
    for(const auto& name:{instance(10,0)+L"#1",std::wstring(L"pid_4294967296_luid_0xABCDEF01_0x00000002_phys_0_eng_0_engtype_3D"),std::wstring(L"pid_10_luid_0xXYZDEF01_0x00000002_phys_0_eng_0_engtype_3D"),std::wstring(L"unexpected")}){
        reset();rows[0].name=name;result=read();assert(result[0].engines.empty() && !result[0].utilization_percent.value);}
    for(unsigned fault=0;fault<4;++fault){reset();bad_pointer=fault==0;bad_count=fault==1;bad_size=fault==2;unterminated=fault==3;result=read();assert(result[0].engines.empty());}
    reset();rows={{L"_Total",90}};result=read();assert(result[0].engines.empty() && !result[0].utilization_percent.value);
    reset();rows.clear();for(unsigned i=0;i<4097;++i)rows.push_back({instance(10,i),0});result=read();assert(result[0].engines.empty() && !result[0].utilization_percent.value);
    reset();rows[0].name=instance(10,0,std::wstring(65,L'a'));result=read();assert(result[0].engines.empty());
    reset();open_error=ERROR_ACCESS_DENIED;result=read();assert(result[0].utilization_percent.support==Support::permission_required);
    reset();add_error=PDH_CSTATUS_NO_COUNTER;result=read();assert(!result[0].utilization_percent.value);
    reset();null_query=true;result=read();assert(!result[0].utilization_percent.value);
    reset();null_counter=true;result=read();assert(!result[0].utilization_percent.value);
    reset();format_error=PDH_MORE_DATA;result=read();assert(!result[0].utilization_percent.value);
    reset();{
        WindowsGpuEngines engines(api);auto gpu=gpus();engines.fill(gpu);engines.fill(gpu);assert(gpu[0].utilization_percent.value==50);
        rows[0].percent=5;rows[1].percent=10;engines.fill(gpu);assert(gpu[0].utilization_percent.value==15);
        collect_error=ERROR_ACCESS_DENIED;engines.fill(gpu);assert(!gpu[0].utilization_percent.value && gpu[0].engines.empty());
        collect_error=0;engines.fill(gpu);assert(!gpu[0].utilization_percent.value);engines.fill(gpu);assert(gpu[0].utilization_percent.value==15);
        gpu[0].id="dxgi-luid:00000000:00000003";engines.fill(gpu);assert(!gpu[0].utilization_percent.value);engines.fill(gpu);assert(gpu[0].engines.empty());
        gpu.clear();engines.fill(gpu);assert(!alive);
    }
    reset();{WindowsGpuEngines engines(api);auto gpu=gpus();gpu.push_back(gpu.front());engines.fill(gpu);assert(opens==0);}
    auto native=read_windows_gpus();{WindowsGpuEngines engines;engines.fill(native);std::this_thread::sleep_for(200ms);engines.fill(native);
        for(const auto& gpu:native){std::cout<<gpu.model<<" engines="<<gpu.engines.size()<<" overall="<<(gpu.utilization_percent.value?std::to_string(*gpu.utilization_percent.value):gpu.utilization_percent.note)<<'\n';
            for(const auto& engine:gpu.engines)std::cout<<engine.name<<" = "<<(engine.utilization_percent.value?std::to_string(*engine.utilization_percent.value):engine.utilization_percent.note)<<'\n';}}
    Monitor monitor;SampleOptions options;options.include_disks=options.include_networks=options.include_processes=false;
    monitor.sample(options);std::this_thread::sleep_for(100ms);const auto snapshot=monitor.sample(options);
    for(const auto& gpu:snapshot.gpus)for(const auto& engine:gpu.engines){assert(!engine.id.empty());if(engine.utilization_percent.value)assert(*engine.utilization_percent.value>=0 && *engine.utilization_percent.value<=100);}
    options.include_gpus=false;assert(monitor.sample(options).gpus.empty());options.include_gpus=true;
    for(const auto& gpu:monitor.sample(options).gpus)assert(gpu.engines.empty() && !gpu.utilization_percent.value);
}
