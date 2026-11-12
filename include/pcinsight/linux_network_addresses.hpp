#pragma once
#include "metric.hpp"
#include <cstdint>
#include <string>
#include <vector>

namespace pcinsight {
struct LinuxInterfaceAddressList {
    std::uint32_t interface_index{};
    std::vector<std::string> ipv4, ipv6;
};
// Current process network namespace, only indices with reported addresses.
// Available + empty vector means a complete empty dump, not failed collection.
// Interface indices can be reused; this is not a persistent identity or a
// transaction with Monitor snapshots. Never merge namespaces by index alone.
// No getifaddrs fallback: incomplete/unavailable dumps remain unknown.
Metric<std::vector<LinuxInterfaceAddressList>> query_linux_network_addresses();
}
