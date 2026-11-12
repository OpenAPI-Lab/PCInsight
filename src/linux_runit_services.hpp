#pragma once
#include "pcinsight/linux_runit_services.hpp"
namespace pcinsight::detail {
Metric<LinuxRunitServiceInventory> project_runit_services(Metric<LinuxRunitInventory>);
}
