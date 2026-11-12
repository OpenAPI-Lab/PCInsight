#include "windows_processes.hpp"
#include "windows_version.hpp"
#include "windows_process_snapshot.hpp"
#include "windows_efficiency.hpp"
#include <cmath>
#include <limits>

namespace pcinsight::detail {
Metric<double> process_cpu_delta(const ProcessCpuSample& previous,const ProcessCpuSample& current) {
    Metric<double> result{std::nullopt,Support::unsupported,"Requires consecutive samples of the same process and CPU topology"};
    if (!current.created || previous.created!=current.created || !current.processors ||
        previous.processors!=current.processors || current.ticks<previous.ticks) return result;
    const auto elapsed=std::chrono::duration<double>(current.captured-previous.captured).count();
    if (elapsed<=0) return result;
    const double value=100.0*double(current.ticks-previous.ticks)/1e7/elapsed/current.processors;
    if (!std::isfinite(value) || value<0 || value>100) return result;
    return {value,Support::available,"Process CPU time / wall time / all logical CPUs; not frequency-weighted utility"};
}
}

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <tlhelp32.h>
#include <psapi.h>
#include <cstring>
#include <cwchar>
#include <system_error>

namespace pcinsight::detail {
namespace {
struct Handle {
    HANDLE value;
    explicit Handle(HANDLE v) : value(v) {}
    ~Handle() { if (value && value!=INVALID_HANDLE_VALUE) CloseHandle(value); }
    Handle(const Handle&)=delete;
    Handle& operator=(const Handle&)=delete;
};
std::uint64_t ticks(FILETIME time) {
    return (std::uint64_t(time.dwHighDateTime)<<32)|time.dwLowDateTime;
}
std::string utf8(const wchar_t* text,int length) {
    const auto size=WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,text,length,nullptr,0,nullptr,nullptr);
    if (size<=0) return {};
    std::string result(size,'\0');
    if(WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,text,length,result.data(),size,nullptr,nullptr)!=size)return {};
    return result;
}
std::string username(HANDLE process,Metric<std::uint32_t>& session) {
    HANDLE raw=nullptr;
    if (!OpenProcessToken(process,TOKEN_QUERY,&raw)) {
        const auto error=GetLastError();
        session={{},error==ERROR_ACCESS_DENIED?Support::permission_required:Support::unsupported,"OpenProcessToken failed; Win32 error="+std::to_string(error)};
        return {};
    }
    Handle token(raw);
    DWORD id=0,returned=0;
    const auto queried=GetTokenInformation(raw,TokenSessionId,&id,sizeof(id),&returned);
    const auto query_error=queried?ERROR_INVALID_DATA:GetLastError();
    if(queried && returned==sizeof(id) && id!=0xffffffffU)
        session={id,Support::available,"Primary process token Terminal Services session ID; not stable against session reuse"};
    else {
        session={{},query_error==ERROR_ACCESS_DENIED?Support::permission_required:Support::unsupported,"TokenSessionId unavailable or invalid; Win32 error="+std::to_string(query_error)};
    }
    DWORD bytes=0;
    GetTokenInformation(raw,TokenUser,nullptr,0,&bytes);
    if (!bytes || bytes>1024*1024) return {};
    std::vector<unsigned char> buffer(bytes);
    if (!GetTokenInformation(raw,TokenUser,buffer.data(),bytes,&bytes)) return {};
    const auto sid=reinterpret_cast<TOKEN_USER*>(buffer.data())->User.Sid;
    DWORD name_size=0,domain_size=0; SID_NAME_USE kind{};
    LookupAccountSidW(nullptr,sid,nullptr,&name_size,nullptr,&domain_size,&kind);
    if (!name_size || name_size>32768 || domain_size>32768) return {};
    std::vector<wchar_t> name(name_size),domain(domain_size+1);
    if (!LookupAccountSidW(nullptr,sid,name.data(),&name_size,domain.data(),&domain_size,&kind)) return {};
    const auto user=utf8(name.data(),static_cast<int>(name_size));
    return domain_size ? utf8(domain.data(),static_cast<int>(domain_size))+"\\"+user : user;
}
std::string architecture(HANDLE process) {
    using IsWow64Process2Fn=BOOL (WINAPI*)(HANDLE,USHORT*,USHORT*);
    static const auto fn=[] {
        IsWow64Process2Fn p=nullptr;
        const auto module=GetModuleHandleW(L"kernel32.dll");
        if (module) {
            const auto symbol=GetProcAddress(module,"IsWow64Process2");
            static_assert(sizeof(p)==sizeof(symbol));
            std::memcpy(&p,&symbol,sizeof(p));
        }
        return p;
    }();
    USHORT machine=0,native=0;
    if (!fn || !fn(process,&machine,&native)) return {};
    if (machine==IMAGE_FILE_MACHINE_UNKNOWN) machine=native;
    switch(machine) {
        case IMAGE_FILE_MACHINE_AMD64: return "x64";
        case IMAGE_FILE_MACHINE_I386: return "x86";
        case IMAGE_FILE_MACHINE_ARM64: return "ARM64";
        case IMAGE_FILE_MACHINE_ARMNT: return "ARM32";
        default: return {};
    }
}
}
std::vector<ProcessInfo> WindowsProcesses::sample() {
    FILETIME snapshot_started{};
    GetSystemTimeAsFileTime(&snapshot_started);
    std::vector<PROCESSENTRY32W> entries;
    try { entries = read_windows_process_snapshot(); }
    catch (...) { reset(); throw; }
    std::vector<ProcessInfo> out;
    std::vector<WindowsProcessIoObservation> io_observations;
    std::map<std::uint32_t,ProcessCpuSample> current;
    std::map<std::wstring,std::string> descriptions;
    const auto cpus=GetActiveProcessorCount(ALL_PROCESSOR_GROUPS);
    for (const auto& entry : entries) {
        ProcessInfo p;
        p.pid=entry.th32ProcessID; p.parent_pid=entry.th32ParentProcessID;
        p.name=utf8(entry.szExeFile,static_cast<int>(std::wcslen(entry.szExeFile)));
        p.thread_count=entry.cntThreads;
        p.status="unknown";
        p.windows_io.note="Process I/O identity unavailable; no disk/network proxy";
        WindowsProcessIoObservation io_observation;io_observation.pid=p.pid;
        p.cpu_percent.note="Requires two accessible process samples";
        p.disk_bytes_per_second.note="Disk attribution not implemented; generic process I/O is not disk-only";
        p.network_bytes_per_second.note="Per-process network accounting not implemented";
        p.efficiency_mode_available.note="Process efficiency capability not yet queried";
        HANDLE raw=OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION|PROCESS_VM_READ,FALSE,p.pid);
        if (!raw) raw=OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION,FALSE,p.pid);
        const DWORD open_error=raw ? ERROR_SUCCESS : GetLastError();
        Handle process(raw);
        if (!raw) {
            const auto support=open_error==ERROR_ACCESS_DENIED ? Support::permission_required : Support::unsupported;
            const auto note="OpenProcess failed; Win32 error="+std::to_string(open_error);
            p.cpu_percent={std::nullopt,support,note};
            p.memory_bytes={std::nullopt,support,note};
            p.windows_creation_time_100ns={std::nullopt,support,note};
            p.windows_session_id={std::nullopt,support,note};
            p.windows_cpu_time_100ns={std::nullopt,support,note};
            p.efficiency_mode_available={std::nullopt,support,note};
            p.efficiency_mode_requested={std::nullopt,support,note};
            io_observation.counters={std::nullopt,support,note};
        } else {
            FILETIME created{},exited{},kernel{},user{};
            if (GetProcessTimes(raw,&created,&exited,&kernel,&user)) {
                // A PID reused after enumeration began must not combine old
                // Toolhelp metadata with a newly opened process's measurements.
                if (ticks(created)>ticks(snapshot_started)) {
                    p.cpu_percent.note="Process created after snapshot began; retry next sample";
                    p.windows_cpu_time_100ns.note=p.cpu_percent.note;
                    out.push_back(std::move(p));
                    continue;
                }
                if (ticks(created)) p.windows_creation_time_100ns={ticks(created),Support::available,
                    "Native Windows creation FILETIME; retain with PID for checked actions"};
                // Active means not exited, NOT that every thread is scheduled/running.
                p.status=ticks(exited)?"exited":"active";
                if(ticks(kernel)>std::numeric_limits<std::uint64_t>::max()-ticks(user)){
                    p.windows_cpu_time_100ns.note="Native process CPU sum overflow";
                    p.cpu_percent.note=p.windows_cpu_time_100ns.note;
                }else{
                const auto total=ticks(kernel)+ticks(user);
                p.windows_cpu_time_100ns={total,Support::available,"GetProcessTimes cumulative kernel+user duration in 100ns units, summed across threads"};
                ProcessCpuSample sample{ticks(created),total,
                    std::chrono::steady_clock::now(),cpus};
                const auto found=previous_.find(p.pid);
                if (found!=previous_.end()) p.cpu_percent=process_cpu_delta(found->second,sample);
                if (p.status=="active") current.emplace(p.pid,sample);
                }
            } else {
                const auto error=GetLastError();
                p.cpu_percent={std::nullopt,error==ERROR_ACCESS_DENIED?Support::permission_required:Support::unsupported,
                    "GetProcessTimes failed; Win32 error="+std::to_string(error)};
                p.windows_creation_time_100ns={std::nullopt,p.cpu_percent.support,p.cpu_percent.note};
                p.windows_cpu_time_100ns={std::nullopt,p.cpu_percent.support,p.cpu_percent.note};
            }
            std::vector<wchar_t> path(32768); DWORD length=static_cast<DWORD>(path.size());
            if(metric_is_known(p.windows_creation_time_100ns))io_observation=read_windows_process_io(raw,p.pid,*p.windows_creation_time_100ns.value);
            else io_observation.counters={{},p.windows_creation_time_100ns.support,"Creation identity unavailable for Windows process I/O"};
            if (QueryFullProcessImageNameW(raw,0,path.data(),&length)) {
                p.executable=utf8(path.data(),static_cast<int>(length));
                const std::wstring executable(path.data(),length);
                auto found=descriptions.find(executable);
                if (found==descriptions.end()) {
                    const auto cached=descriptions_.find(executable);
                    found=descriptions.emplace(executable,cached!=descriptions_.end()
                        ? cached->second : windows_file_description(executable)).first;
                }
                p.description=found->second;
            }
            p.username=username(raw,p.windows_session_id);
            p.architecture=architecture(raw);
            auto efficiency = read_windows_efficiency(raw);
            p.efficiency_mode_available = std::move(efficiency.available);
            p.efficiency_mode_requested = std::move(efficiency.requested);
            PROCESS_MEMORY_COUNTERS memory{};
            if (GetProcessMemoryInfo(raw,&memory,sizeof(memory)))
                p.memory_bytes={static_cast<std::uint64_t>(memory.WorkingSetSize),Support::available,
                    "Resident working set including shared pages; not private working set"};
            else {
                const auto error=GetLastError();
                p.memory_bytes={std::nullopt,error==ERROR_ACCESS_DENIED?Support::permission_required:Support::unsupported,
                    "GetProcessMemoryInfo failed; Win32 error="+std::to_string(error)};
            }
        }
        io_observations.push_back(std::move(io_observation));
        out.push_back(std::move(p));
    }
    const auto io=io_.sample(io_observations);
    for(auto& process:out){const auto found=io.find(process.pid);if(found!=io.end())process.windows_io=found->second;}
    previous_=std::move(current); // drop exited/inaccessible PIDs and bound storage
    descriptions_=std::move(descriptions);
    return out;
}
}
#endif
