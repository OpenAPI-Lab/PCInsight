#pragma once
#include "pcinsight/linux_s6.hpp"
namespace pcinsight::detail {
// Internal relative reader on borrowed pinned directory; no global FD override.
Metric<LinuxS6StatusRecord> read_linux_s6_record_at(int,const std::string&);
}
