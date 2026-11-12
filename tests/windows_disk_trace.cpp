#include "../src/windows_disk_trace.hpp"
#include <cassert>
#include <cstring>
#include <cwchar>
#include <iostream>
#include <thread>
#include <vector>
using namespace pcinsight::detail;
namespace {
ULONG start_error, query_error, update_error, stop_error, absence_error;
unsigned starts, queries, updates, stops, name_queries;
int malformed;
bool enabled;
ULONG lost_events,lost_realtime,lost_log;
constexpr TRACEHANDLE fake_handle = 123;
ULONG WINAPI start(PTRACEHANDLE handle,LPCWSTR name,PEVENT_TRACE_PROPERTIES p) {
    ++starts;
    assert(std::wcscmp(name,disk_trace_name)==0);
    assert(p->Wnode.BufferSize>=sizeof(*p)+2048*sizeof(wchar_t));
    assert(p->Wnode.Flags==WNODE_FLAG_TRACED_GUID && p->Wnode.ClientContext==1);
    assert(std::memcmp(&p->Wnode.Guid,&disk_trace_guid,sizeof(GUID))==0);
    assert(p->BufferSize==64 && p->MinimumBuffers==4 && p->MaximumBuffers==64);
    assert(p->LogFileNameOffset==0 && p->EnableFlags==0);
    assert(p->LogFileMode==(EVENT_TRACE_SYSTEM_LOGGER_MODE|EVENT_TRACE_REAL_TIME_MODE|EVENT_TRACE_NO_PER_PROCESSOR_BUFFERING|EVENT_TRACE_STOP_ON_HYBRID_SHUTDOWN));
    *handle=fake_handle; // Even failure output must not confer ownership.
    return start_error;
}
ULONG WINAPI control(TRACEHANDLE handle,LPCWSTR name,PEVENT_TRACE_PROPERTIES p,ULONG code) {
    assert(p->Wnode.Flags==WNODE_FLAG_TRACED_GUID);
    if(!handle) {
        assert(code==EVENT_TRACE_CONTROL_QUERY && name && std::wcscmp(name,disk_trace_name)==0);
        ++name_queries;return absence_error;
    }
    assert(handle==fake_handle && !name);
    if(code==EVENT_TRACE_CONTROL_STOP) {++stops;p->EventsLost=lost_events;p->RealTimeBuffersLost=lost_realtime;p->LogBuffersLost=lost_log;return stop_error;}
    if(code==EVENT_TRACE_CONTROL_UPDATE) {
        ++updates;assert(p->EnableFlags==EVENT_TRACE_FLAG_DISK_IO && !p->LogFileNameOffset);
        assert(p->LogFileMode==EVENT_TRACE_REAL_TIME_MODE);
        if(!update_error)enabled=true;
        return update_error;
    }
    assert(code==EVENT_TRACE_CONTROL_QUERY);++queries;
    if(query_error)return query_error;
    p->BufferSize=64;p->NumberOfBuffers=4;p->MaximumBuffers=64;
    p->EnableFlags=enabled?EVENT_TRACE_FLAG_DISK_IO:0;
    p->LogFileMode=EVENT_TRACE_SYSTEM_LOGGER_MODE|EVENT_TRACE_REAL_TIME_MODE|EVENT_TRACE_NO_PER_PROCESSOR_BUFFERING|EVENT_TRACE_STOP_ON_HYBRID_SHUTDOWN;
    switch(malformed) {
        case 1:p->Wnode.Guid.Data1++;break;
        case 2:p->MaximumBuffers=0;break;
        case 3:p->NumberOfBuffers=0;break;
        case 4:p->MaximumBuffers=3;break;
        case 5:p->BufferSize=3;break;
        case 6:p->BufferSize=0xffffffffU;p->MaximumBuffers=0xffffffffU;break;
        case 7:p->EnableFlags|=EVENT_TRACE_FLAG_NETWORK_TCPIP;break;
        case 8:p->LogFileMode|=EVENT_TRACE_FILE_MODE_CIRCULAR;break;
        case 9:reinterpret_cast<wchar_t*>(reinterpret_cast<BYTE*>(p)+p->LogFileNameOffset)[0]=L'x';break;
        case 10:p->MaximumBuffers=1024;break; // Exact64MiB ceiling accepted.
        case 11:p->MaximumBuffers=1025;break;
    }
    return 0;
}
DiskTraceApi api{start,control};
void reset(){start_error=query_error=update_error=stop_error=0;absence_error=ERROR_WMI_INSTANCE_NOT_FOUND;
    starts=queries=updates=stops=name_queries=0;malformed=0;enabled=false;lost_events=lost_realtime=lost_log=0;}
}
int main(){
    reset();{WindowsDiskTrace s(api);assert(!s.active());assert(s.start()==0 && s.active());
        assert(s.start()==ERROR_INVALID_PARAMETER && starts==1);assert(s.enable_disk()==0);
        assert(s.enable_disk()==ERROR_INVALID_PARAMETER && updates==1);
        const auto r=s.stop();assert(r.confirmed && !r.stop_error && !s.active());
        assert(s.stop().confirmed && stops==1 && name_queries==1);}
    for(auto error:{ERROR_ACCESS_DENIED,ERROR_ALREADY_EXISTS,ERROR_NO_SYSTEM_RESOURCES,ERROR_PRIVILEGE_NOT_HELD}){
        reset();{WindowsDiskTrace s(api);start_error=error;assert(s.start()==error && !s.active());
            assert(!s.stop().confirmed);assert(s.enable_disk()==ERROR_INVALID_PARAMETER);}
        assert(!stops && !queries && !updates && !name_queries);
    }
    for(int mode=1;mode<=9;++mode){reset();malformed=mode;
        {WindowsDiskTrace s(api);assert(s.start()==ERROR_INVALID_DATA && !s.active());assert(s.stop().confirmed);}
        assert(stops==1 && !updates && name_queries==1);
    }
    reset();malformed=10;{WindowsDiskTrace s(api);assert(!s.start());assert(s.stop().confirmed);}assert(stops==1);
    reset();malformed=11;{WindowsDiskTrace s(api);assert(s.start()==ERROR_INVALID_DATA);assert(s.stop().confirmed);}assert(stops==1);
    reset();{WindowsDiskTrace s(api);assert(!s.start());query_error=ERROR_INVALID_HANDLE;
        assert(s.enable_disk()==ERROR_INVALID_HANDLE && s.stop().confirmed);}assert(stops==1);
    reset();query_error=ERROR_ACCESS_DENIED;{WindowsDiskTrace s(api);assert(s.start()==ERROR_ACCESS_DENIED);assert(s.stop().confirmed);}assert(stops==1);
    reset();{WindowsDiskTrace s(api);assert(!s.start());update_error=ERROR_INVALID_PARAMETER;
        assert(s.enable_disk()==ERROR_INVALID_PARAMETER && !s.active());assert(s.stop().confirmed);}assert(stops==1);
    reset();{WindowsDiskTrace s(api);assert(!s.start());malformed=7;
        assert(s.enable_disk()==ERROR_INVALID_DATA && s.stop().confirmed);}assert(stops==1);
    reset();{WindowsDiskTrace s(api);assert(!s.start());}assert(stops==1 && name_queries==1);
    // A new session may already occupy the name after ours stopped: never STOP
    // it by name or retry the potentially recycled handle, even in destructor.
    reset();{WindowsDiskTrace s(api);assert(!s.start());absence_error=ERROR_SUCCESS;
        auto r=s.stop();assert(!r.confirmed && !r.stop_error && !r.query_error);s.stop();}assert(stops==1);
    reset();{WindowsDiskTrace s(api);assert(!s.start());stop_error=ERROR_ACCESS_DENIED;absence_error=ERROR_ACCESS_DENIED;
        auto r=s.stop();assert(!r.confirmed && r.stop_error==ERROR_ACCESS_DENIED && r.query_error==ERROR_ACCESS_DENIED);}assert(stops==1);
    reset();{WindowsDiskTrace s({nullptr,control});assert(s.start()==ERROR_INVALID_PARAMETER);}assert(!starts && !stops);
    reset();{WindowsDiskTrace s({start,nullptr});assert(s.start()==ERROR_INVALID_PARAMETER);}assert(!starts && !stops);
    reset();{WindowsDiskTrace s(api);assert(!s.start());lost_events=3;lost_realtime=4;lost_log=5;
        const auto r=s.stop();assert(r.confirmed && r.statistics_valid && r.events_lost==3 && r.realtime_buffers_lost==4 && r.log_buffers_lost==5);
        assert(s.stop().events_lost==3);}assert(stops==1);
    reset();{WindowsDiskTrace s(api);assert(!s.start());stop_error=ERROR_ACCESS_DENIED;
        assert(!s.stop().statistics_valid);}assert(stops==1);
    reset();{WindowsDiskTrace s(api);assert(!s.start());std::vector<std::thread> workers;
        for(unsigned i=0;i<8;++i)workers.emplace_back([&]{assert(s.stop().confirmed);});
        for(auto& worker:workers)worker.join();}assert(stops==1 && name_queries==1);
    std::cout<<"ETW disk lifecycle: ownership, collisions, budget, flags, errors, one-shot cleanup passed (injected only)\n";
}
