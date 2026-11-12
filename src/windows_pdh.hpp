#pragma once
#ifdef _WIN32
#include "pcinsight/types.hpp"
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <pdh.h>

namespace pcinsight::detail {
// Injectable native boundary, internal only; each instance owns its query.
struct PdhApi {
    decltype(&PdhOpenQueryW) open=&PdhOpenQueryW;
    decltype(&PdhAddEnglishCounterW) add=&PdhAddEnglishCounterW;
    decltype(&PdhCollectQueryData) collect=&PdhCollectQueryData;
    decltype(&PdhGetFormattedCounterValue) format=&PdhGetFormattedCounterValue;
    decltype(&PdhCloseQuery) close=&PdhCloseQuery;
};
class WindowsCpuCounter {
public:
    explicit WindowsCpuCounter(PdhApi api = {}) : api_(api) {}
    ~WindowsCpuCounter() { reset(); }
    WindowsCpuCounter(const WindowsCpuCounter&)=delete;
    WindowsCpuCounter& operator=(const WindowsCpuCounter&)=delete;
    Metric<double> sample(unsigned processors, unsigned groups);
    // Invalidate deltas when the caller changes collection strategy.
    void reset();
private:
    PdhApi api_;
    PDH_HQUERY query_{};
    PDH_HCOUNTER counter_{};
    bool primed_=false;
    unsigned processors_=0, groups_=0;
};
}
#endif
