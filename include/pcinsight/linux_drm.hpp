#pragma once
#include "types.hpp"
#include <map>
#include <string_view>
namespace pcinsight {
// Exact driver/native-name taxonomy only. Unknown -> no value. No IO/GPU probing.
Metric<LinuxDrmEngineClass> classify_linux_drm_engine(std::string_view driver,std::string_view engine);
// Read-only /proc + DRM sysfs; no opening of GPU device, ioctls, or subprocess.
// Windows/unsupported drivers return no value. PID namespace is the caller's.
Metric<LinuxDrmClientUsage> read_linux_drm_client_usage(std::uint32_t pid, std::uint32_t descriptor);
// One client per sampler. First sample / identity change / reset needs a baseline.
// Not thread-safe. Shared descriptors with identical PCI/client ID must NOT be summed.
class LinuxDrmClientSampler {
public:
    Metric<LinuxDrmClientUsage> sample(std::uint32_t pid, std::uint32_t descriptor);
    void reset(){previous_.reset();}
private:
    std::optional<LinuxDrmClientUsage> previous_;
};
// Explicit caller-selected observations, not an all-process scan or hardware total.
// Deduplicates by full PCI identity/client-id. Invalid/conflicting batch resets state.
class LinuxDrmGpuSampler {
public:
    Metric<std::vector<LinuxDrmGpuAggregate>> sample(const std::vector<Metric<LinuxDrmClientUsage>>&);
    void reset(){previous_.clear();}
private:
    std::map<std::pair<std::string,std::uint64_t>,LinuxDrmClientUsage> previous_;
};
// Bounded scan of visible /proc/PID/fdinfo entries. No GPU open, root, subprocess,
// global coverage claim, or implicit change to Monitor. Pass observations to a
// persistent LinuxDrmGpuSampler; show the coverage/error counters alongside it.
Metric<LinuxDrmDiscovery> discover_linux_drm_clients(const LinuxDrmDiscoveryOptions& = {});
}
