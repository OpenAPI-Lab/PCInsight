#include "windows_disk_trace.hpp"
#ifdef _WIN32
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace pcinsight::detail {
namespace {
constexpr ULONG modes = EVENT_TRACE_SYSTEM_LOGGER_MODE | EVENT_TRACE_REAL_TIME_MODE |
                        EVENT_TRACE_NO_PER_PROCESSOR_BUFFERING | EVENT_TRACE_STOP_ON_HYBRID_SHUTDOWN;
struct Properties {
    EVENT_TRACE_PROPERTIES p{};
    wchar_t name[1024]{};
    wchar_t file[1024]{};
    explicit Properties(bool query = false) {
        p.Wnode.BufferSize = sizeof(*this);
        p.Wnode.Flags = WNODE_FLAG_TRACED_GUID;
        p.Wnode.Guid = disk_trace_guid;
        p.LoggerNameOffset = offsetof(Properties, name);
        if (query) p.LogFileNameOffset = offsetof(Properties, file);
    }
};
bool same_guid(const GUID& a, const GUID& b) { return std::memcmp(&a,&b,sizeof(a)) == 0; }
}
WindowsDiskTrace::~WindowsDiskTrace() { (void)stop(); }
bool WindowsDiskTrace::active() const { std::lock_guard lock(mutex_); return owned_ != 0; }
ULONG WindowsDiskTrace::verify(ULONG flags) {
    Properties result(true);
    const auto error = api_.control(owned_,nullptr,&result.p,EVENT_TRACE_CONTROL_QUERY);
    if (error) return error;
    const auto& p = result.p;
    // Check both the accepted ceiling and current allocation, not just what we
    // requested. Inconsistent output is not an invitation to enlarge quotas.
    if (!same_guid(p.Wnode.Guid,disk_trace_guid) || p.EnableFlags != flags ||
        p.LogFileMode != modes || result.file[0] || p.BufferSize < 4 ||
        !p.MaximumBuffers || !p.NumberOfBuffers || p.NumberOfBuffers > p.MaximumBuffers ||
        std::uint64_t(p.BufferSize)*p.MaximumBuffers > 64u*1024u)
        return ERROR_INVALID_DATA;
    return ERROR_SUCCESS;
}
ULONG WindowsDiskTrace::start() {
    std::lock_guard lock(mutex_);
    if (attempted_ || !api_.start || !api_.control) return ERROR_INVALID_PARAMETER;
    attempted_ = true;
    Properties request;
    request.p.Wnode.ClientContext = 1; // QPC clock; no wall-clock adjustments.
    request.p.BufferSize = 64; // KiB
    request.p.MinimumBuffers = 4;
    request.p.MaximumBuffers = 64; // 4MiB requested, at most64MiB accepted.
    request.p.FlushTimer = 1;
    request.p.LogFileMode = modes;
    // No disk/process/thread events until an explicit consumer is ready.
    TRACEHANDLE created = 0;
    const auto error = api_.start(&created,disk_trace_name,&request.p);
    if (error) return error; // Especially ALREADY_EXISTS: never adopt or stop it.
    if (!created) return ERROR_INVALID_HANDLE; // Native API contract violation.
    owned_ = created;
    const auto checked = verify(0);
    if (checked) (void)stop_locked();
    return checked;
}
ULONG WindowsDiskTrace::enable_disk() {
    std::lock_guard lock(mutex_);
    if (!owned_ || enabled_) return ERROR_INVALID_PARAMETER;
    Properties update;
    update.p.EnableFlags = EVENT_TRACE_FLAG_DISK_IO;
    // UPDATE with LogFileMode=0 disables real-time delivery (not "unchanged").
    // With no ETL output that would leave this session without a destination.
    update.p.LogFileMode = EVENT_TRACE_REAL_TIME_MODE;
    const auto error = api_.control(owned_,nullptr,&update.p,EVENT_TRACE_CONTROL_UPDATE);
    if (error) { (void)stop_locked(); return error; }
    enabled_ = true;
    const auto checked = verify(EVENT_TRACE_FLAG_DISK_IO);
    if (checked) (void)stop_locked();
    return checked;
}
DiskTraceStop WindowsDiskTrace::stop_locked() {
    if (!owned_) return stopped_;
    Properties request;
    stopped_.stop_error = api_.control(owned_,nullptr,&request.p,EVENT_TRACE_CONTROL_STOP);
    stopped_.statistics_valid = stopped_.stop_error == ERROR_SUCCESS;
    if(stopped_.statistics_valid){
        stopped_.events_lost=request.p.EventsLost;
        stopped_.realtime_buffers_lost=request.p.RealTimeBuffersLost;
        stopped_.log_buffers_lost=request.p.LogBuffersLost;
    }
    // Never fall back to STOP by name: it could now identify somebody else's
    // session. Query by name is read-only and absence confirms removal.
    Properties query(true);
    stopped_.query_error = api_.control(0,disk_trace_name,&query.p,EVENT_TRACE_CONTROL_QUERY);
    stopped_.confirmed = stopped_.query_error == ERROR_WMI_INSTANCE_NOT_FOUND;
    // One STOP attempt only. Even if confirmation fails, do not retry an old
    // handle after the session may have stopped and its slot been reused.
    owned_ = 0;
    return stopped_;
}
DiskTraceStop WindowsDiskTrace::stop() { std::lock_guard lock(mutex_); return stop_locked(); }
}
#endif
