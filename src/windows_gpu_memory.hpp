#pragma once
#ifdef _WIN32
#include "windows_pdh.hpp"
#include <array>
#include <set>
namespace pcinsight::detail {
struct GpuMemoryPdhApi {
    decltype(&PdhOpenQueryW) open=&PdhOpenQueryW;
    decltype(&PdhAddEnglishCounterW) add=&PdhAddEnglishCounterW;
    decltype(&PdhCollectQueryData) collect=&PdhCollectQueryData;
    decltype(&PdhGetFormattedCounterArrayW) format=&PdhGetFormattedCounterArrayW;
    decltype(&PdhCloseQuery) close=&PdhCloseQuery;
};
class WindowsGpuMemory {
public:
    explicit WindowsGpuMemory(GpuMemoryPdhApi api={}) : api_(api) {}
    ~WindowsGpuMemory(){reset();}
    WindowsGpuMemory(const WindowsGpuMemory&)=delete;
    WindowsGpuMemory& operator=(const WindowsGpuMemory&)=delete;
    void fill(std::vector<GpuInfo>&);
    void reset();
private:
    struct Slot {PDH_HQUERY query{};PDH_HCOUNTER counter{};bool primed{};};
    void reset(Slot&);
    GpuMemoryPdhApi api_;
    std::array<Slot,2> slots_{};
    std::set<std::string> identities_;
};
}
#endif
