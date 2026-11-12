#pragma once
#include "pcinsight/linux_drm.hpp"
#include <functional>
#include <tuple>
namespace pcinsight::detail {
class LinuxDrmMonitor {
public:
    using Discover=std::function<Metric<LinuxDrmDiscovery>(const LinuxDrmDiscoveryOptions&)>;
    explicit LinuxDrmMonitor(Discover discover=pcinsight::discover_linux_drm_clients):discover_(std::move(discover)){}
    void fill(SystemSnapshot&,const SampleOptions&);
    void reset(){sampler_.reset();limits_.reset();inventory_.clear();}
private:
    Discover discover_;
    LinuxDrmGpuSampler sampler_;
    std::optional<LinuxDrmDiscoveryOptions> limits_;
    std::vector<std::tuple<std::string,Support,std::string>> inventory_;
};
}
