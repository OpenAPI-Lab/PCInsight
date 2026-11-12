// Manual human-authorized probe. Never registered with CTest.
#include "../src/windows_disk_trace.hpp"
#include "../src/windows_disk_events.hpp"
#include <evntcons.h>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <string>
#include <thread>
#include <stdexcept>
#include <cstring>
#include <limits>
using namespace pcinsight::detail;
namespace {
struct Handle {HANDLE h=nullptr;~Handle(){if(h && h!=INVALID_HANDLE_VALUE)CloseHandle(h);}};
struct Privilege {
    Handle token;TOKEN_PRIVILEGES previous{};bool changed=false;
    ULONG enable(){
        if(!OpenProcessToken(GetCurrentProcess(),TOKEN_ADJUST_PRIVILEGES|TOKEN_QUERY,&token.h))return GetLastError();
        TOKEN_PRIVILEGES requested{};requested.PrivilegeCount=1;
        if(!LookupPrivilegeValueW(nullptr,L"SeSystemProfilePrivilege",&requested.Privileges[0].Luid))return GetLastError();
        requested.Privileges[0].Attributes=SE_PRIVILEGE_ENABLED;
        DWORD size=sizeof(previous);SetLastError(0);
        if(!AdjustTokenPrivileges(token.h,FALSE,&requested,sizeof(previous),&previous,&size))return GetLastError();
        const auto error=GetLastError();changed=error==ERROR_SUCCESS;return error;
    }
    ~Privilege(){if(changed)AdjustTokenPrivileges(token.h,FALSE,&previous,0,nullptr,nullptr);}
};
// Deliberately discard every event: this unit validates access/lifecycle only.
// No metadata, paths, command lines, addresses, PIDs, payloads or counts retained.
void WINAPI discard(PEVENT_RECORD) {}
struct Capture {
    std::mutex mutex;
    PrivateDiskTotals totals;
    DWORD thread=0;
    std::atomic<std::int64_t> begin{0},end{std::numeric_limits<std::int64_t>::max()};
    bool invalid=false;
    ULONG schema_error=0;
};
void WINAPI record_private(PEVENT_RECORD event){
    auto& capture=*static_cast<Capture*>(event->UserContext);
    const auto begin=capture.begin.load();if(!begin)return;
    const auto decoded=decode_windows_disk_event(*event,{},capture.thread);
    if(decoded.status==DiskDecodeStatus::ignored)return;
    std::lock_guard lock(capture.mutex);
    if(decoded.status==DiskDecodeStatus::invalid){capture.schema_error=decoded.error;return;}
    const auto result=accumulate_private_disk(capture.totals,decoded.event,capture.thread,begin,capture.end.load());
    if(result==DiskAccumulation::invalid)capture.invalid=true;
    // Nothing from non-matching threads is persisted, not even their counts.
}
std::int64_t qpc(){LARGE_INTEGER value{};if(!QueryPerformanceCounter(&value)||value.QuadPart<=0)throw std::runtime_error("QPC unavailable");return value.QuadPart;}
void require(bool ok,const char* operation){if(!ok)throw std::runtime_error(std::string(operation)+" Win32="+std::to_string(GetLastError()));}
struct Buffer{void* p=VirtualAlloc(nullptr,65536,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);~Buffer(){if(p)VirtualFree(p,0,MEM_RELEASE);}};
void private_io(const std::wstring& path,Capture& capture){
    Handle file{CreateFileW(path.c_str(),GENERIC_READ|GENERIC_WRITE|DELETE,0,nullptr,CREATE_NEW,
        FILE_ATTRIBUTE_NORMAL|FILE_FLAG_DELETE_ON_CLOSE|FILE_FLAG_NO_BUFFERING|FILE_FLAG_WRITE_THROUGH,nullptr)};
    require(file.h!=INVALID_HANDLE_VALUE,"create private no-buffering file");
    FILE_STORAGE_INFO storage{};require(GetFileInformationByHandleEx(file.h,FileStorageInfo,&storage,sizeof(storage))!=0,"query sector sizes");
    Buffer buffer;require(buffer.p!=nullptr,"allocate aligned buffer");
    for(auto sector:{storage.LogicalBytesPerSector,storage.PhysicalBytesPerSectorForPerformance,storage.PhysicalBytesPerSectorForAtomicity})
        require(sector && sector<=65536 && 65536%sector==0 && reinterpret_cast<std::uintptr_t>(buffer.p)%sector==0,"sector alignment unsupported");
    std::memset(buffer.p,0x7b,65536);
    capture.begin=qpc();
    for(unsigned i=0;i<16;++i){DWORD n=0;require(WriteFile(file.h,buffer.p,65536,&n,nullptr)!=0 && n==65536,"private write");}
    LARGE_INTEGER zero{};require(SetFilePointerEx(file.h,zero,nullptr,FILE_BEGIN)!=0,"private rewind");
    for(unsigned i=0;i<16;++i){DWORD n=0;std::memset(buffer.p,0,65536);require(ReadFile(file.h,buffer.p,65536,&n,nullptr)!=0 && n==65536,"private read");
        for(unsigned j=0;j<65536;++j)require(static_cast<BYTE*>(buffer.p)[j]==0x7b,"private readback mismatch");}
    capture.end=qpc();
    // The CREATE_NEW file is deleted by its own handle, never by path/glob.
}
void write(HANDLE out,const std::string& text){const auto line=text+"\r\n";DWORD n{};
    std::printf("%s\n",text.c_str());if(out && out!=INVALID_HANDLE_VALUE){WriteFile(out,line.data(),static_cast<DWORD>(line.size()),&n,nullptr);FlushFileBuffers(out);}}
HANDLE evidence_sink=nullptr;
ULONG WINAPI control(TRACEHANDLE h,LPCWSTR name,PEVENT_TRACE_PROPERTIES p,ULONG action){
    const auto error=ControlTraceW(h,name,p,action);
    if(h && action==EVENT_TRACE_CONTROL_QUERY && !error){
        write(evidence_sink,"Session metadata: mode="+std::to_string(p->LogFileMode)+" flags="+std::to_string(p->EnableFlags)+
            " bufferKiB="+std::to_string(p->BufferSize)+" count="+std::to_string(p->NumberOfBuffers)+
            " max="+std::to_string(p->MaximumBuffers)+" guidMatch="+std::to_string(std::memcmp(&p->Wnode.Guid,&disk_trace_guid,sizeof(GUID))==0));
    }
    return error;
}
}
int main(int argc,char** argv){
    if(argc!=2 || (std::strcmp(argv[1],"--authorized-lifecycle") && std::strcmp(argv[1],"--authorized-watchdog") && std::strcmp(argv[1],"--authorized-private-io"))){
        std::fprintf(stderr,"No trace started: explicit human-authorized mode required\n");return 77;
    }
    const bool watchdog_test=std::strcmp(argv[1],"--authorized-watchdog")==0;
    const bool attribution=std::strcmp(argv[1],"--authorized-private-io")==0;
    wchar_t exe[32768]{};const auto size=GetModuleFileNameW(nullptr,exe,32768);
    if(!size || size>=32768)return 2;
    const auto path=std::wstring(exe,size)+L".result_"+std::to_wstring(GetCurrentProcessId())+L"_"+std::to_wstring(GetTickCount64())+L".txt";
    Handle evidence{CreateFileW(path.c_str(),GENERIC_WRITE,FILE_SHARE_READ,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr)};
    if(evidence.h==INVALID_HANDLE_VALUE)return 2;
    evidence_sink=evidence.h;
    write(evidence.h,attribution?"Manual private disk I/O probe; no ETL; only pinned own thread/window counts retained":"Manual ETW lifecycle probe; session=PCInsight Private Disk Test; no ETL, all events discarded");
    Capture capture;Handle pinned_thread;
    if(attribution){
        capture.thread=GetCurrentThreadId();pinned_thread.h=OpenThread(THREAD_QUERY_LIMITED_INFORMATION,FALSE,capture.thread);
        if(!pinned_thread.h || GetThreadId(pinned_thread.h)!=capture.thread || GetProcessIdOfThread(pinned_thread.h)!=GetCurrentProcessId())return 2;
        // This is the continuously executing main thread, alive until after
        // ProcessTrace joins; its TID cannot be reused during this experiment.
    }
    Privilege privilege;const auto privilege_error=privilege.enable();
    write(evidence.h,"Token-only SeSystemProfilePrivilege enable error="+std::to_string(privilege_error));
    WindowsDiskTrace trace({&StartTraceW,control});
    const auto start=trace.start();write(evidence.h,"start/verify error="+std::to_string(start));
    if(start){const auto r=trace.stop();write(evidence.h,"cleanup confirmed="+std::to_string(r.confirmed)+" stop="+std::to_string(r.stop_error)+" query="+std::to_string(r.query_error));return start==ERROR_ACCESS_DENIED||start==ERROR_PRIVILEGE_NOT_HELD?77:2;}
    std::atomic<TRACEHANDLE> consumer{INVALID_PROCESSTRACE_HANDLE};
    std::atomic<ULONG> consumed{ERROR_IO_PENDING},close_error{ERROR_SUCCESS};
    std::atomic<bool> watchdog_fired{false};
    std::thread worker,watchdog;
    bool capture_enabled=false,probe_error=false;
    std::mutex deadline_mutex;std::condition_variable deadline_cv;bool done=false;
    auto close_consumer=[&]{const auto h=consumer.exchange(INVALID_PROCESSTRACE_HANDLE);if(h!=INVALID_PROCESSTRACE_HANDLE)close_error=CloseTrace(h);};
    // Start watchdog immediately after acquiring the session and before opening
    // consumer/enabling events. STOP only uses our successful StartTrace handle.
    try {
        watchdog=std::thread([&]{std::unique_lock lock(deadline_mutex);
            if(!deadline_cv.wait_for(lock,std::chrono::milliseconds(watchdog_test?1500:10000),[&]{return done;})){
                watchdog_fired=true;lock.unlock();(void)trace.stop();close_consumer();
            }
        });
        EVENT_TRACE_LOGFILEW input{};input.LoggerName=const_cast<wchar_t*>(disk_trace_name);
        input.ProcessTraceMode=PROCESS_TRACE_MODE_REAL_TIME|PROCESS_TRACE_MODE_EVENT_RECORD|PROCESS_TRACE_MODE_RAW_TIMESTAMP;
        input.EventRecordCallback=attribution?record_private:discard;
        input.Context=&capture;
        const auto h=OpenTraceW(&input);
        if(h==INVALID_PROCESSTRACE_HANDLE)throw std::runtime_error("OpenTrace error="+std::to_string(GetLastError()));
        consumer=h;
        worker=std::thread([&,h]{TRACEHANDLE one=h;consumed=ProcessTrace(&one,1,nullptr,nullptr);});
        const auto enabled=trace.enable_disk();
        write(evidence.h,"enable disk/verify error="+std::to_string(enabled));
        if(enabled)throw std::runtime_error("Disk trace enable/verification failed");
        capture_enabled=true;
        if(attribution){
            private_io(path+L".private-io",capture);
            // Delivery is buffered: wait for positive read/write evidence,
            // bounded independently by the existing10s watchdog.
            for(unsigned i=0;i<60;++i){
                {std::lock_guard lock(capture.mutex);if(capture.totals.read_bytes>=1048576 && capture.totals.write_bytes>=1048576)break;}
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
            }
        }else std::this_thread::sleep_for(std::chrono::milliseconds(watchdog_test?2000:1000));
    }catch(const std::exception& error){probe_error=true;write(evidence.h,std::string("probe error: ")+error.what());}
    const auto stopped=trace.stop();close_consumer();
    {std::lock_guard lock(deadline_mutex);done=true;}deadline_cv.notify_all();
    if(watchdog.joinable())watchdog.join();
    if(worker.joinable())worker.join();
    write(evidence.h,"cleanup confirmed="+std::to_string(stopped.confirmed)+" stop="+std::to_string(stopped.stop_error)+" query="+std::to_string(stopped.query_error));
    write(evidence.h,"consumer result="+std::to_string(consumed.load())+" close="+std::to_string(close_error.load())+" watchdog="+std::to_string(watchdog_fired.load()));
    const auto process_error=consumed.load();const auto close=close_error.load();
    bool attribution_pass=true;
    if(attribution){
        const auto attributes=GetFileAttributesW((path+L".private-io").c_str());
        const bool removed=attributes==INVALID_FILE_ATTRIBUTES && GetLastError()==ERROR_FILE_NOT_FOUND;
        write(evidence.h,"Private disk totals read="+std::to_string(capture.totals.read_bytes)+" write="+std::to_string(capture.totals.write_bytes)+
            " reads="+std::to_string(capture.totals.reads)+" writes="+std::to_string(capture.totals.writes)+" schemaError="+std::to_string(capture.schema_error));
        write(evidence.h,"Private file removed="+std::to_string(removed)+" statisticsValid="+std::to_string(stopped.statistics_valid)+
            " eventsLost="+std::to_string(stopped.events_lost)+" realtimeBuffersLost="+std::to_string(stopped.realtime_buffers_lost)+" logBuffersLost="+std::to_string(stopped.log_buffers_lost));
        attribution_pass=removed && !capture.invalid && !capture.schema_error && capture.totals.read_bytes>=1048576 && capture.totals.write_bytes>=1048576 &&
            stopped.statistics_valid && !stopped.events_lost && !stopped.realtime_buffers_lost && !stopped.log_buffers_lost && !watchdog_fired;
    }
    const bool passed=attribution_pass && capture_enabled && !probe_error && stopped.confirmed && (process_error==ERROR_SUCCESS||process_error==ERROR_CANCELLED) &&
        (close==ERROR_SUCCESS||close==ERROR_CTX_CLOSE_PENDING) && (!watchdog_test||watchdog_fired.load());
    write(evidence.h,passed?(attribution?"Private pinned-thread disk I/O PASS (not general PID attribution)":"Lifecycle probe PASS (not disk attribution)"):"Probe FAIL; inspect cleanup before any further capture");
    return passed?0:2;
}
