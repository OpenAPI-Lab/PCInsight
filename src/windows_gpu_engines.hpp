#pragma once
#ifdef _WIN32
#include "windows_gpu_memory.hpp"
namespace pcinsight::detail {
class WindowsGpuEngines {
public:
    explicit WindowsGpuEngines(GpuMemoryPdhApi api={}) : api_(api) {}
    ~WindowsGpuEngines(){reset();}
    WindowsGpuEngines(const WindowsGpuEngines&)=delete;
    WindowsGpuEngines& operator=(const WindowsGpuEngines&)=delete;
    void fill(std::vector<GpuInfo>&);
    void reset();
private:
    GpuMemoryPdhApi api_;
    PDH_HQUERY query_{};PDH_HCOUNTER counter_{};bool primed_{};
    std::set<std::string> identities_;
};
}
#endif
