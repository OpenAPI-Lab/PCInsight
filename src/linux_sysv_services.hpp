#pragma once
#include "pcinsight/linux_sysv_services.hpp"
namespace pcinsight::detail {
// Pure projection of a same-call described result. Private fixture seam only.
Metric<LinuxSysVServiceInventory> project_sysv_services(Metric<LinuxSysVDescribedCatalog>);
}
