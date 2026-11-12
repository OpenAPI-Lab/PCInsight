#pragma once
#ifdef _WIN32
#include "windows_pdh.hpp"
#include <array>
#include <map>
namespace pcinsight::detail {
struct DiskPdhApi {
    decltype(&PdhOpenQueryW) open=&PdhOpenQueryW;
    decltype(&PdhAddEnglishCounterW) add=&PdhAddEnglishCounterW;
    decltype(&PdhCollectQueryData) collect=&PdhCollectQueryData;
    decltype(&PdhGetFormattedCounterArrayW) format=&PdhGetFormattedCounterArrayW;
    decltype(&PdhCloseQuery) close=&PdhCloseQuery;
};
class WindowsDiskPerformance {
public:
    explicit WindowsDiskPerformance(DiskPdhApi api={}) : api_(api) {}
    ~WindowsDiskPerformance() { reset(); }
    WindowsDiskPerformance(const WindowsDiskPerformance&)=delete;
    WindowsDiskPerformance& operator=(const WindowsDiskPerformance&)=delete;
    void fill(std::vector<DiskInfo>&,const std::map<std::uint32_t,std::string>&);
    void reset();
private:
    DiskPdhApi api_;
    PDH_HQUERY query_{};
    std::array<PDH_HCOUNTER,4> counters_{};
    bool primed_=false;
    std::map<std::uint32_t,std::string> identities_;
};
}
#endif
