#include "windows_process_io.hpp"
#include <array>
#include <cmath>
#include <cstring>
#include <stdexcept>
namespace pcinsight::detail {
namespace {
auto fields(const WindowsProcessIoCounters& c){return std::array<std::uint64_t,6>{c.read_operations,c.write_operations,c.other_operations,c.read_transfer_bytes,c.write_transfer_bytes,c.other_transfer_bytes};}
bool monotonic(const WindowsProcessIoCounters& a,const WindowsProcessIoCounters& b){const auto x=fields(a),y=fields(b);for(unsigned i=0;i<6;++i)if(y[i]<x[i])return false;return true;}
}
std::map<std::uint32_t,Metric<WindowsProcessIoDetails>> WindowsProcessIoTracker::sample(std::span<const WindowsProcessIoObservation> rows){
    try{
        if(rows.size()>100000)throw std::invalid_argument("Too many Windows process I/O observations");
        std::map<std::uint32_t,Metric<WindowsProcessIoDetails>> out;
        std::map<std::uint32_t,WindowsProcessIoObservation> next;
        for(const auto& row:rows){
            if(out.contains(row.pid))throw std::invalid_argument("Duplicate Windows process I/O PID");
            Metric<WindowsProcessIoDetails> result{{},row.counters.support==Support::permission_required?Support::permission_required:Support::unsupported,row.counters.note};
            if(row.pid && row.created && metric_is_known(row.counters)){
                WindowsProcessIoDetails details;details.counters=*row.counters.value;
                details.read_bytes_per_second.note=details.write_bytes_per_second.note=details.other_bytes_per_second.note=details.interval_seconds.note="Requires consecutive readable samples of the same PID/creation and increasing monotonic time";
                const auto old=previous_.find(row.pid);
                if(old!=previous_.end() && old->second.created==row.created && row.at>old->second.at && monotonic(*old->second.counters.value,*row.counters.value)){
                    const long double seconds=std::chrono::duration<long double>(row.at.time_since_epoch()).count()-std::chrono::duration<long double>(old->second.at.time_since_epoch()).count();
                    if(seconds>0 && std::isfinite(seconds)){
                        const auto& a=*old->second.counters.value;const auto& b=*row.counters.value;
                        auto rate=[&](std::uint64_t delta){return Metric<double>{static_cast<double>(static_cast<long double>(delta)/seconds),Support::estimated,"Process-wide general I/O byte delta / monotonic seconds, approximate; includes files/network/devices, NOT disk-only or network-only"};};
                        details.read_bytes_per_second=rate(b.read_transfer_bytes-a.read_transfer_bytes);details.write_bytes_per_second=rate(b.write_transfer_bytes-a.write_transfer_bytes);details.other_bytes_per_second=rate(b.other_transfer_bytes-a.other_transfer_bytes);
                        details.interval_seconds={static_cast<double>(seconds),Support::estimated,"Monotonic interval between process I/O captures; not a global snapshot interval"};
                    }
                }
                result={std::move(details),row.counters.support,"GetProcessIoCounters process-wide cumulative I/O (not job/children, disk-only or network-only)"};
                next.emplace(row.pid,row);
            }else if(result.note.empty())result.note="Windows process I/O identity/counters unavailable";
            out.emplace(row.pid,std::move(result));
        }
        previous_=std::move(next);return out;
    }catch(...){reset();throw;}
}
#ifdef _WIN32
namespace {
std::uint64_t ticks(FILETIME t){return (static_cast<std::uint64_t>(t.dwHighDateTime)<<32)|t.dwLowDateTime;}
WindowsProcessIoCounters convert(const IO_COUNTERS& c){return {c.ReadOperationCount,c.WriteOperationCount,c.OtherOperationCount,c.ReadTransferCount,c.WriteTransferCount,c.OtherTransferCount};}
DWORD last_error(){const auto error=GetLastError();return error?error:ERROR_GEN_FAILURE;}
}
WindowsProcessIoObservation read_windows_process_io(HANDLE handle,std::uint32_t pid,std::uint64_t created,WindowsProcessIoApi api){
    WindowsProcessIoObservation out;out.pid=pid;out.created=created;
    auto fail=[&](const char* note,DWORD error=ERROR_SUCCESS){out.counters={{},error==ERROR_ACCESS_DENIED?Support::permission_required:Support::unsupported,std::string(note)+"; Win32="+std::to_string(error)};return out;};
    if(!handle || handle==INVALID_HANDLE_VALUE || !pid || !created || !api.id || !api.times || !api.io || !api.now)return fail("Invalid process I/O handle/identity/API");
    auto identity=[&]()->DWORD{
        const auto actual=api.id(handle);if(!actual)return last_error();if(actual!=pid)return ERROR_INVALID_DATA;
        FILETIME born{},exit{},kernel{},user{};if(!api.times(handle,&born,&exit,&kernel,&user))return last_error();
        return ticks(born)==created && ticks(exit)==0?ERROR_SUCCESS:ERROR_INVALID_DATA;
    };
    auto error=identity();if(error!=ERROR_SUCCESS)return fail("Process identity not active/matching before I/O read",error);
    IO_COUNTERS first,second;std::memset(&first,0xa5,sizeof(first));std::memset(&second,0x5a,sizeof(second));
    if(!api.io(handle,&first)){error=last_error();return fail("GetProcessIoCounters first read failed",error);}
    if(!api.io(handle,&second)){error=last_error();return fail("GetProcessIoCounters second read failed",error);}
    out.at=api.now();error=identity();if(error!=ERROR_SUCCESS)return fail("Process identity changed/exited around I/O read",error);
    for(const auto value:fields(convert(first)))if(value==0xa5a5a5a5a5a5a5a5ULL)return fail("I/O output indistinguishable from unwritten first-read sentinel",ERROR_INVALID_DATA);
    for(const auto value:fields(convert(second)))if(value==0x5a5a5a5a5a5a5a5aULL)return fail("I/O output indistinguishable from unwritten second-read sentinel",ERROR_INVALID_DATA);
    if(!monotonic(convert(first),convert(second)))return fail("I/O counters regressed between native reads",ERROR_INVALID_DATA);
    out.counters={convert(second),Support::available,"GetProcessIoCounters uint64 process accounting; identity guarded, all I/O classes, no physical disk/network attribution"};return out;
}
#endif
}
