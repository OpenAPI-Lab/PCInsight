#pragma once
#include "pcinsight/types.hpp"
#include <string_view>
namespace pcinsight::detail {
struct PciNamesDatabase {
    std::string text, source;
};
// Bounded, regular-file-only native loader. No subprocess or network access.
PciNamesDatabase load_linux_pci_names();
PciNamesDatabase load_linux_pci_names(const std::vector<std::string>& paths);
void fill_linux_gpu_names(std::vector<GpuInfo>&, const PciNamesDatabase&);
}
