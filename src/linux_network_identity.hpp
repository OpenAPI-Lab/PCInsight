#pragma once
#include "pcinsight/types.hpp"
#include "linux_gpu_names.hpp"
namespace pcinsight::detail {
void apply_linux_network_pci_names(LinuxNetworkDetails&,const PciNamesDatabase&);
Metric<LinuxNetworkDetails> read_linux_network_identity(const std::string& name,std::uint64_t expected_index,
    const PciNamesDatabase&,const std::string& root="/sys/class/net",
    const std::string& virtual_root="/sys/devices/virtual/net");
}
