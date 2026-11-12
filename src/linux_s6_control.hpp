#pragma once
#include "pcinsight/linux_s6.hpp"
#include <string_view>
namespace pcinsight::detail {
// Private deterministic transport seam. Borrowed NONBLOCK WRONLY FIFO descriptor,
// caller serializes close/reuse. Only u/d/cru; delivery counts, NOT completion.
LinuxS6RequestResult write_s6_control_fd(int,std::string_view);
}
