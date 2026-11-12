#pragma once
#include "pcinsight/types.hpp"
#include <map>
#include <span>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif
namespace pcinsight::detail {
struct WindowsProcessIoObservation {
    std::uint32_t pid{};
    std::uint64_t created{};
    std::chrono::steady_clock::time_point at;
    Metric<WindowsProcessIoCounters> counters;
};
class WindowsProcessIoTracker {
public:
    std::map<std::uint32_t,Metric<WindowsProcessIoDetails>> sample(std::span<const WindowsProcessIoObservation>);
    void reset(){previous_.clear();}
private:
    std::map<std::uint32_t,WindowsProcessIoObservation> previous_;
};
#ifdef _WIN32
struct WindowsProcessIoApi {
    decltype(&GetProcessId) id=&GetProcessId;
    decltype(&GetProcessTimes) times=&GetProcessTimes;
    decltype(&GetProcessIoCounters) io=&GetProcessIoCounters;
    std::chrono::steady_clock::time_point (*now)()=std::chrono::steady_clock::now;
};
// Borrowed pinned handle; never opens, closes or changes a process/privilege.
WindowsProcessIoObservation read_windows_process_io(HANDLE,std::uint32_t,std::uint64_t,WindowsProcessIoApi={});
#endif
}
