#pragma once
#include "pcinsight/linux_s6_services.hpp"
namespace pcinsight::detail {
Metric<LinuxS6ServiceInventory> project_s6_services(Metric<LinuxS6Inventory>);
}
