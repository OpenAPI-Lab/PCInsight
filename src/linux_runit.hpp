#pragma once
#include "pcinsight/linux_runit.hpp"
#include <string_view>
#include <functional>
namespace pcinsight::detail {
// Private transport helper; borrowed nonblocking writer fd, caller serializes
// close/reuse. FIFO only, commands limited to u/d/tcu. No process/state proof.
LinuxRunitRequestResult write_runit_control_fd(int,std::string_view);
// Per-instance deterministic seam only; no global override. Callbacks may not
// reenter the owner. Request must return delivered byte counts without later throw.
struct RunitTransitionApi {
    std::function<Metric<LinuxRunitStatus>()> read;
    std::function<LinuxRunitRequestResult(ServiceAction,bool)> request;
    std::function<std::chrono::steady_clock::time_point()> now;
};
struct RunitTransitionAccess {static LinuxRunitTransition make(RunitTransitionApi);};
}
