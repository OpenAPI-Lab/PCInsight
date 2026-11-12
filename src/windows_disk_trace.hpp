#pragma once
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <evntrace.h>
#include <mutex>

namespace pcinsight::detail {
inline constexpr wchar_t disk_trace_name[] = L"PCInsight Private Disk Test";
// Component-owned, fixed ID; never use SystemTraceControlGuid / NT Kernel Logger.
inline constexpr GUID disk_trace_guid{0x3145964d,0xf922,0x4fe1,{0xb3,0xe3,0x21,0x63,0x29,0xbb,0xd4,0x6a}};
struct DiskTraceApi {
    decltype(&StartTraceW) start = &StartTraceW;
    decltype(&ControlTraceW) control = &ControlTraceW;
};
struct DiskTraceStop {
    ULONG stop_error = ERROR_SUCCESS;
    ULONG query_error = ERROR_SUCCESS;
    bool confirmed = false;
    bool statistics_valid = false;
    ULONG events_lost = 0, realtime_buffers_lost = 0, log_buffers_lost = 0;
};
// Private controller, NOT called by Monitor. Caller must explicitly authorize
// system-wide native tracing and attach a consumer before enable_disk().
class WindowsDiskTrace {
public:
    explicit WindowsDiskTrace(DiskTraceApi api = {}) : api_(api) {}
    ~WindowsDiskTrace();
    WindowsDiskTrace(const WindowsDiskTrace&) = delete;
    WindowsDiskTrace& operator=(const WindowsDiskTrace&) = delete;
    ULONG start();
    ULONG enable_disk();
    DiskTraceStop stop();
    bool active() const;
private:
    ULONG verify(ULONG flags);
    DiskTraceStop stop_locked();
    DiskTraceApi api_;
    mutable std::mutex mutex_;
    TRACEHANDLE owned_ = 0;
    bool attempted_ = false;
    bool enabled_ = false;
    DiskTraceStop stopped_{};
};
}
#endif
