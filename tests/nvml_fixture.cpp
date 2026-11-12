// Synthetic driver library ONLY used by CTest, never installed with PCInsight.
#include "../src/linux_nvml.hpp"
#include <cstdio>
#include <cstring>
using namespace pcinsight::detail;
namespace { unsigned sessions = 0; }
extern "C" {
int nvmlInit_v2() { ++sessions; return 0; }
int nvmlShutdown() { if (!sessions) return 1; --sessions; return 0; }
int nvmlDeviceGetCount_v2(unsigned* n) { if (!sessions) return 1; *n = 2; return 0; }
int nvmlDeviceGetHandleByIndex_v2(unsigned i, NvmlDevice* d) {
    if (!sessions) return 1;
    if (i >= 2) return 2;
    *d = reinterpret_cast<void*>(static_cast<std::uintptr_t>(i + 1)); return 0;
}
int nvmlDeviceGetName(NvmlDevice d, char* b, unsigned n) {
    std::snprintf(b, n, "Synthetic NVIDIA %zu", reinterpret_cast<std::uintptr_t>(d)); return 0;
}
int nvmlDeviceGetUUID(NvmlDevice d, char* b, unsigned n) {
    std::snprintf(b, n, "GPU-synthetic-%zu", reinterpret_cast<std::uintptr_t>(d)); return 0;
}
int nvmlSystemGetDriverVersion(char* b, unsigned n) { std::snprintf(b, n, "synthetic.1"); return 0; }
int nvmlDeviceGetPciInfo_v3(NvmlDevice d, NvmlPci* p) {
    std::memset(p, 0, sizeof(*p));
    p->bus = static_cast<unsigned>(reinterpret_cast<std::uintptr_t>(d));
    std::snprintf(p->bus_id, sizeof(p->bus_id), "00000000:%02x:00.0", p->bus); return 0;
}
int nvmlDeviceGetMemoryInfo(NvmlDevice, NvmlMemory* m) { *m = {4096, 3072, 1024}; return 0; }
int nvmlDeviceGetUtilizationRates(NvmlDevice, NvmlUtilization* u) { *u = {25, 50}; return 0; }
int nvmlDeviceGetTemperature(NvmlDevice, unsigned sensor, unsigned* t) { if (sensor != 0) return 2; *t = 42; return 0; }
}
