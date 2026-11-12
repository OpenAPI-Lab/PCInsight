#pragma once
#include "pcinsight/linux_drm.hpp"
#include <functional>
#include <string_view>
namespace pcinsight::detail {
std::optional<LinuxDrmClientUsage> parse_linux_drm_fdinfo(std::string_view);
struct DrmClientReadApi {
    std::function<Metric<std::string>(const std::string&)> read;
    std::function<Metric<std::string>(std::uint32_t,std::uint32_t)> device_pci;
    std::function<std::chrono::steady_clock::time_point()> now;
};
Metric<LinuxDrmClientUsage> read_linux_drm_client_usage(std::uint32_t,std::uint32_t,const DrmClientReadApi&);
Metric<LinuxDrmClientUsage> update_linux_drm_client_usage(std::optional<LinuxDrmClientUsage>&,Metric<LinuxDrmClientUsage>);
}
