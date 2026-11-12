#pragma once
#include "pcinsight/linux_openrc_services.hpp"
namespace pcinsight::detail {
Metric<LinuxOpenrcServiceInventory> project_openrc_services(Metric<LinuxOpenrcCatalog>,Metric<LinuxOpenrcMarkerInventory>);
}
