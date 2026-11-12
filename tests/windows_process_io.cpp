#include "../src/windows_process_io.hpp"
#include "pcinsight/monitor.hpp"
#include <array>
#include <cassert>
#include <cmath>
#include <cstring>
#include <iostream>
#include <limits>
#include <stdexcept>
using namespace pcinsight;
using namespace pcinsight::detail;
using namespace std::chrono_literals;
namespace {
WindowsProcessIoObservation observation(std::uint32_t pid=1,std::uint64_t born=10,std::chrono::steady_clock::time_point at={}){return {pid,born,at,{WindowsProcessIoCounters{1,2,3,100,200,300},Support::available,"fixture"}};}
Metric<WindowsProcessIoDetails> one(WindowsProcessIoTracker& tracker,WindowsProcessIoObservation row){return tracker.sample(std::span(&row,1)).at(row.pid);}
#ifdef _WIN32
int mode,ids,times_called,reads;
HANDLE fake=reinterpret_cast<HANDLE>(static_cast<std::uintptr_t>(10));
DWORD WINAPI id(HANDLE value){assert(value==fake);++ids;if(mode==1){SetLastError(ERROR_ACCESS_DENIED);return 0;}return mode==2 && ids==2?2:1;}
BOOL WINAPI times(HANDLE value,LPFILETIME born,LPFILETIME exit,LPFILETIME kernel,LPFILETIME user){
    assert(value==fake);++times_called;*born={10,0};*exit={0,0};*kernel=*user={0,0};
    if(mode==3 || (mode==4 && times_called==2)){SetLastError(ERROR_ACCESS_DENIED);return FALSE;}
    if(mode==5 && times_called==2)born->dwLowDateTime=11;
    if(mode==6 && times_called==2)exit->dwLowDateTime=12;
    if(mode==17){SetLastError(0);return FALSE;}return TRUE;
}
BOOL WINAPI io(HANDLE value,PIO_COUNTERS out){
    assert(value==fake);++reads;
    if(mode==7 || (mode==8 && reads==2)){SetLastError(ERROR_ACCESS_DENIED);return FALSE;}
    IO_COUNTERS counters{1,2,3,100,200,300};
    if(mode==9)std::memset(&counters,0,sizeof(counters));
    if(mode==10)counters.ReadTransferCount=9007199254740993ULL;
    if(mode==11)std::memset(&counters,0xff,sizeof(counters));
    if(mode==12)return TRUE;
    if(mode==13 && reads==2){out->ReadOperationCount=1;return TRUE;}
    if(mode==14 && reads==2)counters.OtherOperationCount=2;
    *out=counters;return TRUE;
}
std::chrono::steady_clock::time_point now(){return std::chrono::steady_clock::time_point(1s);}
WindowsProcessIoApi api{id,times,io,now};
WindowsProcessIoObservation run(int scenario){mode=scenario;ids=times_called=reads=0;return read_windows_process_io(fake,1,10,api);}
struct Handle {HANDLE value{};~Handle(){if(value)CloseHandle(value);}};
#endif
}
int main(){
    WindowsProcessIoTracker tracker;auto a=observation();auto r=one(tracker,a);assert(r.value && !r.value->read_bytes_per_second.value);
    auto b=a;b.at+=2s;b.counters.value->read_transfer_bytes+=200;b.counters.value->write_transfer_bytes+=300;b.counters.value->other_transfer_bytes+=400;
    r=one(tracker,b);assert(r.value->read_bytes_per_second.value==100 && r.value->write_bytes_per_second.value==150 && r.value->other_bytes_per_second.value==200 && r.value->interval_seconds.value==2 && r.value->read_bytes_per_second.support==Support::estimated);
    b.at+=1s;r=one(tracker,b);assert(r.value->read_bytes_per_second.value==0);
    b.created=11;b.at+=1s;r=one(tracker,b);assert(!r.value->read_bytes_per_second.value);b.at+=1s;r=one(tracker,b);assert(r.value->read_bytes_per_second.value==0);
    b.counters.value->read_operations=0;b.at+=1s;r=one(tracker,b);assert(!r.value->read_bytes_per_second.value);
    auto bad=b;bad.counters={{},Support::permission_required,"denied"};r=one(tracker,bad);assert(!r.value && r.support==Support::permission_required);b.at+=1s;r=one(tracker,b);assert(!r.value->read_bytes_per_second.value);
    tracker.sample({});b.at+=1s;r=one(tracker,b);assert(!r.value->read_bytes_per_second.value);tracker.reset();r=one(tracker,b);assert(!r.value->read_bytes_per_second.value);
    r=one(tracker,b);assert(!r.value->read_bytes_per_second.value);auto earlier=b;earlier.at-=1s;r=one(tracker,earlier);assert(!r.value->read_bytes_per_second.value);
    auto huge=observation(1,1);huge.at=std::chrono::steady_clock::time_point::min();one(tracker,huge);huge.at=std::chrono::steady_clock::time_point::max();huge.counters.value->read_transfer_bytes=std::numeric_limits<std::uint64_t>::max();r=one(tracker,huge);assert(r.value->read_bytes_per_second.value && std::isfinite(*r.value->read_bytes_per_second.value) && r.value->counters.read_transfer_bytes==std::numeric_limits<std::uint64_t>::max());
    std::array duplicate{b,b};try{tracker.sample(duplicate);assert(false);}catch(const std::invalid_argument&){}b.at+=1s;r=one(tracker,b);assert(!r.value->read_bytes_per_second.value);
    auto idle=observation(0,0);r=one(tracker,idle);assert(!r.value);
    auto disguised=observation();disguised.counters.support=Support::unsupported;r=one(tracker,disguised);assert(!r.value);
    std::vector<WindowsProcessIoObservation> excessive(100001);try{tracker.sample(excessive);assert(false);}catch(const std::invalid_argument&){}r=one(tracker,b);assert(r.value && !r.value->read_bytes_per_second.value);
    tracker.reset();std::array pair{observation(1),observation(2)};tracker.sample(pair);pair[0].at+=1s;pair[1].counters={{},Support::permission_required,"one denied"};auto mixed=tracker.sample(pair);assert(mixed.at(1).value->read_bytes_per_second.value==0 && !mixed.at(2).value);
#ifdef _WIN32
    auto raw=run(0);assert(raw.counters.value && raw.counters.value->read_transfer_bytes==100);
    for(int scenario:{1,2,3,4,5,6,7,8,12,13,14,17}){raw=run(scenario);assert(!raw.counters.value);if(scenario==1 || scenario==3 || scenario==4 || scenario==7 || scenario==8)assert(raw.counters.support==Support::permission_required);}
    raw=run(9);assert(raw.counters.value && raw.counters.value->read_transfer_bytes==0);
    raw=run(10);assert(raw.counters.value->read_transfer_bytes==9007199254740993ULL);
    raw=run(11);assert(raw.counters.value->read_transfer_bytes==std::numeric_limits<std::uint64_t>::max());
    assert(!read_windows_process_io(nullptr,1,10,api).counters.value && !read_windows_process_io(fake,0,10,api).counters.value && !read_windows_process_io(fake,1,0,api).counters.value);
    for(unsigned missing=0;missing<4;++missing){auto partial=api;switch(missing){case 0:partial.id=nullptr;break;case 1:partial.times=nullptr;break;case 2:partial.io=nullptr;break;case 3:partial.now=nullptr;break;}assert(!read_windows_process_io(fake,1,10,partial).counters.value);}
    const auto pid=GetCurrentProcessId();Handle self{OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION,FALSE,pid)};assert(self.value);
    FILETIME born{},exit{},kernel{},user{};assert(GetProcessTimes(self.value,&born,&exit,&kernel,&user));const auto created=(std::uint64_t(born.dwHighDateTime)<<32)|born.dwLowDateTime;
    HANDLE read{},write{};assert(CreatePipe(&read,&write,nullptr,4096));Handle pipe_read{read},pipe_write{write};
    const auto first=read_windows_process_io(self.value,pid,created);assert(first.counters.value);
    std::array<char,1024> bytes{};DWORD written{},got{};assert(WriteFile(write,bytes.data(),static_cast<DWORD>(bytes.size()),&written,nullptr) && written==bytes.size());assert(ReadFile(read,bytes.data(),static_cast<DWORD>(bytes.size()),&got,nullptr) && got==bytes.size());
    const auto second=read_windows_process_io(self.value,pid,created);assert(second.counters.value && second.counters.value->read_transfer_bytes>=first.counters.value->read_transfer_bytes+1024 && second.counters.value->write_transfer_bytes>=first.counters.value->write_transfer_bytes+1024);
    tracker.reset();one(tracker,first);r=one(tracker,second);assert(r.value->read_bytes_per_second.value && *r.value->read_bytes_per_second.value>0);
    Monitor monitor;SampleOptions options;options.include_gpus=options.include_disks=options.include_networks=false;auto snapshot=monitor.sample(options);
    auto find_self=[&](const auto& s)->const ProcessInfo&{for(const auto& p:s.processes)if(p.pid==pid)return p;assert(false);throw std::runtime_error("self absent");};
    assert(find_self(snapshot).windows_io.value && !find_self(snapshot).windows_io.value->read_bytes_per_second.value);
    snapshot=monitor.sample(options);assert(find_self(snapshot).windows_io.value->read_bytes_per_second.value && !find_self(snapshot).disk_bytes_per_second.value && !find_self(snapshot).network_bytes_per_second.value);
    options.include_processes=false;assert(monitor.sample(options).processes.empty());options.include_processes=true;snapshot=monitor.sample(options);assert(find_self(snapshot).windows_io.value && !find_self(snapshot).windows_io.value->read_bytes_per_second.value);
    std::cout<<"Native private pipe I/O: read delta="<<second.counters.value->read_transfer_bytes-first.counters.value->read_transfer_bytes<<" write delta="<<second.counters.value->write_transfer_bytes-first.counters.value->write_transfer_bytes<<"; Monitor identity/rates/off-on passed\n";
#endif
    std::cout<<"Windows process general I/O tests passed\n";
}
