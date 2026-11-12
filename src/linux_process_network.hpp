#pragma once
#include "pcinsight/linux_process_network.hpp"
#include "linux_network_multi.hpp"
namespace pcinsight::detail {
// Pure checked projection. No syscalls, capture, ID conversion or legacy totals.
Metric<LinuxProcessNetworkSnapshot> publish_linux_process_network(const PrivateCommonNetworkWindow&);
}
