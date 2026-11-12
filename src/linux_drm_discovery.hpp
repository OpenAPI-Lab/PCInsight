#pragma once
#include "pcinsight/linux_drm.hpp"
#include <functional>
namespace pcinsight::detail {
struct DrmDirectoryEntries {std::vector<std::uint32_t> ids;bool limit_reached{};};
struct DrmDiscoveryApi {
    std::function<Metric<DrmDirectoryEntries>(const std::string&,std::uint32_t)> list;
    std::function<Metric<bool>(std::uint32_t,std::uint32_t)> is_drm;
    std::function<Metric<LinuxDrmClientUsage>(std::uint32_t,std::uint32_t)> read_client;
    std::function<std::chrono::steady_clock::time_point()> now;
};
Metric<LinuxDrmDiscovery> discover_linux_drm_clients(const LinuxDrmDiscoveryOptions&,const DrmDiscoveryApi&);
}
