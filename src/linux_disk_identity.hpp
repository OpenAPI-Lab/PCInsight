#pragma once
#include "pcinsight/types.hpp"
#include <string_view>
#include <functional>
namespace pcinsight::detail {
struct LinuxDiskIdentityInputs {
    Metric<std::string> subsystem, ata_vpd, characteristics_vpd, identification_vpd, nvme_transport;
};
LinuxDiskDetails parse_linux_disk_identity(const LinuxDiskIdentityInputs&);
// Internal fixture hook runs after capture and before identity/data revalidation.
Metric<LinuxDiskDetails> read_linux_disk_identity(const std::string& name,const std::string& root="/sys/block",const std::function<void()>& after_capture={});
void apply_linux_disk_identity(DiskInfo&,Metric<LinuxDiskDetails>);
}
