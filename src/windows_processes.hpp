#pragma once
#include "pcinsight/types.hpp"
#include "windows_process_io.hpp"
#include <map>

namespace pcinsight::detail {
struct ProcessCpuSample {
    std::uint64_t created=0, ticks=0;
    std::chrono::steady_clock::time_point captured;
    unsigned processors=0;
};
Metric<double> process_cpu_delta(const ProcessCpuSample& previous,const ProcessCpuSample& current);
#ifdef _WIN32
class WindowsProcesses {
public:
    std::vector<ProcessInfo> sample();
    void reset() { previous_.clear(); descriptions_.clear(); io_.reset(); }
private:
    std::map<std::uint32_t,ProcessCpuSample> previous_;
    std::map<std::wstring,std::string> descriptions_;
    WindowsProcessIoTracker io_;
};
#endif
}
